
```sh
zig cc -target $(uname -m)-linux-musl -o $PREFIX/bin/prootie prootie.c -s -Os
zig cc -target $(uname -m)-linux-musl -o $PREFIX/bin/plogin tmp/plogin.c -s -Os

file $(command -v prootie)
file $(command -v plogin)
du -ahd0 $(command -v prootie)
du -ahd0 $(command -v plogin)
```