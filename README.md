# Tonix
![build](https://github.com/Kyota-exe/Tonix/workflows/Tonix%20kernel%20CI/badge.svg)

A modern UNIX-like x86_64 operating system and monolithic kernel I'm working on for fun.
It aims to be POSIX-compliant enough to make porting Linux software easier. 
Contributions are always welcome and highly encouraged!

![Demo](/demo.gif "Demo")

## Can it run DOOM? Of course!
![DOOM](/doom.gif "DOOM")

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
- DOOM port

## Work In Progress
- Python port

## Building and Running
Build the distro: `make distro`

Build the ramdisk: `make ramdisk`

Build Tonix: `make`

Run it in qemu: `make run`

Make sure you place `doom1.wad` in `root-directory/` if you want to run DOOM.

## Contributing
Contributions are always welcome, but please never use `[&]` as a lambda capture.
