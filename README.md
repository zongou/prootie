# prootie

Supercharges your PRoot experience.

## prootie VS proot-distro

Login 40x faster than proot-distro.

| Command                                    |      Mean [s] | Min [s] | Max [s] |      Relative |
| :----------------------------------------- | ------------: | ------: | ------: | ------------: |
| `proot-distro login alpine -- pwd`         | 1.894 ± 0.070 |   1.739 |   2.004 | 42.71 ± 11.66 |
| `./prootie.sh login distros/alpine -- pwd` | 0.301 ± 0.022 |   0.270 |   0.337 |   6.79 ± 1.90 |
| `./prootie login distros/alpine -- pwd`    | 0.044 ± 0.012 |   0.028 |   0.080 |          1.00 |

## Quick start

### Compile from source

On Termux:

```sh
cc -o "${PREFIX}/bin/prootie" prootie.c -s -Os
```

On Linux:

```sh
cc -o prootie prootie.c -s -Os
sudo mv prootie /usr/local/bin/prootie
```

### Manage distros with prootie-tui.sh

make sure you have [`gum`](https://github.com/charmbracelet/gum) installed

```sh
./tmp/prootie_tui.sh
```

### Manage distros manually

#### Install rootfs

```sh
version=3.20.3
DISTROS_DIR=${HOME}/.prootie/distros
mkdir -p "${DISTROS_DIR}"

arch=$(uname -m)
version_main=$(echo "${version}" | grep -Eo "^[0-9]\.[0-9]+")
archive=alpine-minirootfs-${version}-${arch}.tar.gz
rootfs_url=https://dl-cdn.alpinelinux.org/alpine/v${version_main}/releases/${arch}/${archive}

curl -Lk "${rootfs_url}" | gzip -d | prootie -v install ./alpine
```

#### Login rootfs

```sh
prootie login ./alpine
```

#### Archive rootfs

```sh
prootie archive ./alpine | gzip > alpine.tar.gz
```

#### Clone rootfs

```sh
prootie archive ./alpine | prootie install ./alpine2
```

### Tested distros

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

### More

[Document for configuring Desktop enviroment and Audio on android](doc.md)
