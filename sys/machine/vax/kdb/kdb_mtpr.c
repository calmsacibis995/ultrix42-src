#ifndef lint
static	char	*sccsid = "@(#)kdb_mtpr.c	4.1	ULTRIX	7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1984 by				*
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
/*
 *	UNIX debugger
 *
 *	Oct-06-88	gmm
 *	Update the P[01][BL]R registers correctly in getpcb(). Otherwise
 *	a process's context is not fully mapped correctly
 *
 *	Jul-19-88	gmm
 *	Store all register information in noproc_usr if no proc present
 *	while entering kdb
 *
 *	Jun-03-88	gmm
 *	Check if the processor has a process context while entering kdb. If
 * 	there is no process context, do not update the user pcb with 
 *	register values. This prevents some of the panics on :c
 *	Jim Woodward's changes to the routine kdb_chgprot() to make it work
 *	correctly.
 */

#include "../machine/mtpr.h"
#include "../machine/reg.h"
#include "../machine/psl.h"
#include "../machine/pte.h"
#include "../machine/cons.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
/* pcb.h is included by user.h */
#include "../h/user.h"
#include "../h/smp_lock.h"
#include "../h/cpudata.h"
#include "../h/proc.h"
#include "mac.h"
#include "mode.h"
#include "../machine/trap.h"
extern MAP kdb_datmap, kdb_txtmap;
extern int *kdb_psl_ptr, *kdb_pc_ptr, *kdb_regs_ptr;
extern ADDR kdb_userpc;
extern lbolt;

extern struct user *uptr;
struct proc *kdb_firstprocp;
struct	user kdbusr, noproc_usr;
extern REGLIST kdb_reglist[];

/* 
 * Flush any garbage from the console.
 * Should really be in kdb_misc.c but that file
 * doesn't get run through inline and i'm lazy.
 */
kdb_flush_input()
{
	int i = 20;
	register c; 

	while ((c=(mfpr(RXCS)&RXCS_DONE)) == 0 && i--)
		;
	if (c) c = mfpr(RXDB)&0177;  /* flush char */
}

getpcb(pte)
struct pte *pte;
{
	register int i;
	register struct pte *map,*mp;
	register REGPTR p;
	register int *pcbptr;
	/* This is used to change who we are mapping */

	/* following code lifted from uaccess() in sys/vm_subr.c */
	mp = map = &Kdbmap;
	for (i = 0; i< UPAGES; i++) {
		*(int *)map = 0;
		map->pg_pfnum = pte[i].pg_pfnum;
		map++;
	}
	vmaccess(mp, (caddr_t)&kdbusr, UPAGES);
	uptr = &kdbusr;
	pcbptr = &uptr->u_pcb;
	p = kdb_reglist;
	for(p = kdb_reglist; p < &kdb_reglist[24]; p++ ) { 
		p->roffs = pcbptr++;
	}

	mtpr(P0BR,uptr->u_pcb.pcb_p0br);
	mtpr(P0LR,uptr->u_pcb.pcb_p0lr);
	mtpr(P1BR,uptr->u_pcb.pcb_p1br);
	mtpr(P1LR,uptr->u_pcb.pcb_p1lr);
	mtpr(TBIA,0);
	cprintf("p0br %X p0lr %X p1br %X p1lr %X\n",
	uptr->u_pcb.pcb_p0br, uptr->u_pcb.pcb_p0lr,
	uptr->u_pcb.pcb_p1br, uptr->u_pcb.pcb_p1lr);
}

setcor()
{
	/*
		 * There is only one image, and no core file,
		 * so just do a structure copy
		 */
	extern struct pte *kdb_sbr, *kdb_slr;
	kdb_datmap.b1 = kdb_datmap.b2 = KERNOFF - (NBPG*UPAGES);
	kdb_datmap.e1 = kdb_datmap.e2 = 0xffffffff;
	kdb_datmap.f1 = kdb_datmap.f2 = KERNOFF - (NBPG*UPAGES);
	kdb_sbr = (struct pte *)mfpr(SBR);
	kdb_slr = (struct pte *)mfpr(SLR);
	cprintf("kdb_sbr %X kdb_slr %X\n", kdb_sbr, kdb_slr);
}



extern lbolt;
kdb_chgprot(va, prot)
char * va;	/* virtual address */
{
	struct pte *pte, *sbr;
	unsigned page;
	int old_protection;

	pte = 	&Sysmap[btop(((int) va) & ~0x80000000)];
	old_protection = *(int *)pte & PG_PROT;
	*(int *)pte &= ~PG_PROT;
	*(int *)pte |= prot;
	mtpr(TBIS,va);
	return (old_protection);
}

readregs()
{
	/*
		 * Load values from the trap into the pcb
		 * of the currently executing process.
		 * This should only be called on entering kdb.
		 * kdb_regs_ptr points to the (0 of the trap) frame.
		 */

	register struct cpudata *pcpu;
	pcpu = CURRENT_CPUDATA;  /* kdb assumed to be running on PRIMARY */
	uptr = &u;
	kdb_firstprocp = u.u_procp;
	if (pcpu->cpu_noproc) {
		cprintf("No process context exists. Use $p to get a process context\n");
		if(pcpu->cpu_proc)
			cprintf("Last running proc = 0x%X, p_addr = 0x%X\n",pcpu->cpu_proc,pcpu->cpu_proc->p_addr);
		uptr = &noproc_usr;
	}
	else {
		uptr = &u;
		kdb_userpc = u.u_pcb.pcb_pc;
	}
	uptr->u_pcb.pcb_ksp = mfpr(KSP);
	uptr->u_pcb.pcb_esp = mfpr(ESP);
	uptr->u_pcb.pcb_ssp = mfpr(SSP);
	uptr->u_pcb.pcb_usp = mfpr(USP);
	uptr->u_pcb.pcb_r0 = kdb_regs_ptr[5]; 
	uptr->u_pcb.pcb_r1 = kdb_regs_ptr[6]; 
	uptr->u_pcb.pcb_r2 = kdb_regs_ptr[7]; 
	uptr->u_pcb.pcb_r3 = kdb_regs_ptr[8]; 
	uptr->u_pcb.pcb_r4 = kdb_regs_ptr[9]; 
	uptr->u_pcb.pcb_r5 = kdb_regs_ptr[10]; 
	uptr->u_pcb.pcb_r6 = kdb_regs_ptr[11]; 
	uptr->u_pcb.pcb_r7 = kdb_regs_ptr[12]; 
	uptr->u_pcb.pcb_r8 = kdb_regs_ptr[13]; 
	uptr->u_pcb.pcb_r9 = kdb_regs_ptr[14]; 
	uptr->u_pcb.pcb_r10 = kdb_regs_ptr[15]; 
	uptr->u_pcb.pcb_r11 = kdb_regs_ptr[16]; 
	uptr->u_pcb.pcb_ap = kdb_regs_ptr[2]; 
	uptr->u_pcb.pcb_fp = kdb_regs_ptr[3]; 
	uptr->u_pcb.pcb_pc = *kdb_pc_ptr;
	uptr->u_pcb.pcb_psl = *kdb_psl_ptr;
	uptr->u_ar0 = &uptr->u_pcb.pcb_r0;
}

struct proc *
get_proc(cpunum)
{
	register struct cpudata *pcpu;
	pcpu = CPUDATA(cpunum);
	if (pcpu->cpu_noproc)
		return(0);
	else
		return(pcpu->cpu_proc);
}


extern kdb_saved_ipl;
struct state	single_step_state;
struct state	kdb_bkpt_state;
struct state	kdb_continue_state;

force_call(type)
{
	/* This gives the state changes room to happen
		 * without trodding the rest of my stack
		 */
	char take_up_space[2048];
	register int *r11;
	struct state *l;
	l = &kdb_continue_state;

	/* This must be done in-line, as otherwise the values
		 * change when you return, and we write over what had
		 * been saved.  Note that we lie and say that the sp
		 * is the same as the frame pointer.  The idea is that
		 * when we return to this spot, we want to continue
		 * executing in the 2048 bytes we reserved in 'take_up_space'.
		 * If we execute other than in that spot, the longjmp to
		 * where we came from (another long jmp) will not succeed.
		 */

	asm("movl ap,r11"); 
	l->kdb_ap = r11;
	asm("movl fp,r11"); 
	l->kdb_fp = r11;
	l->kdb_sp = l->kdb_fp;
	asm("moval fclabel,r11"); 
	l->kdb_pc = r11;

	/*
		 * Move the stack pointer to the end of the 'take_up_space'
		 * variable when we are here on the breakpoint path, so
		 * that the * longjmps will use it instead of trodding our
		 * carefully preserved stack.  The calls just works with
		 * the stack pointer, the potential subsequent return just
		 * uses the frame pointer, so this should work.
		 */
	if (type == T_BPTFLT) {
		r11 = &take_up_space[2044];
		asm("movl r11,sp");
		kdb_longjmp(&kdb_bkpt_state);
	} 
	else if (type == T_TRCTRAP) {
		r11 = &take_up_space[2044];
		asm("movl r11,sp");
		kdb_longjmp(&single_step_state);
	} 
	else if (type == T_KDB_ENTRY) {
		cprintf("type is T_KDB_ENTRY\n"); 
		kern_debugger();
	}
	asm("fclabel:");
}

extern char kdb_redzone[];
kdbred()
{
	register int *ip,i;

	ip = (int *)Sysmap + (btop(((int) &kdb_redzone[512]) & 0x7fffffff));
	*ip &= ~PG_PROT; *ip |= PG_KR;
	mtpr(TBIS, &kdb_redzone[512]);
}
