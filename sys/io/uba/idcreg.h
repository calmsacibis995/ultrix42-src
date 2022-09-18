

/* @(#)idcreg.h	4.1	(ULTRIX)	7/2/90	*/

/************************************************************************
 *									*
 *			Copyright (c) 1984, 1986 by			*
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
 * idcreg.h	 6.1	 07/29/83
 *
 * Modification history
 *
 * IDC/RL02/R80 registers/data structures and definitions
 *
 * 10-Feb-87 - pmk
 *	Changed initialization of idc_softc arrays from NRB to 4.
 *
 * 16-Apr-86 - ricky palmer
 *	Derived from 4.2BSD labeled: idcreg.h	 6.1	 83/07/29.
 *	Converted definitions to hex values and cleaned up comments. V2.0
 *	Moved idc_softc data structure here. V2.0
 *
 */

/* Register device structure */
struct idcdevice
{
	int	idccsr; 		/* Control status register	*/
	int	idcbar; 		/* Bus address register 	*/
	int	idcbcr; 		/* Byte count register		*/
	int	idcdar; 		/* Disk address register	*/
	int	idcmpr; 		/* Multi-purpose register	*/
	int	idceccpos;		/* Ecc position register	*/
	int	idceccpat;		/* Ecc pattern register 	*/
	int	idcreset;		/* Master reset register	*/
};

/* Control status register definitions (idccsr)
#define IDC_NOP 	0x00		/* No operation 		*/
#define IDC_ECS_NONE	0x00		/* No data error		*/
#define IDC_DRDY	0x01		/* Drive ready			*/
#define IDC_WCHK	0x02		/* Write check data		*/
#define IDC_GETSTAT	0x04		/* Get status			*/
#define IDC_SEEK	0x06		/* Seek 			*/
#define IDC_RHDR	0x08		/* Read header			*/
#define IDC_WRITE	0x0a		/* Write data			*/
#define IDC_READ	0x0c		/* Read data			*/
#define IDC_RNOHCHK	0x0e		/* Read data w/o header check	*/
#define IDC_FUNC	0x0e		/* Function code		*/
#define IDC_IE		0x40		/* Interrupt enable		*/
#define IDC_CRDY	0x80		/* Controller ready		*/
#define IDC_DS		0x300		/* Drive select bits		*/
#define IDC_OPI 	0x400		/* Operation incomplete 	*/
#define IDC_DCK 	0x800		/* Data check			*/
#define IDC_DLT 	0x1000		/* Data late			*/
#define IDC_HNF 	IDC_DLT 	/* Header not found		*/
#define IDC_NXM 	0x2000		/* Non-existent memory		*/
#define IDC_DE		0x4000		/* Drive error			*/
#define IDC_ERR 	0x8000		/* Composite error		*/
#define IDC_ATTN	0xf0000 	/* Attention bits		*/
#define IDC_ECS_HARD	0x200000	/* Hard ecc error		*/
#define IDC_ECS_SOFT	0x300000	/* Soft ecc error		*/
#define IDC_ECS 	0x300000	/* R80 ecc status		*/
#define IDC_SSEI	0x400000	/* R80 skip sector error inhibit*/
#define IDC_SSE 	0x800000	/* R80 skip sector error	*/
#define IDC_IR		0x1000000	/* Interrupt request		*/
#define IDC_MTN 	0x2000000	/* Maintenance			*/
#define IDC_R80 	0x4000000	/* Selected disk is R80 	*/
#define IDC_ASSI	0x8000000	/* Automatic skip sector inhibit*/
#define IDC_TOI 	0x10000000	/* Time out inhibit		*/
#define IDC_HARD	(IDC_NXM|IDC_DE)		/* Hard error	*/
#define IDCCSR_BITS	"\20\35TOI\34ASSI\33R80\
			 \32MTN\31IR\30SSE\27SSEI\
			 \26ECS1\25ECS0\24ATN3\
			 \23ATN2\22ATN1\21ATN0\
			 \20ERR\17DE\16NXM\15DLT\
			 \14DCK\13OPI\12DS1\11DS0\
			 \10CRDY\7IE\4F2\3F1\2F0\1DRDY" /* Status bits  */

/* Drive get status register definitions for rb02 and rb80 (idcmpr) */
#define IDCGS_M 	0x01		/* Mark, must be 1		*/
#define IDCGS_GS	0x02		/* Get status, must be 1	*/
#define IDCGS_RST	0x08		/* Reset			*/
#define IDCGS_GETSTAT	(IDCGS_RST|IDCGS_GS|IDCGS_M)	/* Get status	*/
/* rb02 status values */
#define IDCDS_STATE	0x07		/* Drive state			*/
#define IDCDS_BH	0x08		/* Brush home			*/
#define IDCDS_HO	0x10		/* Heads out			*/
#define IDCDS_CO	0x20		/* Cover open			*/
#define IDCDS_HS	0x40		/* Head select			*/
#define IDCDS_DSE	0x100		/* Drive select error		*/
#define IDCDS_VC	0x200		/* Volume check 		*/
#define IDCDS_WGE	0x400		/* Write gate error		*/
#define IDCDS_SPD	0x800		/* Spindle error		*/
#define IDCDS_SKTO	0x1000		/* Seek timeout 		*/
#define IDCDS_WL	0x2000		/* Write lock			*/
#define IDCDS_HCE	0x4000		/* Head current error		*/
#define IDCDS_WDE	0x8000		/* Write data error		*/
#define IDCRB02DS_BITS	"\10\20WDE\17HCE\16WL\
			 \15SKTO\14SPD\13WBE\
			 \12VC\11DSE\7HS\6CO\
			 \5HO\4BH\3STC\2STB\1STA"       /* Status bits  */
/* rb80 status values */
#define IDCDS_FLT	0x100		/* Fault			*/
#define IDCDS_PLGV	0x200		/* Plug valid			*/
#define IDCDS_SKE	0x400		/* Seek error			*/
#define IDCDS_ONCY	0x800		/* On cylinder			*/
#define IDCDS_DRDY	0x1000		/* Driver ready 		*/
#define IDCDS_WTP	0x2000		/* Write protect		*/
#define IDCRB80DS_BITS	"\10\16WTP\15DRDY\
			 \14ONCY\13SKE\12PLGV\
			 \11FLT\5SEC4\4SEC3\
			 \3SEC2\2SEC1\1SEC0"            /* Status bits  */

/* Layout definitions */
#define NRB02SECT	40		/* RB02 sectors/track		*/
#define NRB02TRK	2		/* RB02 tracks/cylinder 	*/
#define NRB02CYL	512		/* RB02 cylinders/disk		*/
#define NRB80SECT	31		/* RB80 sectors/track		*/
#define NRB80TRK	14		/* RB80 tracks/cylinder 	*/
#define NRB80CYL	559		/* RB80 cylinders/disk		*/

struct idc_softc {
	int	sc_bcnt;		/* Number of bytes to transfer	*/
	int	sc_resid;		/* Total bytes to transfer	*/
	int	sc_ubaddr;		/* Unibus address of data	*/
	short	sc_unit;		/* Unit doing transfer		*/
	short	sc_softas;		/* Software att. summary bits	*/
	short	sc_offline[4];		/* Initialized idcslave offl.	*/
	union idc_dar {
		long	dar_l;
		u_short dar_w[2];
		u_char	dar_b[4];
	} sc_un;			/* Prototype disk address reg.	*/
	long	sc_flags[4];		/* General device flags 	*/
	long	sc_category_flags[4];   /* Category device flags	*/
	u_long	sc_softcnt[4];		/* Soft error count		*/
	u_long	sc_hardcnt[4];		/* Hard error count		*/
	char	sc_device[DEV_SIZE][4];	/* Device type string	*/
};

