/*
Note: PROOT_L2S_DIR must be absolute path, and it only affects hard link
creation.
PROOT_NO_SECCOMP=1
 */

#include <getopt.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "utils/getpw.c"
#include "utils/utils.c"

#define FMT_PROOT_DATA_DIR "%s/.proot"
#define FMT_PROOT_L2S_DIR "%s/.proot/l2s"
#define FMT_PROOT_FAKEROOTFS_DIR "%s/.proot/rootfs"
#define HOT_UTILS "host_utils.sh"

#define ROOTFS_NOT_SET fprintf(stderr, "%s: Rootfs not set.\n", program);
#define SHOW_HELP                                                              \
  if (argc < 2) {                                                              \
    fputs(help_info, stderr);                                                  \
    return EXIT_FAILURE;                                                       \
  }

int is_verbose;
char *program;
char *help_info;
char *command;
char *rootfs_dir;
int link2symlink_default;

int is_android() { return access("/system/bin/app_process", F_OK) == 0; }
int is_termux() { return getenv("TERMUX_VERSION") != NULL; }
int is_anotherterm() { return getenv("TERMSH") != NULL; }

void set_proot_path(char ***strlistp) {
  char *proot_path = NULL;
  if ((proot_path = getenv("PROOT")) != NULL) {
    strlist_addl(strlistp, proot_path, NULL);
  } else if ((proot_path = get_tool_path("proot")) != NULL) {
    strlist_addl(strlistp, proot_path, NULL);
  } else {
    fprintf(stderr, "%s: Cannot find proot in PATH and env\n", program);
    exit(EXIT_FAILURE);
  }
}

void set_proot_env(char ***strlistp) {
  strlist_addl(
      strlistp,
      my_asprintf("PROOT_L2S_DIR=%s",
                  my_asprintf(FMT_PROOT_L2S_DIR, rootfs_dir, getpid())),
      NULL);

  char *tmp = NULL;
  if ((tmp = getenv("PROOT_TMP_DIR")) != NULL) {
    strlist_addl(strlistp, my_asprintf("PROOT_TMP_DIR=%s", tmp), NULL);
    return;
  }

  if ((tmp = getenv("TMPDIR")) != NULL) {
    strlist_addl(strlistp, my_asprintf("PROOT_TMP_DIR=%s", tmp), NULL);
    return;
  }
}

void sigint_handler(int signum) {
  // printf("Caught SIGINT. Custom handler in action...\n");
  // printf("Changing signal handler back to default...\n");
  // signal(SIGINT, SIG_DFL); // Reset to default handler
}

int command_install(int argc, char *argv[]) {
  help_info = my_asprintf("\
Install rootfs to the specific dir from stdin.\n\
\n\
Usage:\n\
  %s %s [OPTION...] [ROOTFS]\n\
\n\
Options:\n\
  --help              show this help\n\
\n\
Tar relavent options:\n\
  -v, --verbose\n\
  --exclude\n\
",
                          program, command);
  SHOW_HELP

  static struct {
    int tar_is_verbose;
    char **tar_excludes;
  } options;

  options.tar_is_verbose = 0;
  options.tar_excludes = NULL;

  int c;
  int longopt_index;

  static struct option long_options[] = {
      {"help", no_argument, NULL, 'h'},
      {"verbose", no_argument, NULL, 'v'},
      {"exclude", required_argument, NULL, 0},
      {NULL, 0, NULL, 0}};

  while ((c = getopt_long(argc, argv, "hv", long_options, &longopt_index)) !=
         -1) {
    switch (c) {
    case 0:
      if (strcmp("exclude", long_options[longopt_index].name) == 0) {
        if (options.tar_excludes == NULL) {
          options.tar_excludes = strlist_new();
        }
        strlist_addl(&options.tar_excludes, optarg, NULL);
      }
      break;
    case 'h':
      fputs(help_info, stderr);
      return EXIT_SUCCESS;
    case 'v':
      options.tar_is_verbose = 1;
      break;
    case '?':
      fprintf(stderr, "%s: Unknown option '%s'.\n", program, argv[optind - 1]);
      return EXIT_FAILURE;
      break;
    default:
      abort();
    }
  }

  if (optind < argc) {
    if (access(argv[optind], F_OK) != 0) {
      mkdir_wrapper(argv[optind]);
      rootfs_dir = realpath(argv[optind], NULL);
      optind = optind + 1;
    } else {
      fprintf(stderr, "%s: Rootfs '%s' already exists.\n", program,
              argv[optind]);
      return EXIT_FAILURE;
    }
  } else {
    ROOTFS_NOT_SET
    return EXIT_FAILURE;
  }

  char **proot_envp = strlist_new();
  char **proot_argv = strlist_new();

  set_proot_env(&proot_envp);

  set_proot_path(&proot_argv);
  strlist_addl(&proot_argv, "--link2symlink", "--root-id", NULL);

  char *tar_path = NULL;
  if ((tar_path = get_tool_path("tar")) != NULL) {
    strlist_addl(&proot_argv, tar_path, NULL);
  } else {
    fprintf(stderr, "%s: Cannot find tar in PATH\n", program);
    return EXIT_FAILURE;
  }

  if (options.tar_is_verbose) {
    strlist_addl(&proot_argv, "-v", NULL);
  }

  strlist_addl(&proot_argv, "-C", rootfs_dir, "-x", NULL);
  if (options.tar_excludes != NULL && strlist_len(options.tar_excludes) > 0) {
    for (int i = 0; i < strlist_len(options.tar_excludes); i++) {
      strlist_addl(&proot_argv,
                   my_asprintf("--exclude=%s", options.tar_excludes[i]), NULL);
    }
  }

  pid_t pid;
  pid = fork();
  if (pid < 0) {
    perror("fork");
    return EXIT_FAILURE;
  } else if (pid == 0) {
    if (is_verbose) {
      strlist_list(proot_envp, "proot_envp");
      strlist_list(proot_argv, "proot_argv");
    }
    // close(STDERR_FILENO);
    mkdir_wrapper(my_asprintf(FMT_PROOT_DATA_DIR, rootfs_dir));
    mkdir_wrapper(my_asprintf(FMT_PROOT_L2S_DIR, rootfs_dir));
    execve(proot_argv[0], proot_argv, proot_envp);
    perror("execve");
  } else {
    int status;
    signal(SIGINT, sigint_handler);
    waitpid(pid, &status, 0);
    if (WIFEXITED(status)) {
      if (WEXITSTATUS(status) != 0) {
        fprintf(stderr, "%s: Child exited with status: %d\n", program, status);
        fprintf(stderr, "%s: Failed.\n", program);
        fprintf(stderr, "%s: Cleaning up...\n", program);
        system(my_asprintf("chmod +rw -R %s", rootfs_dir));
        system(my_asprintf("rm -rf %s", rootfs_dir));
        return EXIT_FAILURE;
      }
    } else {
      fprintf(stderr, "%s: Child did not terminate normally.\n", program);
      return EXIT_FAILURE;
    }
  }

  mkdir_wrapper(my_asprintf(FMT_PROOT_FAKEROOTFS_DIR, rootfs_dir));
  mkdir_wrapper(my_asprintf(FMT_PROOT_FAKEROOTFS_DIR "/proc", rootfs_dir));

  fputs("0.00 0.00 0.00 0/0 0\n",
        fopen(my_asprintf(FMT_PROOT_FAKEROOTFS_DIR "/proc/loadavg", rootfs_dir),
              "w"));
  fputs("cpu  0 0 0 0 0 0 0 0 0 0\n",
        fopen(my_asprintf(FMT_PROOT_FAKEROOTFS_DIR "/proc/stat", rootfs_dir),
              "w"));
  fputs("0.00 0.00\n",
        fopen(my_asprintf(FMT_PROOT_FAKEROOTFS_DIR "/proc/uptime", rootfs_dir),
              "w"));
  fputs("\
Linux localhost 6.1.0-22 #1 SMP PREEMPT_DYNAMIC 6.1.94-1 (2024-06-21) GNU/Linux\n\
",
        fopen(my_asprintf(FMT_PROOT_FAKEROOTFS_DIR "/proc/version", rootfs_dir),
              "w"));
  fputs("\
nr_free_pages 136777\n\
nr_zone_inactive_anon 14538\n\
nr_zone_active_anon 215\n\
nr_zone_inactive_file 45670\n\
nr_zone_active_file 14489\n\
nr_zone_unevictable 6936\n\
nr_zone_write_pending 812\n\
nr_mlock 6936\n\
nr_bounce 0\n\
nr_zspages 0\n\
nr_free_cma 0\n\
numa_hit 325814\n\
numa_miss 0\n\
numa_foreign 0\n\
numa_interleave 2632\n\
numa_local 325814\n\
numa_other 0\n\
nr_inactive_anon 14538\n\
nr_active_anon 215\n\
nr_inactive_file 45670\n\
nr_active_file 14489\n\
nr_unevictable 6936\n\
nr_slab_reclaimable 6400\n\
nr_slab_unreclaimable 7349\n\
nr_isolated_anon 0\n\
nr_isolated_file 0\n\
workingset_nodes 0\n\
workingset_refault_anon 0\n\
workingset_refault_file 0\n\
workingset_activate_anon 0\n\
workingset_activate_file 0\n\
workingset_restore_anon 0\n\
workingset_restore_file 0\n\
workingset_nodereclaim 0\n\
nr_anon_pages 19166\n\
nr_mapped 18188\n\
nr_file_pages 62691\n\
nr_dirty 812\n\
nr_writeback 0\n\
nr_writeback_temp 0\n\
nr_shmem 266\n\
nr_shmem_hugepages 0\n\
nr_shmem_pmdmapped 0\n\
nr_file_hugepages 0\n\
nr_file_pmdmapped 0\n\
nr_anon_transparent_hugepages 0\n\
nr_vmscan_write 0\n\
nr_vmscan_immediate_reclaim 0\n\
nr_dirtied 1688\n\
nr_written 875\n\
nr_kernel_misc_reclaimable 0\n\
nr_foll_pin_acquired 0\n\
nr_foll_pin_released 0\n\
nr_kernel_stack 2172\n\
nr_page_table_pages 492\n\
nr_swapcached 0\n\
nr_dirty_threshold 35995\n\
nr_dirty_background_threshold 17975\n\
pgpgin 299220\n\
pgpgout 4416\n\
pswpin 0\n\
pswpout 0\n\
pgalloc_dma 32\n\
pgalloc_dma32 333528\n\
pgalloc_normal 0\n\
pgalloc_movable 0\n\
allocstall_dma 0\n\
allocstall_dma32 0\n\
allocstall_normal 0\n\
allocstall_movable 0\n\
pgskip_dma 0\n\
pgskip_dma32 0\n\
pgskip_normal 0\n\
pgskip_movable 0\n\
pgfree 478037\n\
pgactivate 13017\n\
pgdeactivate 0\n\
pglazyfree 20\n\
pgfault 196449\n\
pgmajfault 1180\n\
pglazyfreed 0\n\
pgrefill 0\n\
pgreuse 36999\n\
pgsteal_kswapd 0\n\
pgsteal_direct 0\n\
pgdemote_kswapd 0\n\
pgdemote_direct 0\n\
pgscan_kswapd 0\n\
pgscan_direct 0\n\
pgscan_direct_throttle 0\n\
pgscan_anon 0\n\
pgscan_file 0\n\
pgsteal_anon 0\n\
pgsteal_file 0\n\
zone_reclaim_failed 0\n\
pginodesteal 0\n\
slabs_scanned 0\n\
kswapd_inodesteal 0\n\
kswapd_low_wmark_hit_quickly 0\n\
kswapd_high_wmark_hit_quickly 0\n\
pageoutrun 0\n\
pgrotated 0\n\
drop_pagecache 0\n\
drop_slab 0\n\
oom_kill 0\n\
numa_pte_updates 0\n\
numa_huge_pte_updates 0\n\
numa_hint_faults 0\n\
numa_hint_faults_local 0\n\
numa_pages_migrated 0\n\
pgmigrate_success 0\n\
pgmigrate_fail 0\n\
thp_migration_success 0\n\
thp_migration_fail 0\n\
thp_migration_split 0\n\
compact_migrate_scanned 0\n\
compact_free_scanned 0\n\
compact_isolated 0\n\
compact_stall 0\n\
compact_fail 0\n\
compact_success 0\n\
compact_daemon_wake 0\n\
compact_daemon_migrate_scanned 0\n\
compact_daemon_free_scanned 0\n\
htlb_buddy_alloc_success 0\n\
htlb_buddy_alloc_fail 0\n\
unevictable_pgs_culled 93162\n\
unevictable_pgs_scanned 0\n\
unevictable_pgs_rescued 7\n\
unevictable_pgs_mlocked 6943\n\
unevictable_pgs_munlocked 7\n\
unevictable_pgs_cleared 0\n\
unevictable_pgs_stranded 0\n\
thp_fault_alloc 0\n\
thp_fault_fallback 0\n\
thp_fault_fallback_charge 0\n\
thp_collapse_alloc 0\n\
thp_collapse_alloc_failed 0\n\
thp_file_alloc 0\n\
thp_file_fallback 0\n\
thp_file_fallback_charge 0\n\
thp_file_mapped 0\n\
thp_split_page 0\n\
thp_split_page_failed 0\n\
thp_deferred_split_page 0\n\
thp_split_pmd 0\n\
thp_split_pud 0\n\
thp_zero_page_alloc 0\n\
thp_zero_page_alloc_failed 0\n\
thp_swpout 0\n\
thp_swpout_fallback 0\n\
balloon_inflate 0\n\
balloon_deflate 0\n\
balloon_migrate 0\n\
swap_ra 0\n\
swap_ra_hit 0\n\
direct_map_level2_splits 28\n\
direct_map_level3_splits 0\n\
nr_unstable 0\n\
",
        fopen(my_asprintf(FMT_PROOT_FAKEROOTFS_DIR "/proc/vmstat", rootfs_dir),
              "w"));

  mkdir_wrapper(my_asprintf(FMT_PROOT_FAKEROOTFS_DIR "/proc/sys", rootfs_dir));
  mkdir_wrapper(
      my_asprintf(FMT_PROOT_FAKEROOTFS_DIR "/proc/sys/kernel", rootfs_dir));
  fputs("40\n", fopen(my_asprintf(FMT_PROOT_FAKEROOTFS_DIR
                                  "/proc/sys/kernel/cap_last_cap",
                                  rootfs_dir),
                      "w"));

  mkdir_wrapper(
      my_asprintf(FMT_PROOT_FAKEROOTFS_DIR "/proc/sys/fs", rootfs_dir));
  mkdir_wrapper(
      my_asprintf(FMT_PROOT_FAKEROOTFS_DIR "/proc/sys/fs/inotify", rootfs_dir));
  fputs("16384\n", fopen(my_asprintf(FMT_PROOT_FAKEROOTFS_DIR
                                     "/proc/sys/fs/inotify/max_queued_events",
                                     rootfs_dir),
                         "w"));
  fputs("128\n", fopen(my_asprintf(FMT_PROOT_FAKEROOTFS_DIR
                                   "/proc/sys/fs/inotify/max_user_instances",
                                   rootfs_dir),
                       "w"));
  fputs("65536\n", fopen(my_asprintf(FMT_PROOT_FAKEROOTFS_DIR
                                     "/proc/sys/fs/inotify/max_user_watches",
                                     rootfs_dir),
                         "w"));

  mkdir_wrapper(my_asprintf(FMT_PROOT_FAKEROOTFS_DIR "/etc", rootfs_dir));
  fputs("\
nameserver 8.8.8.8\n\
nameserver 8.8.4.4\n\
",
        fopen(my_asprintf(FMT_PROOT_FAKEROOTFS_DIR "/etc/resolv.conf",
                          rootfs_dir),
              "w"));
  fputs("\
# IPv4.\n\
127.0.0.1   localhost.localdomain localhost\n\
\n\
# IPv6.\n\
::1         localhost.localdomain localhost ip6-localhost ip6-loopback\n\
fe00::0     ip6-localnet\n\
ff00::0     ip6-mcastprefix\n\
ff02::1     ip6-allnodes\n\
ff02::2     ip6-allrouters\n\
ff02::3     ip6-allhosts\n\
",
        fopen(my_asprintf(FMT_PROOT_FAKEROOTFS_DIR "/etc/hosts", rootfs_dir),
              "w"));

  mkdir_wrapper(
      my_asprintf(FMT_PROOT_FAKEROOTFS_DIR "/etc/profile.d", rootfs_dir));
  fputs("\
export CHARSET=${CHARSET:-UTF-8}\n\
export LANG=${LANG:-C.UTF-8}\n\
export LC_COLLATE=${LC_COLLATE:-C}\n\
",
        fopen(my_asprintf(FMT_PROOT_FAKEROOTFS_DIR "/etc/profile.d/locale.sh",
                          rootfs_dir),
              "w"));
  fputs("\
export COLORTERM=truecolor\n\
[ -z \"\\$LANG\" ] && export LANG=C.UTF-8\n\
export TERM=xterm-256color\n\
export TMPDIR=/tmp\n\
export PULSE_SERVER=127.0.0.1\n\
export MOZ_FAKE_NO_SANDBOX=1\n\
export CHROMIUM_FLAGS=--no-sandbox\n\
",
        fopen(my_asprintf(FMT_PROOT_FAKEROOTFS_DIR "/etc/profile.d/proot.sh",
                          rootfs_dir),
              "w"));

  if (is_android()) {
    FILE *fp =
        fopen(my_asprintf(FMT_PROOT_FAKEROOTFS_DIR "/etc/profile.d/" HOT_UTILS,
                          rootfs_dir),
              "w");
    fprintf(fp, "\
## prootie host utils\n\
export ANDROID_DATA='%s'\n\
export ANDROID_RUNTIME_ROOT='%s'\n\
export ANDROID_TZDATA_ROOT='%s'\n\
export BOOTCLASSPATH='%s'\n\
",
            getenv("ANDROID_DATA"), getenv("ANDROID_RUNTIME_ROOT"),
            getenv("ANDROID_TZDATA_ROOT"), getenv("BOOTCLASSPATH"));

    if (is_anotherterm()) {
      fprintf(fp, "\
export DATA_DIR='%s'\n\
export TERMSH_UID='%u'\n\
export TERMSH='%s/libtermsh.so'\n\
",
              getenv("DATA_DIR"), getuid(), getenv("LIB_DIR"));
    }

    char *prefix_dir = getenv("PREFIX");
    if (prefix_dir != NULL) {
      fprintf(fp, "export PREFIX='%s'\n", prefix_dir);
      fprintf(fp, "export PATH=\"${PATH}:${PREFIX}/bin\"\n");
    }
  }

  return 0;
}

int command_login(int argc, char *argv[]) {
  help_info = my_asprintf("\
Start login shell of the specific rootfs.\n\
\n\
Usage:\n\
  ## Start login shell as root\n\
  %s %s [OPTION...] [ROOTFS]\n\
  \n\
  ## Execute a command\n\
  %s %s [OPTION...] [ROOTFS] -- [COMMAND [ARG]...]\n\
\n\
Options:\n\
  -h, --help          show this help\n\
  --host-utils        enable host utils\n\
  --env               set environment variables\n\
\n\
PRoot relavent options:\n\
  -b, --bind, -m, --mount\n\
  --no-kill-on-exit\n\
  --link2symlink\n\
  --no-link2symlink\n\
  --no-sysvipc\n\
  --fix-low-ports\n\
  -q, --qemu\n\
  -k, --kernel-release\n\
",
                          program, command, program, command);
  SHOW_HELP

  static struct {
    char *cwd;
    int kill_on_exit;
    int link2symlink;
    int fix_low_ports;
    char *kernel_release;
    char *qemu;
    int sysvipc;
    char **bindings;
    int host_utils;
    char **env;
  } options;

  options.kill_on_exit = 1;
  options.bindings = NULL;
  options.link2symlink = link2symlink_default;
  options.fix_low_ports = 0;
  options.cwd = "/root";
  options.sysvipc = 1;
  options.host_utils = 0;

  int c;
  int longopt_index;

  static struct option long_options[] = {
      {"help", no_argument, NULL, 'h'},
      {"no-kill-on-exit", no_argument, &options.kill_on_exit, 0},
      {"link2symlink", no_argument, &options.link2symlink, 1},
      {"no-link2symlink", no_argument, &options.link2symlink, 0},
      {"fix-low-ports", no_argument, &options.fix_low_ports, 1},
      {"bind", required_argument, NULL, 'b'},
      {"cwd", required_argument, NULL, 'w'},
      {"kernel-release", required_argument, NULL, 'k'},
      {"qemu", required_argument, NULL, 'q'},
      {"host-utils", no_argument, &options.host_utils, 1},
      {"env", required_argument, NULL, 0},
      {NULL, 0, NULL, 0}};

  while ((c = getopt_long(argc, argv, "hb:w:k:q:", long_options,
                          &longopt_index)) != -1) {
    switch (c) {
    case 0:
      /* If this option set a flag, do nothing else now. */
      // if (long_options[longopt_index].flag == 0) {
      //   printf("long_option[%d]:%s=%s", longopt_index,
      //          long_options[longopt_index].name, optarg);
      // }
      if (strcmp("env", long_options[longopt_index].name) == 0) {
        if (options.env == NULL) {
          options.env = strlist_new();
        }
        strlist_addl(&options.env, optarg, NULL);
      }
      break;
    case 'h':
      fputs(help_info, stderr);
      return EXIT_SUCCESS;
      break;
    case 'b':
      if (options.bindings == NULL) {
        options.bindings = strlist_new();
      }
      strlist_addl(&options.bindings, optarg, NULL);
      break;
    case 'w':
      options.cwd = optarg;
      break;
    case 'k':
      options.kernel_release = optarg;
      break;
    case 'q':
      options.qemu = optarg;
      break;
    case '?':
      fprintf(stderr, "%s: Unknown option '%s'.\n", program, argv[optind - 1]);
      return EXIT_FAILURE;
      break;
    default:
      abort();
    }
  }

  // Access non-option arguments
  if (optind < argc) {
    if (access(argv[optind], F_OK) == 0) {
      rootfs_dir = realpath(argv[optind], NULL);
      optind = optind + 1;
    } else {
      fprintf(stderr, "%s: Rootfs '%s' not exists.\n", program, argv[optind]);
      return EXIT_FAILURE;
    }

    if (strcmp("--", argv[1]) != 0 && optind < argc) {
      printf("%s: Excessive argument '%s'.\n", program, argv[optind]);
      return EXIT_FAILURE;
    }
  } else {
    ROOTFS_NOT_SET
    return EXIT_FAILURE;
  }

  char **proot_envp = strlist_new();
  char **proot_argv = strlist_new();
  char **fakerootfs_files = strlist_new();
  set_proot_env(&proot_envp);

  set_proot_path(&proot_argv);
  strlist_addl(&proot_argv, my_asprintf("--rootfs=%s", rootfs_dir), NULL);
  if (options.qemu != NULL) {
    strlist_addl(&proot_argv, my_asprintf("--qemu=%s", options.qemu), NULL);
  }
  strlist_addl(&proot_argv, my_asprintf("--cwd=%s", options.cwd), NULL);
  if (options.kill_on_exit) {
    strlist_addl(&proot_argv, "--kill-on-exit", NULL);
  }
  if (options.kernel_release != NULL) {
    strlist_addl(&proot_argv,
                 my_asprintf("--kernel-release=%s", options.kernel_release),
                 NULL);
  }
  strlist_addl(&proot_argv, "--root-id", NULL);
  if (options.link2symlink) {
    strlist_addl(&proot_argv, "--link2symlink", NULL);
  }
  if (is_android()) {
    if (options.sysvipc) {
      strlist_addl(&proot_argv, "--sysvipc", NULL);
    }
    strlist_addl(&proot_argv, "--ashmem-memfd", NULL);
    strlist_addl(&proot_argv, "-H", NULL);
    if (options.fix_low_ports) {
      strlist_addl(&proot_argv, "-P", NULL);
    }
    strlist_addl(&proot_argv, "-L", NULL);
  }

  strlist_addl(&proot_argv, "--bind=/dev", NULL);
  strlist_addl(&proot_argv, "--bind=/dev/urandom:/dev/random", NULL);
  strlist_addl(&proot_argv, "--bind=/proc", NULL);
  strlist_addl(&proot_argv, "--bind=/proc/self/fd:/dev/fd", NULL);
  if (isatty(STDIN_FILENO)) {
    strlist_addl(&proot_argv, "--bind=/proc/self/fd/0:/dev/stdin", NULL);
  }
  if (isatty(STDOUT_FILENO)) {
    strlist_addl(&proot_argv, "--bind=/proc/self/fd/1:/dev/stdout", NULL);
  }
  if (isatty(STDERR_FILENO)) {
    strlist_addl(&proot_argv, "--bind=/proc/self/fd/2:/dev/stderr", NULL);
  }
  strlist_addl(&proot_argv, "--bind=/sys", NULL);

  // Bind fakerootfs files
  char *fakerootfs_dir = my_asprintf(FMT_PROOT_FAKEROOTFS_DIR, rootfs_dir);
  int prefix_len = strlen(fakerootfs_dir);

  filelist(&fakerootfs_files, fakerootfs_dir);

  for (int i = 0; i < strlist_len(fakerootfs_files); i++) {
    char *path_short = strdup(fakerootfs_files[i] + prefix_len);

    char *token = NULL;
    token = strtok(strdup(path_short), "/");

    if (strcmp("proc", token) == 0) {
      if (access(path_short, R_OK) != 0) {
        strlist_addl(
            &proot_argv,
            my_asprintf("--bind=%s:%s", fakerootfs_files[i], path_short), NULL);
      }
    } else if (strcmp(basename(path_short), HOT_UTILS) == 0) {
      if (options.host_utils) {
        strlist_addl(
            &proot_argv,
            my_asprintf("--bind=%s:%s", fakerootfs_files[i], path_short), NULL);
      }
    } else {
      strlist_addl(&proot_argv,
                   my_asprintf("--bind=%s:%s", fakerootfs_files[i], path_short),
                   NULL);
    }
  }

  strlist_addl(&proot_argv, my_asprintf("--bind=%s/tmp:/dev/shm", rootfs_dir),
               NULL);
  if (options.host_utils) {
    if (is_android()) {
      if (access("/system", F_OK) == 0) {
        strlist_addl(&proot_argv, "--bind=/system", NULL);
      }
      if (access("/apex", F_OK) == 0) {
        strlist_addl(&proot_argv, "--bind=/apex", NULL);
      }
      if (access("/linkerconfig/ld.config.txt", F_OK) == 0) {
        strlist_addl(&proot_argv, "--bind=/linkerconfig/ld.config.txt", NULL);
      }

      if (is_anotherterm()) {
        putenv(my_asprintf("TERMSH_UID=%d", getuid()));
        strlist_addl(&proot_argv,
                     my_asprintf("--bind=%s/libtermsh.so", getenv("LIB_DIR")),
                     NULL);
      }

      if (access("/vendor", F_OK)) {
        strlist_addl(&proot_argv, "--bind=/vendor", NULL);
        strlist_addl(&proot_argv, "--bind=/data/app", NULL);
      }
      if (is_anotherterm()) {
        strlist_addl(&proot_argv, my_asprintf("--bind=%s", getenv("DATA_DIR")),
                     NULL);
      }

      if (is_termux()) {
        strlist_addl(&proot_argv, "--bind=/data/dalvik-cache", NULL);
        if (access("/data/data/com.termux/files/apps", F_OK) == 0) {
          strlist_addl(&proot_argv, "--bind=/data/data/com.termux/files/apps",
                       NULL);
        }
        strlist_addl(&proot_argv, my_asprintf("--bind=%s", getenv("PREFIX")),
                     NULL);
      }
    }
  }

  if (options.bindings != NULL) {
    for (int i = 0; i < strlist_len(options.bindings); i++) {
      strlist_addl(&proot_argv, my_asprintf("--bind=%s", options.bindings[i]),
                   NULL);
    }
  }

  struct stat fileStat;

  if (lstat(my_asprintf("%s/usr/bin/env", rootfs_dir), &fileStat) == 0) {
    strlist_addl(
        &proot_argv, "/usr/bin/env", "-i", "HOME=/root", "LANG=C.UTF-8",
        my_asprintf("TERM=%s", getenv("TERM") != NULL ? getenv("TERM")
                                                      : "TERM=xterm-256color"),
        NULL);
    if (getenv("COLORTERM") != NULL) {
      strlist_addl(&proot_argv,
                   my_asprintf("COLORTERM=%s", getenv("COLORTERM")), NULL);
    }

    if (options.host_utils && is_anotherterm()) {
      strlist_addl(
          &proot_argv,
          my_asprintf("SHELL_SESSION_TOKEN=%s", getenv("SHELL_SESSION_TOKEN")),
          NULL);
    }

    if (options.env != NULL) {
      for (int i = 0; i < strlist_len(options.env); i++) {
        strlist_addl(&proot_argv, options.env[i], NULL);
      }
    }
  }

  if (optind < argc) {
    if (is_verbose) {
      list_argv(argc - optind, argv + optind, "forwarded_argv");
    }
    for (int i = optind; i < argc; i++) {
      strlist_addl(&proot_argv, argv[i], NULL);
    }
  } else {
    if (access(my_asprintf("%s/etc/passwd", rootfs_dir), F_OK) == 0) {
      strlist_addl(
          &proot_argv,
          getpw(my_asprintf("%s/etc/passwd", rootfs_dir), 0, 0)->pw_shell, "-l",
          NULL);
    } else {
      strlist_addl(&proot_argv, "/bin/sh", "-l", NULL);
    }
  }

  if (is_verbose) {
    strlist_list(proot_envp, "proot_env");
    strlist_list(proot_argv, "proot_argv");
  }
  execve(proot_argv[0], proot_argv, proot_envp);

  return 0;
}

int command_archive(int argc, char *argv[]) {
  help_info = my_asprintf("\
Archive the specific rootfs to stdout.\n\
\n\
Usage:\n\
  %s %s [ROOTFS]\n\
\n\
Options:\n\
  --help              show this help\n\
\n\
Tar relavent options:\n\
  -v, --verbose\n\
  --exclude\n\
",
                          program, command);
  SHOW_HELP

  static struct {
    int tar_is_verbose;
    char **tar_excludes;
  } options;

  options.tar_is_verbose = 0;
  options.tar_excludes = NULL;

  int c;
  int longopt_index = 0;

  static struct option long_options[] = {
      {"help", no_argument, NULL, 'h'},
      {"verbose", no_argument, NULL, 'v'},
      {"exclude", required_argument, NULL, 0},
      {NULL, 0, NULL, 0}};

  while ((c = getopt_long(argc, argv, "hv", long_options, &longopt_index)) !=
         -1) {
    switch (c) {
    case 0:
      if (strcmp("exclude", long_options[longopt_index].name) == 0) {
        if (options.tar_excludes == NULL) {
          options.tar_excludes = strlist_new();
        }
        strlist_addl(&options.tar_excludes, optarg, NULL);
      }
      break;
    case 'h':
      fputs(help_info, stderr);
      return EXIT_SUCCESS;
    case 'v':
      options.tar_is_verbose = 1;
      break;
    case '?':
      fprintf(stderr, "%s: Unknown option '%s'.\n", program, argv[optind - 1]);
      return EXIT_FAILURE;
      break;
    default:
      abort();
    }
  }

  if (optind < argc) {
    if (access(argv[optind], F_OK) == 0) {
      rootfs_dir = realpath(argv[optind], NULL);
      optind = optind + 1;
    } else {
      fprintf(stderr, "%s: Rootfs '%s' not exists.\n", program, argv[optind]);
      return EXIT_FAILURE;
    }
  } else {
    ROOTFS_NOT_SET
    return EXIT_FAILURE;
  }

  if (isatty(STDOUT_FILENO)) {
    fprintf(stderr, "%s: Refusing to write archive contents to terminal\n",
            program);
    exit(EXIT_FAILURE);
  }

  char **proot_argv = strlist_new();
  char **proot_envp = strlist_new();

  set_proot_env(&proot_envp);

  set_proot_path(&proot_argv);
  strlist_addl(&proot_argv, my_asprintf("--rootfs=%s", rootfs_dir), "--root-id",
               "--cwd=/", "/bin/tar", NULL);

  if (options.tar_is_verbose) {
    strlist_addl(&proot_argv, "-v", NULL);
  }

  strlist_addl(&proot_argv, "--exclude=./tmp/*", NULL);
  strlist_addl(&proot_argv, "--exclude=.*sh_history", NULL);
  strlist_addl(&proot_argv, my_asprintf("--exclude=" FMT_PROOT_DATA_DIR, "."),
               NULL);

  if (options.tar_excludes != NULL && strlist_len(options.tar_excludes) > 0) {
    for (int i = 0; i < strlist_len(options.tar_excludes); i++) {
      strlist_addl(&proot_argv,
                   my_asprintf("--exclude=%s", options.tar_excludes[i]), NULL);
    }
  }

  strlist_addl(&proot_argv, "-c", ".", NULL);

  if (is_verbose) {
    strlist_list(proot_argv, "proot_argv");
  }
  execve(proot_argv[0], proot_argv, proot_envp);

  return 0;
}

int main(int argc, char *argv[]) {
  is_verbose = 0;
  program = basename(argv[0]);

  // Disable automatic error messages from getopt_long
  // extern int opterr;
  opterr = 0;
  link2symlink_default = 0;
  if (is_android()) {
    link2symlink_default = 1;
  }

  help_info = my_asprintf("\
Supercharges your PRoot experience.\n\
\n\
Usage:\n\
  %s [OPTION...] [COMMAND]\n\
\n\
  ## show help of a command\n\
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
Environment variables:\n\
  PROOT               path to proot\n\
",
                          program, program, program);
  SHOW_HELP

  for (int i = 1; i < argc; i++) {
    switch (argv[i][0]) {
    case '-':
      if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
        is_verbose = 1;
      } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
        fputs(help_info, stderr);
      } else {
        fprintf(stderr, "%s: Unknown option '%s'.\n", program, argv[i]);
        return EXIT_FAILURE;
      }
      break;
    default:
      command = argv[i];
      int forwarded_argc = argc - i;
      char **forwarded_argv = argv + i;

      if (strcmp("install", command) == 0) {
        return command_install(forwarded_argc, forwarded_argv);
      } else if (strcmp("login", command) == 0) {
        return command_login(forwarded_argc, forwarded_argv);
      } else if (strcmp("archive", command) == 0) {
        return command_archive(forwarded_argc, forwarded_argv);
      } else {
        fprintf(stderr, "%s: Unknown command '%s'.\n", program, command);
        return EXIT_FAILURE;
      }
      return 0;
    }
  }

  return 0;
}
