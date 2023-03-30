#!/bin/sh
set -eux

# For 4.4~4.9 Kernel
GKI_ROOT=$(pwd)
DRIVER_DIR=$GKI_ROOT/drivers

echo "[+] DRIVER_DIR: $DRIVER_DIR"


if test -d "$DRIVER_DIR"; then
     echo 'Find Drivers Floder.'
else
     echo '[ERROR] "drivers/" directory is not found.'
     echo '[+] You should modify this scrpit by yourself.'
     exit 127
fi

mkdir "$DRIVER_DIR/KernelSU"
test -d "$DRIVER_DIR/KernelSU" || git clone https://github.com/tiann/KernelSU
cd "$DRIVER_DIR/"
git stash && git pull
cd "$DRIVER_DIR"


echo '[+] Add kernel su driver to Makefile'

DRIVER_MAKEFILE=$DRIVER_DIR/Makefile
grep -q "KernelSU" "$DRIVER_MAKEFILE" || printf "\nobj-y += KernelSU/\n" >> "$DRIVER_MAKEFILE"

rm -rf "$DRIVER_DIR/KernelSU"

echo '[+] Done.'
