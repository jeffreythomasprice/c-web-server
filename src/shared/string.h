#ifndef string_h
#define string_h

#include "buffer.h"

typedef struct {
	buffer b;
} string;

void string_init(string *s);
void string_init_cstr(string *s, char *c);
void string_init_cstr_len(string *s, char *c, size_t len);
void string_dealloc(string *s);

size_t string_get_length(string *s);
char *string_get_cstr(string *s);

void string_clear(string *s);

void string_appends(string *s, string *other);
void string_appendf(string *s, char *fmt, ...);

/**
 * @param dst the string to add to
 * @param src the string from which the substring is taken
 * @param start the index of the first character in src (inclusive)
 * @param end the index after the last character in src (exclusive)
 */
void string_append_substr(string *dst, string *src, size_t start, size_t end);

/**
 * Splits the string around the given delimiter. Exact matches are looked for.
 *
 * If more elements would be produced than max_results, the characters at the end of the match are returned as a single result. 0 is treated
 * as infinite matches.
 *
 * @param s the string to spllit
 * @param delim a delimiter to check for
 * @param results an array of indices to fill in, in pairs for the start and end index of each substring
 * @param results_capacity the size of results, divided by two (i.e. the number of results, not the number of indices)
 * @param max_results the maximum number of results to produce
 * @returns the number of strings that would be produced, which may be more than results_capacity
 */
size_t string_split(string *s, char *delim, size_t *results, size_t results_capacity, size_t max_results);

#endif