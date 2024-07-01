#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

// 函数声明
void list_files_in_dir(const char *dir_path);

int main(int argc, char *argv[]) {
  // 调用函数，传入你想要遍历的目录路径
  list_files_in_dir(argv[1]);

  return 0;
}

void list_files_in_dir(const char *dir_path) {
  DIR *dir;
  struct dirent *entry;

  // 打开目录流
  dir = opendir(dir_path);
  if (dir == NULL) {
    perror("无法打开目录");
    return;
  }
  // 遍历目录中的每个条目
  while ((entry = readdir(dir)) != NULL) {
    // 跳过当前目录和父目录
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }

    // 构建完整的文件或子目录路径
    char full_path[1024]; // 这里的长度取决于你的文件路径长度
    snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);

    struct stat info;
    if (stat(full_path, &info) == -1) {
      perror("stat failed");
      continue;
    }

    // 检查是否为目录
    if (S_ISDIR(info.st_mode)) {
      printf("Directory: %s\n", full_path);
      // 递归调用，继续遍历子目录
      list_files_in_dir(full_path);
    } else {
      // 打印文件名
      printf("File: %s\n", full_path);
    }
  }

  // 关闭目录流
  closedir(dir);
}
