#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"


extern int memsize;

extern int debug;

extern struct frame *coremap;

int ref_time;

/* Page to evict is chosen using the accurate LRU algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */

int lru_evict() {
	int i;
	int frame;
	int least_ref = ref_time;
	
	for(i = 0; i < memsize; i++){
		
		//if there's a page that's been referenced less than least_ref, 
		//update least_ref
		
		//return that page and update least_ref
		if (coremap[i].timestamp < least_ref){
			least_ref = coremap[i].timestamp;
			frame = i;
		}
		
	}
	
	return frame;
}

/* This function is called on each access to a page to update any information
 * needed by the lru algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void lru_ref(pgtbl_entry_t *p) {
	
	//get corresponding frame and update its timestamp
	//update reference time each time a page is referenced
	int frame = p->frame >> PAGE_SHIFT;
	coremap[frame].timestamp = ref_time;
	ref_time++;

	return;
}


/* Initialize any data structures needed for this 
 * replacement algorithm 
 */
void lru_init() {
	ref_time = 0;
}
