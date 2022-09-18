
/*
 * 	@(#)spreg.h	4.1	(ULTRIX)	7/2/90
 */

/************************************************************************
 *									*
 *			Copyright (c) 1984, 1986, 1987, 1989 by		*
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
 *   This software is  derived	from  software	received  from	the	*
 *   University    of	California,   Berkeley,   and	from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is	subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or	reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/*
 * spreg.h
 *
 * Modification history
 *
 * MicroVAX 2000/3100 pseudo driver for user devices
 *
 * The "sp" driver is a place holder for a user supplied device driver.
 * It provides the linkages needed to add a user written driver to the
 * system. It also contains comments about the driver to system interface.
 * The "sp" driver is not a tutorial on writing device drivers.
 *
 * 03-Dec-89	Fred Canter
 *	Created this sp driver header file.
 *
 */

/*
 * Definitions, pte maps, and virtual address pointer, for
 * mapping a user device's I/O space by the "sp" pseudo driver.
 *
 * Three address maps are provided (others could be added):
 *   CSR: physical address of device's control and status registers.
 *   ROM: physical address of device's firmware rom.
 *   IOS: physical address of any other I/O space that needs to be mapped.
 *
 * NOTE: The following are examples. The real maps are driver dependent.
 *       See also sys/machine/vax/spt.s.
 *       Please only map the number of pages actually needed.
 *       Set the SPXXX_PAGES to zero if the entire map is not needed.
 *	 Please do not use get_sys_ptes(). Could exhaust kmemmap.
 */
#define	SPCSR_PHYSADR	((char *)(0x39000000))	/* CSR physical address */
#define	SPCSR_PAGES	2			/* number of pages to map */
extern	struct pte SPCSRmap[][512];		/* Page tables */
extern	char spcsr[][512*NBPG];			/* Virtual address pointer */

#define	SPROM_PHYSADR	((char *)(0x20140000))	/* ROM physical address */
#define	SPROM_PAGES	1			/* number of pages to map */
extern	struct pte SPROMmap[][512];		/* Page tables */
extern	char sprom[][512*NBPG];			/* Virtual address pointer */

#define	SPIOS_PHYSADR	((char *)(0x39008000))	/* IOS physical address */
#define	SPIOS_PAGES	1			/* number of pages to map */
extern	struct pte SPIOSmap[][512];		/* Page tables */
extern	char spios[][512*NBPG];			/* Virtual address pointer */

/*
 * KA410/KA420 interrupt controller bit definitions
 *
 * Used to access the interrupt controller registers:
 *	INT_REQ	- Interrupt request register
 *	INT_CLR	- Interrupt clear register
 *	INT_MSK	- Interrupt mask register
 *
 * The available interrupt vectors are:
 *	NS	0x254
 *	VF	0x244
 *	VS	0x248
 * All devices interrupt at IPL 0x14.
 *
 * A option board (plugged into the video option connector on the system
 * board) may use one of the following combinations of interrupts:
 *	NS
 *	VF
 *	VS
 *	VF and VS
 *
 * NOTE: to use the VF interrupt the device driver must set the I3OPT
 *	 bit in the VDC_SEL register (see code in spprobe() in sp.c).
 */

#define	SINT_NS	0x10	/* Option board, network controller secondary	      */
#define	SINT_VF	0x08	/* VDCSEL = 0: system board, video end of frame       */
			/* VDCSEL = 1: option board, primary		      */
#define	SINT_VS	0x04	/* Option board, secondary			      */

/*
 * Device register structure
 *
 * Example of device registers (from sh driver).
 *
 */

struct spdevice {
	struct {
		char low;
		char high;
	} csr;	 			/* 0 SOP_CSR  Cntrl status reg	*/
	union {
		short rbuf;		/* 2 SOP_RBUF Receive buffer	*/
		char rxtimer;		/* 2 SOP_RTIM Receive timer	*/
	} run;
	u_short lpr;			/* 4 SOP_LPR  Line param reg	*/
	union {				/* 6 SOP_DATA			*/
		short fifodata; 	/*	Fifo data		*/
		struct	{
			char fifosize;	/*	Fifo size		*/
			char stat;	/*	Line status		*/
		} fs;
	} fun;
	u_short lnctrl;			/* 8 SOP_CNTL Line control	*/
	u_short tbuffad1;		/* A NOT USED Xmit. bufr addr 1	*/
	struct	{
		char low;
		char high;
	} tbuffad2;			/* C SOP_TXA  Xmit. bufr addr 2	*/
	u_short tbuffcnt;		/* E NOT USED Xmit. bufr count	*/
};

/*
 * Example of a partial software control structure.
 * Most of softc is driver dependent.
 */

struct	sp_softc {
	long	sc_flags[8];		/* Flags (one per line) 	*/
	long	sc_category_flags[8];	/* Category flags (one per line)*/
	u_long	sc_softcnt[8];	 	/* Soft error count total	*/
	u_long	sc_hardcnt[8];	 	/* Hard error count total	*/
	char	sc_device[DEV_SIZE][8];	/* Device type string		*/
};
