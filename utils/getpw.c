// https://github.com/gportay/iamroot/blob/master/passwd.c

/*
 * Copyright 2021-2023 Gaël PORTAY
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#if defined __linux__ || defined __OpenBSD__ || defined __NetBSD__
/*
 * Stolen from musl (src/include/features.h)
 *
 * SPDX-FileCopyrightText: The musl Contributors
 *
 * SPDX-License-Identifier: MIT
 */
#define hidden __attribute__((__visibility__("hidden")))

/*
 * Stolen from musl (src/passwd/pwf.h)
 *
 * SPDX-FileCopyrightText: The musl Contributors
 *
 * SPDX-License-Identifier: MIT
 */
#include <errno.h>
#include <grp.h>
#include <limits.h>
#include <pwd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *passwd_path;

// Optimized number parsing
static inline unsigned atou(char **s) {
    unsigned x = 0;
    while (**s >= '0' && **s <= '9') {
        x = (x * 10) + (unsigned)((**s) - '0');
        ++*s;
    }
    return x;
}

hidden int __getpwent_a(FILE *f, struct passwd *pw, char **line, size_t *size, struct passwd **res) {
    ssize_t l;
    char *s;
    int rv = 0;

    *res = NULL;
    
    while ((l = getline(line, size, f)) > 0) {
        (*line)[l - 1] = 0;
        s = *line;

        // Fast path for malformed lines
        if (!strchr(s, ':')) continue;

        pw->pw_name = s++;
        if (!(s = strchr(s, ':'))) continue;
        *s++ = 0;
        
        pw->pw_passwd = s;
        if (!(s = strchr(s, ':'))) continue;
        *s++ = 0;

        pw->pw_uid = atou(&s);
        if (*s != ':') continue;
        *s++ = 0;

        pw->pw_gid = atou(&s);
        if (*s != ':') continue;
        *s++ = 0;

        pw->pw_gecos = s;
        if (!(s = strchr(s, ':'))) continue;
        *s++ = 0;

        pw->pw_dir = s;
        if (!(s = strchr(s, ':'))) continue;
        *s++ = 0;

        pw->pw_shell = s;
        *res = pw;
        break;
    }

    if (l < 0 && ferror(f)) {
        rv = errno;
    }

    if (!*res) {
        free(*line);
        *line = NULL;
    }

    return rv;
}

hidden int __getpw_a(const char *name, uid_t uid, struct passwd *pw, char **buf, size_t *size, struct passwd **res) {
    FILE *f;
    int rv = 0;

    *res = NULL;

    if (!passwd_path) {
        errno = EINVAL;
        return EINVAL;
    }

    if (!(f = fopen(passwd_path, "re"))) {
        return errno;
    }

    while (!(rv = __getpwent_a(f, pw, buf, size, res)) && *res) {
        if ((name && !strcmp(name, (*res)->pw_name)) ||
            (!name && (*res)->pw_uid == uid)) {
            break;
        }
    }

    fclose(f);
    return rv;
}

struct passwd *getpw(char *passwd_file_path, const char *name, uid_t uid) {
    static char *line;
    static struct passwd pw;
    static size_t size;
    struct passwd *res;

    passwd_path = passwd_file_path;
    if (__getpw_a(name, uid, &pw, &line, &size, &res)) {
        return NULL;
    }
    return res;
}

#endif

