#include <stdarg.h>
#include <stdio.h>

// 可变参数函数示例，打印多个整数
void printIntegers(int count, ...) {
  va_list args;

  // 初始化va_list
  va_start(args, count);

  for (int i = 0;; i++) {
    // 使用va_arg获取并打印每个整数
    int num = va_arg(args, int);
    if (!num) {
      break;
    }
    printf("%d ", num);
  }

  // 清理va_list
  va_end(args);
}

int main() {
  // 调用可变参数函数
  printIntegers(5, 1, 2, 3, 4, 5);
  return 0;
}