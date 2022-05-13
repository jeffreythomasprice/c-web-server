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
	string userinfo;
	string host;
	string port_str;
	int port;
	string path;
	string query;
	string fragment;
} uri;

void uri_init(uri *u);
void uri_dealloc(uri *u);

/**
 * @returns 0 on successful parse, non-0 if there were no valid URI components in the input, or if there were extra characters in the input
 */
int uri_parse_str(uri *u, string *input, size_t index);
/**
 * @returns 0 on successful parse, non-0 if there were no valid URI components in the input, or if there were extra characters in the input
 */
int uri_parse_cstr(uri *u, char *input);
/**
 * @returns 0 on successful parse, non-0 if there were no valid URI components in the input, or if there were extra characters in the input
 */
int uri_parse_cstr_len(uri *u, char *input, size_t input_length);

#ifdef __cplusplus
}
#endif

#endif