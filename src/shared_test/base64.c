#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../shared/base64.h"

struct test_data {
	char *input;
	char *expected_dst;
	size_t expected_dst_len;
	// how much to allocate for the dst array
	size_t dst_len;
	int expected_output;
};

static struct test_data ENCODE_DATA[] = {
	{"", "", 0, 100, 0},
	{"", "", 0, 0, 1},
	{"\x01", "AQ==", 5, 100, 0},
	{"\x02", "Ag==", 5, 100, 0},
	{"\xff", "/w==", 5, 100, 0},
	{"\x01\x02", "AQI=", 5, 100, 0},
	{"\xff\xfe", "//4=", 5, 100, 0},
	{"\x01\x02\x03", "AQID", 5, 100, 0},
	{"\xff\xfe\xfd", "//79", 5, 100, 0},
	{"\x01\x02\x03\x04", "AQIDBA==", 9, 100, 0},
	{"\xff\xfe\xfd\xfc", "//79/A==", 9, 100, 0},
	{"Hello, World!", "SGVsbG8sIFdvcmxkIQ==", 21, 100, 0},
	{"Hello, World!", "", 0, 5, 1},
	{"Hello, World!", "", 0, 0, 1},
};

static struct test_data DECODE_DATA[] = {
	{"", "", 0, 100, 0},
	{"", "", 0, 0, 0},
	{"AQ==", "\x01", 1, 100, 0},
	{"AQ=", "", 0, 100, 1},
	{"AQ", "", 0, 100, 1},
	{"Ag==", "\x02", 1, 100, 0},
	{"/w==", "\xff", 1, 100, 0},
	{"AQI=", "\x01\x02", 2, 100, 0},
	{"//4=", "\xff\xfe", 2, 100, 0},
	{"AQID", "\x01\x02\x03", 3, 100, 0},
	{"//79", "\xff\xfe\xfd", 3, 100, 0},
	{"AQIDBA==", "\x01\x02\x03\x04", 4, 100, 0},
	{"//79/A==", "\xff\xfe\xfd\xfc", 4, 100, 0},
	{"SGVsbG8sIFdvcmxkIQ==", "Hello, World!", 13, 100, 0},
	{"SGVsbG8sIFdvcmxkIQ==", "", 0, 5, 1},
	{"SGVsbG8sIFdvcmxkIQ==", "", 0, 0, 1},
	{"SGVsbG8sIFdvcmxkIQ=", "", 0, 100, 1},
	{"SGVsbG8sIFdvcmxkIQ", "", 0, 100, 1},
	{"SGVsbG8sIFdvcmxkIQ=a", "", 0, 100, 1},
	{"SGVsbG8=IFdvcmxkIQa=", "", 0, 100, 1},
	{"_", "", 0, 100, 1},
};

void print_hex(uint8_t *data, size_t len) {
	for (size_t i = 0; i < len; i++) {
		printf("%02x", data[i]);
	}
}

int main(int argc, char **argv) {
	printf("encode\n");
	for (int i = 0; i < sizeof(ENCODE_DATA) / sizeof(ENCODE_DATA[0]); i++) {
		printf("test case %i\n", i);
		char *dst = calloc(1, ENCODE_DATA[i].dst_len);
		int result = base64_encode(dst, ENCODE_DATA[i].dst_len, ENCODE_DATA[i].input, strlen(ENCODE_DATA[i].input));
		printf("result = %i, expected = %i\n", result, ENCODE_DATA[i].expected_output);
		printf("dst = %s, expected = %s\n", dst, ENCODE_DATA[i].expected_dst);
		fflush(stdout);
		assert(result == ENCODE_DATA[i].expected_output);
		assert(memcmp(dst, ENCODE_DATA[i].expected_dst, ENCODE_DATA[i].expected_dst_len) == 0);
		free(dst);
	}
	printf("\n");

	printf("decode\n");
	for (int i = 0; i < sizeof(DECODE_DATA) / sizeof(DECODE_DATA[0]); i++) {
		printf("test case %i\n", i);
		char *dst = calloc(1, DECODE_DATA[i].dst_len);
		int result = base64_decode(dst, DECODE_DATA[i].dst_len, DECODE_DATA[i].input);
		printf("result = %i, expected = %i\n", result, DECODE_DATA[i].expected_output);
		printf("dst = ");
		print_hex(dst, DECODE_DATA[i].expected_dst_len);
		printf(", expected = ");
		print_hex(DECODE_DATA[i].expected_dst, DECODE_DATA[i].expected_dst_len);
		printf("\n");
		fflush(stdout);
		assert(result == DECODE_DATA[i].expected_output);
		assert(memcmp(dst, DECODE_DATA[i].expected_dst, DECODE_DATA[i].expected_dst_len) == 0);
		free(dst);
	}
	printf("\n");

	return 0;
}