## proot-tool help info
	Tool to install Linux under PRoot on Android

	Usage:
	  proot-tool [command] [<args...>]
	  proot-tool [option]

	  View command help information
	  proot-tool help [command]

	Commands:
	  help          # Show help message.
	  backup        # Backup Container.
	  cp|clone      # Clone Container.
	  fix-l2s       # Fix proot link2symlink.
	  gen-start     # Generate startup script only.
	  install       # Install rootfs container.
	  update        # Update this tool to latest.

	Options:
	  --help|-h     # Same as command "help". 
### Download rootfs archives
- lxc images
	- [get from linuxcontainer.org](http://images.linuxcontainers.org/images "get from linuxcontainer.org")
	- [get from tsinghua mirror](https://mirrors.tuna.tsinghua.edu.cn/lxc-images/images/ "get from tsinghua mirror")
- ubuntu base 
	- [get from official sites](https://cdimage.ubuntu.com/ubuntu-base "get from official sites")
	- [get from ustc](https://mirrors.ustc.edu.cn/ubuntu-cdimage/ubuntu-base "get from ustc")
	- [get from aliyun](https://mirrors.aliyun.com/ubuntu-cdimage/ubuntu-base "get from aliyun")

### Get repo source
- [ustc repogen](https://mirrors.ustc.edu.cn/repogen/ "ustc repogen")

### Install proot container on Android 10 (API 29) and higher.
run this command to get help information
```shell
proot-tool help install
```

### Relavent Links
- [AnotherTerm-scripts](https://github.com/green-green-avk/AnotherTerm-scripts "AnotherTerm-scripts")
- [termux/proot-distro ](https://github.com/termux/proot-distro/blob/master/proot-distro.sh)
- [AnotherTerm-docs](https://green-green-avk.github.io/AnotherTerm-docs/#main_content "AnotherTerm-docs")
- https://wiki.termux.com/wiki/PRoot
- [Moe-hacker/termux-container](https://github.com/Moe-hacker/termux-container/blob/main/package/data/data/com.termux/files/usr/bin/container "Moe-hacker container")
- [tmoe](https://github.com/2moe/tmoe)
