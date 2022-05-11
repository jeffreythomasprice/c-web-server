#include "uri.h"
#include "log.h"

#include <string.h>

void uri_init(uri *u) {
	string_init(&u->scheme);
	string_init(&u->authority);
	string_init(&u->userinfo);
	string_init(&u->host);
	string_init(&u->port);
	string_init(&u->path);
	string_init(&u->query);
	string_init(&u->fragment);
}

void uri_dealloc(uri *u) {
	string_dealloc(&u->scheme);
	string_dealloc(&u->authority);
	string_dealloc(&u->userinfo);
	string_dealloc(&u->host);
	string_dealloc(&u->port);
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
	string_clear(&u->userinfo);
	string_clear(&u->host);
	string_clear(&u->port);
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
	int found_scheme = 0;
	char c = input[i];
	if ((c >= 'a' && c <= 'z') || c >= 'A' && c <= 'Z') {
		for (i = 1; i < input_length; i++) {
			c = input[i];
			if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '+' || c == '-' || c == '.') {
				continue;
			}
			// TODO JEFF scheme doesn't necessarily have to be followed by "//", could just be ":"
			if (c == ':' && i + 1 < input_length && input[i + 1] == '/') {
				found_scheme = 1;
				string_init_cstr_len(&u->scheme, input, i);
				i++;
				string_tolower(&u->scheme);
				log_trace("TODO JEFF scheme = %s\n", string_get_cstr(&u->scheme));
				break;
			}
		}
		if (!found_scheme) {
			i = 0;
		}
	}

	if (i < input_length) {
		string_init_cstr_len(&scratch, input + i, input_length - i);
	} else {
		string_clear(&scratch);
	}
	// log_trace("TODO JEFF (post-scheme) remaining string at i=%zu, %s\n", i, string_get_cstr(&scratch));

	// authority
	// https://datatracker.ietf.org/doc/html/rfc3986#section-3.2
	if ((found_scheme && i + 1 < input_length && input[i] == '/' && input[i + 1] == '/') || !found_scheme) {
		size_t start;
		if (found_scheme) {
			start = i + 2;
			i += 2;
		} else {
			start = i;
		}
		size_t user_info_separator = -1;
		size_t port_separator = -1;
		for (; i < input_length; i++) {
			c = input[i];
			// user info, if provided, is before the first instance of this separator
			if (c == '@' && user_info_separator == -1) {
				user_info_separator = i;
			}
			// port, if provided, is after the last instance of this separator
			else if (c == ':') {
				port_separator = i;
			} else if (c == '/' || c == '?' || c == '#') {
				break;
			}
		}
		// special case for when there isn't a port, but that separator appears in the user info
		if (user_info_separator != -1 && port_separator != -1 && port_separator < user_info_separator) {
			port_separator = -1;
		}
		string_init_cstr_len(&u->authority, input + start, i - start);
		log_trace("TODO JEFF authority = %s\n", string_get_cstr(&u->authority));

		// TODO JEFF user info
		// https://datatracker.ietf.org/doc/html/rfc3986#section-3.2.1
		if (user_info_separator != -1) {
			string_set_cstr_len(&u->userinfo, input + start, user_info_separator - start);
			log_trace("TODO JEFF userinfo = %s\n", string_get_cstr(&u->userinfo));
		}

		// TODO JEFF host
		// https://datatracker.ietf.org/doc/html/rfc3986#section-3.2.2
		size_t host_start;
		if (user_info_separator == -1) {
			host_start = start;
		} else {
			host_start = user_info_separator;
		}
		size_t host_end;
		if (port_separator == -1) {
			host_end = i;
		} else {
			host_end = port_separator;
		}
		string_set_cstr_len(&u->host, input + host_start, host_end - host_start);
		string_tolower(&u->host);
		log_trace("TODO JEFF host = %s\n", string_get_cstr(&u->host));

		// TODO JEFF port
		// https://datatracker.ietf.org/doc/html/rfc3986#section-3.2.3
		if (port_separator != -1) {
			string_set_cstr_len(&u->port, input + port_separator, i - port_separator);
			log_trace("TODO JEFF port = %s\n", string_get_cstr(&u->port));
		}
	}

	if (i < input_length) {
		string_init_cstr_len(&scratch, input + i, input_length - i);
	} else {
		string_clear(&scratch);
	}
	// log_trace("TODO JEFF (post-authority) remaining string at i=%zu, %s\n", i, string_get_cstr(&scratch));

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
	// log_trace("TODO JEFF (post-path) remaining string at i=%zu, %s\n", i, string_get_cstr(&scratch));

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
	// log_trace("TODO JEFF (post-query) remaining string at i=%zu, %s\n", i, string_get_cstr(&scratch));

	// fragment
	// https://datatracker.ietf.org/doc/html/rfc3986#section-3.5
	if (i < input_length) {
		string_set_cstr_len(&u->fragment, input + i, input_length - i);
		log_trace("TODO JEFF fragment = %s\n", string_get_cstr(&u->fragment));
	}

	string_dealloc(&scratch);
	return 1;
}