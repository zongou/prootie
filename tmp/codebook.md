# Code Book

Use this file with ease in vscode with extension [zongou.simple-runner](https://marketplace.visualstudio.com/items?itemName=zongou.simple-runner)

## Installing

Termux

```sh
set -eu
zig cc -target $(uname -m)-linux-musl -o $PREFIX/bin/prootie prootie.c -s -Os
zig cc -target $(uname -m)-linux-musl -o $PREFIX/bin/plogin tmp/plogin.c -s -Os
cp prootie.sh $PREFIX/bin/prootie.sh

file $(command -v prootie)
file $(command -v plogin)
du -ahd0 $(command -v prootie)
du -ahd0 $(command -v plogin)
```

Linux

```sh
set -eu
zig cc -target $(uname -m)-linux-musl -o prootie prootie.c -s -Os
zig cc -target $(uname -m)-linux-musl -o plogin tmp/plogin.c -s -Os
sudo mv prootie /usr/local/bin/prootie
sudo mv plogin /usr/local/bin/plogin
sudo cp prootie.sh /usr/local/bin/prootie.sh

file $(command -v prootie)
file $(command -v plogin)
du -ahd0 $(command -v prootie)
du -ahd0 $(command -v plogin)
```

## Test

### Prepare rootfs

```sh
set -eu
arch=$(uname -m)
version=3.20.3
version_main=$(echo "${version}" | grep -Eo "^[0-9]\.[0-9]+")
archive_url=https://dl-cdn.alpinelinux.org/alpine/v${version_main}/releases/${arch}/alpine-minirootfs-${version}-${arch}.tar.gz
archive_url=https://mirrors.tuna.tsinghua.edu.cn/alpine/v${version_main}/releases/${arch}/alpine-minirootfs-${version}-${arch}.tar.gz
archive_path="${TMPDIR-/tmp}/alpine-rootfs.tar.gz"

dl_cmd="curl -SLk"
if ! command -v curl >/dev/null && command -v wget >/dev/null; then
	dl_cmd="wget -o-"
fi

${dl_cmd} "${archive_url}" >"${archive_path}.download"
mv "${archive_path}.download" "${archive_path}"
```

### Instll

```sh
set -eu
archive_path="${TMPDIR-/tmp}/alpine-rootfs.tar.gz"
rootfs="${HOME}/.distros/alpine"
mkdir -p $(dirname ${rootfs})
gzip -d <"${archive_path}" | prootie -v install "${rootfs}"
```

### Login

```sh
set -eu
rootfs="${HOME}/.distros/alpine"
prootie -v login "${rootfs}"
```

### Archive

```sh
set -eu
rootfs="${HOME}/.distros/alpine"
prootie archive "${rootfs}" | tar -t | wc -l
```

### Env option

```sh
set -eu
rootfs="${HOME}/.distros/alpine"
prootie login "${rootfs}" --env "A=Apple" --env="B=Boy" -- /bin/sh -lc env | grep -E "^(A=|B=|C=).+$"
```

### Host utils

```sh
set -eu
is_android() { test -f /system/bin/linker; }
is_termux() { test ${TERMUX_VERSION+1}; }
is_anotherterm() { echo "${APP_ID-}" | grep -q "anotherterm"; }

rootfs="${HOME}/.distros/alpine"

if is_termux; then
	prootie login "${rootfs}" --host-utils -- /bin/sh -lc "termux-open-url https://bing.com"
elif is_anotherterm; then
	prootie login "${rootfs}" --host-utils -- /bin/sh -lc "$TERMSH notify 'hostutils work'"
fi
```

## Benchmark

Termux

```sh
#!/bin/sh
set -eu
rootfs="${HOME}/.distros/alpine"
result_md="${TMPDIR-/tmp}/prootie_benchmark_result.md"

hyperfine \
	--warmup 2 \
	--min-runs 20 \
	--max-runs 100 \
	--export-markdown "${result_md}" \
	"proot-distro login alpine -- pwd" \
	"prootie.sh login ${rootfs} -- pwd" \
	"prootie login ${rootfs} -- pwd"

echo "Result file: ${result_md}"
```

Linux

```sh
#!/bin/sh
set -eu
rootfs="${HOME}/.distros/alpine"
result_md="${TMPDIR-/tmp}/prootie_benchmark_result.md"

hyperfine \
	--warmup 2 \
	--min-runs 20 \
	--max-runs 1000 \
	--export-markdown "${result_md}" \
	"prootie.sh login ${rootfs} -- pwd" \
	"prootie login ${rootfs} -- pwd"

echo "Result file: ${result_md}"
```
