#!/bin/bash

# Install qemu-mips user mode static and chroot
sudo apt install qemu-user-static coreutils

sudo cp $(which qemu-mips-static) ./chroot
