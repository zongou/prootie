#include <dirent.h>
#include <errno.h>
#include <getopt.h>
#include <libgen.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

// Pre-allocate with reasonable initial size to reduce reallocations
#define STRLIST_INITIAL_SIZE 16

char **strlist_new() {
    char **list = calloc(STRLIST_INITIAL_SIZE, sizeof(char *));
    if (!list) {
        perror("Failed to allocate memory");
        exit(EXIT_FAILURE);
    }
    return list;
}

void list_argv(int argc, char **argv, char *tittle) {
  for (int i = 0; i < argc; i++) {
    fprintf(stderr, "%s[%d]=%s\n", tittle, i, argv[i]);
  }
}

int strlist_len(char **strlist) {
  int len = 0;
  while (1) {
    if (strlist[len] == NULL) {
      break;
    }
    len = len + 1;
  }
  return len;
}

void strlist_list(char **strlist, char *title) {
  list_argv(strlist_len(strlist), strlist, title);
}

int strlist_addl(char ***str_listp, ...) {
    va_list args;
    va_start(args, str_listp);

    // Count number of new items first
    int len_appending = 0;
    va_list args_copy;
    va_copy(args_copy, args);
    while (va_arg(args_copy, char *) != NULL) {
        len_appending++;
    }
    va_end(args_copy);

    // Calculate new size needed
    int current_len = strlist_len(*str_listp);
    int new_size = current_len + len_appending + 1;
    
    // Round up to next multiple of STRLIST_INITIAL_SIZE
    new_size = ((new_size + STRLIST_INITIAL_SIZE - 1) / STRLIST_INITIAL_SIZE) * STRLIST_INITIAL_SIZE;

    char **strlist_tmp = realloc(*str_listp, sizeof(char *) * new_size);
    if (!strlist_tmp) {
        perror("Failed to realloc memory");
        return EXIT_FAILURE;
    }
    *str_listp = strlist_tmp;

    // Add new items
    for (int i = 0; i < len_appending; i++) {
        (*str_listp)[current_len + i] = va_arg(args, char *);
    }
    (*str_listp)[current_len + len_appending] = NULL;
    
    va_end(args);
    return EXIT_SUCCESS;
}

// Optimized string formatting
char *my_asprintf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    
    va_list args_copy;
    va_copy(args_copy, args);
    int len = vsnprintf(NULL, 0, format, args_copy);
    va_end(args_copy);

    if (len < 0) {
        va_end(args);
        return NULL;
    }

    char *str = malloc(len + 1);
    if (!str) {
        va_end(args);
        return NULL;
    }

    int result = vsnprintf(str, len + 1, format, args);
    va_end(args);

    if (result < 0) {
        free(str);
        return NULL;
    }

    return str;
}

// Cached path search
char *get_tool_path(const char *tool_name) {
    static char path_buf[PATH_MAX];
    static char *last_tool = NULL;
    static char *last_path = NULL;

    // Return cached result if asking for same tool
    if (last_tool && strcmp(tool_name, last_tool) == 0) {
        return strdup(last_path);
    }

    const char *path_env = getenv("PATH");
    if (!path_env) return NULL;

    char *path_copy = strdup(path_env);
    char *token = strtok(path_copy, ":");
    
    while (token) {
        snprintf(path_buf, PATH_MAX, "%s/%s", token, tool_name);
        if (access(path_buf, X_OK) == 0) {
            free(path_copy);
            // Cache result
            free(last_tool);
            free(last_path);
            last_tool = strdup(tool_name);
            last_path = strdup(path_buf);
            return strdup(path_buf);
        }
        token = strtok(NULL, ":");
    }

    free(path_copy);
    return NULL;
}

void filelist(char ***strllist, const char *dir_path) {
  DIR *dir;
  struct dirent *entry;

  if ((dir = opendir(dir_path)) != NULL) {
    while ((entry = readdir(dir)) != NULL) {
      if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
        char new_path[PATH_MAX];
        snprintf(new_path, sizeof(new_path), "%s/%s", dir_path, entry->d_name);

        // Recursively call list_files if it's a directory
        if (entry->d_type == DT_DIR) {
          filelist(strllist, new_path);
        } else {
          strlist_addl(strllist, strdup(new_path), NULL);
        }
      }
    }
    closedir(dir);
  } else {
    perror("Could not open directory");
  }
}

void mkdir_wrapper(char *path) {
  if (mkdir(path, 0755) != 0) {
    fprintf(stderr, "Cannot create directory '%s': %s\n", path,
            strerror(errno));
    exit(EXIT_FAILURE);
  }
}
