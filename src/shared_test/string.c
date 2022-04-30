#include <assert.h>
#include <string.h>

#include "../shared/string.h"

int main() {
	string s, s2;

	string_init(&s);
	assert(string_get_length(&s) == 0);
	assert(!strcmp(string_get_cstr(&s), ""));
	string_dealloc(&s);

	string_init_cstr(&s, "Hello, World!");
	assert(string_get_length(&s) == 13);
	assert(!strcmp(string_get_cstr(&s), "Hello, World!"));
	string_dealloc(&s);

	string_init_cstr_len(&s, "abcdefghijklmnopqrstuvwxyz", 20);
	assert(string_get_length(&s) == 20);
	assert(!strcmp(string_get_cstr(&s), "abcdefghijklmnopqrst"));

	string_clear(&s);

	string_init_cstr(&s2, "abcd");
	string_appends(&s, &s2);
	string_dealloc(&s2);

	string_appendf(&s, " foo-%d-bar %s", 42, "Hello, World!");

	assert(!strcmp(string_get_cstr(&s), "abcd foo-42-bar Hello, World!"));

	string_dealloc(&s);

	string_init_cstr(&s, "abc  def   gehi    jk");

	size_t split_results[10];
	string_init_cstr(&s2, "  ");
	size_t num_split_results = string_split(&s, "  ", split_results, 5, 0);
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
	num_split_results = string_split(&s, "  ", split_results, 10, 3);
	assert(num_split_results == 3);
	assert(split_results[0] == 0);
	assert(split_results[1] == 3);
	assert(split_results[2] == 5);
	assert(split_results[3] == 8);
	assert(split_results[4] == 10);
	assert(split_results[5] == 21);
	num_split_results = string_split(&s, "     ", split_results, 10, 0);
	assert(num_split_results == 1);
	assert(split_results[0] == 0);
	assert(split_results[1] == 21);
	string_dealloc(&s2);

	string_dealloc(&s);

	string_init_cstr(&s, "abc def geh");
	string_init(&s2);

	string_clear(&s2);
	string_append_substr(&s2, &s, 8, 11);
	string_append_substr(&s2, &s, 4, 7);
	string_append_substr(&s2, &s, 0, 3);
	string_append_substr(&s2, &s, 5, 25);
	string_append_substr(&s2, &s, 75, 0);
	string_append_substr(&s2, &s, 75, 80);
	assert(!strcmp(string_get_cstr(&s2), "gehdefabcef geh"));

	string_dealloc(&s);
	string_dealloc(&s2);

	string_init(&s);

	string_init_cstr(&s2, "foobarbaz");
	string_sets(&s, &s2);
	assert(!strcmp(string_get_cstr(&s), "foobarbaz"));
	string_dealloc(&s2);
	string_setf(&s, "blah %d", 42);
	assert(!strcmp(string_get_cstr(&s), "blah 42"));
	string_dealloc(&s);

	string_init(&s);
	string_init_cstr(&s2, "foobarbaz");
	string_set_substr(&s, &s2, 3, 6);
	assert(!strcmp(string_get_cstr(&s), "bar"));
	string_dealloc(&s);
	string_dealloc(&s2);

	string_init_cstr(&s, "foofoobarbazbaz");
	string_init_cstr(&s2, "bar");
	assert(string_index_of_str(&s, &s2, 0) == 6);
	assert(string_index_of_str(&s, &s2, 6) == 6);
	assert(string_index_of_str(&s, &s2, 7) == -1);
	assert(string_reverse_index_of_str(&s, &s2, 20) == 6);
	assert(string_reverse_index_of_str(&s, &s2, 8) == 6);
	assert(string_reverse_index_of_str(&s, &s2, 6) == 6);
	assert(string_reverse_index_of_str(&s, &s2, 5) == -1);
	string_setf(&s2, "baz");
	assert(string_index_of_str(&s, &s2, 0) == 9);
	assert(string_index_of_str(&s, &s2, 9) == 9);
	assert(string_index_of_str(&s, &s2, 10) == 12);
	assert(string_reverse_index_of_str(&s, &s2, 20) == 12);
	assert(string_reverse_index_of_str(&s, &s2, 12) == 12);
	assert(string_reverse_index_of_str(&s, &s2, 11) == 9);
	assert(string_reverse_index_of_str(&s, &s2, 9) == 9);
	assert(string_reverse_index_of_str(&s, &s2, 8) == -1);
	string_setf(&s2, "whizbang");
	assert(string_index_of_str(&s, &s2, 0) == -1);
	string_setf(&s2, "whizbangwhizbangwhizbangwhizbang");
	assert(string_index_of_str(&s, &s2, 0) == -1);
	string_setf(&s2, "");
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

	return 0;
}