#
# aml_nftl
#

ifneq ($(KBUILD_SRC),)
TOP_KBUILD_SRC := $(KBUILD_SRC)/
endif

$(obj)/aml_nftl_core.o:
	@cp $(TOP_KBUILD_SRC)drivers/amlogic/nand/aml_nftl_new/nftl/aml_nftl_core_140322b.z \
        drivers/amlogic/nand/aml_nftl_new/nftl/aml_nftl_core.o

obj-$(CONFIG_AML_NFTL_NEW)	+= aml_nftl_core.o
