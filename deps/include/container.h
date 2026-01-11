#ifndef CONTAINER_H
#define CONTAINER_H

#include <stddef.h>

typedef struct Vector Vector;

#define CONCAT2(a, b) a##b
#define CONCAT(a, b) CONCAT2(a, b)

// Wrapper definition for vector_init
Vector *vector_init_impl(size_t element_size);
#define vector_init(TYPE) vector_init_impl(sizeof(TYPE))

// Wrapper definition for append_impl
void append_impl(Vector *v, const void *element);
// will be used for appending values directly
#define append(type, vec, value)                                               \
	do {                                                                       \
		type CONCAT(_tmp_, __LINE__) = (value);                                \
		append_impl((vec), &CONCAT(_tmp_, __LINE__));                          \
	} while (0)

// Wrapper definition for at_impl
void *at_impl(Vector *vector, int pos);
#define at(TYPE, vec, pos) (*(TYPE *)at_impl((vec), (pos)))

// Wrapper definition for pop_impl
void *pop_impl(Vector *vector);
#define pop(TYPE, vec) (*(TYPE *)pop_impl((vec)))

// Wrapper definition for replace_at_impl
void replace_at_impl(Vector *vector, int pos, const void *value);
// will be used for appending values directly
#define replace_at(type, vec, pos, value)                                      \
	do {                                                                       \
		type CONCAT(_tmp_, __LINE__) = (value);                                \
		replace_at_impl((vec), (pos), &CONCAT(_tmp_, __LINE__));               \
	} while (0)

int length(Vector *vector);

// Frees underlying data structure
void vector_free(Vector *vector);

#endif // CONTAINER_H
