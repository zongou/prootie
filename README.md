# prootie

Supercharges your PRoot experience.

## prootie VS proot-distro

Login 35x faster

| Command                                        |      Mean [s] | Min [s] | Max [s] |      Relative |
| :--------------------------------------------- | ------------: | ------: | ------: | ------------: |
| `proot-distro login alpine -- /bin/sh -lc pwd` | 1.507 ± 0.482 |   0.918 |   2.098 | 35.90 ± 23.54 |
| `./prootie.sh login alpine -- /bin/sh -lc pwd` | 0.620 ± 0.153 |   0.494 |   1.018 |  14.77 ± 9.21 |
| `./prootie login alpine -- /bin/sh -lc pwd`    | 0.042 ± 0.024 |   0.018 |   0.101 |          1.00 |

Comparison:

| Item                              | prootie         | proot-distro |
| --------------------------------- | --------------- | ------------ |
| written language                  | c               | bash         |
| install custom rootfs archive     | easy            | hard         |
| customizable rootfs location      | yes             | no           |
| installing while downloading      | yes             | no           |
| clone rootfs                      | yes             | no           |
| output standard rootfs archive    | yes             | no           |
| handling stdin, stdout and stderr | yes             | no           |
| user-friendly                     | only 3 commands | 10 commands  |
| supported platforms               | Android, Linux  | Android      |

## Quick start

### Compile from source

```sh
cc prootie.c -o prootie
```

### Install rootfs

```sh
version=3.20.1
rootfs=./alpine

arch=$(uname -m)
version_main=$(echo "${version}" | grep -Eo "^[0-9]\.[0-9]+")
archive=alpine-minirootfs-${version}-${arch}.tar.gz
rootfs_url=https://dl-cdn.alpinelinux.org/alpine/v${version_main}/releases/${arch}/${archive}

curl -Lk "${rootfs_url}" | gzip -d | ./prootie -v install "${rootfs}"
```

### Login rootfs

```sh
prootie login alpine
```

### Archive rootfs

```sh
prootie archive alpine | gzip > alpine.tar.gz
```

### Clone rootfs

```sh
prootie archive alpine | prootie install alpine2
```

## Tested distros

| distro            | status | notes                                          |
| ----------------- | ------ | ---------------------------------------------- |
| almalinux (9)     | OK     |                                                |
| alt (p11)         | OK     |                                                |
| alpine (3.20)     | OK     |                                                |
| amazonlinux       | OK     |                                                |
| archlinux         | OK     |                                                |
| debian (bookworm) | OK     |                                                |
| voidlinux (musl)  | OK     | `vi /etc/passwd` to change login shell to bash |
| fedora (40)       | OK     |                                                |
| ubuntu (noble)    | OK     | `touch /root/.hushlogin`                       |
| busybox (1.36.1)  | OK     |                                                |
| gentoo (openrc)   | OK     |                                                |
| kali              | OK     |                                                |
| rockylinux (9)    | OK     |                                                |
| openwrt (23.05)   | OK     |                                                |

## More

[Document for configuring Desktop enviroment and Audio on android](doc.md)
