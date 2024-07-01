#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

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

char *get_tool_path_with_command(const char *tool_name) {
  char cmd[128];
  sprintf(cmd, "command -v %s", tool_name);

  FILE *pipe = popen(cmd, "r");
  if (!pipe) {
    perror("popen");
    return NULL;
  }

  char path[PATH_MAX];
  if (fgets(path, PATH_MAX, pipe) == NULL) {
    if (feof(pipe)) {
      // Tool not found
      return NULL;
    } else {
      perror("fgets");
      return NULL;
    }
  }

  // Remove newline character if present
  path[strcspn(path, "\n")] = 0;

  pclose(pipe);
  return strdup(path); // Remember to free this memory later
}

// int main() {
//   const char *tool_name = "ls";
// //   char *tool_path = find_tool_path_with_command(tool_name);
//   char *tool_path = find_tool_path(tool_name);
//   if (tool_path) {
//     printf("Path to '%s': %s\n", tool_name, tool_path);
//     free(tool_path); // Free the allocated memory
//   } else {
//     printf("Could not find '%s'\n", tool_name);
//   }
//   return 0;
// }

// pid_t get_pid() { return getpid(); }