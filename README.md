## proot-tool help info
	Tool to install Linux under PRoot on Android

	Usage: proot-tool [options...] [file] [dir]

	Examples:
	  proot-tool --install ./distro-rootfs.tar.xz ./distro

	Options:
	  --fix-l2s     [dir]           # Fix proot link2symlink.
	  --gen-start   [<dir>|<file>]  # Generate startup script only.
	  --install     [archive] [dir] # Install rootfs container.
	  --lib-uid     [dir]           # Define the uid of essentials lib dir
	  --help                        # Show help information.
	  --update                      # Update this tool to latest.
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
Install this App [AnotherTermShellPlugin-Android10Essentials](https://github.com/green-green-avk/AnotherTermShellPlugin-Android10Essentials)  
<details markdown='1'><summary>Define uid if exists in lib app source dir</summary>  

<li> We can get essentials lib app source dir via adb  
	
```adb shell
cmd package path green_green_avk.anothertermshellplugin_android10essentials | cut -d: -f2
```
</li>
<li> We can also get it with file explorer application such as X-plorer, Solid Explorer...  </li>  
</br>  

On Android 7, it is a static dir: ```/data/app/green_green_avk.anothertermshellplugin_android10essentials-1/apk```  
On Android 10, its a dir with uid and the uid changes every time you reinstall lib apk, for example: ```/data/app/green_green_avk.anothertermshellplugin_android10essentials-e7AQtL72ErptOc9lBxzYZA==/apk```  

If there is uid, we can can pass it in 3 ways
1. Pass it via script option when you run this script (--lib-uid).
2. Define and Get it from env (LIB_UID).
3. Directy define uid in this script (CUSTOM_LIB_UID).
	
If more than one of the ways is used at the same time, priority: 1 > 2 > 3

</details>  

Examples to install linux distro with essentials lib  
```sh
export LIB_UID='e7AQtL72ErptOc9lBxzYZA'
sh ./proot-tool --install ./distro-rootfs.tar.xz distro
# Or
sh ./proot-tool --install ./distro-rootfs.tar.xz distro --lib-uid 'e7AQtL72ErptOc9lBxzYZA'
```

### Relavent Links
- [AnotherTerm-scripts](https://github.com/green-green-avk/AnotherTerm-scripts "AnotherTerm-scripts")
- [termux/proot-distro ](https://github.com/termux/proot-distro/blob/master/proot-distro.sh)
- [AnotherTerm-docs](https://green-green-avk.github.io/AnotherTerm-docs/#main_content "AnotherTerm-docs")
- https://wiki.termux.com/wiki/PRoot
- [Moe-hacker/termux-container](https://github.com/Moe-hacker/termux-container/blob/main/package/data/data/com.termux/files/usr/bin/container "Moe-hacker container")
- [tmoe](https://github.com/2moe/tmoe)
