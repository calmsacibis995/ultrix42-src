#ifndef lint
static char *sccsid = "@(#)proc.c	4.3	(ULTRIX)	4/11/91";
#endif

/************************************************************************
 *									*
 *			Copyright (c) 1988 by				*
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

#include	"crash.h"
#include	<sys/proc.h>

extern struct uarea u;

int gt_proc_slt(s)
	register char *s;
{
	int index;
	int addr;
	
	switch(*s) {
		case '@' :
		case '*' :
			sscanf(++s, "%x", &addr);
			index =	 getindex(addr, procbuckets, PROCBUCKETS);
			if(index == -1)
				printf("addr 0x%x is not a proc\n", addr);
			break;
		case '#' :
			sscanf(++s, "%d", &addr);
			index = pid_to_slot(addr);
			if(index == -1)
				printf("cannot find slot %d\n", addr);
			break;
		default:
			if(isdigit(*s))
				index = atoi(s);
			else {
				printf("%s is an invalid token\n", s);
				index = -1;
			}
	}
	return(index);
}

int
pid_to_slot(pid)
	int pid;
{
	int slot;

	for (slot = 0; slot < tab[PROC_T].ents; slot++) {
		if (proctab[slot].p_pid == pid)
			return (slot);
	}
	return (-1);
}


prproc(c, run, all)
	int	c;
	int	run;
	int	all;
{
	char	ch;
	struct proc *p;
	int flag;
	if(c < 0)
		return;
	p = &proctab[c];
	if(!all && p->p_stat == SIDL)
		return;
	if(run && p->p_stat != SRUN)
		return;
	switch(p->p_stat) {
		case NULL :
			ch = ' ';
			break;
		case SSLEEP :
			ch = 's';
			break;
		case SWAIT :
			ch = 'w';
			break;
		case SRUN :
			ch = 'r';
			break;
		case SIDL :
			ch = 'i';
			break;
		case SZOMB :
			ch = 'z';
			break;
		case SSTOP :
			ch = 't';
			break;
		default :
			ch = '?';
	}
	printf("%3d %c", c, ch);
	printf(" %5u %5u %5u %4u %3u %3u %6X ",
	    p->p_pid, p->p_ppid, p->p_pgrp, p->p_uid,
	    p->p_pri & 0377, p->p_cpu & 0377, p->p_sig);
	if (p->p_wchan == 0) 
		printf("         ");
	else {
/*		printf(" %8.8x ", p->p_wchan); */
		praddr(p->p_wchan);
	}
	flag =  p->p_sched|p->p_vm|p->p_type|p->p_trace|p->p_file;
	printf("\t%s%s%s%s%s%s%s%s%s%s%s%s",
	    flag & SLOAD ? " in" : "",
	    flag & SSYS ? " sys" : "",
	    flag & SLOCK ? " lck" : "",
	    flag & SSWAP ? " swap" : "",
	    flag & STRC ? " trace" : "",
	    flag & SWTED ? " trace" : "",
	    flag & SULOCK ? " ulck" : "",
	    flag & SPAGE ? " page" : "",
	    flag & SKEEP ? " keep" : "",
	    flag & SOMASK ? " omask" : "",
	    flag & SWEXIT ? " exit" : "",
	    flag & SPHYSIO ? " physio" : "");
	printf("%s%s%s%s%s%s%s%s%s%s%s%s",
	    flag & SVFORK ? " vfork" : "",
	    flag & SVFDONE ? " vfd" : "",
	    flag & SNOVM ? " novm" : "",
	    flag & SPAGI ? " pagi" : "",
	    flag & SSEQL ? " seq" : "",
	    flag & SUANOM ? " rand" : "",
	    flag & STIMO ? " timo" : "",
#ifdef SOUSIG
	    flag & SOUSIG ? " osig" : "",
#else
	    "",
#endif
	    flag & SOWEUPC ? " owe" : "",
	    flag & SSEL ? " sel" : "",
	    flag & SLOGIN ? " login" : "",
	    flag & SPTECHG ? " ptchg" : "");
#ifdef mips
	printf("%s%s%s",
	       flag & SXCTDAT ? " xdat" : "",
	       flag & SFIXADE ? " fix" : "",
	       flag & SIDLEP ? " idlep" : "");
#endif
	printf("%s%s%s\n",
	       flag & SNOCLDSTP ? " chldstp" : "",
	       flag & SNFSPGN ? " nfspg" : "",
	       flag & SLKDONE ? " lkdn" : "");

}

void
printprochd()
{
	printf("SLT S   PID  PPID  PGRP  UID  PY CPU   SIGS    Event Flags\n");
}


void
printptehd() 
{
	printf("   PTE      Frame            Status           Segment Prot\n");}

prproclock(c, run, all)
	int	c;
	int	run;
	int	all;
{
	char	ch;
	struct proc *p;
	struct lock_t *lp, lock;
	int flag;
	if(c < 0)
		return;
	p = &proctab[c];
	if(!all && p->p_stat == NULL)
		return;
	if(run && p->p_stat != SRUN)
		return;
	
	lp= (p->p_hlock);
	while (lp != NULL) {
		printf("%3d %5u ", c, p->p_pid, p->p_hlock);
/*		praddr(lp); */
		if (readmem((char *)&lock, (char *)lp, sizeof(struct lock_t))
		    != sizeof(struct lock_t)) {
			printf("couldn't find lock at 0x%x\n",lp);
			return;
		}
	
		prlock_long(lp, 1);
		lp=lock.l_plock;
	}
}
