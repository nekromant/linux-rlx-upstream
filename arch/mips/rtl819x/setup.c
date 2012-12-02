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
//#include <asm/rlxbsp.h>
//#include <asm/rtl865x/rtl865xc_asicregs.h>

#include "bspchip.h"

void (*__wbflush) (void);

static void mips_wbflush(void)
{
	__fast_iob();
}


void __init wbflush_setup(void)
{
	__wbflush = mips_wbflush;
}


void prom_putchar(char c)
{
#define UART0_BASE		0xB8002000
#define UART0_THR		(UART0_BASE + 0x000)
#define UART0_FCR		(UART0_BASE + 0x008)
#define UART0_LSR       (UART0_BASE + 0x014)
#define TXRST			0x04
#define CHAR_TRIGGER_14	0xC0
#define LSR_THRE		0x20
#define TxCHAR_AVAIL	0x00
#define TxCHAR_EMPTY	0x20
	unsigned int busy_cnt = 0;

	do
	{
		/* Prevent Hanging */
		if (busy_cnt++ >= 30000)
		{
			/* Reset Tx FIFO */
			REG8(UART0_FCR) = TXRST | CHAR_TRIGGER_14;
			return;
		}
	} while ((REG8(UART0_LSR) & LSR_THRE) == TxCHAR_AVAIL);

	/* Send Character */
	REG8(UART0_THR) = c;
}



EXPORT_SYMBOL(__wbflush);
