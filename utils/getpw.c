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

hidden int __getpwent_a(FILE *f, struct passwd *pw, char **line, size_t *size,
                        struct passwd **res);
hidden int __getpw_a(const char *name, uid_t uid, struct passwd *pw, char **buf,
                     size_t *size, struct passwd **res);

/*
 * Stolen and hacked from musl (src/passwd/getpwent_a.c)
 *
 * SPDX-FileCopyrightText: The musl Contributors
 *
 * SPDX-License-Identifier: MIT
 */

static unsigned atou(char **s) {
  unsigned x;
  for (x = 0; **s - '0' < 10; ++*s)
    x = 10 * x + (**s - '0');
  return x;
}

int __getpwent_a(FILE *f, struct passwd *pw, char **line, size_t *size,
                 struct passwd **res) {
  ssize_t l;
  char *s;
  int rv = 0;
  // int cs;
  // pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &cs);
  for (;;) {
    if ((l = getline(line, size, f)) < 0) {
      rv = ferror(f) ? errno : 0;
      free(*line);
      *line = 0;
      pw = 0;
      break;
    }
    line[0][l - 1] = 0;

    s = line[0];
    pw->pw_name = s++;
    if (!(s = strchr(s, ':')))
      continue;

    *s++ = 0;
    pw->pw_passwd = s;
    if (!(s = strchr(s, ':')))
      continue;

    *s++ = 0;
    pw->pw_uid = atou(&s);
    if (*s != ':')
      continue;

    *s++ = 0;
    pw->pw_gid = atou(&s);
    if (*s != ':')
      continue;

    *s++ = 0;
    pw->pw_gecos = s;
    if (!(s = strchr(s, ':')))
      continue;

    *s++ = 0;
    pw->pw_dir = s;
    if (!(s = strchr(s, ':')))
      continue;

    *s++ = 0;
    pw->pw_shell = s;
    break;
  }
  // pthread_setcancelstate(cs, 0);
  *res = pw;
  if (rv)
    errno = rv;
  return rv;
}

/*
 * Stolen and hacked from musl (src/passwd/getpw_a.c)
 *
 * SPDX-FileCopyrightText: The musl Contributors
 *
 * SPDX-License-Identifier: MIT
 */
int __getpw_a(const char *name, uid_t uid, struct passwd *pw, char **buf,
              size_t *size, struct passwd **res) {
  FILE *f;
  // int cs;
  int rv = 0;

  *res = 0;

  // pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &cs);

  if (passwd_path) {
    f = fopen(passwd_path, "rbe");
  } else {
    fprintf(stderr, "passwd_path not set\n");
    exit(EXIT_FAILURE);
  }
  if (!f) {
    rv = errno;
    goto done;
  }

  while (!(rv = __getpwent_a(f, pw, buf, size, res)) && *res) {
    if ((name && !strcmp(name, (*res)->pw_name)) ||
        (!name && (*res)->pw_uid == uid))
      break;
  }
  fclose(f);

done:
  // pthread_setcancelstate(cs, 0);
  if (rv)
    errno = rv;
  return rv;
}

struct passwd *getpw(char * passwd_file_path, const char *name, uid_t uid) {
  passwd_path=passwd_file_path;
  static char *line;
  static struct passwd pw;
  static size_t size;

  struct passwd *res;
  __getpw_a(name, uid, &pw, &line, &size, &res);
  return res;
}

#endif

