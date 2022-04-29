#include <assert.h>
#include <string.h>

#include "../shared/string.h"

int main() {
	string s;

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
	string_dealloc(&s);

	return 0;
}