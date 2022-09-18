/*
 *	@(#)scb.h	4.2	(ULTRIX)	9/4/90
 */

/************************************************************************
 *									*
 *			Copyright (c) 1989 by				*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any  other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   This software is  derived  from  software  received  from  the	*
 *   University    of   California,   Berkeley,   and   from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is  subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/*
 * Revision History:
 * 25-May-89	Bill Burns
 *	Created the file.
 */

#ifdef KERNEL
#include "../h/ansi_compat.h"
#else
#include <ansi_compat.h>
#endif

/*
 * VAX System control block layout
 */

struct scb {
	int	(*scb_stray)(); 	/* reserved */
	int	(*scb_machchk)();	/* machine chack */
	int	(*scb_kspinval)();	/* KSP invalid */
	int	(*scb_powfail)();	/* power fail */
	int	(*scb_resinstr)();	/* reserved instruction */
	int	(*scb_custinst)();	/* XFC instr */
	int	(*scb_resopnd)();	/* reserved operand */
	int	(*scb_resaddr)();	/* reserved addr mode */
	int	(*scb_acv)();		/* access control violation */
	int	(*scb_tnv)();		/* translation not valid */
	int	(*scb_tracep)();	/* trace pending */
	int	(*scb_bpt)();		/* breakpoint instr */
	int	(*scb_compat)();	/* compatibility mode fault */
	int	(*scb_arith)(); 	/* arithmetic fault */
	int	(*scb_stray2)();
	int	(*scb_stray3)();
	int	(*scb_chmk)();		/* CHMK instr */
	int	(*scb_chme)();		/* CHME instr */
	int	(*scb_chms)();		/* CHMS instr */
	int	(*scb_chmu)();		/* CHMU instr */
	int	(*scb_sbisilo)();	/* SBI silo compare */
	int	(*scb_cmrd)();		/* corrected mem read data */
	int	(*scb_sbialert)();	/* SBI alert */
	int	(*scb_sbiflt)();	/* SBI fault */
	int	(*scb_wtime)(); 	/* memory write timeout */
	int	(*scb_stray4[8])();
	int	(*scb_soft[15])();	/* software interrupt */
	int	(*scb_timer)(); 	/* interval timer interrupt */
	int	(*scb_stray5)();
	int	(*scb_ctr1)();	       /* serial line units */
	int	(*scb_ctx1)();
	int	(*scb_ctr2)();	       /* serial line units */
	int	(*scb_ctx2)();
	int	(*scb_ctr3)();	       /* serial line units */
	int	(*scb_ctx3)();
	int	(*scb_stray6[4])();
	int	(*scb_csdr)();		/* console storage receive */
	int	(*scb_csdx)();		/* console storage transmit */
	int	(*scb_ctr)();		/* console terminal receive */
	int	(*scb_ctx)();		/* console terminal transmit */
	int	(*scb_ipl14[16])();	/* device interrupts IPL 14 */
	int	(*scb_ipl15[16])();	/*   "		"    IPL 15 */
	int	(*scb_ipl16[16])();	/*   "		"    IPL 16 */
	int	(*scb_ipl17[16])();	/*   "		"    IPL 17 */
	struct	scb_pg {
		int	(*scb_pageoff[128])();	/* Unibus 1 device intr */
	}scb_page[32];
};

#define SCB_UNIBUS_PAGEOFFSET(ubanumber) \
		(((((int) (UNIvec + (128*ubanumber))) << 1) \
		- (((int) &scb.scb_stray) << 1)) >> 1)


#define SCB_UNIBUS_PAGE(ubanumber) \
	(UNIvec + (128*ubanumber))

#ifdef KERNEL
extern	struct scb scb;
/* scb.scb_ubaint is the same as UNIvec */
#endif

#ifdef __vax
#define scbentry(f, how)		((int (*)())(((int)f)+how))
#endif /* __vax */
#ifdef __mips
#define scbentry(f, how)		((int (*)())((int)f))
#endif /* __mips */

#define SCB_KSTACK	0
#define SCB_ISTACK	1
#define SCB_WCS 	2
#define SCB_HALT	3
