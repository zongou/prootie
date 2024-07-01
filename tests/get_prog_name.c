#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>

int main(int argc, char *argv[]) {
    // 使用argv[0]获取程序名称，包含路径
    char *program = argv[0];

    // 使用basename函数去掉路径部分，获取程序基名
    char *base_name = basename(program);

    printf("Program basename: %s\n", base_name);

    return 0;
}