## THIS SCRIPT IS WRITTEN IN POSIX SHELL
set -eu

SCRIPT_NAME="$(basename "$0")"
SCRIPT_PID="$$"
ANDROID10ESSENTIALS_PLUGIN_PACKAGE="green_green_avk.anothertermshellplugin_android10essentials"

msg() { printf "%s\n" "$*" >&2; }
info() { printf "%s\n" "${SCRIPT_NAME+${SCRIPT_NAME}:}$*" >&2; }
err() { info "ERROR: $*" && exit 1; }
err_no_args() { err "no arguments"; }
err_args() { err "argument error: [$*]"; }
vmsg() { if script_opt_is_verbose; then msg "${SCRIPT_NAME}:${FUNC_NAME}:$*"; fi; }

test_is_android() { test -f /system/bin/linker; }
test_is_termux() { test "${TERMUX_VERSION+1}"; }
test_is_anotherterm() { echo "${APP_ID-}" | grep -q "anotherterm"; }
test_anotherterm_android10essentials_plugin() { test_is_anotherterm && "${LIB_DIR}/libtermsh.so" plugin -h "${ANDROID10ESSENTIALS_PLUGIN_PACKAGE}" >/dev/null 2>&1; }

## [rootfs_dir] [item]
get_proot_conf() {
	case "$2" in
	proot_data_dir) echo "$1/.proot" ;;
	proot_l2s_dir) echo "$(get_proot_conf "$(realpath "$1")" proot_data_dir)/meta" ;;
	proot_fake_rootfs_dir) echo "$(get_proot_conf "$1" proot_data_dir)/rootfs" ;;
	proot_sessions_dir) echo "$(get_proot_conf "$1" proot_data_dir)/sessions" ;;
	proot_tmp_dir) echo "$(get_proot_conf "$1" proot_sessions_dir)/${SCRIPT_PID}" ;;
	esac
}

set_proot_path() {
	if ! test ${PROOT+1}; then
		if _val=$(command -v proot >&1); then
			PROOT="${_val}"
		elif test_is_anotherterm && test_anotherterm_android10essentials_plugin; then
			PROOT="$("${LIB_DIR}/libtermsh.so" plugin "${ANDROID10ESSENTIALS_PLUGIN_PACKAGE}" proot)"
		else
			err "proot not found"
		fi
	fi
}

## [rootfs_dir]
set_proot_env() {
	unset LD_PRELOAD
	PROOT_TMP_DIR="$(get_proot_conf "$1" proot_tmp_dir)"
	if ! test -d "${PROOT_TMP_DIR}"; then
		mkdir -p "${PROOT_TMP_DIR}"
	fi
	export PROOT_TMP_DIR

	PROOT_L2S_DIR="$(get_proot_conf "$1" proot_l2s_dir)"
	if ! test -d "${PROOT_L2S_DIR}"; then
		mkdir -p "${PROOT_L2S_DIR}"
	fi
	export PROOT_L2S_DIR

	vmsg "env=LD_PRELOAD= PROOT_TMP_DIR=\"${PROOT_TMP_DIR}\" PROOT_L2S_DIR=\"${PROOT_L2S_DIR}\""
}

## [rootfs_dir]
clean_proot_sessions() {
	PROOT_SESSIONS_DIR=$(get_proot_conf "$1" proot_sessions_dir)
	if test -d "${PROOT_SESSIONS_DIR}"; then
		find "${PROOT_SESSIONS_DIR}" -maxdepth 1 -mindepth 1 -type d | while IFS= read -r dir; do
			if ! kill -0 "$(basename "${dir}")" 2>/dev/null; then
				rm -rf "${dir}"
			fi
		done
	fi
}

command_install() {
	_show_help() {
		msg "\
Install tar archived rootfs from stdin.

Usage:
  ${SCRIPT_NAME} ${FUNC_NAME} [OPTION...] [ROOTFS_DIR]

Options:
  --help              show this help
  -v, -vv             tar verbose\
"
	}

	if test $# -eq 0; then
		err_no_args
	else
		while test $# -gt 0; do
			case "$1" in
			--help)
				shift
				_show_help
				exit
				;;
			-v | -vv)
				_opt_tar_verbose="$1"
				shift
				;;
			*)
				if ! test "${_opt_rootfs_dir+1}"; then
					if ! test -e "$1"; then
						_opt_rootfs_dir="$1"
						shift
					else
						err "file [$1] exists!"
					fi
				else
					err_args "$@"
				fi
				;;
			esac
		done

		if test -t 0; then
			err "No data from stdin"
		fi

		on_error() {
			rm -rf "${_opt_rootfs_dir}"
			msg "Exiting due to failure."
			exit 1
		}

		on_interrupt() {
			trap - EXIT
			rm -rf "${_opt_rootfs_dir}"
			msg "\rExit as requested!"
			exit 1
		}

		trap "on_error" EXIT
		trap "on_interrupt" HUP INT TERM
		info "Installing to [${_opt_rootfs_dir}] ..."

		mkdir -p "${_opt_rootfs_dir}"
		set_proot_path
		set_proot_env "${_opt_rootfs_dir}"
		set -- --link2symlink --root-id tar -C "${_opt_rootfs_dir}" -x ${_opt_tar_verbose+${_opt_tar_verbose}}
		vmsg "cmd=${PROOT} $*"
		"${PROOT}" "$@"
		clean_proot_sessions "${_opt_rootfs_dir}"

		_create_file() {
			mkdir -p "$(dirname "$1")"
			vmsg "Create $1"
			cat >"$1"
		}

		## Make fake files
		proot_fake_rootfs_dir="$(get_proot_conf "${_opt_rootfs_dir}" proot_fake_rootfs_dir)"

		_create_file "${proot_fake_rootfs_dir}/proc/loadavg" <<-EOF
			0.77 0.18 0.06 5/136 903
		EOF

		base64 -d <<-EOF | gzip -d | _create_file "${proot_fake_rootfs_dir}/proc/stat"
			H4sIAAAAAAAAA+2S3QrCMAyF7/sU5xGaLP3Z0wirVYrazbUDH9/O7UoErwU5kOSEJDf5wrQAIhYa
			1BsGaQ3u/Gq5hSYVpkV/GUm5ziDpyUE0DG3tN5HxW7a79x1YWiHeMth8XPrrh6VCfdSGjjaihppu
			sf2+156JyappHkMsJRZ4cS9XDvOSc8pnmN0P1zFc4rFdKuOppvmOjsg4dLDaMSyswDXusLK4wUjO
			CaknkgDXlNcCAAA=
		EOF

		_create_file "${proot_fake_rootfs_dir}/proc/uptime" <<-EOF
			27.95 1.00
		EOF

		_create_file "${proot_fake_rootfs_dir}/proc/version" <<-EOF
			Linux version 6.1.0-13-faked (builder@server) \
			(gcc-12 (12.2.0-14) 12.2.0, GNU ld (GNU Binutils) 2.40) \
			#1 SMP PREEMPT_DYNAMIC Debian 6.1.55-1 (2023-09-29)
		EOF

		base64 -d <<-EOF | gzip -d | _create_file "${proot_fake_rootfs_dir}/proc/vmstat"
			H4sIAAAAAAAAA3VWWZqzKhB971W4hDhE42r4iBLlBsELmP66V/9XFTjbb9Y51EBRg9qylxWCjbwT
			LknzsqqqL23Zr9GCSc0bLz+CcW10khb3/LFwWyZL72edl1QiKe5ldTvqEJMWxaNemEmLj2w8fwJT
			1nm5EN9WeohO6FbqLnmkGTKDMs17Ofc0k25EEty4cBES6GLNwFGaBs566ZM8uz/SIsiDdG7mXsYK
			2elZlNoLqwT/iCQr8yyA4JWrnQXje2GDsz9ydZGmPzJ0mZyrvDjFn8yKRnE5BKK43RZi0luqyguy
			I51R3Is2BHLbQeTx9vVt7Bty7IRn2rSUww1kxYtPys/6F8zZDN0IXFwoLdRZywrn4TEuPQXmOuB4
			7XA5VJ5ruk5LytvAx1G0SfpIH/Q0aCaeKbOyThFrpfU/c5lR7T05lNptJzIvhjFgrh/EACVSLgLr
			p05sijCA49BG97fF9eFgCGd/jq7hLddQ1lZof9T5DK7hOkS2Q+QwiFZigndpwetJzEEZUoCKXujk
			UVFlvoXVQmFfNLsCC/EZpdgoNbzd/5O0y1Vm2AroFzfD0ZTzmL4srSihGDmjYo55L2rC3TcfG970
			szK9AvM9PHhvVJvk97q+rwS+QWeh69vNmbSq4RJjN3YSOq2us+xGkpl8UhRp+TW6bwgTHOAHoshz
			BU3NIOfQ1lsxz5I8z+/ZYwG1sQN0/6o0mE9MDslwU0hES9NmD4CtHbRY2mCrsbFzbzlGQ6tARqK4
			CYXkrTLOvKSoHre8wkhjm8Fgv6UItGKB8LTivz+kQcmiPoZ+KQsYGmM38P8ikj62h1tShbaXSsXP
			yYkkL+sa1ZwXXLE3vmk4GQB4OtGEpLdiMFCZmyMR2ZyhKt4aQfnIBxmLwHg/Zw+JODuiFOfFHMlK
			khhZ2jax6NmLAxgcSxqGeBJEnLCOoVFNdAiQ7c5ETJlv9j1w+8a1w6Bjmrf6Weledv0lj40B1Wmn
			ECPcDIc0fLfWjNQ21CkzgBHBtzEDe4f3oMU0QjansQXVZcPh6LjEYdcxeugrKC692Sx2LcyHzsag
			oEqCwNzUNIL26YphGgHw/Rh1JIyz9eAevz48KokP3phhhMpli7flCWaGlv0ZnrfcBqKO22oGxwu9
			xDcjLYfyhCnL3+KM/h1RPHAIrPfqyZ5T2/6wMEdWfycqRrb5CWBQtayZFFZnnadldiJXT0cGJmUz
			AVOdGPqfAqasi/xMTjrSZ8UGRr69dOZwa7VE4YvGnwe81Q55ARQ37BlkTc9tJyLXwKLhoxM7K3tw
			7Vsyhgt15xKBo8cttndI1LKNEaFqpCY4AXvXrXgJC4n5U2M4mJxm+VdYEwxuIz+ge2fLOluF7S2f
			qAJ1KPVLhbk/IxDlAYnFjJMOphSzfP2iv2cYOmHkQlqYEh+hsnADl8CqPHL5zN3Cr6zzcU39A6tn
			Lxp1DAAA
		EOF

		_create_file "${proot_fake_rootfs_dir}/proc/sys/kernel/cap_last_cap" <<-EOF
			40
		EOF

		_create_file "${proot_fake_rootfs_dir}/proc/sys/fs/inotify/max_queued_events" <<-EOF
			16384
		EOF

		_create_file "${proot_fake_rootfs_dir}/proc/sys/fs/inotify/max_user_instances" <<-EOF
			128
		EOF

		_create_file "${proot_fake_rootfs_dir}/proc/sys/fs/inotify/max_user_watches" <<-EOF
			65536
		EOF

		_create_file "${proot_fake_rootfs_dir}/etc/resolv.conf" <<-EOF
			nameserver 8.8.8.8
			nameserver 8.8.4.4
		EOF

		_create_file "${proot_fake_rootfs_dir}/etc/hosts" <<-EOF
			127.0.0.1       localhost localhost.localdomain_func
			::1             localhost localhost.localdomain_func
		EOF

		_create_file "${proot_fake_rootfs_dir}/etc/profile.d/locale.sh" <<-EOF
			export CHARSET=UTF-8
			export LANG=C.UTF-8
			export LC_COLLATE=C
		EOF

		base64 -d <<-EOF | _create_file "${proot_fake_rootfs_dir}/etc/profile.d/shell.sh"
			aWYgdGVzdCAiJHtTSEVMTH0iID0gIi9iaW4vYmFzaCI7IHRoZW4KICAgIFBTMT0nXFtcZVszMm1c
			XVx1XFtcZVszM21cXUBcW1xlWzMybVxdXGhcW1xlWzMzbVxdOlxbXGVbMzJtXF1cd1xbXGVbMzNt
			XF1cJFxbXGVbMG1cXSAnCiAgICBQUzI9J1xbXGVbMzNtXF0+XFtcZVswbVxdICcKZmkK
		EOF

		_create_file "${proot_fake_rootfs_dir}/etc/profile.d/proot.sh" <<-EOF
			export COLORTERM=truecolor
			[ -z "\$LANG" ] && export LANG=C.UTF-8
			export TERM=xterm-256color
			export TMPDIR=/tmp
			export PULSE_SERVER=127.0.0.1
			export MOZ_FAKE_NO_SANDBOX=1
			export CHROMIUM_FLAGS="--no-sandbox"
		EOF

		if test_is_android; then
			_create_file "${proot_fake_rootfs_dir}/etc/profile.d/host_utils.sh" <<-EOF
				## prootie host utils
				export ANDROID_DATA='${ANDROID_DATA}'
				export ANDROID_RUNTIME_ROOT='${ANDROID_RUNTIME_ROOT}'
				export ANDROID_TZDATA_ROOT='${ANDROID_TZDATA_ROOT}'
				export BOOTCLASSPATH='${BOOTCLASSPATH}'
			EOF

			if test_is_anotherterm; then
				cat <<-EOF >>"${proot_fake_rootfs_dir}/etc/profile.d/host_utils.sh"
					export DATA_DIR='$(realpath "${DATA_DIR}")'
					export TERMSH_UID='${USER_ID-$(id -u)}'
					export TERMSH='${LIB_DIR}/libtermsh.so'
				EOF
			fi

			if test "${PREFIX+1}"; then
				cat <<-EOF >>"${proot_fake_rootfs_dir}/etc/profile.d/host_utils.sh"
					export PATH="\${PATH}:${PREFIX}/bin"
				EOF
			fi
		fi

		## Reset trap for HUP/INT/TERM.
		trap - EXIT
		info "Installing rootfs to [${_opt_rootfs_dir}] done."
		info "To login, run '${SCRIPT_NAME} login ${_opt_rootfs_dir}'."
	fi
}

command_login() {
	_show_help() {
		msg "\
Login into an installed rootfs.

Usage:
  ## Login default shell as root
  ${SCRIPT_NAME} ${FUNC_NAME} [OPTION...] [ROOTFS_DIR]
  
  ## Login and run commands
  ${SCRIPT_NAME} ${FUNC_NAME} [OPTION...] [ROOTFS_DIR] -- [COMMAND] ...

Options:
  --help              show this help
  --host-utils        enhances anotherterm & termux
  --                  run commands within rootfs
  
PRoot relavent options:
  -b, --bind, -m, --mount
  --no-kill-on-exit
  --no-link2symlink
  --no-sysvipc
  --fix-low-ports
  -q, --qemu
  -k, --kernel-release\
"
	}

	## Default option value
	_opt_is_host_utils() { false; }

	## PRoot relavent options
	_opt_is_kill_on_exit() { true; }
	_opt_is_link2symlink() { true; }
	_opt_is_sysvipc() { true; }
	_opt_is_fix_low_ports() { false; }

	get_longopt_val() { echo "$1" | cut -d= -f2-; }

	if test $# -gt 0; then
		while test $# -gt 0; do
			case "$1" in
			--)
				shift
				if test $# -gt 0; then
					_opt_args_base64="$(
						while test $# -gt 0; do
							printf "%s\n" "$(echo "$1" | base64 -w1024000000)"
							shift
						done
					)"
				else
					err_no_arg "--"
				fi
				shift $#
				;;
			-h | --help)
				_show_help
				exit
				;;
			-b | --bind=* | -m | --mount=*)
				case "$1" in
				-b | -m)
					shift
					_opt_val="$1"
					;;
				*)
					_opt_val="$(get_longopt_val "$1")"
					;;
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
				_opt_is_host_utils() { true; }
				;;
			--no-kill-on-exit)
				shift
				_opt_is_kill_on_exit() { false; }
				;;
			--no-link2symlink)
				shift
				_opt_is_link2symlink() { false; }
				;;
			--no-sysvipc)
				shift
				_opt_is_sysvipc() { false; }
				;;
			--fix-low-ports)
				shift
				_opt_is_fix_low_ports() { true; }
				;;
			-q | --qemu=*)
				case "$1" in
				-q)
					shift
					_opt_qemu="$1"
					;;
				*)
					_opt_qemu="$(get_longopt_val "$1")"
					;;
				esac
				shift
				;;
			-k | --kernel-release=*)
				case "$1" in
				-k)
					_opt_kernel_release="$1"
					info 'kernel called'
					shift
					;;
				*)
					_val="$(get_longopt_val "$1")"
					if test -n "${_val}"; then
						_opt_kernel_release="${_val}"
					else
						err_args "$1"
					fi
					;;
				esac
				shift
				;;
			*)
				if ! test "${_opt_rootfs_dir+1}"; then
					if test -d "$1"; then
						_opt_rootfs_dir="$1"
					else
						err "dir [$1] not exists"
					fi
				else
					err_args "$@"
					break
				fi
				shift
				;;
			esac
		done
	else
		err_no_args
	fi

	if ! test -d "${_opt_rootfs_dir}"; then
		err "rootfs dir not set"
	fi

	clean_proot_sessions "${_opt_rootfs_dir}"
	set_proot_path
	set_proot_env "${_opt_rootfs_dir}"

	## ========== Options from proot ==========
	## Regular options:
	##   -r *path*, --rootfs=*path*
	##         Use *path* as the new guest root file-system, default is /.

	##         The specified path typically contains a Linux distribution where
	##         all new programs will be confined.  The default rootfs is /
	##         when none is specified, this makes sense when the bind mechanism
	##         is used to relocate host files and directories, see the -b
	##         option and the Examples section for details.

	##         It is recommended to use the -R or -S options instead.

	set -- "$@" "--rootfs=${_opt_rootfs_dir}"

	##   -b *path*, --bind=*path*, -m *path*, --mount=*path*
	##         Make the content of *path* accessible in the guest rootfs.

	##         This option makes any file or directory of the host rootfs
	##         accessible in the confined environment just as if it were part of
	##         the guest rootfs.  By default the host path is bound to the same
	##         path in the guest rootfs but users can specify any other location
	##         with the syntax: -b *host_path*:*guest_location*.  If the
	##         guest location is a symbolic link, it is dereferenced to ensure
	##         the new content is accessible through all the symbolic links that
	##         point to the overlaid content.  In most cases this default
	##         behavior shouldn't be a problem, although it is possible to
	##         explicitly not dereference the guest location by appending it the
	##         ! character: -b *host_path*:*guest_location!*.

	##   -q *command*, --qemu=*command*
	##         Execute guest programs through QEMU as specified by *command*.

	##         Each time a guest program is going to be executed, PRoot inserts
	##         the QEMU user-mode command in front of the initial request.
	##         That way, guest programs actually run on a virtual guest CPU
	##         emulated by QEMU user-mode.  The native execution of host programs
	##         is still effective and the whole host rootfs is bound to
	##         /host-rootfs in the guest environment.

	if test "${_opt_qemu+1}"; then
		set -- "$@" "--qemu=${_opt_qemu}"
	fi

	##   -w *path*, --pwd=*path*, --cwd=*path*
	##         Set the initial working directory to *path*.

	##         Some programs expect to be launched from a given directory but do
	##         not perform any chdir by themselves.  This option avoids the
	##         need for running a shell and then entering the directory manually.

	if test "${_opt_cwd+1}"; then
		set -- "$@" "--cwd=${_opt_cwd}"
	else
		set -- "$@" "--cwd=/root"
	fi

	##   --kill-on-exit
	##         Kill all processes on command exit.

	##         When the executed command leaves orphean or detached processes
	##         around, proot waits until all processes possibly terminate. This option forces
	##         the immediate termination of all tracee processes when the main_func command exits.

	if _opt_is_kill_on_exit; then
		set -- "$@" "--kill-on-exit"
	fi

	##   -v *value*, --verbose=*value*
	##         Set the level of debug information to *value*.

	##         The higher the integer value is, the more detailed debug
	##         information is printed to the standard error stream.  A negative
	##         value makes PRoot quiet except on fatal errors.

	##   -V, --version, --about
	##         Print version, copyright, license and contact, then exit.

	##   -h, --help, --usage
	##         Print the version and the command-line usage, then exit.

	## Extension options:
	##   -k *string*, --kernel-release=*string*
	##         Make current kernel appear as kernel release *string*.

	##         If a program is run on a kernel older than the one expected by its
	##         GNU C library, the following error is reported: "FATAL: kernel too
	##         old".  To be able to run such programs, PRoot can emulate some of
	##         the features that are available in the kernel release specified by
	##         *string* but that are missing in the current kernel.

	if test "${_opt_kernel_release+1}"; then
		set -- "$@" "--kernel-release=${_opt_kernel_release}"
	fi

	##   -0, --root-id
	##         Make current user appear as "root" and fake its privileges.

	##         Some programs will refuse to work if they are not run with "root"
	##         privileges, even if there is no technical reason for that.  This
	##        is typically the case with package managers.  This option allows
	##         users to bypass this kind of limitation by faking the user/group
	##         identity, and by faking the success of some operations like
	##         changing the ownership of files, changing the root directory to
	##         /, ...  Note that this option is quite limited compared to
	##         fakeroot.

	set -- "$@" "--root-id"

	##   -i *string*, --change-id=*string*
	##         Make current user and group appear as *string* "uid:gid".

	##         This option makes the current user and group appear as uid and
	##         gid.  Likewise, files actually owned by the current user and
	##         group appear as if they were owned by uid and gid instead.
	##         Note that the -0 option is the same as -i 0:0.

	##   --link2symlink, -l
	##         Replace hard links with symlinks, pretending they are really hardlinks

	##         Emulates hard links with symbolic links when SELinux policies
	##         do not allow hard links.

	if _opt_is_link2symlink; then
		set -- "$@" "--link2symlink"
	fi

	##   --sysvipc
	##         Handle System V IPC syscalls in proot

	##         Handles System V IPC syscalls (shmget, semget, infoget, etc.)
	##         syscalls inside proot. IPC is handled inside proot and launching 2 proot instances
	##         will lead to 2 different IPC Namespaces

	if test_is_android; then
		if _opt_is_sysvipc; then
			set -- "$@" "--sysvipc"
		fi
	fi

	##   --ashmem-memfd
	##         Emulate memfd_create support through ashmem and simulate fstat.st_size for ashmem

	if test_is_android; then
		set -- "$@" "--ashmem-memfd"
	fi

	##   -H
	##         Hide files and directories starting with '.proot.' .

	if test_is_android; then
		set -- "$@" "-H"
	fi

	##   -p
	##         Modify bindings to protected ports to use a higher port number.

	if test_is_android; then
		if _opt_is_fix_low_ports; then
			set -- "$@" "-P"
		fi
	fi

	##   -L
	##         Correct the size returned from lstat for symbolic links.

	if test_is_android; then
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
	set -- "$@" "--bind=/proc/self/fd/1:/dev/stdout"
	set -- "$@" "--bind=/proc/self/fd/2:/dev/stderr"
	set -- "$@" "--bind=/sys"

	## Map static fake rootfs files
	PROOT_FAKE_ROOTFS_DIR=$(get_proot_conf "${_opt_rootfs_dir}" proot_fake_rootfs_dir)
	for f in $(cd "${PROOT_FAKE_ROOTFS_DIR}" && find . -type f -exec sh -c 'printf "%s" "$1"|cut -b 2-' _ {} \;); do
		case "${f}" in
		/etc/profile.d/host_utils.sh)
			if _opt_is_host_utils; then
				set -- "$@" "--bind=${PROOT_FAKE_ROOTFS_DIR}${f}:${f}"
			fi
			;;
		/etc/*)
			if ! cat "${_opt_rootfs_dir}${f}" >/dev/null 2>&1; then
				if cat "${f}" >/dev/null 2>&1; then
					set -- "$@" "--bind=${f}"
				else
					set -- "$@" "--bind=${PROOT_FAKE_ROOTFS_DIR}${f}:${f}"
				fi
			fi
			;;
		/proc/*)
			if ! cat "${f}" >/dev/null 2>&1; then
				set -- "$@" "--bind=${PROOT_FAKE_ROOTFS_DIR}${f}:${f}"
			fi
			;;
		esac
	done

	## Bind /tmp to /dev/shm.
	if ! test -d "${_opt_rootfs_dir}/tmp"; then
		mkdir -p "${_opt_rootfs_dir}/tmp"
		chmod 1777 "${_opt_rootfs_dir}/tmp"
	fi
	set -- "$@" "--bind=${_opt_rootfs_dir}/tmp:/dev/shm"

	if _opt_is_host_utils; then
		if test_is_android; then
			## For anotherterm termsh
			test -d "/system" && set -- "$@" "--bind=/system"
			test -d "/apex" && set -- "$@" "--bind=/apex"
			## https://green-green-avk.github.io/AnotherTerm-docs/local-shell-utility.html
			if test_is_anotherterm; then
				export TERMSH_UID="${USER_ID:-$(id -u)}"
				set -- "$@" "--bind=${LIB_DIR}/libtermsh.so"
			fi

			## For termux-x11
			## https://github.com/termux/termux-x11
			test -d "/vendor" && set -- "$@" "--bind=/vendor"
			set -- "$@" "--bind=/data/app"
			if test_is_anotherterm; then
				set -- "$@" "--bind=${DATA_DIR}"
			fi

			## For termux
			if test_is_termux; then
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
	fi

	## Start user command or default shell
	if test "${_opt_args_base64+1}"; then
		for line in ${_opt_args_base64}; do
			set -- "$@" "$(echo "$line" | base64 -d)"
		done
	else
		## If no argument command, login to root default shell
		set -- "$@" "$(grep -e "^root" "${_opt_rootfs_dir}/etc/passwd" | cut -d: -f7)" -l
	fi

	## ========== Starting proot ==========
	vmsg "cmd=${PROOT} $*"
	exec "${PROOT}" "$@"
}

command_archive() {
	_show_help() {
		msg "\
Archive a rootfs to stdout.

Usage:
  ${SCRIPT_NAME} ${FUNC_NAME} [ROOTFS_DIR]

Options:
  --help              show this help
  -v, -vv             tar verbose\
"
	}

	if test $# -gt 0; then
		while test $# -gt 0; do
			case "$1" in
			-h | --help)
				_show_help
				exit
				;;
			-v | -vv)
				_opt_tar_verbose="$1"
				shift
				;;
			*)
				if ! test "${_opt_rootfs_dir+1}"; then
					if test -d "$1"; then
						_opt_rootfs_dir="$1"
					else
						err "Rootfs dir [$1] not exists"
					fi
					shift
				else
					err_args "$@"
				fi
				;;
			esac
		done
	else
		err_no_args
	fi

	if ! test ${_opt_rootfs_dir+1}; then
		err "rootfs not set"
	fi

	## Do not write to terminal
	if test -t 1; then
		err "Do not write to terminal"
	fi

	## Do not archive when rootfs is in use.
	PROOT_SESSIONS_DIR=$(get_proot_conf "${_opt_rootfs_dir}" proot_sessions_dir)
	if test -d "${PROOT_SESSIONS_DIR}"; then
		find "${PROOT_SESSIONS_DIR}" -maxdepth 1 -mindepth 1 -type d | while IFS= read -r dir; do
			if kill -0 "$(basename "${dir}")" 2>/dev/null; then
				err "rootfs dir [${_opt_rootfs_dir}] is in use"
			fi
		done
	fi

	set -- "$@" "--rootfs=${_opt_rootfs_dir}"
	set -- "$@" "--root-id"
	set -- "$@" "--link2symlink"
	set -- "$@" "--cwd=/"
	set -- "$@" "/bin/tar"

	if test "${_opt_tar_verbose+1}"; then
		set -- "$@" "${_opt_tar_verbose}"
	fi

	set -- "$@" "--exclude=./tmp/*"
	set -- "$@" "--exclude=.*sh_history"
	set -- "$@" "--exclude=$(get_proot_conf . proot_data_dir)"
	set -- "$@" "--exclude=./etc/profile.d/proot.sh"
	set -- "$@" "-c" "."

	info "Archiving [${_opt_rootfs_dir}] ..."
	set_proot_path
	vmsg "cmd=${PROOT} $*"
	unset LD_PRELOAD
	"${PROOT}" "$@"
	info "Archiving [${_opt_rootfs_dir}] done."
}

main() {
	_show_help() {
		msg "\
${SCRIPT_NAME} is a PRoot script wrapper.

Usage:
  ${SCRIPT_NAME} [OPTION...] [COMMAND]

  ## show help of each command
  ${SCRIPT_NAME} [COMMAND] --help

Options:
  --help              show this help
  -v                  print more information
  -x                  run this script in 'set -x'

Commands:
  install             install rootfs
  login               login rootfs
  archive             archive rootfs

Related environment variables:
  PROOT               path to proot\
"
	}

	if test $# -eq 0; then
		_show_help
	else
		script_opt_is_verbose() { false; }

		while test $# -gt 0; do
			case "$1" in
			-x)
				shift
				set -x
				;;
			-v)
				shift
				script_opt_is_verbose() { true; }
				;;
			--help)
				shift
				_show_help
				break
				;;
			install)
				FUNC_NAME="$1"
				shift
				command_install "$@"
				break
				;;
			login)
				FUNC_NAME="$1"
				shift
				command_login "$@"
				break
				;;
			archive)
				FUNC_NAME="$1"
				shift
				command_archive "$@"
				break
				;;
			*)
				err_args "$@"
				break
				;;
			esac
		done
	fi
}

main "$@"
