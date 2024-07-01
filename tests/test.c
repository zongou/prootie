#include "../utils/getpw.c"
#include "../utils/stringutils.c"
#include <pwd.h>
#include <stdio.h>

void test_stringutils() {
  char **strlist = strlist_new();
  strlist_add(&strlist, "hello");
  strlist_list(strlist, "strlist_add");
  strlist_add_va(&strlist, "world", "!", 0);
  strlist_list(strlist, "strlist_add_va");
}

void test_getpw() {
  passwd_path = "/data/data/com.termux/files/home/alpine/etc/passwd";

  struct passwd *pw;
  pw = getpw(passwd_path, "sync", 0);

  printf("login shell of %s is %s\n", pw->pw_name, pw->pw_shell);
}

#include <dirent.h>
#include <stdio.h>

int test_loopdir() {
  DIR *dir;
  struct dirent *entry;

  // 打开目录流
  dir = opendir(".");
  if (dir == NULL) {
    perror("无法打开目录");
    return 1;
  }

  // 遍历目录中的每个条目
  while ((entry = readdir(dir)) != NULL) {
    printf("%s\n", entry->d_name);
  }

  // 关闭目录流
  closedir(dir);

  return 0;
}

int main() {
  test_stringutils();
  test_getpw();
  test_loopdir();
  return 0;
}