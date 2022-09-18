
/* @(#)rlreg.h	4.1	(ULTRIX)	7/2/90	*/

/************************************************************************
 *									*
 *			Copyright (c) 1984, 1986, 1989 by		*
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
 * rlreg.h	6.1	07/29/83
 *
 * Modification history
 *
 * RLU211/RLV211/RL02 registers/data structures and definitions
 *
 * 13-Aug-89 - Tim Burke
 *	Added drive state definitions; returned from get drive status.
 *
 * 10-Feb-87 - pmk
 *	Changed rl_softc initialization of arrays from NRL to 4.
 *
 * 16-Apr-86 - ricky palmer
 *	Derived from 4.2BSD labeled: rlreg.h	6.1	83/07/29.
 *	Converted definitions to hex values and cleaned up comments. V2.0
 *	Moved rl_softc data structure here. V2.0
 *
 */

/* Register device structure */
struct rldevice {
	short	rlcs;			/* Control status		*/
	u_short rlba;			/* Bus address			*/
	union { 			/* Disk address 		*/
		u_short seek;		/* Disk seek address		*/
		u_short rw;		/* Disk read/write address	*/
		u_short getstat;	/* Get disk status command	*/
	} rlda;
	union { 			/* Multi-purpose register	*/
		u_short getstat;	/* Get status			*/
		u_short readhdr;	/* Read header			*/
		u_short rw;		/* Read/write word count	*/
	} rlmp;
};

/* Control status register definitions (rlcs) */
#define RL_NOOP 	0x00		/* No-operation 		*/
#define RL_DRDY 	0x01		/* When set indicates drive rdy.*/
#define RL_WCHECK	0x02		/* Write check			*/
#define RL_GETSTAT	0x04		/* Get status			*/
#define RL_SEEK 	0x06		/* Seek 			*/
#define RL_RHDR 	0x08		/* Read header			*/
#define RL_WRITE	0x0a		/* Write data			*/
#define RL_READ 	0x0c		/* Read data			*/
#define RL_RDNCK	0x0e		/* Read data without hdr check	*/
#define RL_BAE		0x30		/* BUS address bits 16 & 17	*/
#define RL_IE		0x40		/* Interrupt enable		*/
#define RL_CRDY 	0x80		/* Controller ready		*/
#define RL_DS0		0x100		/* Drive select 0		*/
#define RL_DS1		0x200		/* Drive select 1		*/
#define RL_OPI		0x400		/* Operation incomplete 	*/
#define RL_DCRC 	0x800		/* CRC error occurred		*/
#define RL_DLT		0x1000		/* Data late or header not found*/
#define RL_NXM		0x2000		/* Non-existent memory		*/
#define RL_DE		0x4000		/* Selected drive flagged error */
#define RL_ERR		0x8000		/* Composite error		*/
#define RL_DCRDY	(RL_DRDY | RL_CRDY)	/* Drive ready		*/
#define RLCS_BITS	"\10\20ERR\17DE\
			 \16NXM\15DLT\14DCRC\
			 \13OPI\1DRDY"          /* Status bits          */
#define RLCS_STATUS	(RL_ERR|RL_CRDY)	/* Read status command	*/
#define RLCS_STATOK	(RL_CRDY)		/* Status ok check	*/

/* Disk address seek register definitions (rlda.seek) */
#define RLDA_HSU	0x00		/* Upper head select		*/
#define RLDA_LOW	0x01		/* Lower cylinder seek		*/
#define RLDA_HGH	0x05		/* Higher cylinder seek 	*/
#define RLDA_HSL	0x10		/* Lower head select		*/
#define RLDA_CA 	0xff80		/* Cylinder address		*/

/* Disk address read/write register definitions (rlda.rw) */
#define RLDA_HST	0x00		/* Upper head select		*/
#define RLDA_SA 	0x3f		/* Sector address		*/
#define RLDA_HSB	0x40		/* Lower head select		*/

/* Disk address getstat register definitions (rlda.getstat) */
#define RL_GSTAT	0x03		/* Get status			*/
#define RL_RESET	0x0b		/* Get status with reset	*/

/* Disk multipurpose getstat register definitions (rlmp.getstat) */
#define RLMP_STA	0x01		/* See state defs below		*/
#define RLMP_STB	0x02		/* See state defs below         */
#define RLMP_STC	0x04		/* See state defs below         */
#define RLMP_BH 	0x08		/* Set when brushes are home	*/
#define RLMP_HO 	0x10		/* Set when heads over the disk */
#define RLMP_CO 	0x20		/* Set when cover open		*/
#define RLMP_HS 	0x40		/* Indicates selected head(0,1) */
#define RLMP_DT 	0x80		/* Indicates drive type(0,1)	*/
#define RLMP_DSE	0x100		/* Set on mult. drive selection */
#define RLMP_VC 	0x200		/* Set on pack mounted		*/
#define RLMP_WGE	0x400		/* Write gate error		*/
#define RLMP_SPE	0x800		/* Spin speed error		*/
#define RLMP_SKTO	0x1000		/* Seek time out error		*/
#define RLMP_WL 	0x2000		/* Set on protected drive	*/
#define RLMP_CHE	0x4000		/* Current head error		*/
#define RLMP_WDE	0x8000		/* Write data error		*/
#define RLER_BITS	"\10\20WDE\17CHE\
			 \16WL\15SKTO\14SPE\
			 \13WGE\12VC\11DSE\
			 \10DT\7HS\6CO\5HO\
			 \4BH\3STC\2STB\1STA"   /* Error bits           */
/*
 * The state of the drive is specified in the 3 bits STA, STB, STC
 */
#define RLMP_STATMASK 	(RLMP_STA|RLMP_STB|RLMP_STC)
#define RLMP_UNLOADED 	0
#define RLMP_SPINUP 	(RLMP_STA)
#define RLMP_BRUSH 	(RLMP_STB)
#define RLMP_LDHEADS 	(RLMP_STA | RLMP_STB)
#define RLMP_SEEK 	(RLMP_STC)
#define RLMP_LOCKON 	(RLMP_STA | RLMP_STC)
#define RLMP_UNLDHEAD 	(RLMP_STB | RLMP_STC)
#define RLMP_SPINDOWN 	(RLMP_STA | RLMP_STB | RLMP_STC)

/* Disk multipurpose getstat register definitions (rlmp.readhdr) */
#define RLMP_SA 	0x3f		/* Sector address		*/
#define RLMP_CA 	0xff80		/* Cylinder address		*/

/* Disk multipurpose read/write register definitions (rlmp.rw) */
#define RLMP_WC 	0x1fff		/* Word count 2's complement	*/

/* Layout definitions */
#define NRLCYLN 	512		/* Number of cylinders per disk */
#define NRLTRKS 	2		/* Number of tracks per cylinder*/
#define NRLSECT 	40		/* Number of sectors per track	*/
#define NRLBPSC 	256		/* Bytes per sector		*/

struct	rl_softc {
	int	rl_softas;		/* Attention sumary, seeks pend.*/
	int	rl_ndrive;		/* Number of drives on cont.	*/
	int	rl_wticks;		/* Monitor time for function	*/
	long	sc_flags[4];		/* General device flags		*/
	long	sc_category_flags[4];   /* Category device flags	*/
	u_long	sc_softcnt[4];	        /* Soft error count		*/
	u_long	sc_hardcnt[4];	        /* Hard error count		*/
	char	sc_device[DEV_SIZE][4]; /* Device type string		*/
};

