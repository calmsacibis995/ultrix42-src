#ifndef lint
static char *sccsid = "@(#)stacktrace.c	4.1	(ULTRIX)	7/17/90";
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

#include	"../crash.h"
#include	<sys/proc.h>
#include	<machine/pte.h>
#include	<sys/cpudata.h>
#include	<sys/vm.h>
#include	<frame.h>
#include	<stdio.h>

#define KERNELSTACK (0x7fffffff)

prtrace(vars,procslot,cpu)
	int	procslot;
	int	vars;			/*  want vars */
	int	cpu;
{
	register int pc, *aap;
	int *afp;
	int ret;
	struct	Symbol *name, *search();
	int *stack;
	struct frame *frame;
	int regmask;
	int *regptr;
	int *oldreg;
	int regcount;
	int *parmlist;
	int parmcnt;
	int *ksp;
	int varcnt;
	int i = 0;
	int neednl = 1;
	int doingintstack = 0;
	char *intstack = NULL;
	u_int space;
	struct Symbol *scbend;	

	if(procslot > tab[PROC_T].ents) {
		printf("%d out of range, use Process Table Slot\n", procslot);
		return(-1);
	}
	if(procslot == -1) {
		struct Symbol *scb;
		struct Symbol *symsrch();
		struct cpudata cpudata;
		int curproc;
		
		get_cpudata(&cpudata, cpu);
		afp = (int *) cpudata.cpu_stack;
		printf("afp 0x%x\n istack ",afp, cpudata.cpu_istack);
		if(afp == NULL) {
			printf("cannot trace running process\n");
			return(-1);
		}
		aap = afp - sizeof(char *);
		ksp = afp;
		pc = (int) afp + sizeof(int);
#ifdef DEBUG
		printf("aap 0x%x afp 0x%x pc 0x%x\n", aap, afp, pc);
#endif
		curproc = getindex((char *)cpudata.cpu_proc, procbuckets,
		    PROCBUCKETS);
#ifdef DEBUG
		printf("proc addr 0x%x\n", cpudata.cpu_proc);
		printf("process slot %d\n", curproc);
#endif
		if((ret=getuarea(curproc)) == -1) {
			error("bad read of uarea");
			return(-1);
		}
	} else {
		if((ret=getuarea(procslot)) == -1) {
			error("bad read of uarea");
			return(-1);
		}
		else if(ret == SWAPPED) {
			error("process is swapped");
			return(-1);
		}
		aap = (int *) U.u_pcb.pcb_r12;
		afp = (int *) U.u_pcb.pcb_fp;
		pc = (int) U.u_pcb.pcb_pc;
		ksp = (int *) U.u_pcb.pcb_ksp;
#ifdef DEBUG
		printf("aap 0x%x afp 0x%x pc 0x%x ksp 0x%x\n", aap, afp,
		pc, ksp);
#endif
	}
	doingintstack = (int) ((int)afp & 0x80000000);
	if(doingintstack) {
		char *malloc();
		scbend = symsrch("_scbend");		
		intstack = malloc(NISP * NBPG);
		readmem(intstack, (int)scbend->s_value, NISP * NBPG);
		/* point us to the end of the interrupt stack */
		stack = ((int *) intstack + NISP * NBPG);
		space = scbend->s_value;
		afp = (int *) ((int)intstack + ((int)afp - (int)space));
		aap = (int *) ((int)intstack + ((int)aap) - (int)space);
		ksp = (int *) ((int)intstack + ((int)ksp - (int)space));
#ifdef DEBUG
		printf("on intstack at 0x%x scbend 0x%x\n", intstack,
		    scbend->s_value);
#endif
	} else {
		/* point us to the end of the kernel stack */
		stack = (int *) ((char *)&U + (UPAGES * 512));
		space = 0x7fffffff;
		/*
		 * we have the entire uarea in U. the actual displacement
		 * of fp (ap, and pc) is stack - (7fffffff - fp) (I think)
		 */
		afp = (int *) (stack - ((int *)space - afp)) - 1;
		aap = (int *) (stack - ((int *)space - aap));
		ksp = (int *) (stack - ((int *)space - ksp)) - 1;
	}
	frame = (struct frame *) afp;
#ifdef DEBUG
		printf("adjusted aap 0x%x afp 0x%x pc 0x%x ksp 0x%x\n",
		aap, afp, pc, ksp);
#endif

#ifdef DEBUG
	if(doingintstack) {
		int k = 0;
		int *i;
		
		i = (int *) intstack;
		while(i < (int *) (intstack + 0xa00)) {
			printf("0x%x(%4x):   ", scbend->s_value + (u_int)i -
			    (u_int)intstack, (u_int)i);
			for(k = 0; k < 4; k++, i++)
				printf("%08x ", *i);
			printf("\n");
		}
		fflush(stdout);
	}
	{ 
		int *i, k = 0;
		i = (int*)((int)&U + UPAGES * NBPG);
		printf("bottom of stack 0x%x absolute top 0x%x\n",
		    &U.u_stack[UPAGES * NBPG / 4], U.u_stack);
		printf("0x%x(%4x):   ", 0x7ffffffc - k, i);
		for(;k < 1000; i--, k++) {
			printf("%08x ",*i);
			if ((k % 4) == 3)
				printf("\n0x%x(%4x):   ", 0x7ffffffc - k * 4,
			 	i);		
		}
		printf("\n");fflush(stdout);
	}
	printf("frame at 0x%x mask 0%o ap 0x%x fp 0x%x pc 0x%x\n",
	    frame, frame->fr_mask, frame->fr_savap, frame->fr_savfp,
	    frame->fr_savpc); 
#endif
	name = search((unsigned)pc);
	printf("from %s+0x%x\n", name ? name->s_name : "?",
	name ? frame->fr_savpc - name->s_value : -1);
	while(!((u_int)frame & 0x80000000) && frame && 
	    (frame->fr_savfp != NULL)) {
		name = search((unsigned)pc);
#ifdef DEBUG
		printf ("handler 0x%x psw %x mask %x %s ap %x fp %x pc %x\n",
		    frame->fr_handler, frame->fr_psw, frame->fr_mask,
		    frame->fr_s ? "calls" : "callg", frame->fr_savap,
		    frame->fr_savfp, frame->fr_savpc);
#endif
		parmlist = ((int *) ((int) frame + sizeof(struct frame)
		     + (biton(frame->fr_mask) * sizeof(int))));
		parmcnt = *parmlist++;
		printf("%s(", name ? name->s_name : "?");
		while(parmcnt--) {
			int t;
			t= *parmlist++;
			printf("%x",t);
			if ((vars < 0) && (t&0x80000000)) {
				printf(" (");
				praddr(t);
				printf(") ");
			}
			printf("%s", parmcnt ? ",": "");
		}
		name = search(frame->fr_savpc);
		printf(") from %s+0x%x\n", name ? name->s_name :
		    "?", frame->fr_savpc - name->s_value),
		regmask = frame->fr_mask;
/*		oldreg = (int *)((frame + 1) + biton(regmask)); */
		regptr = (int *) &U.u_pcb.pcb_r11;
		oldreg = ((int *) ((int) frame + sizeof (struct frame)
		    + ((biton(frame->fr_mask) - 1) * sizeof(int))));
#ifdef DEBUG
		printf("pulling registers from 0x%x\n", oldreg);
#endif
		regcount = 11;
		i = 0;
		if(vars && (regmask & 0xfff)) {
			neednl = 0;
			while(regmask & 0xfff) {
				if(regmask & 0x800) {
					printf("  R%02d 0x%08x ", regcount, *regptr);
					if ((vars < 0) && (*regptr&0x80000000)) {
						printf("(");
						praddr(*regptr);
						printf(") ");
					}
					*regptr = *oldreg--;
					i++;
				}
				regcount--;
				regptr--;
				regmask <<= 1;
				if((i == (vars<0 ? 2: 4)) && (regmask & 0xfff)) {
					i = 0;
					printf("\n");
				} else
					printf("%s", vars < 0 ? "\t" : "");
				}
			printf("\n");
		}
		i = 0;
		pc = frame->fr_savpc;
		varcnt = (int) ((int *) frame - ksp);
#ifdef DEBUG
		printf("ksp 0x%x frame 0x%x vars %d\n", ksp, frame, varcnt);
#endif
		if(vars && varcnt) {
			printf(" local variables\n");
			ksp = (int *) frame;
			neednl = 0;
			while(varcnt--) {
				int t;
				t= *--ksp;
				printf("0x%08x ",t);
				if ((vars < 0) && (t&0x80000000)) {
					printf("(");
					praddr(t);
					printf(") ");
				}
				if((i++ == (vars<0 ? 1 : 3)) && (varcnt)) {
					i = 0;
					printf("\n");
				} else 
					printf("\t");
			}
			printf("\n");
		}
		printf("\n");
		if (neednl <= 0) printf("\n");
		ksp = parmlist;
		if(doingintstack && !(frame->fr_savfp & 0x80000000)) {
			/* toggle between int stack and ksp */
			doingintstack = 0;
			printf("Off interrupt stack\n");
#ifdef DEBUG
			printf("frame (toggle) savap 0x%x savfp 0x%x\n",
			    frame->fr_savap, frame->fr_savfp);
#endif
			stack = (int *) ((int)&U + (UPAGES * 512));
			space = 0x7fffffff;
			afp = (int *) ((int)stack - ((int)space -
			    (int)frame->fr_savfp));
			aap = (int *) ((int)stack - ((int)space -
			    (int)frame->fr_savap));
			ksp = (int *) afp; /*???*/
			free((char *)intstack);
			frame = (struct frame *) afp;
#ifdef DEBUG
			printf("toggle stack 0x%x fp 0x%x ap 0x%x ksp 0x%x\n",
			    stack, afp, aap, ksp);
#endif
		} else {
			if(doingintstack) 
				frame = (struct frame *)
				    ((int)intstack +
				    ((int)frame->fr_savfp - (int)space));
			else {
				frame = (struct frame *) ((char *) stack - 
				    ((char *) space - (char *)frame->fr_savfp));
				if((int *) frame < U.u_stack)
					frame = NULL;	/* stack switch */
			}
		}
		frame = (struct frame *) ((int) frame & ~0x3);
#ifdef DEBUG
		printf("new frame at 0x%x savap 0x%x fp 0x%x pc 0x%x\n", 
		    frame, frame->fr_savap, frame->fr_savfp, frame->fr_savpc);
#endif
		fflush(stdout);
	}
	return(-1);
}

