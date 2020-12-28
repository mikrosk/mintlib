#!/bin/bash -eux
# -e: Exit immediately if a command exits with a non-zero status.
# -u: Treat unset variables as an error when substituting.
# -x: Display expanded script commands

BINTRAY_REPO="travis" SYSROOT_DIR="/" ${SCRIPTS_DIR}/install_bintray.sh m68k-atari-mint-binutils-gdb m68k-atari-mint-gcc mintbin
