#define _DEFAULT_SOURCE

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>

#include "malloc.h"
#include "printfmt.h"

#define ALIGN4(s) (((((s) -1) >> 2) << 2) + 4)
#define REGION2PTR(r) ((r) + 1)
#define PTR2REGION(ptr) ((struct region *) (ptr) -1)
#define MIN_REGION_SIZE 40

void try_split_region(struct region *region, size_t size);
void *reallocate_to_new_region(void *ptr, size_t size, struct region *ptr_region);

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

struct region *first_region;

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
	if (new_region_block == MAP_FAILED) {
		return NULL;
	}
	new_region_block->size = block_size - sizeof(struct region);
	new_region_block->is_first = true;

	new_region_block->next = NULL;

	try_split_region(new_region_block, size);

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

	size = ALIGN4(size);

	if (size < MIN_REGION_SIZE)
		size = MIN_REGION_SIZE;

	amount_of_mallocs++;
	requested_memory += size;

	free_region = find_free_region(size);

	if (!free_region) {
		blocks_counter++;
		free_region = grow_heap(size);
	}
	if (!free_region) {
		errno = ENOMEM;
		return NULL;
	}

	return REGION2PTR(free_region);
}

void
free(void *ptr)
{
	if (!ptr) {
		errno = ENOMEM;
		return;
	}

	struct region *curr = PTR2REGION(ptr);

	if (curr->free) {
		printfmt("ERROR: free() called on already freed pointer\n");
		errno = ENOMEM;
		return;
	}

	curr->free = true;
	amount_of_frees++;

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
		if (curr == first_region)
			first_region = curr->next;
		else
			curr->prev->next = curr->next;
		if (curr->next)
			curr->next->prev = curr->prev;
		munmap(curr, curr->size);
		blocks_counter--;
	}
}

void *
calloc(size_t nmemb, size_t size)
{
	if (nmemb == 0 || size == 0) {
		errno = ENOMEM;
		return NULL;
	}

	void *var = malloc(nmemb * size);
	if (var == NULL) {
		return NULL;
	}

	memset(var, 0, nmemb * size);
	return var;
}

void *
realloc(void *ptr, size_t size)
{
	if (!ptr)
		return malloc(size);

	if (size == 0) {
		free(ptr);
		return NULL;
	}

	struct region *ptr_region =
	        (struct region *) ((size_t) ptr - sizeof(struct region));

	if (ptr_region->size >= size) {
		try_split_region(ptr_region, size);
		return ptr;
	}

	if (!ptr_region->next || !ptr_region->next->free) {
		return reallocate_to_new_region(ptr, size, ptr_region);
	}

	if (ptr_region->next->size + ptr_region->size + sizeof(struct region) >=
	    size) {
		ptr_region->size +=
		        ptr_region->next->size + sizeof(struct region);
		ptr_region->next = ptr_region->next->next;
		try_split_region(ptr_region->next, size);
		return ptr;
	}

	return reallocate_to_new_region(ptr, size, ptr_region);
}

void *
reallocate_to_new_region(void *ptr, size_t size, struct region *ptr_region)
{
	void *var = malloc(size);
	memcpy(var, ptr, ptr_region->size);
	free(ptr);
	return var;
}

void
get_stats(struct malloc_stats *stats)
{
	stats->mallocs = amount_of_mallocs;
	stats->frees = amount_of_frees;
	stats->requested_memory = requested_memory;
	stats->blocks_counter = blocks_counter;
}
