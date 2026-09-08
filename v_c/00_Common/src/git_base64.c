#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "rng.h"
#include "git_base64.h"

void generate_key(char *key) 
{
	int i;
    uint32_t single_random_number[1];
    
    getRandom(1, single_random_number);

	for (i = 0; i < KEY_LENGTH; ++i) {
		key[i] = single_random_number[0] % 256;
	}
}

void generate_mask(char *key) 
{
	int i;
    uint32_t single_random_number[1];
    
    getRandom(1, single_random_number);
    
	for (i = 0; i < KEY_LENGTH; ++i) {
		key[i] = single_random_number[0] % 255 + 1;
	}
}

int base64_decode(char *text, unsigned char *dst, int numBytes )
{
	const char* cp;
	int space_idx = 0, phase;
	int d, prev_d = 0;
	unsigned char c;
	space_idx = 0;
	phase = 0;
	for ( cp = text; *cp != '\0'; ++cp ) 
	{
		d = DecodeMimeBase64[(int) *cp];
		if ( d != -1 ) 
		{
			switch ( phase ) 
			{
				case 0:
					++phase;
					break;
				case 1:
					c = ( ( prev_d << 2 ) | ( ( d & 0x30 ) >> 4 ) );
					if ( space_idx < numBytes )
						dst[space_idx++] = c;
					++phase;
					break;
				case 2:
					c = ( ( ( prev_d & 0xf ) << 4 ) | ( ( d & 0x3c ) >> 2 ) );
					if ( space_idx < numBytes )
						dst[space_idx++] = c;
					++phase;
					break;
				case 3:
					c = ( ( ( prev_d & 0x03 ) << 6 ) | d );
					if ( space_idx < numBytes )
						dst[space_idx++] = c;
					phase = 0;
					break;
			}
			prev_d = d;
		}
	}
	return space_idx;
}

int base64_encode(char *text, int numBytes, char **encodedText)
{
	unsigned char input[3] 	= {0,0,0};
	unsigned char output[4] = {0,0,0,0};
	int size = 4 * ((numBytes + 2) / 3);			// Calculate size to be allocated
	*encodedText = (char *)malloc(size + 1);		// Allocate memory for encoded text
	if (!*encodedText) return 0; 					// Return 0 if memory allocation fails
	int i, j = 0;
    
	for (i = 0; i < numBytes; i += 3) {
		// Fill the input buffer with up to three bytes
		input[0] = text[i];
		input[1] = (i + 1 < numBytes) ? text[i + 1] : 0;
		input[2] = (i + 2 < numBytes) ? text[i + 2] : 0;

		// Convert to output
		output[0] = (input[0] >> 2);
		output[1] = ((input[0] & 0x03) << 4) | (input[1] >> 4);
		output[2] = ((input[1] & 0x0F) << 2) | (input[2] >> 6);
		output[3] = input[2] & 0x3F;

		// Map to Base64 characters
		(*encodedText)[j++] = MimeBase64[output[0]];
		(*encodedText)[j++] = MimeBase64[output[1]];
		(*encodedText)[j++] = (i + 1 < numBytes) ? MimeBase64[output[2]] : '=';
		(*encodedText)[j++] = (i + 2 < numBytes) ? MimeBase64[output[3]] : '=';
	}

	(*encodedText)[j] = '\0';					// Null-terminate the string
	return size;							// Return the size of the encoded data
}
