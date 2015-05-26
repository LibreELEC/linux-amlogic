#
# Makefile for the DVB driver.
#

obj-$(CONFIG_AM_DVB) += aml.o

aml-objs=aml_dvb.o aml_dmx.o

obj-$(CONFIG_AM_DVB) += aml_fe.o

obj-$(CONFIG_AM_CI) += amlci/

obj-$(CONFIG_AM_DTVDEMOD) += amldemod/

obj-$(CONFIG_AM_ATVDEMOD) += amlatvdemod/

ifneq ($(wildcard $(srctree)/../hardware/dvb/altobeam/drivers/atbm887x),)
	obj-$(CONFIG_AM_ATBM8869) += ../../../../hardware/dvb/altobeam/drivers/atbm887x/
endif

ifneq ($(wildcard $(srctree)/../hardware/dvb/silabs/drivers/si2157),)
	obj-$(CONFIG_AM_SI2157) += ../../../../hardware/dvb/silabs/drivers/si2157/
endif

ifneq ($(wildcard $(srctree)/../hardware/dvb/silabs/drivers/si2177),)
	obj-$(CONFIG_AM_SI2177) += ../../../../hardware/dvb/silabs/drivers/si2177/
endif

ifneq ($(wildcard $(srctree)/../hardware/dvb/silabs/drivers/si2168),)
	obj-$(CONFIG_AM_SI2168) += ../../../../hardware/dvb/silabs/drivers/si2168/
endif

ifneq ($(wildcard $(srctree)/../hardware/dvb/availink/drivers/avl6211),)
	obj-$(CONFIG_AM_AVL6211) += ../../../../hardware/dvb/availink/drivers/avl6211/
endif

ifneq ($(wildcard $(srctree)/../hardware/dvb/availink/drivers/avl68xx),)
	obj-$(CONFIG_AM_AVL68xx) += ../../../../hardware/dvb/availink/drivers/avl68xx/
endif

ifneq ($(wildcard $(srctree)/../hardware/dvb/maxlinear/drivers/mxl101),)
	obj-$(CONFIG_AM_MXL101) += ../../../../hardware/dvb/maxlinear/drivers/mxl101/
endif

ifneq ($(wildcard $(srctree)/../hardware/dvb/maxlinear/drivers/mxl241),)
	obj-$(CONFIG_AM_MXL241_FE) += ../../../../hardware/dvb/maxlinear/drivers/mxl241/
endif

ifneq ($(wildcard $(srctree)/../hardware/dvb/maxlinear/drivers/mxl683),)
	obj-$(CONFIG_AM_MXL683_FE) += ../../../../hardware/dvb/maxlinear/drivers/mxl683/
endif

ifneq ($(wildcard $(srctree)/../hardware/dvb/rafael/drivers/r840),)
	obj-$(CONFIG_AM_R840) += ../../../../hardware/dvb/rafael/drivers/r840/
endif

ifneq ($(wildcard $(srctree)/../hardware/dvb/silabs/drivers/si2151),)
	obj-$(CONFIG_AM_SI2151) += ../../../../hardware/dvb/silabs/drivers/si2151/
endif

EXTRA_CFLAGS += -I.
