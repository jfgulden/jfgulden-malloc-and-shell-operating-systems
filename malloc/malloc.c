#define _DEFAULT_SOURCE

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

#include "malloc.h"
#include "printfmt.h"

#define ALIGN4(s) (((((s) -1) >> 2) << 2) + 4)
#define REGION2PTR(r) ((r) + 1)
#define PTR2REGION(ptr) ((struct region *) (ptr) -1)

// Quizas puede servir esta estructura y que tenga un campo con el tamaño
// (chico, mediano, grande) (parte 1 del tp solo tiene el chico) y otro campo
// con un puntero a la primera region del bloque? Esta idea fue sugerida en la
// clase del 28/4. Mas adelante se puede agregar un campo con  una referencia al
// bloque anterior o al siguiente o ambos.
struct block {
	size_t size;
	struct region *first;
};

// Estructura que representa una region de memoria
// Es un header que contiene informacion sobre la region, no los datos que el usuario guarda en si. CREO
struct region {
	bool free;
	size_t size;
	struct region *next;
};

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
	struct region *next = region_free_list;

#ifdef FIRST_FIT
	// Your code here for "first fit"
#endif

#ifdef BEST_FIT
	// Your code here for "best fit"
#endif

	return next;
}

static struct region *
grow_heap(size_t size)
{
	// finds the current heap break
	struct region *curr = (struct region *) sbrk(0);

	// allocates the requested size
	struct region *prev =
	        (struct region *) sbrk(sizeof(struct region) + size);

	// verifies that the returned address
	// is the same that the previous break
	// (ref: sbrk(2))
	assert(curr == prev);

	// verifies that the allocation
	// is successful
	//
	// (ref: sbrk(2))
	if (curr == (struct region *) -1) {
		return NULL;
	}

	// first time here
	if (!region_free_list) {
		region_free_list = curr;
	}

	curr->size = size;
	curr->next = NULL;
	curr->free = false;

	return curr;
}

/// Public API of malloc library ///

void *
malloc(size_t size)
{
	struct region *next;

	// aligns to multiple of 4 bytes
	size = ALIGN4(size);

	// updates statistics
	amount_of_mallocs++;
	requested_memory += size;

	next = find_free_region(size);

	if (!next) {
		next = grow_heap(size);
	}

	// Your code here
	//
	// hint: maybe split free regions?

	// Esto lo que hace es agarrar el puntero al header de la region y
	// moverlo (en aritmetica de punteros es sumar 1), de esta forma queda
	// apuntando a los datos que el usuario va a guardar.
	return REGION2PTR(next);
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
	assert(curr->free == 0);

	curr->free = true;

	// Your code here
	//
	// hint: maybe coalesce regions?

	// Una idea para coalescer es recorrer
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
