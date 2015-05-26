#! /bin/bash

#
# Notice:
# 	This scripit will get and update all the modules code of kernel.
#
# For example:
#	cd <work-directory>
#	./common/arch/arm/mach-mesong9tv/tools/upmod/updatemodules.sh


if [ ! -d "common/" ]; then
	echo "Note: <work-directory> should be the parent directory of common"
	exit 1
fi

WORK_DIR=`pwd`


# $1 MOD_DIR
# $2 MOD_NAME
# $3 MOD_GIT
# $4 MOD_BRANCH
updatemodule() {

	echo "module $2..."
	if [ ! -d $1/$2 ]; then
		mkdir -p $1
		cd $1
		git clone $3 -b $4
	else
		cd $1/$2
		git checkout -t origin/$4 -b $4
		git pull
	fi

	# cd <work-directory>
	cd $WORK_DIR
	echo ""
	echo ""
}




################################################################################
#
# mali
#
################################################################################

MOD_DIR="hardware/arm"
MOD_NAME="gpu"
MOD_GIT="git://git-sc.amlogic.com/platform/hardware/arm/gpu.git"
MOD_BRANCH="r4p0-01"
updatemodule $MOD_DIR $MOD_NAME $MOD_GIT $MOD_BRANCH



################################################################################
#
# tvin
#
################################################################################

MOD_DIR="hardware"
MOD_NAME="tvin"
MOD_GIT="ssh://gituser@git-sc.amlogic.com/linux/amlogic/tvin.git"
MOD_BRANCH="amlogic-3.10-bringup"
updatemodule $MOD_DIR $MOD_NAME $MOD_GIT $MOD_BRANCH



################################################################################
#
# nand
#
################################################################################

MOD_DIR="hardware/amlogic"
MOD_NAME="nand"
MOD_GIT="git://git-sc.amlogic.com/platform/hardware/amlogic/nand.git"
MOD_BRANCH="amlogic-nand"
updatemodule $MOD_DIR $MOD_NAME $MOD_GIT $MOD_BRANCH



################################################################################
#
# dvb
#
################################################################################

MOD_DIR="hardware/dvb/altobeam/drivers"
MOD_NAME="atbm887x"
MOD_GIT="ssh://android@10.8.9.5/linux/dvb/altobeam/drivers/atbm887x.git"
MOD_BRANCH="amlogic-3.10"
updatemodule $MOD_DIR $MOD_NAME $MOD_GIT $MOD_BRANCH



MOD_DIR="hardware/dvb/silabs/drivers"
MOD_NAME="si2177"
MOD_GIT="ssh://android@10.8.9.5/linux/dvb/silabs/drivers/si2177.git"
MOD_BRANCH="amlogic-3.10"
updatemodule $MOD_DIR $MOD_NAME $MOD_GIT $MOD_BRANCH



MOD_DIR="hardware/dvb/silabs/drivers"
MOD_NAME="si2168"
MOD_GIT="ssh://android@10.8.9.5/linux/dvb/silabs/drivers/si2168.git"
MOD_BRANCH="amlogic-3.10"
updatemodule $MOD_DIR $MOD_NAME $MOD_GIT $MOD_BRANCH



MOD_DIR="hardware/dvb/availink/drivers"
MOD_NAME="avl6211"
MOD_GIT="ssh://android@10.8.9.5/linux/dvb/availink/drivers/avl6211.git"
MOD_BRANCH="amlogic-3.10"
updatemodule $MOD_DIR $MOD_NAME $MOD_GIT $MOD_BRANCH



MOD_DIR="hardware/dvb/maxlinear/drivers"
MOD_NAME="mxl101"
MOD_GIT="ssh://android@10.8.9.5/linux/dvb/maxlinear/drivers/mxl101.git"
MOD_BRANCH="amlogic-3.10"
updatemodule $MOD_DIR $MOD_NAME $MOD_GIT $MOD_BRANCH



################################################################################
#
# wifi broadcom
#
################################################################################

MOD_DIR="hardware/wifi/broadcom/drivers"
MOD_NAME="ap6xxx"
MOD_GIT="git://git-sc.amlogic.com/platform/hardware/wifi/broadcom/drivers/ap6xxx.git"
MOD_BRANCH="ap6xxx"
updatemodule $MOD_DIR $MOD_NAME $MOD_GIT $MOD_BRANCH



MOD_DIR="hardware/wifi/broadcom/drivers"
MOD_NAME="usi"
MOD_GIT="git://git-sc.amlogic.com/platform/hardware/wifi/broadcom/drivers/usi.git"
MOD_BRANCH="kk-amlogic"
updatemodule $MOD_DIR $MOD_NAME $MOD_GIT $MOD_BRANCH



################################################################################
#
# wifi realtek
#
################################################################################

MOD_DIR="hardware/wifi/realtek/drivers"
MOD_NAME="8188eu"
MOD_GIT="git://git-sc.amlogic.com/platform/hardware/wifi/realtek/drivers/8188eu.git"
MOD_BRANCH="8188eu"
updatemodule $MOD_DIR $MOD_NAME $MOD_GIT $MOD_BRANCH



MOD_DIR="hardware/wifi/realtek/drivers"
MOD_NAME="8192cu"
MOD_GIT="git://git-sc.amlogic.com/platform/hardware/wifi/realtek/drivers/8192cu.git"
MOD_BRANCH="8192cu"
updatemodule $MOD_DIR $MOD_NAME $MOD_GIT $MOD_BRANCH



MOD_DIR="hardware/wifi/realtek/drivers"
MOD_NAME="8192du"
MOD_GIT="git://git-sc.amlogic.com/platform/hardware/wifi/realtek/drivers/8192du.git"
MOD_BRANCH="8192du"
updatemodule $MOD_DIR $MOD_NAME $MOD_GIT $MOD_BRANCH



MOD_DIR="hardware/wifi/realtek/drivers"
MOD_NAME="8192eu"
MOD_GIT="git://git-sc.amlogic.com/platform/hardware/wifi/realtek/drivers/8192eu.git"
MOD_BRANCH="8192eu"
updatemodule $MOD_DIR $MOD_NAME $MOD_GIT $MOD_BRANCH



MOD_DIR="hardware/wifi/realtek/drivers"
MOD_NAME="8189es"
MOD_GIT="git://git-sc.amlogic.com/platform/hardware/wifi/realtek/drivers/8189es.git"
MOD_BRANCH="8189es"
updatemodule $MOD_DIR $MOD_NAME $MOD_GIT $MOD_BRANCH



MOD_DIR="hardware/wifi/realtek/drivers"
MOD_NAME="8723bs"
MOD_GIT="git://git-sc.amlogic.com/platform/hardware/wifi/realtek/drivers/8723bs.git"
MOD_BRANCH="8723bs"
updatemodule $MOD_DIR $MOD_NAME $MOD_GIT $MOD_BRANCH



MOD_DIR="hardware/wifi/realtek/drivers"
MOD_NAME="8723au"
MOD_GIT="git://git-sc.amlogic.com/platform/hardware/wifi/realtek/drivers/8723au.git"
MOD_BRANCH="8723au"
updatemodule $MOD_DIR $MOD_NAME $MOD_GIT $MOD_BRANCH



MOD_DIR="hardware/wifi/realtek/drivers"
MOD_NAME="8811au"
MOD_GIT="git://git-sc.amlogic.com/platform/hardware/wifi/realtek/drivers/8811au.git"
MOD_BRANCH="8811au"
updatemodule $MOD_DIR $MOD_NAME $MOD_GIT $MOD_BRANCH



MOD_DIR="hardware/wifi/realtek/drivers"
MOD_NAME="8723bu"
MOD_GIT="git://git-sc.amlogic.com/platform/hardware/wifi/realtek/drivers/8723bu.git"
MOD_BRANCH="8723bu"
updatemodule $MOD_DIR $MOD_NAME $MOD_GIT $MOD_BRANCH



MOD_DIR="hardware/wifi/realtek/drivers"
MOD_NAME="8812au"
MOD_GIT="git://git-sc.amlogic.com/platform/hardware/wifi/realtek/drivers/8812au.git"
MOD_BRANCH="8812au"
updatemodule $MOD_DIR $MOD_NAME $MOD_GIT $MOD_BRANCH

