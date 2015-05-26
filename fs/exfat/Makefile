obj-$(CONFIG_EXFAT_FS) += exfat_core.o exfat_fs.o

exfat_fs-objs	:= exfat_super.o

exfat_core-objs	:= exfat.o exfat_api.o exfat_blkdev.o exfat_cache.o \
			   exfat_data.o exfat_global.o exfat_nls.o exfat_oal.o exfat_upcase.o

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	
