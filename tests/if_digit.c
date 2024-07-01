#include <stdio.h>
int main() {
  // false
  if (0) {
    printf("0\n");
  }

  // true
  if (1) {
    printf("1\n");
  }

  // true
  if (-1) {
    printf("-1\n");
  }

  // false
  if(NULL){
    printf("NULL\n");
  }

  return 0;
}
