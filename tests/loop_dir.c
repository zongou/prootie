#include <dirent.h>
#include <stdio.h>

int main() {
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
