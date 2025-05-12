#include <dirent.h>
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

#define FMT_PROOT_DATA_DIR "%s/.proot"
#define FMT_PROOT_L2S_DIR  "%s/.proot/l2s"
#define FMT_PROOT_BIND_DIR "%s/.proot/bind"
#define HOT_UTILS          "host_utils.sh"

struct config {
    char *program;

    // Flags
    int help;
    int verbose;

    char *help_message;
    char *rootfs_path;
    char *command;

    int    tar_verbose;
    char **tar_exclude_list;
} config;

void info_msg(const char *format, ...) {
    if (!config.verbose) return;

    va_list args;
    va_start(args, format);
    fprintf(stderr, "%s info: ", config.program);
    vfprintf(stderr, format, args);
    va_end(args);
}

void error_msg(const char *format, ...) {
    va_list args;
    va_start(args, format);
    fprintf(stderr, "%s error: ", config.program);
    vfprintf(stderr, format, args);
    va_end(args);
}

void warning_msg(const char *format, ...) {
    va_list args;
    va_start(args, format);
    fprintf(stderr, "%s warning: ", config.program);
    vfprintf(stderr, format, args);
    va_end(args);
}

void *safe_malloc(size_t size) {
    void *ptr = malloc(size);
    if (!ptr) {
        error_msg("Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    return ptr;
}

char *a_printf(const char *format, ...) {
    va_list args;
    va_start(args, format);

    va_list args_copy;
    va_copy(args_copy, args);
    int str_len = vsnprintf(NULL, 0, format, args_copy);
    va_end(args_copy);

    char *result = safe_malloc(str_len + 1);
    vsnprintf(result, str_len + 1, format, args);
    va_end(args);
    return result;
}

void mkdir_wrapper(const char *path) {
    if (mkdir(path, 0755) != 0) {
        error_msg("Cannot create directory %s\n", path);
        exit(EXIT_FAILURE);
    }
}

char *get_login_shell(const char *passwd_file, int query_userid) {
    FILE *fp = fopen(passwd_file, "r");
    if (!fp) {
        perror("Error opening file");
        return NULL;
    }

    char line[1024];

    while (fgets(line, sizeof(line), fp)) {
        char *username = strtok(line, ":");
        char *password = strtok(NULL, ":");
        char *uid      = strtok(NULL, ":");
        char *gid      = strtok(NULL, ":");
        char *gecos    = strtok(NULL, ":");
        char *home     = strtok(NULL, ":");
        char *shell    = strtok(NULL, ":");

        // Remove newline character from shell
        if (shell && shell[strlen(shell) - 1] == '\n') {
            shell[strlen(shell) - 1] = '\0';
        }

        if (query_userid == atoi(uid)) {
            return strdup(shell);
        }
    }

    fclose(fp);
    return NULL;
}

int is_android() { return access("/system/bin/linker", F_OK) == 0; }
int is_termux() { return getenv("TERMUX_VERSION") != NULL; }
int is_anotherterm() { return getenv("TERMSH") != NULL; }

// Optimize process trace checking with a single read
int is_traced() {
    char  tracer_pid[32];
    FILE *status_file = fopen("/proc/self/status", "r");
    if (!status_file) return 0;

    while (fgets(tracer_pid, sizeof(tracer_pid), status_file)) {
        if (!strncmp(tracer_pid, "TracerPid:", 10)) {
            fclose(status_file);
            return atoi(tracer_pid + 10) != 0;
        }
    }
    fclose(status_file);
    return 0;
}

char **strings_new() {
    char **list = safe_malloc(sizeof(char *));
    list[0]     = NULL; // Ensure NULL-termination
    return list;
}

void strings_print(int argc, char **argv, char *tittle) {
    for (int i = 0; i < argc; i++) {
        fprintf(stderr, "%s[%d]=%s\n", tittle, i, argv[i]);
    }
}

int strings_len(char **strlist) {
    int len = 0;
    if (strlist == NULL) {
        return 0;
    }
    while (1) {
        if (strlist[len] == NULL) {
            break;
        }
        len = len + 1;
    }
    return len;
}

int strings_add(char ***str_listp, ...) {
    va_list args;
    va_start(args, str_listp);

    int     len_appending = 0;
    va_list args_copy;
    va_copy(args_copy, args);
    while (va_arg(args_copy, char *) != NULL) {
        len_appending = len_appending + 1;
    }
    va_end(args_copy);

    int    len         = strings_len(*str_listp);
    char **strlist_tmp = realloc(*str_listp, sizeof(char *) * (len + len_appending + 1));
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

/* As shell `command -v` */
char *get_program_path(const char *program_name) {
    const char *path_env = getenv("PATH");
    if (!path_env)
        return NULL;

    char *path_copy = strdup(path_env);
    char *token     = strtok(path_copy, ":");
    while (token != NULL) {
        char path[PATH_MAX];
        snprintf(path, PATH_MAX, "%s/%s", token, program_name);
        if (access(path, X_OK) == 0) {
            free(path_copy);
            return strdup(path);
        }
        token = strtok(NULL, ":");
    }

    free(path_copy);
    return NULL;
}

void get_file_list(char ***strlist, const char *dir_path) {
    DIR           *dir;
    struct dirent *entry;

    if ((dir = opendir(dir_path)) != NULL) {
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                char new_path[PATH_MAX];
                snprintf(new_path, sizeof(new_path), "%s/%s", dir_path, entry->d_name);

                // Recursively call list_files if it's a directory
                if (entry->d_type == DT_DIR) {
                    get_file_list(strlist, new_path);
                } else {
                    strings_add(strlist, strdup(new_path), NULL);
                }
            }
        }
        closedir(dir);
    } else {
        perror(dir_path);
    }
}

char **new_proot_argv() {
    char **proot_argv = strings_new();
    char  *proot_path = NULL;
    if ((proot_path = getenv("PROOT")) != NULL) {
        strings_add(&proot_argv, proot_path, NULL);
    } else if ((proot_path = get_program_path("proot")) != NULL) {
        strings_add(&proot_argv, proot_path, NULL);
    } else {
        error_msg("Cannot find proot.\n");
        exit(EXIT_FAILURE);
    }
    return proot_argv;
}

char **new_proot_envp() {
    char **proot_envp = strings_new();
    char   path_buffer[PATH_MAX];

    snprintf(path_buffer, sizeof(path_buffer), FMT_PROOT_L2S_DIR, config.rootfs_path);
    strings_add(&proot_envp, a_printf("PROOT_L2S_DIR=%s", path_buffer), NULL);

    char *tmp = getenv("PROOT_TMP_DIR");
    if (!tmp) tmp = getenv("TMPDIR");
    if (tmp) {
        strings_add(&proot_envp, a_printf("PROOT_TMP_DIR=%s", tmp), NULL);
    }
    return proot_envp;
}

void show_help() {
    fprintf(stderr, "%s", config.help_message);
}

int command_install(int argc, char *argv[]) {
    config.help_message = a_printf("\
Usage: %s %s [OPTION]... [ROOTFS]\n\
Extract and setup the tar-format rootfs archive from stdin.\n\
\n\
Options:\n\
  --help              Show this help.\n\
\n\
Tar-relavent options:\n\
  -v, --verbose\n\
  --exclude\n\
",
                                   config.program, config.command);

    int option_index = 0;
    int c;

    static struct option long_options[] = {
        {"help", no_argument, NULL, 'h'},
        {"verbose", no_argument, NULL, 'v'},
        {"exclude", required_argument, NULL, 0},
        {NULL, 0, NULL, 0}};

    while ((c = getopt_long(argc, argv, "hv", long_options, &option_index)) !=
           -1) {
        switch (c) {
            case 0:
                if (strcmp("exclude", long_options[option_index].name) == 0) {
                    if (config.tar_exclude_list == NULL) {
                        config.tar_exclude_list = strings_new();
                    }
                    strings_add(&config.tar_exclude_list, optarg, NULL);
                }
                break;
            case 'h':
                show_help();
                return EXIT_SUCCESS;
            case 'v':
                config.tar_verbose = 1;
                break;
            case '?':
                error_msg("Unknown option '%s'.\n", argv[optind - 1]);
                return EXIT_FAILURE;
                break;
            default:
                abort();
        }
    }

    if (optind < argc) {
        if (access(argv[optind], F_OK) != 0) {
            mkdir_wrapper(argv[optind]);
            config.rootfs_path = realpath(argv[optind], NULL);
            optind             = optind + 1;
        } else {
            error_msg("Rootfs '%s' already exists.\n", argv[optind]);
            return EXIT_FAILURE;
        }
    } else {
        show_help();
        return EXIT_FAILURE;
    }

    char **proot_envp = new_proot_envp();
    char **proot_argv = new_proot_argv();

    strings_add(&proot_argv, "--link2symlink", "--root-id", NULL);

    char *tar_path = NULL;
    if ((tar_path = get_program_path("tar")) != NULL) {
        strings_add(&proot_argv, tar_path, NULL);
    } else {
        error_msg("Cannot find tar\n");
        return EXIT_FAILURE;
    }

    if (config.tar_verbose) {
        strings_add(&proot_argv, "-v", NULL);
    }

    strings_add(&proot_argv, "--exclude=dev", "--exclude=./dev", NULL);
    strings_add(&proot_argv, "-C", config.rootfs_path, "-x", NULL);
    if (strings_len(config.tar_exclude_list) > 0) {
        for (int i = 0; i < strings_len(config.tar_exclude_list); i++) {
            strings_add(&proot_argv, a_printf("--exclude=%s", config.tar_exclude_list[i]), NULL);
        }
    }

    pid_t pid;
    pid = fork();
    if (pid < 0) {
        perror("fork");
        return EXIT_FAILURE;
    } else if (pid == 0) {
        if (config.verbose) {
            strings_print(strings_len(proot_envp), proot_envp, "proot_envp");
            strings_print(strings_len(proot_argv), proot_argv, "proot_argv");
        }
        // close(STDERR_FILENO);
        mkdir_wrapper(a_printf(FMT_PROOT_DATA_DIR, config.rootfs_path));
        mkdir_wrapper(a_printf(FMT_PROOT_L2S_DIR, config.rootfs_path));
        execve(proot_argv[0], proot_argv, proot_envp);
        perror("execve");
    } else {
        int status;
        // signal(SIGINT, sigint_handler);
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            if (WEXITSTATUS(status) != 0) {
                error_msg("Child exited with status: %d\n", status);
                error_msg("Failed.\n");
                error_msg("Cleaning up...\n");
                system(a_printf("chmod +rw -R %s", config.rootfs_path));
                system(a_printf("rm -rf %s", config.rootfs_path));
                return EXIT_FAILURE;
            }
        } else {
            error_msg("Child did not terminate normally.\n");
            return EXIT_FAILURE;
        }
    }

    mkdir_wrapper(a_printf(FMT_PROOT_BIND_DIR, config.rootfs_path));
    mkdir_wrapper(a_printf(FMT_PROOT_BIND_DIR "/proc", config.rootfs_path));

    fputs("0.00 0.00 0.00 0/0 0\n",
          fopen(a_printf(FMT_PROOT_BIND_DIR "/proc/loadavg", config.rootfs_path),
                "w"));
    fputs("cpu  0 0 0 0 0 0 0 0 0 0\n",
          fopen(a_printf(FMT_PROOT_BIND_DIR "/proc/stat", config.rootfs_path),
                "w"));
    fputs("0.00 0.00\n",
          fopen(a_printf(FMT_PROOT_BIND_DIR "/proc/uptime", config.rootfs_path),
                "w"));
    fputs("\
Linux localhost 6.1.0-22 #1 SMP PREEMPT_DYNAMIC 6.1.94-1 (2024-06-21) GNU/Linux\n\
",
          fopen(a_printf(FMT_PROOT_BIND_DIR "/proc/version", config.rootfs_path),
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
          fopen(a_printf(FMT_PROOT_BIND_DIR "/proc/vmstat", config.rootfs_path),
                "w"));

    mkdir_wrapper(a_printf(FMT_PROOT_BIND_DIR "/proc/sys", config.rootfs_path));
    mkdir_wrapper(
        a_printf(FMT_PROOT_BIND_DIR "/proc/sys/kernel", config.rootfs_path));
    fputs("40\n", fopen(a_printf(FMT_PROOT_BIND_DIR
                                 "/proc/sys/kernel/cap_last_cap",
                                 config.rootfs_path),
                        "w"));

    mkdir_wrapper(
        a_printf(FMT_PROOT_BIND_DIR "/proc/sys/fs", config.rootfs_path));
    mkdir_wrapper(
        a_printf(FMT_PROOT_BIND_DIR "/proc/sys/fs/inotify", config.rootfs_path));
    fputs("16384\n", fopen(a_printf(FMT_PROOT_BIND_DIR
                                    "/proc/sys/fs/inotify/max_queued_events",
                                    config.rootfs_path),
                           "w"));
    fputs("128\n", fopen(a_printf(FMT_PROOT_BIND_DIR
                                  "/proc/sys/fs/inotify/max_user_instances",
                                  config.rootfs_path),
                         "w"));
    fputs("65536\n", fopen(a_printf(FMT_PROOT_BIND_DIR
                                    "/proc/sys/fs/inotify/max_user_watches",
                                    config.rootfs_path),
                           "w"));

    mkdir_wrapper(a_printf(FMT_PROOT_BIND_DIR "/etc", config.rootfs_path));
    fputs("\
nameserver 8.8.8.8\n\
nameserver 8.8.4.4\n\
",
          fopen(a_printf(FMT_PROOT_BIND_DIR "/etc/resolv.conf",
                         config.rootfs_path),
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
          fopen(a_printf(FMT_PROOT_BIND_DIR "/etc/hosts", config.rootfs_path),
                "w"));

    mkdir_wrapper(
        a_printf(FMT_PROOT_BIND_DIR "/etc/profile.d", config.rootfs_path));
    fputs("\
export CHARSET=${CHARSET:-UTF-8}\n\
export LANG=${LANG:-C.UTF-8}\n\
export LC_COLLATE=${LC_COLLATE:-C}\n\
",
          fopen(a_printf(FMT_PROOT_BIND_DIR "/etc/profile.d/locale.sh",
                         config.rootfs_path),
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
          fopen(a_printf(FMT_PROOT_BIND_DIR "/etc/profile.d/proot.sh",
                         config.rootfs_path),
                "w"));

    if (is_android()) {
        FILE *fp =
            fopen(a_printf(FMT_PROOT_BIND_DIR "/etc/profile.d/" HOT_UTILS,
                           config.rootfs_path),
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
    config.help_message = a_printf("\
Usage: %s %s [OPTION]... [ROOTFS] [-- COMMAND]\n\
Start login shell or execute a command.\n\
\n\
Examples:\n\
  # Start login shell as root\n\
  %s %s [OPTION]... [ROOTFS]\n\
  \n\
  # Start login shell as user\n\
  %s %s [OPTION]... [ROOTFS] -- su -l [USER]\n\
\n\
Options:\n\
  -h, --help          Show this help.\n\
  --host-utils        Enable host utilities.\n\
  --env VAR=VALUE     Set environment variables.\n\
\n\
PRoot-relavent options:\n\
  -b, --bind, -m, --mount\n\
  -w, --cwd\n\
  --no-kill-on-exit\n\
  --link2symlink\n\
  --no-link2symlink\n\
  --no-sysvipc   \n\
  --fix-low-ports  \n\
  -q, --qemu\n\
  -k, --kernel-release\n\
",
                                   config.program, config.command, config.program, config.command, config.program, config.command);

    static struct {
        char  *cwd;
        int    kill_on_exit;
        int    link2symlink;
        int    fix_low_ports;
        char  *kernel_release;
        char  *qemu;
        int    sysvipc;
        char **bindings;
        int    host_utils;
        char **env;
    } options;

    options.kill_on_exit  = 1;
    options.bindings      = NULL;
    options.link2symlink  = is_android();
    options.fix_low_ports = 0;
    options.cwd           = "/root";
    options.sysvipc       = 1;
    options.host_utils    = 0;

    int option_index = 0;
    int c;

    static struct option long_options[] = {
        {"help", no_argument, NULL, 'h'},
        {"no-kill-on-exit", no_argument, &options.kill_on_exit, 0},
        {"link2symlink", no_argument, &options.link2symlink, 1},
        {"no-link2symlink", no_argument, &options.link2symlink, 0},
        {"fix-low-ports", no_argument, &options.fix_low_ports, 1},
        {"bind", required_argument, NULL, 'b'},
        {"mount", required_argument, NULL, 'm'},
        {"cwd", required_argument, NULL, 'w'},
        {"pwd", required_argument, NULL, 'w'},
        {"kernel-release", required_argument, NULL, 'k'},
        {"qemu", required_argument, NULL, 'q'},
        {"host-utils", no_argument, &options.host_utils, 1},
        {"env", required_argument, NULL, 0},
        {NULL, 0, NULL, 0}};

    while ((c = getopt_long(argc, argv, "hb:m:w:k:q:", long_options,
                            &option_index)) != -1) {
        switch (c) {
            case 0:
                /* If this option set a flag, do nothing else now. */
                // if (long_options[option_index].flag == 0) {
                //   printf("long_option[%d]:%s=%s", option_index,
                //          long_options[option_index].name, optarg);
                // }
                if (strcmp("env", long_options[option_index].name) == 0) {
                    if (options.env == NULL) {
                        options.env = strings_new();
                    }
                    strings_add(&options.env, optarg, NULL);
                }
                break;
            case 'h':
                show_help();
                return EXIT_SUCCESS;
                break;
            case 'b':
            case 'm':
                if (options.bindings == NULL) {
                    options.bindings = strings_new();
                }
                strings_add(&options.bindings, optarg, NULL);
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
                error_msg("Unknown option '%s'.\n", argv[optind - 1]);
                return EXIT_FAILURE;
                break;
            default:
                abort();
        }
    }

    // Access non-option arguments
    if (optind < argc) {
        if (access(argv[optind], F_OK) == 0) {
            config.rootfs_path = realpath(argv[optind], NULL);
            optind             = optind + 1;
        } else {
            error_msg("Rootfs '%s' not exists.\n", argv[optind]);
            return EXIT_FAILURE;
        }

        if (strcmp("--", argv[optind - 2]) != 0 && optind < argc) {
            error_msg("Excessive argument '%s'.\n", argv[optind]);
            return EXIT_FAILURE;
        }
    } else {
        show_help();
        return EXIT_FAILURE;
    }

    char **proot_envp = new_proot_envp();
    char **proot_argv = new_proot_argv();

    strings_add(&proot_argv, a_printf("--rootfs=%s", config.rootfs_path), NULL);
    if (options.qemu != NULL) {
        strings_add(&proot_argv, a_printf("--qemu=%s", options.qemu), NULL);
    }
    strings_add(&proot_argv, a_printf("--cwd=%s", options.cwd), NULL);
    if (options.kill_on_exit) {
        strings_add(&proot_argv, "--kill-on-exit", NULL);
    }
    if (options.kernel_release != NULL) {
        strings_add(&proot_argv,
                    a_printf("--kernel-release=%s", options.kernel_release),
                    NULL);
    }
    strings_add(&proot_argv, "--root-id", NULL);
    if (options.link2symlink) {
        strings_add(&proot_argv, "--link2symlink", NULL);
    }
    if (is_android()) {
        if (options.sysvipc) {
            strings_add(&proot_argv, "--sysvipc", NULL);
        }
        strings_add(&proot_argv, "--ashmem-memfd", NULL);
        strings_add(&proot_argv, "-H", NULL);
        if (options.fix_low_ports) {
            strings_add(&proot_argv, "-P", NULL);
        }
        strings_add(&proot_argv, "-L", NULL);
    }

    strings_add(&proot_argv, "--bind=/dev", NULL);
    strings_add(&proot_argv, "--bind=/dev/urandom:/dev/random", NULL);
    strings_add(&proot_argv, "--bind=/proc", NULL);
    strings_add(&proot_argv, "--bind=/proc/self/fd:/dev/fd", NULL);
    if (isatty(STDIN_FILENO)) {
        strings_add(&proot_argv, "--bind=/proc/self/fd/0:/dev/stdin", NULL);
    }
    if (isatty(STDOUT_FILENO)) {
        strings_add(&proot_argv, "--bind=/proc/self/fd/1:/dev/stdout", NULL);
    }
    if (isatty(STDERR_FILENO)) {
        strings_add(&proot_argv, "--bind=/proc/self/fd/2:/dev/stderr", NULL);
    }
    strings_add(&proot_argv, "--bind=/sys", NULL);

    // Bind fakerootfs files
    char **fakerootfs_files = strings_new();
    char  *fakerootfs_dir   = a_printf(FMT_PROOT_BIND_DIR, config.rootfs_path);
    int    prefix_len       = strlen(fakerootfs_dir);

    get_file_list(&fakerootfs_files, fakerootfs_dir);

    for (int i = 0; i < strings_len(fakerootfs_files); i++) {
        char *path_short = strdup(fakerootfs_files[i] + prefix_len);

        char *token = NULL;
        token       = strtok(strdup(path_short), "/");

        if (strcmp("proc", token) == 0) {
            if (access(path_short, R_OK) != 0) {
                strings_add(
                    &proot_argv,
                    a_printf("--bind=%s:%s", fakerootfs_files[i], path_short), NULL);
            }
        } else if (strcmp(basename(path_short), HOT_UTILS) == 0) {
            if (options.host_utils) {
                strings_add(
                    &proot_argv,
                    a_printf("--bind=%s:%s", fakerootfs_files[i], path_short), NULL);
            }
        } else {
            strings_add(&proot_argv,
                        a_printf("--bind=%s:%s", fakerootfs_files[i], path_short),
                        NULL);
        }
    }

    strings_add(&proot_argv, a_printf("--bind=%s/tmp:/dev/shm", config.rootfs_path),
                NULL);
    if (options.host_utils) {
        if (is_android()) {
            if (access("/system", F_OK) == 0) {
                strings_add(&proot_argv, "--bind=/system", NULL);
            }
            if (access("/apex", F_OK) == 0) {
                strings_add(&proot_argv, "--bind=/apex", NULL);
            }
            if (access("/linkerconfig/ld.config.txt", F_OK) == 0) {
                strings_add(&proot_argv, "--bind=/linkerconfig/ld.config.txt", NULL);
            }

            if (is_anotherterm()) {
                putenv(a_printf("TERMSH_UID=%d", getuid()));
                strings_add(&proot_argv,
                            a_printf("--bind=%s/libtermsh.so", getenv("LIB_DIR")),
                            NULL);
            }

            if (access("/vendor", F_OK)) {
                strings_add(&proot_argv, "--bind=/vendor", NULL);
                strings_add(&proot_argv, "--bind=/data/app", NULL);
            }
            if (is_anotherterm()) {
                strings_add(&proot_argv, a_printf("--bind=%s", getenv("DATA_DIR")),
                            NULL);
            }

            if (is_termux()) {
                strings_add(&proot_argv, "--bind=/data/dalvik-cache", NULL);
                if (access("/data/data/com.termux/files/apps", F_OK) == 0) {
                    strings_add(&proot_argv, "--bind=/data/data/com.termux/files/apps",
                                NULL);
                }
                strings_add(&proot_argv, a_printf("--bind=%s", getenv("PREFIX")),
                            NULL);
            }
        }
    }

    if (options.bindings != NULL) {
        for (int i = 0; i < strings_len(options.bindings); i++) {
            strings_add(&proot_argv, a_printf("--bind=%s", options.bindings[i]),
                        NULL);
        }
    }

    struct stat fileStat;

    if (lstat(a_printf("%s/usr/bin/env", config.rootfs_path), &fileStat) == 0) {
        strings_add(
            &proot_argv, "/usr/bin/env", "-i", "HOME=/root", "LANG=C.UTF-8",
            a_printf("TERM=%s", getenv("TERM") != NULL ? getenv("TERM")
                                                       : "TERM=xterm-256color"),
            NULL);
        if (getenv("COLORTERM") != NULL) {
            strings_add(&proot_argv,
                        a_printf("COLORTERM=%s", getenv("COLORTERM")), NULL);
        }

        if (options.host_utils && is_anotherterm()) {
            strings_add(
                &proot_argv,
                a_printf("SHELL_SESSION_TOKEN=%s", getenv("SHELL_SESSION_TOKEN")),
                NULL);
        }

        if (options.env != NULL) {
            for (int i = 0; i < strings_len(options.env); i++) {
                strings_add(&proot_argv, options.env[i], NULL);
            }
        }
    }

    if (optind < argc) {
        for (int i = optind; i < argc; i++) {
            strings_add(&proot_argv, argv[i], NULL);
        }
    } else {
        char *passwd_file = a_printf("%s/etc/passwd", config.rootfs_path);
        if (access(passwd_file, F_OK) == 0) {
            char *login_shell = get_login_shell(passwd_file, 0);
            if (login_shell) {
                strings_add(&proot_argv, login_shell, "-l", NULL);
            } else {
                strings_add(&proot_argv, "/bin/sh", "-l", NULL);
            }
        } else {
            strings_add(&proot_argv, "/bin/sh", "-l", NULL);
        }
    }

    if (config.verbose) {
        strings_print(strings_len(proot_envp), proot_envp, "proot_env");
        strings_print(strings_len(proot_argv), proot_argv, "proot_argv");
    }
    execve(proot_argv[0], proot_argv, proot_envp);

    return 0;
}

int command_archive(int argc, char *argv[]) {
    config.help_message = a_printf("\
Usage: %s %s [ROOTFS]\n\
Archive rootfs to stdout.\n\
\n\
Options:\n\
  --help              Show this help.\n\
\n\
Tar-relavent options:\n\
  -v, --verbose\n\
  --exclude\n\
",
                                   config.program, config.command);

    int option_index = 0;
    int c;

    static struct option long_options[] = {
        {"help", no_argument, NULL, 'h'},
        {"verbose", no_argument, NULL, 'v'},
        {"exclude", required_argument, NULL, 0},
        {NULL, 0, NULL, 0}};

    while ((c = getopt_long(argc, argv, "hv", long_options, &option_index)) !=
           -1) {
        switch (c) {
            case 0:
                if (strcmp("exclude", long_options[option_index].name) == 0) {
                    if (config.tar_exclude_list == NULL) {
                        config.tar_exclude_list = strings_new();
                    }
                    strings_add(&config.tar_exclude_list, optarg, NULL);
                }
                break;
            case 'h':
                show_help();
                return EXIT_SUCCESS;
            case 'v':
                config.tar_verbose = 1;
                break;
            case '?':
                error_msg("Unknown option '%s'.\n", argv[optind - 1]);
                return EXIT_FAILURE;
                break;
            default:
                abort();
        }
    }

    if (optind < argc) {
        if (access(argv[optind], F_OK) == 0) {
            mkdir_wrapper(argv[optind]);
            config.rootfs_path = realpath(argv[optind], NULL);
            optind             = optind + 1;
        } else {
            error_msg("Rootfs '%s' not exists.\n", argv[optind]);
            return EXIT_FAILURE;
        }
    } else {
        show_help();
        return EXIT_FAILURE;
    }

    if (isatty(STDOUT_FILENO)) {
        error_msg("Refusing to write archive contents to terminal\n");
        exit(EXIT_FAILURE);
    }

    char **proot_argv = new_proot_argv();
    char **proot_envp = new_proot_envp();

    strings_add(&proot_argv, a_printf("--rootfs=%s", config.rootfs_path), "--root-id", "--cwd=/", "/bin/tar", NULL);

    if (config.tar_verbose) {
        strings_add(&proot_argv, "-v", NULL);
    }

    strings_add(&proot_argv, "--exclude=./tmp/*", NULL);
    strings_add(&proot_argv, "--exclude=.*sh_history", NULL);
    strings_add(&proot_argv, a_printf("--exclude=" FMT_PROOT_DATA_DIR, "."),
                NULL);

    if (config.tar_exclude_list != NULL && strings_len(config.tar_exclude_list) > 0) {
        for (int i = 0; i < strings_len(config.tar_exclude_list); i++) {
            strings_add(&proot_argv, a_printf("--exclude=%s", config.tar_exclude_list[i]), NULL);
        }
    }

    strings_add(&proot_argv, "-c", ".", NULL);

    if (config.verbose) {
        strings_print(strings_len(proot_argv), proot_argv, "proot_argv");
    }
    execve(proot_argv[0], proot_argv, proot_envp);

    return 0;
}


int main(int argc, char **argv) {
    config.program      = basename(argv[0]);
    config.help_message = a_printf("\
Usage: %s [OPTION]... [COMMAND]\n\
Supercharge your PRoot experience.\n\
\n\
Options and enviroment variables:\n\
  -h, --help          Show this help.\n\
  -v, --verbose       Print more information.\n\
  -f, --force         Skip trace check.\n\
  PROOT               Path to proot.\n\
\n\
Commands:\n\
  install             Install rootfs.\n\
  login               Login rootfs.\n\
  archive             Archive rootfs.\n\
",
                                   config.program, config.program);

    // Parse options
    int arg_index = 1;
    while (arg_index < argc) {
        char *current_arg     = argv[arg_index];
        int   current_arg_len = strlen(current_arg);
        // printf("%s\n", current_arg);

        if (current_arg_len > 1 && current_arg[0] == '-') { // Is option
            if (current_arg[1] != '-') {                    // Is short options
                for (int short_opt_index = 1; short_opt_index < current_arg_len; short_opt_index++) {
                    char short_opt = current_arg[short_opt_index];
                    switch (short_opt) {
                        case 'v':
                            config.verbose = 1;
                            break;
                        case 'h':
                            config.help = 1;
                            break;
                        default:
                            error_msg("Unknown option: %c\n", short_opt);
                            return 1;
                    }
                }
            } else { // Is a long option
                if (strcmp(current_arg, "--verbose") == 0) {
                    config.verbose = 1;
                } else if (strcmp(current_arg, "--help") == 0) {
                    config.help = 1;
                } else {
                    error_msg("Unknown option: %s\n", current_arg);
                    return 1;
                }
            }
        } else { // Not an option
            break;
        }

        arg_index++;
    }

    if (config.help) {
        show_help();
        return 0;
    }

    if (arg_index < argc) {
        config.command = argv[arg_index];
        free(config.help_message);
        int    sub_argc = argc - arg_index;
        char **sub_argv = argv + arg_index;

        if (is_traced()) {
            warning_msg("Current process is being traced.\n");
        }

        if (strcmp(config.command, "install") == 0) {
            command_install(sub_argc, sub_argv);
        } else if (strcmp(config.command, "login") == 0) {
            command_login(sub_argc, sub_argv);
        } else if (strcmp(config.command, "archive") == 0) {
            command_archive(sub_argc, sub_argv);
        } else {
            error_msg("Unknown command: %s\n", config.command);
            return 1;
        }
    } else {
        show_help();
    }
    return 0;
}