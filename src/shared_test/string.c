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

	return 0;
}