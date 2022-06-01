# Tonix
![build](https://github.com/Kyota-exe/Tonix/workflows/Tonix%20kernel%20CI/badge.svg)

A simple UNIX-like x86_64 operating system I'm working on for fun.
Aims to be POSIX-compliant enough to make porting Linux software easier.

![Screenshot](/screenshot.png "Screenshot")

## Features
- VFS with ext2 and devfs
- Dynamically-linked ELF user processes
- Terminal with raw/cooked mode (with ANSI escape sequences)
- Multi-core scheduling
- ncurses port
- Bash port
- GNU coreutils port (`ls`, `stat`, `cat`, `echo`, `sha512sum`, `cksum`, etc.)
- GCC port
- FIGlet port

## Work In Progress
- DOOM port

## Building and Running
Build the distro: `make distro`

Build the ramdisk: `make ramdisk`

Build Tonix: `make`

Run it in qemu: `make run`

## Contributing
Contributions are always welcome, but please never use `[&]` as a lambda capture.
