#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"


extern int memsize;

extern int debug;

extern struct frame *coremap;

int oldest_page_order;

/* Page to evict is chosen using the fifo algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */
int fifo_evict() {

	int valid_page_order = oldest_page_order % memsize;
	oldest_page_order++;

	// checks if the victim frame number is beyond
	// physical memory size (ie. if memory is full)
	if (valid_page_order == memsize) {
		valid_page_order = 0;
	}

	return valid_page_order;

}

/* This function is called on each access to a page to update any information
 * needed by the fifo algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void fifo_ref(pgtbl_entry_t *p) {

	return;
}

/* Initialize any data structures needed for this 
 * replacement algorithm 
 */
void fifo_init() {

	oldest_page_order = 0;

}
