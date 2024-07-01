#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

int my_asprintf(char **strp, const char *fmt, ...) {
    // 初始化变量
    va_list ap;
    int len, written;

    // 第一次调用 va_start 来初始化 ap，准备获取参数
    va_start(ap, fmt);

    // 使用 vasprintf 的方式计算所需长度，这里假设有一个虚构的vasprintf_len函数
    // 实际情况下vasprintf直接分配内存并返回写入的字符数，但这里为了演示分离计算长度
    len = vsnprintf(NULL, 0, fmt, ap); // 计算长度，不写入实际缓冲区

    // 检查长度计算是否成功
    if (len < 0) {
        va_end(ap); // 清理va_list
        return -1; // 发生错误
    }

    // 分配足够的内存给字符串，+1 是为了包含结尾的空字符'\0'
    *strp = (char*)malloc(len + 1);
    if (*strp == NULL) {
        va_end(ap);
        return -1; // 内存分配失败
    }

    // 第二次调用 va_start，因为之前已经va_end过，需要重新初始化
    va_start(ap, fmt);

    // 现在正式进行格式化写入到新分配的内存中
    written = vsnprintf(*strp, len + 1, fmt, ap);

    // 清理va_list
    va_end(ap);

    // 检查写入是否成功
    if (written < 0 || written > len) {
        free(*strp); // 如果出错，释放内存
        return -1;
    }

    return written; // 成功，返回写入的字符数
}

// 示例使用
int main() {
    char *result;
    int len;

    len = my_asprintf(&result, "Hello, %s!", "World");
    if (len >= 0) {
        printf("%s\n", result); // 输出结果
        free(result); // 记得释放内存
    } else {
        fprintf(stderr, "Error occurred.\n");
    }

    return 0;
}