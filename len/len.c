/* len.c: Created by Ekko on 2025/11/18 */
#include <stdio.h>
#include <argparse.h>

static struct option options[] = {
        { 'h', "help", no_argument, opt_single, "帮助" },
        { 'v', "version", no_argument, opt_single, "版本号" },
        { 'b', "bytemode", no_argument, opt_single, "按字节计算" },
        { '?', "encode", required_argument, opt_single, "字符串编码" },
        { '?', "decode", required_argument, opt_single, "字符串解码" },
        { 0 },
};

int main(int argc, char **argv)
{
        argparse_t *parser;

        parser = argparse_parse(options, argc, argv);

        return 0;
}
