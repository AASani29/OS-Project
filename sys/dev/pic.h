/*
 * Hardware definitions for the 8259A Programmable Interrupt Controller (PIC).
 *
 * Copyright (C) 1997 Massachusetts Institute of Technology
 * See section "MIT License" in the file LICENSES for licensing terms.
 *
 * Derived from the MIT Exokernel and JOS.
 * Adapted for PIOS by Bryan Ford at Yale University.
 */

#ifndef _KERN_DEV_PIC_H_
#define _KERN_DEV_PIC_H_

#ifndef _KERN_
#error "This is a kernel header file; do not include it in userspace programs."
#endif

#define MAX_IRQS	16	// Number of IRQs

/* I/O Addresses of the two 8259A programmable interrupt controllers */
#define IO_PIC1		0x20	/* Master (IRQs 0-7) */
#define IO_PIC2		0xA0	/* Slave (IRQs 8-15) */

#define IRQ_SLAVE	2	/* IRQ at which slave connects to master */

#define IO_ELCR1	0x4d0
#define IO_ELCR2	0x4d1

#ifndef __ASSEMBLER__

#include <sys/types.h>
#include <sys/x86.h>

extern uint16_t irq_mask_8259A;

void pic_init(void);
void pic_setmask(uint16_t mask);
void pic_enable(int irq);
void pic_eoi(void);
void pic_reset(void);

#endif // !__ASSEMBLER__

#endif // !_KERN_DEV_PIC_H_
