#!/bin/sh
set -eu

msg() { printf "%s\n" "$*" >&2; }

is_android() { test -f /system/bin/linker; }
is_termux() { test ${TERMUX_VERSION+1}; }
is_anotherterm() { echo "${APP_ID-}" | grep -q "anotherterm"; }

download_rootfs() {

	mkdir -p "${DISTROS_DIR}"

	dl_cmd="curl -SsLk"
	if ! command -v curl >/dev/null && command -v wget >/dev/null; then
		dl_cmd="wget -qo-"
	fi

	if [ ! -f "${archive_path}" ]; then
		${dl_cmd} "${archive_url}" >"${archive_path}.download"
		mv "${archive_path}.download" "${archive_path}"
	fi
}

test_install() {
	msg "=== Test install ==="
	rm -rf "${rootfs}"
	gzip -d <"${archive_path}" | "${PROOTIE}" -v install "${rootfs}"
}

test_login() {
	msg "=== Test login ==="
	"${PROOTIE}" -v login "${rootfs}" -- /bin/pwd
}

test_archive() {
	msg "=== Test archive ==="
	"${PROOTIE}" -v archive "${rootfs}" | tar -t | wc -l
}

test_env_option() {
	msg "=== Test env option ==="
	"${PROOTIE}" -v login "${rootfs}" --env "A=Apple" --env="B=Boy" -- /bin/sh -lc env | grep -E "^(A=|B=|C=).+$"
}

test_host_utils() {
	msg "=== Test host utils ==="
	if is_termux; then
		"${PROOTIE}" -v login "${rootfs}" --host-utils -- /bin/sh -lc "termux-open-url https://bing.com"
	elif is_anotherterm; then
		"${PROOTIE}" -v login "${rootfs}" --host-utils -- /bin/sh -lc "$TERMSH clipboard-copy xx"
	fi
}

main() {
	PROOTIE="${PROOTIE-./prootie.sh}"
	DISTROS_DIR="${DISTROS_DIR-./distros}"

	rootfs="${DISTROS_DIR}/alpine"
	arch=$(uname -m)
	version=3.20.1
	version_main=$(echo "${version}" | grep -Eo "^[0-9]\.[0-9]+")
	archive_name=alpine-minirootfs-${version}-${arch}.tar.gz
	archive_path="${TMPDIR-/tmp}/${archive_name}"
	archive_url=https://dl-cdn.alpinelinux.org/alpine/v${version_main}/releases/${arch}/${archive_name}

	reset
	# cc -s -static prootie.c -o prootie
	cc -s prootie.c -o prootie

	download_rootfs
	test_install
	test_login
	test_archive
	test_host_utils
}

main "$@"
