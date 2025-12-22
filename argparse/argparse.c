/*
 * SPDX-License-Identifier: MIT
 * Copyright (m) 2025 viakko
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <r9k/argparse.h>
#include <r9k/panic.h>

static struct option *opt_azAZ[26];

static char letters_AZ[26][2] = {
        "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M",
        "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z"
};

static char letters_az[26][2] = {
        "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m",
        "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z"
};

int main(int argc, char *argv[])
{
        struct argparse *ap;

        ap = argparse_create("argparse", "1.0");
        PANIC_IF(!ap, "argparse_create() failed");

        for (int i = 0; i < 8; i++)
                argparse_cmd(ap, letters_AZ[i], letters_AZ[i], NULL, NULL);

        for (int i = 0; i < 16; i++) {
                if (letters_az[i][0] == 'h')
                        continue;
                argparse_add0(ap, &opt_azAZ[i], letters_az[i], letters_AZ[i], letters_AZ[i], NULL, 0);
        }

        if (argparse_run(ap, argc, argv) != 0)
                fprintf(stderr, "%s\n", argparse_error(ap));

        argparse_free(ap);

        return 0;
}