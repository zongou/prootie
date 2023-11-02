# prootie

prootie is a PRoot script wrapper.

## Show help

```sh
prootie --help
prootie install --help
prootie login --help
prootie archive --help
```

## Quick start

### Install rootfs

```sh
ARCH=$(uname -m)
VERSION=3.19.1
ALPINE_ROOTFS_URL=https://dl-cdn.alpinelinux.org/alpine/latest-stable/releases/${ARCH}/alpine-minirootfs-${VERSION}-${ARCH}.tar.gz
curl -Lk "${ALPINE_ROOTFS_URL}" | gzip -d | prootie install alpine -v
```

### Login rootfs

```sh
prootie login alpine
```

### Archive rootfs

```sh
prootie archive alpine | xz -T0 -v > alpine.tar.xz
```

### Clone rootfs

```sh
prootie archive alpine | prootie install alpine_clone
```

## More

[Document for configuring Desktop enviroment and Audio on android](doc.md)
