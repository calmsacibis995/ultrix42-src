#ifndef lint
static char *sccsid = "@(#)genassym.c	4.2	ULTRIX	9/6/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1985,86,87,88 by			*
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
 * genassym.c
 */

/*
 * Modification History:
 *
 * 4-Sep-90	dlh
 *	add defines for vector support
 *
 * 06-Jun-90 pmk
 *	Remove 	printf("#define\tCIADAP_SZ %d\n", sizeof(CIADAP));
 *
 * 09-Nov-89 jaw
 *	remove unused fields in vmmeter.
 *
 * 20-Jul-89 jaw
 *	rearrange lock structure.
 *
 *  10-Feb-89	jaw
 *	move TBI flag to sperate per-cpu field.
 *
 *  02-Feb-89	jaw
 *	Make longjmp mpsafe.  move newpc and newfp global to per-cpudata base.
 *
 *  26-Jan-89	jaw
 *	fix up start/stop cpu.
 *
 *  3-Jan-89	jmartin
 *	#define BITPOS et al; invoke for c_intrans and c_free fields.
 *
 * 18-Jul-88	jaa
 *	removed #define of USRPTSIZE
 *
 * 13-Jun-88 -- chet
 *	add configurable buffer cache changes
 *
 * 8-Mar-88 jaa
 *	ifdef'd USRPTSIZE
 *
 * 2-Feb-88 tresvik
 *	if building a STANDALONE KERNEL, add NMD (Memory Device) pages
 *	to the value of SYSPTSIZE
 *
 * 15-Jan-88 lp
 *	NMBCLUSTERS is gone.
 *
 * 05-Jan-88 -- Larry Cohen
 *	Increase SYSPTSIZE so standalone kernels will boot.
 *
 * 13-Feb-87 -- Chase
 *	Increase size of mbuf map.  This change will consume an additional
 *	2k of memory on every system, but it should eliminate system panics
 *	caused by running out of mbuf map.
 *
 * 16-Jul-86 -- Todd M. Katz
 *	Add definition for CIADAP_SZ, the size of a CI adapter structure.
 *
 * 14-Apr-86 -- jaw
 *	remove MAXNUBA referances.....use NUBA only!
 *
 * 02-Apr-86 -- jrs
 *	Removed NQ stuff added in previous (defunct) scheduler
 *
 * 11-Mar-86 -- Larry
 *	NMBCLUSTERS is now a function of MAXUSERS
 *
 * 05-Mar-86 -- bglover
 *	Removed msgbuf from kernel; replaced with error log buffer
 *
 * 03-Mar-86 -- jrs
 *	Add in rpb offsets needed to fill in slave start code
 *
 * 05-Feb-86 -- jrs
 *	Add c_paddr cpudata structure offset
 *
 * 04-feb-86 -- jaw  get rid of biic.h.
 *
 * 22-Jul-85 -- jrs
 *	Add cpudata structure offsets for multiprocessor work
 *
 * 31 Jun 85	darrell
 *	Added a lines to define UH_ZVCNT and UH_ZVFLG to make those
 *	elements of the uba_hd structure available to macro code (locore.s).
 *
 * 19-Jun-85 -- jaw
 *	VAX8200 name change.
 *
 * 13-Mar-85 -jaw
 *	Changes for support of the VAX8200 were merged in.
 *
 * 27-Feb-85 -tresvik
 *	Changes for support of the VAX8600 were merged in.
 *
 */

#define VAX8800 1
#define VAX8200 1
#define	VAX780	1
#define	VAX750	1
#define	VAX730	1

#include "../machine/pte.h"
#include "../machine/param.h"

#include "../h/param.h"
#include "../h/buf.h"
#include "../h/vmmeter.h"
#include "../h/vmparam.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/cmap.h"
#include "../h/map.h"
#include "../h/kmalloc.h"
#include "../io/uba/ubareg.h"
#include "../io/uba/ubavar.h"
#include "../h/proc.h"
#include "../h/text.h"
#include "../machine/rpb.h"
#include "../h/mbuf.h"
#include "../machine/ioa.h"
#include "../machine/nexus.h"
#include "../io/bi/buareg.h"
#include "../h/cpudata.h"
#include "../io/ci/ciadapter.h"
#include "../machine/vectors.h"

char *calloc();
static char *BITPOS_ptr;
#define BITPOS(TYPE, FIELD) (BITPOS_ptr = calloc(1, sizeof(TYPE)),	\
			  ((TYPE *)BITPOS_ptr)->FIELD=1,		\
			  free(BITPOS_ptr),				\
			  str_ffs(BITPOS_ptr, sizeof(TYPE)*NBBY))
int
str_ffs(base, size)
	register int *base;
	register int size;
{
	register int findpos;
	register int bits;

	for (findpos=0; *base==0 && findpos<size; findpos+=NBBY*sizeof(int))
		++base;
	for (bits = *base; (bits&1)==0 && findpos<size; ++findpos)
		bits>>=1;
	return findpos;
}

struct uba_hd uba_hd[1]; /* keep make happy */
int tty_ubinfo[1];  /* ditto */

main(argc, argv)
char *argv[];
{
	register struct proc *p = (struct proc *)0;
	register struct uba_regs *uba = (struct uba_regs *)0;
	register struct uba_hd *uh = (struct uba_hd *)0;
	register struct vmmeter *vm = (struct vmmeter *)0;
	register struct user *up = (struct user *)0;
	register struct rusage *rup = (struct rusage *)0;
	register struct kmemusage *kup = (struct kmemusage *)0;
	register struct kmembuckets *kbp = (struct kmembuckets *)0;
	register struct kmemelement *kep = (struct kmemelement *)0;
	struct rpb *rp = (struct rpb *)0;
	struct text *tp = (struct text *)0;
	struct cpudata *cpd = (struct cpudata *)0;
	struct lock_t *lptr = (struct lock_t *)0;
	struct vpdata *vpd = (struct vpdata *)0;

	struct pcb *pcbp = (struct pcb *) 0;
	int maxusers = 10;
	int physmem_est = MINMEM_MB; /* est. of physical memory in megabytes */
	int factor;	/* inflate ptes by this factor because of phys. mem */
	int bufcache;	/* percentage of mem for buffer cache */
	int nbufs;	/* number of buffer headers */
	int bufptes;	/* # of PTEs for buffer cache */

	printf("#ifdef LOCORE\n");
	printf("#define\tU_ESP %d\n", &pcbp->pcb_esp);
	printf("#define\tP_LINK %d\n", &p->p_link);
	printf("#define\tP_RLINK %d\n", &p->p_rlink);
	printf("#define\tP_XLINK %d\n", &p->p_xlink);
	printf("#define\tP_ADDR %d\n", &p->p_addr);
	printf("#define\tP_PRI %d\n", &p->p_pri);
	printf("#define\tP_STAT %d\n", &p->p_stat);
	printf("#define\tP_VM %d\n", &p->p_vm);
	printf("#define\tP_WCHAN %d\n", &p->p_wchan);
	printf("#define\tP_TSIZE %d\n", &p->p_tsize);
	printf("#define\tP_SSIZE %d\n", &p->p_ssize);
	printf("#define\tP_P0BR %d\n", &p->p_p0br);
	printf("#define\tP_SZPT %d\n", &p->p_szpt);
	printf("#define\tP_TEXTP %d\n", &p->p_textp);
	printf("#define\tP_AFFINITY %d\n", &p->p_affinity);
	printf("#define\tP_PCB %d\n", &p->p_pcb);
	printf("#define\tP_VPCONTEXT %d\n", &p->p_vpcontext);
	printf("#define\tSSLEEP %d\n", SSLEEP);
	printf("#define\tSRUN %d\n", SRUN);
	printf("#define\tUBA_BRRVR %d\n", uba->uba_brrvr);
	printf("#define\tUH_UBA %d\n", &uh->uh_uba);
	printf("#define\tUH_VEC %d\n", &uh->uh_vec);
	printf("#define\tUH_SIZE %d\n", sizeof (struct uba_hd));
	printf("#define\tUH_ZVCNT %d\n", &uh->uh_zvcnt);
	printf("#define\tUH_ZVFLG %d\n", &uh->uh_zvflg);
	printf("#define\tRP_FLAG %d\n", &rp->rp_flag);
	printf("#define\tRP_BUGCHK %d\n", &rp->bugchk);
	printf("#define\tRP_WAIT %d\n", rp->wait);
	printf("#define\tX_CADDR %d\n", &tp->x_caddr);
	printf("#define\tV_PDMA %d\n", &vm->v_pdma);
	printf("#define\tV_FAULTS %d\n", &vm->v_faults);
	printf("#define\tV_PGREC %d\n", &vm->v_pgrec);
	printf("#define\tV_FASTPGREC %d\n", &vm->v_fastpgrec);
	printf("#define\tL_PC	%d\n", &lptr->l_pc);
	printf("#define\tL_WANTED %d\n", &lptr->l_wanted);
	printf("#define\tL_WON	%d\n", &lptr->l_won);
	printf("#define\tL_PLOCK %d\n", &lptr->l_plock);
	printf("#define\tCPU_PADDR %d\n", &cpd->cpu_paddr);
	printf("#define\tCPU_ROUNDROBIN %d\n", &cpd->cpu_roundrobin);
	printf("#define\tCPU_STATE %d\n", &cpd->cpu_state);
	printf("#define\tCPU_TBI_FLAG %d\n", &cpd->cpu_tbi_flag);
	printf("#define\tCPU_RUNRUN %d\n", &cpd->cpu_runrun);
	printf("#define\tCPU_NEWFP %d\n", &cpd->cpu_newfp);
	printf("#define\tCPU_NEWPC %d\n", &cpd->cpu_newpc);
	printf("#define\tCPU_NOPROC %d\n", &cpd->cpu_noproc);
	printf("#define\tCPU_PROC %d\n", &cpd->cpu_proc);
	printf("#define\tCPU_INT_REQ %d\n", &cpd->cpu_int_req);
	printf("#define\tCPU_SWITCH %d\n", &cpd->cpu_switch);
	printf("#define\tCPU_NUM %d\n", &cpd->cpu_num);
	printf("#define\tCPU_MASK %d\n", &cpd->cpu_mask);
	printf("#define\tCPU_INTR %d\n", &cpd->cpu_intr);
	printf("#define\tCPU_STOPS %d\n", &cpd->cpu_stops);
	printf("#define\tCPU_STACK %d\n", &cpd->cpu_stack);
	printf("#define\tCPU_ISTACK %d\n", &cpd->cpu_istack);
	printf("#define\tCPU_TRAP %d\n", &cpd->cpu_trap);
	printf("#define\tCPU_HLOCK %d\n", &cpd->cpu_hlock);
	printf("#define\tCPU_VPDATA %d\n", &cpd->cpu_vpdata);
	printf("#define\tUPAGES %d\n", UPAGES);
	printf("#define\tHIGHPAGES %d\n", HIGHPAGES);
	printf("#define\tNISP %d\n", NISP);
	printf("#define\tCLSIZE %d\n", CLSIZE);
	printf("#define\tCMAPSZ %d\n", sizeof(struct cmap));
	printf("#define\tCMAP_INTRANS %d\n", BITPOS(struct cmap, c_intrans));
	printf("#define\tCMAP_FREE %d\n", BITPOS(struct cmap, c_free));
	printf("#define\tKB_SIZE %d\n", sizeof *kbp);
	printf("#define\tKU_SIZE %d\n", sizeof *kup);
	printf("#define\tKB_EFL %d\n", &kbp->kb_efl);
	printf("#define\tKU_REFCNT %d\n", &kup->ku_refcnt);
	printf("#define\tKU_HELE %d\n", &kup->ku_hele);
	printf("#define\tKU_TELE %d\n", &kup->ku_tele);
	printf("#define\tKU_INDEX %d\n", &kup->ku_index);
	printf("#define\tKE_FL %d\n", &kep->ke_fl);
	printf("#define\tVPD_PROC %d\n", &vpd->vpd_proc);

	maxusers = atoi(argv[1]);
	physmem_est = atoi(argv[2]);
	if (argc > 3)
		bufcache = atoi(argv[3]); /* % of memory for buffer cache */
	else
		bufcache = 10; /* default */

	if (physmem_est < MINMEM_MB)
		physmem_est = MINMEM_MB;
	else if (physmem_est > MAXMEM_MB)
		physmem_est = MAXMEM_MB;

	/*
	 * The number of system page table entries increases with maxusers,
	 * physical memory, and size of the buffer cache.
	 *
	 */

	/* Black magic fudge factor */
	factor = (physmem_est) + (maxusers / 5);

	/* Calculate amount of memory (in 1K units) used for buffer data. */
	/* Assume 1 buffer header for every 2K of buffer data. */
	nbufs = ((physmem_est * 1024) * ((float)bufcache / 100)) / 2;

	/* Calculate # of ptes needed for buffer data. */
	/* Add the number of ptes needed for the buffer headers. */
	/* Add a little more for the pte's themselves. */
	bufptes = (nbufs * MAXBSIZE) /NBPG;
	bufptes += ((nbufs * sizeof(struct buf)) / NBPG) + 1;
	bufptes += ((bufptes * sizeof(struct pte)) / NBPG) + 1;

	printf("/*maxusers=%d, bufcache=%d, SYSPTSIZE=((30+maxusers+%d)*NPTEPG)+%d*/ \n",
	       maxusers, bufcache, factor, bufptes);
	printf("#ifdef SAS\n");
	printf("#include \"md.h\"\n");
	printf("#define\tSYSPTSIZE (%d+NMD)\n",
		((30+maxusers+factor)*NPTEPG) + bufptes);
	printf("#else SAS\n");
	printf("#define\tSYSPTSIZE %d\n",
		((30+maxusers+factor)*NPTEPG) + bufptes);
	printf("#endif SAS\n");
	printf("#define\tU_PROCP %d\n", &up->u_procp);
	printf("#define\tU_RU %d\n", &up->u_ru);
	printf("#define\tRU_MINFLT %d\n", &rup->ru_minflt);
	printf("#define\tMAXNIOA %d\n", MAXNIOA);
	printf("#else\n");
	printf("asm(\".set\tU_ARG,%d\");\n", up->u_arg);
	printf("asm(\".set\tU_QSAVE,%d\");\n", up->u_qsave);
	printf("#endif\n");

}
