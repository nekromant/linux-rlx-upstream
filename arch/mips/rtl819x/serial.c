/*
 * Copyright 2006, Realtek Semiconductor Corp.
 *
 * arch/rlx/rlxocp/serial.c
 *     RLXOCP serial port initialization
 *
 * Tony Wu (tonywu@realtek.com.tw)
 * Nov. 07, 2006
 */

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/serial.h>
#include <linux/serial_core.h>
#include <linux/serial_8250.h>
#include <linux/string.h>

#include <asm/serial.h>

#include <bspchip.h>

