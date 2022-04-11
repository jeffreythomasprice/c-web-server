#include "base64.h"

#define BASE64_PADDING '='
#define BASE64_ILLEGAL_VALUE 0xff

static char BASE64_VALUE_TO_CHAR[] = {
	// 0 through 25
	'A',
	'B',
	'C',
	'D',
	'E',
	'F',
	'G',
	'H',
	'I',
	'J',
	'K',
	'L',
	'M',
	'N',
	'O',
	'P',
	'Q',
	'R',
	'S',
	'T',
	'U',
	'V',
	'W',
	'X',
	'Y',
	'Z',
	// 26 through 51
	'a',
	'b',
	'c',
	'd',
	'e',
	'f',
	'g',
	'h',
	'i',
	'j',
	'k',
	'l',
	'm',
	'n',
	'o',
	'p',
	'q',
	'r',
	's',
	't',
	'u',
	'v',
	'w',
	'x',
	'y',
	'z',
	// 52 through 61
	'0',
	'1',
	'2',
	'3',
	'4',
	'5',
	'6',
	'7',
	'8',
	'9',
	// 62
	'+',
	// 63
	'/',
};

static uint8_t BASE64_CHAR_TO_VALUE[] = {
	// 0
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	// 8
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	// 16
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	// 24
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	// 32
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	// 40
	0xff,
	0xff,
	0xff,
	// 43 = '+'
	62,
	0xff,
	0xff,
	0xff,
	// 47 = '/'
	63,
	// 48 = '0'
	52,
	53,
	54,
	55,
	56,
	57,
	58,
	59,
	// 56
	60,
	// 57 = '9'
	61,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	// 64
	0xff,
	// 65 = 'A'
	0,
	1,
	2,
	3,
	4,
	5,
	6,
	// 72
	7,
	8,
	9,
	10,
	11,
	12,
	13,
	14,
	// 80
	15,
	16,
	17,
	18,
	19,
	20,
	21,
	22,
	// 88
	23,
	24,
	// 90 = 'Z'
	25,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	// 96
	0xff,
	// 97 = 'a'
	26,
	27,
	28,
	29,
	30,
	31,
	32,
	// 104
	33,
	34,
	35,
	36,
	37,
	38,
	39,
	40,
	// 112
	41,
	42,
	43,
	44,
	45,
	46,
	47,
	48,
	// 120
	49,
	50,
	// 122 = 'z'
	51,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	// 128
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	// 136
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	// 144
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	// 152
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	// 160
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	// 168
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	// 176
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	// 184
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	// 192
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	// 200
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	// 208
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	// 216
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	// 224
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	// 232
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	// 240
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	// 248
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
};

int base64_encode(char *dst, size_t dst_len, uint8_t *src, size_t src_len) {
	// fail if we didn't provide enough space
	size_t expected_dst_len = (src_len + 2) / 3 * 4;
	if (expected_dst_len + 1 > dst_len) {
		return 1;
	}
	for (size_t i = 0; i < src_len; i += 3, src += 3, dst += 4) {
		uint8_t src0 = src[0];
		uint8_t src1;
		int padding2;
		if (i + 1 < src_len) {
			src1 = src[1];
			padding2 = 0;
		} else {
			src1 = 0;
			padding2 = 1;
		}
		uint8_t src2;
		int padding3;
		if (i + 2 < src_len) {
			src2 = src[2];
			padding3 = 0;
		} else {
			src2 = 0;
			padding3 = 1;
		}
		dst[0] = BASE64_VALUE_TO_CHAR[(src0 & 0b11111100) >> 2];
		dst[1] = BASE64_VALUE_TO_CHAR[((src0 & 0b00000011) << 4) | ((src1 & 0b11110000) >> 4)];
		if (padding2) {
			dst[2] = BASE64_PADDING;
		} else {
			dst[2] = BASE64_VALUE_TO_CHAR[((src1 & 0b00001111) << 2) | ((src2 & 0b11000000) >> 6)];
		}
		if (padding3) {
			dst[3] = BASE64_PADDING;
		} else {
			dst[3] = BASE64_VALUE_TO_CHAR[src2 & 0b00111111];
		}
	}
	dst[0] = 0;
	return 0;
}

int base64_decode(uint8_t *dst, size_t dst_len, char *src) {
	size_t src_len = strlen(src);
	// wrong number of characters, we have a partial block
	if (src_len % 4 != 0) {
		return 1;
	}
	// fail if we didn't provide enough space
	size_t expected_dst_len = (src_len + 3) / 4 * 3;
	if (expected_dst_len > dst_len) {
		return 1;
	}
	for (size_t i = 0; i < src_len; i += 4, src += 4, dst += 3) {
		// translate into the 6-bit values
		uint8_t value0 = BASE64_CHAR_TO_VALUE[src[0]];
		uint8_t value1 = BASE64_CHAR_TO_VALUE[src[1]];
		uint8_t value2 = BASE64_CHAR_TO_VALUE[src[2]];
		uint8_t value3 = BASE64_CHAR_TO_VALUE[src[3]];
		// prove these are all valid
		if (value0 == BASE64_ILLEGAL_VALUE || value1 == BASE64_ILLEGAL_VALUE) {
			return 1;
		}
		if (i + 4 < src_len) {
			// not the last block, fail if any padding is present
			if (value2 == BASE64_ILLEGAL_VALUE || value3 == BASE64_ILLEGAL_VALUE) {
				return 1;
			}
		} else {
			// last block, only fail if the padding is out of order
			if (src[2] == BASE64_PADDING) {
				// we had padding on the 3rd one, so the 4th must also be padding
				if (src[3] != BASE64_PADDING) {
					return 1;
				}
				// last 2 bytes are definitely padding
				value2 = 0;
				value3 = 0;
			} else {
				// we didn't have padding on the 3rd, so the 3rd must be a legal character
				if (BASE64_CHAR_TO_VALUE[src[2]] == BASE64_ILLEGAL_VALUE) {
					return 1;
				}
				if (src[3] == BASE64_PADDING) {
					// last byte is padding
					value3 = 0;
				} else {
					// last byte must be a legal character
					if (BASE64_CHAR_TO_VALUE[src[3]] == BASE64_ILLEGAL_VALUE) {
						return 1;
					}
				}
			}
		}
		// write the output
		dst[0] = ((value0 & 0b111111) << 2) | ((value1 & 0b110000) >> 4);
		dst[1] = ((value1 & 0b001111) << 4) | ((value2 & 0b111100) >> 2);
		dst[2] = ((value2 & 0b000011) << 6) | (value3 & 0b111111);
	}
	return 0;
}