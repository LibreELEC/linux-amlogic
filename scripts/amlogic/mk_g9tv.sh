#! /bin/bash

#make UIMAGE_COMPRESSION=none uImage -j
make uImage -j16
#make modules

make mesong9tv_n300.dtd
make mesong9tv_n300.dtb

#ROOTFS="rootfs.cpio"
ROOTFS="arch/arm/mach-mesong9tv/rootfs.cpio"

./mkbootimg --kernel ./arch/arm/boot/uImage --ramdisk ./${ROOTFS} --second ./arch/arm/boot/dts/amlogic/mesong9tv_n300.dtb --output ./boot.img
ls -l ./boot.img
echo "boot.img done"

xxd -p -c1 boot.img > boot.hex
