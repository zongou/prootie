# prootie

Supercharges your PRoot experience.

## prootie VS proot-distro

Login 40x faster

| Command                                    |      Mean [s] | Min [s] | Max [s] |      Relative |
| :----------------------------------------- | ------------: | ------: | ------: | ------------: |
| `proot-distro login alpine -- pwd`         | 1.894 ± 0.070 |   1.739 |   2.004 | 42.71 ± 11.66 |
| `./prootie.sh login distros/alpine -- pwd` | 0.301 ± 0.022 |   0.270 |   0.337 |   6.79 ± 1.90 |
| `./prootie login distros/alpine -- pwd`    | 0.044 ± 0.012 |   0.028 |   0.080 |          1.00 |

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
