#ifndef CSTRING_H

#define CSTRING_H

#ifndef ARENA_H

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

#endif // ARENA_H

typedef struct String String;

/* Creates a String object from c-string (char *) */
String *string_from(Arena *arena, char *str);

/* Creates a deep copy of an existing String object */
String *string_clone(Arena *arena, String *str);

/* Concatinates n string objects */
String *string_concat(Arena *arena, int n, ...);

/* Returns the length of the string */
int string_len(String *str);

/* Returns a substring from a String where `begin` is inclusive but `end` is
 * exclusive */
String *string_sub(Arena *arena, String *str, int begin, int end);

/* Returns the c-string (char *) from the String object */
char *string(String *st);

/* Concatinates n c-strings (char *) */
String *string_concat_cstr(Arena *arena, int n, ...);

/* Trims all leading and trailing white-spaces from the String */
String *string_trim(Arena *arena, String *str);

/* Takes String input from console */
String *string_get(Arena *arena);

/* Converts all lower-case characters in a string to upper-case*/
char *string_upper(Arena *arena, String *str);

/* Converts all upper-case characters in a string to lower-case*/
char *string_lower(Arena *arena, String *str);

#endif // CSTRING_H
