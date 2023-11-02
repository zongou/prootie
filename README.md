[Extra Doc](./extra/docs/android_proot_gui_and_audio.md)

```txt
  █▀█ █▀█ █▀█ █▀█ ▀█▀ █ █▀▀
  █▀▀ █▀▄ █▄█ █▄█ ░█░ █ ██▄
```

* Minimal proot wrapper written in posix shell.
* Supports both android and linux.
* Anotherterm and Termux are well supported.
* Install the rootfs archive to anywhere you want.
* Archive to the standerd rootfs format that will not break.
  
Get help

```sh
# simple help
prootie --help

# help of a single command
prootie [COMMAND] --help

# help of all commands
prootie --help-all
```

## Install rootfs

```sh
# Install internet rootfs archive while downloading
# Install a .tar.gz formatted rootfs
curl -Lk rootfs_img_url.tar.gz | gzip -d | prootie install [ROOTFS_DIR]

# Install a .tar.xz formated rootfs
curl -Lk rootfs_img_url.tar.xz | xz -d -T0 | prootie install [ROOTFS_DIR]
```

## Run commands

```sh
# simple run shell as root user
prootie run [ROOTFS_DIR]

# with PROOT env point to custom proot path
PROOT=/path/to/proot prootie run [ROOTFS_DIR]

# run command in distro
echo hello | prootie run [ROOTFS_DIR] -- cat

# run as the specific user
prootie run [ROOTFS_DIR] -- su -l [USER_NAME]

# run login program
prootie run [ROOTFS_DIR] -- login

# enable host utils
# anotherterm and termux will be well supported
prootie run [ROOTFS_DIR] --host-utils

# with proot options
prootie run [ROOTFS_DIR] --bind=*:*
```

## Archive rootfs

```sh
# Archive to gzip formatted
prootie archive [ROOTFS_DIR] | gzip > [ROOTFS_DIR].tar.gz

# Archive to xz formatted
prootie archive [ROOTFS_DIR] | xz -T0 > [ROOTFS_DIR].tar.xz

# Clone container
prootie archive [ROOTFS_DIR] | prootie install [ROOTFS_DIR]
```

## Resources

* [zongou/build-proot-android](https://github.com/zongou/build-proot-android/actions/workflows/build.yaml) (proot and android10essentialsApp)

### Download rootfs archives

* lxc images
  * [linuxcontainer.org](http://images.linuxcontainers.org/images "linuxcontainer.org")
  * [lxc tsinghua mirror](https://mirrors.tuna.tsinghua.edu.cn/lxc-images/images/ "lxc tsinghua mirror")
* ubuntu base
  * [official site](https://cdimage.ubuntu.com/ubuntu-base "official site")
  * [ustc mirror](https://mirrors.ustc.edu.cn/ubuntu-cdimage/ubuntu-base "ustc mirror")
  * [aliyun mirror](https://mirrors.aliyun.com/ubuntu-cdimage/ubuntu-base "aliyun mirror")

### Get repo source

* [ustc repogen](https://mirrors.ustc.edu.cn/repogen/ "ustc repogen")

### Relavent Links

* [AnotherTerm-scripts](https://github.com/green-green-avk/AnotherTerm-scripts "AnotherTerm-scripts")
* [termux/proot-distro](https://github.com/termux/proot-distro/blob/master/proot-distro.sh)
* [AnotherTerm-docs](https://green-green-avk.github.io/AnotherTerm-docs/#main_content "AnotherTerm-docs")
* [PRoot](https://github.com/proot-me/proot)
