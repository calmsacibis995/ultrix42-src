#ifndef lint
static char *sccsid = "@(#)startup.c	4.11      (ULTRIX)  2/28/91";
#endif	lint

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

/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */

/*
 * Modification History: startup.c
 *
 * 23-Feb-91	jsd
 *	new allocation scheme for gateway screen buffers
 *
 * 15-Nov-90	Randall Brown
 *	Fixed a bug in msize_bitmap where the for loop was not being 
 *	broken out of when the first bad page was hit.  Also changed
 *	the bitmaplen for REX roms.  The bitmaplen is now in bytes for
 *	those roms.
 *
 * 13-Sep-90	Joe Szczypek
 *	Added support for new 3max/3min TURBOchannel console callbacks.  If
 *	new console ROM present, use rex_getbitmap to get bitmap and bitmaplen,
 *	and use rex_getsystype to get systype.  Added a fourth argument as 
 *	input to startup.  This passes in the base address of the new callback
 *	vector dispatch table.  Save all four arguments in globals.
 * 
 * 17-Aug-90	Randall Brown
 *	Removed cpu check for DS3100 after cninit(), cninit() now works
 *	correctly on DS3100. 
 *
 * 09-Aug-90	Randall Brown
 *	Added support for 3min - DS_5000_100.
 *
 * 21-Jun-90	Paul Grist
 *	fixed bug in msize_bitmap. It was using btoc() to do
 *	calculation, this round up to the next page boundry, so when
 *	the bitmap doesn't fall on a 4k boundry it was putting back
 * 	unavialable memory, the result was the bitmap was marked usable
 *	and got stepped on. So on reboots the system thought it had no memory
 *
 * 01-May-90 -- Paul Grist
 *      Added support for mipsmate - DS_5100.
 *
 * 15-Feb-90 -- Robin
 *	Made a bug fix to allow the DS_5500 to size memory via the
 *	bitmap (same as the 5800).
 *
 * 19-Dec-89 -- Fred Canter
 *	Bug fix to last delta. Need 2 PTEs per buffer header not one.
 *
 * 18-Dec-89 -- burns
 *	Fixed check of nbuf against sysptsize to exclude the MAXMEM
 *	fudge factor which is incorrect for mips based systems.
 *
 * 12-Dec-89 -- burns
 *	Moved out sysptsize to param.c since it is now calculated
 *	based upon physmem from the config file. It needs to be
 *	set in the targetbuild directory, not in a binary file.
 *
 * 11-Nov-89 -- Randall Brown
 *	Moved when we call cninit() to after mapinit(), so that cninit() can
 *	do BADDADDR if it needs to.  Changed msize_bitmap to ignore the first
 *	2 long words if we are on a 3max.
 *
 * 10-Nov-89 -- jaw
 *	remove references to maxcpu.
 *
 * 16-Oct-89 -- Alan Frechette
 *	No longer dump out the buffer cache during a system crash
 *	for partial selective crash dumps. Save the PFN of the last 
 *	kernel page to dump out for the crashdump code. Readjusted
 *	the order of allocation so that the buffer cache is allocated
 *	last. The buffer cache must be the last allocated data for 
 *	this to work. 
 *
 * 13-Oct-89  -- gmm
 *	smp support
 *
 * 10-July-89 -- burns
 *	Added memory sizing via bitmap, and added afd's separate cpu
 *	switch troutines for sizing memory via badaddr probes or bitmap.
 *
 * 30-Jun-89 -- afd
 *	Call memsize routine (to size and clear memory) thru the cpu switch.
 *	Added 2 new memory sizing rotines: msize_baddr and msize_bitmap.
 *
 * 15-Jun-89 -- afd
 *	Call cpu_initialize() routine as soon as "cpup" is set up in startup().
 *  
 * 14-Jun-89 -- darrell
 *	Removed the splhigh in startup, as spl() cannot be called yet,
 *
 * 14 Jun 89 -- chet
 *	Make buffer header allocations based on maxcpu (uniprocessor
 *	or multiprocessor).
 *
 * 01-Jun-89 -- Kong
 *	Added a field in the "save_state" struct to store the value
 *	of "cpu".  This is to allow the ULTRIX installation program
 *	"finder" to determine the type of machine it is running on
 *	without doing a name list on the kernel.  "save_state" is
 *	initialised by "init_restartblk".
 *
 * 08-Jun-89 -- gmm
 *	Allocate (KMALLOC) the idle stack for boot cpu during startup
 *
 * 02-May-89 -- jaw, jmartin
 *	fix forkutl to work on mips.
 *
 * 20-May-89 -- Randall Brown
 *	added call to cninit(), so that the cons_swp is setup correctly.
 *	printf's can not be done before this call is executed.
 *
 * 17-Apr-89 -- afd
 *	Created "save_state" struct at the bottom of the ULTRIX startup stack.
 *	This struct contains a magic number and the address of "doadump".
 *	This allows people to force a memory dump from console mode when
 *	a system is hung.
 *
 * 07-Apr-89 -- afd
 *	Updated system_type routine.
 *	Don't need "system.h"; cpu & systype info is in cpuconf.h
 *	Added "cpuswitch_entry" routine to initialize the pointer into
 *	    the cpusw table (cpup).
 *
 * 07-Apr-89 -- kong
 *	Added global variable "qmapbase" which gets initialised to
 *	the physical address of the Qbus map registers.  Changed
 *	routine "mapinit" so that on a kn210, the last 32K bytes
 *	of physical memory is not used by the kernel, but is used
 *	by the Qbus as map registers.  Note that the Qbus map must
 *	sit on a 32Kb boundary.
 *
 * 24-Mar-89 -- burns
 *	Added ISIS specifics.
 *
 * 23-Mar-89 -- Kong
 *	Added variable "sysptsize" and initialize it to SYSPTSIZE.
 *	Some of the VAX i/o device drivers need it.
 *
 * 23-Feb-89 -- Kong
 *	Added variable "Physmem" so that the "sizer" program knows
 * 	the actual size of physical memory.
 *
 * 12-Jan-89 -- Kong
 *     .Added variable "cpu" whose value is defined in system.h.
 *	Determine on what we are running and properly initialize "cpu".
 *     .Based on the system type, initialize all the interrupt handler
 *	entry points in c0vec_tbl (in trap.c).  This way each different
 *	system can use different handlers.
 *     .Initialized the interrupt masks based on the type of system
 *	we're on.  This allows the different interrupt schemes of the
 *	different systems to use the same splxxx routines.
 *
 * 10 Nov 88 -- chet
 *	Add configurable buffer cache.
 *
 * 09-Nov-88 -- afd
 *	Get the "systype" word from the boot PROM, and save it in
 *	"cpu_systype".
 *	This is done in startup because it must be done before the first
 *	autoconfig printf so that the error log packet with config messages
 *	will have the systype in the header.
 *
 * 09-Sep-88 -- afd
 *	Cleanup startup msgs; no configuration printfs until after kmeminit(),
 *	  because the kernel printf routine does a km_alloc.
 *	Set & clear appendflg so that startup msgs get logged together.
 *	Set "printstate" to what level of printing is possible as the system
 *	  comes up.
 *	Call chk_cpe() after configuration is done (records cache parity errs).
 *
 */

#include "../machine/reg.h"
#include "../machine/pte.h"
#include "../machine/cpu.h"
#include "../machine/hwconf.h"
#include "../../machine/common/cpuconf.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/gnode.h"
#include "../h/dir.h"
#include "../h/kernel.h"
#include "../h/map.h"
#include "../h/vm.h"
#include "../h/proc.h"
#include "../h/buf.h"
#include "../h/reboot.h"
#include "../h/conf.h"
#include "../h/file.h"
#include "../h/text.h"
#include "../h/clist.h"
#include "../h/callout.h"
#include "../h/cmap.h"
#include "../h/mbuf.h"
#include "../h/quota.h"
#include "../h/cpudata.h"
#include "../fs/nfs/dnlc.h"
#include "../h/flock.h"
#include "../h/dump.h"
#include "../machine/param.h"
#include "../h/kmalloc.h"
#if PROFILING
#include "../h/prof.h"
#endif PROFILING

#include "../machine/entrypt.h"
#include "./assym.h"

extern Sysmaplimit;
unsigned Syssize;
extern	int sysptsize;			/* for device drivers to know */
extern struct kmemusage *kmemusage;
extern kmeminit();
extern  int     appendflg;		/* for logging startup messages */
u_int printstate = PROMPRINT;		/* what level of printf is available */
int	Physmem = 0;			/* Size of physical memory   */

#ifdef CACHETRICKS
unsigned short icachecnt[MAXCACHE/NBPG];
unsigned short dcachecnt[MAXCACHE/NBPG];
#endif CACHETRICKS
unsigned icachemask, dcachemask;
unsigned cpu_systype;			/* systype word in boot PROM */
int	cpu;				/* value defined in cpuconf.h */
struct	cpusw *cpup;			/* pointer to cpusw entry    */
int	splm[SPLMSIZE];			/* interrupt masks 	     */
struct cpusw *cpuswitch_entry();

int	qmapbase;			/* Physical address of Qbus map regs*/
extern  int rex_base;                   /* Base of ROM Executive callback 
					   table used by 3max and 3min */
extern  int rex_magicid;                /* Console type id */ 
char    bootctlr[2];
char	consmagic[4];
int	ub_argc;
char	**ub_argv;
int     console_magic;                  /* Console id */
struct  mem_bitmap rex_map[1];          /* allocate a mem_bitmap structure */

/*
 * Machine-dependent startup code.  The boot stack is still in use.
 * The kernel sp in proc 0 U area is setup on return.
 */
startup(argc, argv, envp, vector)
	int argc;
	char **argv;
	char **envp;
        char **vector;
{
	unsigned fpage;	/* first free physical page */
	int i; /* DEBUG */
	extern char end[], edata[];
	extern char utlbmiss[], eutlbmiss[];
	extern char exception[], eexception[];
	extern struct pte eSysmap[];
	extern icache_size, dcache_size;

	/*
	 * copy down exception vector code
	 */
	bcopy(utlbmiss, UT_VEC, eutlbmiss - utlbmiss);
	bcopy(exception, E_VEC, eexception - exception);
	bzero(edata, end - edata);	/* zero bss */
	/* initialize this cpu as boot processor */
	/*
	 * Get the "systype" word from the boot PROM, which contains
	 * the processor type, the system implementation type, firmware
	 * rev and hardware rev.
	 *
	 * Also set "cpu" (the ULTRIX system type) based on both
	 * processor type and system implementation type.
	 *
	 * Initialize "cpup" to point to the cpusw table entry for the system
	 * that we are currently running on.
	 *
	 * As soon as we have "cpup" set, call the cpu initialization routine
	 * to fill in the splm, interupt vector table, ipl mask, etc.
	 * This MUST be done before any spl routines can be called (thus before
	 * any printfs can be done).
	 */
	console_magic = (int)envp;  /* save console id */
	ub_argc = argc;
	ub_argv = argv;
	if(console_magic == REX_MAGIC) {
		rex_base = (int)vector;
		rex_magicid = (int)envp;
		cpu_systype = rex_getsystype();
	}
	else {
		rex_base = 0;
		rex_magicid=0;
		cpu_systype = xtob(prom_getenv("systype"));
	}

	cpu = system_type(cpu_systype);
	cpup = cpuswitch_entry(cpu);
	/*
	 * Cannot call spl() before cpu_initialize is called -- therefore
	 * printf's cannot be done before now.
	 */
	cpu_initialize();

	/* init_boot_cpu() will call splextreme, so make sure spls are setup */
	/* initialize this cpu as boot processor. 'cpu' should be setup
	   before calling init_boot_cpu */
	init_boot_cpu();

	config_cache();
	icachemask = (icache_size - 1) >> PGSHIFT;
	dcachemask = (dcache_size - 1) >> PGSHIFT;
	flush_cache();
	/*
	 * Acknowledge all interrupts
	 */
	stopclocks();
	Syssize = eSysmap - Sysmap;
	fpage = btoc(K0_TO_PHYS((unsigned)end)); /* first page phys mem */
	getargs(argc, argv, envp);
	fpage = xprinit(fpage);	/* initialize XPRBUG buffer */
	mapinit(fpage);		/* initialize system maps */
	/* 
	 * In mapinit we initialize the kernel stack, so now we can
	 * handle exceptions.  For some systems, cninit will cause
	 * an exception while doing badaddr probes.
	 * Once cninit is done we can do kernel printfs thru the Ultrix
	 * driver (rather than thru console call backs).
	 */
	cninit();
	printstate |= CONSPRINT; /* We can print on the console */
	/*
	 * We can do mprintfs after kmeminit is done in mapinit.
	 */
	printstate = printstate | MEMPRINT;
	appendflg = 1;
	printf(version);
	printf("real mem = %d\n", ctob(physmem));
	printf("avail mem = %d\n", ctob(maxmem));
	printf("using %d buffers containing %d bytes of memory\n",
		nbuf, bufpages * CLBYTES);
	if (cpup->flags & SCS_START_SYSAPS)
	    scs_start_sysaps();
	configure();		/* auto config.  */
	getbootctlr();          /* Get logical controller */
	appendflg = 0;		/* done with startup config messages */
	printf("");
	(*(cpup->timer_action));
	/*
	 * on return from configure machine is at spl0()
	 */
	init_restartblk();
}

/*
 * Convert an ASCII string of hex characters to a binary number.
 */
xtob(str)
	char *str;
{
	register int hexnum;

	if (str == NULL || *str == NULL)
		return(0);
	hexnum = 0;
	if (str[0] == '0' && str[1] == 'x')
		str = str + 2;
	for ( ; *str; str++) {
		if (*str >= '0' && *str <= '9')
			hexnum = hexnum * 16 + (*str - 48);
		else if (*str >= 'a' && *str <= 'f')
			hexnum = hexnum * 16 + (*str - 87);
		else if (*str >= 'A' && *str <= 'F')
			hexnum = hexnum * 16 + (*str - 55);
	}
	return(hexnum);
}

/*
 * Mapinit:
 *	Initialize kernel memory allocation structures.  The kernel now
 * has itself mapped virtual to physical from address 0x100000 to the
 * end of bss.
 */

/*
 * Declare these as initialized data so we can patch them.
 */
#ifdef MEMLIMIT
int memlimit = MEMLIMIT;    /* to artificially reduce memory */
#else  MEMLIMIT
int memlimit = 0;
#endif MEMLIMIT

int	nbuf = 0;	/* number of file system buffers */
int	nswbuf = 0;	/* number of swap buffers */
int	bufpages = 0;	/* file system buffer pages */
int	bufdebug = 0;	/* buffer cache initialization messages */
int	clperbuf = 0;	/* one buf struct for every clperbuf page clusters */
			/* of cache */
int	pteperbuf = 2;	/* we need two 4K ptes for each 8K buf */

mapinit(fpage)
{
	register int i,j;
	int maxbufs, base, residual;
	int maxbufpages, requestpages, tenpercent;
	unsigned mapvpn;
	caddr_t v, paddr;
	struct tlbinfo *wtlb;
	struct pte *umap;
	struct cpudata *pcpu;
	extern VEC_nofault(), VEC_trap();
	extern  (*causevec[])();
#if PROFILING
	extern char eprol[], etext[];
	extern char *s_lowpc;
	extern u_long	s_textsize;
	extern struct phdr phdr;
#endif PROFILING
	extern int bufcache;	/* % of memory used for buffer cache data */
	extern int cache_bufcache;	/* cache the buffer cache? */
	extern int maxusers;
	extern long usrptsize;
	int bcpages;

	/*
	 * Invalidate entire tlb (including wired entries)
	 */
	for (i = 0; i < NTLBENTRIES; i++)
		if (i != TLBWIREDBASE)  /* TLBWIREDBASE already used for 
					   mapping u area of idle proc for the
					   boot cpu. So don't invalidate it */
			invaltlb(i);

	/*
	 * Size memory, by calling a routine thru the cpu switch.
	 */
	if ((i = (*(cpup->memsize))(fpage)) <= 0)
		panic("No memory sizing routine configured\n");

	if ((cpu == DS_5400) || ( cpu == DS_5500)) {
		/*
		 * On a Q bus, we need to grab 32K bytes of memory
		 * for the Qbus map registers.  Here we simply use
		 * the last 32Kb of physical memory.  Note that the Qbus
		 * map must be on a 32Kb boundary.
		 * This causes us to lose at least 32Kb of memory.
		 */
		i -= (i % 8);	/* make sure we'on 32Kb boundary */
		i -= 8;		/* lose the last 32Kb 		 */
		qmapbase = i * 4096;
	}

	if (memlimit && (i > btop(memlimit)))
		i = btop(memlimit);
	maxmem = physmem = freemem = i;

	/*
	 * If we used adb (or something) on the kernel and set "Physmem"
	 *    to some value, then use that value for "physmem".
	 * This allows us to run in less physical memory than the
	 *   machine really has. (rr's idea).
	 */
	if (Physmem >= MINMEM_PGS) {
		physmem = Physmem;
	}
	/*
	 * Save the real amount of memory (or the above artificial
	 * amount) to allow the sizer to accurately configure physmem
	 * in the config file.  `Physmem' was originally added for the
	 * sizer utility - tomt
	 */
	Physmem = physmem;	/* Let sizer know the size */

	/*
	 * Initialize error message buffer (at end of core).
	 */
#ifdef notdef
	maxmem -= btoc(sizeof(struct msgbuf));
#if NOMEMCACHE==1
	pmsgbuf = (struct msgbuf *)PHYS_TO_K1((unsigned)ptob(maxmem));
#else
	pmsgbuf = (struct msgbuf *)PHYS_TO_K0((unsigned)ptob(maxmem));
#endif
#ifndef SABLE
	if (pmsgbuf->msg_magic != MSG_MAGIC)
		for (i = 0; i < MSG_BSIZE; i++)
			pmsgbuf->msg_bufc[i] = 0;
#endif
#endif notdef

	/*
	 * Initialization message print.
	 */

	/*
	 * Determine how many buffers to allocate.
	 *
	 * Use bufcache% (a config'able option) of memory.
	 * If bufpgs > (physmem - min mem reserved for system)
	 * then set bufpgs to (physmem - min mem reserved for system).
	 * Ensure that bufpages is at least 10% of memory.
	 *
	 * We allocate 1/2 as many swap buffer headers (nswbuf)
	 * as file i/o buffers (nbuf), but never more than 256.
	 *
	 * NB:  The maximum number of buffers is determined by sysptsize,
	 * which determines how much of k0 is allocated to pte's for the
	 * buffer cache.
	 */

	if (bufpages == 0) {

		/*
		 * requestpages, maxbufpages, tenpercent are measured 
		 * in NBPG byte units.
		 *
		 * requestpages is what was requested in the config file.
		 * maxbufpages is the upper limit of what is allowed.
		 * tenpercent is the lower limit of what is allowed.
		 *
		 * As silly as this sounds, the upper limit can be lower
		 * than the lower limit (even negative on minimum
		 * configuration machines), so make sure that the upper
		 * limit is >= tenpercent
		 *
		 * Since we only have MINMEM_PGS to guide us, and
		 * since this number is only meaningful for kernels
		 * with relatively small system table sizes, scale
		 * up the estimate of the demands that these tables
		 * make on memory as a function of maxusers when
		 * computing the upper limit.
		 * Allow for the buffer headers as well.
		 *
		 */
		requestpages = (physmem * bufcache) / 100;
		maxbufpages = physmem
			- MINMEM_PGS
			- (maxusers/32) * ((1024*1024)/NBPG) /* 1MB/32 users */
			- (sysptsize * sizeof(struct pte)) / NBPG
			- 1;
		/*
		 * clperbuf is set to 1 (a historical value) for
		 * uniprocessors, and 2 (no moving of physical
		 * memory under a virtual address) for multi-processors.
		 * Sanity test clperbuf on multiprocessors to
		 * guarantee that buffer memory is not moved around.
		 */
		if (!clperbuf) /* hasn't been manually patched */
			clperbuf = ((smp) ? 2 : 1);
		if (smp && clperbuf != 2)
			panic("buffer header allocation failure");

		maxbufs = maxbufpages / CLSIZE / clperbuf;
		maxbufpages -= (maxbufs * sizeof(struct buf)) / NBPG;
		tenpercent = physmem / 10;
		if (maxbufpages < tenpercent)
			maxbufpages = tenpercent;

if (bufdebug) {
printf("startup: physmem %d bufcache %d requestpages %d\n",
       physmem, bufcache, requestpages);
printf("startup: clperbuf %d maxbufs %d maxbufpages %d tenpercent %d\n",
       clperbuf, maxbufs, maxbufpages, tenpercent);
}

		if (requestpages > maxbufpages) {

if (bufdebug) {
printf("startup: bufcache request of %d bytes reduced to %d bytes\n",
       requestpages*NBPG, maxbufpages*NBPG);
}

			requestpages = maxbufpages;
		}

		/* bufpages is measured in page cluster units */
		bufpages = requestpages / CLSIZE;
	}			

	if (nbuf == 0) {
		/* nbuf is # of page cluster objects that fit in bufpages */
		nbuf = (bufpages * CLSIZE) / clperbuf;
	}

	/*
	 * Finally, check to make sure that the system page table is
	 * big enough to map nbuf buffers.
	 * Note: This assumes that on mips based systems that nothing
	 * but the buffer cache uses the system page table. 
	 */
	if (nbuf > (sysptsize / CLSIZE / pteperbuf)) {
if (bufdebug) {
printf("startup: nbufs reduced due to system page table size\n");
}
		nbuf = (sysptsize / CLSIZE / pteperbuf);
	}

	if (nswbuf == 0) {
		nswbuf = (nbuf / 2) &~ 1;	/* force even */
		if (nswbuf > 256)
			nswbuf = 256;		/* sanity */
	}

if (bufdebug) {
printf("startup: bufpages %d nbuf %d nswbuf %d sysptsize %d\n",
	bufpages, nbuf, nswbuf, sysptsize);
}

	/*
	 * Allocate space for unmapped system data structures.
	 * Next available unmapped kernel address is maintained in paddr;
	 */
#if NOMEMCACHE==1
	paddr = (caddr_t)PHYS_TO_K1((u_int)ptob(fpage));
#else
	paddr = (caddr_t)PHYS_TO_K0((u_int)ptob(fpage));
#endif

#define	palloc(name, type, num) \
	    (name) = (type *)(paddr); \
	    (paddr) = (caddr_t)((name)+(num))

#define	palloclim(name, type, num, lim) \
	    (name) = (type *)(paddr); \
	    (paddr) = (caddr_t)((lim) = ((name)+(num)))

	palloc(buf, struct buf, nbuf);
	palloc(swbuf, struct buf, nswbuf);
	palloclim(gnode, struct gnode, ngnode, gnodeNGNODE);
	palloclim(file, struct file, nfile, fileNFILE);
	palloclim(proc, struct proc, nproc, procNPROC);
	palloc(umap, struct pte, nproc*UPAGES);
	palloc(wtlb, struct tlbinfo, nproc*NPAGEMAP);
	palloclim(text, struct text, ntext, textNTEXT);
	palloc(cfree, struct cblock, nclist);
	palloc(callout, struct callout, ncallout);
	palloc(swapmap, struct map, nswapmap = nproc * 2);
	/*
	 * kernelmap must be large enough to allocate 3 page tables
	 * per process (text, data, stack)
	 */
	palloc(kernelmap, struct map, nproc*3);
        palloc (flox, struct filock, flckinfo.recs );
        palloc (flinotab, struct flino, flckinfo.fils );
        palloc (kmemmap, struct map, (ekmempt - kmempt) - km_wmapsize);
        palloc (kmemwmap, struct map, km_wmapsize);
        palloc (kmemusage, struct kmemusage, (ekmempt - kmempt));
	palloc(nch, struct nch, nchsize);
#if PROFILING
	/* get some memory for the profiling routines to use */
	s_textsize = 0;
	s_lowpc = phdr.lowpc = (char *)
	    ROUNDDOWN((unsigned)eprol, HISTFRACTION*sizeof(HISTCOUNTER));
	phdr.highpc = (char *)
	    ROUNDUP((unsigned)etext, HISTFRACTION*sizeof(HISTCOUNTER));
	s_textsize = phdr.highpc - phdr.lowpc;
	phdr.pc_bytes = (s_textsize / HISTFRACTION);
	cprintf("Profiling kernel, textsize=%d(%d) [%x..%x]\n",
		phdr.pc_bytes, s_textsize, phdr.lowpc, phdr.highpc);
	palloc(phdr.pc_buf, char, phdr.pc_bytes);
	cprintf("got pc_buf at %x\n", phdr.pc_buf);
#if PROFTYPE == 2 || PROFTYPE == 3
	phdr.bb_bytes = s_textsize / BB_SCALE;
	palloc(phdr.bb_buf, char, phdr.bb_bytes);
#endif PROFTYPE == 2 | 3
#endif PROFILING
#ifdef QUOTA
	palloclim(quota, struct quota, nquota, quotaNQUOTA);
	palloclim(dquot, struct dquot, ndquot, dquotNDQUOT);
#endif
	{
	    /* allocate space for and initialize gateway screen freelist */
	    char *screen_storage;
	    palloc(screen_storage, char, screen_space_needed());
	    screen_init_freelist(screen_storage);
	}
	fpage = btoc(K0_TO_PHYS((u_int)paddr));

	/* 
	 * Initialize the proc table to point to the table which contains
	 * per process tlb mapping information. These mappings are dropped
	 * into the tlb wired entries and include mappings for the u-area.
	 * This table is naturally a part of the proc table but could be 
	 * configurable in size. 
	 * It is undesirable to change the size of the proc table at
	 * sysgen time because of the programs that know about its structure.
	 * This is the same trick used in SysV to deal with the pregion table.
	 */
	for (i=0; i<nproc; i++) {
		proc[i].p_addr = &umap[i*UPAGES];
		proc[i].p_tlbinfo = &wtlb[i*NPAGEMAP];
	}

	/*
	 * initialize proc[0]'s u page
	 */
	for (i = 0; i < UPAGES; i++) {
		*(int *)&proc[0].p_addr[i] =
		 	(fpage << PTE_PFNSHIFT)|PG_V|PG_M|PG_G|PG_KW;
#ifndef SABLE
		clearseg(fpage);
#endif
		fpage++;
		tlbwired(TLBWIREDBASE+i, 0, UADDR+(i*NBPG),
			proc[0].p_addr[i]);
	}
	clear_tlbmappings(&proc[0]);

	up = &u;	/* handle on u for debugging */
	u.u_pcb.pcb_cpuptr = &boot_cpudata;  /* put the cpudata pointer in the real u */
	u.u_ar0 = (int *)&USER_REG(0);
	u.u_procp = &proc[0];
	init_tlbpid();
	u.u_procp->p_tlbpid = -1;
	get_tlbpid(u.u_procp);

	/*
	 * Now allocate space for core map
	 * Allow space for all of physical memory minus the amount 
	 * dedicated to the system. fpage indicates the first 
	 * physical page currently available.
	 */
#if NOMEMCACHE==1
	paddr = (caddr_t)PHYS_TO_K1(ctob(fpage));
#else
	paddr = (caddr_t)PHYS_TO_K0(ctob(fpage));
#endif

	/* Calculate # of physical memory pages for buffer cache */
	base = bufpages / nbuf;
	residual = bufpages % nbuf;
	bcpages = (residual * ((base + 1) * CLSIZE)) +
			((nbuf - residual) * (base * CLSIZE));
	/*
	 * Allocate one cmap entry for the head of the list.
	 */
	palloc(cmap, struct cmap, 1);
	/*
	 * Calculate how may cmap entry/page frame pairs will fit in
	 * the remaining physical memory.
	 */
	ncmap = (ctob(maxmem - (fpage+bcpages)))/(sizeof(struct cmap)+ctob(1));
	palloc(ecmap, struct cmap, ncmap);
	ecmap = (struct cmap *)paddr;
	fpage = btoc(K0_TO_PHYS((u_int)paddr));

	/* Save PFN of last kernel page to dump for crashdump code */
	lastkpage = fpage;

	/*
	 * Allocate space for mapped system data structures.
	 * The first available real memory page is in "fpage".
	 * The first available kernel virtual address is in "v".
	 * As kernel virtual address space is allocated, "v" is incremented.
	 * A virtual page number for the kernel page table corresponding to
	 * the virtual memory address maintained in "v" is kept in "mapvpn".
	 * TODO: it would be more space efficient to allocate all page
	 * aligned things at once, followed by non-paged aligned structures.
	 */
	v = (caddr_t)K2BASE;
	mapvpn = btop(v);

#define	valloc(name, type, num) \
	    (name) = (type *)(v); \
	    (v) = (caddr_t)((name)+(num))

#define	valloclim(name, type, num, lim) \
	    (name) = (type *)(v); \
	    (v) = (caddr_t)((lim) = ((name)+(num)))

if (bufdebug) {
printf("startup: valloc buffers, nbuf %d MAXBSIZE %d v 0x%x\n",
	nbuf, MAXBSIZE, v);
}
	valloc(buffers, char, MAXBSIZE * nbuf);
	base = bufpages / nbuf;	/* pages of physical memory per buf struct */
	residual = bufpages % nbuf;

if (bufdebug) {
printf("startup: base (pages/buf) = %d, residual = %d\n", base, residual);
}

	for (i = 0; i < residual; i++) {
		for (j = 0; j < (base + 1) * CLSIZE; j++) {
			if (cache_bufcache)
				mapin(mapvpn + j, fpage, PG_V|PG_KW);
			else
				mapin(mapvpn + j, fpage, PG_V|PG_KW|PG_N);
#ifndef SABLE
			clearseg((unsigned)fpage);
#endif
			fpage++;
		}
		mapvpn += MAXBSIZE / NBPG;
	}
	for (i = residual; i < nbuf; i++) {
		for (j = 0; j < base * CLSIZE; j++) {
			if (cache_bufcache)
				mapin(mapvpn + j, fpage, PG_V|PG_KW);
			else
				mapin(mapvpn + j, fpage, PG_V|PG_KW|PG_N);
#ifndef SABLE
			clearseg((unsigned)fpage);
#endif
			fpage++;
		}
		mapvpn += MAXBSIZE / NBPG;
	}

	if (fpage >= maxmem - 8*UPAGES)
		panic("no memory");

	/*
	 * Initialize callouts
	 */
	callfree = callout;
	for (i = 1; i < ncallout; i++)
		callout[i-1].c_next = &callout[i];

	/*
	 * Initialize memory allocator and swap
	 * and user page table maps.
	 *
	 * THE USER PAGE TABLE MAP IS CALLED ``kernelmap''
	 * WHICH IS A VERY UNDESCRIPTIVE AND INCONSISTENT NAME.
	 */
	meminit(fpage, maxmem);
	maxmem = freemem;	/* freemem is a global set by meminit */
 /*
  * km_alloc's can be used after this
  */
        kmeminit();
	rminit(kernelmap, (long)usrptsize, (long)1,
	    "usrpt", nproc*3);
}


/*
 * Size memory by walking thru memory invoking the BADADDR macro,
 * which calls a processor specific badaddr routine.
 */
msize_baddr(fpage)
{
	register int i;

	causevec[EXC_DBE>>CAUSE_EXCSHIFT] = VEC_nofault;
	for (i = fpage; i < btop(K0SIZE); i++) {
		if (BADADDR(PHYS_TO_K1((unsigned)ptob(i)),4))
			break;
		clearseg(i);
	}
	causevec[EXC_DBE>>CAUSE_EXCSHIFT] = VEC_trap;
	return(i);
}

/*
 * Size memory by using the bitmap.
 */
msize_bitmap(fpage)
{
	register int i,j;
        int *bitmap, memsize, start = 0;
	u_int bitmaplen;
	struct mem_bitmap *mbmp;

	if(console_magic == REX_MAGIC) {
		mbmp = rex_map;
		/* rex_getbitmap returns number of bytes, the algorithm */
		/* below uses number of long words */
		bitmaplen = rex_getbitmap(rex_map) / 4;
		if (mbmp->mbmap_pagesize != NBPG) 
			rex_printf("msize_bitmap: bitmap page size not NBPG\n");   /* DAD */
		bitmap = (int *)mbmp->mbmap_bitmap;
	}
	else {
		bitmap = (int *)xtob((char *)prom_getenv("bitmap"));
		bitmaplen = xtob(prom_getenv("bitmaplen"));
	}

	memsize = 0;
	/* if DS_5000, rom is returning the first and third 64k chunks as bad */
	/* assume the are good */
	if (cpu == DS_5000) {
	    bitmap += 2;
	    memsize += 64;
	    start = 2; /* start at the second longword, since we hardwired the */
	               /* first two */
	}
	for (i = start; i < bitmaplen; i++, bitmap++) {
	    if (*bitmap == 0xffffffff) {
		memsize += 32;
		continue;
	    } else {
		for (j = 0; j < 32; j++) {
		    if (*bitmap & (1 << j))
			memsize += 1;
		    else
			break;
		}
		break;
	    }
	}
	if ((cpu == DS_5800) || (cpu == DS_5500) || (cpu == DS_5100)) {
		/*
		 * Bitmap page representation is 512 byte pages,
		 * so convert to natural machine page size. 
		 *
		 * Take the number of (512-size) pages * 512 to get the
		 * total memory size, then divide by 4k by shifting
		 * right 12, to get the number of 4k pages available.
		 * This drops the remainder, hence leaving any partial
		 * page as unusable. This corrects the problem of
		 * marking bitmaps which don't fall on 4k boundries
		 * as unusable which was caused by using btoc().
		 *
		 */
		memsize = (memsize * 512) >> PGSHIFT ;
	}
	for (i = fpage; i < memsize; i++) {
		clearseg(i);
	}
	return(memsize);
}

init_restartblk()
{
	register struct restart_blk *rb = (struct restart_blk *)RESTART_ADDR;
	register struct save_state *sst = (struct save_state *)SST_ADDR;
	register int i, sum;
	extern int doadump();

	rb->rb_magic = RESTART_MAGIC;
	rb->rb_occurred = 0;
	rb->rb_restart = doadump;

	sum = 0;
	for (i = 0; i < RESTART_CSUMCNT; i++)
		sum += ((int *)rb->rb_restart)[i];

	rb->rb_checksum = sum;
	/*
	 * Save state for ULTRIX
	 */
	sst->sst_magic = SST_MAGIC;
	sst->sst_dump = doadump;
	sst->cpu = cpu;
}

/*
 * Determine what we are running on and return the system type.
 * To be used as the index into the cpu switch (system specific switch table).
 *
 * Parameter:
 *	cpu_systype		Entire system type work from the PROM
 * 
 * Return:
 *	Value to be stored in cpu, defined in cpuconf.h
 */
system_type(cpu_systype)
	unsigned  cpu_systype;
{
	int ret_val = UNKN_SYSTEM;	/* Assume we don't know yet */

	switch (GETCPUTYPE(cpu_systype)) {
	case R3000_CPU:
	/* case R2000a_CPU: */
		switch (GETSYSTYPE(cpu_systype)) {
		case ST_DS3100:
			ret_val = DS_3100;
			break;
		case ST_DS5000:
			ret_val = DS_5000;
			break;
		case ST_DS5000_100:
			ret_val = DS_5000_100;
			break;
		case ST_DS5100:
			ret_val = DS_5100;
			break;
		case ST_DS5400:
			ret_val = DS_5400;
			break;
		case ST_DS5500:
			ret_val = DS_5500;
			break;
		case ST_DS5800:
			ret_val = DS_5800;
			break;
		}
		break;
	}
	return(ret_val);
}

/*
 * Get pointer to cpusw table entry for the system we are currently running
 * on.  The pointer returned by this routine will go into "cpup".
 *
 * The "cpu" variable (ULTRIX system type) is passed in and compared to the
 * system_type entry in the cpusw table for a match.
 */
struct cpusw *
cpuswitch_entry(cpu)
	int cpu;			/* the ULTRIX system type */
{
	register int i;			/* loop index */

	for (i = 0; cpusw[i].system_type != 0; i++) {
		if (cpusw[i].system_type == cpu)
			return(&cpusw[i]);
	}
	panic("processor type not configured");
}

