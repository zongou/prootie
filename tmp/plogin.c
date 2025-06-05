/*
prootie login shortcut
 */

#include <libgen.h>
#include <linux/limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

char *program_name;

void error_msg(const char *format, ...) {
    va_list args;
    va_start(args, format);
    fprintf(stderr, "%s error: ", program_name);
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

int main(int argc, char *argv[]) {
    program_name        = basename(argv[0]);
    char  *prootie_path = NULL;
    char **prootie_argv = NULL;
    char  *rootfs       = NULL;
    char   cwd[PATH_MAX];

    getcwd(cwd, sizeof(cwd));

    if (argc == 1) {
        fprintf(stderr, "%s: Usage %s [ROOTFS]\n", program_name, program_name);
        return EXIT_FAILURE;
    }

    prootie_path = get_program_path("prootie");
    prootie_argv = strings_new();

    strings_add(&prootie_argv, prootie_path, "login", rootfs, NULL);
    if (getenv("PREFIX") != NULL) {
        strings_add(&prootie_argv,
                    a_printf("--bind=%s", dirname(getenv("PREFIX"))), NULL);
    } else {
        strings_add(&prootie_argv, a_printf("--bind=%s", cwd), NULL);
    }
    if (access("/storage/emulated/0", R_OK) == 0) {
        strings_add(&prootie_argv, "--bind=/storage/emulated/0", NULL);
        strings_add(&prootie_argv, "--bind=/storage/emulated/0:/sdcard", NULL);
    }
    if (getenv("TMPDIR") != NULL) {
        strings_add(&prootie_argv, a_printf("--bind=%s:/tmp", getenv("TMPDIR")),
                    NULL);
    }
    strings_add(&prootie_argv, a_printf("--env=HOME=%s", getenv("HOME")),
                NULL);
    strings_add(
        &prootie_argv,
        "--env=PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin",
        NULL);
    strings_add(&prootie_argv, a_printf("--cwd=%s", cwd), NULL);
    // strlist_addl(&prootie_argv, "--host-utils", NULL);

    if (argc > 0) {
        strings_add(&prootie_argv, "--", NULL);
        for (int i = 1; i < argc; i++) {
            strings_add(&prootie_argv, argv[i], NULL);
        }
    }

    // strlist_list(prootie_argv, "plogin");
    execv(prootie_argv[0], prootie_argv);
    return 0;
}
