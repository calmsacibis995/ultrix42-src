/* static	char	*sccsid = "@(#)pcb.h	4.1	(ULTRIX)	7/2/90"; */

/************************************************************************
 *									*
 *			Copyright (c) 1984,1986,1989 by			*
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

/************************************************************************
 *
 * Modification history: /sys/vax/pcb.h
 *
 * 13-Jun-89 -- jmartin
 *	Add hardware PCB fields defined by ECO 119, PCB_PRVCPU and
 *	PCB_ASN.
 *
 * 18 Jul 85 -- jrs
 *	Added cpu index to pcb
 *
 * 29 Nov 84 -- jrs
 *	Changed aston(0 and astoff() to only set ast in processor
 *	register.  The store into the pcb has been eliminated as
 *	it caused an extraneous context switch.
 *
 *	Derived from 4.2bsd labelled:
 *		pcb.h	6.2	84/07/31
 *
 ************************************************************************/

/*
 * VAX process control block
 */

#ifndef	LOCORE

struct pcb
{
	int	pcb_ksp; 	/* kernel stack pointer */
	int	pcb_esp; 	/* exec stack pointer */
	int	pcb_ssp; 	/* supervisor stack pointer */
	int	pcb_usp; 	/* user stack pointer */
	int	pcb_r0; 
	int	pcb_r1; 
	int	pcb_r2; 
	int	pcb_r3; 
	int	pcb_r4; 
	int	pcb_r5; 
	int	pcb_r6; 
	int	pcb_r7; 
	int	pcb_r8; 
	int	pcb_r9; 
	int	pcb_r10; 
	int	pcb_r11; 
	int	pcb_r12; 
#define	pcb_ap pcb_r12
	int	pcb_r13; 
#define	pcb_fp pcb_r13
	int	pcb_pc; 	/* program counter */
	int	pcb_psl; 	/* program status longword */
	struct  pte *pcb_p0br; 	/* seg 0 base register */
	int	pcb_p0lr; 	/* seg 0 length register and astlevel */
	struct  pte *pcb_p1br; 	/* seg 1 base register */
	int	pcb_p1lr; 	/* seg 1 length register and pme */
	unsigned
		pcb_prvcpu:8,	/* previous CPU on which process ran */
		pcb_asn:24;	/* address space number */
			
/*
 * Software pcb (extension)
 */
	int	pcb_szpt; 	/* number of pages of user page table */
	int	*pcb_sswap;
	int	pcb_sigc[4];
	int	pcb_cpundx;	/* index to processor data array */
};

#define	aston() \
	{ \
		mtpr(ASTLVL, ASTLVL_USER); \
	}

#define	astoff() \
	{ \
		mtpr(ASTLVL, ASTLVL_NONE); \
	}

#endif /*	LOCORE */

#define	AST_NONE	0x04000000	/* ast level */
#define	AST_USER	0x03000000	/* ast for user mode */

#define	ASTLVL_NONE	4
#define	ASTLVL_USER	3

#define	AST_CLR		0x07000000
#define	PME_CLR		0x80000000

#define NO_PRVCPU	255
