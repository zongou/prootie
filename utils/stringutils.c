
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

char **strlist_new() { return calloc(1, sizeof(char *)); }

// void log_print(const char *format, ...) {
//   extern int is_verbose;
//   if (is_verbose) {
//     va_list args;
//     va_start(args, format);
//     vfprintf(stderr, format, args);
//     va_end(args);
//   }
// }

void list_string_array(int argc, char *argv[], char *title) {
  printf("==== List of %s ====\n", title);
  for (int i = 0; i < argc; i++) {
    printf("%s[%d]=%s\n", title, i, argv[i]);
  }
  printf("==== End list ====\n");
}

int strlist_len(char **str_list) {
  int i = 0;
  while (str_list[i] != NULL) {
    i++;
  }
  return i;
}

void strlist_list(char **str_list, char *title) {
  list_string_array(strlist_len(str_list), str_list, title);
}

/**
 * 添加一个字符串到args数组中。
 * @param str_listp 指向字符串数组的指针。
 * @param arg 待添加的字符串。
 * @return 如果成功，返回EXIT_SUCCESS；如果内存分配失败，返回EXIT_FAILURE。
 */
int strlist_add(char ***str_listp, char *str) {
  // 参数验证
  if (!str_listp || *str_listp == NULL) {
    // str_listp 尚未初始化，先进行初始化
    *str_listp = malloc(2 * sizeof(char *));
    if (*str_listp == NULL) {
      return EXIT_FAILURE; // 内存分配失败
    }
    (*str_listp)[0] = str;
    (*str_listp)[1] = NULL;
    return EXIT_SUCCESS;
  }

  int len =
      strlist_len(*str_listp); // 假设strlist_len正确实现并返回当前元素数量
  // new len = len + len_appending + 1
  char **new_strlist = realloc(*str_listp, sizeof(char *) * (len + 2));

  // 处理realloc的失败情况
  if (new_strlist == NULL) {
    return EXIT_FAILURE; // 内存分配失败
  }

  *str_listp = new_strlist;
  (*str_listp)[len] = str;
  (*str_listp)[len + 1] = NULL;

  return EXIT_SUCCESS;
}

int strlist_add_va(char ***str_listp, ...) {
  va_list args;
  va_start(args, str_listp);

  int len_appending = 0;
  va_list args_copy;
  va_copy(args_copy, args);
  while (1) {
    char *data = va_arg(args_copy, char *);
    if (!data) {
      break;
    }
    len_appending = len_appending + 1;
  }
  va_end(args_copy);

  int len = strlist_len(*str_listp);
  char **new_strlist =
      realloc(*str_listp, sizeof(char *) * (len + len_appending + 1));
  if (new_strlist == 0) {
    perror("Failed to realloc memory\n");
    exit(EXIT_FAILURE);
  }
  *str_listp = new_strlist;

  for (int i = 0; i < len_appending; i++) {
    (*str_listp)[len + i] = va_arg(args, char *);
  }
  new_strlist[len + len_appending] = 0;
  va_end(args);

  return EXIT_SUCCESS;
}

/**
 * 动态地根据提供的格式和参数分配内存并打印字符串。
 *
 * @param format 格式字符串，类似于printf的格式。
 * @return 动态分配的字符串指针，或在出错时返回NULL。
 */
char *my_asprintf(const char *format, ...) {
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
  if (!str) {                  // 分配失败
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
