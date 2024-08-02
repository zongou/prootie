## THIS SCRIPT IS WRITTEN IN POSIX SHELL
set -eu

PROGRAM="$(basename "$0")"

msg() { printf "%s\n" "$*" >&2; }
info() { printf "%s\n" "${PROGRAM+${PROGRAM}:}$*" >&2; }
vmsg() { if script_opt_is_verbose; then msg "${PROGRAM}:${COMMAND}:$*"; fi; }
error_exit() { printf "%s: %s\n" "${PROGRAM}" "$*" >&2 && exit 1; }
error_exit_unknown_option() { error_exit "Unknown option '$*'"; }
error_exit_argument_error() { error_exit "Argument error: '$*'"; }

is_android() { test -f /system/bin/linker; }
is_termux() { test "${TERMUX_VERSION+1}"; }
is_anotherterm() { echo "${APP_ID-}" | grep -q "anotherterm"; }

## [ROOTFS] [ITERM]
get_proot_conf() {
	case "$2" in
	proot_data_dir) echo "$1/.proot" ;;
	proot_fakerootfs_dir) echo "$1/.proot/rootfs" ;;
	proot_l2s_dir) echo "$1/.proot/l2s" ;;
	*) ;;
	esac
}

set_proot_path() {
	if ! test ${PROOT+1}; then
		if _val=$(command -v proot >&1); then
			PROOT="${_val}"
		else
			error_exit "Cannot find proot in PATH and env PROOT not set."
		fi
	fi
}

## [ROOTFS]
set_proot_env() {
	unset LD_PRELOAD

	if ! test ${PROOT_L2S_DIR+1}; then
		PROOT_L2S_DIR="$(get_proot_conf "$1" proot_l2s_dir)"
		if ! test -d "${PROOT_L2S_DIR}"; then
			mkdir -p "${PROOT_L2S_DIR}"
		fi
	fi
	PROOT_L2S_DIR=$(realpath "${PROOT_L2S_DIR}")
	export PROOT_L2S_DIR

	if ! test ${PROOT_TMP_DIR+1}; then
		if test "${TMPDIR+1}"; then
			PROOT_TMP_DIR="${TMPDIR}"
		else
			PROOT_TMP_DIR="/tmp"
		fi
	fi
	mkdir -p "${PROOT_TMP_DIR}"
	export PROOT_TMP_DIR

	vmsg "env=LD_PRELOAD= PROOT_L2S_DIR=\"${PROOT_L2S_DIR}\" PROOT_TMP_DIR=\"${PROOT_TMP_DIR}\""
}

command_install() {
	_show_help() {
		msg "\
Install rootfs to the specific dir from stdin.

Usage:
  ${PROGRAM} ${COMMAND} [OPTION...] [ROOTFS]

Options:
  --help              show this help

Tar relavent options:
  -v, --verbose
  --exclude\
"
	}

	_opt_tar_is_verbose=0

	if test $# -gt 0; then
		while test $# -gt 0; do
			case "$1" in
			--help)
				_show_help
				exit
				;;
			-v | --verbose)
				shift
				_opt_tar_is_verbose=1
				;;
			--exclude=*)
				if test "${tar_excludes+1}"; then
					tar_excludes=$(printf "%s\n%s\n" "${tar_excludes}" "$(echo "$1" | base64)")
				else
					tar_excludes=$(echo "$1" | base64)
				fi
				shift
				;;
			-*) error_exit_unknown_option "$1" ;;
			*)
				if ! test "${_opt_rootfs_dir+1}"; then
					if ! test -e "$1"; then
						_opt_rootfs_dir="$1"
						shift
					else
						error_exit "Rootfs '$1' already exists."
					fi
				else
					error_exit_argument_error "$@"
				fi
				;;
			esac
		done
	else
		_show_help
		exit 1
	fi

	on_error() {
		chmod +rw -R "${_opt_rootfs_dir}"
		rm -rf "${_opt_rootfs_dir}"
		printf "Exiting due to failure.\n" >&2
		exit 1
	}

	on_interrupt() {
		trap - EXIT
		chmod +rw -R "${_opt_rootfs_dir}"
		rm -rf "${_opt_rootfs_dir}"
		printf "\rExit as requested." >&2
		exit 1
	}

	trap "on_error" EXIT
	trap "on_interrupt" HUP INT TERM

	mkdir -p "${_opt_rootfs_dir}"
	set_proot_path
	set_proot_env "${_opt_rootfs_dir}"
	set -- --link2symlink --root-id tar

	if ${_opt_tar_is_verbose}; then
		set -- "$@" -v
	fi

	set -- "$@" -C "${_opt_rootfs_dir}" -x

	if test "${tar_excludes+1}"; then
		# shellcheck disable=SC2046
		shift -- "$@" $(echo "${tar_excludes}" | base64 -d)
	fi

	vmsg "cmd=${PROOT} $*"
	"${PROOT}" "$@"

	proot_fakerootfs_dir="$(get_proot_conf "${_opt_rootfs_dir}" proot_fakerootfs_dir)"

	mkdir -p "${proot_fakerootfs_dir}/proc"
	cat >"${proot_fakerootfs_dir}/proc/loadavg" <<-EOF
		0.00 0.00 0.00 0/0 0
	EOF

	cat >"${proot_fakerootfs_dir}/proc/stat" <<-EOF
		cpu  0 0 0 0 0 0 0 0 0 0
	EOF

	cat >"${proot_fakerootfs_dir}/proc/uptime" <<-EOF
		0.00 0.00
	EOF

	cat >"${proot_fakerootfs_dir}/proc/version" <<-EOF
		Linux localhost 6.1.0-22 #1 SMP PREEMPT_DYNAMIC 6.1.94-1 (2024-06-21) GNU/Linux
	EOF

	cat >"${proot_fakerootfs_dir}/proc/vmstat" <<-EOF
		nr_free_pages 136777
		nr_zone_inactive_anon 14538
		nr_zone_active_anon 215
		nr_zone_inactive_file 45670
		nr_zone_active_file 14489
		nr_zone_unevictable 6936
		nr_zone_write_pending 812
		nr_mlock 6936
		nr_bounce 0
		nr_zspages 0
		nr_free_cma 0
		numa_hit 325814
		numa_miss 0
		numa_foreign 0
		numa_interleave 2632
		numa_local 325814
		numa_other 0
		nr_inactive_anon 14538
		nr_active_anon 215
		nr_inactive_file 45670
		nr_active_file 14489
		nr_unevictable 6936
		nr_slab_reclaimable 6400
		nr_slab_unreclaimable 7349
		nr_isolated_anon 0
		nr_isolated_file 0
		workingset_nodes 0
		workingset_refault_anon 0
		workingset_refault_file 0
		workingset_activate_anon 0
		workingset_activate_file 0
		workingset_restore_anon 0
		workingset_restore_file 0
		workingset_nodereclaim 0
		nr_anon_pages 19166
		nr_mapped 18188
		nr_file_pages 62691
		nr_dirty 812
		nr_writeback 0
		nr_writeback_temp 0
		nr_shmem 266
		nr_shmem_hugepages 0
		nr_shmem_pmdmapped 0
		nr_file_hugepages 0
		nr_file_pmdmapped 0
		nr_anon_transparent_hugepages 0
		nr_vmscan_write 0
		nr_vmscan_immediate_reclaim 0
		nr_dirtied 1688
		nr_written 875
		nr_kernel_misc_reclaimable 0
		nr_foll_pin_acquired 0
		nr_foll_pin_released 0
		nr_kernel_stack 2172
		nr_page_table_pages 492
		nr_swapcached 0
		nr_dirty_threshold 35995
		nr_dirty_background_threshold 17975
		pgpgin 299220
		pgpgout 4416
		pswpin 0
		pswpout 0
		pgalloc_dma 32
		pgalloc_dma32 333528
		pgalloc_normal 0
		pgalloc_movable 0
		allocstall_dma 0
		allocstall_dma32 0
		allocstall_normal 0
		allocstall_movable 0
		pgskip_dma 0
		pgskip_dma32 0
		pgskip_normal 0
		pgskip_movable 0
		pgfree 478037
		pgactivate 13017
		pgdeactivate 0
		pglazyfree 20
		pgfault 196449
		pgmajfault 1180
		pglazyfreed 0
		pgrefill 0
		pgreuse 36999
		pgsteal_kswapd 0
		pgsteal_direct 0
		pgdemote_kswapd 0
		pgdemote_direct 0
		pgscan_kswapd 0
		pgscan_direct 0
		pgscan_direct_throttle 0
		pgscan_anon 0
		pgscan_file 0
		pgsteal_anon 0
		pgsteal_file 0
		zone_reclaim_failed 0
		pginodesteal 0
		slabs_scanned 0
		kswapd_inodesteal 0
		kswapd_low_wmark_hit_quickly 0
		kswapd_high_wmark_hit_quickly 0
		pageoutrun 0
		pgrotated 0
		drop_pagecache 0
		drop_slab 0
		oom_kill 0
		numa_pte_updates 0
		numa_huge_pte_updates 0
		numa_hint_faults 0
		numa_hint_faults_local 0
		numa_pages_migrated 0
		pgmigrate_success 0
		pgmigrate_fail 0
		thp_migration_success 0
		thp_migration_fail 0
		thp_migration_split 0
		compact_migrate_scanned 0
		compact_free_scanned 0
		compact_isolated 0
		compact_stall 0
		compact_fail 0
		compact_success 0
		compact_daemon_wake 0
		compact_daemon_migrate_scanned 0
		compact_daemon_free_scanned 0
		htlb_buddy_alloc_success 0
		htlb_buddy_alloc_fail 0
		unevictable_pgs_culled 93162
		unevictable_pgs_scanned 0
		unevictable_pgs_rescued 7
		unevictable_pgs_mlocked 6943
		unevictable_pgs_munlocked 7
		unevictable_pgs_cleared 0
		unevictable_pgs_stranded 0
		thp_fault_alloc 0
		thp_fault_fallback 0
		thp_fault_fallback_charge 0
		thp_collapse_alloc 0
		thp_collapse_alloc_failed 0
		thp_file_alloc 0
		thp_file_fallback 0
		thp_file_fallback_charge 0
		thp_file_mapped 0
		thp_split_page 0
		thp_split_page_failed 0
		thp_deferred_split_page 0
		thp_split_pmd 0
		thp_split_pud 0
		thp_zero_page_alloc 0
		thp_zero_page_alloc_failed 0
		thp_swpout 0
		thp_swpout_fallback 0
		balloon_inflate 0
		balloon_deflate 0
		balloon_migrate 0
		swap_ra 0
		swap_ra_hit 0
		direct_map_level2_splits 28
		direct_map_level3_splits 0
		nr_unstable 0
	EOF

	mkdir -p "${proot_fakerootfs_dir}/proc/sys/kernel"
	cat >"${proot_fakerootfs_dir}/proc/sys/kernel/cap_last_cap" <<-EOF
		40
	EOF

	mkdir -p "${proot_fakerootfs_dir}/proc/sys/fs/inotify"
	cat >"${proot_fakerootfs_dir}/proc/sys/fs/inotify/max_queued_events" <<-EOF
		16384
	EOF

	cat >"${proot_fakerootfs_dir}/proc/sys/fs/inotify/max_user_instances" <<-EOF
		128
	EOF

	cat >"${proot_fakerootfs_dir}/proc/sys/fs/inotify/max_user_watches" <<-EOF
		65536
	EOF

	mkdir -p "${proot_fakerootfs_dir}/etc"
	cat >"${proot_fakerootfs_dir}/etc/resolv.conf" <<-EOF
		nameserver 8.8.8.8
		nameserver 8.8.4.4
	EOF

	cat >"${proot_fakerootfs_dir}/etc/hosts" <<-EOF
		::1         localhost.localdomain localhost ip6-localhost ip6-loopback
		fe00::0     ip6-localnet
		ff00::0     ip6-mcastprefix
		ff02::1     ip6-allnodes
		ff02::2     ip6-allrouters
		ff02::3     ip6-allhosts
	EOF

	mkdir -p "${proot_fakerootfs_dir}/etc/profile.d"
	cat >"${proot_fakerootfs_dir}/etc/profile.d/locale.sh" <<-EOF
		export CHARSET=\${CHARSET:-UTF-8}
		export LANG=\${LANG:-C.UTF-8}
		export LC_COLLATE=\${LC_COLLATE:-C}
	EOF

	cat >"${proot_fakerootfs_dir}/etc/profile.d/proot.sh" <<-EOF
		export COLORTERM=truecolor
		[ -z "\$LANG" ] && export LANG=C.UTF-8
		export TERM=xterm-256color
		export TMPDIR=/tmp
		export PULSE_SERVER=127.0.0.1
		export MOZ_FAKE_NO_SANDBOX=1
		export CHROMIUM_FLAGS=--no-sandbox
	EOF

	if is_android; then
		cat >"${proot_fakerootfs_dir}/etc/profile.d/host_utils.sh" <<-EOF
			## prootie host utils
			export ANDROID_DATA='${ANDROID_DATA}'
			export ANDROID_RUNTIME_ROOT='${ANDROID_RUNTIME_ROOT}'
			export ANDROID_TZDATA_ROOT='${ANDROID_TZDATA_ROOT}'
			export BOOTCLASSPATH='${BOOTCLASSPATH}'
		EOF

		if is_anotherterm; then
			cat <<-EOF >>"${proot_fakerootfs_dir}/etc/profile.d/host_utils.sh"
				export DATA_DIR='$(realpath "${DATA_DIR}")'
				export TERMSH_UID='${USER_ID-$(id -u)}'
				export TERMSH='${LIB_DIR}/libtermsh.so'
			EOF
		fi

		if test "${PREFIX+1}"; then
			cat <<-EOF >>"${proot_fakerootfs_dir}/etc/profile.d/host_utils.sh"
				export PREFIX='${PREFIX}'
				export PATH="\${PATH}:${PREFIX}/bin"
			EOF
		fi
	fi

	## Reset trap for HUP/INT/TERM.
	trap - EXIT
}

command_login() {
	_show_help() {
		msg "\
Start login shell of the specific rootfs.

Usage:
  ## Start login shell as root
  ${PROGRAM} ${COMMAND} [OPTION...] [ROOTFS]
  
  ## Execute a command
  ${PROGRAM} ${COMMAND} [OPTION...] [ROOTFS] -- [COMMAND [ARG]...]

Options:
  -h, --help          show this help
  --host-utils        enable host utils
  --env               set environment variables
  
PRoot relavent options:
  -b, --bind, -m, --mount
  --no-kill-on-exit
  --link2symlink
  --no-link2symlink
  --no-sysvipc
  --fix-low-ports
  -q, --qemu
  -k, --kernel-release\
"
	}

	## Default option value
	_opt_is_host_utils=0

	## PRoot relavent options
	_opt_is_kill_on_exit=1
	_opt_is_link2symlink=${link2symlink_default}
	_opt_is_sysvipc=1
	_opt_is_fix_low_ports=0

	get_longopt_val() { echo "$1" | cut -d= -f2-; }

	if test $# -gt 0; then
		while test $# -gt 0; do
			case "$1" in
			--)
				shift
				if test $# -gt 0; then
					args=$(
						while test $# -gt 0; do
							echo "$1" | base64
							shift
						done
					)
					shift $#
				fi
				;;
			-h | --help)
				_show_help
				exit
				;;
			-b | --bind=* | -m | --mount=*)
				case "$1" in
				-b | -m) shift && _opt_val="$1" ;;
				*) _opt_val="$(get_longopt_val "$1")" ;;
				esac
				_opt_bindings="${_opt_bindings+${_opt_bindings} }\"${_opt_val}\""
				shift
				;;
			--cwd=* | --pwd=*)
				_opt_cwd="$(get_longopt_val "$1")"
				shift
				;;
			--host-utils)
				shift
				_opt_is_host_utils=1
				;;
			--env | --env=*)
				case "$1" in
				--env) shift && _opt_val="$1" ;;
				*) _opt_val="$(get_longopt_val "$1")" ;;
				esac
				opt_envs="$(printf "%s\n%s" "${opt_envs-}" "$(echo "${_opt_val}" | base64)")"
				shift
				;;
			--no-kill-on-exit)
				shift
				_opt_is_kill_on_exit=0
				;;
			--link2symlink)
				shift
				_opt_is_link2symlink=1
				;;
			--no-link2symlink)
				shift
				_opt_is_link2symlink=0
				;;
			--no-sysvipc)
				shift
				_opt_is_sysvipc=0
				;;
			--fix-low-ports)
				shift
				_opt_is_fix_low_ports=1
				;;
			-q | --qemu=*)
				case "$1" in
				-q) shift && _opt_qemu="$1" ;;
				*) _opt_qemu="$(get_longopt_val "$1")" ;;
				esac
				shift
				;;
			-k | --kernel-release=*)
				case "$1" in
				-k) shift && _opt_kernel_release="$1" ;;
				*) _opt_kernel_release="$(get_longopt_val "$1")" ;;
				esac
				shift
				;;
			-*) error_exit_unknown_option "$1" ;;
			*)
				if ! test "${_opt_rootfs_dir+1}"; then
					_opt_rootfs_dir="$1"
				else
					error_exit "Excessive argument 'env'."
				fi
				shift
				;;
			esac
		done
	else
		_show_help
		exit 1
	fi

	if test "${_opt_rootfs_dir+1}"; then
		if ! test -d "${_opt_rootfs_dir}"; then
			error_exit "Rootfs '${_opt_rootfs_dir}' not exists."
		fi
	else
		error_exit "Rootfs not set"
	fi

	set_proot_path
	set_proot_env "${_opt_rootfs_dir}"

	set -- "$@" "--rootfs=${_opt_rootfs_dir}"

	if test "${_opt_qemu+1}"; then
		set -- "$@" "--qemu=${_opt_qemu}"
	fi

	if test "${_opt_cwd+1}"; then
		set -- "$@" "--cwd=${_opt_cwd}"
	else
		set -- "$@" "--cwd=/root"
	fi

	if ${_opt_is_kill_on_exit}; then
		set -- "$@" "--kill-on-exit"
	fi

	if test "${_opt_kernel_release+1}"; then
		set -- "$@" "--kernel-release=${_opt_kernel_release}"
	fi

	set -- "$@" "--root-id"

	if ${_opt_is_link2symlink}; then
		set -- "$@" "--link2symlink"
	fi

	if is_android; then
		if ${_opt_is_sysvipc}; then
			set -- "$@" "--sysvipc"
		fi
	fi

	if is_android; then
		set -- "$@" "--ashmem-memfd"
	fi

	if is_android; then
		set -- "$@" "-H"
	fi

	if is_android; then
		if ${_opt_is_fix_low_ports}; then
			set -- "$@" "-P"
		fi
	fi

	if is_android; then
		set -- "$@" "-L"
	fi

	## ========== Bindings ==========
	## Core file systems that should always be present.
	set -- "$@" "--bind=/dev"
	set -- "$@" "--bind=/dev/urandom:/dev/random"
	set -- "$@" "--bind=/proc"
	set -- "$@" "--bind=/proc/self/fd:/dev/fd"
	if test -t 0; then
		set -- "$@" "--bind=/proc/self/fd/0:/dev/stdin"
	fi
	if test -t 1; then
		set -- "$@" "--bind=/proc/self/fd/1:/dev/stdout"
	fi
	if test -t 2; then
		set -- "$@" "--bind=/proc/self/fd/2:/dev/stderr"
	fi
	set -- "$@" "--bind=/sys"

	## Map static fake rootfs files
	proot_fakerootfs_dir=$(get_proot_conf "${_opt_rootfs_dir}" proot_fakerootfs_dir)
	prefix=$(echo "${proot_fakerootfs_dir}" | wc -m)
	for f in $(find "${proot_fakerootfs_dir}" -type f | cut -b"${prefix}"-); do
		case "${f}" in
		/proc/*)
			if ! cat "${f}" >/dev/null 2>&1; then
				set -- "$@" "--bind=${proot_fakerootfs_dir}${f}:${f}"
			fi
			;;
		/etc/profile.d/host_utils.sh)
			if ${_opt_is_host_utils}; then
				set -- "$@" "--bind=${proot_fakerootfs_dir}${f}:${f}"
			fi
			;;
		*)
			set -- "$@" "--bind=${proot_fakerootfs_dir}${f}:${f}"
			;;
		esac
	done

	## Bind /tmp to /dev/shm.
	if ! test -d "${_opt_rootfs_dir}/tmp"; then
		mkdir -p "${_opt_rootfs_dir}/tmp"
		chmod 1777 "${_opt_rootfs_dir}/tmp"
	fi
	set -- "$@" "--bind=${_opt_rootfs_dir}/tmp:/dev/shm"

	if ${_opt_is_host_utils}; then
		if is_android; then
			## For anotherterm termsh
			if test -d "/system"; then
				set -- "$@" "--bind=/system"
			fi
			if test -d "/apex"; then
				set -- "$@" "--bind=/apex"
			fi
			if test -e "/linkerconfig/ld.config.txt"; then
				set -- "$@" "--bind=/linkerconfig/ld.config.txt"
			fi

			## https://green-green-avk.github.io/AnotherTerm-docs/local-shell-utility.html
			if is_anotherterm; then
				export TERMSH_UID="${USER_ID:-$(id -u)}"
				set -- "$@" "--bind=${LIB_DIR}/libtermsh.so"
			fi

			## For termux-x11
			## https://github.com/termux/termux-x11
			test -d "/vendor" && set -- "$@" "--bind=/vendor"
			set -- "$@" "--bind=/data/app"
			if is_anotherterm; then
				set -- "$@" "--bind=${DATA_DIR}"
			fi

			## For termux
			if is_termux; then
				set -- "$@" "--bind=/data/dalvik-cache"
				# set -- "$@" "--bind=/data/data/com.termux/cache"
				# set -- "$@" "--bind=/data/data/com.termux/files/usr"
				if test -d "/data/data/com.termux/files/apps"; then
					set -- "$@" "--bind=/data/data/com.termux/files/apps"
				fi

				set -- "$@" "--bind=${PREFIX}"
			fi
		fi
	fi

	## Bindings from command options
	if test "${_opt_bindings+1}"; then
		for val in ${_opt_bindings}; do
			set -- "$@" "--bind=$(eval "printf '%s' ${val}")"
		done
	fi

	## ========== Program to exec ==========
	## Setup the default environment
	if stat "${_opt_rootfs_dir}/usr/bin/env" >/dev/null 2>&1; then
		set -- "$@" "/usr/bin/env" "-i" \
			"HOME=/root" \
			"LANG=C.UTF-8" \
			"TERM=${TERM-xterm-256color}"

		if test "${COLORTERM+1}"; then
			set -- "$@" "COLORTERM=${COLORTERM}"
		fi

		if ${_opt_is_host_utils} && is_anotherterm; then
			set -- "$@" "SHELL_SESSION_TOKEN=${SHELL_SESSION_TOKEN}"
		fi

		if test "${opt_envs+1}"; then
			for line in ${opt_envs}; do
				if test -n "${line}"; then
					set -- "$@" "$(echo "${line}" | base64 -d)"
				fi
			done
		fi
	fi

	## Start user command or default shell
	if test "${args+1}"; then
		for l in ${args}; do
			set -- "$@" "$(echo $l | base64 -d)"
		done
	else
		## If no argument command, login to root default shell
		if test -e "${_opt_rootfs_dir}/etc/passwd"; then
			set -- "$@" "$(grep -e "^root" "${_opt_rootfs_dir}/etc/passwd" | cut -d: -f7)" -l
		else
			set -- "$@" /bin/sh -l
		fi
	fi

	## ========== Starting proot ==========
	vmsg "cmd=${PROOT} $*"
	exec "${PROOT}" "$@"
}

command_archive() {
	_show_help() {
		msg "\
Archive the specific rootfs to stdout.

Usage:
  ${PROGRAM} ${COMMAND} [ROOTFS]

Options:
  --help              show this help
	--rootfs-tar				use tar from rootfs

Tar relavent options:
  -v, --verbose
  --exclude\
"
	}

	_opt_tar_is_verbose=0
	_opt_is_rootfs_tar=0

	if test $# -gt 0; then
		while test $# -gt 0; do
			case "$1" in
			-h | --help)
				_show_help
				exit
				;;
			-v | --verbose)
				shift
				_opt_tar_is_verbose=1
				;;
			--rootfs-tar)
				shift
				_opt_is_rootfs_tar=1
				;;
			--exclude=*)
				if test "${tar_excludes+1}"; then
					tar_excludes=$(printf "%s\n%s\n" "${tar_excludes}" "$(echo "$1" | base64)")
				else
					tar_excludes=$(echo "$1" | base64)
				fi
				shift
				;;
			-*) error_exit_unknown_option "$1" ;;
			*)
				if ! test "${_opt_rootfs_dir+1}"; then
					if test -d "$1"; then
						_opt_rootfs_dir="$1"
					else
						error_exit "Rootfs '$1' not exists."
					fi
					shift
				else
					error_exit_argument_error "$@"
				fi
				;;
			esac
		done
	else
		_show_help
		exit 1
	fi

	if ! test ${_opt_rootfs_dir+1}; then
		error_exit "rootfs not set"
	fi

	if test -t 1; then
		error_exit "Refusing to write archive contents to terminal"
	fi

	if ${_opt_is_rootfs_tar}; then
		set -- "$@" "--rootfs=${_opt_rootfs_dir}"
		set -- "$@" "--root-id"
		set -- "$@" "--cwd=/"
		set -- "$@" "/bin/tar"
	else
		set -- "$@" "--root-id"
		set -- "$@" "--cwd=${_opt_rootfs_dir}"
		set -- "$@" "tar"
	fi

	if ${_opt_tar_is_verbose}; then
		set -- "$@" "-v"
	fi

	set -- "$@" "--exclude=./tmp/*"
	set -- "$@" "--exclude=.*sh_history"
	set -- "$@" "--exclude=$(get_proot_conf . proot_data_dir)"
	set -- "$@" "--exclude=./etc/profile.d/proot.sh"
	set -- "$@" "-c" "."

	if test "${tar_excludes+1}"; then
		# shellcheck disable=SC2046
		set -- "$@" $(echo "${tar_excludes}" | base64 -d)
	fi

	set_proot_path
	unset LD_PRELOAD
	vmsg "cmd=${PROOT} $*"
	exec "${PROOT}" "$@"
}

main() {
	_show_help() {
		msg "\
Supercharges your PRoot experience.

Usage:
  ${PROGRAM} [OPTION...] [COMMAND]

  ## show help of a command
  ${PROGRAM} [COMMAND] --help

Options:
  -h, --help          show this help
  -v, --verbose       print more information

Commands:
  install             install rootfs
  login               login rootfs
  archive             archive rootfs

Related environment variables:
  PROOT               path to proot\
"
	}

	link2symlink_default=0
	if is_android; then
		link2symlink_default=1
	fi

	if test $# -eq 0; then
		_show_help
	else
		script_opt_is_verbose() { false; }

		while test $# -gt 0; do
			case "$1" in
			-v | --verbose)
				shift
				script_opt_is_verbose() { true; }
				;;
			--help)
				_show_help
				exit
				;;
			-*) error_exit_unknown_option "$1" ;;
			install | login | archive)
				COMMAND="$1"
				case "$1" in
				install) shift && command_install "$@" ;;
				login) shift && command_login "$@" ;;
				archive) shift && command_archive "$@" ;;
				esac
				break
				;;
			*)
				error_exit "Unknown command '$1'"
				;;
			esac
		done
	fi
}

main "$@"
