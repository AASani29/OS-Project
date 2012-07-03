/*
 * Derived from BHyVe (svn 237539).
 * Adapted for CertiKOS by Haozhong Zhang at Yale.
 */

/*-
 * Copyright (c) 2011 NetApp, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY NETAPP, INC ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL NETAPP, INC OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD$
 */

#include <sys/debug.h>
#include <sys/mmu.h>
#include <sys/types.h>
#include <sys/x86.h>

#include "ept.h"
#include "vmx_msr.h"

#define	EPT_PWL4(cap)			((cap) & (1ULL << 6))
#define	EPT_MEMORY_TYPE_WB(cap)		((cap) & (1ULL << 14))
#define	EPT_PDE_SUPERPAGE(cap)		((cap) & (1ULL << 16))	/* 2MB pages */
#define	EPT_PDPTE_SUPERPAGE(cap)	((cap) & (1ULL << 17))	/* 1GB pages */
#define	INVVPID_SUPPORTED(cap)		((cap) & (1ULL << 32))
#define	INVEPT_SUPPORTED(cap)		((cap) & (1ULL << 20))

#define	INVVPID_ALL_TYPES_MASK		0xF0000000000ULL
#define	INVVPID_ALL_TYPES_SUPPORTED(cap)	\
	(((cap) & INVVPID_ALL_TYPES_MASK) == INVVPID_ALL_TYPES_MASK)

#define	INVEPT_ALL_TYPES_MASK		0x6000000ULL
#define	INVEPT_ALL_TYPES_SUPPORTED(cap)					\
	(((cap) & INVEPT_ALL_TYPES_MASK) == INVEPT_ALL_TYPES_MASK)

#define	EPT_PG_RD			(1 << 0)
#define	EPT_PG_WR			(1 << 1)
#define	EPT_PG_EX			(1 << 2)
#define	EPT_PG_MEMORY_TYPE(x)		((x) << 3)
#define	EPT_PG_IGNORE_PAT		(1 << 6)
#define	EPT_PG_SUPERPAGE		(1 << 7)

#define	EPT_ADDR_MASK			((uint64_t)-1 << 12)

static uint64_t page_sizes_mask;

int
ept_init(void)
{
	int page_shift;
	uint64_t cap;

	cap = rdmsr(MSR_VMX_EPT_VPID_CAP);

	/*
	 * Verify that:
	 * - page walk length is 4 steps
	 * - extended page tables can be laid out in write-back memory
	 * - invvpid instruction with all possible types is supported
	 * - invept instruction with all possible types is supported
	 */
	if (!EPT_PWL4(cap) ||
	    !EPT_MEMORY_TYPE_WB(cap) ||
	    !INVVPID_SUPPORTED(cap) ||
	    !INVVPID_ALL_TYPES_SUPPORTED(cap) ||
	    !INVEPT_SUPPORTED(cap) ||
	    !INVEPT_ALL_TYPES_SUPPORTED(cap))
		return 1;

	/* Set bits in 'page_sizes_mask' for each valid page size */
	page_shift = PAGESHIFT;
	page_sizes_mask = 1UL << page_shift;		/* 4KB page */

	page_shift += 9;
	if (EPT_PDE_SUPERPAGE(cap))
		page_sizes_mask |= 1UL << page_shift;	/* 2MB superpage */

	page_shift += 9;
	if (EPT_PDPTE_SUPERPAGE(cap))
		page_sizes_mask |= 1UL << page_shift;	/* 1GB superpage */

	return 0;
}
