#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"


extern int memsize;

extern int debug;

extern struct frame *coremap;

int first_dereference;

/* Page to evict is chosen using the clock algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */

int clock_evict() {

	while (first_dereference < memsize) {
		first_dereference++;
		first_dereference %= memsize;

		// checks for the first occurrence of PG_REF being 0 
		// (at which point victim frame is found)
		if (!((coremap[first_dereference].pte)->frame & PG_REF)) {
			return first_dereference;
		} else {
			(coremap[first_dereference].pte)->frame &= ~PG_REF;
		}
	}

	first_dereference = -1;
	return first_dereference;

}

/* This function is called on each access to a page to update any information
 * needed by the clock algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void clock_ref(pgtbl_entry_t *p) {

	return;
}

/* Initialize any data structures needed for this replacement
 * algorithm. 
 */
void clock_init() {

	first_dereference = 0;

}
