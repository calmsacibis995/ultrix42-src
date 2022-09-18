/*	@(#)systm.h	4.2  (ULTRIX)        9/4/90     */

/************************************************************************
 *									*
 *			Copyright (c) 1983,86,87 by			*
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

/* ---------------------------------------------------------------------
 * Modification History 
 *
 * 11 Dec 89 jaa
 *	dynamic swap related variables curviras, maxviras, 
 *	novas, lowswap and totalswap are not used
 *
 * 09-Nov-89 jaw
 *	remove asymmetric syscall support.
 *
 * 12 Jun 89 - gg
 *	Added dynamic swap related variables curviras, maxviras, 
 *	novas, lowswap and totalswap.
 *
 * 14 Dec 87 -jaa
 *	calloc() and wmemall() are no more.
 * 
 * 02-Apr-86 -- jrs
 *	Add mp-safe flag to syscall table
 * 
 * 23 Jul 85 -- jrs
 *	Remove sched variables for multicpu case
 * 
 * 20 Jan 86  -- pmk
 *	Added rundown variable; system going down; set in boot,
 *	checked in sleep; to resolve recursive panic problem.
 * 
 * 13 Feb 84 --rjl
 *	Added bootdevice variable for MicroVAX I auto boot code.
 *	Allows the system to run off of the filesystem it was booted
 *	from.
 * ---------------------------------------------------------------------
 */
#ifdef KERNEL
#include "../h/ansi_compat.h"
#else
#include <ansi_compat.h>
#endif

/*
 * Random set of variables
 * used by more than one
 * routine.
 */
int	hand;			/* current index into coremap used by daemon */
extern	char version[];		/* system version */

/*
 * Nblkdev is the number of entries
 * (rows) in the block switch. It is
 * set in binit/bio.c by making
 * a pass over the switch.
 * Used in bounds checking on major
 * device numbers.
 */
int	nblkdev;

/*
 * Number of character switch entries.
 * Set by cinit/prim.c
 */
int	nchrdev;

int	nswdev;			/* number of swap devices */
int	mpid;			/* generic for unique process id's */
char	runin;			/* scheduling flag */
char	runout;			/* scheduling flag */
int	swapself;		/* a process has requested to be swapped out */
int	slavehold;		/* global slave lockout */
char	kmapwnt;		/* kernel map want flag */
int 	rundown;		/* system going down flag, set in boot */
				/* checked in sleep */
int	maxmem;			/* actual max memory per process */
int	physmem;		/* physical memory on this CPU */

int	nswap;			/* size of swap space */
int	updlock;		/* lock for sync */
daddr_t	rablock;		/* block to be read ahead */
int	rasize;			/* size of block in rablock */
extern	int intstack[];		/* stack for interrupts */
dev_t	rootdev;		/* device of the root */
dev_t	dumpdev;		/* device to take dumps on */
long	dumplo;			/* offset into dumpdev */
dev_t	swapdev;		/* swapping device */

#ifdef __vax
extern	int icode[];		/* user init code */
extern	int szicode;		/* its size */
#endif
#ifdef __mips
extern int icode();		/* user init code */
extern char eicode[];		/* address of end of icode */
#endif /* __mips */

daddr_t	bmap();
unsigned max();
unsigned min();
int	memall();
int	uchar(), schar();
int	vmemall();
swblk_t	vtod();

/*
 * Structure of the system-entry table
 */
extern struct sysent
{
	int	sy_narg;		/* total number of arguments */
	int	(*sy_call)();		/* handler */
} sysent[];

char	*panicstr;
int	wantin;
int	boothowto;		/* reboot flags, from console subsystem */
int 	bootdevice;		/* To allow the system to run from boot disc */

int	selwait;

extern	char vmmap[];		/* poor name! */

/* casts to keep lint happy */
#define	insque(q,p)	_insque((caddr_t)q,(caddr_t)p)
#define	remque(q)	_remque((caddr_t)q)
#define	queue(q,p)	_queue((caddr_t)q,(caddr_t)p)
#define	dequeue(q)	_dequeue((caddr_t)q)
