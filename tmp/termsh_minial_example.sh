rootfs=./alpine
archive=/sdcard/mirrors/lxc/archives/rootfs/alpine-minirootfs-3.20.1-aarch64.tar.gz
rm -rf "${rootfs}"
mkdir -p "${rootfs}"
gzip -d <"${archive}" | proot tar -C "${rootfs}" -xv

unset LD_PRELOAD

# termsh
proot \
	--rootfs=$PWD/"${rootfs}" \
	--link2symlink \
	--kill-on-exit \
	--root-id \
	--cwd=/root \
	--bind=/dev \
	--bind=/proc \
	--bind=/system \
	--bind=/apex \
	--bind=${LIB_DIR}/libtermsh.so:/termsh \
	/usr/bin/env -i \
	TERMSH_UID="$(id -u)" \
	SHELL_SESSION_TOKEN=${SHELL_SESSION_TOKEN} \
	/termsh notify "$(date)"

# termsh plugin
proot \
	--rootfs=$PWD/"${rootfs}" \
	--link2symlink \
	--kill-on-exit \
	--root-id \
	--cwd=/root \
	--bind=/dev \
	--bind=/proc \
	--bind=/system \
	--bind=/apex \
	--bind=${LIB_DIR}/libtermsh.so:/termsh \
	--bind=$(dirname $("$TERMSH" plugin green_green_avk.anothertermshellplugin_android10essentials apk)) \
	/usr/bin/env -i \
	TERMSH_UID="$(id -u)" \
	SHELL_SESSION_TOKEN=${SHELL_SESSION_TOKEN} \
	/bin/sh -l -c "\$(/termsh plugin green_green_avk.anothertermshellplugin_android10essentials proot) --version"
