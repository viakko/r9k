/* argparse.h: Created by Ekko on 2025/11/19 */
#ifndef ARGPARSE_H_
#define ARGPARSE_H_

#include <stdlib.h>

#define no_argument       (0)
#define required_argument (1)

#define opt_single        (0)
#define opt_multi         (1)

struct option
{
        const char short_name;
        const char *long_name;
        unsigned int has_arg;
        unsigned int multi;
        const char *description;
};

typedef struct argparse argparse_t;

argparse_t *argparse_parse(const struct option *opts, int argc, char **argv);
void argparse_free(argparse_t *ap);
int argparse_has(argparse_t *ap, const char *name);
const char *argparse_val(argparse_t *ap, const char *name);
const char **argparse_multi_val(argparse_t *ap, const char *name, size_t *nval);
const char *argparse_arg(argparse_t *ap);
const char *argparse_error(void);

#endif /* ARGPARSE_H_ */
