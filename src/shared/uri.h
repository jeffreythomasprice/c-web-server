/*
Reference:
https://datatracker.ietf.org/doc/html/rfc3986
*/

#ifndef uri_h
#define uri_h

#include "string.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	string scheme;
	string authority;
	string path;
	string query;
	string fragment;
} uri;

void uri_init(uri *u);
void uri_dealloc(uri *u);

int uri_parse_str(uri *u, string *input, size_t index);
int uri_parse_cstr(uri *u, char *input);
int uri_parse_cstr_len(uri *u, char *input, size_t input_length);

#ifdef __cplusplus
}
#endif

#endif