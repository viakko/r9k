/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2025
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <r9k/argparse.h>
#include <r9k/panic.h>

static const char b64_map[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static signed char b64_rev[256];

static void b64_init(void)
{
	memset(b64_rev, -1, sizeof(b64_rev));

	for (int i = 0; i < 26; i++) {
		b64_rev['A' + i] = i;
		b64_rev['a' + i] = i + 26;
	}

	for (int i = 0; i < 10; i++)
		b64_rev['0' + i] = i + 52;

	b64_rev['+'] = 62;
	b64_rev['/'] = 63;
}

char *base64_encode(const unsigned char *data, size_t len)
{
	size_t out_len = (len + 2) / 3 * 4;
	char *out = malloc(out_len + 1);
	size_t i, j;
	unsigned int v;

	if (!out)
		return NULL;

	for (i = 0, j = 0; i < len; i += 3) {
		v = data[i] << 16;
		if (i + 1 < len)
			v |= data[i + 1] << 8;
		if (i + 2 < len)
			v |= data[i + 2];

		out[j++] = b64_map[(v >> 18) & 0x3f];
		out[j++] = b64_map[(v >> 12) & 0x3f];
		out[j++] = (i + 1 < len) ? b64_map[(v >> 6) & 0x3f] : '=';
		out[j++] = (i + 2 < len) ? b64_map[v & 0x3f] : '=';
	}

	out[j] = '\0';
	return out;
}

unsigned char *base64_decode(const char *b64, size_t *out_len)
{
	size_t len = strlen(b64);
	size_t alloc = len / 4 * 3;
	unsigned char *out;
	size_t i, j;
	unsigned int v;
	int pad;

	if (len % 4)
		PANIC("error: invalid base64 length\n");

	if (len >= 1 && b64[len - 1] == '=') alloc--;
	if (len >= 2 && b64[len - 2] == '=') alloc--;

	out = malloc(alloc);
	if (!out)
		return NULL;

	for (i = 0, j = 0; i < len; i += 4) {
		pad = 0;

		if (b64_rev[(unsigned char)b64[i]] < 0 ||
		    b64_rev[(unsigned char)b64[i + 1]] < 0)
			PANIC("invalid base64");

		v  = b64_rev[(unsigned char)b64[i]] << 18;
		v |= b64_rev[(unsigned char)b64[i + 1]] << 12;

		if (b64[i + 2] == '=') {
			pad++;
		} else {
			if (b64_rev[(unsigned char)b64[i + 2]] < 0)
				PANIC("error: invalid base64");
			v |= b64_rev[(unsigned char)b64[i + 2]] << 6;
		}

		if (b64[i + 3] == '=') {
			pad++;
		} else {
			if (b64_rev[(unsigned char)b64[i + 3]] < 0)
				PANIC("error: invalid base64");
			v |= b64_rev[(unsigned char)b64[i + 3]];
		}

		out[j++] = (v >> 16) & 0xff;
		if (pad < 2)
			out[j++] = (v >> 8) & 0xff;
		if (pad < 1)
			out[j++] = v & 0xff;
	}

	if (out_len)
		*out_len = j;

	return out;
}

static int encode(struct argparse *ap, struct option *e)
{
        __attr_ignore(e);

        if (argparse_count(ap) <= 0)
                PANIC("error: invalid arguments");

        const char *plain = argparse_val(ap, 0);
        char *cipher = base64_encode((unsigned char *) plain, strlen(plain));

	if (argparse_has(ap, "u")) {
		size_t n = strlen(cipher);
		for (size_t i = 0; i < n; i++) {
			if (cipher[i] == '+')
				cipher[i] = '-';
			else if (cipher[i] == '/')
				cipher[i] = '_';
		}
	}

        printf("%s\n", cipher);
        free(cipher);

        return 0;
}

static int decode(struct argparse *ap, struct option *e)
{
	__attr_ignore(e);

	if (argparse_count(ap) <= 0)
		PANIC("error: invalid arguments");

	const char *src = argparse_val(ap, 0);
	size_t n = strlen(src);
	char *cipher = malloc(n + 4);

	memcpy(cipher, src, n);
	cipher[n] = '\0';

	if (argparse_has(ap, "u")) {
		for (size_t i = 0; i < n; i++) {
			if (cipher[i] == '-') cipher[i] = '+';
			else if (cipher[i] == '_') cipher[i] = '/';
		}

		size_t pad = n & 3;
		if (pad == 1)
			PANIC("error: invalid base64");

		if (pad == 2) {
			cipher[n++] = '=';
			cipher[n++] = '=';
		} else if (pad == 3) {
			cipher[n++] = '=';
		}
		cipher[n] = '\0';
	}

	size_t out_len;
	unsigned char *plain = base64_decode(cipher, &out_len);
	fwrite(plain, 1, out_len, stdout);
	putchar('\n');

	free(cipher);
	free(plain);

	return 0;
}

int main(int argc, char* argv[])
{
	b64_init();

        struct argparse *ap;
        struct option *e, *d, *u;

        ap = argparse_create("b64", "1.0");
        PANIC_IF(!ap, "error: argparse initialize failed");

        argparse_add0(ap, &e, "e", NULL, "encode", encode, 0);
        argparse_add0(ap, &d, "d", NULL, "decode", decode, 0);
        argparse_add0(ap, &u, "u", NULL, "url safe", NULL, 0);

        argparse_mutual_exclude(ap, &e, &d);

        if (argparse_run(ap, argc, argv) != 0)
                PANIC("%s\n", argparse_error(ap));

        argparse_destroy(ap);

        return 0;
}