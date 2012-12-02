#include <linux/module.h>
#include <linux/init.h>
#include <asm/wbflush.h>
#include <linux/console.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/netdevice.h>
#include <linux/rtnetlink.h>

#include <asm/addrspace.h>
#include <asm/irq.h>
#include <asm/io.h>

#include <asm/bootinfo.h>
#include <asm/time.h>
#include <asm/reboot.h>
#include <prom.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/serial.h>
#include <linux/serial_core.h>
#include <linux/serial_8250.h>
#include <linux/string.h>

#include <asm/serial.h>

#include <bspchip.h>


void (*__wbflush) (void);

static void mips_wbflush(void)
{
	__fast_iob();
}


void __init wbflush_setup(void)
{
	__wbflush = mips_wbflush;
}

static void __init serial_init(void)
{
	struct uart_port s;

	/* clear memory */
	memset(&s, 0, sizeof(s));

	/*
	 * UART0
	 */
	s.line = 0;
	s.type = PORT_16550A;
	s.irq = BSP_UART0_IRQ;
	s.iotype = UPIO_MEM;
	s.regshift = 2;
#if 1
	s.uartclk = BSP_SYS_CLK_RATE;
	s.fifosize = 16;
	//s.flags = UPF_SKIP_TEST | UPF_LOW_LATENCY;
	s.flags = UPF_SKIP_TEST;
	s.mapbase = BSP_UART0_MAP_BASE;
	//s.membase = ioremap_nocache(s.mapbase, BSP_UART0_MAPSIZE);
	s.membase = ioremap_nocache(s.mapbase, 0x20);
#else
	s.uartclk = BSP_SYS_CLK_RATE - BSP_BAUDRATE * 24; //???
	s.fifosize = 1;                           //???
	s.flags = UPF_SKIP_TEST | UPF_LOW_LATENCY | UPF_SPD_CUST;
	s.membase = (unsigned char *)BSP_UART0_BASE;
	s.custom_divisor = BSP_SYS_CLK_RATE / (BSP_BAUDRATE * 16) - 1;
#endif

	if (early_serial_setup(&s) != 0) {
		panic("RTL8196C: bsp_serial_init failed!");
	}
}


void __init plat_mem_setup(void)
{
   /* Platform Specific Setup */
   /* define io/mem region */
    ioport_resource.start = 0x18000000; 
    ioport_resource.end = 0x1fffffff;

    iomem_resource.start = 0x18000000;
    iomem_resource.end = 0x1fffffff;
       
   serial_init();

//   board_time_init = rtl8652_time_init;
//   mips_timer_ack = rtl8652_timer_ack;
}




EXPORT_SYMBOL(__wbflush);
