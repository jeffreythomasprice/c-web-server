/*
references:
https://en.wikipedia.org/wiki/Base64
https://datatracker.ietf.org/doc/html/rfc4648#section-4
*/

#ifndef base64_h
#define base64_h

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdlib.h>

/*
base64 encodes src (a byte array of length src_len) and writes the output to dst (and buffer of length dst_len).

dst_len should include the 0-terminator, so a maximum of dst_len - 1 characters will be written.

Returns 0 on success, and non-0 on failure.
*/
int base64_encode(char *dst, size_t dst_len, uint8_t *src, size_t src_len);

/*
base64 decodes a 0-terminated ascii string src into the dst buffer. The dst buffer is dst_len bytes long.

Returns 0 on success, and non-0 on failure.

Fails if dst_len is not big enough to store the full output.

Fails if the src is not a multiple of three bytes, or contains any characters outside the allowed character set. Fails if the padding
character apperas anywhere by the final two characters.
*/
int base64_decode(uint8_t *dst, size_t dst_len, char *src);

#ifdef __cplusplus
}
#endif

#endif