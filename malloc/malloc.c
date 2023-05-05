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

struct region *region_free_list = NULL;

int amount_of_mallocs = 0;
int amount_of_frees = 0;
int requested_memory = 0;

// finds the next free region
// that holds the requested size
//
static struct region *
find_free_region(size_t size)
{
	if (!first_block) {
		return NULL;
	}

	struct region *last_region = first_block->first;

	while (last_region->next) {
		if (last_region->free && last_region->size >= size) {
			if (last_region->size - size >=
			    MIN_REGION_SIZE + sizeof(struct region)) {
				struct region *new_region =
				        (size_t) last_region +
				        sizeof(struct region) + size;
				new_region->size = last_region->size - size -
				                   sizeof(struct region);
				new_region->free = true;
				new_region->next = last_region->next;
				new_region->prev = last_region;
				last_region->size = size;
				last_region->next = new_region;
			}
			last_region->free = false;
			return last_region;
		}
		last_region = last_region->next;
	}
	size_t free_space = (size_t) first_block->size + (size_t) first_block -
	                    (size_t) last_region;
	if (free_space < size)
		return NULL;


#ifdef FIRST_FIT
		// Your code here for "first fit"
#endif

#ifdef BEST_FIT
		// Your code here for "best fit"
#endif


	struct region *new_region =
	        (size_t) last_region + sizeof(struct region) + last_region->size;
	new_region->size = size;
	new_region->free = false;
	new_region->next = NULL;
	new_region->prev = last_region;
	last_region->next = new_region;

	return new_region;
}

static struct region *
grow_heap(size_t size)
{
	if (size > SMALL) {
		return NULL;
	}

	if (!first_block) {
		first_block = mmap(NULL,
		                   SMALL,
		                   PROT_READ | PROT_WRITE,
		                   MAP_PRIVATE | MAP_ANONYMOUS,
		                   -1,
		                   0);
		first_block->size = SMALL;
		first_block->first = (size_t) first_block + sizeof(struct block);
		first_block->first->prev = NULL;
		first_block->first->next = NULL;
		first_block->first->free = false;
		first_block->first->size = size;
		return first_block->first;
	}

	return NULL;
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

	if (curr->prev && curr->prev->free) {
		curr->prev->size += curr->size + sizeof(struct region);
		curr->prev->next = curr->next;
		if (curr->next)
			curr->next->prev = curr->prev;
		curr = curr->prev;
	}

	if (curr->next && curr->next->free) {
		curr->size += curr->next->size + sizeof(struct region);
		curr->next = curr->next->next;
		curr->next->prev = curr;
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
}
