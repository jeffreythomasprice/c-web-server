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
	int has_scheme;
	string scheme;
	int has_authority;
	string authority;
	int has_userinfo;
	string userinfo;
	int has_host;
	string host;
	int has_port;
	string port_str;
	int port;
	int has_path;
	string path;
	int has_query;
	string query;
	int has_fragment;
	string fragment;
} uri;

void uri_decode_str(string *dst, string *src);
void uri_decode_cstr(string *dst, char *src);
void uri_decode_cstr_len(string *dst, char *src, size_t src_len);

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

/**
 * @returns the scheme, including the ":" or "://", if present, or NULL if omitted
 */
string *uri_get_scheme(uri *u);
/**
 * @returns the authority (the combination of user info, host, and port), or NULL if omitted
 */
string *uri_get_authority(uri *u);
/**
 * @returns the user info component of the authority, excluding the "@", or NULL if omitted
 */
string *uri_get_userinfo(uri *u);
/**
 * @returns the host component of the authority, excluding any "@" or ":", or NULL if omitted
 */
string *uri_get_host(uri *u);
/**
 * @returns the port component of the authority, excluding any ":", or NULL if omitted
 */
string *uri_get_port_str(uri *u);
/**
 * @returns the port component of the authority, parsed as an integer, or 0 if omitted
 */
int uri_get_port(uri *u);
/**
 * @returns the path component, including any leading "/", or NULL if omitted
 */
string *uri_get_path(uri *u);
/**
 * @returns the query component, excluding any leading "?", or NULL if omitted
 */
string *uri_get_query(uri *u);
/**
 * @returns the fragment component, excluding any leading "#", or NULL if omitted
 */
string *uri_get_fragment(uri *u);

// TODO JEFF setters for building a uri from parts
// TODO JEFF encode back to string

#ifdef __cplusplus
}
#endif

#endif