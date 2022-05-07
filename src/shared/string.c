#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "string.h"

void string_init(string *s) {
	buffer_init(&s->b);
	buffer_set_length(&s->b, 1);
	s->b.data[0] = 0;
}

void string_init_cstr(string *s, char *c) {
	string_init_cstr_len(s, c, strlen(c));
}

void string_init_cstr_len(string *s, char *c, size_t len) {
	buffer_init(&s->b);
	buffer_set_capacity(&s->b, len + 1);
	buffer_append_bytes(&s->b, c, len);
	buffer_set_length(&s->b, len + 1);
	s->b.data[len] = 0;
}

void string_dealloc(string *s) {
	buffer_dealloc(&s->b);
}

size_t string_get_length(string *s) {
	// -1 because the length of the buffer includes the terminating 0
	return s->b.length - 1;
}

char *string_get_cstr(string *s) {
	return s->b.data;
}

void string_clear(string *s) {
	buffer_set_length(&s->b, 1);
	s->b.data[0] = 0;
}

void string_append_str(string *s, string *other) {
	size_t s_len = string_get_length(s);
	size_t other_len = string_get_length(other);
	// resize to fit both
	buffer_set_capacity(&s->b, s_len + other_len + 1);
	// chop off the trailing 0 from this string
	buffer_set_length(&s->b, s_len);
	// add the other one, including that trailing 0
	buffer_append_bytes(&s->b, other->b.data, other_len + 1);
}

void string_set_str(string *s, string *other) {
	string_clear(s);
	string_append_str(s, other);
}

void string_append_cstr(string *s, char *other) {
	size_t s_len = string_get_length(s);
	size_t other_len = strlen(other);
	// resize to fit both
	buffer_set_capacity(&s->b, s_len + other_len + 1);
	// chop off the trailing 0 from this string
	buffer_set_length(&s->b, s_len);
	// add the other one, including that trailing 0
	buffer_append_bytes(&s->b, other, other_len + 1);
}

void string_set_cstr(string *s, char *other) {
	string_clear(s);
	string_append_cstr(s, other);
}

void string_append_cstr_len(string *s, char *other, size_t other_len) {
	size_t s_len = string_get_length(s);
	// resize to fit both
	buffer_set_capacity(&s->b, s_len + other_len + 1);
	// chop off the trailing 0 from this string
	buffer_set_length(&s->b, s_len);
	// add the other one
	buffer_append_bytes(&s->b, other, other_len);
	// add the trailing 0
	buffer_set_length(&s->b, s_len + other_len + 1);
	s->b.data[s_len + other_len] = 0;
}

void string_set_cstr_len(string *s, char *other, size_t other_len) {
	string_clear(s);
	string_append_cstr_len(s, other, other_len);
}

void string_append_cstrf(string *s, char *fmt, ...) {
	// check how much more space we need
	va_list args;
	va_start(args, fmt);
	size_t n = vsnprintf(NULL, 0, fmt, args);
	va_end(args);
	// resize to fit
	size_t current_length = string_get_length(s);
	buffer_ensure_capacity(&s->b, current_length + n + 1);
	// chop off the trailing 0 from this string
	buffer_set_length(&s->b, current_length);
	// write the string here
	va_start(args, fmt);
	vsnprintf(s->b.data + current_length, n + 1, fmt, args);
	va_end(args);
	buffer_set_length(&s->b, current_length + n + 1);
}

void string_set_cstrf(string *s, char *fmt, ...) {
	// check how much more space we need
	va_list args;
	va_start(args, fmt);
	size_t n = vsnprintf(NULL, 0, fmt, args);
	va_end(args);
	// resize to fit
	buffer_set_length(&s->b, n + 1);
	// write the string here
	va_start(args, fmt);
	vsnprintf(s->b.data, n + 1, fmt, args);
	va_end(args);
}

void string_append_substr(string *dst, string *src, size_t start, size_t end) {
	size_t src_len = string_get_length(src);
	if (start >= end || start >= src_len) {
		return;
	}
	if (end > src_len) {
		end = src_len;
	}
	size_t substr_len = end - start;
	size_t dst_len = string_get_length(dst);
	// resize to fit
	buffer_set_capacity(&dst->b, dst_len + substr_len + 1);
	// chop off the trailing 0 from this string
	buffer_set_length(&dst->b, dst_len);
	// write the substring here
	buffer_append_bytes(&dst->b, src->b.data + start, substr_len);
	// write the trailing 0
	buffer_set_length(&dst->b, dst_len + substr_len + 1);
	dst->b.data[dst_len + substr_len] = 0;
}

void string_set_substr(string *dst, string *src, size_t start, size_t end) {
	if (dst == src) {
		size_t len = string_get_length(dst);
		if (start >= end || start >= len) {
			return;
		}
		if (end > len) {
			end = len;
		}
		size_t substr_len = end - start;
		memcpy(dst->b.data, src->b.data + start, substr_len);
		dst->b.data[substr_len] = 0;
		buffer_set_length(&dst->b, substr_len + 1);
	} else {
		string_clear(dst);
		string_append_substr(dst, src, start, end);
	}
}

size_t string_index_of_str(string *s, string *find, size_t start) {
	size_t s_len = string_get_length(s);
	size_t find_len = string_get_length(find);
	if (find_len > s_len || find_len == 0) {
		return -1;
	}
	for (size_t i = start; i <= s_len - find_len; i++) {
		if (!memcmp(s->b.data + i, find->b.data, find_len)) {
			return i;
		}
	}
	return -1;
}

size_t string_index_of_cstr(string *s, char *find, size_t start) {
	size_t s_len = string_get_length(s);
	size_t find_len = strlen(find);
	if (find_len > s_len || find_len == 0) {
		return -1;
	}
	for (size_t i = start; i <= s_len - find_len; i++) {
		if (!memcmp(s->b.data + i, find, find_len)) {
			return i;
		}
	}
	return -1;
}

size_t string_index_of_char(string *s, char find, size_t start) {
	size_t s_len = string_get_length(s);
	if (s_len == 0) {
		return -1;
	}
	for (size_t i = start; i < s_len; i++) {
		if (s->b.data[i] == find) {
			return i;
		}
	}
	return -1;
}

size_t string_reverse_index_of_str(string *s, string *find, size_t start) {
	size_t s_len = string_get_length(s);
	size_t find_len = string_get_length(find);
	if (find_len > s_len || find_len == 0) {
		return -1;
	}
	size_t last_valid_index = s_len - find_len;
	if (start > last_valid_index) {
		start = last_valid_index;
	}
	for (size_t i = start; i != -1; i--) {
		if (!memcmp(s->b.data + i, find->b.data, find_len)) {
			return i;
		}
	}
	return -1;
}

size_t string_reverse_index_of_cstr(string *s, char *find, size_t start) {
	size_t s_len = string_get_length(s);
	size_t find_len = strlen(find);
	if (find_len > s_len || find_len == 0) {
		return -1;
	}
	size_t last_valid_index = s_len - find_len;
	if (start > last_valid_index) {
		start = last_valid_index;
	}
	for (size_t i = start; i != -1; i--) {
		if (!memcmp(s->b.data + i, find, find_len)) {
			return i;
		}
	}
	return -1;
}

size_t string_reverse_index_of_char(string *s, char find, size_t start) {
	size_t s_len = string_get_length(s);
	if (s_len == 0) {
		return -1;
	}
	size_t last_valid_index = s_len - 1;
	if (start > last_valid_index) {
		start = last_valid_index;
	}
	for (size_t i = start; i != -1; i--) {
		if (s->b.data[i] == find) {
			return i;
		}
	}
	return -1;
}

size_t string_index_of_any_str(string *s, string *find, size_t start) {
	return string_index_of_any_cstr(s, string_get_cstr(find), start);
}

size_t string_index_of_any_cstr(string *s, char *find, size_t start) {
	size_t s_len = string_get_length(s);
	if (s_len == 0) {
		return -1;
	}
	if (start >= s_len) {
		return -1;
	}
	size_t i = start;
	for (char *sc = s->b.data + start; *sc; sc++, i++) {
		for (char *fc = find; *fc; fc++) {
			if (*sc == *fc) {
				return i;
			}
		}
	}
	return -1;
}

size_t string_reverse_index_of_any_str(string *s, string *find, size_t start) {
	return string_reverse_index_of_any_cstr(s, string_get_cstr(find), start);
}

size_t string_reverse_index_of_any_cstr(string *s, char *find, size_t start) {
	size_t s_len = string_get_length(s);
	if (s_len == 0) {
		return -1;
	}
	if (start >= s_len) {
		start = s_len - 1;
	}
	size_t i = start;
	for (char *sc = s->b.data + start; sc != (char *)s->b.data - 1; sc--, i--) {
		for (char *fc = find; *fc; fc++) {
			if (*sc == *fc) {
				return i;
			}
		}
	}
	return -1;
}

size_t string_index_not_of_any_str(string *s, string *find, size_t start) {
	return string_index_not_of_any_cstr(s, string_get_cstr(find), start);
}

size_t string_index_not_of_any_cstr(string *s, char *find, size_t start) {
	size_t s_len = string_get_length(s);
	if (s_len == 0) {
		return -1;
	}
	if (start >= s_len) {
		return -1;
	}
	size_t i = start;
	for (char *sc = s->b.data + start; *sc; sc++, i++) {
		int found = 0;
		for (char *fc = find; *fc; fc++) {
			if (*sc == *fc) {
				found = 1;
				break;
			}
		}
		if (!found) {
			return i;
		}
	}
	return -1;
}

size_t string_reverse_index_not_of_any_str(string *s, string *find, size_t start) {
	return string_reverse_index_not_of_any_cstr(s, string_get_cstr(find), start);
}

size_t string_reverse_index_not_of_any_cstr(string *s, char *find, size_t start) {
	size_t s_len = string_get_length(s);
	if (s_len == 0) {
		return -1;
	}
	if (start >= s_len) {
		start = s_len - 1;
	}
	size_t i = start;
	for (char *sc = s->b.data + start; sc != (char *)s->b.data - 1; sc--, i--) {
		int found = 0;
		for (char *fc = find; *fc; fc++) {
			if (*sc == *fc) {
				found = 1;
				break;
			}
		}
		if (!found) {
			return i;
		}
	}
	return -1;
}

size_t string_split(string *s, size_t start, char *delim, size_t *results, size_t results_capacity, size_t max_results) {
	if (max_results == 0) {
		max_results = -1;
	}
	size_t len = string_get_length(s);
	size_t remaining_len = len - start;
	size_t delim_len = strlen(delim);
	if (delim_len > remaining_len) {
		if (results_capacity >= 1) {
			results[0] = start;
			results[1] = len;
		}
		return 1;
	}
	size_t result_count = 0;
	for (size_t i = start; i < len - delim_len; i++) {
		if (!memcmp(s->b.data + i, delim, delim_len)) {
			if (result_count < results_capacity) {
				results[result_count * 2 + 0] = start;
				results[result_count * 2 + 1] = i;
			}
			start = i + delim_len;
			i += delim_len - 1;
			result_count++;
			if (result_count + 1 >= max_results) {
				break;
			}
		}
	}
	if (start < len) {
		if (result_count < results_capacity && result_count < max_results) {
			results[result_count * 2 + 0] = start;
			results[result_count * 2 + 1] = len;
		}
		result_count++;
	}
	return result_count;
}

int string_compare_str(string *a, string *b, string_compare_mode mode) {
	return string_compare_cstr_len(a, b->b.data, string_get_length(b), mode);
}

int string_compare_cstr(string *a, char *b, string_compare_mode mode) {
	return string_compare_cstr_len(a, b, strlen(b), mode);
}

int string_compare_cstr_len(string *a, char *b, size_t b_len, string_compare_mode mode) {
	size_t a_len = string_get_length(a);
	size_t min_len = a_len < b_len ? a_len : b_len;
	for (size_t i = 0; i < min_len; i++) {
		char ac = a->b.data[i];
		char bc = b[i];
		if (mode == STRING_COMPARE_CASE_INSENSITIVE) {
			ac = tolower(ac);
			bc = tolower(bc);
		}
		char diff = ac - bc;
		if (diff) {
			return diff;
		}
	}
	size_t diff = a_len - b_len;
	return diff;
}