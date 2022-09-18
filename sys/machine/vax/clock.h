/*
 * @(#)clock.h	4.1 	Ultrix 7/2/90
 */
/************************************************************************
 *									*
 *			Copyright (c) 1985 by				*
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


/* ------------------------------------------------------------------------
 * Modification History: /sys/vax/clock.h
 *
 * 19-Jun-85 -- jaw
 *	VAX8200 name change.
 *
 * 7 JAN 85 -- jaw
 *	Add support for VAX8200.
 *
 * ------------------------------------------------------------------------
 */


/*
 * VAX clock registers
 */

#define	ICCS_RUN	0x00000001
#define	ICCS_TRANS	0x00000010
#define	ICCS_SS		0x00000020
#define	ICCS_IE		0x00000040
#define	ICCS_INT	0x00000080
#define	ICCS_ERR	0x80000000
	
#define	SECDAY		((unsigned)(24*60*60))		/* seconds per day */
#define	SECYR		((unsigned)(365*SECDAY))	/* per common year */
/*
 * TODRZERO is the what the TODR should contain when the ``year'' begins.
 * The TODR should always contain a number between 0 and SECYR+SECDAY.
 */
#define	TODRZERO	((unsigned)(1<<28))

#define	YRREF		1970
#define	LEAPYEAR(year)	((year)%4==0)	/* good till time becomes negative */

/*
 * Has the time-of-day clock wrapped around?
 */
#define	clkwrap()	(((unsigned)mfpr(TODR) - TODRZERO)/100 > SECYR+SECDAY)

/*
 * Software clock is software interrupt level 8,
 * implemented as mtpr(SIRR, 0x8) in asm.sed.
 */

/* VAX8200 watch chip */

#define V8200_BUSY	0x0001
#define V8200_SETUP	0x000d
#define V8200_ASET	0x0040
#define V8200_BSET	0x000c

#ifndef LOCORE
struct v8200watch {
	short	v8200_secs;
	short	av8200_secs;
	short	v8200_mins;
	short	av8200_mins;
	short	v8200_hours;
	short	av8200_hours;
	short	v8200_wdays;
	short	v8200_mdays;
	short	v8200_months;
	short 	v8200_years;
	short	v8200_acsr;
	short	v8200_bcsr;
	short 	v8200_ccsr;
	short 	v8200_dcsr;
	short	v8200_ram[50];
};

struct v8200watch v8200watch[1];

#endif /* LOCORE */
