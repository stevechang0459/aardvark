#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "utility.h"

void *aligned_alloc(size_t size, u32 align)
{
	size_t addr;
	void *ptr, *aligned_ptr;

	ptr = malloc(size + align + sizeof(size_t));
	if (ptr != NULL) {
		addr        = (size_t)ptr + align + sizeof(size_t);
		aligned_ptr = (void *)(addr - (addr % align));
		*((size_t *)aligned_ptr - 1) = (size_t)ptr;
		// printf("[%s](%x,%x)\n", __FUNCTION__, ptr, aligned_ptr);
	} else {
		return NULL;
	}

	return aligned_ptr;
}

void aligned_free(void *aligned_ptr)
{
	// printf("[%s](%x,%x)\n", __FUNCTION__, *((size_t *)aligned_ptr - 1), aligned_ptr);
	free((void *)(*((size_t *)aligned_ptr - 1)));
}

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

void print_buf(const void *buf, size_t len, const char *title, ...)
{
	u32 i, addr = (u32)buf;
	u32 j, p;

	if (title) {
		va_list argp;
		va_start(argp, title);
		vfprintf(stderr, title, argp);
		va_end(argp);
		fputc('\n', stderr);
	}

	if (len == 0) {
		printf("[%s][%d] SIZE is zero.\n\n", __FUNCTION__, __LINE__);
		return;
	}

	printf("0x%08X: ", addr);

	for (i = 0; i < len; i++) {
		if (i) {
			if (i % 16 == 0) {
				printf("   ");
				for (j = i - 16; j < i; j++) {
					if (((u8 *)buf)[j] < 0x20 || ((u8 *)buf)[j] > 0x7E) {
						printf(".");
					} else {
						printf("%c", ((u8 *)buf)[j]);
					}
				}
				printf("\n0x%08X: ", addr + i);
				p = 1;
			} else if (i % 8 == 0) {
				printf(" ");
			}
		}

		printf("%02X ", ((u8 *)buf)[i]);
		p = 0;
	}

	if (!p) {
		if (i % 16) {
			for (j = i; j < i + 16 - (i & 0xF); j++) {
				if (j % 8 == 0) {
					printf(" ");
				}

				printf("   ");
			}
		}

		printf("   ");

		for (j = i - ((i % 16) ? i % 16 : 16); j < i; j++) {
			if (((u8 *)buf)[j] < 0x20 || ((u8 *)buf)[j] > 0x7E) {
				printf(".");
			} else {
				printf("%c", ((u8 *)buf)[j]);
			}
		}

		for (j = i; j < i + 16 - (i & 0xF); j++) {
			printf(".");
		}
	}

	printf("\n\n");
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
