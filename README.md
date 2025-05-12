# prootie

## Build

| key     | value     |
| ------- | --------- |
| program | ./prootie |

Build this program

```sh
${CC-cc} -o ${program} prootie.c ${CFLAGS-} ${LDFLAGS-} "$@"
```

### Run

Build and run

- [x] sanitize

```sh
if [ ${sanitize} -eq 1 ]; then
    CFLAGS="${CFLAGS-} -fsanitize=address"
fi

export CFLAGS
${MD_EXE} --file=${MD_FILE} build
${program} "$@"
```

### Release

Build static and stripped

- [x] static
- [x] stripped
- [x] opt_size

```sh
if [ ${static} -eq 1 ]; then
    LDFLAGS="${LDFLAGS+${LDFLAGS}} -static"
fi

if [ ${stripped} -eq 1 ]; then
    LDFLAGS="${LDFLAGS+${LDFLAGS}} -s"
fi

if [ ${opt_size} -eq 1 ]; then
    LDFLAGS="${LDFLAGS+${LDFLAGS}} -Os"
fi

export LDFLAGS
${MD_EXE} --file=${MD_FILE} build
du -ahd0 ${program}
file ${program}
```

### Install

Install this program

```sh
if ! test "${CC+1}" && command -v zig >/dev/null; then
    export CC="zig cc --target=$(uname -m)-linux-musl"
fi

${MD_EXE} --file=${MD_FILE} release
program=$(basename "${PWD}")
if command -v sudo >/dev/null; then
    sudo install "${program}" "/usr/local/bin/${program}"
elif test "${PREFIX+1}"; then
    install "${program}" "${PREFIX}/bin/${program}"
fi
```

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

## alpine

```sh
export CC=/opt/cosmo/bin/cosmocc
export MODE=tiny
$MD_EXE --file=${MD_FILE} release

set -eu
arch=$(uname -m)
version=3.20.3
version_main=$(echo "${version}" | grep -Eo "^[0-9]\.[0-9]+")
# mirror=https://dl-cdn.alpinelinux.org
mirror=https://mirrors.ustc.edu.cn
archive_url=${mirror}/alpine/v${version_main}/releases/${arch}/alpine-minirootfs-${version}-${arch}.tar.gz

rootfs="${HOME}/.distros/alpine"
mkdir -p "$(dirname "${rootfs}")"
curl -SsLk "${archive_url}" | gzip -d | ./prootie -v install -v "${rootfs}"
```

### Login

```sh
prootie login "${HOME}/.distros/alpine"
```

### Archive

```sh
prootie archive "${HOME}/.distros/alpine" | gzip > alpine.tar.gz
```

### Clone

```sh
prootie archive "${HOME}/.distros/alpine" | prootie install "${HOME}/.distros/alpine2"
```

## More

[Document for configuring Desktop enviroment and Audio on android](tmp/doc.md)
