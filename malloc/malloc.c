#define _DEFAULT_SOURCE

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>

#include "malloc.h"
#include "printfmt.h"

#define ALIGN4(s) (((((s) -1) >> 2) << 2) + 4)
#define REGION2PTR(r) ((r) + 1)
#define PTR2REGION(ptr) ((struct region *) (ptr) -1)
#define MIN_REGION_SIZE 32


int amount_of_mallocs = 0;
int amount_of_frees = 0;
int requested_memory = 0;
int blocks_counter = 0;

void
try_split_region(struct region *region, size_t size)
{
	if (region->size - size < MIN_REGION_SIZE + sizeof(struct region))
		return;

	struct region *new_region =
	        (struct region *) ((size_t) region + sizeof(struct region) + size);
	new_region->size = region->size - size - sizeof(struct region);
	new_region->free = true;
	new_region->next = region->next;
	new_region->prev = region;

	region->size = size;
	region->next = new_region;
	region->free = false;
}


// new_region_block->size = size;
// new_region_block->prev = NULL;
// new_region_block->is_first = true;

// struct region *rest_block_region =
//         (struct region *) ((size_t) new_region_block + size +
//                            sizeof(struct region));
// rest_block_region->free = true;
// rest_block_region->size = block_size - size - 2 * sizeof(struct
// region); rest_block_region->prev = new_region_block;
// rest_block_region->next = NULL; rest_block_region->is_first = false;


struct region *first_region;
// finds the next free region
// that holds the requested size
//
static struct region *
find_free_region(size_t size)
{
	if (!first_region) {
		return NULL;
	}


#ifdef FIRST_FIT
	struct region *last_region = first_region;
	while (last_region) {
		if (last_region->free && last_region->size >= size) {
			try_split_region(last_region, size);
			return last_region;
		}
		last_region = last_region->next;
	}
#endif

#ifdef BEST_FIT
	struct region *best_region = NULL;
	struct region *last_region = first_region;
	while (last_region) {
		if (last_region->free && last_region->size >= size) {
			if (!best_region || last_region->size < best_region->size) {
				best_region = last_region;
			}
		}
		last_region = last_region->next;
	}

	if (best_region) {
		try_split_region(best_region, size);
		return best_region;
	}
#endif

	return NULL;
}

static struct region *
grow_heap(size_t size)
{
	size_t block_size = SMALL;
	if (size > LARGE - sizeof(struct region)) {
		return NULL;
	} else if (size > MEDIUM - sizeof(struct region)) {
		block_size = LARGE;
	} else if (size > SMALL - sizeof(struct region)) {
		block_size = MEDIUM;
	}

	struct region *new_region_block = mmap(NULL,
	                                       block_size,
	                                       PROT_READ | PROT_WRITE,
	                                       MAP_PRIVATE | MAP_ANONYMOUS,
	                                       -1,
	                                       0);

	new_region_block->size = block_size - sizeof(struct region);
	new_region_block->is_first = true;

	new_region_block->next = NULL;

	try_split_region(new_region_block, size);


	// new_region_block->free = false;
	// new_region_block->size = size;
	// new_region_block->prev = NULL;

	// struct region *rest_block_region =
	//         (struct region *) ((size_t) new_region_block + size +
	//                            sizeof(struct region));
	// rest_block_region->free = true;
	// rest_block_region->size = block_size - size - 2 * sizeof(struct
	// region); rest_block_region->prev = new_region_block;
	// rest_block_region->next = NULL; rest_block_region->is_first = false;


	// new_region_block->next = rest_block_region;


	if (!first_region) {
		first_region = new_region_block;
	} else {
		struct region *last_region = first_region;
		while (last_region->next) {
			last_region = last_region->next;
		}

		last_region->next = new_region_block;
		new_region_block->prev = last_region;
	}


	return new_region_block;
}

/// Public API of malloc library ///

void *
malloc(size_t size)
{
	struct region *free_region;

	// aligns to multiple of 4 bytes
	size = ALIGN4(size);

	if (size < MIN_REGION_SIZE)
		size = MIN_REGION_SIZE;

	// updates statistics
	amount_of_mallocs++;
	requested_memory += size;

	free_region = find_free_region(size);

	if (!free_region) {
		blocks_counter++;
		free_region = grow_heap(size);
	}
	if (!free_region) {
		return NULL;
	}

	// Esto lo que hace es agarrar el puntero al header de la region y
	// moverlo (en aritmetica de punteros es sumar 1), de esta forma queda
	// apuntando a los datos que el usuario va a guardar.
	return REGION2PTR(free_region);
}

void
free(void *ptr)
{
	if (!ptr)
		return;
	// updates statistics
	amount_of_frees++;

	// Esto lo que hace es agarrar el ptr que recibe por parametro (que apunta
	// a los datos que el usuario guardo) y le resta el tamaño del header de
	// la region, de esta forma queda apuntando al header de la region. Y como
	// el header guarda el tamaño de la region, se puede saber rapidamente cuanta memoria hay que liberar.
	struct region *curr = PTR2REGION(ptr);


	if (curr->free) {
		printfmt("ERROR: free() called on already freed pointer\n");
		assert(true);
	}

	curr->free = true;


	if (curr->prev && curr->prev->free && !curr->is_first) {
		curr->prev->size += curr->size + sizeof(struct region);
		curr->prev->next = curr->next;
		if (curr->next)
			curr->next->prev = curr->prev;
		curr = curr->prev;
	}

	if (curr->next && curr->next->free && !curr->next->is_first) {
		curr->size += curr->next->size + sizeof(struct region);
		curr->next = curr->next->next;
		if (curr->next)
			curr->next->prev = curr;
	}

	if (curr->is_first && (!curr->next || curr->next->is_first)) {
		curr->prev->next = curr->next;
		munmap(curr, curr->size);
		blocks_counter--;
	}
}

void *
calloc(size_t nmemb, size_t size)
{
	// Your code here

	return NULL;
}

void *
realloc(void *ptr, size_t size)
{
	// Your code here

	return NULL;
}

void
get_stats(struct malloc_stats *stats)
{
	stats->mallocs = amount_of_mallocs;
	stats->frees = amount_of_frees;
	stats->requested_memory = requested_memory;
	stats->blocks_counter = blocks_counter;
}
