#include <arena.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef union {
	char c;
	short s;
	int i;
	long l;
	float f;
	double d;
	void *p;
	void (*fp)(void);
} MaxAlign;

struct AlignMax {
	char c;
	MaxAlign m;
};

#define MAX_ALIGN offsetof(struct AlignMax, m)

struct Arena {
	void *region;
	size_t size;
	size_t capacity;
	struct Arena *next;
};

void *m_align_alloc(size_t capacity) {
	size_t alignment;
	size_t header_size;
	size_t total_size;
	void *origin;
	char *current;
	size_t current_addr;
	size_t mask;
	size_t aligned_addr;
	char *aligned_ptr;
	char **head;

	alignment = MAX_ALIGN;

	header_size = sizeof(void *);

	/*
	 * (alignment - 1) ensures proper padding is done and no byte is wasted
	 * for most cases sizeof(void *) is 8 bytes, so for worst case scenario we
	 * need maximum 7 bytes for padding.
	 * */
	total_size = capacity + (alignment - 1) + header_size;
	origin = malloc(total_size);
	if (origin == NULL) {
		return NULL;
	}

	/*
	 * This moves the pointer to starting of memory block of ((alignment - 1) +
	 * aligned memory)
	 * */
	current = (char *)origin + header_size;

	/*
	 * current and (alignment - 1) needs to be casted to an integer type
	 * as we need to do bit manipulation later
	 * */
	current_addr = (size_t)current;
	mask = (size_t)(alignment - 1);

	aligned_addr = (current_addr + mask) & ~mask;

	/*
	 * casting to char* for pointer arithmetic
	 * */
	aligned_ptr = (char *)aligned_addr;

	head = (char **)(aligned_ptr - header_size);
	*head = (char *)origin;

	memset(aligned_ptr, 0, capacity);

	return (void *)aligned_ptr;
}

void m_align_free(void *ptr) {
	char **head;
	void *origin;

	if (ptr == NULL) {
		return;
	}

	/*
	 * head is a pointer-to-pointer which stores the address of the original
	 * address returned by malloc. ptr is the aligned pointer, so if we move
	 * back by a size of a pointer we will have the address where the original
	 * pointer is stored.
	 *
	 *     v-head
	 * [   |========][============]
	 * ^-malloc      ^-aligned memory
	 * */
	head = (char **)((char *)ptr - sizeof(void *));
	origin = (void *)*head;

	free(origin);
}

void *m_align_realloc(void *ptr, size_t old_size, size_t new_size) {
	void *new_ptr;
	size_t copy_size;

	/* If ptr is NULL, behave like malloc */
	if (ptr == NULL) {
		return m_align_alloc(new_size);
	}

	if (new_size == -1) {
		m_align_free(ptr);
		return NULL;
	}

	new_ptr = m_align_alloc(new_size);
	if (new_ptr == NULL) {
		return NULL; /* Keep old ptr valid on failure */
	}

	copy_size = old_size < new_size ? old_size : new_size;
	memcpy(new_ptr, ptr, copy_size);

	m_align_free(ptr);

	return new_ptr;
}

Arena *arena_init(size_t capacity) {
	Arena *arena;
	void *data;

	arena = (Arena *)m_align_alloc(sizeof(Arena));
	data = m_align_alloc(capacity);

	if (arena == NULL || data == NULL) {
		fprintf(stderr, "Fatal: Failed to allocate arena");
		abort();
	}

	arena->region = data;
	arena->capacity = capacity;
	arena->size = 0;
	arena->next = NULL;

	return arena;
}

void *arena_alloc(Arena *arena, size_t size) {
	Arena *current_arena;
	void *data;
	Arena *new_arena;

	current_arena = arena;
	data = NULL;

	/*
	 * if the element fits in the arena, then store it.
	 * */
	if (current_arena->size + size <= current_arena->capacity) {
		data = (char *)current_arena->region + current_arena->size;
		current_arena->size += size;
	}
	/*
	 * if it doesn't fit, check if it has any other arena attached to it.
	 * if no arena is attached, then allocate a new one.
	 * */
	else {
		if (current_arena->next != NULL) {
			current_arena = current_arena->next;
			data = arena_alloc(current_arena, size);
		} else {
			new_arena = NULL;
			if (current_arena->capacity < size) {
				new_arena = arena_init(size);
			} else {
				new_arena = arena_init(arena->capacity);
			}
			current_arena->next = new_arena;
			data = arena_alloc(new_arena, size);
		}
	}

	return data;
}

void arena_free(Arena **arena) {
	if (*arena == NULL) {
		return;
	}
	if ((*arena)->next != NULL) {
		arena_free(&(*arena)->next);
	}
	m_align_free((*arena)->region);
	(*arena)->region = NULL;
	m_align_free(*arena);
	*arena = NULL;
	return;
}

void arena_reset(Arena **arena) {
	Arena *next;
	(*arena)->size = 0;
	next = (*arena)->next;
	arena_free(&next);
	(*arena)->next = NULL;
	memset((*arena)->region, 0, (*arena)->capacity);
}
