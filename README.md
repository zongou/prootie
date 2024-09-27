# prootie

Supercharges your PRoot experience.

- Login 50x faster than proot-distro.
- 0 knowledge required to use with the TUI script.
- Install while downloading rootfs.
- Install distro to any directory you like.
- Outputs standard rootfs archive.
- Use with ease, only 3 commands.
- supportes Android and linux.
- Clone rootfs with command `archive` and `install` used together.
- built-in commands in Termux and Anotherterm are supported with option `--host-utils` of command `login`.

| Command                                         |      Mean [s] | Min [s] | Max [s] |      Relative |
| :---------------------------------------------- | ------------: | ------: | ------: | ------------: |
| `proot-distro login alpine -- pwd`              | 1.999 ± 0.046 |   1.892 |   2.074 | 50.40 ± 10.03 |
| `prootie.sh login $HOME/.distros/alpine -- pwd` | 0.337 ± 0.023 |   0.307 |   0.387 |   8.51 ± 1.78 |
| `prootie login $HOME/.distros/alpine -- pwd`    | 0.040 ± 0.008 |   0.027 |   0.067 |          1.00 |

## Compile from source

On Termux:

```sh
cc -o "${PREFIX}/bin/prootie" prootie.c -s -Os
cp prootie_tui "${PREFIX}/bin/prootie_tui"
```

On Linux:

```sh
cc -o prootie prootie.c -s -Os
sudo mv prootie /usr/local/bin/prootie
sudo cp prootie_tui /usr/bin/prootie_tui
```

## Manage distros with TUI script

make sure you have [`gum`](https://github.com/charmbracelet/gum) installed

```sh
prootie_tui
```

## Manage distros manually

### Install rootfs

```sh
set -eu
arch=$(uname -m)
version=3.20.3
version_main=$(echo "${version}" | grep -Eo "^[0-9]\.[0-9]+")
archive_url=https://dl-cdn.alpinelinux.org/alpine/v${version_main}/releases/${arch}/alpine-minirootfs-${version}-${arch}.tar.gz
rootfs="${HOME}/.distros/alpine"

mkdir -p "$(dirname "${rootfs}")"
curl -SsLk "${archive_url}" | gzip -d | prootie -v install "${rootfs}"
```

### Login rootfs

```sh
prootie login "${HOME}/.distros/alpine"
```

### Archive rootfs

```sh
prootie archive "${HOME}/.distros/alpine" | gzip > alpine.tar.gz
```

### Clone rootfs

```sh
prootie archive "${HOME}/.distros/alpine" | prootie install "${HOME}/.distros/alpine2"
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

[Document for configuring Desktop enviroment and Audio on android](tmp/doc.md)
