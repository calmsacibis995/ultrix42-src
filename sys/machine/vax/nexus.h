/*
 * @(#)nexus.h	4.1	ULTRIX  7/2/90
 */
/************************************************************************
 *									*
 *			Copyright (c) 1983,86,87,88 by			*
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
 * Modification History: /sys/vax/nexus.h
 * 15-Feb-88 -- fred (Fred Canter)
 *	Also define MAXNNEXUS as 32 for VAX420 (CVAXstar/PVAX).
 *
 * 20-Apr-87 -- afd
 *	Added comments to make it clear that CVAXQ does not use nexus space.
 *	Changed name CVAXQ to VAX3600 for Mayfair.
 *
 * 06-Mar-87 -- afd
 *	Set "MAXNNEXUS" to 1 if ONLY "CVAXQ" is defined.  It does
 *	not use "nexus" space, but rather has its own map names in spt.s.
 *
 * 14-Apr-86 -- afd
 *	Put MVAX into ifdef's on how big to make "MAXNNEXUS".
 *
 * 18-Mar-86 -- jaw  add routines to cpu switch for nexus/unibus addreses
 *		     also got rid of some globals like nexnum.
 *		     ka8800 cleanup.
 *
 * 19-Jun-85 -- jaw
 *	VAX8200 name change.
 *
 * 20-Mar-85 - JAW
 *	Changes for support of the VAX8200 were merged in.
 *
 * 12-Mar-85 -tresvik
 *	Adjusted NEXUS count for VAX8600 to be per IOA
 *
 * 27-Feb-85 -tresvik
 *	Changes for support of the VAX8600 were merged in.
 *
 *  6 Nov 84 -- rjl
 *	Support for MicroVAX-II implimentation of q-bus and local 
 *	registers.
 *
 *  2 Jan 84 -- jmcg
 *	Added support for MicroVAX Q22 bus.
 *
 *  2 Jan 84 --jmcg
 *	Derived from Ultrix baseline sources; heritage based on
 *	4.2BSD labeled:
 *		nexus.h	6.1	83/08/0
 *
 * ------------------------------------------------------------------------
 */

/*
 * Information about nexus's.
 *
 * Each machine has an address of backplane slots (nexi).
 * Each nexus is some type of adapter, whose code is the low
 * byte of the first word of the adapter address space.
 * At boot time the system looks through the array of available
 * slots and finds the interconnects for the machine.
 *
 * We stretch this a little for MicroVAX-II. It has a local register
 * space for the toy, q-bus map registers and other goodies. Because
 * these things are similar int concept to adapters we use this space 
 * to map them in also.
 *
 * VAX3600 (ka650) does not use nexus space. If only VAX3600 is defined we set
 * MAXNNEXUS to 1 (so we don't get a compiler error about 0 sized nexus array).
 */
#define	NEXSIZE	0x2000
#define QNEXSIZE 0x40000


#define NNEX8800 64
#define NEX8800(io,i)	((short *)(0x20000000+NEXSIZE*i+(io<<25)))
#define	NNEX8600	16		/* per I/o controller */
#define	NEX8600(io,i)	((short *)(0x20000000+NEXSIZE*i+(io<<25)))
#define NNEX8200 16
#define NEX8200(i)	((short *)(0x20000000+NEXSIZE*i))
#define	NNEX780	16
#define	NEX780(i)	((short *)(0x20000000+NEXSIZE*i))
#define	NNEX750	16
#define	NEX750(i)	((short *)(0xf20000+NEXSIZE*i))
#define	NNEX730	16
#define	NEX730(i)	((short *)(0xf20000+NEXSIZE*i))
#define	NNEXUVI	1
#define	NEXUVI		((short *) -1)
#define NEXUVII		((short *)0x20080000)


#ifndef LOCORE
struct	nexus {
	union nexcsr {
		long	nex_csr;
		u_char	nex_type;
	} nexcsr;
	long	nex_pad[NEXSIZE / sizeof (long) - 1];
};
#ifdef	KERNEL
extern struct nexus nexus[];
#endif /*  KERNEL */
#endif /*	LOCORE */

/*
 * Bits in high word of nexus's.
 */
#define	SBI_PARFLT	(1<<31)		/* sbi parity fault */
#define	SBI_WSQFLT	(1<<30)		/* write sequence fault */
#define	SBI_URDFLT	(1<<29)		/* unexpected read data fault */
#define	SBI_ISQFLT	(1<<28)		/* interlock sequence fault */
#define	SBI_MXTFLT	(1<<27)		/* multiple transmitter fault */
#define	SBI_XMTFLT	(1<<26)		/* transmit fault */

#define	NEX_CFGFLT	(0xfc000000)

#ifndef LOCORE
/* 780 and 8600 only */ 
#define	NEXFLT_BITS \
"\20\40PARFLT\37WSQFLT\36URDFLT\35ISQFLT\34MXTFLT\33XMTFLT"
#endif

#define	NEX_APD		(1<<23)		/* adaptor power down */
#define	NEX_APU		(1<<22)		/* adaptor power up */

#define	MBA_OT		(1<<21)		/* overtemperature */

#define	UBA_UBINIT	(1<<18)		/* unibus init */
#define	UBA_UBPDN	(1<<17)		/* unibus power down */
#define	UBA_UBIC	(1<<16)		/* unibus initialization complete */

/*
 * Types for nex_type.
 */
#define	NEX_ANY		0		/* pseudo for handling 11/750
					   and 11/730 */
#define NEX_MS780C	0x00		/* All combinations of MS780C */
#define	NEX_MBA		0x20		/* Massbus adaptor */
#define	NEX_UBA0	0x28		/* Unibus adaptor */
#define	NEX_UBA1	0x29		/* 4 flavours for 4 addr spaces */
#define	NEX_UBA2	0x2a
#define	NEX_UBA3	0x2b
#define	NEX_DR32	0x30		/* DR32 
user i'face to SBI */
#define	NEX_CI		0x38		/* CI adaptor */
#define	NEX_MPM0	0x40		/* Multi-port mem */
#define	NEX_MPM1	0x41		/* Who knows why 4 different ones ? */
#define	NEX_MPM2	0x42
#define	NEX_MPM3	0x43
#define NEX_MS780E	0x60		/* All combinations of MS780E */
#define NEX_MEM750	0xff		/* Fudge an adapter type for MS750 */
#define NEX_MEM730	0xfe		/* Fudge an adapter type for MS730 */
#define NEX_Q22		0xfd		/* Fudge an adapter type for MicroVAX */
