#include "../utils/stringutils.c"
#include <stdio.h>

void test_stringutils() {
  char **strlist = strlist_new();
  strlist_add(&strlist, "hello");
  strlist_list(strlist, "strlist_add");
  strlist_add_va(&strlist, "world", "!", 0);
  strlist_list(strlist, "strlist_add_va");
}

void test_part_of_string() {
  char *str = "hello world";
  char buf[] = {str[0], str[1], 0};
  printf("part=%s\n", buf);
}

int main() {
  test_stringutils();
  test_part_of_string();
  return 0;
}