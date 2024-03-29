#ifndef string_h
#define string_h

#include "buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	STRING_COMPARE_CASE_SENSITIVE = 0,
	STRING_COMPARE_CASE_INSENSITIVE = 1
} string_compare_mode;

typedef struct {
	buffer b;
} string;

void string_init(string *s);
void string_init_cstr(string *s, char *c);
void string_init_cstr_len(string *s, char *c, size_t len);
void string_dealloc(string *s);

size_t string_get_length(string *s);
/**
 * Changes the length of this string. If the new length is bigger new characters are filled with fill.
 */
void string_set_length(string *s, size_t new_len, char fill);
char *string_get_cstr(string *s);

/**
 * Sets this string to the empty string.
 */
void string_clear(string *s);
/**
 * Adds the contents of other to s.
 */
void string_append_str(string *s, string *other);
/**
 * Replaces the contents of s with other.
 */
void string_set_str(string *s, string *other);
/**
 * Adds the contents of other to s.
 * @param other a 0-terminated string
 */
void string_append_cstr(string *s, char *other);
/**
 * Replaces the contents of s with other.
 * @param other a 0-terminated string
 */
void string_set_cstr(string *s, char *other);
/**
 * Adds the contents of other to s.
 * @param other doesn't have to be 0-terminated
 * @param other_len the length of other
 */
void string_append_cstr_len(string *s, char *other, size_t other_len);
/**
 * Replaces the contents of s with other.
 * @param other doesn't have to be 0-terminated
 * @param other_len the length of other
 */
void string_set_cstr_len(string *s, char *other, size_t other_len);
/**
 * Adds the contents of the formatted string to s.
 */
void string_append_cstrf(string *s, char *fmt, ...);
/**
 * Replaces the contents of s with the formatted string.
 */
void string_set_cstrf(string *s, char *fmt, ...);
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
 * Looks for the first occurance of another string in this one.
 * @param s the string to search in
 * @param find a string to search for
 * @param start the index in s to start looking
 * @returns the index of the first match in s, or -1 if no match
 */
size_t string_index_of_str(string *s, string *find, size_t start);
/**
 * Looks for the first occurance of another string in this one.
 * @param s the string to search in
 * @param find a string to search for
 * @param start the index in s to start looking
 * @returns the index of the first match in s, or -1 if no match
 */
size_t string_index_of_cstr(string *s, char *find, size_t start);
/**
 * Looks for the first occurance of another string in this one.
 * @param s the string to search in
 * @param find a character to search for
 * @param start the index in s to start looking
 * @returns the index of the first match in s, or -1 if no match
 */
size_t string_index_of_char(string *s, char find, size_t start);

/**
 * Looks for the first occurance of another string in this one, searching backwards from the end of this string.
 * @param s the string to search in
 * @param find a string to search for
 * @param start the index in s to start looking
 * @returns the index of the last match in s, or -1 if no match
 */
size_t string_reverse_index_of_str(string *s, string *find, size_t start);
/**
 * Looks for the first occurance of another string in this one, searching backwards from the end of this string.
 * @param s the string to search in
 * @param find a string to search for
 * @param start the index in s to start looking
 * @returns the index of the last match in s, or -1 if no match
 */
size_t string_reverse_index_of_cstr(string *s, char *find, size_t start);
/**
 * Looks for the first occurance of another string in this one, searching backwards from the end of this string.
 * @param s the string to search in
 * @param find a character to search for
 * @param start the index in s to start looking
 * @returns the index of the last match in s, or -1 if no match
 */
size_t string_reverse_index_of_char(string *s, char find, size_t start);

/**
 * Looks for the first occurance of one of a set of characters in this string.
 * @param s the string to search in
 * @param find a set of characters to search for
 * @param start the index in s to start looking
 * @returns the index of the first match in s, or -1 if no match
 */
size_t string_index_of_any_str(string *s, string *find, size_t start);
/**
 * Looks for the first occurance of one of a set of characters in this string.
 * @param s the string to search in
 * @param find a set of characters to search for
 * @param start the index in s to start looking
 * @returns the index of the first match in s, or -1 if no match
 */
size_t string_index_of_any_cstr(string *s, char *find, size_t start);

/**
 * Looks for the first occurance of one of a set of characters in this string, searching backwards from the end of this string.
 * @param s the string to search in
 * @param find a set of characters to search for
 * @param start the index in s to start looking
 * @returns the index of the first match in s, or -1 if no match
 */
size_t string_reverse_index_of_any_str(string *s, string *find, size_t start);
/**
 * Looks for the first occurance of one of a set of characters in this string, searching backwards from the end of this string.
 * @param s the string to search in
 * @param find a set of characters to search for
 * @param start the index in s to start looking
 * @returns the index of the first match in s, or -1 if no match
 */
size_t string_reverse_index_of_any_cstr(string *s, char *find, size_t start);

/**
 * Looks for the first occurance of any character othar than one of a set of characters in this string.
 * @param s the string to search in
 * @param find a set of characters to search for
 * @param start the index in s to start looking
 * @returns the index of the first match in s, or -1 if no match
 */
size_t string_index_not_of_any_str(string *s, string *find, size_t start);
/**
 * Looks for the first occurance of any character othar than one of a set of characters in this string.
 * @param s the string to search in
 * @param find a set of characters to search for
 * @param start the index in s to start looking
 * @returns the index of the first match in s, or -1 if no match
 */
size_t string_index_not_of_any_cstr(string *s, char *find, size_t start);

/**
 * Looks for the first occurance of any character othar than one of a set of characters in this string, searching backwards from the end of
 * this string.
 * @param s the string to search in
 * @param find a set of characters to search for
 * @param start the index in s to start looking
 * @returns the index of the first match in s, or -1 if no match
 */
size_t string_reverse_index_not_of_any_str(string *s, string *find, size_t start);
/**
 * Looks for the first occurance of any character othar than one of a set of characters in this string, searching backwards from the end of
 * this string.
 * @param s the string to search in
 * @param find a set of characters to search for
 * @param start the index in s to start looking
 * @returns the index of the first match in s, or -1 if no match
 */
size_t string_reverse_index_not_of_any_cstr(string *s, char *find, size_t start);

/**
 * Splits the string around the given delimiter. Exact matches are looked for.
 *
 * If more elements would be produced than max_results, the characters at the end of the match are returned as a single result. 0 is treated
 * as infinite matches.
 *
 * @param s the string to spllit
 * @param start the index in s to start looking for results
 * @param delim a delimiter to check for
 * @param results an array of indices to fill in, in pairs for the start and end index of each substring
 * @param results_capacity the size of results, divided by two (i.e. the number of results, not the number of indices)
 * @param max_results the maximum number of results to produce
 * @returns the number of strings that would be produced, which may be more than results_capacity
 */
size_t string_split(string *s, size_t start, char *delim, size_t *results, size_t results_capacity, size_t max_results);

/**
 * @return zero if the two strings are equal, a negative value if a should be sorted before b, or a positive value if b should be sorted
 * after a
 */
int string_compare_str(string *a, string *b, string_compare_mode mode);
/**
 * @param b a 0-terminated string
 * @return zero if the two strings are equal, a negative value if a should be sorted before b, or a positive value if b should be sorted
 * after a
 */
int string_compare_cstr(string *a, char *b, string_compare_mode mode);
/**
 * @param b doesn't have to be 0-terminated
 * @param b_len the length of other
 * @return zero if the two strings are equal, a negative value if a should be sorted before b, or a positive value if b should be sorted
 * after a
 */
int string_compare_cstr_len(string *a, char *b, size_t b_len, string_compare_mode mode);

/**
 * Converts all characters in src to lower case and sets dst to the result. Source and destination may be the same.
 */
void string_tolower(string *dst, string *src);
/**
 * Converts all characters in src to upper case and sets dst to the result. Source and destination may be the same.
 */
void string_toupper(string *dst, string *src);

/**
 * Removes all instances of find from the start of src and sets dst to the result. Source and destination may be the same.
 */
void string_trim_start_char(string *dst, string *src, char find);
/**
 * Removes all instances of any character in find from the start of src and sets dst to the result. Source and destination may be the same.
 */
void string_trim_start_any_of_str(string *dst, string *src, string *find);
/**
 * Removes all instances of any character in find from the start of src and sets dst to the result. Source and destination may be the same.
 */
void string_trim_start_any_of_cstr(string *dst, string *src, char *find);
/**
 * Removes all instances of find from the end of src and sets dst to the result. Source and destination may be the same.
 */
void string_trim_end_char(string *dst, string *src, char find);
/**
 * Removes all instances of any character in find from the end of src and sets dst to the result. Source and destination may be the same.
 */
void string_trim_end_any_of_str(string *dst, string *src, string *find);
/**
 * Removes all instances of any character in find from the end of src and sets dst to the result. Source and destination may be the same.
 */
void string_trim_end_any_of_cstr(string *dst, string *src, char *find);
/**
 * Removes all instances of find from the start and end of src and sets dst to the result. Source and destination may be the same.
 */
void string_trim_char(string *dst, string *src, char find);
/**
 * Removes all instances of any character in find from the start and end of src and sets dst to the result. Source and destination may be
 * the same.
 */
void string_trim_any_of_str(string *dst, string *src, string *find);
/**
 * Removes all instances of any character in find from the start and end of src and sets dst to the result. Source and destination may be
 * the same.
 */
void string_trim_any_of_cstr(string *dst, string *src, char *find);

#ifdef __cplusplus
}
#endif

#endif