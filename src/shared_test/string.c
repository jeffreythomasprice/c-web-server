#include <assert.h>
#include <string.h>

#include "../shared/string.h"

int main() {
	string s, s2, s3;

	string_init(&s);
	assert(string_get_length(&s) == 0);
	assert(!strcmp(string_get_cstr(&s), ""));
	string_dealloc(&s);

	string_init_cstr(&s, "Hello, World!");
	assert(string_get_length(&s) == 13);
	assert(!strcmp(string_get_cstr(&s), "Hello, World!"));
	string_dealloc(&s);

	string_init(&s);
	string_set_length(&s, 5, '*');
	assert(string_get_length(&s) == 5);
	assert(!strcmp(string_get_cstr(&s), "*****"));
	string_set_length(&s, 4, '_');
	assert(string_get_length(&s) == 4);
	assert(!strcmp(string_get_cstr(&s), "****"));
	string_set_length(&s, 8, '~');
	assert(string_get_length(&s) == 8);
	assert(!strcmp(string_get_cstr(&s), "****~~~~"));
	string_set_length(&s, 10, '_');
	assert(string_get_length(&s) == 10);
	assert(!strcmp(string_get_cstr(&s), "****~~~~__"));
	string_set_length(&s, 7, '&');
	assert(string_get_length(&s) == 7);
	assert(!strcmp(string_get_cstr(&s), "****~~~"));
	string_dealloc(&s);

	string_init_cstr_len(&s, "abcdefghijklmnopqrstuvwxyz", 20);
	assert(string_get_length(&s) == 20);
	assert(!strcmp(string_get_cstr(&s), "abcdefghijklmnopqrst"));

	string_clear(&s);

	string_init_cstr(&s2, "abcd");
	string_append_str(&s, &s2);
	string_dealloc(&s2);

	string_append_cstrf(&s, " foo-%d-bar %s", 42, "Hello, World!");

	assert(!strcmp(string_get_cstr(&s), "abcd foo-42-bar Hello, World!"));

	string_clear(&s);
	string_append_cstr(&s, "foo %d %zu %s");
	string_append_cstr_len(&s, "%llu %lu", 4);
	assert(!strcmp(string_get_cstr(&s), "foo %d %zu %s%llu"));

	string_set_cstr(&s, "%d %d");
	assert(!strcmp(string_get_cstr(&s), "%d %d"));

	string_set_cstr_len(&s, "%d %d", 2);
	assert(!strcmp(string_get_cstr(&s), "%d"));

	string_dealloc(&s);

	string_init_cstr(&s, "abc  def   gehi    jk");

	size_t split_results[10];
	string_init_cstr(&s2, "  ");
	size_t num_split_results = string_split(&s, 0, "  ", split_results, 5, 0);
	assert(num_split_results == 5);
	assert(split_results[0] == 0);
	assert(split_results[1] == 3);
	assert(split_results[2] == 5);
	assert(split_results[3] == 8);
	assert(split_results[4] == 10);
	assert(split_results[5] == 15);
	assert(split_results[6] == 17);
	assert(split_results[7] == 17);
	assert(split_results[8] == 19);
	assert(split_results[9] == 21);
	num_split_results = string_split(&s, 0, "  ", split_results, 10, 3);
	assert(num_split_results == 3);
	assert(split_results[0] == 0);
	assert(split_results[1] == 3);
	assert(split_results[2] == 5);
	assert(split_results[3] == 8);
	assert(split_results[4] == 10);
	assert(split_results[5] == 21);
	num_split_results = string_split(&s, 0, "     ", split_results, 10, 0);
	assert(num_split_results == 1);
	assert(split_results[0] == 0);
	assert(split_results[1] == 21);
	string_dealloc(&s2);

	string_set_cstr(&s, "01-23-45-67-89");

	num_split_results = string_split(&s, 0, "-", split_results, 10, 3);
	assert(num_split_results == 3);
	assert(split_results[0] == 0);
	assert(split_results[1] == 2);
	assert(split_results[2] == 3);
	assert(split_results[3] == 5);
	assert(split_results[4] == 6);
	assert(split_results[5] == 14);

	num_split_results = string_split(&s, 0, "-", split_results, 2, 3);
	assert(num_split_results == 3);
	assert(split_results[0] == 0);
	assert(split_results[1] == 2);
	assert(split_results[2] == 3);
	assert(split_results[3] == 5);

	num_split_results = string_split(&s, 1, "-", split_results, 3, 3);
	assert(num_split_results == 3);
	assert(split_results[0] == 1);
	assert(split_results[1] == 2);
	assert(split_results[2] == 3);
	assert(split_results[3] == 5);
	assert(split_results[4] == 6);
	assert(split_results[5] == 14);

	num_split_results = string_split(&s, 13, "-", split_results, 3, 3);
	assert(num_split_results == 1);
	assert(split_results[0] == 13);
	assert(split_results[1] == 14);

	string_dealloc(&s);

	string_init_cstr(&s, "abc def geh");
	string_init(&s2);

	string_clear(&s2);
	string_append_substr(&s2, &s, 8, 11);
	assert(!strcmp(string_get_cstr(&s2), "geh"));
	string_append_substr(&s2, &s, 4, 7);
	assert(!strcmp(string_get_cstr(&s2), "gehdef"));
	string_append_substr(&s2, &s, 0, 3);
	assert(!strcmp(string_get_cstr(&s2), "gehdefabc"));
	string_append_substr(&s2, &s, 5, 25);
	assert(!strcmp(string_get_cstr(&s2), "gehdefabcef geh"));
	string_append_substr(&s2, &s, 75, 0);
	string_append_substr(&s2, &s, 75, 80);
	assert(!strcmp(string_get_cstr(&s2), "gehdefabcef geh"));

	string_dealloc(&s);
	string_dealloc(&s2);

	string_init(&s);

	string_init_cstr(&s2, "foobarbaz");
	string_set_str(&s, &s2);
	assert(!strcmp(string_get_cstr(&s), "foobarbaz"));
	string_dealloc(&s2);
	string_set_cstrf(&s, "blah %d", 42);
	assert(!strcmp(string_get_cstr(&s), "blah 42"));
	string_dealloc(&s);

	string_init(&s);
	string_init_cstr(&s2, "foobarbaz");
	string_set_substr(&s, &s2, 3, 6);
	assert(!strcmp(string_get_cstr(&s), "bar"));
	string_dealloc(&s);
	string_dealloc(&s2);

	string_init_cstr(&s, "foobarbaz");
	string_append_substr(&s, &s, 3, 6);
	assert(!strcmp(string_get_cstr(&s), "foobarbazbar"));
	string_append_substr(&s, &s, 6, 9);
	assert(!strcmp(string_get_cstr(&s), "foobarbazbarbaz"));
	string_set_substr(&s, &s, 3, 12);
	assert(!strcmp(string_get_cstr(&s), "barbazbar"));
	string_dealloc(&s);

	string_init_cstr(&s, "foofoobarbazbaz");
	string_init_cstr(&s2, "bar");
	assert(string_index_of_str(&s, &s2, 0) == 6);
	assert(string_index_of_str(&s, &s2, 6) == 6);
	assert(string_index_of_str(&s, &s2, 7) == -1);
	assert(string_reverse_index_of_str(&s, &s2, 20) == 6);
	assert(string_reverse_index_of_str(&s, &s2, 8) == 6);
	assert(string_reverse_index_of_str(&s, &s2, 6) == 6);
	assert(string_reverse_index_of_str(&s, &s2, 5) == -1);
	string_set_cstrf(&s2, "baz");
	assert(string_index_of_str(&s, &s2, 0) == 9);
	assert(string_index_of_str(&s, &s2, 9) == 9);
	assert(string_index_of_str(&s, &s2, 10) == 12);
	assert(string_reverse_index_of_str(&s, &s2, 20) == 12);
	assert(string_reverse_index_of_str(&s, &s2, 12) == 12);
	assert(string_reverse_index_of_str(&s, &s2, 11) == 9);
	assert(string_reverse_index_of_str(&s, &s2, 9) == 9);
	assert(string_reverse_index_of_str(&s, &s2, 8) == -1);
	string_set_cstrf(&s2, "whizbang");
	assert(string_index_of_str(&s, &s2, 0) == -1);
	string_set_cstrf(&s2, "whizbangwhizbangwhizbangwhizbang");
	assert(string_index_of_str(&s, &s2, 0) == -1);
	string_set_cstrf(&s2, "");
	assert(string_index_of_str(&s, &s2, 0) == -1);
	string_dealloc(&s2);
	assert(string_index_of_cstr(&s, "foo", 0) == 0);
	assert(string_index_of_cstr(&s, "foo", 1) == 3);
	assert(string_index_of_cstr(&s, "foo", 4) == -1);
	assert(string_index_of_cstr(&s, "bar", 0) == 6);
	assert(string_index_of_cstr(&s, "bar", 6) == 6);
	assert(string_index_of_cstr(&s, "bar", 7) == -1);
	assert(string_index_of_cstr(&s, "baz", 0) == 9);
	assert(string_index_of_cstr(&s, "baz", 9) == 9);
	assert(string_index_of_cstr(&s, "baz", 10) == 12);
	assert(string_index_of_cstr(&s, "baz", 13) == -1);
	assert(string_index_of_cstr(&s, "blarg", 0) == -1);
	assert(string_index_of_cstr(&s, "foofoobarbazbaz2", 0) == -1);
	assert(string_reverse_index_of_cstr(&s, "foo", 20) == 3);
	assert(string_reverse_index_of_cstr(&s, "foo", 3) == 3);
	assert(string_reverse_index_of_cstr(&s, "foo", 2) == 0);
	assert(string_reverse_index_of_cstr(&s, "foo", 0) == 0);
	assert(string_index_of_cstr(&s, "", 0) == -1);
	assert(string_index_of_char(&s, 'f', 0) == 0);
	assert(string_index_of_char(&s, 'o', 0) == 1);
	assert(string_index_of_char(&s, 'a', 0) == 7);
	assert(string_index_of_char(&s, 'y', 0) == -1);
	assert(string_reverse_index_of_char(&s, 'f', 20) == 3);
	assert(string_reverse_index_of_char(&s, 'f', 1) == 0);
	assert(string_reverse_index_of_char(&s, 'f', 0) == 0);
	assert(string_reverse_index_of_char(&s, 'y', 0) == -1);
	string_dealloc(&s);

	string_init_cstr(&s, " \t foo bar baz   \t");
	string_init_cstr(&s2, " \t");
	assert(string_index_of_any_str(&s, &s2, 0) == 0);
	assert(string_index_of_any_str(&s, &s2, 3) == 6);
	assert(string_index_of_any_str(&s, &s2, 6) == 6);
	assert(string_index_of_any_cstr(&s, " \t", 0) == 0);
	assert(string_index_of_any_cstr(&s, " \t", 3) == 6);
	assert(string_index_of_any_cstr(&s, " \t", 6) == 6);
	assert(string_reverse_index_of_any_str(&s, &s2, 18) == 17);
	assert(string_reverse_index_of_any_str(&s, &s2, 13) == 10);
	assert(string_reverse_index_of_any_str(&s, &s2, 10) == 10);
	assert(string_reverse_index_of_any_cstr(&s, " \t", 18) == 17);
	assert(string_reverse_index_of_any_cstr(&s, " \t", 13) == 10);
	assert(string_reverse_index_of_any_cstr(&s, " \t", 10) == 10);
	assert(string_index_not_of_any_str(&s, &s2, 0) == 3);
	assert(string_index_not_of_any_str(&s, &s2, 3) == 3);
	assert(string_index_not_of_any_str(&s, &s2, 6) == 7);
	assert(string_index_not_of_any_cstr(&s, " \t", 0) == 3);
	assert(string_index_not_of_any_cstr(&s, " \t", 3) == 3);
	assert(string_index_not_of_any_cstr(&s, " \t", 6) == 7);
	assert(string_reverse_index_not_of_any_str(&s, &s2, 18) == 13);
	assert(string_reverse_index_not_of_any_str(&s, &s2, 13) == 13);
	assert(string_reverse_index_not_of_any_str(&s, &s2, 10) == 9);
	assert(string_reverse_index_not_of_any_cstr(&s, " \t", 18) == 13);
	assert(string_reverse_index_not_of_any_cstr(&s, " \t", 13) == 13);
	assert(string_reverse_index_not_of_any_cstr(&s, " \t", 10) == 9);
	string_dealloc(&s2);
	string_dealloc(&s);

	string_init_cstr(&s, "foobarbaz");
	string_init_cstr(&s2, "foobarbaz");
	assert(string_compare_str(&s, &s2, STRING_COMPARE_CASE_SENSITIVE) == 0);
	assert(string_compare_str(&s, &s2, STRING_COMPARE_CASE_INSENSITIVE) == 0);
	string_set_cstr(&s2, "fOoBaRBaZ");
	assert(string_compare_str(&s, &s2, STRING_COMPARE_CASE_SENSITIVE) > 0);
	assert(string_compare_str(&s, &s2, STRING_COMPARE_CASE_INSENSITIVE) == 0);
	string_set_cstr(&s, "FOObarbaz");
	assert(string_compare_str(&s, &s2, STRING_COMPARE_CASE_SENSITIVE) < 0);
	assert(string_compare_str(&s, &s2, STRING_COMPARE_CASE_INSENSITIVE) == 0);
	string_set_cstr(&s, "foobarbaz");
	assert(string_compare_cstr(&s, "foobarbaz", STRING_COMPARE_CASE_SENSITIVE) == 0);
	assert(string_compare_cstr(&s, "foobarbaz", STRING_COMPARE_CASE_INSENSITIVE) == 0);
	assert(string_compare_cstr(&s, "fOoBaRBaZ", STRING_COMPARE_CASE_SENSITIVE) > 0);
	assert(string_compare_cstr(&s, "fOoBaRBaZ", STRING_COMPARE_CASE_INSENSITIVE) == 0);
	assert(string_compare_cstr_len(&s, "foobarbazbaz", 12, STRING_COMPARE_CASE_SENSITIVE) < 0);
	assert(string_compare_cstr_len(&s, "foobarbazbaz", 12, STRING_COMPARE_CASE_INSENSITIVE) < 0);
	assert(string_compare_cstr_len(&s, "foobarbazbaz", 9, STRING_COMPARE_CASE_SENSITIVE) == 0);
	assert(string_compare_cstr_len(&s, "foobarbazbaz", 9, STRING_COMPARE_CASE_INSENSITIVE) == 0);
	assert(string_compare_cstr_len(&s, "fOoBaRBaZbaz", 12, STRING_COMPARE_CASE_SENSITIVE) > 0);
	assert(string_compare_cstr_len(&s, "fOoBaRBaZbaz", 12, STRING_COMPARE_CASE_INSENSITIVE) < 0);
	assert(string_compare_cstr_len(&s, "fOoBaRBaZbaz", 9, STRING_COMPARE_CASE_SENSITIVE) > 0);
	assert(string_compare_cstr_len(&s, "fOoBaRBaZbaz", 9, STRING_COMPARE_CASE_INSENSITIVE) == 0);
	string_dealloc(&s);
	string_dealloc(&s2);

	string_init_cstr(&s, "fOoBaRbAz");
	string_init(&s2);
	string_tolower(&s, &s);
	assert(string_compare_cstr(&s, "foobarbaz", STRING_COMPARE_CASE_SENSITIVE) == 0);
	string_set_cstr(&s, "fOoBaRbAz");
	string_toupper(&s, &s);
	assert(string_compare_cstr(&s, "FOOBARBAZ", STRING_COMPARE_CASE_SENSITIVE) == 0);
	string_set_cstr(&s, "fOoBaRbAz");
	string_set_cstr(&s2, "fffffffff");
	string_tolower(&s2, &s);
	assert(string_compare_cstr(&s, "fOoBaRbAz", STRING_COMPARE_CASE_SENSITIVE) == 0);
	assert(string_compare_cstr(&s2, "foobarbaz", STRING_COMPARE_CASE_SENSITIVE) == 0);
	string_set_cstr(&s, "fOoBaRbAz");
	string_set_cstr(&s2, "fffffffff");
	string_toupper(&s2, &s);
	assert(string_compare_cstr(&s, "fOoBaRbAz", STRING_COMPARE_CASE_SENSITIVE) == 0);
	assert(string_compare_cstr(&s2, "FOOBARBAZ", STRING_COMPARE_CASE_SENSITIVE) == 0);
	string_dealloc(&s);
	string_dealloc(&s2);

	string_init_cstr(&s, "  \tfoobar \t  ");
	string_init(&s2);
	string_init(&s3);
	string_trim_start_char(&s2, &s, ' ');
	assert(string_compare_cstr(&s2, "\tfoobar \t  ", STRING_COMPARE_CASE_SENSITIVE) == 0);
	string_trim_start_char(&s2, &s, 'X');
	assert(string_compare_cstr(&s2, "  \tfoobar \t  ", STRING_COMPARE_CASE_SENSITIVE) == 0);
	string_trim_end_char(&s2, &s, ' ');
	assert(string_compare_cstr(&s2, "  \tfoobar \t", STRING_COMPARE_CASE_SENSITIVE) == 0);
	string_trim_end_char(&s2, &s, 'X');
	assert(string_compare_cstr(&s2, "  \tfoobar \t  ", STRING_COMPARE_CASE_SENSITIVE) == 0);
	string_trim_char(&s2, &s, ' ');
	assert(string_compare_cstr(&s2, "\tfoobar \t", STRING_COMPARE_CASE_SENSITIVE) == 0);
	string_trim_char(&s2, &s, 'X');
	assert(string_compare_cstr(&s2, "  \tfoobar \t  ", STRING_COMPARE_CASE_SENSITIVE) == 0);
	string_set_cstr(&s3, " \t");
	string_trim_start_any_of_str(&s2, &s, &s3);
	assert(string_compare_cstr(&s2, "foobar \t  ", STRING_COMPARE_CASE_SENSITIVE) == 0);
	string_set_cstr(&s3, "X ");
	string_trim_start_any_of_str(&s2, &s, &s3);
	assert(string_compare_cstr(&s2, "\tfoobar \t  ", STRING_COMPARE_CASE_SENSITIVE) == 0);
	string_set_cstr(&s3, " \t");
	string_trim_end_any_of_str(&s2, &s, &s3);
	assert(string_compare_cstr(&s2, "  \tfoobar", STRING_COMPARE_CASE_SENSITIVE) == 0);
	string_set_cstr(&s3, "X ");
	string_trim_end_any_of_str(&s2, &s, &s3);
	assert(string_compare_cstr(&s2, "  \tfoobar \t", STRING_COMPARE_CASE_SENSITIVE) == 0);
	string_set_cstr(&s3, " \t");
	string_trim_any_of_str(&s2, &s, &s3);
	assert(string_compare_cstr(&s2, "foobar", STRING_COMPARE_CASE_SENSITIVE) == 0);
	string_set_cstr(&s3, "X ");
	string_trim_any_of_str(&s2, &s, &s3);
	assert(string_compare_cstr(&s2, "\tfoobar \t", STRING_COMPARE_CASE_SENSITIVE) == 0);
	string_trim_start_any_of_cstr(&s2, &s, " \t");
	assert(string_compare_cstr(&s2, "foobar \t  ", STRING_COMPARE_CASE_SENSITIVE) == 0);
	string_trim_start_any_of_cstr(&s2, &s, "X ");
	assert(string_compare_cstr(&s2, "\tfoobar \t  ", STRING_COMPARE_CASE_SENSITIVE) == 0);
	string_trim_end_any_of_cstr(&s2, &s, " \t");
	assert(string_compare_cstr(&s2, "  \tfoobar", STRING_COMPARE_CASE_SENSITIVE) == 0);
	string_trim_end_any_of_cstr(&s2, &s, "X ");
	assert(string_compare_cstr(&s2, "  \tfoobar \t", STRING_COMPARE_CASE_SENSITIVE) == 0);
	string_trim_any_of_cstr(&s2, &s, " \t");
	assert(string_compare_cstr(&s2, "foobar", STRING_COMPARE_CASE_SENSITIVE) == 0);
	string_trim_any_of_cstr(&s2, &s, "X ");
	assert(string_compare_cstr(&s2, "\tfoobar \t", STRING_COMPARE_CASE_SENSITIVE) == 0);
	string_dealloc(&s);
	string_dealloc(&s2);

	return 0;
}