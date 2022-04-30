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

/**
 * Sets this string to the empty string.
 */
void string_clear(string *s);
/**
 * Adds the contents of other to s.
 */
void string_appends(string *s, string *other);
/**
 * Replaces the contents of s with other.
 */
void string_sets(string *s, string *other);
/**
 * Adds the contents of the formatted string to s.
 */
void string_appendf(string *s, char *fmt, ...);
/**
 * Replaces the contents of s with the formatted string.
 */
void string_setf(string *s, char *fmt, ...);
/**
 * Appends a portion of src to dst.
 * @param dst the string to add to
 * @param src the string from which the substring is taken
 * @param start the index of the first character in src (inclusive)
 * @param end the index after the last character in src (exclusive)
 */
void string_append_substr(string *dst, string *src, size_t start, size_t end);
/**
 * Replaces the contents of dst with a portion of src.
 * @param dst the string to replace
 * @param src the string from which the substring is taken
 * @param start the index of the first character in src (inclusive)
 * @param end the index after the last character in src (exclusive)
 */
void string_set_substr(string *dst, string *src, size_t start, size_t end);

/**
 * @param s the string to search in
 * @param find a string to search for
 * @param start the index in s to start looking
 * @returns the index of the first match in s, or -1 if no match
 */
size_t string_index_of_str(string *s, string *find, size_t start);
/**
 * @param s the string to search in
 * @param find a string to search for
 * @param start the index in s to start looking
 * @returns the index of the first match in s, or -1 if no match
 */
size_t string_index_of_cstr(string *s, char *find, size_t start);
/**
 * @param s the string to search in
 * @param find a character to search for
 * @param start the index in s to start looking
 * @returns the index of the first match in s, or -1 if no match
 */
size_t string_index_of_char(string *s, char find, size_t start);

/**
 * @param s the string to search in
 * @param find a string to search for
 * @param start the index in s to start looking
 * @returns the index of the last match in s, or -1 if no match
 */
size_t string_reverse_index_of_str(string *s, string *find, size_t start);
/**
 * @param s the string to search in
 * @param find a string to search for
 * @param start the index in s to start looking
 * @returns the index of the last match in s, or -1 if no match
 */
size_t string_reverse_index_of_cstr(string *s, char *find, size_t start);
/**
 * @param s the string to search in
 * @param find a character to search for
 * @param start the index in s to start looking
 * @returns the index of the last match in s, or -1 if no match
 */
size_t string_reverse_index_of_char(string *s, char find, size_t start);

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