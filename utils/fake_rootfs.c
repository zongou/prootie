#include <dirent.h>
#include <linux/limits.h>
#include <linux/stat.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int strlist_add(char ***strlistp, char *str);
char *my_asprintf(const char *format, ...);

int get_fake_binding_strlist(char ***fake_bindings, char *rootfs_path,
                             int prefix_len) {
  DIR *dir;
  struct dirent *entry;

  // 打开目录流
  dir = opendir(rootfs_path);
  if (dir == NULL) {
    perror("Cannot open dir");
    return 1;
  }
  // 遍历目录中的每个条目
  while ((entry = readdir(dir)) != NULL) {
    // 跳过当前目录和父目录
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }

    // 构建完整的文件或子目录路径
    char full_path[PATH_MAX]; // 这里的长度取决于你的文件路径长度
    snprintf(full_path, sizeof(full_path), "%s/%s", rootfs_path, entry->d_name);

    struct stat info;
    if (stat(full_path, &info) == -1) {
      perror("stat failed");
      continue;
    }

    // 检查是否为目录
    if (S_ISDIR(info.st_mode)) {
      //   printf("Directory: %s\n", full_path);
      // 递归调用，继续遍历子目录
      get_fake_binding_strlist(fake_bindings, full_path, prefix_len);
    } else {
      char *path_to_root = full_path + prefix_len;
      if (access(path_to_root, R_OK)) {
        char *data = my_asprintf("%s:%s", full_path, path_to_root);
        strlist_add(fake_bindings, data);
      }
    }
  }

  // 关闭目录流
  closedir(dir);
  return 0;
}
