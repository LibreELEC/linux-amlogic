#! /bin/bash

#make UIMAGE_COMPRESSION=none uImage -j
make uImage -j8
#make modules

make mesong9bb_t828_skt.dtd
make mesong9bb_t828_skt.dtb

ROOTFS="rootfs.cpio"
#ROOTFS="arch/arm/mach-mesong9bb/rootfs.cpio"

./mkbootimg --kernel ./arch/arm/boot/uImage --ramdisk ./${ROOTFS} --second ./arch/arm/boot/dts/amlogic/mesong9bb_t828_skt.dtb --output ./boot.img
ls -l ./boot.img
echo "boot.img done"

xxd -p -c1 boot.img > boot.hex
