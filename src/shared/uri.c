#include "uri.h"
#include "log.h"

#include <stdio.h>
#include <string.h>

void uri_init(uri *u) {
	string_init(&u->scheme);
	string_init(&u->authority);
	string_init(&u->userinfo);
	string_init(&u->host);
	string_init(&u->port_str);
	u->port = 0;
	string_init(&u->path);
	string_init(&u->query);
	string_init(&u->fragment);
}

void uri_dealloc(uri *u) {
	string_dealloc(&u->scheme);
	string_dealloc(&u->authority);
	string_dealloc(&u->userinfo);
	string_dealloc(&u->host);
	string_dealloc(&u->port_str);
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
	string_clear(&u->port_str);
	u->port = 0;
	string_clear(&u->path);
	string_clear(&u->query);
	string_clear(&u->fragment);

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
			// section 3.2 specifies that the authority must be preceeded by "//"
			// but several examples show a schema and authority spearated by only ":"
			// e.g. mailto:John.Doe@example.com
			if (c == ':') {
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

	// authority
	// https://datatracker.ietf.org/doc/html/rfc3986#section-3.2
	if (i + 1 < input_length && input[i] == '/' && input[i + 1] == '/') {
		i += 2;
	}
	size_t authority_start = i;
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
	string_init_cstr_len(&u->authority, input + authority_start, i - authority_start);
	log_trace("TODO JEFF authority = %s\n", string_get_cstr(&u->authority));

	// user info
	// https://datatracker.ietf.org/doc/html/rfc3986#section-3.2.1
	if (user_info_separator != -1) {
		string_set_cstr_len(&u->userinfo, input + authority_start, user_info_separator - authority_start);
		log_trace("TODO JEFF userinfo = %s\n", string_get_cstr(&u->userinfo));
	}

	// host
	// https://datatracker.ietf.org/doc/html/rfc3986#section-3.2.2
	size_t host_start;
	if (user_info_separator == -1) {
		host_start = authority_start;
	} else {
		host_start = user_info_separator + 1;
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

	// port
	// https://datatracker.ietf.org/doc/html/rfc3986#section-3.2.3
	if (port_separator != -1 && i - port_separator >= 1) {
		string_set_cstr_len(&u->port_str, input + port_separator + 1, i - port_separator - 1);
		log_trace("TODO JEFF port = %s\n", string_get_cstr(&u->port_str));
		int n;
		if (!(sscanf(string_get_cstr(&u->port_str), "%i%n", &u->port, &n) == 1 && u->port >= 0 && u->port < 65536 &&
			  n == string_get_length(&u->port_str))) {
			host_end = i;
			string_set_cstr_len(&u->host, input + host_start, host_end - host_start);
			string_tolower(&u->host);
			string_clear(&u->port_str);
			u->port = 0;
			log_trace("TODO JEFF port is invalid, host is actually %s\n", string_get_cstr(&u->host));
		}
	}

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

	// fragment
	// https://datatracker.ietf.org/doc/html/rfc3986#section-3.5
	if (i < input_length) {
		string_set_cstr_len(&u->fragment, input + i, input_length - i);
		log_trace("TODO JEFF fragment = %s\n", string_get_cstr(&u->fragment));
	}

	// fail if we didn't parse anything
	if (string_get_length(&u->scheme) == 0 && string_get_length(&u->authority) == 0 && string_get_length(&u->path) == 0 &&
		string_get_length(&u->query) == 0 && string_get_length(&u->fragment) == 0) {
		return 1;
	}
	// fail if there were extra characters in the input
	if (i != input_length) {
		return 1;
	}

	return 0;
}