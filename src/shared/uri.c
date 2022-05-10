#include "uri.h"
#include "log.h"

#include <string.h>

void uri_init(uri *u) {
	string_init(&u->scheme);
}

void uri_dealloc(uri *u) {
	string_dealloc(&u->scheme);
}

int uri_parse_str(uri *u, string *input, size_t index) {
	size_t len = string_get_length(input);
	if (index >= len) {
		return 1;
	}
	return uri_parse_cstr_len(u, string_get_cstr(input) + index, len - index);
}

int uri_parse_cstr(uri *u, char *input) {
	return uri_parse_cstr_len(u, input, strlen(input));
}

int uri_parse_cstr_len(uri *u, char *input, size_t input_length) {
	string scratch;
	string_init(&scratch);
	string_init_cstr_len(&scratch, input, input_length);
	log_trace("TODO JEFF implement uri_parse_cstr_len, input = %s\n", string_get_cstr(&scratch));

	// scheme
	// https://datatracker.ietf.org/doc/html/rfc3986#section-3.1
	size_t i = 0;
	char c = input[i];
	if ((c >= 'a' && c <= 'z') || c >= 'A' && c <= 'Z') {
		int found = 0;
		for (i = 1; i < input_length; i++) {
			c = input[i];
			if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '+' || c == '-' || c == '.') {
				continue;
			}
			if (c == ':' && i + 1 < input_length && input[i + 1] == '/') {
				string_init_cstr_len(&u->scheme, input, i);
				i++;
				string_tolower(&u->scheme);
				log_trace("TODO JEFF scheme = %s\n", string_get_cstr(&u->scheme));
				found = 1;
				break;
			}
		}
		if (!found) {
			string_clear(&u->scheme);
		}
	} else {
		string_clear(&u->scheme);
	}
	log_trace("TODO JEFF after parsing scheme, i = %zu\n", i);

	/*
	TODO JEFF remaining implementation
	authority
	path
	query
	fragment
	*/

	string_dealloc(&scratch);
	return 1;
}