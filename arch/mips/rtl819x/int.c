#include <linux/errno.h>
#include <linux/init.h>
#include <linux/kernel_stat.h>
#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/timex.h>
#include <linux/slab.h>
#include <linux/random.h>
#include <linux/irq.h>

#include <asm/bitops.h>
#include <asm/bootinfo.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/irq_cpu.h>
#include <irq_vec.h>
#include <bspchip.h>


static struct irqaction irq_cascade = { 
  .handler = no_action,
  //.mask = CPU_MASK_NONE,
  .name = "cascade",
};

static void bsp_ictl_irq_mask(struct irq_data* d)
{
	REG32(BSP_GIMR) &= ~(1 << (d->irq - BSP_IRQ_ICTL_BASE));
}

static void bsp_ictl_irq_unmask(struct irq_data* d)
{
	REG32(BSP_GIMR) |= (1 << (d->irq - BSP_IRQ_ICTL_BASE));
}

static struct irq_chip bsp_ictl_irq = {
    .name = "ICTL",
    .irq_ack = bsp_ictl_irq_mask,
    .irq_mask = bsp_ictl_irq_mask,
    .irq_mask_ack = bsp_ictl_irq_mask,
    .irq_unmask = bsp_ictl_irq_unmask,
};


static void bsp_ictl_irq_dispatch(void)
{
	volatile unsigned int pending;

	pending = REG32(BSP_GIMR) & REG32(BSP_GISR);

	if (pending & BSP_UART0_IP)
		do_IRQ(BSP_UART0_IRQ);
	else if (pending & BSP_TC1_IP)
		do_IRQ(BSP_TC1_IRQ);
	else if (pending & BSP_GPIO_ABCD_IP)
		do_IRQ(BSP_GPIO_ABCD_IRQ);
	else {
		REG32(BSP_GIMR) &= (~pending);
		REG32(BSP_GISR) = REG32(BSP_GISR);
		printk("Unknown ICTL Interrupt: 0x%x\n", pending);
		spurious_interrupt();
	}
}

static void __init bsp_ictl_irq_init(unsigned int irq_base)
{
    int i;

    for (i=0; i < BSP_IRQ_ICTL_NUM; i++) 
        irq_set_chip_and_handler(irq_base + i, &bsp_ictl_irq, handle_level_irq);

    setup_irq(BSP_ICTL_IRQ, &irq_cascade);
}


void plat_irq_dispatch(void)
{
	volatile unsigned int pending;
	pending = read_c0_cause() & read_c0_status();
	
	if (pending & CAUSEF_IP2)
		bsp_ictl_irq_dispatch();
	else if (pending & CAUSEF_IP0)
		do_IRQ(0);
	else if (pending & CAUSEF_IP1)
		do_IRQ(1);
	else {
	spurious_interrupt();
	}
}


void __init arch_init_irq(void)
{
	//unsigned int	status;
	//volatile unsigned int status;
	/* disable ict interrupt */
	REG32(BSP_GIMR) = 0;

	/* initialize IRQ action handlers */
	rlx_cpu_irq_init(BSP_IRQ_CPU_BASE);
	rlx_vec_irq_init(BSP_IRQ_LOPI_BASE);
	bsp_ictl_irq_init(BSP_IRQ_ICTL_BASE);

	/* Set IRR */
	REG32(BSP_IRR0) = BSP_IRR0_SETTING;
	REG32(BSP_IRR1) = BSP_IRR1_SETTING;
	REG32(BSP_IRR2) = BSP_IRR2_SETTING;
	REG32(BSP_IRR3) = BSP_IRR3_SETTING;  

	//status = read_c0_status();
	//status = (status&(~ST0_IM))|(CAUSEF_IP2|CAUSEF_IP3|CAUSEF_IP4|CAUSEF_IP5|CAUSEF_IP6);
	//write_c0_status(status);
}

#if 0
static DEFINE_SPINLOCK(irq_lock);


static void rtl8196_enable_irq(unsigned int irq)
{
   unsigned long flags;

   spin_lock_irqsave(&irq_lock, flags);
   REG32(BASP_GIMR) = REG32(BSP_GIMR) | (1 << irq);
   spin_unlock_irqrestore(&irq_lock, flags);
}

static unsigned int rtl8196_startup_irq(unsigned int irq)
{
   rtl8196_enable_irq(irq);

   return 0;
}

static void rtl8196_disable_irq(unsigned int irq)
{
   unsigned long flags;

   spin_lock_irqsave(&irq_lock, flags);
   REG32(GIMR) = REG32(GIMR) & (~(1 << irq));
   spin_unlock_irqrestore(&irq_lock, flags);
}

static void rtl8196_end_irq(unsigned int irq)
{
   unsigned long flags;

   spin_lock_irqsave(&irq_lock, flags);
   REG32(GIMR) = REG32(GIMR) | (1 << irq);
   spin_unlock_irqrestore(&irq_lock, flags);
}

#define rtl8196_shutdown_irq      rtl8196_disable_irq
#define rtl8196_mask_and_ack_irq  rtl8196_disable_irq

static struct irq_chip irq_type = {
   .typename = "RTL8196",
   .startup = rtl8196_startup_irq,
   .shutdown = rtl8196_shutdown_irq,
   .enable = rtl8196_enable_irq,
   .disable = rtl8196_disable_irq,
   .ack = rtl8196_mask_and_ack_irq,
   .end = rtl8196_end_irq,
   .mask = rtl8196_disable_irq,
   .mask_ack = rtl8196_mask_and_ack_irq,
   .unmask = rtl8196_enable_irq,   
};


/*
 *   RTL8196b Interrupt Scheme (Subject to change)
 *
 *   Source     EXT_INT   IRQ      CPU INT
 *   --------   -------   ------   -------
 *   PCIB0TO    0         0        2
 *   PCIB1TO    1         1        2
 *   LBCTMOm0   2         2        2
 *   LBCTMOm1   3         3        2
 *   LBCTMOs    4         4        2
 *   TIMER0     8         8        7
 *   TIMER1     9         9        2
 *   USB        10        10       4
 *   UART0      12        12       3
 *   UART1      13        13       2
 *   PCI        14        14       5
 *   SWCORE     15        15       6
 *   GPIO_ABCD  16        16       2
 *   GPIO_EFGH  17        17       2
 *   HCI        18        18       2
 *   PCM        19        19       2
 *   CRYPTO     20        20       2
 *   GDMA       21        21       2
 */

void __init arch_init_irq(void)
{
   int i;

   /* Initialize for IRQ: 0~31 */
   for (i = 0; i < 32; i++) {
      //irq_desc[i].chip = &irq_type;
	set_irq_chip_and_handler(i, &irq_type, handle_level_irq);
   }

   /* Enable all interrupt mask of CPU */
   write_c0_status(read_c0_status() | ST0_IM);

   /* Set GIMR, IRR */
   REG32(GIMR) = TC0_IE | UART0_IE ;

   REG32(IRR0) = IRR0_SETTING;
   REG32(IRR1) = IRR1_SETTING;
   REG32(IRR2) = IRR2_SETTING;
   REG32(IRR3) = IRR3_SETTING;
}

#define	ST0_USED_IM	(_ULCAST_(0xfc00))	/* interrupt 2/3/4/5/6/7	*/
#define	ST0_REENTRY_IM	(_ULCAST_(0xe000))	/* interrupt 5/6/7 */

#define __IRAM_GEN			__attribute__  ((section(".iram-gen")))

__IRAM_GEN asmlinkage void plat_irq_dispatch(void)
{
#if 0
   unsigned int cpuint_ip;
   unsigned int cpuint_mask;
   unsigned int extint_ip;

   cpuint_mask = read_c0_status() & ST0_IM;
   cpuint_ip = read_c0_cause() & read_c0_status() & ST0_IM;

#if 1
   write_c0_status(read_c0_status()&(~ST0_IM));
#else
   write_c0_status((read_c0_status()&(~ST0_IM))|(cpuint_mask&(~(STATUSF_IP6|STATUSF_IP5))));
#endif

   do
   {
   	   if (cpuint_ip & CAUSEF_IP7)
	   {
	      /* Timer 0 */
	      do_IRQ(TC0_IRQ);
	   }
	   else if (cpuint_ip & CAUSEF_IP5)
	   {
	   	/* PCIE */
       		do_IRQ(PCIE_IRQ);
	   }
	   if (cpuint_ip & CAUSEF_IP6)
	   {
	            	/* switch core*/
		 do_IRQ(SWCORE_IRQ);
   	
	   }
	   else if (cpuint_ip & CAUSEF_IP3)
	   {
	      /* pci */
	      do_IRQ(PCI_IRQ);
	   }
          else if (cpuint_ip & CAUSEF_IP4)
          {
                   /*USB*/
                   do_IRQ(USB_IRQ);
          }
	   else if (cpuint_ip & CAUSEF_IP2)
	{
		/* For shared interrupts */
		unsigned int extint_ip = REG32(GIMR) & REG32(GISR);

		if (extint_ip & UART0_IP)
		{
			/* UART 0 */
			do_IRQ(UART0_IRQ);
		}
		else if (extint_ip & TC1_IP)
		{
			do_IRQ(TC1_IRQ);
		}
#if 0
		/* currently we do not use uart1	*/
		else if (extint_ip & UART1_IP)
		{
			do_IRQ(UART1_IRQ);
		}
#endif
		else
		{
			prom_printf("Unknown Interrupt2:%x\n",extint_ip);
			REG32(GISR) = extint_ip;
		}
	}

	   cpuint_ip = read_c0_cause() & (STATUSF_IP6|STATUSF_IP5|STATUSF_IP7);
   } while(cpuint_ip);

#if 0	/*	patch for wds+wep hang up issue */
   write_c0_status((read_c0_status()&(~ST0_IM))|(cpuint_mask));
#else
   write_c0_status((read_c0_status()|(ST0_IM)));
#endif
#else
	unsigned int cpuint_ip;
	unsigned int extint_ip;

	cpuint_ip = read_c0_cause() & ST0_USED_IM;
	write_c0_status(read_c0_status()&(~ST0_IM));
	if ( ST0_REENTRY_IM & cpuint_ip )
	{
		do
		{
			if (cpuint_ip & CAUSEF_IP7)
			{
				/* Timer 0 */
				do_IRQ(TC0_IRQ);
			}
			if (cpuint_ip & CAUSEF_IP6)
			{
				/* switch core*/
				do_IRQ(SWCORE_IRQ);
			}
			if (cpuint_ip & CAUSEF_IP5)
			{
				/* PCIE */
				do_IRQ(PCIE_IRQ);
			}
			cpuint_ip = read_c0_cause() & ST0_REENTRY_IM;

		} while(cpuint_ip);
	}
	else if(cpuint_ip & CAUSEF_IP3)
	{
		/* pci */
		do_IRQ(PCI_IRQ);
	}
	else if (cpuint_ip & CAUSEF_IP4)
	{
		/*USB*/
		do_IRQ(USB_IRQ);
	}
	else if (cpuint_ip & CAUSEF_IP2)
	{
		/* For shared interrupts */
		extint_ip = REG32(GIMR) & REG32(GISR);

		if (extint_ip & UART0_IP)
		{
			/* UART 0 */
			do_IRQ(UART0_IRQ);
		}
#if 0
		else if (extint_ip & TC1_IP)
		{
			do_IRQ(TC1_IRQ);
		}
		/* currently we do not use uart1	*/
		else if (extint_ip & UART1_IP)
		{
			do_IRQ(UART1_IRQ);
		}
		else
		{
			prom_printf("Unknown Interrupt2:%x\n",extint_ip);
			REG32(GISR) = extint_ip;
		}
#endif
	}
	write_c0_status((read_c0_status()|(ST0_USED_IM)));
#endif
}

#endif
