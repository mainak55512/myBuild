#include <container.h>
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

struct Vector {
	size_t element_size;
	int capacity;
	int length;
	char *items;
};

void *mem_align_alloc(size_t capacity) {
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

void mem_align_free(void *ptr) {
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

void *mem_align_realloc(void *ptr, size_t old_size, size_t new_size) {
	void *new_ptr;
	size_t copy_size;

	/* If ptr is NULL, behave like malloc */
	if (ptr == NULL) {
		return mem_align_alloc(new_size);
	}

	if (new_size == -1) {
		mem_align_free(ptr);
		return NULL;
	}

	new_ptr = mem_align_alloc(new_size);
	if (new_ptr == NULL) {
		return NULL; /* Keep old ptr valid on failure */
	}

	copy_size = old_size < new_size ? old_size : new_size;
	memcpy(new_ptr, ptr, copy_size);

	mem_align_free(ptr);

	return new_ptr;
}

Vector *vector_init_impl(size_t element_size) {
	Vector *vector = (Vector *)mem_align_alloc(sizeof(Vector));
	vector->items = NULL;
	vector->element_size = element_size;
	vector->capacity = 1;
	vector->length = 0;
	return vector;
}

void append_impl(Vector *vector, const void *element) {
	if (vector->items == NULL) {
		vector->items = (char *)mem_align_alloc(vector->element_size);
		vector->capacity = 1;
	} else if (vector->length >= vector->capacity) {
		vector->capacity *= 2;
		vector->items = (char *)mem_align_realloc(
			vector->items, (vector->capacity / 2) * vector->element_size,
			vector->capacity * vector->element_size);
	}
	memcpy((char *)vector->items + (vector->length * vector->element_size),
		   element, vector->element_size);
	vector->length++;
}

void *pop_impl(Vector *vector) {
	if (vector->length > 0) {
		vector->length--;
		return (char *)vector->items + (vector->length * vector->element_size);
	}
	return NULL;
}

void *at_impl(Vector *vector, int pos) {
	if (pos >= 0 && pos < vector->length) {
		return (char *)vector->items + (pos * vector->element_size);
	}
	return NULL;
}

void replace_at_impl(Vector *vector, int pos, const void *value) {
	if (pos >= 0 && pos < vector->length) {
		memcpy((char *)vector->items + (pos * vector->element_size), value,
			   vector->element_size);
	}
}

int length(Vector *vector) { return vector->length; }

void vector_free(Vector *vector) {
	mem_align_free(vector->items);
	mem_align_free(vector);
}
