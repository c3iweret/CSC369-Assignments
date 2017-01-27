#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"
#include "sim.h"


//extern int memsize;

extern int debug;

extern struct frame *coremap;

typedef struct ref {
	addr_t addr;	//Address used in each reference
	struct ref * next;	//Pointer to next reference
} ref_t;


ref_t * curr_ref;
ref_t * next_ref;


/* Page to evict is chosen using the optimal (aka MIN) algorithm. 
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */
int opt_evict() {
	int frame;
	int next_reference;
	int longest_unused_page = 0;
	int longest_unused_ref = 0;
	
	//Search for frame that wouldn't be referenced for the longest time
	for (frame = 0; frame < memsize; frame++){
		next_ref = curr_ref;
		next_reference = 0;

		//Find when next frame is used and increment next reference
		while (next_ref->addr != coremap[frame].addr){
			
			//frame is not referenced again so best choice for eviction
			if (next_ref->next == NULL){
				return frame;
			}
			next_reference++;
			next_ref = next_ref->next;
		}
		
		//updates the longest page unused with the maximum distance 
		//before next reference
		if (next_reference > longest_unused_ref){
			longest_unused_ref = next_reference;
			longest_unused_page = frame;
		}
	}
	
	
	return longest_unused_page;
}

/* This function is called on each access to a page to update any information
 * needed by the opt algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void opt_ref(pgtbl_entry_t *p) {
	
	//move linked list forward after every reference so we do not go 
	//through previous references
	curr_ref = curr_ref->next;
	return;
}

/* Initializes any data structures needed for this
 * replacement algorithm.
 */
void opt_init() {
	
	//create a linked list of pages referenced with each page's next attribute
	//pointing to the next time it's referenced
	FILE *fp;
	char buf[MAXLINE];
	addr_t vaddr = 0;
	char type;
	
	//open trace file for reading
	if((fp = fopen(tracefile, "r")) == NULL) {
		perror("Error opening tracefile:");
		exit(1);
	}
	
	//initialize structs 
	curr_ref = NULL;
	next_ref = NULL;
	
	//build the linked list using the trace file
	while(fgets(buf, MAXLINE, fp) != NULL) {
		if(buf[0] != '=') {
			sscanf(buf, "%c %lx", &type, &vaddr);
			
			//create a new node for the page reference and 
			//add it to the linked list
			ref_t * new_ref = malloc(sizeof(ref_t));
			
			//check if memory was successfully allocated
			if(new_ref == NULL){
				fprintf(stderr, 
					"Cannot allocate memory for new page reference.\n");
				exit(1);
			}
			
			new_ref->addr = vaddr;
			new_ref->next = NULL;
			
			if(curr_ref){
				next_ref->next = new_ref;
				next_ref = new_ref;
			}
			else{
				curr_ref = new_ref;
				next_ref = new_ref;
			}
			
		}
	}
	
	fclose(fp);

}

