# Tonix
A simple UNIX-like x86_64 operating system I'm working on for fun.
Aims to be POSIX-compliant enough to make porting Linux software easier.

## Features
- VFS with ext2 and devfs
- Dynamically-linked ELF user processes
- Terminal with raw/cooked mode (supports ANSI escape sequences!)
- Multi-core scheduling
- ncurses port
- Bash port

## Work In Progress
- GNU coreutils port (`ls`, `cat`, `echo`, `mkdir`, etc.)

## Building and Running
Build the distro: `make distro`

Build the ramdisk: `make ramdisk`

Build Tonix: `make`

Run it in qemu: `make run`

## Contributing
Contributions are always welcome, but please never use `[&]` as a lambda capture.
