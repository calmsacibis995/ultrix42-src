/* @(#)trap.h	4.2  (ULTRIX)        9/6/90    */

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

/************************************************************************
 * Modification History: /sys/vax/trap.h
 *
 * 4-Sep-90 -- dlh
 *	added vector disable fault code
 *
 * 23-Jan-89 -- jmartin
 *	Introduce a new trap type NOACCESS for protection fault on read.
 ************************************************************************/

/*
 * Trap type values
 */

/* The first three constant values are known to the real world <signal.h> */
#define	T_RESADFLT	0		/* reserved addressing fault */
#define	T_PRIVINFLT	1		/* privileged instruction fault */
#define	T_RESOPFLT	2		/* reserved operand fault */
/* End of known constants */
#define	T_BPTFLT	3		/* bpt instruction fault */
#define	T_XFCFLT	4		/* xfc instruction fault */
#define	T_SYSCALL	5		/* chmk instruction (syscall trap) */
#define	T_ARITHTRAP	6		/* arithmetic trap */
#define	T_KDB_ENTRY	T_ARITHTRAP	/* arithmetic trap */
#define	T_ASTFLT	7		/* software level 2 trap (ast deliv) */
#define	T_SEGFLT	8		/* segmentation fault */
#define	T_PROTFLT	9		/* protection fault */
#define	T_TRCTRAP	10		/* trace trap */
#define	T_COMPATFLT	11		/* compatibility mode fault */
#define	T_PAGEFLT	12		/* page fault */
#define	T_TABLEFLT	13		/* page table fault */
#define T_NOACCESS	14		/* protection fault on read */
#define T_VDISFLT	15		/* vector processor disabled fault */
