#!/bin/sh
set -eu

local_arch="$(uname -m)"
case "${local_arch}" in
armv7a) remote_arch=armhf ;;
aarch64) remote_arch=arm64 ;;
i686) remote_arch=i386 ;;
x86_64) remote_arch=amd64 ;;
*) remote_arch="$1" ;;
esac

if command -v curl >/dev/null; then
	dl_cmd="curl -SsLk"
elif command -v wget >/dev/null; then
	dl_cmd="wget -qo-"
fi

filter_data() {
	index=0
	echo "distro,release,variant,path"
	while IFS=';' read -r DISTRO RELEASE ARCH VAR TS PATH; do
		if test $remote_arch = $ARCH && ! test $VAR = cloud && ! test ${DISTRO} = "nixos"; then
			# echo "${DISTRO}" "${RELEASE}" "${ARCH}" "${VAR}" "${TS}" "${PATH}"
			echo "${DISTRO},${RELEASE},${VAR},${PATH}"
			index=$((index + 1))
		fi
	done
}

lxc_mirror="$(
	gum choose \
		--header="Choose a LXC mirror:" \
		https://images.linuxcontainers.org \
		https://mirrors.tuna.tsinghua.edu.cn/lxc-images \
		https://mirrors.cernet.edu.cn/lxc-images \
		https://mirrors.nju.edu.cn/lxc-images \
		https://mirrors.bfsu.edu.cn/lxc-images \
		https://mirror.lzu.edu.cn/lxc-images \
		https://mirror.nyist.edu.cn/lxc-images
)"

meta="${lxc_mirror}/meta/1.0/index-user"
selection=$(${dl_cmd} -Ss "${meta}" | filter_data | gum table --widths=10,10,10,0)
if [ -z "${selection}" ]; then
	exit 1
fi
distro=$(echo "${selection}" | cut -d',' -f1)
release=$(echo "${selection}" | cut -d',' -f2)
variant=$(echo "${selection}" | cut -d',' -f3)
path=$(echo "${selection}" | cut -d',' -f4)

${dl_cmd} "${lxc_mirror}${path}rootfs.tar.xz" | xz -d | "${PROOTIE-./prootie}" install -v "${distro}-${release}-${variant}"
