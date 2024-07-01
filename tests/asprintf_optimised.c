#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

/**
 * 动态地根据提供的格式和参数分配内存并打印字符串。
 * 
 * @param format 格式字符串，类似于printf的格式。
 * @return 动态分配的字符串指针，或在出错时返回NULL。
 */
char* my_asprintf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    
    // 使用vsnprintf来确定所需的缓冲区大小
    va_list args_copy;
    va_copy(args_copy, args);
    int len = vsnprintf(NULL, 0, format, args_copy);
    va_end(args_copy);
    
    // 检查vsnprintf是否成功
    if (len < 0) { // 发生错误
        // 记录或处理错误
        fprintf(stderr, "vsnprintf error.\n");
        va_end(args);
        return NULL;
    }
    
    // 根据计算的长度分配内存
    char *str = malloc(len + 1); // +1是为了'\0'
    if (!str) { // 分配失败
        // 记录或处理内存分配失败的错误
        fprintf(stderr, "Memory allocation failed.\n");
        va_end(args);
        return NULL;
    }
    
    // 现在真正格式化字符串
    int result = vsnprintf(str, len + 1, format, args);
    // 确保格式化字符串操作成功
    if (result < 0) {
        // 处理格式化错误
        free(str); // 避免内存泄漏
        fprintf(stderr, "vsnprintf error during formatting.\n");
        va_end(args);
        return NULL;
    }
    
    // 清理
    va_end(args);
    
    return str;
}

int main() {
    char *result = my_asprintf("Hello, %s! Today is %d/%d/%d.", 
                               "Alice", 10, 11, 2023);
    if (result) {
        printf("%s\n", result);
        free(result); // 记得释放内存
    } else {
        printf("Memory allocation failed or formatting error occurred.\n");
    }
    return 0;
}