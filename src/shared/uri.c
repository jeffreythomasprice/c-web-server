#include "uri.h"
#include "log.h"

#include <stdio.h>
#include <string.h>

void uri_append_decoded_str(string *dst, string *src) {
	uri_append_decoded_cstr_len(dst, string_get_cstr(src), string_get_length(src));
}

void uri_append_decoded_cstr(string *dst, char *src) {
	uri_append_decoded_cstr_len(dst, src, strlen(src));
}

void uri_append_decoded_cstr_len(string *dst, char *src, size_t src_len) {
	// TODO JEFF reserve size in dst ahead of time

	// if src is a substring in dst we have to copy it, otherwise a future realloc might move this pointer
	string temp;
	int should_dealloc_temp;
	if (src >= string_get_cstr(dst) && src < string_get_cstr(dst) + string_get_length(dst)) {
		string_init_cstr_len(&temp, src, src_len);
		should_dealloc_temp = 1;
		src = string_get_cstr(&temp);
	} else {
		should_dealloc_temp = 0;
	}

	for (size_t i = 0; i < src_len; i++, src++) {
		if (*src == '%') {
			if (i + 2 >= src_len) {
				goto not_escaped;
			}
			int high, low;
			high = src[1];
			low = src[2];
			if (high >= '0' && high <= '9') {
				high -= '0';
			} else if (high >= 'a' && high <= 'f') {
				high -= 'a';
				high += 10;
			} else if (high >= 'A' && high <= 'F') {
				high -= 'A';
				high += 10;
			} else {
				goto not_escaped;
			}
			if (low >= '0' && low <= '9') {
				low -= '0';
			} else if (low >= 'a' && low <= 'f') {
				low -= 'a';
				low += 10;
			} else if (low >= 'A' && low <= 'F') {
				low -= 'A';
				low += 10;
			} else {
				goto not_escaped;
			}
			char c = (high << 4) | low;
			string_append_cstr_len(dst, &c, 1);
			i += 2;
			src += 2;
			continue;
		}
	not_escaped:
		string_append_cstr_len(dst, src, 1);
	}

	if (should_dealloc_temp) {
		string_dealloc(&temp);
	}
}

void uri_append_encoded_str(string *dst, string *src) {
	uri_append_encoded_cstr_len(dst, string_get_cstr(src), string_get_length(src));
}

void uri_append_encoded_cstr(string *dst, char *src) {
	uri_append_encoded_cstr_len(dst, src, strlen(src));
}

void uri_append_encoded_cstr_len(string *dst, char *src, size_t src_len) {
	// TODO JEFF reserve size in dst ahead of time

	// if src is a substring in dst we have to copy it, otherwise a future realloc might move this pointer
	string temp;
	int should_dealloc_temp;
	if (src >= string_get_cstr(dst) && src < string_get_cstr(dst) + string_get_length(dst)) {
		string_init_cstr_len(&temp, src, src_len);
		should_dealloc_temp = 1;
		src = string_get_cstr(&temp);
	} else {
		should_dealloc_temp = 0;
	}

	for (size_t i = 0; i < src_len; i++, src++) {
		// https://datatracker.ietf.org/doc/html/rfc3986#section-2.2
		// reserved = ":" / "/" / "?" / "#" / "[" / "]" / "@" / "!" / "$" / "&" / "'" / "(" / ")" / "*" / "+" / "," / ";" / "="
		// https://datatracker.ietf.org/doc/html/rfc3986#section-2.3
		// unreserved  = ALPHA / DIGIT / "-" / "." / "_" / "~"
		char c = *src;
		if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '.' || c == '_' || c == '~' ||
			c == ':' || c == '/' || c == '?' || c == '#' || c == '[' || c == ']' || c == '@' || c == '!' || c == '$' || c == '&' ||
			c == '\'' || c == '(' || c == ')' || c == '*' || c == '+' || c == ',' || c == ';' || c == '=') {
			string_append_cstr_len(dst, src, 1);
		} else {
			char encoded[] = {'%', ((uint8_t)c) / 16, ((uint8_t)c) % 16};
			if (encoded[1] < 10) {
				encoded[1] += '0';
			} else {
				encoded[1] += 'A' - 10;
			}
			if (encoded[2] < 10) {
				encoded[2] += '0';
			} else {
				encoded[2] += 'A' - 10;
			}
			string_append_cstr_len(dst, encoded, 3);
		}
	}

	if (should_dealloc_temp) {
		string_dealloc(&temp);
	}
}

void uri_init(uri *u) {
	u->has_scheme = 0;
	string_init(&u->scheme);
	u->has_authority = 0;
	string_init(&u->authority);
	u->has_userinfo = 0;
	string_init(&u->userinfo);
	u->has_host = 0;
	string_init(&u->host);
	u->has_port = 0;
	string_init(&u->port_str);
	u->port = 0;
	u->has_path = 0;
	string_init(&u->path);
	u->has_query = 0;
	string_init(&u->query);
	u->has_fragment = 0;
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
	u->has_scheme = 0;
	string_clear(&u->scheme);
	u->has_authority = 0;
	string_clear(&u->authority);
	u->has_userinfo = 0;
	string_clear(&u->userinfo);
	u->has_host = 0;
	string_clear(&u->host);
	u->has_port = 0;
	string_clear(&u->port_str);
	u->port = 0;
	u->has_path = 0;
	string_clear(&u->path);
	u->has_query = 0;
	string_clear(&u->query);
	u->has_fragment = 0;
	string_clear(&u->fragment);

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
			// section 3.2 specifies that the authority must be preceeded by "//"
			// but several examples show a schema and authority spearated by only ":"
			// e.g. mailto:John.Doe@example.com
			if (c == ':') {
				i++;
				// we might have a "://" separator
				if (i + 2 <= input_length && input[i] == '/' && input[i + 1] == '/') {
					i += 2;
				}
				u->has_scheme = 1;
				string_init_cstr_len(&u->scheme, input, i);
				string_tolower(&u->scheme, &u->scheme);
				break;
			}
			// unrecognized character, and we didn't hit our separator so we don't have a scheme
			break;
		}
		if (!u->has_scheme) {
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
	// special case for where there is no host data but there is a port separator
	if (port_separator == 0 || (user_info_separator != -1 && port_separator == user_info_separator + 1)) {
		port_separator = -1;
	}
	size_t authority_len = i - authority_start;
	if (authority_len > 0) {
		u->has_authority = 1;
		string_init_cstr_len(&u->authority, input + authority_start, authority_len);

		// user info
		// https://datatracker.ietf.org/doc/html/rfc3986#section-3.2.1
		if (user_info_separator != -1) {
			u->has_userinfo = 1;
			string_set_cstr_len(&u->userinfo, input + authority_start, user_info_separator - authority_start);
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
		u->has_host = 1;
		string_set_cstr_len(&u->host, input + host_start, host_end - host_start);
		string_tolower(&u->host, &u->host);

		// degenerate case, bad host
		string_trim_any_of_cstr(&u->host, &u->host, " \t");
		if (string_get_length(&u->host) == 0) {
			u->has_host = 0;
			string_clear(&u->host);
		}

		// port
		// https://datatracker.ietf.org/doc/html/rfc3986#section-3.2.3
		if (u->has_host && port_separator != -1 && i - port_separator >= 1) {
			string_set_cstr_len(&u->port_str, input + port_separator + 1, i - port_separator - 1);
			int n;
			if (sscanf(string_get_cstr(&u->port_str), "%i%n", &u->port, &n) == 1 && u->port >= 0 && u->port < 65536 &&
				n == string_get_length(&u->port_str)) {
				u->has_port = 1;
			} else {
				host_end = i;
				string_set_cstr_len(&u->host, input + host_start, host_end - host_start);
				string_tolower(&u->host, &u->host);
				string_clear(&u->port_str);
				u->port = 0;
			}
		}
	}

	// path
	// https://datatracker.ietf.org/doc/html/rfc3986#section-3.3
	c = input[i];
	if (c == '/') {
		size_t start = i;
		i++;
		for (; i < input_length; i++) {
			c = input[i];
			if (c == '?' || c == '#') {
				break;
			}
		}
		size_t len = i - start;
		if (len >= 1) {
			u->has_path = 1;
			uri_append_decoded_cstr_len(&u->path, input + start, len);
		}
	}

	// query
	// https://datatracker.ietf.org/doc/html/rfc3986#section-3.4
	c = input[i];
	if (c == '?') {
		i++;
		size_t start = i;
		i++;
		for (; i < input_length; i++) {
			c = input[i];
			if (c == '#') {
				break;
			}
		}
		size_t len = i - start;
		if (len >= 1) {
			u->has_query = 1;
			uri_append_decoded_cstr_len(&u->query, input + start, len);
		}
	}

	// fragment
	// https://datatracker.ietf.org/doc/html/rfc3986#section-3.5
	c = input[i];
	if (c == '#' && i + 1 < input_length) {
		i++;
		size_t start = i;
		size_t len = input_length - start;
		i = input_length;
		if (len >= 1) {
			u->has_fragment = 1;
			uri_append_decoded_cstr_len(&u->fragment, input + start, len);
		}
	}

	// fail if we didn't parse anything
	if (!u->has_scheme && !u->has_host && !u->has_path && !u->has_query && !u->has_fragment) {
		return 1;
	}
	// fail if there were extra characters in the input
	if (i != input_length) {
		return 1;
	}

	return 0;
}

string *uri_get_scheme(uri *u) {
	if (u->has_scheme) {
		return &u->scheme;
	}
	return NULL;
}

string *uri_get_authority(uri *u) {
	if (u->has_authority) {
		return &u->authority;
	}
	return NULL;
}

string *uri_get_userinfo(uri *u) {
	if (u->has_userinfo) {
		return &u->userinfo;
	}
	return NULL;
}

string *uri_get_host(uri *u) {
	if (u->has_host) {
		return &u->host;
	}
	return NULL;
}

string *uri_get_port_str(uri *u) {
	if (u->has_port) {
		return &u->port_str;
	}
	return NULL;
}

int uri_get_port(uri *u) {
	if (u->has_port) {
		return u->port;
	}
	return 0;
}

string *uri_get_path(uri *u) {
	if (u->has_path) {
		return &u->path;
	}
	return NULL;
}

string *uri_get_query(uri *u) {
	if (u->has_query) {
		return &u->query;
	}
	return NULL;
}

string *uri_get_fragment(uri *u) {
	if (u->has_fragment) {
		return &u->fragment;
	}
	return NULL;
}

void uri_append_to_string(uri *u, string *s) {
	if (u->has_scheme) {
		string_append_str(s, &u->scheme);
	}
	if (u->has_userinfo) {
		string_append_str(s, &u->userinfo);
		string_append_cstr(s, "@");
	}
	if (u->has_host) {
		string_append_str(s, &u->host);
	}
	if (u->has_port) {
		string_append_cstrf(s, ":%d", u->port);
	}
	if (u->has_path) {
		uri_append_encoded_str(s, &u->path);
	}
	if (u->has_query) {
		string_append_cstr(s, "?");
		uri_append_encoded_str(s, &u->query);
	}
	if (u->has_fragment) {
		string_append_cstr(s, "#");
		uri_append_encoded_str(s, &u->fragment);
	}
}