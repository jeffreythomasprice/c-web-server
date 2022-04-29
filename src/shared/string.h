#ifndef string_h
#define string_h

#include "buffer.h"

typedef struct {
	buffer b;
} string;

void string_init(string *s);
void string_init_cstr(string *s, char *c);
void string_init_cstr_len(string *s, char *c, size_t len);
void string_dealloc(string *s);

size_t string_get_length(string *s);
char *string_get_cstr(string *s);

void string_appends(string *s, string *other);
void string_appendf(string *s, char *fmt, ...);

#endif