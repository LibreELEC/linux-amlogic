#include <linux/init.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/jiffies.h>
#include <linux/smp.h>
#include <linux/io.h>
#include <plat/io.h>
#include <mach/io.h>
#include <mach/cpu.h>
#include <asm/smp_scu.h>
#include <asm/hardware/gic.h>
#include <asm/smp_plat.h>
#include <asm/smp_scu.h>
#include <asm/cacheflush.h>
#include <asm/mach-types.h>
#include <asm/cp15.h>

extern void meson_turn_off_smp(void);
extern void meson_cleanup(void);

int meson_cpu_kill(unsigned int cpu)
{
	unsigned int value=0;
	unsigned int offset=(cpu<<3);
	unsigned long timeout;

	timeout = jiffies + (50 * HZ);
	while (time_before(jiffies, timeout)) {
		msleep(10);
		smp_rmb();
		value=aml_read_reg32(MESON_CPU_POWER_CTRL_REG);
		if ((value&(3<<offset)) == (3<<offset))
			break;
	}
	//printk("cpu %d timeout, value=0x%x, compare=%d\n", cpu, value, ((value&(3<<offset)) == (3<<offset)));
	msleep(30);
	meson_set_cpu_power_ctrl(cpu, 0);
	return 1;
}




void meson_cpu_die(unsigned int cpu)
{
	meson_set_cpu_ctrl_reg(cpu, 0);
	//printk("[cpu hotplug]cpu:%d shutdown\n", cpu);
	flush_cache_all();
	dsb();
	dmb();

	aml_set_reg32_bits(MESON_CPU_POWER_CTRL_REG,0x3,(cpu << 3),2);
	dmb();
	dsb();

	meson_cleanup();
	meson_turn_off_smp();
	asm volatile(
		"dsb\n"
		"wfi\n"
	);
	BUG();
}

int meson_cpu_disable(unsigned int cpu)
{
	/*
	 * we don't allow CPU 0 to be shutdown (it is still too special
	 * e.g. clock tick interrupts)
	 */
	return cpu == 0 ? -EPERM : 0;
}
