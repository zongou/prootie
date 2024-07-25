/*
prootie login shortcut
 */

#include "../utils/utils.c"
#include <libgen.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  char *program = NULL;
  char *prootie_path = NULL;
  char **prootie_argv = NULL;
  char *rootfs = NULL;
  char cwd[PATH_MAX];

  program = basename(argv[0]);
  getcwd(cwd, sizeof(cwd));

  if (argc == 1) {
    fprintf(stderr, "%s: Usage %s [ROOTFS]\n", program, program);
    return EXIT_FAILURE;
  }

  prootie_path = get_tool_path("prootie");
  prootie_argv = strlist_new();

  strlist_addl(&prootie_argv, prootie_path, "login", rootfs, NULL);
  if (getenv("PREFIX") != NULL) {
    strlist_addl(&prootie_argv,
                 my_asprintf("--bind=%s", dirname(getenv("PREFIX"))), NULL);
  } else {
    strlist_addl(&prootie_argv, my_asprintf("--bind=%s", cwd), NULL);
  }
  if (access("/storage/emulated/0", R_OK) == 0) {
    strlist_addl(&prootie_argv, "--bind=/storage/emulated/0", NULL);
    strlist_addl(&prootie_argv, "--bind=/storage/emulated/0:/sdcard", NULL);
  }
  if (getenv("TMPDIR") != NULL) {
    strlist_addl(&prootie_argv, my_asprintf("--bind=%s:/tmp", getenv("TMPDIR")),
                 NULL);
  }
  strlist_addl(&prootie_argv, my_asprintf("--env=HOME=%s", getenv("HOME")),
               NULL);
  strlist_addl(
      &prootie_argv,
      "--env=PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin",
      NULL);
  strlist_addl(&prootie_argv, my_asprintf("--cwd=%s", cwd), NULL);
  // strlist_addl(&prootie_argv, "--host-utils", NULL);

  if (argc > 0) {
    strlist_addl(&prootie_argv, "--", NULL);
    for (int i = 1; i < argc; i++) {
      strlist_addl(&prootie_argv, argv[i], NULL);
    }
  }

  // strlist_list(prootie_argv, "plogin");
  execv(prootie_argv[0], prootie_argv);
  return 0;
}
