#!/bin/sh
set -eu

is_android() { test -f /system/bin/linker; }
is_termux() { test "${TERMUX_VERSION+1}"; }
is_anotherterm() { echo "${APP_ID-}" | grep -q "anotherterm"; }

PROOTIE="${PROOTIE-./prootie}"
version=3.20.1
rootfs=./alpine

arch=$(uname -m)
version_main=$(echo "${version}" | grep -Eo "^[0-9]\.[0-9]+")
archive_name=alpine-minirootfs-${version}-${arch}.tar.gz
archive_path="${TMPDIR-/tmp}/${archive_name}"
archive_url=https://dl-cdn.alpinelinux.org/alpine/v${version_main}/releases/${arch}/${archive_name}

if command -v curl >/dev/null; then
	dl_cmd="curl -Lk"
elif command -v wget >/dev/null; then
	dl_cmd="wget -o-"
fi

if [ ! -f "${archive_path}" ]; then
	${dl_cmd} "${archive_url}" >"${archive_path}.download"
	mv "${archive_path}.download" "${archive_path}"
fi

reset
# cc -s -static prootie.c -o prootie
cc -s prootie.c -o prootie

rm -rf "${rootfs}"

## Test install, login and archive
gzip -d <"${archive_path}" | "${PROOTIE}" -v install "${rootfs}"
"${PROOTIE}" -v login "${rootfs}" -- /bin/pwd
"${PROOTIE}" -v archive "${rootfs}" | tar -t | wc -l

## Test Variables
"${PROOTIE}" -v login "${rootfs}" --env "A=Apple" --env="B=Boy" -- /bin/sh -lc env | grep -E "^(A=|B=|C=).+$"

## Test host-utils
if is_termux; then
	"${PROOTIE}" -v login "${rootfs}" --host-utils -- /bin/sh -lc "termux-open-url https://bing.com"
elif is_anotherterm; then
	"${PROOTIE}" -v login "${rootfs}" --host-utils -- /bin/sh -lc "$TERMSH clipboard-copy xx"
fi
