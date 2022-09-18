/* static	char	*sccsid = "@(#)pcb.m	4.1	(ULTRIX)	7/2/90" */

/************************************************************************
 *									*
 *			Copyright (c) 1985,1986,1989 by			*
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

/*-----------------------------------------------------------------------
 * 13-Jun-89 -- jmartin
 *	Add hardware PCB fields defined by ECO 119, PCB_PRVCPU and
 *	PCB_ASN.
 *
 * 21-Jul-85 -- jrs
 *	Added cpu index field
 *
 *	Derived from 4.2 bsd labelled:
 *	 	pcb.m	6.1	83/07/29
 *
 *-----------------------------------------------------------------------
 */

/*
 * VAX process control block
 */
	.set	PCB_KSP,	0
	.set	PCB_ESP,	4
	.set	PCB_SSP,	8
	.set	PCB_USP,	12
	.set	PCB_R0,		16
	.set	PCB_R1,		20
	.set	PCB_R2,		24
	.set	PCB_R3,		28
	.set	PCB_R4,		32
	.set	PCB_R5,		36
	.set	PCB_R6,		40
	.set	PCB_R7,		44
	.set	PCB_R8,		48
	.set	PCB_R9,		52
	.set	PCB_R10,	56
	.set	PCB_R11,	60
	.set	PCB_R12,	64
	.set	PCB_R13,	68
	.set	PCB_PC,		72
	.set	PCB_PSL,	76
	.set	PCB_P0BR,	80
	.set	PCB_P0LR,	84
	.set	PCB_P1BR,	88
	.set	PCB_P1LR,	92
	.set	PCB_PRVCPU,	96
	.set	PCB_ASN,	97
/*
 * Software pcb extension
 */
	.set	PCB_SZPT,	100	/* number of user page table pages */
	.set	PCB_SSWAP,	104	/* flag for non-local goto */
	.set	PCB_SIGC,	108	/* signal trampoline code */
	.set	PCB_CPUNDX,	124	/* cpu index */
