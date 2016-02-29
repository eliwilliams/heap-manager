#include <stdio.h> //needed for size_t
#include <unistd.h> //needed for sbrk
#include <assert.h> //For asserts
#include "dmm.h"

/* You can improve the below metadata structure using the concepts from Bryant
 * and OHallaron book (chapter 9).
 */

typedef struct metadata {
       /* size_t is the return type of the sizeof operator. Since the size of
 	* an object depends on the architecture and its implementation, size_t 
	* is used to represent the maximum size of any object in the particular
 	* implementation. 
	* size contains the size of the data object or the amount of free
 	* bytes
	*/
	bool isfree;
	size_t size;
	//struct metadata* next; // This pointer will point to the next free block (to make iteration efficient)
	//struct metadata* prev; //What's the use of prev pointer? To me, none. I couldn't figure out the prev and next pointers for the life of me for some reason.
} metadata_t;

/* freelist maintains all the blocks which are not in use; freelist is kept
 * always sorted to improve the efficiency of coalescing
 */

static metadata_t* freelist = NULL;
static metadata_t* freelist_tail = NULL;

void* dmalloc(size_t numbytes) {
	if(freelist == NULL) { 			// Initialize through sbrk call first time
		if(!dmalloc_init())
			return NULL;
	}
	
	assert(numbytes > 0);

	/* Your code goes here */

	// initialize headers and footers for the upcoming block split
	void* return_pointer;
	size_t full_size = ((2 * METADATA_T_ALIGNED) + ALIGN(numbytes));  // store the size of header and footer to make traversal easy
	metadata_t* conductor = freelist;  // start traversal for free block at the top of the heap
	metadata_t* found_block_header;  //  
	metadata_t* found_block_footer;  // 
	metadata_t* split_block_footer;  // 
	
	while(conductor < freelist_tail) {
			if(conductor->isfree && conductor->size >= full_size) {
				found_block_header = conductor;
				conductor = (metadata_t*) ((void*) found_block_header + full_size);  // conductor is now functioning as the split block header
				conductor->size = (found_block_header->size - full_size);
				conductor->isfree = true;

				found_block_header->size = ALIGN(numbytes);
				found_block_header->isfree = false;
				found_block_footer = (metadata_t*) ((void*) conductor - METADATA_T_ALIGNED);
				found_block_footer->size = found_block_header->size;
				found_block_footer->isfree = false;
				
				split_block_footer = (metadata_t*) ((void*) conductor + conductor->size + METADATA_T_ALIGNED);
				split_block_footer->size = conductor->size;
				split_block_footer->isfree = true;
				return_pointer = (metadata_t*) ((void*) found_block_header + METADATA_T_ALIGNED);
				
				return return_pointer;
			}
			else {
				conductor = (metadata_t*) ((void*) conductor + conductor->size + (2 * METADATA_T_ALIGNED)); // iterate to the next block
			}
	}
	return NULL;
}

void dfree(void* ptr) {
	
	/* Your free and coalescing code goes here */
	// initialize pointers to every applicable header or footer needed for freeing/coalescing
	metadata_t* block_header = (metadata_t*) ((void*) ptr - METADATA_T_ALIGNED);  // this is the header of the block that we want to free
	metadata_t* block_footer = (metadata_t*) ((void*) ptr + block_header->size);  // this is the footer of the block we want to free
	metadata_t* next_block_header = (metadata_t*) ((void*) block_footer + METADATA_T_ALIGNED);  // next block's header to check for possible coalesce
	metadata_t* prev_block_footer = (metadata_t*) ((void*) block_header - METADATA_T_ALIGNED);  // previous block's footer to check for possible coalesce
	metadata_t* coal_header = block_header;  // this will be the header of the new block if one or more coalescing operations occur
	
	//change boolean isfree to true to free the header and footer in O(1)
	block_header->isfree = true;
	block_footer->isfree = true;
	
	// check previous node for possible coalesce in O(1)
	if((prev_block_footer > freelist) && (prev_block_footer->isfree)){
		prev_block_footer = (metadata_t*) ((void*) prev_block_footer - prev_block_footer->size - METADATA_T_ALIGNED); // this is now the prologue
		prev_block_footer->size = (prev_block_footer->size + block_header->size + (2 * METADATA_T_ALIGNED)); // must add header and footer sizes because we freed a block
		prev_block_footer->isfree = true;
		coal_header = prev_block_footer;
		block_footer->size = coal_header->size;
	}
	
	// check next node for possible coalesce in O(1)
	if(next_block_header->isfree && (next_block_header < freelist_tail)){
		next_block_header = (metadata_t*) ((void*) next_block_header + next_block_header->size + METADATA_T_ALIGNED); // this is now the epilogue
		next_block_header->size = (next_block_header->size + coal_header->size + (2 * METADATA_T_ALIGNED));  // must add header and footer sizes because we freed a block
		next_block_header->isfree = true;
		coal_header->size = next_block_header->size;
	}
}

bool dmalloc_init() {

	size_t max_bytes = ALIGN(MAX_HEAP_SIZE);
	freelist = (metadata_t*) sbrk(max_bytes); // returns heap_region, which is initialized to freelist
	
	if (freelist == (void *)-1)
		return false;

	freelist->size = max_bytes - (2 * METADATA_T_ALIGNED);
	freelist->isfree = true;
	
	// initialize freelist footer
	freelist_tail = (metadata_t*) ((void*) freelist + freelist->size + METADATA_T_ALIGNED);
	freelist_tail->size = freelist->size;
	freelist_tail->isfree = false;
	
	return true;
}i