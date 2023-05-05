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

// Quizas puede servir esta estructura y que tenga un campo con el tamaño
// (chico, mediano, grande) (parte 1 del tp solo tiene el chico) y otro campo
// con un puntero a la primera region del bloque? Esta idea fue sugerida en la
// clase del 28/4. Mas adelante se puede agregar un campo con  una referencia al
// bloque anterior o al siguiente o ambos.
struct block {
	block_size_t size;
	struct region *first;
};

// Estructura que representa una region de memoria
// Es un header que contiene informacion sobre la region, no los datos que el usuario guarda en si. CREO
struct region {
	bool free;
	size_t size;
	struct region *next;
	struct region *prev;
};

struct block *first_block;

void *malloc(size_t size);

void free(void *ptr);

void *calloc(size_t nmemb, size_t size);

void *realloc(void *ptr, size_t size);

void get_stats(struct malloc_stats *stats);

#endif  // _MALLOC_H_
