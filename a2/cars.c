#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "traffic.h"

extern struct intersection isection;
extern struct car *in_cars[];
extern struct car *out_cars[];

/**
 * Populate the car lists by parsing a file where each line has
 * the following structure:
 *
 * <id> <in_direction> <out_direction>
 *
 * Each car is added to the list that corresponds with 
 * its in_direction
 * 
 * Note: this also updates 'inc' on each of the lanes
 */
void parse_schedule(char *file_name) {
	int id;
	struct car *cur_car;
	enum direction in_dir, out_dir;
	FILE *f = fopen(file_name, "r");

	/* parse file */
	while (fscanf(f, "%d %d %d", &id, (int*)&in_dir, (int*)&out_dir) == 3) {
		printf("Car %d going from %d to %d\n", id, in_dir, out_dir);
		/* construct car */
		cur_car = malloc(sizeof(struct car));
		cur_car->id = id;
		cur_car->in_dir = in_dir;
		cur_car->out_dir = out_dir;

		/* append new car to head of corresponding list */
		cur_car->next = in_cars[in_dir];
		in_cars[in_dir] = cur_car;
		isection.lanes[in_dir].inc++;
	}

	fclose(f);
}

/**
 * TODO: Fill in this function
 *
 * Do all of the work required to prepare the intersection
 * before any cars start coming
 * 
 */
void init_intersection() {

	int i;

	// initializes the locks for each quadrant
	for (i = 0; i < 4; i++) {
		pthread_mutex_init(&isection.quad[i], NULL);
	}

	// initializes the struct lane array (lanes)
	for (i = 0; i < 4; i++) {
		pthread_mutex_init(&isection.lanes[i].lock, NULL);
		pthread_cond_init(&isection.lanes[i].producer_cv, NULL);
		pthread_cond_init(&isection.lanes[i].consumer_cv, NULL);

		isection.lanes[i].inc = 0;
		isection.lanes[i].passed = 0;
		
		isection.lanes[i].buffer = malloc(sizeof(struct car *) * LANE_LENGTH);
		isection.lanes[i].head = 0;
		isection.lanes[i].tail = 0;
		
		isection.lanes[i].capacity = LANE_LENGTH;
		isection.lanes[i].in_buf = 0;
	}

}

/**
 * TODO: Fill in this function
 *
 * Populates the corresponding lane with cars as room becomes
 * available. Ensure to notify the cross thread as new cars are
 * added to the lane.
 * 
 */
void *car_arrive(void *arg) {
	/* THIS IS THE PRODUCER */
	struct lane *l = &(isection.lanes[*(int*) arg]);
	struct car *current_car;

	for (current_car = in_cars[*(int*) arg]; current_car != NULL; current_car = current_car->next) {
		pthread_mutex_lock(&l->lock);

		while (l->in_buf == l->capacity) {
			// producer waits on consumer for space in the lane
			pthread_cond_wait(&l->consumer_cv, &l->lock);
		}

		// Adds current_car to the appropriate lane
		l->buffer[l->tail] = current_car;
		l->tail = (l->tail + 1) % LANE_LENGTH;
		l->in_buf++;

		pthread_cond_signal(&l->producer_cv);
		pthread_mutex_unlock(&l->lock);
	}

	return NULL;
}

/**
 * TODO: Fill in this function
 *
 * Moves cars from a single lane across the intersection. Cars
 * crossing the intersection must abide the rules of the road
 * and cross along the correct path. Ensure to notify the
 * arrival thread as room becomes available in the lane.
 *
 * Note: After crossing the intersection the car should be added
 * to the out_cars list that corresponds to the car's out_dir
 * 
 * Note: For testing purposes, each car which gets to cross the 
 * intersection should print the following three numbers on a 
 * new line, separated by spaces:
 *  - the car's 'in' direction, 'out' direction, and id.
 * 
 * You may add other print statements, but in the end, please 
 * make sure to clear any prints other than the one specified above, 
 * before submitting your final code. 
 */
void *car_cross(void *arg) {
	/* THIS IS THE CONSUMER */
	struct lane *l = &(isection.lanes[*(int*) arg]);

	if (l->head < l->tail) {
		pthread_mutex_lock(&l->lock);
		int i;

		while (l->head < l->tail) {
			struct car *current_car = l->buffer[l->head];

			while (l->in_buf == 0) {
				// consumer waits on producer if no cars in lane
				pthread_cond_wait(&l->producer_cv, &l->lock);
			}

			// computes path for current_car
			int *path = compute_path(current_car->in_dir, current_car->out_dir);

			// acquires locks for the appropriate quadrants in the computed path
			for (i = 0; i < 4; i++) {
				if (path[i] != -1) {
					pthread_mutex_lock(&isection.quad[i]);
				}
			}

			// adds car to the beginning of the linked list in out_car
			// corresponding to its out_dir,
			// and increments/decrements necessary variables
			current_car->next = out_cars[current_car->out_dir];
			out_cars[current_car->out_dir] = current_car;

			l->head = (l->head + 1) % LANE_LENGTH;
			l->passed++;
			l->in_buf--;

			printf("%d %d %d\n", current_car->in_dir, current_car->out_dir, current_car->id);

			// releases locks for the acquired quadrants in the computed path
			for (i = 3; i >= 0; i--) {
				if (path[i] != -1) {
					pthread_mutex_unlock(&isection.quad[i]);
				}
			}

			pthread_cond_signal(&l->consumer_cv);
		}

		pthread_mutex_unlock(&l->lock);
	}
	
	return NULL;
}



/* ============================================ */
/* ===== HELPER METHOD FOR INT COMPARISON ===== */
/* ============================================ */
int compare_num(const void *a, const void *b) {
	return ( *(int *)a - *(int *)b );
}


/**
 * TODO: Fill in this function
 *
 * Given a car's in_dir and out_dir return a sorted 
 * list of the quadrants the car will pass through.
 * 
 */
int *compute_path(enum direction in_dir, enum direction out_dir) {

	int *dir_pointer = NULL;
	int dir[4];

	// checks if in_dir and out_dir are valid directions
	if ((in_dir == NORTH || in_dir == SOUTH || in_dir == EAST || in_dir == WEST) &&
		(out_dir == NORTH || out_dir == SOUTH || out_dir == EAST || out_dir == WEST)) {

		//computes path for when in_dir is NORTH
		if (in_dir == NORTH) {

			if (out_dir == NORTH) {
				dir[0] = 2;
				dir[1] = 1;
				dir[2] = -1;
				dir[3] = -1;

			}
			else if (out_dir == SOUTH) {
				dir[0] = 2;
				dir[1] = 3;
				dir[2] = -1;
				dir[3] = -1;

			}
			else if (out_dir == EAST) {
				dir[0] = 2;
				dir[1] = 3;
				dir[2] = 4;
				dir[3] = -1;

			}
			else {
				dir[0] = 2;
				dir[1] = -1;
				dir[2] = -1;
				dir[3] = -1;

			}

		}
		
		//computes path for when in_dir is SOUTH
		else if (in_dir == SOUTH) {

			if (out_dir == SOUTH) {
				dir[0] = 4;
				dir[1] = 3;
				dir[2] = -1;
				dir[3] = -1;

			}
			else if (out_dir == NORTH) {
				dir[0] = 4;
				dir[1] = 1;
				dir[2] = -1;
				dir[3] = -1;

			}
			else if (out_dir == EAST) {
				dir[0] = 4;
				dir[1] = -1;
				dir[2] = -1;
				dir[3] = -1;

			}
			else {
				dir[0] = 4;
				dir[1] = 1;
				dir[2] = 2;
				dir[3] = -1;

			}

		}
		
		//computes path for when in_dir is EAST
		else if (in_dir == EAST) {

			if (out_dir == EAST) {
				dir[0] = 1;
				dir[1] = 4;
				dir[2] = -1;
				dir[3] = -1;

			}
			else if (out_dir == NORTH) {
				dir[0] = 1;
				dir[1] = -1;
				dir[2] = -1;
				dir[3] = -1;

			}
			else if (out_dir == SOUTH) {
				dir[0] = 1;
				dir[1] = 2;
				dir[2] = 3;
				dir[3] = -1;

			}
			else {
				dir[0] = 1;
				dir[1] = 2;
				dir[2] = -1;
				dir[3] = -1;

			}

		}
		
		//computes path for when in_dir is WEST
		else {

			if (out_dir == WEST) {
				dir[0] = 3;
				dir[1] = 2;
				dir[2] = -1;
				dir[3] = -1;

			}
			else if (out_dir == NORTH) {
				dir[0] = 3;
				dir[1] = 4;
				dir[2] = 1;
				dir[3] = -1;

			}
			else if (out_dir == SOUTH) {
				dir[0] = 3;
				dir[1] = -1;
				dir[2] = -1;
				dir[3] = -1;

			}
			else {
				dir[0] = 3;
				dir[1] = 4;
				dir[2] = -1;
				dir[3] = -1;

			}

		}

		qsort(dir, sizeof(dir)/sizeof(int), sizeof(int), compare_num);
		dir_pointer = dir;

	}

	return dir_pointer;

}