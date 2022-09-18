
/*
 *	@(#)ioasic.h	4.1	(ULTRIX)	8/9/90
 */

/************************************************************************
 *									*
 *			Copyright (c) 1988 by				*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any	other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or	reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************
 *
 * ioasic.h
 *
 * Modification history
 *
 * 20-Feb-1990 - pgt (Philip Gapuz Te)
 * 	created file.
 *
 */

/* IOASIC Registers */
#define  IOC_COMM1_DMA_BASE      (PHYS_TO_K1(0x1c040030))
#define  IOC_COMM2_DMA_BASE      (PHYS_TO_K1(0x1c040050))
#define  IOC_SSR                 (PHYS_TO_K1(0x1c040100))
#define  IOC_SIR                 (PHYS_TO_K1(0x1c040110))

#define  SCC_INTR (SIR_COMM1_XINT | SIR_COMM1_RINT | \
		   SIR_COMM1_XERROR | SIR_COMM1_RERROR | \
		   SIR_COMM2_XINT | SIR_COMM2_RINT | \
		   SIR_COMM2_XERROR | SIR_COMM2_RERROR | \
		   SIR_SCC0 | SIR_SCC1)

/* IOASIC System Interrupt Register bits */
#define  SIR_COMM1_XINT      0x80000000
#define  SIR_COMM1_XERROR    0x40000000
#define  SIR_COMM1_RINT      0x20000000
#define  SIR_COMM1_RERROR    0x10000000
#define  SIR_COMM2_XINT      0x08000000
#define  SIR_COMM2_XERROR    0x04000000
#define  SIR_COMM2_RINT      0x02000000
#define  SIR_COMM2_RERROR    0x01000000
#define  SIR_SCC1            0x00000080
#define  SIR_SCC0            0x00000040

/* IOASIC System Support Register bits */
#define  SSR_COMM1_XEN       0x80000000
#define  SSR_COMM1_REN       0x40000000
#define  SSR_COMM2_XEN       0x20000000
#define  SSR_COMM2_REN       0x10000000
#define  SSR_RESET           0x00000800

#define  IOC_RD(reg, var)        (var) = (*(u_long *)(reg))
#define  IOC_WR(reg, var)        *(u_long *)(reg) = (u_long)(var) 

#define  IOC_SET(reg, mask)      {   u_long temp; \
				     IOC_RD((reg), temp); \
				     IOC_WR((reg), temp|(mask)); \
				     }

#define  IOC_CLR(reg, mask)      {   u_long temp; \
				     IOC_RD((reg), temp); \
                                     IOC_WR((reg), temp & ~(mask)); \
				     }

