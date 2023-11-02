# Android PRoot Graphics and Audio

## Install a desktop enviroment

### Install xfce4

```sh
DEBIAN_FRONTEND=noninteractive apt install --no-install-recommends -y xfce4 xorg xfce4-terminal
```

### Install gnome

```sh
DEBIAN_FRONTEND=noninteractive apt install --no-install-recommends -y gnome-core gnome-shell-extension-prefs
```

### Noninteractive install reconfigure

```sh
dpkg-reconfigure --default-priority tzdata keyboard-configuration
```

## xsdl

```sh
export DISPLAY=127.0.0.1:0
export PULSE_SERVER=tcp:127.0.0.1:4713
```

## termux-x11

[termux-x11 repo](https://github.com/termux/termux-x11)  

### Required files and PRoot bindings

- termux-x11(android app)
- loader.apk

Required android env:

- ANDROID_DATA
- ANDROID_RUNTIME_ROOT
- ANDROID_TZDATA_ROOT
- BOOTCLASSPATH

Required proot bindings:

- /system
- /apex
- /vendor
- /data/app

Required package for alpine:

- mesa-dri-gallium

### termux-x11 script

```sh
export CLASSPATH=<path_to_loader.apk>
unset LD_LIBRARY_PATH LD_PRELOAD
exec /system/bin/app_process / com.termux.x11.Loader "$@"
```

### start xfce4

```sh
termux-x11 :0 -xstartup "dbus-launch --exit-with-session xfce4-session"
```

### start gnome-shell

<https://github.com/termux/termux-x11/issues/340>

```sh
# Install dependencies
apt install dbus-x11

# Remove org.freedesktop.login1.service
mv /usr/share/dbus-1/system-services/org.freedesktop.login1.service /usr/share/dbus-1/system-services/org.freedesktop.login1.service.bak

# Start dbus service
service dbus start

# Start gnome in x11
termux-x11 :0 -xstartup "gnome-shell --x11"

# Start gnome in wayland
termux-x11 :0 -xstartup "gnome-shell --wayland --nested"
```

## anotherterm Xwayland

requires: xwayland, xlib  
build package: libwrapandroid

ubuntu

```sh
apt install xwayland python3-xlib -y
```

alpine

```sh
apk add xwayland py3-xlib
```

start a graphic session

```sh
/opt/Xutils/bin/wlstart-X | /opt/Xutils/bin/wlstart-WM
```

### VNC

```sh
# vnc on alpine

# ==== VNC ====
apk add tigervnc
# Setup password
vncpasswd
000000
000000

# Start vncserver
# if falid to start
# DISPLAY env can cause vncserver fail to start
# check if port 5900 + :<num> occupied
vncserver --localhost=no :0
```

## Audio support

### pulseaudio tcp & rtp server

android apps:
- [PulseDroidRtp](https://github.com/wenxin-wang/PulseDroidRtp)
- [SimpleProtocolPlayer](https://github.com/kaytat/SimpleProtocolPlayer)

rtp dependencies:
- alpine: gst-plugins-good
- debian: gstreamer1.0-plugins-rtp


#### Run this script

```sh
pulseaudio --daemonize=true --exit-idle-time=-1 --no-cpu-limit=true \
	--load="module-native-protocol-tcp" \
	--load="module-simple-protocol-tcp source=auto_null.monitor rate=48000 channels=2 format=s16le record=true listen=127.0.0.1 port=4010" \
	--load="module-rtp-send source=auto_null.monitor rate=48000 channels=2 mtu=320 destination=127.0.0.1 port=4010"
```

#### Or add your pulseaudio configuration  

> /etc/pulse/default.pa.d/sender.pa

```pa
#!/usr/bin/pulseaudio -nF

## Load module that works on android
load-module module-native-protocol-tcp

## tcp sender
load-module module-simple-protocol-tcp source=auto_null.monitor rate=48000 channels=2 format=s16le record=true listen=127.0.0.1 port=4010

## rtp sender
load-module module-rtp-send source=auto_null.monitor rate=48000 channels=2 mtu=320 destination=127.0.0.1 port=4010
```

and run

```sh
pulseaudio --daemonize=true --exit-idle-time=-1 --no-cpu-limit=true
```

### termux start pulseaudio server

```sh
pulseaudio \
    --start \
    --exit-idle-time=-1 && 
pacmd load-module module-native-protocol-tcp \
    auth-ip-acl=127.0.0.1 \
    auth-anonymous=1
```

or use xsdl app

## Install chromium & firefox on ubuntu without snap

<https://askubuntu.com/questions/1204571/how-to-install-chromium-without-snap>

```sh
# Remove Ubuntu chromium packages:
apt remove chromium-browser chromium-browser-l10n chromium-codecs-ffmpeg-extra

# Add Debian repository
mirror=mirrors.ustc.edu.cn
cat <<- EOF >/etc/apt/sources.list.d/debian.list
deb http://${mirror}/debian bookworm main
deb http://${mirror}/debian bookworm-updates main
deb http://${mirror}/debian-security bookworm-security main
EOF
cat /etc/apt/sources.list.d/debian.list

# Add the Debian signing keys:
apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 648ACFD622F3D138
apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 0E98404D386FA1D9
apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 54404762BBB6E853

# Configure apt pinning
cat <<- EOF >/etc/apt/preferences.d/chromium.pref
# Note: 2 blank lines are required between entries

Package: *
Pin: release a=bookworm
Pin-Priority: 500

Package: *
Pin: origin "deb.debian.org"
Pin-Priority: 300

# Pattern includes 'chromium', 'chromium-browser', 'firefox' and similarly

# named dependencies

Package: chromium*
Pin: origin "deb.debian.org"
Pin-Priority: 700


Package: firefox*
Pin: origin "deb.debian.org"
Pin-Priority: 700
EOF

# Install Chromium again
apt update
apt install chromium --no-install-recommends -y
```

## Install noto fonts

ubuntu

```sh
apt install --no-install-recommends -y fonts-noto-cjk
```

alpine

```sh
apk add font-noto-cjk
```

## Profile to emulate init

> /etc/profile.d/init.sh

```sh
#!/bin/sh

start_dbus_service() {
	if ! sudo service dbus status >/dev/null; then
		sudo service dbus start
	fi
}

start_pulseaudio_service() {
	## Start without config
	# pulseaudio --daemonize=true --exit-idle-time=-1 --no-cpu-limit=true \
	# 	--load=module-native-protocol-tcp \
	# 	--load="module-simple-protocol-tcp source=auto_null.monitor rate=48000 channels=2 format=s16le record=true listen=127.0.0.1 port=4010" \
	# 	--load="module-rtp-send source=auto_null.monitor rate=48000 channels=2 mtu=320 destination=127.0.0.1 port=4010"

	## Start with config
	pulseaudio --daemonize=true --exit-idle-time=-1 --no-cpu-limit=true
}

start_dbus_service
start_pulseaudio_service

```
