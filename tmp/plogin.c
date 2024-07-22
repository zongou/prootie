#include "../utils/utils.c"
#include <libgen.h>
#include <linux/limits.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  char *program = NULL;
  char *prootie_path = NULL;
  char **prootie_argv = NULL;
  char *rootfs = NULL;
  char cwd[PATH_MAX];

  program = basename(argv[0]);

  if (argc > 0) {
    rootfs = argv[1];
  } else {
    fprintf(stderr, "%s: Rootfs not set.\n", program);
    exit(EXIT_FAILURE);
  }

  char *distro_home = my_asprintf("%s/.distros", getenv("HOME"));

  prootie_path = get_tool_path("prootie");
  prootie_argv = strlist_new();

  strlist_addl(&prootie_argv, prootie_path, "-v", "login", rootfs, NULL);
  strlist_addl(&prootie_argv,
               my_asprintf("--bind=%s", dirname(getenv("PREFIX"))), NULL);
  strlist_addl(&prootie_argv, "--bind=/storage/emulated/0", NULL);
  strlist_addl(&prootie_argv, "--bind=/storage/emulated/0:/sdcard", NULL);
  strlist_addl(&prootie_argv, my_asprintf("--bind=%s:/tmp", getenv("TMPDIR")),
               NULL);
  strlist_addl(&prootie_argv, my_asprintf("--env=HOME=%s", getenv("HOME")),
               NULL);
  strlist_addl(&prootie_argv, my_asprintf("--cwd=%s", getcwd(cwd, sizeof(cwd))),
               NULL);
  strlist_addl(&prootie_argv, "--host-utils", NULL);

  if (argc > 2) {
    strlist_addl(&prootie_argv, "--", NULL);
    for (int i = 2; i < argc; i++) {
      strlist_addl(&prootie_argv, argv[i], NULL);
    }
  }

  strlist_list(prootie_argv, "prootie_argv");
  execv(prootie_argv[0], prootie_argv);
  return 0;
}