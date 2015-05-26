obj-y 	+=display.o
display-objs=vout_notify.o  vout_serve.o

obj-$(CONFIG_AML_VDAC_HW_SWITCH)  += vdac_switch.o


obj-$(CONFIG_AM_TV_OUTPUT)  += am_tvout.o
am_tvout-objs :=   tvoutc.o  tvconf.o
ifeq ($(CONFIG_ARCH_MESONG9TV),y)
else
ifeq ($(CONFIG_ARCH_MESONG9BB),y)
else
am_tvout-objs += enc_clk_config.o
endif
endif
obj-$(CONFIG_AM_LCD_OUTPUT)  += lcdoutc.o
obj-$(CONFIG_AM_LCD_OUTPUT)  += lcd_extern/

#EXTRA_CFLAGS += -DCONFIG_PM
