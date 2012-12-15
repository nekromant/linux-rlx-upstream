#include <bspcpu.h>
#include <bspchip.h>

#define UART0_BASE		0xB8002000
#define UART0_THR		(UART0_BASE + 0x000)
#define UART0_FCR		(UART0_BASE + 0x008)
#define UART0_LSR       (UART0_BASE + 0x014)
#define TXRST			0x04
#define CHAR_TRIGGER_14	0xC0
#define LSR_THRE		0x20
#define TxCHAR_AVAIL	0x00
#define TxCHAR_EMPTY	0x20

void putc(char c)
{
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
