#include "uri.h"
#include "log.h"

#include <string.h>

void uri_init(uri *u) {
	string_init(&u->scheme);
	string_init(&u->authority);
	string_init(&u->path);
	string_init(&u->query);
	string_init(&u->fragment);
}

void uri_dealloc(uri *u) {
	string_dealloc(&u->scheme);
	string_dealloc(&u->authority);
	string_dealloc(&u->path);
	string_dealloc(&u->query);
	string_dealloc(&u->fragment);
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
	string_clear(&u->scheme);
	string_clear(&u->authority);
	string_clear(&u->path);
	string_clear(&u->query);
	string_clear(&u->fragment);

	string scratch;
	string_init(&scratch);
	string_init_cstr_len(&scratch, input, input_length);
	log_trace("TODO JEFF implement uri_parse_cstr_len, input = %s\n", string_get_cstr(&scratch));

	// scheme
	// https://datatracker.ietf.org/doc/html/rfc3986#section-3.1
	size_t i = 0;
	char c = input[i];
	if ((c >= 'a' && c <= 'z') || c >= 'A' && c <= 'Z') {
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
				break;
			}
		}
	}

	if (i < input_length) {
		string_init_cstr_len(&scratch, input + i, input_length - i);
	} else {
		string_clear(&scratch);
	}
	// log_trace("TODO JEFF remaining string at i=%zu, %s\n", i, string_get_cstr(&scratch));

	// authority
	// https://datatracker.ietf.org/doc/html/rfc3986#section-3.2
	if (i + 1 < input_length && input[i] == '/' && input[i + 1] == '/') {
		size_t start = i + 2;
		i += 2;
		for (; i < input_length; i++) {
			c = input[i];
			if (c == '/' || c == '?' || c == '#') {
				break;
			}
		}
		string_init_cstr_len(&u->authority, input + start, i - start);
		log_trace("TODO JEFF authority = %s\n", string_get_cstr(&u->authority));

		// TODO JEFF user info
		// https://datatracker.ietf.org/doc/html/rfc3986#section-3.2.1

		// TODO JEFF host
		// https://datatracker.ietf.org/doc/html/rfc3986#section-3.2.2

		// TODO JEFF port
		// https://datatracker.ietf.org/doc/html/rfc3986#section-3.2.3
	}

	if (i < input_length) {
		string_init_cstr_len(&scratch, input + i, input_length - i);
	} else {
		string_clear(&scratch);
	}
	// log_trace("TODO JEFF remaining string at i=%zu, %s\n", i, string_get_cstr(&scratch));

	// path
	// https://datatracker.ietf.org/doc/html/rfc3986#section-3.3
	c = input[i];
	if (c != '?' && c != '#') {
		size_t start = i;
		i++;
		for (; i < input_length; i++) {
			c = input[i];
			if (c == '?' || c == '#') {
				break;
			}
		}
		if (i - start > 1) {
			string_set_cstr_len(&u->path, input + start, i - start);
			log_trace("TODO JEFF path = %s\n", string_get_cstr(&u->path));
		}
	}

	if (i < input_length) {
		string_init_cstr_len(&scratch, input + i, input_length - i);
	} else {
		string_clear(&scratch);
	}
	// log_trace("TODO JEFF remaining string at i=%zu, %s\n", i, string_get_cstr(&scratch));

	// query
	// https://datatracker.ietf.org/doc/html/rfc3986#section-3.4
	c = input[i];
	if (c != '#') {
		size_t start = i;
		i++;
		for (; i < input_length; i++) {
			c = input[i];
			if (c == '#') {
				break;
			}
		}
		if (i - start > 1) {
			string_set_cstr_len(&u->query, input + start, i - start);
			log_trace("TODO JEFF query = %s\n", string_get_cstr(&u->query));
		}
	}

	if (i < input_length) {
		string_init_cstr_len(&scratch, input + i, input_length - i);
	} else {
		string_clear(&scratch);
	}
	// log_trace("TODO JEFF remaining string at i=%zu, %s\n", i, string_get_cstr(&scratch));

	// fragment
	// https://datatracker.ietf.org/doc/html/rfc3986#section-3.5
	if (i < input_length) {
		string_set_cstr_len(&u->fragment, input + i, input_length - i);
		log_trace("TODO JEFF fragment = %s\n", string_get_cstr(&u->fragment));
	}

	string_dealloc(&scratch);
	return 1;
}