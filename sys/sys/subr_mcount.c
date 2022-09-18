#ifndef lint
static	char	*sccsid = "@(#)subr_mcount.c	4.1	(ULTRIX)	7/2/90";
#endif

/************************************************************************
 *									*
 *			Copyright (c) 1986,88 by			*
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

#ifdef GPROF
#include "../h/gprof.h"
#include "../h/param.h"
#include "../h/systm.h"
#include "../machine/mtpr.h"
#include "../h/cpudata.h"
#include "../h/kmalloc.h"
/*
 * Froms is actually a bunch of unsigned shorts indexing tos
 */
int profiling = PROFILING_OFF;
u_short *froms = NULL;
struct tostruct *tos = NULL;
int tostruct_size = sizeof(struct tostruct);
long tolimit = 0;
#ifdef vax
char	*s_lowpc = (char *)0x80000000;
#endif
extern char etext;
char *s_highpc = &etext;
u_long	s_textsize = 0;
int ssiz = 0;
u_short	*sbuf = NULL;
u_short	*kcount = NULL;
u_long prof_lock = 0;	/* smp lock */
int *Mcount_ptr = NULL;
long Mcount_cpu = 0;
long kprofdebug = 0;
char *mon_buf = "No space for monitor buffer %s\n";

kmstartup()
{
	u_long	fromssize, tossize;

	/*
	 *	round lowpc and highpc to multiples of the density we're using
	 *	so the rest of the scaling (here and in gprof) stays in ints.
	 */
	s_lowpc = (char *)
	    ROUNDDOWN((unsigned)s_lowpc, HISTFRACTION*sizeof(HISTCOUNTER));
	s_highpc = (char *)
	    ROUNDUP((unsigned)s_highpc, HISTFRACTION*sizeof(HISTCOUNTER));
	s_textsize = s_highpc - s_lowpc;
	printf("Profiling kernel, s_textsize=%d [%x..%x]\n",
		s_textsize, s_lowpc, s_highpc);
	ssiz = (s_textsize / HISTFRACTION) + sizeof(struct phdr);
	KM_ALLOC(sbuf, u_short *, ssiz, KM_GPROF, KM_NOWAIT|KM_CLEAR|KM_CALL);
	if (sbuf == 0) {
		printf("No space for monitor buffer(s)\n");
		return;
	}

	fromssize = s_textsize / HASHFRACTION;
	KM_ALLOC(froms, u_short *, fromssize, KM_GPROF, KM_NOWAIT|KM_CLEAR|KM_CALL);
	if (froms == 0) {
		printf("No space for monitor buffer(s)\n");
		KM_FREE(sbuf, KM_GPROF);
		sbuf = 0;
		return;
	}
	tolimit = s_textsize * ARCDENSITY / 100;
	if (tolimit < MINARCS) {
		tolimit = MINARCS;
	} else if (tolimit > 65534) {
		tolimit = 65534;
	}
	tossize = tolimit * sizeof(struct tostruct);
	KM_ALLOC(tos, struct tostruct *, tossize, KM_GPROF, KM_NOWAIT|KM_CLEAR|KM_CALL);
	if (tos == 0) {
		printf("No space for monitor buffer(s)\n");
		KM_FREE(sbuf, KM_GPROF);
		sbuf = 0;
		KM_FREE(froms, KM_GPROF);
		froms = 0;
		return;
	}
	tos[0].link = 0;
	((struct phdr *)sbuf)->lpc = s_lowpc;
	((struct phdr *)sbuf)->hpc = s_highpc;
	((struct phdr *)sbuf)->ncnt = ssiz;
	kcount = (u_short *)(((int)sbuf) + sizeof(struct phdr));
#ifdef notdef
	/*
	 *	profiling is what mcount checks to see if
	 *	all the data structures are ready!!!
	 */
	profiling = PROFILING_ON;	/* patch by hand when you're ready */
#endif
}

#ifdef vax
/*
 *	This routine is massaged so that it may be jsb'ed to.
 *	SMP changes -- The logic here is as follows:
 *	if profiling is off just rsb
 *	else get the lock to proceed (the lock stops recursion)
 *	now test to see if profiling is still on
 *		(adb-ing the kernel turns it off and mcount can turn it off)
 *	if it's off goto done to unlock and rsb
 *	if it's on do your duty....
 *	NOTE: we cannot call an smp subroutine since it will in turn
 *	be compiled with profiling code which would call us back to get
 *	the same lock we're already trying to get. This may mean that
 *	in general there are some lock's that cannot go through the subroutine
 *	calls in a profiled kern (or otherwise).
 *	Hence, we don't follow the smp standards for locks since we must do
 *	our own inline code. (NO SUBROUTINE CALLS IN MCOUNT!)
 *	NOTE2: no variable can be pushed on the stack so make everything
 *	GLOBAL!!!
 */
asm(".text");
asm("#the beginning of mcount()");
asm(".data");
mcount()
{
	register char			*selfpc;	/* r11 => r5 */
	register unsigned short		*frompcindex;	/* r10 => r4 */
	register struct tostruct	*top;		/* r9  => r3 */
	register struct tostruct	*prevtop;	/* r8  => r2 */
	register long			toindex;	/* r7  => r1 */

#ifdef lint
	selfpc = (char *)NULL;
	frompcindex = 0;
#else not lint
#define PUSHR		asm("pushr $0x3f")
#define POPR		asm("popr $0x3f")
	asm("	.text");		/* make sure we're in text space */
	if (profiling == PROFILING_OFF) {
		/* hey if it's off it's not likely to  */
		/* change just because you get the lock */
		/* and if it does so you miss an event */
		/* anyway we have to do this to bootstrap the system */
		asm("	rsb");
	}
	/*
	 *	find the return address for mcount,
	 *	and the return address for mcount's caller.
	 */
	asm("	movl (sp), r11");	/* selfpc = ... (jsb frame) */
	asm("	pushr	$0x01");
	asm("	movl 16(fp), r10");	/* frompcindex =     (calls frame) */
#endif not lint
	/*
	 *	check that we are profiling
	 *	and that we aren't recursively invoked.
	 *	NO CALLS's in here!!!!!!!!!!!!!!!!!!!!!
	 */
	asm("mfpr $0x12,r0");		/* store current ipl in r0 XXX */
	asm("mtpr $0x1f,$0x12");	/* go to ipl extreme XXX */
	asm("mtpr r0,$0x2");		/* save old ipl in ssp reg XXX */
	while(setlock(&prof_lock) == 0);/* 0 is failure to get lock */
#define KOFF profiling=PROFILING_OFF;PUSHR
#define KON  profiling=PROFILING_ON;POPR
	if (kprofdebug) {
		KOFF;printf("mcount: got lock\n");KON;
	}
	/* once you have the lock you must always do a goto (overflow|done) */
	/* no calls's in this routine or we get recursive */
	/* and block on the lock (hey it rhymes!) */
	if (profiling == PROFILING_OFF) {
		if (kprofdebug) {
		    KOFF;printf("mcount: got lock but profiling is off\n");KON;
		}
		/* must do this test as adb and mcount itself can turn it off */
		goto done;
	}
	/*
	 *	check that frompcindex is a reasonable pc value.
	 *	for example:	signal catchers get called from the stack,
	 *			not from text space.  too bad.
	 */
	frompcindex = (unsigned short *)((long)frompcindex - (long)s_lowpc);
	if ((unsigned long)frompcindex > s_textsize) {
		goto done;
	}
	frompcindex =
	    &froms[((long)frompcindex) / (HASHFRACTION * sizeof(*froms))];
	if (kprofdebug) {
		KOFF;printf("mcount: getting cpuindex()\n");KON;
	}

	Mcount_cpu = CURRENT_CPUDATA->cpu_num; /*get the cpu we are on */
	if (kprofdebug) {
		KOFF;printf("mcount: got cpuindex() = %d\n",Mcount_cpu);KON;
	}
	toindex = *frompcindex;	/* first access of r1 after the jsb's above */
	if (toindex == 0) {
		/*
		 *	first time traversing this arc
		 */
		toindex = ++tos[0].link;
		if (toindex >= tolimit) {
			goto overflow;
		}
		*frompcindex = toindex;
		top = &tos[toindex];
		top->selfpc = selfpc;
		Mcount_ptr = &top->count;
		Mcount_ptr[Mcount_cpu] = 1;
		top->link = 0;
		goto done;
	}
	top = &tos[toindex];
	if (top->selfpc == selfpc) {
		/*
		 *	arc at front of chain; usual case.
		 */
		Mcount_ptr = &top->count;
		Mcount_ptr[Mcount_cpu]++;
		goto done;
	}
	/*
	 *	have to go looking down chain for it.
	 *	top points to what we are looking at,
	 *	prevtop points to previous top.
	 *	we know it is not at the head of the chain.
	 */
	for (; /* goto done */; ) {
		if (top->link == 0) {
			/*
			 *	top is end of the chain and none of the chain
			 *	had top->selfpc == selfpc.
			 *	so we allocate a new tostruct
			 *	and link it to the head of the chain.
			 */
			toindex = ++tos[0].link;
			if (toindex >= tolimit) {
				goto overflow;
			}
			top = &tos[toindex];
			top->selfpc = selfpc;
			Mcount_ptr = &top->count;
			Mcount_ptr[Mcount_cpu] = 1;
			top->link = *frompcindex;
			*frompcindex = toindex;
			goto done;
		}
		/*
		 *	otherwise, check the next arc on the chain.
		 */
		prevtop = top;
		top = &tos[top->link];
		if (top->selfpc == selfpc) {
			/*
			 *	there it is.
			 *	increment its count
			 *	move it to the head of the chain.
			 */
			Mcount_ptr = &top->count;
			Mcount_ptr[Mcount_cpu]++;
			toindex = prevtop->link;
			prevtop->link = top->link;
			top->link = *frompcindex;
			*frompcindex = toindex;
			goto done;
		}
	}
overflow:
	profiling = PROFILING_OFF;
	clearlock(&prof_lock);	
	/* unlock before calling printf which we shouldn't trace now */
	printf("mcount: tos overflow tolimit = %d\n",tolimit);
	asm("mfpr $0x2,r0");	/* retrieve old ipl from ssp XXX */
	asm("mtpr r0,$0x12");	/* set old ipl XXX */
	asm("	popr	$0x01");
	asm("	rsb");
done:
	if (kprofdebug) {
		KOFF;printf("mcount: about to clear lock\n");KON;
		KOFF;DELAY(250000);KON;
	}
	clearlock(&prof_lock);
	asm("mfpr $0x2,r0");	/* retrieve old ipl from ssp XXX */
	asm("mtpr r0,$0x12");	/* set old ipl XXX */
	asm("	popr	$0x01");
	asm("	rsb");
}
asm(".text");
asm("#the end of mcount()");
asm(".data");
#endif vax
#endif GPROF
