rootfs=./alpine
archive=/sdcard/mirrors/lxc/archives/rootfs/alpine-minirootfs-3.20.1-aarch64.tar.gz
rm -rf "${rootfs}"
mkdir -p "${rootfs}"
gzip -d <"${archive}" | proot tar -C "${rootfs}" -xv

unset LD_PRELOAD
exec proot \
	--rootfs=$PWD/"${rootfs}" \
	--link2symlink \
	--kill-on-exit \
	--root-id \
	--cwd=/root \
	--bind=/dev \
	--bind=/proc \
	--bind=/system \
	--bind=/apex \
	--bind="${PREFIX}" \
	/usr/bin/env -i \
	ANDROID_DATA='/data' \
	ANDROID_RUNTIME_ROOT='/apex/com.android.runtime' \
	ANDROID_TZDATA_ROOT='/apex/com.android.tzdata' \
	BOOTCLASSPATH='/apex/com.android.runtime/javalib/core-oj.jar:/apex/com.android.runtime/javalib/core-libart.jar:/apex/com.android.runtime/javalib/okhttp.jar:/apex/com.android.runtime/javalib/bouncycastle.jar:/apex/com.android.runtime/javalib/apache-xml.jar:/system/framework/com.nxp.nfc.nq.jar:/system/framework/framework.jar:/system/framework/ext.jar:/system/framework/telephony-common.jar:/system/framework/voip-common.jar:/system/framework/ims-common.jar:/system/framework/miuisdk@boot.jar:/system/framework/miuisystemsdk@boot.jar:/system/framework/android."${rootfs}".base.jar:/system/framework/telephony-ext.jar:/system/framework/tcmiface.jar:/system/framework/QPerformance.jar:/system/framework/UxPerformance.jar:/system/framework/WfdCommon.jar:/apex/com.android.conscrypt/javalib/conscrypt.jar:/apex/com.android.media/javalib/updatable-media.jar' \
	PATH="${PREFIX}/bin:/usr/bin:/bin" \
	${PREFIX}/bin/termux-open-url https://bing.com
