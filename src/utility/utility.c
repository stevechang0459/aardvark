#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "utility.h"

#ifdef WIN32
void *align_alloc(size_t size, u32 align)
{
	size_t addr;
	void *ptr, *aligned_ptr;

	ptr = malloc(size + align + sizeof(size_t));
	if (ptr != NULL) {
		addr        = (size_t)ptr + align + sizeof(size_t);
		aligned_ptr = (void *)(addr - (addr % align));
		*((size_t *)aligned_ptr - 1) = (size_t)ptr;
		// printf("[%s](%x,%x)\n", __func__, ptr, aligned_ptr);
	} else {
		return NULL;
	}

	return aligned_ptr;
}

void aligned_free(void *aligned_ptr)
{
	// printf("[%s](%x,%x)\n", __func__, *((size_t *)aligned_ptr - 1), aligned_ptr);
	free((void *)(*((size_t *)aligned_ptr - 1)));
}
#endif

/*
 *  strlen - Find the length of a string
 *  @s: The string to be sized
 */
size_t strlen(const char *s)
{
	const char *sc;
	for (sc = s; *sc != '\0'; ++sc)
		/* Do nothing*/;

	return sc - s;
}

/**
 * @brief This function print_buf is designed to print the contents of a buffer
 * in a formatted hexadecimal and ASCII representation. The output includes the
 * buffer address, hexadecimal values, and corresponding ASCII characters, with
 * unprintable characters shown as dots (.). The function ensures the output
 * format is consistent, even when the buffer length is not a multiple of 16
 * bytes.
 *
 * @param buf Pointer to the buffer to be printed.
 * @param len Length of the buffer in bytes.
 * @param title Optional title to print before the buffer contents. This can be
 *              a formatted string followed by variadic arguments.
 */
void print_buf(const void *buf, size_t len, const char *title, ...)
{
	u32 i, j;
	int new_line = 1;

	// If a title is provided, use variadic arguments to print the title.
	if (title && *title) {
		va_list argp;
		va_start(argp, title);
		vfprintf(stderr, title, argp);
		va_end(argp);
		fputc('\n', stderr);
	}

	// If the buffer length is zero, print a message and return.
	if (len == 0) {
		printf("len is zero.\n\n");
		return;
	}

	// Traverse each byte of the buffer.
	for (i = 0; i < len; i++) {
		// Print new line and address every 16 bytes.
		if (new_line) {
			printf("0x%p: ", (u8 *)buf + i);
			new_line = 0;
		}

		// Print a space every 8 bytes for better readability.
		if (i % 8 == 0 && i % 16 != 0) {
			printf(" ");
		}

		// Print the current byte in hexadecimal format.
		printf("%02X ", ((u8 *)buf)[i]);

		// Print ASCII characters at the end of every 16 bytes.
		if ((i + 1) % 16 == 0) {
			printf("   ");
			for (j = i - 15; j <= i; j++) {
				if (((u8 *)buf)[j] < 0x20 || ((u8 *)buf)[j] > 0x7E) {
					printf(".");
				} else {
					printf("%c", ((char *)buf)[j]);
				}
			}
			printf("\n");
			new_line = 1;
		}
	}

	// Handle the last line if it's not complete.
	if (!new_line) {
		// Pad the last line with spaces if it's not 16 bytes.
		for (j = i; j % 16 != 0; j++) {
			if (j % 8 == 0) {
				printf(" ");
			}
			printf("   ");
		}

		// Print ASCII characters for the last line.
		printf("   ");
		for (j = i - (i % 16); j < i; j++) {
			if (((u8 *)buf)[j] < 0x20 || ((u8 *)buf)[j] > 0x7E) {
				printf(".");
			} else {
				printf("%c", ((char *)buf)[j]);
			}
		}

		/**
		 * If the current byte count is not a multiple of 16, pad the ASCII
		 * characters
		 */
		if (i % 16) {
			/**
			 * From the current position to the next 16-byte boundary, pad the
			 * characters.
			 */
			for (j = i; j < i + 16 - (i % 16); j++) {
				// Use '.' to pad the characters to align the ASCII characters.
				printf(".");
			}
		}
		printf("\n");
	}

	printf("\n");
}

void reverse(void *in, u32 len)
{
	char *c1 = in;
	char *c2 = in + len - 1;

	while (1) {
		*c1 = *c1 ^ *c2;        // x = a ^ b
		*c2 = *c1 ^ *c2;        // b = (a = x ^ b)
		*c1 = *c1 ^ *c2;        // a = (b = x ^ a)
		c1++;
		c2--;
		if (c1 >= c2) {
			break;
		}
	}
}
