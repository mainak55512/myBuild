#ifndef ARENA_H

#define ARENA_H

#include <stddef.h>

typedef struct Arena Arena;

/* Allocates memory on Heap with maximum memory alignment */
void *m_align_alloc(size_t capacity);

/* Frees memory allocated by m_align_alloc or m_align_realloc */
void m_align_free(void *ptr);

/* Re-allocates memory on Heap with maximum memory alignment */
void *m_align_realloc(void *ptr, size_t old_size, size_t new_size);

/* Initiates an arena with the given capacity */
Arena *arena_init(size_t capacity);

/* Allocates a chunk on the arena, when the arena
 * gets full, it calls `arena_init` again internally to allocate more memory */
void *arena_alloc(Arena *arena, size_t size);

/* Frees the arena and all the allocations done on it */
void arena_free(Arena **arena);

/* Wipes out all the allocations done on the arena and restores it to its
 * initial capacity */
void arena_reset(Arena **arena);

#endif
