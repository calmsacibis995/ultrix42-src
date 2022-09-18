#ifndef lint
static char *sccsid = "@(#)u.c	4.5	(ULTRIX)	4/11/91";
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
#include	<machine/pte.h>
#include	<sys/cpudata.h>
#include	<sys/vm.h>
#include	<frame.h>
#include	<stdio.h>

#ifdef vax
#define KERNELSTACK	(0x7fffffff)
#endif vax

struct uarea u;			/* external uarea buffer */

char cmdbuf[CLSIZE*NBPG];	/* global command string buffer */
int argaddr;			/* global vaddr of command space */

prcpudata(option)
	int option;
{
	struct cpudata cpudata;
	int maxcpu;
	int i;

printf("CPU  ID State   Proc      PCB      Swtch    Syscall     Ints  Locks\n");
/*      DDD DDD SSSSSSS  Shhhh 0x88888888 ddddddddd ddddddddd ddddddddd */
	for(i=0;i<MAXCPU;i++) {
		if (get_cpudata(&cpudata, i) == -1) 
			continue;
		printf("%3d %3d %s%s%s%s%s%s",cpudata.cpu_num,
		       i,
		       cpudata.cpu_state & CPU_RUN ? "R":" ",
		       cpudata.cpu_state & CPU_TBI ? "T":" ",
		       cpudata.cpu_state & CPU_PANIC ? "P":" ",
		       cpudata.cpu_state & CPU_SOFT_DISABLE ? "D":" ",
		       cpudata.cpu_state & CPU_BOOT ? "B":" ",
		       cpudata.cpu_state & CPU_STOP ? "S":" ");
#ifdef mips
		printf("%s", cpudata.cpu_inisr == 1 ? "I":" ");
#else
		printf(" ");
#endif
		printf("  %s",cpudata.cpu_noproc ? "*" : " ");
#ifdef mips
		if (cpudata.cpu_noproc)
			printf("%4d ",getindex((char *)cpudata.cpu_idleproc, procbuckets,
					       PROCBUCKETS));
		else
		printf("%4d ",getindex((char *)cpudata.cpu_proc, procbuckets,
		    PROCBUCKETS));

#else
		printf("%4d ",getindex((char *)cpudata.cpu_proc, procbuckets,
		    PROCBUCKETS));
#endif

		printf("0x%8x ", cpudata.cpu_paddr);
		printf("%9d %9d %9d ", cpudata.cpu_switch,
		    cpudata.cpu_syscall,cpudata.cpu_intr);
		if (cpudata.cpu_hlock) {
			praddr(cpudata.cpu_hlock);
			printf("\n");
		} else
			printf("None\n");


		if (cpudata.cpu_panicstr) {
			printf("panicstr: ");
			prod(cpudata.cpu_panicstr, 1, "s");
		}
#ifdef DEBUG
		printf(" stack 0x%x istack 0x%x\n",
		       cpudata.cpu_stack, cpudata.cpu_istack);
#endif

		
				}

}
prlockhead() {
printf("State   Lost       Spin      Won      avg spin  pcnt lost   Cost  Name\n");
/*      SSS dddddddddd dddddddddd dddddddddd   xxx.xxx   xxx.xxx  xxx.xxx */
}
prlock(lp,mflg)
struct lock_t *lp;
int mflg; /* if =0, lock pointer is in local memory, =1 -> kernel memory */
{
	struct lock_t lock;
	struct lock_trace trace;
	float avg_spin, percent_lost;

	if (mflg == 1) {
		if (readmem((char *)&lock, (char *)lp, sizeof(struct lock_t))
		    != sizeof(struct lock_t)) {
			printf("couldn't find lock at 0x%x\n",lp);
			return(1);
		}
	} else 
		bcopy(lp, &lock, sizeof(struct lock_t));
	if (lock.l_lock)
		printf("*L*");
	else
		printf("   ");

	printf(" %10d %10d %10d ",lock.l_lost, lock.l_spin, lock.l_won);
	if (lock.l_lost != 0)
		avg_spin = (float) lock.l_spin/(float)lock.l_lost;
	else
		avg_spin = 0;
			
	if ((lock.l_lost+lock.l_won) != 0)
		percent_lost = 100.0 * (float) lock.l_lost/
			(float) (lock.l_lost + lock.l_won);
	else
		percent_lost = 0;
	printf(" %6.1f %7.2f %7.1f ", avg_spin, percent_lost, 
	       avg_spin*percent_lost);


	if (mflg == 1)
		praddr(lp);
	printf("\n");

}

prlock_long(lp,mflg)
struct lock_t *lp;
int mflg; /* if =0, lock pointer is in local memory, =1 -> kernel memory */
{
	struct lock_t lock;
	struct lock_trace trace;
	struct Symbol *search();
	int i;

	if (mflg == 1) {
		if (readmem((char *)&lock, (char *)lp, sizeof(struct lock_t))
		    != sizeof(struct lock_t)) {
			printf("couldn't find lock at 0x%x\n",lp);
			return(1);
		}
	} else 
		bcopy(lp, &lock, sizeof(struct lock_t));

	printf("Lock - ");
	if (mflg == 1)
		praddr(lp);
	else
		printf("None");
	printf("\n");

	printf("\t Lock State: %s%s Type: %s  Pos %4d IPL: %4d\n",
	       lock.l_wanted ? "W":" ",
	       lock.l_lock ? "L" : " ",
	       lock.l_type == LK_SPIN ? "spin" : lock.l_type == LK_WAIT ? "wait" : "UNKN",
	       lock.l_hierpos, lock.l_ipl);
	printf("\tplock: ");
	if (lock.l_plock == NULL)
		printf(" Null ");
	else
		printf("0x%8x",lock.l_plock);
	printf(" lost %d spin %d won %d\n",
	       lock.l_lost, lock.l_spin, lock.l_won);

}

prlcktrace(tp)
	struct lock_trace *tp;
{
	struct Symbol *search();

	printf("pid %6d  pc 0x%8x (", tp->tr_pid,tp->tr_pc);
	prsym(search(tp->tr_pc), tp->tr_pc);
	printf(") psl 0x%x lock 0x%8x ",tp->tr_psl,tp->tr_lock);
	if (tp->tr_lock)
		praddr(tp->tr_lock);
	else
		printf("None");
	printf("\n");
}
prpcb (p)
	int p;			/* proc slot number */
{
	struct pcb *pcb;
	int i;

	if (p < 0) return;
	if (getuarea(p) == -1) {
		printf ("process %d (slot %d): uarea not in core.\n", 
		proctab[p].p_pid,p);
		return;
	}
	pcb = &U.u_pcb;
	printf("\n");
	printf ("process %d (slot %d)\n",proctab[p].p_pid,p);
	printf("\n");
#ifdef vax
	printf ("ksp:   %8x    ",pcb->pcb_ksp);
	printf ("usp:   %8x    ",pcb->pcb_usp);
	printf ("r0:    %8x\n",  pcb->pcb_r0);
	printf ("r1:    %8x    ",pcb->pcb_r1);
	printf ("r2:    %8x    ",pcb->pcb_r2);
	printf ("r3:    %8x\n",  pcb->pcb_r3);
	printf ("r4:    %8x    ",pcb->pcb_r4);
	printf ("r5:    %8x    ",pcb->pcb_r5);
	printf ("r6:    %8x\n",  pcb->pcb_r6);
	printf ("r7:    %8x    ",pcb->pcb_r7);
	printf ("r8:    %8x    ",pcb->pcb_r8);
	printf ("r9:    %8x\n",  pcb->pcb_r9);
	printf ("r10:   %8x    ",pcb->pcb_r10);
	printf ("r11:   %8x    ",pcb->pcb_r11);
	printf ("ap:    %8x\n",  pcb->pcb_r12);
	printf ("fp:    %8x    ",pcb->pcb_r13);
	printf ("pc:    %8x    ",pcb->pcb_pc);
	printf ("psl:   %8x\n",  pcb->pcb_psl);
	printf ("p0br:  %8x    ",(unsigned)pcb->pcb_p0br);
	printf ("p0lr:  %8x    ",pcb->pcb_p0lr & 0x3fffff);
	printf ("p1br:  %8x\n",  (unsigned)pcb->pcb_p1br);
	printf ("p1lr:  %8x\n",  pcb->pcb_p1lr & 0x3fffff);
#endif vax
#ifdef mips
	printf ("pc:    %8x    ",pcb->pcb_regs[10]);
	printf ("sp:    %8x    ",pcb->pcb_regs[8]);
	printf ("sr:    %8x\n",pcb->pcb_regs[11]);
	for(i=0; i<12; i+=4) {
		printf("r%2d  %8x r%2d  %8x r%2d  %8x r%2d  %8x \n",
		       i,pcb->pcb_regs[i],i+1,pcb->pcb_regs[i+1],
		       i+2,pcb->pcb_regs[i+2],i+3,pcb->pcb_regs[i+3]);
	}
#endif mips
}

pruarea (p)
	int p;			/* proc slot number */
{
	int uresult;
	struct ucred ucred;
	char fflg;
	char *pofile_of;
	int *ofile_of, fp;

	register i;
	register struct nameidata *nd;
	register int *ip;
	
	uresult = getuarea(p);

	if (uresult != -1) {
		printf ("process %d (slot %d):",
		    proctab[uresult].p_pid, uresult);		
		printf ("\n");
		printf ("\n");
	}
	else {
		printf (" uarea not in core.\n");
		return;
	}

/* uarea printing code below is stolen from pstat */
	nd  = &U.u_nd;
	printf("procp\t%9.1x\n", U.u_procp);
	printf("ar0\t%9.1x\n", U.u_ar0);
	printf("comm\t %s\n", U.u_comm);
	printf("arg");
	for (i=0; i<sizeof(U.u_arg)/sizeof(U.u_arg[0]); i++) {
		if (i%5==0) printf("\t");
		printf("%9.1x", U.u_arg[i]);
		if (i%5==4) printf("\n");
	}
	printf("\n");
	printf("ap\t%9.1x\n", U.u_ap);
	printf("qsave");
	for (i=0; i<sizeof(label_t)/sizeof(int); i++) {
		if (i%5==0) printf("\t");
		printf("%9.1x", U.u_qsave.val[i]);
		if (i%5==4) printf("\n");
	}
	printf("\n");
	printf("error\t%9.1x\n", U.u_error);
	printf("r_val?\t%9.1x %9.1x\n", U.u_r.r_val1, U.u_r.r_val2);
	printf("eosys\t%9.1x\n", U.u_eosys);
/* credentials is now km_alloc-ed -- have to go get them!!! - rr */
	lseek(mem,sysvad(U.u_cred),0);
	read(mem,(char *)&ucred,sizeof(struct ucred));
	printf("\tref %d\n",ucred.cr_ref);
	printf("\tuid\t%d\tgid\t%d\n",ucred.cr_uid,ucred.cr_gid);
	printf("\truid\t%d\trgid\t%d\n",ucred.cr_ruid,ucred.cr_rgid);
	printf("\tgroups\t");
	for (i=0;i<NGROUPS;i++) {
		if (ucred.cr_groups[i] == -1) continue;
		printf("%d",ucred.cr_groups[i]);
		if ((i==(NGROUPS/2 - 1))) printf("\n\t\t");
		else if (i<NGROUPS-1) putchar(',');
	}
	printf("\n");
/* end of new credentials code */
	printf("u_ofile: (u_lastfile: %d)\n",U.u_lastfile);
	if (U.u_of_count != 0) {
		printf("u_ofile_of: 0x%8x u_pofile_of: 0x%8x\n",
		       U.u_ofile_of,U.u_pofile_of);
		ofile_of = (int *) malloc(U.u_of_count*
					  sizeof(int));
		pofile_of = (char *) 
			malloc(U.u_of_count);
		
		if (readmem((char *)ofile_of, (int)U.u_ofile_of, 
			    U.u_of_count*sizeof(int)) !=
		    U.u_of_count*sizeof(int)) {
			printf(" read error on fd overflow area\n");
			return;
		}
		if (readmem((char *)pofile_of, (int)U.u_pofile_of, 
			    U.u_of_count) !=
		    U.u_of_count) {
			printf(" read error on fd overflow area\n");
			return;
		}
	}
	for (i = 0; i <= U.u_omax; ++i) {
		if (i > NOFILE_IN_U) {
			fp = (int) ofile_of [i - NOFILE_IN_U];
			fflg = pofile_of [i - NOFILE_IN_U];
		} else {
			fp = (int)U.u_ofile[i];
			fflg = U.u_pofile[i];
		}
		if (fp == 0) continue;
		printf(" %d 0x%x ", i, fp);
			printf(" %s%s%s%s ",
		       (int) fflg & UF_EXCLOSE ? "Auto-close " : "",
		       (int) fflg & UF_MAPPED ? "Mapped " : "",
		       (int) fflg & UF_INUSE ? "In-Use " : "",
		       (int) fflg & UF_FDLOCK ? "SV Lock " : "");
		praddr(fp);
		printf("\n");
	}
	printf("sizes\t%d %d %d (clicks)\n", U.u_tsize, U.u_dsize, U.u_ssize);
/*	printf("dmap (%d bytes)\n\t%x\t%x\t%x\t%x\n",sizeof(struct dmap),
		U.u_dmap,U.u_smap,U.u_cdmap,U.u_csmap); moved to proc entry */
	printf("ssave");
	for (i=0; i<sizeof(label_t)/sizeof(int); i++) {
		if (i%5==0)
			printf("\t");
		printf("%9.1x", U.u_ssave.val[i]);
		if (i%5==4)
			printf("\n");
	}
	printf("\n");
	printf("u_odsize\t%d\n",U.u_odsize);
	printf("u_ossize\t%d\n",U.u_odsize);
	printf("u_outime\t%d\n",U.u_outime);
	printf("sigs");
	for (i=0; i<NSIG; i++) {
		if (i % 8 == 0)
			printf("\t");
		printf("%5.1x ", U.u_signal[i]);
		if (i % 8 == 7)
			printf("\n");
	}
	if (NSIG % 8 != 0) printf("\n");
	printf("sigmask");
	for (i=0; i<NSIG; i++) {
		if (i % 8 == 0)
			printf("\t");
		printf("%5.1x ", U.u_sigmask[i]);
		if (i % 8 == 7)
			printf("\n");
	}
	if (NSIG % 8 != 0) printf("\n");
	printf("sigonstack\t%9.1x\n",U.u_sigonstack);
	printf("oldmask   \t%9.1x\n",U.u_oldmask);
	printf("code      \t%9.1x\n", U.u_code);
	printf("sigstack  \t%9.1x\t%9.1x\n",
		U.u_sigstack.ss_sp,U.u_sigstack.ss_onstack);
	printf("cdir rdir\t%9.1x %9.1x\n", U.u_cdir, U.u_rdir);
	printf("cmask    \t0%o\n", U.u_cmask);
	printf("ru\t");
	ip = (int *)&U.u_ru;
	for (i = 0; i < sizeof(U.u_ru)/sizeof(int); i++) {
		if ( i % 10 == 0 && i ) printf("\n\t");
		printf("%d ", ip[i]);
	}
	if (sizeof(U.u_ru)/sizeof(int) % 10 != 0) printf("\n");
	ip = (int *)&U.u_cru;
	printf("cru\t");
	for (i = 0; i < sizeof(U.u_cru)/sizeof(int); i++) {
		if ( i % 10 == 0 && i ) printf("\n\t");
		printf("%d ", ip[i]);
	}
	if (sizeof(U.u_cru)/sizeof(int) % 10 != 0) printf("\n");
	printf("timers");
	for(i=0;i<sizeof(U.u_timer)/sizeof(struct itimerval);i++) {
		printf("\t%12d %12d %12d %12d\n",
			U.u_timer[i].it_interval.tv_sec,
			U.u_timer[i].it_interval.tv_usec,
			U.u_timer[i].it_value.tv_sec,
			U.u_timer[i].it_value.tv_usec);
	}
/*
 * Nothing now but will handle larger timer structure in the future!
 *	printf("u_XXX[3]\t%x %x %x\n",U.u_XXX[0],U.u_XXX[1],U.u_XXX[2]);
 */
	printf("start    \t%d\n", U.u_start);
	printf("acflag   \t%d\n", U.u_acflag);
	printf("limits   \t");
	for(i=0;i<RLIM_NLIMITS;i++) {
		printf("%d ",U.u_rlimit[i]);
	}
	printf("\n");
	printf("quota    \t%9.1x\n",U.u_quota);
	printf("quotaflag\t%9.1x\n",U.u_qflags);
	printf("smem     \t%9.1x %9.1x\n",
		U.u_smsize,U.u_lock);
	printf("prof     \t%9.1x %9.1x %9.1x %9.1x\n",
		U.u_prof.pr_base, U.u_prof.pr_size,
		U.u_prof.pr_off, U.u_prof.pr_scale);
	printf("u_nache  \toff %d ino %d dev %d tim %d\n",
		U.u_ncache.nc_prevoffset,U.u_ncache.nc_inumber,
		U.u_ncache.nc_dev,U.u_ncache.nc_time);
	printf("nameidata\n");
	printf("\tnameiop, error, endoff\t%8x %8d %8d\n",
		nd->ni_nameiop,nd->ni_error,nd->ni_endoff);
	printf("\t   base, count, offset\t%8x %8d %8d\n",
		nd->ni_base,nd->ni_count,nd->ni_offset);
	printf("\tdent ino %d name %.14s dirp %x\n",
		nd->ni_dent.d_ino,nd->ni_dent.d_name, nd->ni_dirp);
	printf("\tsegflg\t%8d\n", nd->ni_segflg);
	printf("u_stack  \t%9.1x\n",&U.u_stack[0]);
}
profile (p)
	int p;			/* proc slot number */
{
	do_ufile(p);
}
getcmd(i)
	int i;
{
	union {
		char	argc[CLSIZE*NBPG];
		int	argi[CLSIZE*NBPG/sizeof (int)];
	} argspac;
	register char *cp;
	register int *ip;
	char c;
	int nbad,flag;

	flag =  proctab[i].p_sched|proctab[i].p_vm|
		proctab[i].p_type|proctab[i].p_trace|
			proctab[i].p_file;

	if (flag & SSYS)
		goto retucomm;

	if (proctab[i].p_stat==SZOMB ||
	    flag&SWEXIT || !argaddr) {
		argspac.argc[0] = '\0';
		return(0);
	}
	if (lseek(mem, (int) argaddr,0) == -1) goto bad;
	if (read(mem, (char *)&argspac, sizeof (argspac))
		!= sizeof (argspac))
		goto bad;
#ifdef vax
	ip = &argspac.argi[CLSIZE*NBPG/sizeof (int)];
	ip -= 2;		/* last arg word and .long 0 */
#endif vax
#ifdef mips
	ip = &argspac.argi[(CLSIZE*NBPG-EA_SIZE)/sizeof (int)];
        while (*--ip == 0)
                if (ip == argspac.argi)
                        goto retucomm;

#endif mips
	while (*--ip)
		if (ip == argspac.argi)
			goto retucomm;
	*(char *)ip = ' ';
	ip++;
	nbad = 0;
#ifdef vax
	for (cp = (char *)ip; cp < &argspac.argc[CLSIZE*NBPG]; cp++) {
#endif vax
#ifdef mips
	for (cp = (char *)ip; cp < &argspac.argc[CLSIZE*NBPG-EA_SIZE]; cp++) {
#endif mips
		c = *cp & 0177;
		if (c == 0)
			*cp = ' ';
		else if (c < ' ' || c > 0176) {
			if (++nbad >= 5) {
				*cp++ = ' ';
				break;
			}
			*cp = '?';
		} else if (c == '=') {
			while (*--cp != ' ')
				if (cp <= (char *)ip)
					break;
			break;
		}
	}
	*cp = 0;
	while (*--cp == ' ')
		*cp = 0;
	cp = (char *)ip;
	(void) strncpy(cmdbuf, cp, &argspac.argc[CLSIZE*NBPG] - cp);
	if (cp[0] == '-' || cp[0] == '?' || cp[0] <= ' ') {
		(void) strcat(cmdbuf, " (");
		(void) strncat(cmdbuf, U.u_comm, sizeof(U.u_comm));
		(void) strcat(cmdbuf, ")");
	}
	return(1);

bad:
	printf("error locating command name for pid %d\n",proctab[i].p_pid);
	return(0);
retucomm:
	(void) strcpy(cmdbuf, " (");
	(void) strncat(cmdbuf, U.u_comm, sizeof (U.u_comm));
	(void) strcat(cmdbuf, ")");
	return(1);
}

ps_hdr()
{
	printf("SLOT   PID   UID   COMMAND\n");
}

do_ps(c)
	int c;			/* process slot number */
{
	if (c < 0) return;
	cmdbuf[0] = '\0';
	if(getuarea(c) == -1)
		return;
	if(getcmd(c))
		printf("%4d%6d%6d   %s\n", c, proctab[c].p_pid,
		    proctab[c].p_uid, cmdbuf);
}

void
prstack(procslot)
int	procslot;
{
	register  int	ret;
	int *afp;
	int *ksp;
	int *stack;
	int curproc;	
	int todo;
	unsigned where;
	
	if(procslot > (int)tab[PROC_T].ents) {
		printf("%d out of range, use Process Table Slot\n", procslot);
		return;
	}
	if(procslot == -1) {
		struct Symbol *scb;
		struct Symbol *symsrch();
		struct cpudata cpudata;
		
		scb = symsrch("_scb");
		if(readmem((char *)&afp, (int)scb->s_value - 4, sizeof afp) !=
		    sizeof afp) {
			printf("could not get locore\n");
			return;
		}
		if(afp == NULL) {
			printf("cannot dump stack of running process\n");
			return;
		}
		ksp = afp;
	
		get_cpudata(&cpudata, 0);
		curproc = getindex((char *)cpudata.cpu_proc, procbuckets,
		    PROCBUCKETS);
#ifdef DEBUG
		printf("proc addr 0x%x\n", cpudata.cpu_proc);
		printf("process slot %d\n", curproc);
#endif
		if((ret=getuarea(curproc)) == -1) {
			error("bad read of uarea");
			return;
		}
	} else {
		if((ret=getuarea(procslot)) == -1) {
			error("bad read of uarea");
			return;
		}
		else if(ret == SWAPPED) {
			error("process is swapped");
			return;
		}
#ifdef mips		
		ksp = (int *) U.u_pcb.pcb_regs[8];
#endif
#ifdef vax		
		afp = (int *) U.u_pcb.pcb_r13;
		ksp = (int *) U.u_pcb.pcb_ksp;
#endif
	}
	 /* make sure we are alligned */	
	ksp = (int *)(((unsigned) ksp + 0xf) & ~0xf);
	where = (int)ksp;
	todo = (int) (KERNELSTACK - (unsigned)ksp) / sizeof(int);	
	stack = (int *) ((char *)&U + (UPAGES * NBPG));
	/*
	 * we have the entire uarea in U. the actual displacement
	 * of fp (ap, and pc) is stack - (7fffffff - fp) (I think)
	 */
	printf ("ksp 0x%x, todo 0x%x stack 0x%x\n",ksp,todo,stack);
	ksp = (int *) (stack - ((int *)0xffffffff - ksp)) - 1;
	printf ("ksp 0x%x, todo 0x%x stack 0x%x\n",ksp,todo,stack);
	
	while (todo > 0) {
		printf("0x%x:  0x%08x 0x%08x 0x%08x 0x%08x\n", where,
		    *ksp, *(ksp + 1), *(ksp + 2), *(ksp + 3));
		ksp += 4;
		where += (4 * sizeof(int));
		todo -= 4;
	}
#ifdef vax
	printf("\n");
	printf("\n");
	printf("STACK FRAME:\n");
	printf("|- - - - - - - - - - ->|\tKFP == %x\n", afp);
	printf("________________________________________________________________________\n");
	printf("|  fp  |  pc  |  regs  |  cond  |   psl  |  ap  |  fp  |  pc  |  regs  |\n");
	printf("|______|______|________|________|________|______|______|______|________|\n");
#endif mips
}
#ifdef mips
prtrace(vars,procslot)
	int	procslot;
	int	vars;			/*  want vars */
{
	long stack_getwd();
	extern char *namelist;
	if (procslot < 0) return;
	if (getuarea(procslot) == -1) {
		printf ("process %d (slot %d): uarea not in core.\n", 
		proctab[procslot].p_pid,procslot);
		return;
	}
	U.u_pcb.pcb_regs[31] = U.u_pcb.pcb_regs[10];
	stacktrace(namelist,U.u_pcb.pcb_regs[10],U.u_pcb.pcb_regs[8],
		   U.u_pcb.pcb_regs,stack_getwd, vars);
}

long stack_getwd(p)
	unsigned int p;
{
	long data, *addr;
	if (p < 0xffffc000)
		return(0);
	addr = (int *) ((char *)&U + ((char *) p - (char *) 0xffffc000));
	data = (int) *addr;
	return(data);
}
#endif



biton(i)
	register unsigned int i;
{
	register int j = 0;

	while(i) {
		if(i & 0x1)
			j++;
		i >>= 1;
	}
	return(j);
}


