#ifndef _MALLOC_H_
#define _MALLOC_H_

struct malloc_stats {
	int mallocs;
	int frees;
	int requested_memory;
};

typedef enum block_size {
	SMALL = 16 * 128,
	MEDIUM = 1 * 1024 * 128,
	LARGE = 32 * 1024 * 128,
} block_size_t;

// Estructura que representa una region de memoria
// Es un header que contiene informacion sobre la region, no los datos que el usuario guarda en si. CREO
struct region {
	bool free;
	size_t size;
	struct region *next;
	struct region *prev;
	bool is_first;
};

struct region *first_region;

void *malloc(size_t size);

void free(void *ptr);

void *calloc(size_t nmemb, size_t size);

void *realloc(void *ptr, size_t size);

void get_stats(struct malloc_stats *stats);

#endif  // _MALLOC_H_
