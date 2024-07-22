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

char **strlist_new() { return malloc(sizeof(char *)); }

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

  int len_appending = 0;
  va_list args_copy;
  va_copy(args_copy, args);
  while (va_arg(args_copy, char *) != NULL) {
    len_appending = len_appending + 1;
  }
  va_end(args_copy);

  int len = strlist_len(*str_listp);
  char **strlist_tmp =
      realloc(*str_listp, sizeof(char *) * (len + len_appending + 1));
  if (strlist_tmp == 0) {
    perror("Failed to realloc memory\n");
    return EXIT_FAILURE;
  }
  *str_listp = strlist_tmp;

  for (int i = 0; i < len_appending; i++) {
    (*str_listp)[len + i] = va_arg(args, char *);
  }
  strlist_tmp[len + len_appending] = 0;
  va_end(args);

  return EXIT_SUCCESS;
}

// Implementation of asprintf function
char *my_asprintf(const char *format, ...) {
  // Initialize variable argument list
  va_list args;
  va_start(args, format);

  // First, determine the length of the formatted string
  // We use a temporary copy of the argument list for this
  va_list args_copy;
  va_copy(args_copy, args);
  int len = vsnprintf(NULL, 0, format, args_copy);
  va_end(args_copy);

  // Check if vsnprintf was successful
  if (len < 0) {
    // Error occurred during vsnprintf
    fprintf(stderr, "Error during vsnprintf.\n");
    va_end(args);
    return NULL;
  }

  // Allocate memory for the formatted string
  char *str = malloc(len + 1); // +1 for the null terminator
  if (!str) {
    // Memory allocation failed
    fprintf(stderr, "Memory allocation failed.\n");
    va_end(args);
    return NULL;
  }

  // Now format the string into the allocated memory
  int result = vsnprintf(str, len + 1, format, args);
  if (result < 0) {
    // Error occurred during the second vsnprintf call
    free(str); // Free the allocated memory to avoid a leak
    fprintf(stderr, "Error during vsnprintf while formatting.\n");
    va_end(args);
    return NULL;
  }

  // Clean up the variable argument list
  va_end(args);

  // Return the formatted string
  return str;
}

char *get_tool_path(const char *tool_name) {
  const char *path_env = getenv("PATH");
  if (!path_env)
    return NULL;

  char *path_copy = strdup(path_env);
  char *token = strtok(path_copy, ":");
  while (token != NULL) {
    char path[PATH_MAX];
    snprintf(path, PATH_MAX, "%s/%s", token, tool_name);
    if (access(path, X_OK) == 0) {
      free(path_copy);
      return strdup(path);
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
