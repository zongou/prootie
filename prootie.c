// #include <bits/getopt.h>
#include <bits/getopt.h>
#include <dirent.h>
#include <getopt.h>
#include <libgen.h>
#include <limits.h>
#include <linux/limits.h>
#include <pwd.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "utils/command.c"
#include "utils/fake_rootfs.c"
#include "utils/getpw.c"
#include "utils/stringutils.c"

static char *prog_path;
static char *prog_short;
static char *command_name;
static int is_verbose;

int command_install(int argc, char *argv[]) {
  printf("Install %s\n", argv[1]);
  return 0;
}

void show_login_help() {
  fprintf(stderr, "\
Login into an installed rootfs.\n\
\n\
Usage:\n\
  ## Login default shell as root\n\
  %s %s [OPTION...] [ROOTFS_DIR]\n\
  \n\
  ## Login and run commands\n\
  %s %s [OPTION...] [ROOTFS_DIR] -- [COMMAND] ...\n\
\n\
Options:\n\
  -h, --help          show this help\n\
  --host-utils        enhances anotherterm & termux\n\
  --                  run commands within rootfs\n\
  \n\
PRoot relavent options:\n\
  -b, --bind, -m, --mount\n\
  --no-kill-on-exit\n\
  --no-link2symlink\n\
  --no-sysvipc\n\
  --fix-low-ports\n\
  -q, --qemu\n\
  -k, --kernel-release\n\
",
          prog_short, command_name, prog_short, command_name);
}

int command_login(int argc, char *argv[]) {
  if (is_verbose) {
    list_string_array(argc, argv, "argv");
  }
  command_name = "login";
  if (argc < 2) {
    show_login_help();
    return 0;
  }

  static struct {
    int *argc;
    char **argv;

    char **accepted_argv;
    char **forwarded_argv;

    char **proot_argv;
    char **proot_envp;

    char *rootfs;
    char *cwd;
    int kill_on_exit;
    int link2symlink;
    int fix_low_ports;
    char *kernel_release;
    char *qemu;
    char **bind_mounts;
  } login_options;

  login_options.argc = &argc;
  login_options.argv = argv;

  // Split argv with token '--'. comparing reversed
  int split_token_index = argc;
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[argc - i], "--") == 0) {
      split_token_index = argc - i;
      break;
    }
  }

  // list_array(split_token_index, argv, "accepted_argv");
  // printf("split_token_index=%d\n", split_token_index);
  // list_array(argc - split_token_index - 1, argv + split_token_index + 1,
  //            "forwarded_argv");

  login_options.kill_on_exit = 1;
  login_options.bind_mounts = NULL;
  login_options.link2symlink = 1;
  login_options.fix_low_ports = 0;
  login_options.cwd = "/root";

  static struct option long_options[] = {
      {"help", no_argument, NULL, 'h'},
      {"no-kill-on-exit", no_argument, &login_options.kill_on_exit, 0},
      {"no-link2symlink", no_argument, &login_options.link2symlink, 0},
      {"fix-low-ports", no_argument, &login_options.fix_low_ports, 1},
      {"bind", required_argument, NULL, 'b'},
      {"pwd", required_argument, NULL, 'w'},
      {"cwd", required_argument, NULL, 'w'},
      {"kernel-release", required_argument, NULL, 'k'},
      {"qemu", required_argument, NULL, 'q'},
      {NULL, 0, NULL, 0}}; // End mark
  int c;
  while ((c = getopt_long(split_token_index, argv, "hvb:w:k:q:", long_options,
                          NULL)) != -1) {
    // printf("optind=%d\n", optind);
    switch (c) {
    case 0:
      printf("longopt[%d]=%s, %s\n", optind, long_options[optind].name, optarg);
      break;
    case 'h':
      show_login_help();
      break;
    case 'v':
      printf("Version information.\n");
      break;
    case 'b':
      if (login_options.bind_mounts == NULL) {
        login_options.bind_mounts = strlist_new();
      }
      strlist_add(&login_options.bind_mounts, optarg);
      break;
    case 'w':
      login_options.cwd = optarg;
      break;
    case 'k':
      login_options.kernel_release = optarg;
      break;
    case 'q':
      login_options.qemu = optarg;
      break;
    case '?':
      fprintf(stderr, "%s:%s: Unknown option '%s'.\n", prog_path, command_name,
              argv[optind - 1]);
      exit(EXIT_FAILURE);
      break;
    default:
      abort();
    }
  }

  // if (optind < split_token_index) {
  //   list_array(split_token_index - optind, argv + optind,
  //              "non-option ARGV-elements");
  // }

  // list_array(split_token_index, argv, "getopt_long");
  // printf("optind[%d]=%s\n", optind, argv[optind]);
  if (optind == split_token_index - 1) {
    if (access(argv[optind], R_OK) == 0) {
      login_options.rootfs = argv[optind];
    } else {
      fprintf(stderr, "%s:%s: Cannot access rootfs dir '%s'\n", prog_path,
              command_name, argv[optind]);
      exit(EXIT_FAILURE);
    }
  } else {
    fprintf(stderr, "wrong arguments\n");
    exit(EXIT_FAILURE);
  }

  char *proot_path = get_tool_path("proot");
  if (proot_path == NULL) {
    fprintf(stderr, "%s:%s: Cannot find proot\n", prog_path, command_name);
    exit(EXIT_FAILURE);
  }

  char **proot_envp = strlist_new();
  char resolved_rootfs[PATH_MAX];
  char *r = realpath(login_options.rootfs, resolved_rootfs);
  char *proot_tmp_dir =
      my_asprintf("%s/.proot/sessions/%ld", resolved_rootfs, getpid());
  umask(0000);
  mkdir(proot_tmp_dir, S_IRWXU | S_IRWXG | S_IRWXO);
  strlist_add(&proot_envp, my_asprintf("PROOT_TMP_DIR=%s", proot_tmp_dir));
  char *proot_l2s_dir = my_asprintf("%s/.proot/meta", resolved_rootfs);
  strlist_add(&proot_envp, my_asprintf("PROOT_L2S_DIR=%s", proot_l2s_dir));
  if (is_verbose) {
    strlist_list(proot_envp, "env");
  }

  char **proot_argv = strlist_new();
  strlist_add(&proot_argv, proot_path);

  strlist_add(&proot_argv, my_asprintf("--rootfs=%s", resolved_rootfs));
  if (login_options.qemu != NULL) {
    strlist_add(&proot_argv, my_asprintf("--qemu=%s", login_options.qemu));
  }
  strlist_add(&proot_argv, my_asprintf("--cwd=%s", login_options.cwd));

  if (login_options.kill_on_exit) {
    strlist_add(&proot_argv, "--kill-on-exit");
  }

  if (login_options.kernel_release != NULL) {
    strlist_add(&proot_argv, my_asprintf("--kernel-release=%s",
                                         login_options.kernel_release));
  }

  strlist_add(&proot_argv, "--root-id");
  if (login_options.link2symlink) {
    strlist_add(&proot_argv, "--link2symlink");
  }
  strlist_add(&proot_argv, "--sysvipc");
  strlist_add(&proot_argv, "--ashmem-memfd");
  strlist_add(&proot_argv, "-H");
  if (login_options.fix_low_ports) {
    strlist_add(&proot_argv, "-p");
  }
  strlist_add(&proot_argv, "-L");

  // Core file systems that should always be present.
  strlist_add(&proot_argv, "--bind=/dev");
  strlist_add(&proot_argv, "--bind=/dev/urandom:/dev/random");
  strlist_add(&proot_argv, "--bind=/proc");
  strlist_add(&proot_argv, "--bind=/proc/self/fd:/dev/fd");
  if (isatty(STDIN_FILENO)) {
    strlist_add(&proot_argv, "--bind=/proc/self/fd/0:/dev/stdin");
  }
  strlist_add(&proot_argv, "--bind=/proc/self/fd/1:/dev/stdout");
  strlist_add(&proot_argv, "--bind=/proc/self/fd/2:/dev/stderr");
  strlist_add(&proot_argv, "--bind=/sys");

  // Bind faked files
  char *proot_fake_dir = my_asprintf("%s/.proot/rootfs", resolved_rootfs);
  char **fake_bindings = strlist_new();
  get_fake_binding_strlist(&fake_bindings, proot_fake_dir,
                           strlen(proot_fake_dir));

  // strlist_list(fake_bindings, "fake bindings");
  for (int i = 0; i < strlist_len(fake_bindings); i++) {
    strlist_add(&proot_argv, my_asprintf("--bind=%s", fake_bindings[i]));
  }

  // strlist_list(login_options.bind_mounts, "bindings");
  if (login_options.bind_mounts) {
    for (int i = 0; login_options.bind_mounts[i]; i++) {
      strlist_add(&proot_argv,
                  my_asprintf("--bind=%s", login_options.bind_mounts[i]));
    }
  }

  if (split_token_index != argc) {
    for (int i = 0; i < argc - split_token_index - 1; i++) {
      strlist_add(&proot_argv, (argv + split_token_index + 1)[i]);
    }
  } else {
    strlist_add(&proot_argv, "/usr/bin/env");
    strlist_add(&proot_argv, "-i");
    strlist_add(&proot_argv, "HOME=/root");
    strlist_add(&proot_argv, "LANG=C.UTF-8");
    strlist_add(&proot_argv, "TERM=xterm-256color");
    // strlist_add(&proot_argv, "/bin/sh");
    strlist_add(
        &proot_argv,
        getpw(my_asprintf("%s/etc/passwd", resolved_rootfs), 0, 0)->pw_shell);

    strlist_add(&proot_argv, "-l");
  }

  if (is_verbose) {
    strlist_list(proot_argv, "args");
  }
  execve(proot_path, proot_argv, proot_envp);

  exit(0);
}

int command_archive(int argc, char *argv[]) {
  printf("Archive %s\n", argv[1]);
  return 0;
}

void show_help() {
  fprintf(stderr, "\
%s super charges PRoot.\n\
\n\
Usage:\n\
  %s [OPTION...] [COMMAND]\n\
\n\
  ## show help of each command\n\
  %s [COMMAND] --help\n\
\n\
Options:\n\
  -h, --help          show this help\n\
  -v, --verbose       print more information\n\
\n\
Commands:\n\
  install             install rootfs\n\
  login               login rootfs\n\
  archive             archive rootfs\n\
\n\
Related environment variables:\n\
  PROOT               path to proot\n\
",
          prog_short, prog_short, prog_short);
}

int main(int argc, char *argv[]) {
  prog_path = argv[0];
  prog_short = basename(prog_path);

  if (argc < 2) {
    show_help();
    return 1;
  }

  for (int i = 1; i < argc; i++) {
    switch (argv[i][0]) {
    // // Empty string argument is passed as 0
    // case 0:
    //   break;
    case '-':
      if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
        is_verbose = 1;
      } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
        show_help();
        exit(EXIT_SUCCESS);
      } else {
        fprintf(stderr, "%s: Unknown option '%s'\n", prog_path, argv[i]);
        exit(EXIT_FAILURE);
      }
      break;
    default:
      command_name = argv[i];
      int forwarded_argc = argc - i;
      char **forwarded_argv = argv + i;

      if (strcmp("install", command_name) == 0) {
        command_install(forwarded_argc, forwarded_argv);
      } else if (strcmp("login", command_name) == 0) {
        command_login(forwarded_argc, forwarded_argv);
      } else if (strcmp("archive", command_name) == 0) {
        command_archive(forwarded_argc, forwarded_argv);
      } else {
        fprintf(stderr, "%s: Unknown command '%s'\n", prog_path, command_name);
        exit(EXIT_FAILURE);
      }
      break;
    }
  }

  return 0;
}
