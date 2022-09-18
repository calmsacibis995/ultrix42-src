#ifndef lint
static char	*sccsid = "@(#)pstat.c	4.5	(ULTRIX)	3/7/91";
#endif

/************************************************************************
 *									*
 *			Copyright (c) 1988, 1990 by			*
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

/* ------------------------------------------------------------------------
 * Modification History: /usr/src/etc/pstat.c
 *
 * 14 Nov 90 -- paradis
 *	Added -v flag to display vector processes.
 *
 * 18 Sept 90 -- Philip C. Gapuz Te
 *      Added scc_tty to name list for 3MIN serial line driver.
 *
 * 5 June 90 - Kuo-Hsiung Hsieh 
 *	Added mdc_tty name list for mipsmate serical line driver.
 *
 * 12 Jan 90 -- jaa
 *	added reserved virtual address field to show swap 
 *	thats reserved but not allocated
 *
 * 08 Dec 89 --	sekhar
 *	Fixed the following:
 * 	    1. Complete rewrite of doswap() code to handle dynamic swap changes
 *             Dynamic swap useage is now tracked in a kernel data structure
 *             swapu which is read here.
 *	    2. print usage information when no parameters are specified to 
 *	       the pstat command.
 *	    3. modify klseek to work on mips.
 *	    4. fixed to work with new pte layout on mips.
 * 
 * 12 Jun 89 -- gg
 *	Modified routine doswap() to reflect the dynamic swap changes.
 *	Also modified doproc() routine to print the address of the 
 *	data and stack segment dmap structures.
 *
 * 12 Oct 88 - Tim Burke
 *	Print out stats on firefox driver for the -t option.
 *
 * 6 Sep 88 -rnf
 *	Added exit status of 0 on successful completion
 * 
 * 9 Jun 88
 *	Bugs fixes to doswap
 * 
 * 12 Apr 88 -jaa
 *	Added check for errors on calloc()'s
 *	Fixed printing of text information now tha text is km_alloc'd
 * 
 * 03 Mar 88 -- Tim Burke
 *	Added ability to print out terminal status for lat lines.  Also added
 *	checks for line disciplines TERMIODISC and SLPDISC.
 *	Display the following tty flag: TS_OABORT.
 *
 *  2 Jun 88 -- Tim Burke
 *	Added "-l" option to print out smp lock contention statistics.
 *	This is only useful when the kernel has been compiled with SMP_STAT
 *	defined.
 *
 * 24 Feb 88 -- depp
 *      Remove reference to argdev in doswap() as argdev is now km_alloc'ed.
 *      Added "swap configured" line to -s option.
 *
 * 23 Dec 87 -- Tim Burke
 *	Added CLKT functionality for pstat -p.  Also added TTYP field to
 * 	pstat -p because this field has been moved from the u struct to
 *	the proc struct.
 *
 * 29 Jan 87 -- Marc Teitelbaum
 *	Check for INUSE flag on inode, display as "I".  Check for
 *	FBLKINUSE on file flags, display as "B". Add checks for
 *	following tty flags:
 *		TT_STOP  - 'S'
 *		TT_INUSE - 'I'
 *		TT_ONDELAY - 'D'
 *		TT_IGNCAR - 'G'
 *		TT_NBIO  - 'N'
 *		TT_ASYNC - 'Z'
 *	
 *
 * 26 Jan 86 -- depp
 *	Changed method that text swap space is calculated, do to the
 *	changes in text table allocation/deallocation.  The text table
 *	allocation/deallocation changes also made changes to dotext().
 *
 *  4 Dec 86 -- Tim burke
 *	Added support for dmb driver as part of the pstat -t option.
 *
 * 15 Sep 86 -- bglover 
 *	Change refs to x_iptr to x_gptr (text.h change)
 *
 * 22 Aug 86 -- fred (Fred Canter)
 *	Added support for VAXstar and MICROVAX 1800 serial line
 *	device drivers to the pstat -t option. The ss device is
 *	like the dzq11 and the sh is like an 8 line dhu11.
 *
 *  8 Apr 86 -- depp
 *	Removed reference to u.u_exdata, as it no longer exists
 *
 * 14 Oct 85 -- rr
 *	Fixed the handling of the -u ubase option. First problem
 *	was that the assumption was made that the UPAGES+CLSIZE
 *	preallocated 512 byte pages were contiguous and they are not.
 *	Stole the code from ps.c to read in each CLICK (1024 presently)
 *	and modified ps.c to print out the page table page frame
 *	so that all we do in pstat.c is lseek and back up exactly
 *	UPAGES+CLSIZE pte's, read the pte's, and then get each page
 *	one click at a time. Hopefully, this extends the life of this routine.
 *	Also, added new fields to the printout so that all fields are
 *	now dumped by the pstat -u ubase option.
 *
 * 23 Feb 84 -- jmcg
 *	Fixed -k flag handling.  Now that bufpages are allocated
 *	sparsely in virtual space to system buffers, there is no
 *	longer a simple mapping between kernel virtual addresses
 *	and physical memory.  /dev/kmem provides this mapping when
 *	running on a live system, but pstat must decipher the system
 *	page table itself when running against a vmcore.  These changes
 *	were originally made and tested around 24 Jan 84.  They are
 *	almost identical to those provided over the Usenet by A R White
 *	from University of Waterloo (newsgroup net.bugs.4bsd
 *	<6927@watmath.UUCP> 17 Feb 1984).
 *
 * 23 Feb 84 --jmcg
 *	Derived from 4.2BSD, labeled:
 *		pstat.c	4.22 (Berkeley) 6/18/83
 *	with an update obtained by aps from ucbmonet.  He updated the
 *	sccsid to read:
 *	*sccsid = "@(#)pstat.c	1.4 (Ultrix 1.0)  (from 4.24 Berkeley) 2/24/84"
 *	but lost the date.
 *
 * ------------------------------------------------------------------------
 */

/*
 * Print system stuff
 */

#define mask(x) (x&0377)
#define	clear(x) ((int)x&0x7fffffff)

#include <sys/param.h>
#include <sys/dir.h>
#define	KERNEL
#include <sys/file.h>
#undef	KERNEL
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/text.h>
#include <sys/gnode.h>
#include <sys/map.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/conf.h>
#include <sys/vm.h>
#include <sys/time.h>
#include <nlist.h>
#include <machine/pte.h>
#ifdef vax
#include <machine/vectors.h>
#endif /* vax */

#include <sys/ipc.h>
#include <sys/shm.h>

#undef SMP_STAT

char	*fcore	= "/dev/kmem";
char	*fmem	= "/dev/mem";
char	*fnlist	= "/vmunix";
int	fc, fm;

struct nlist nl[] = {
#define	SGNODE	0
	{ "_gnode" },
#define	STEXT	1
	{ "_text" },
#define	SPROC	2
	{ "_proc" },
#define	SDZ	3
	{ "_dz_tty" },
#define	SNDZ	4
	{ "_dz_cnt" },
#define	SKL	5
	{ "_cons" },
#define	SFIL	6
	{ "_file" },
#define	USRPTMA	7
	{ "_Usrptmap" },
#define	USRPT	8
	{ "_usrpt" },
#define	SWAPMAP	9
	{ "_swapmap" },
#define	SDH	10
	{ "_dh11" },
#define	SNDH	11
	{ "_ndh11" },
#define	SNPROC	12
	{ "_nproc" },
#define	SNTEXT	13
	{ "_ntext" },
#define	SNFILE	14
	{ "_nfile" },
#define	SNGNODE	15
	{ "_ngnode" },
#define	SNSWAPMAP 16
	{ "_nswapmap" },
#define	SPTY	17
	{ "_pt_tty" },
#define	SNSWDEV	18
	{ "_nswdev" },
#define	SSWDEVT	19
	{ "_swdevt" },
#define SSYSMAP	20
	{ "_Sysmap" },
#define SSYSSIZE	21
	{ "_Syssize" },
#define SNPTY	22
	{ "_nNPTY" },
#define SDHU	23
	{ "_dhu11" },
#define SNDHU	24
	{ "_ndhu11"},
#define SDMF	25
	{ "_dmf_tty"},
#define SNDMF	26
	{ "_ndmf" },
#define SDMZ	27
	{ "_dmz_tty" },
#define SNDMZ	28
	{ "_ndmz" },
#define SSS	29
	{ "_ss_tty" },
#define SNSS	30
	{ "_ss_cnt" },
#define SSH	31
	{ "_sh_tty" },
#define SNSH	32
	{ "_sh_cnt" },
#define SDMB	33
	{"_dmb_tty"},
#define SNDMB	34
	{"_ndmb"},
#define SLAT	35
	{"_lata"},
#define SNLAT	36
	{"_nLAT1"},
#define SSMEM   37
	{"_smem"},
#define SSMINFO 38
	{"_sminfo"},
#define SNLKDMB	39
	{"_lk_dmb"},
#define SNCFREE	40
	{"_cfreelist"},
#define SPROC_BITMAP	41
	{"_proc_bitmap"},
#define SFC	42
	{"_fc_tty"},
#define SNFC	43
	{"_fc_cnt"},
#define SSWAPFRAG 44
	{"_swapfrag"},
#define SSWAPU 45
	{"_swapu"},
#define AVAILVAS 46
	{"_availvas"},
#define SDC 47
        {"_dc_tty"},
#define SNDC 48
        {"_dc_cnt"},
#define SMDC 49
        {"_mdc_tty"},
#define SNMDC 50
        {"_mdc_cnt"},
#define SSCC 51
        {"_scc_tty"},
#define SNSCC 52
        {"_scc_cnt"},
	{ "" }
};

struct ttytype {
	char *name;
	int ttybase;
	int nttys;
} ttytypes[] = {
	{ "dz", SDZ, SNDZ },
	{ "ss", SSS, SNSS },
	{ "dh", SDH, SNDH },
	{ "dhu", SDHU, SNDHU },
	{ "sh", SSH, SNSH },
	{ "dmf", SDMF, SNDMF },
	{ "dmz", SDMZ, SNDMZ },
	{ "dmb", SDMB, SNDMB },
	{ "fc", SFC, SNFC },
        { "dc", SDC, SNDC },
        { "mdc", SMDC, SNMDC },
        { "scc", SSCC, SNSCC },
	{ "pty", SPTY, SNPTY },
	{ "lat", SLAT, SNLAT },
	{ "" }
};
int	inof;
int	txtf;
int	prcf;
int	ttyf;
int	vecf;
int	ttylock;
int	smp_title;
int	usrf;
long	ubase;
int	filf;
int	swpf;
int	totflg;
char	partab[1];
struct	cdevsw	cdevsw[1];
struct	bdevsw	bdevsw[1];
int	allflg;
int	kflg;
struct	pte *Usrptma;
struct	pte *usrpt;
struct	pte *Sysmap = 0;
int	sizeSysmap;

main(argc, argv)
char **argv;
{
	register char *argp;
	register int allflags;

	if (argc == 1) {
		usage();
		exit(1);
	}
	argc--, argv++;
	while (argc > 0 && **argv == '-') {
		argp = *argv++;
		argp++;
		argc--;
		while (*argp++)
		switch (argp[-1]) {

		case 'T':
			totflg++;
			break;

		case 'a':
			allflg++;
			break;

		case 'i':
			inof++;
			break;

		case 'k':
			kflg++;
			fcore = fmem = "/vmcore";
			break;

		case 'x':
			txtf++;
			break;

		case 'p':
			prcf++;
			break;

		case 't':
			ttyf++;
			break;

		case 'u':
			if (argc == 0)
				break;
			argc--;
			usrf++;
			sscanf( *argv++, "%x", &ubase);
			break;
#ifdef vax
		case 'v':
			vecf++;
			break;
#endif /* vax */
		case 'f':
			filf++;
			break;
		case 's':
			swpf++;
			break;
		case 'l':
			/*
			 * SMP lock stats.  Intended for proto pool only due to
			 * the fact that SMP_STAT will not be defined in
			 * production versions.
			 */
			switch (argp[0]) {
				/*
				 * tty stats
				 */
				case 't':
					ttylock++;
					break;
				default:
					printf("usage: pstat -l[t]\n"); 
					exit(1);
			}
			argp++;
			break;
		default:
			usage();
			exit(1);
		}
	}
	if (argc>1)
		fcore = fmem = argv[1];
	if ((fc = open(fcore, 0)) < 0) {
		printf("Can't find %s\n", fcore);
		exit(1);
	}
	if ((fm = open(fmem, 0)) < 0) {
		printf("Can't find %s\n", fmem);
		exit(1);
	}
	if (argc>0)
		fnlist = argv[0];
	nlist(fnlist, nl);
	usrpt = (struct pte *)nl[USRPT].n_value;
	Usrptma = (struct pte *)nl[USRPTMA].n_value;
	if (nl[0].n_type == 0) {
		printf("no namelist\n");
		exit(1);
	}
	allflags = filf | totflg | inof | prcf | vecf | txtf | ttyf | usrf | swpf | ttylock;
	if (allflags == 0) {
#ifdef vax
		printf("pstat: one or more of -[aixpvtfsu] is required\n");
#else
		printf("pstat: one or more of -[aixptfsu] is required\n");
#endif /* vax */
		exit(1);
	}
	if (filf||totflg)
		dofile();
	if (inof||totflg)
		dognode();
	if (prcf||totflg)
		doproc();
#ifdef vax
	if (vecf||totflg)
		dovec();
#endif /* vax */
	if (txtf||totflg)
		dotext();
	if (ttyf)
		dotty();
	if (usrf)
		dousr();
	if (swpf||totflg)
		doswap();
	if (ttylock)
		dotty();
	exit(0);
}

usage()
{

	printf("usage: pstat -[akixptfs] [-u [ubase]] [system] [core]\n");
}

dognode()
{
	register struct gnode *gp;
	struct gnode *xgnode;
	register struct gnode *agnode;
	register int ngn;
	register int ngnode;

	ngn = 0;
	ngnode = getw(nl[SNGNODE].n_value);
	if((xgnode = (struct gnode *)calloc(ngnode, sizeof (struct gnode))) == 
	(struct gnode *) NULL) {
		perror("calloc: dognode");
		exit(1);
	}
	klseek(fc, (int)(agnode = (struct gnode *)getw(nl[SGNODE].n_value)), 0);
	read(fc, xgnode, ngnode * sizeof(struct gnode));
	for (gp = xgnode; gp < &xgnode[ngnode]; gp++)
		if (gp->g_count)
			ngn++;
	if (totflg) {
		printf("%4d/%4d\tgnodes\n", ngn, ngnode);
		return;
	}
	printf("%d/%d active gnodes\n", ngn, ngnode);
printf("   LOC      FLAGS     CNT DEVICE  RDC WRC  GNO  MODE  NLK UID   SIZE/DEV\n");
	for (gp = xgnode; gp < &xgnode[ngnode]; gp++) {
		if (gp->g_count == 0)
			continue;
		printf("%8.1x ", agnode + (gp - xgnode));
		putf(gp->g_lk.l_lock, 'L');
		putf(gp->g_flag&GUPD, 'U');
		putf(gp->g_flag&GACC, 'A');
		putf(gp->g_flag&GMOUNT, 'M');
		putf(gp->g_flag&GWANT, 'W');
		putf(gp->g_flag&GTEXT, 'T');
		putf(gp->g_flag&GINUSE, 'I');
		putf(gp->g_flag&GCHG, 'C');
		putf(gp->g_flag&GSHLOCK, 'S');
		putf(gp->g_flag&GEXLOCK, 'E');
		putf(gp->g_flag&GLWAIT, 'Z');
		printf("%4d", gp->g_count&0377);
		printf("%4d,%3d", major(gp->g_dev), minor(gp->g_dev));
		printf("%4d", gp->g_shlockc&0377);
		printf("%4d", gp->g_exlockc&0377);
		printf("%6d", gp->g_number);
		printf("%6x", gp->g_mode & 0xffff);
		printf("%4d", gp->g_nlink);
		printf("%4d", gp->g_uid);
		if ((gp->g_mode&GFMT)==GFBLK || (gp->g_mode&GFMT)==GFCHR)
			printf("%6d,%3d", major(gp->g_rdev), minor(gp->g_rdev));
		else
			printf("%10ld", gp->g_size);
		printf("\n");
	}
	free(xgnode);
}

getw(loc)
	unsigned long loc;
{
	int word;

	klseek(fc, loc, 0);
	read(fc, &word, sizeof (word));
	return (word);
}

putf(v, n)
{
	if (v)
		printf("%c", n);
	else
		printf(" ");
}

dotext()
{
	register struct text *xp;
	register int ntext;
	struct text *xtext;
	register struct text *atext;
	register int nrecl=0,nactive=0;

	ntext = getw(nl[SNTEXT].n_value);
	if((xtext = (struct text *)calloc(ntext, sizeof (struct text))) == 
	(struct text *) NULL) {
		perror("calloc: dotext");
		exit(1);
	}
	klseek(fc, (int)(atext = (struct text *)getw(nl[STEXT].n_value)), 0);
	read(fc, xtext, ntext * sizeof (struct text));
	for (xp = xtext; xp < &xtext[ntext]; xp++) {
		if (xp->x_gptr!=NULL && (xp->x_flag & XFREE))
			nrecl++;
		if ((xp->x_flag & XFREE) == 0)
			nactive++;
	}
	printf("%4d/%4d/%4d\tactive/reclaimable/total texts\n",
					nactive, nrecl, ntext);
	if (totflg) return;
	printf("   LOC   FLAGS        DADDR    CADDR    RSS SIZE   GPTR   CNT CCNT LCNT POIP\n");
	for (xp = xtext; xp < &xtext[ntext]; xp++) {
		if (xp->x_gptr == NULL)
			continue;
		printf("%8.1x", atext + (xp - xtext));
		printf(" ");
		putf(xp->x_flag&XPAGI, 'P');
		putf(xp->x_flag&XTRC, 'T');
		putf(xp->x_flag&XWRIT, 'W');
		putf(xp->x_flag&XLOAD, 'L');
		putf(xp->x_flag&XLOCK, 'K');
		putf(xp->x_flag&XFREE, 'F');
		putf(xp->x_flag&XWANT, 'w');
		putf(xp->x_flag&XNOSW, 'l');
		putf(xp->x_flag&XBAD, 'B');
		putf(xp->x_flag&XREMOTE, 'R');
		printf("%9x", xp->x_dmap);
		printf("%9.1x", xp->x_caddr);
		printf("%5d", xp->x_rssize);
		printf("%5d", xp->x_size);
		printf("%9.1x", xp->x_gptr);
		printf("%4d", xp->x_count&0377);
		printf("%4d", xp->x_ccount);
		printf("%5d", xp->x_lcount);
		printf("%5d", xp->x_poip);
		printf("\n");
	}
	free(xtext);
}

doproc()
{
	struct proc *xproc;
	register struct proc *aproc;
	register int nproc;
	register struct proc *pp;
	register int np;
	struct timeval curr_time;
	struct pte apte;
	register int ten_msec;
	int *bit_map;
	int i;

	nproc = getw(nl[SNPROC].n_value);
	if((xproc = (struct proc *)calloc(nproc, sizeof (struct proc))) == 
	(struct proc *) NULL) {
		perror("calloc: doproc");
		exit(1);
	}
	klseek(fc, (int)(aproc = (struct proc *)getw(nl[SPROC].n_value)), 0);
	read(fc, xproc, nproc * sizeof (struct proc));
	bit_map = (int *) calloc((nproc+31)/32,sizeof (int));

	klseek(fc, (int) nl[SPROC_BITMAP].n_value, 0);
	read(fc, bit_map, ((nproc+31)/32) * sizeof (int));

	np = 0;
	for(i=0; i< nproc; i++)
		if (bit_map[(i/32)] & (1<<(i%32)))
			np++;

	if (totflg) {
		printf("%4d/%4d\tprocesses\n", np, nproc);
		return;
	}
	gettimeofday(&curr_time, 0);
	printf("%d/%d processes\n", np, nproc);
	printf("   LOC    S    F POIP PRI      SIG  UID SLP TIM  CPU  NI   PGRP    PID   PPID    ADDR   RSS SRSS SIZE    WCHAN     LINK   TEXTP CLKT  TTYP LOCK DMAP    SMAP\n");

	for (pp=xproc, i=0; pp<&xproc[nproc]; pp++,i++) {
		if (((bit_map[(i/32)] & (1<<(i%32))) == 0)
			&& allflg ==0) continue;

		printf("%8x", aproc + (pp - xproc));
		printf(" %2d", pp->p_stat);
		printf(" %4x", (pp->p_sched | pp->p_select | pp->p_vm |
				pp->p_trace |
				pp->p_type |pp->p_file) & 0xffff);
		printf(" %4d", pp->p_poip);
		printf(" %3d", pp->p_pri);
		printf(" %8x", pp->p_sig);
		printf(" %4d", pp->p_uid);
		printf(" %3d", pp->p_slptime);
		printf(" %3d", pp->p_time);
		printf(" %4d", pp->p_cpu&0377);
		printf(" %3d", pp->p_nice);
		printf(" %6d", pp->p_pgrp);
		printf(" %6d", pp->p_pid);
		printf(" %6d", pp->p_ppid);
		klseek(fc, (long)(Usrptma+btokmx(pp->p_addr)), 0);
		read(fc, &apte, sizeof(apte));
		printf(" %8x", apte.pg_pfnum);	/* print page of page table */
		printf(" %4x", pp->p_rssize);
		printf(" %4x", pp->p_swrss);
		printf(" %5x", pp->p_dsize+pp->p_ssize);
		printf(" %8x", clear(pp->p_wchan));
		printf(" %7x", clear(pp->p_link));
		printf(" %7x", clear(pp->p_textp));
		if ((pp->p_realtimer.it_value.tv_sec == 0) && 
		    (pp->p_realtimer.it_value.tv_usec == 0))
			printf("     ");
		else if (timercmp(&pp->p_realtimer.it_value,&curr_time,<))
			printf("  %3d",-1);
		else {
			ten_msec = (pp->p_realtimer.it_value.tv_sec - 
				curr_time.tv_sec) * 100;
			ten_msec += ((pp->p_realtimer.it_value.tv_usec - 
				curr_time.tv_usec) / 10000);
			if (ten_msec > 1000000) 
				printf(" %3dM",ten_msec / 1000000);
			else if (ten_msec > 1000) 
				printf(" %3dK",ten_msec / 1000);
			else
				printf("  %3d",ten_msec);
		}
		printf(" %5x", clear(pp->p_ttyp));
		printf(" %8x", pp->p_hlock);
		printf(" %8x", pp->p_dmap);
		printf(" %8x", pp->p_smap);

		printf("\n");
	}
}

#ifdef vax
dovec()
{
	struct proc *xproc;
	register struct proc *aproc;
	register int nproc;
	register struct proc *pp;
	register int nvp;
	struct vpcontext	vpc;
	int i;
	int nocxt;

	nproc = getw(nl[SNPROC].n_value);
	if((xproc = (struct proc *)calloc(nproc, sizeof (struct proc))) == 
	(struct proc *) NULL) {
		perror("calloc: doproc");
		exit(1);
	}
	klseek(fc, (int)(aproc = (struct proc *)getw(nl[SPROC].n_value)), 0);
	read(fc, xproc, nproc * sizeof (struct proc));

	nvp = 0;
	for(i=0; i< nproc; i++) {
		if(xproc[i].p_vpcontext) {
			nvp++;
		}
	}

	printf("%d/%d vector processes\n", nvp, nproc);

	printf("   LOC     PGRP    PID   PPID   VST   VERRS  REFS CHPCXT EXPCXT\n");
	for (pp=xproc, i=0; pp<&xproc[nproc]; pp++,i++) {
		if(pp->p_vpcontext) {
			klseek(fc, pp->p_vpcontext, 0);
			read(fc, &vpc, sizeof(struct vpcontext));

			printf("%8x %6d %6d %6d ", aproc + (pp - xproc),
				pp->p_pgrp, pp->p_pid, pp->p_ppid);
			nocxt = 0;
			switch(vpc.vpc_state) {
				case VPC_WAIT:	
					printf(" WAIT ");	
					break;
				case VPC_LOAD:	
					printf(" LOAD ");	
					break;
				case VPC_SAVED:	
					printf("SAVED ");	
					break;
				case VPC_LIMBO:	
					printf("LIMBO ");	
					break;
				default:	
					printf("  ??? ");	
					nocxt = 1;
					break;
			}

			if(!nocxt) {
			    printf("%6d %6d %6d %6d\n", vpc.vpc_error,
				vpc.vpc_refuse, vpc.vpc_cheap, vpc.vpc_expen);
			}
			else {
			    printf("\n");
			}
		}
		
	}
}
#endif /* vax */

dotty()
{
	struct tty dz_tty[256];
	int ndz;
	register struct tty *tp;
	register char *mesg;
	register struct ttytype *ttype;
	struct lock_t dmb_lock[16], cfree_lock;
	int i;

	if (ttylock == 0) {
		mesg = " # RAW CAN OUT    MODE    ADDR   DEL COL  STATE         PGRP DISC\n";
		printf(mesg);
	}
	else {
		smp_title++;
	}
	klseek(fc, (long)nl[SKL].n_value, 0);
	read(fc, dz_tty, sizeof(dz_tty[0]));
	/*
	 * The console will be on ss line 0 or 3 if the
	 * CPU is a VAXstar or MICROVAX 1800. Don't print
	 * the console line if this is the case.
	 */
	tp = dz_tty;
	if((tp->t_state&TS_ISOPEN) || (((long)nl[SSS].n_type == 0) &&
	   ((long)nl[SDC].n_type == 0)) && ((long)nl[SMDC].n_type == 0)) {
		printf("\n1 console\n"); /* console */
		ttyprt(&dz_tty[0], 0);
	}
	for (ttype=ttytypes; *ttype->name != '\0'; ttype++) {
		if (nl[ttype->nttys].n_type == 0)
			continue;
		klseek(fc, (long)nl[ttype->nttys].n_value, 0);
		read(fc, &ndz, sizeof(ndz));
		/*
		 * Display DMB lock status for the per-board lock.
		 */
		if ((ttype->ttybase == SDMB) && (nl[SDMB].n_type != 0)) {
			klseek(fc, (long)nl[SNLKDMB].n_value, 0);
			read(fc, dmb_lock, ((sizeof (dmb_lock)) * ndz/16));
			for (i = 0; i < ndz/16; i++) {
				printf("\n dmb board %d lock statistics\n",i);
				smp_stat(-1,dmb_lock[i].l_spin,dmb_lock[i].l_lost,
					dmb_lock[i].l_won);
				smp_title++;
			}
		}
		printf("\n%d %s lines\n", ndz, ttype->name);
		klseek(fc, (long)nl[ttype->ttybase].n_value, 0);
		read(fc, dz_tty, ndz * sizeof (struct tty));
		for (tp = dz_tty; tp < &dz_tty[ndz]; tp++)
			ttyprt(tp, tp - dz_tty);
		if (ttylock)
			smp_title++;
	}
}

ttyprt(atp, line)
struct tty *atp;
{
	register struct tty *tp;

	tp = atp;
    if (ttylock == 0) {
	printf("%2d", line);
	printf("%4d ", tp->t_outq.c_cc);
	printf("%8.1x", tp->t_flags);
	printf(" %8.1x", tp->t_addr);
	printf("%3d", tp->t_delct);
	printf("%4d ", tp->t_col);
	putf(tp->t_state&TS_TIMEOUT, 'T');
	putf(tp->t_state&TS_WOPEN, 'W');
	putf(tp->t_state&TS_ISOPEN, 'O');
	putf(tp->t_state&TS_CARR_ON, 'C');
	putf(tp->t_state&TS_BUSY, 'B');
	putf(tp->t_state&TS_ASLEEP, 'A');
	putf(tp->t_state&TS_XCLUDE, 'X');
	putf(tp->t_cflag&HUPCL, 'H');
	putf(tp->t_state&TS_TTSTOP, 'S');
	putf(tp->t_state&TS_INUSE, 'I');
	putf(tp->t_state&TS_ONDELAY, 'D');
	putf(tp->t_state&TS_IGNCAR, 'G');
	putf(tp->t_state&TS_NBIO, 'N');
	putf(tp->t_state&TS_ASYNC, 'Z');
	putf(tp->t_state&TS_CLOSING, 'L');
	putf(tp->t_state&TS_OABORT, 'Q');
	printf("%6d", tp->t_pgrp);
	switch (tp->t_line) {

	case NTTYDISC:
		printf(" ntty");
		break;

	case NETLDISC:
		printf(" net");
		break;

	case HCLDISC:
		printf(" uucp");
		break;

	case TERMIODISC:
		printf(" termio");
		break;

	case SLPDISC:
		printf(" slp");
		break;
	}
	printf("\n");
    }
    /*
     * Display lock stats for SMP terminal drivers.
     */
    else {
	if (tp->t_smp) {
		smp_stat(line,tp->t_lk_tty.l_spin,tp->t_lk_tty.l_lost,tp->t_lk_tty.l_won);
  	}
    }
}

dousr()
{
	union {
		struct user user;
		char upages[UPAGES+CLSIZE][NBPG];
	} un_user;
#define U un_user.user
	register int i, j, *ip, ncl;
	register struct nameidata *nd = &U.u_nd;
	struct pte arguutl[UPAGES+CLSIZE];
	struct ucred ucred;
	int *ofile_of,fp;
	char *pofile_of,fflg;

	/* ubase is the 1st page of the user page table */
	/* We will use this to seek to then back up UPAGES+CLSIZE pte's */
	/* and read in the pte's to get all the pages of the u area */
	klseek(fm,(long)ctob(ubase)-(UPAGES+CLSIZE)*sizeof(struct pte),0);
	if (read(fm, (char *)arguutl, sizeof(arguutl)) != sizeof(arguutl)) {
		printf("pstat: cant read page table for u at addr %x\n",ubase);
		return;
	}
	/* Now figure out the number of clicks to read */
	ncl = (sizeof(struct user) + NBPG*CLSIZE - 1) / (NBPG*CLSIZE);
	/* Read them in reverse order into the union which serves */
	/* to let us read an even number of clicks even though the user */
	/* structure may be smaller than that. */
	while (--ncl >= 0) {
		i = ncl * CLSIZE;
		klseek(fm, (long)ctob(arguutl[CLSIZE+i].pg_pfnum), 0);
		if (read(fm, un_user.upages[i], CLSIZE*NBPG) != CLSIZE*NBPG) {
			printf("pstat: cant read page %d of u from %s\n",
			    arguutl[CLSIZE+i].pg_pfnum, fmem);
			return;
		}
	}
	printf("pcb (%d bytes)\n",sizeof(struct pcb));
	ip = (int *)&U.u_pcb;
	while (ip < (int *)((int)&U.u_pcb + sizeof(struct pcb))) {
		if ((ip - (int *)&U.u_pcb) % 4 == 0)
			printf("\t");
		printf("%9.1x ", *ip++);
		if ((ip - (int *)&U.u_pcb) % 4 == 0)
			printf("\n");
	}
	if ((ip - (int *)&U.u_pcb) % 4 != 0)
		printf("\n");
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
	printf("r_val?\t%9.1x %9.1x\n", U.u_r.r_val1, U.u_r.r_val2);
	printf("error\t%9.1x\n", U.u_error);
	printf("eosys\t%9.1x\n", U.u_eosys);
	printf("u_cred\t%x\n",U.u_cred);
	/* credentials is now km_alloc-ed -- have to go get them!!! - rr */
	klseek(fc,U.u_cred,0);
	read(fc,(char *)&ucred,sizeof(struct ucred));
	printf("\tref %d\n",ucred.cr_ref);
	printf("\tuid\t%d\tgid\t%d\n",ucred.cr_uid,ucred.cr_gid);
	printf("\truid\t%d\trgid\t%d\n",ucred.cr_ruid,ucred.cr_rgid);
	printf("\tgroups\t");
	for (i=0;i<NGROUPS;i++) {
		printf("%d",ucred.cr_groups[i]);
		if ((i==(NGROUPS/2 - 1))) printf("\n\t\t");
		else if (i<NGROUPS-1) putchar(',');
	}
	printf("\n");

	printf("sizes\t%d %d %d (clicks)\n", U.u_tsize, U.u_dsize, U.u_ssize);
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
	printf("sigintr\t%9.1x\n",U.u_sigintr);
	printf("oldmask   \t%9.1x\n",U.u_oldmask);
	printf("code      \t%9.1x\n", U.u_code);
	printf("sigstack  \t%9.1x\t%9.1x\n",
		U.u_sigstack.ss_sp,U.u_sigstack.ss_onstack);
	printf("file");
	if (U.u_of_count != 0) {
		ofile_of = (int *) malloc(U.u_of_count*
					  sizeof(int));
		pofile_of = (char *) 
			malloc(U.u_of_count);
		klseek(fc,U.u_ofile_of,0);
		read(fc,(char *)ofile_of,U.u_of_count*sizeof(int));
		klseek(fc,U.u_pofile_of,0);
		read(fc,(char *)pofile_of,U.u_of_count);
	}

	for (i=0; i<U.u_omax; i++) {

		if (i > NOFILE_IN_U) 
			fp = (int) ofile_of [i - NOFILE_IN_U];
		else 
			fp = (int)U.u_ofile[i];

		if (i % 8 == 0)
			printf("\t");
		printf("%9.1x", fp);
		if (i % 8 == 7)
			printf("\n");
	}
	if (U.u_omax % 8 != 0) printf("\n");
	printf("pofile");
	for (i=0; i<NOFILE; i++) {
		if (i > NOFILE_IN_U) 
			fflg = pofile_of [i - NOFILE_IN_U];
		else 
			fflg = U.u_pofile[i];

		if (i % 8 == 0)
			printf("\t");
		printf("%9.1x",fflg);
		if (i % 8 == 7)
			printf("\n");
	}
	if (NOFILE % 8 != 0) printf("\n");
	printf("maxfile  \t%d\n", U.u_omax);
	printf("cdir rdir\t%9.1x %9.1x\n", U.u_cdir, U.u_rdir);
	printf("ttyd     \t%d,%d\n", major(U.u_ttyd), minor(U.u_ttyd));
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
	printf("tracedev \t%d\n", U.u_tracedev);
	printf("start    \t%ds %dus\n", U.u_start.tv_sec,U.u_start.tv_usec);
	printf("acflag   \t%d\n", U.u_acflag);
	printf("limits   \t");
	for(i=0;i<RLIM_NLIMITS;i++) {
		printf("%d ",U.u_rlimit[i]);
	}
	printf("\n");
	printf("quota    \t%9.1x\n",U.u_quota);
	printf("quotaflag\t%9.1x\n",U.u_qflags);

	printf("smem     \t%9.1x %9.1x %9.1x\n",
		U.u_smsize,U.u_osmsize,U.u_lock);
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
	printf("\tdent ino %d name %.14s\n",
		nd->ni_dent.d_ino,nd->ni_dent.d_name);
	printf("\tsegflg\t%8d\n", nd->ni_segflg);
	printf("u_stack  \t%9.1x\n",U.u_stack[0]);
/*
	i =  U.u_stack - &U;
	while (U[++i] == 0);
	i &= ~07;
	while (i < 512) {
		printf("%x ", 0140000+2*i);
		for (j=0; j<8; j++)
			printf("%9x", U[i++]);
		printf("\n");
	}
*/
}

oatoi(s)
register char *s;
{
	register int v;

	v = 0;
	while (*s)
		v = (v<<3) + *s++ - '0';
	return(v);
}

dofile()
{
	register int nfile;
	struct file *xfile;
	register struct file *afile;
	register struct file *fp;
	register nf;
	register int loc;
	static char *dtypes[] = { "???", "gnode", "socket" };

	nf = 0;
	nfile = getw(nl[SNFILE].n_value);
	if((xfile = (struct file *)calloc(nfile, sizeof (struct file))) ==
	(struct file *) NULL) {
		perror("calloc: dofile");
		exit(1);
	}
	klseek(fc, (int)(afile = (struct file *)getw(nl[SFIL].n_value)), 0);
	read(fc, xfile, nfile * sizeof (struct file));
	for (fp=xfile; fp < &xfile[nfile]; fp++)
		if (fp->f_count)
			nf++;
	if (totflg) {
		printf("%4d/%4d\tfiles\n", nf, nfile);
		return;
	}
	printf("%d/%d open files\n", nf, nfile);
	printf("   LOC   TYPE    FLG      CNT  MSG    DATA    OFFSET\n");
	for (fp=xfile,loc=(int)afile; fp < &xfile[nfile]; fp++,loc+=sizeof(xfile[0])) {
		if (fp->f_count==0)
			continue;
		printf("%8x ", loc);
		if (fp->f_type <= DTYPE_SOCKET)
			printf("%-8.8s", dtypes[fp->f_type]);
		else
			printf("8d", fp->f_type);
		putf(fp->f_flag&FREAD, 'R');
		putf(fp->f_flag&FWRITE, 'W');
		putf(fp->f_flag&FAPPEND, 'A');
		putf(fp->f_flag&FSHLOCK, 'S');
		putf(fp->f_flag&FEXLOCK, 'X');
		putf(fp->f_flag&FASYNC, 'I');
		putf(fp->f_flag&FBLKINUSE, 'B');
		printf("  %3d", mask(fp->f_count));
		printf("  %3d", mask(fp->f_msgcount));
		printf("  %8.1x", fp->f_data);
		if (fp->f_offset < 0)
			printf("  %x\n", fp->f_offset);
		else
			printf("  %ld\n", fp->f_offset);
	}
}

int swapfrag,nswdev;

doswap()
{
	struct map *swapmap;
	struct swapu_t swapu;   /* ptr to swap useage structure */
	register int nswapmap;
	struct swdevt *swdevt, *sw;
	int nswap, free, j, avas;
	register struct mapent *me;

	int sys_clsize = getpagesize() / 512; /* 512 is clicks, not pages */

	avas = btodb(ctob(getw(nl[AVAILVAS].n_value)));

	nswapmap = getw(nl[SNSWAPMAP].n_value);
	if((swapmap = (struct map *)calloc(nswapmap, sizeof (struct map))) ==
	(struct map *) NULL) {
		perror("calloc: doswap swapmap");
		exit(1);
	}
	nswdev = getw(nl[SNSWDEV].n_value);
	if((swdevt = (struct swdevt *)calloc(nswdev, sizeof (struct swdevt))) ==
	(struct swdevt *) NULL) {
		perror("calloc: doswap swdevt");
		exit(1);
	}
	klseek(fc, nl[SSWAPU].n_value, L_SET);
	read(fc, &swapu, sizeof (struct swapu_t));

	klseek(fc, nl[SSWDEVT].n_value, L_SET);
	read(fc, swdevt, nswdev * sizeof (struct swdevt));

	klseek(fc, getw(nl[SWAPMAP].n_value), 0);
	read(fc, swapmap, nswapmap * sizeof (struct map));
	swapmap->m_name = "swap";
	swapmap->m_limit = (struct mapent *)&swapmap[nswapmap];
	swapfrag = getw(nl[SSWAPFRAG].n_value);
	nswap = 0;
	for (sw = swdevt; sw < &swdevt[nswdev]; sw++)
		nswap += sw->sw_nblks;
	/*
	 * in swapmap blk addr 0 is unused and also another
	 * CLSIZE is thrown away --
	 */
	nswap = nswap -ctod(CLSIZE) - ctod(CLSIZE);
	free = 0;
	for (me = (struct mapent *)(swapmap+1);
	    me < (struct mapent *)&swapmap[nswapmap]; me++)
		free += me->m_size;

#ifdef vax
#define CLRND(x) (((x) + (sys_clsize-1)) & ~(sys_clsize-1))
#endif vax
#ifdef mips
#define CLRND(x) (x)
#endif mips

	if (totflg) {
#define	btok(x)	((x) / (1024 / DEV_BSIZE))
		printf("%4d/%4d\t00k swap\n",
		    btok(swapu.total_used/100), 
		    btok((swapu.total_used+free)/100));
		return;
	}
	printf("%dk swap configured\n%dk reserved virtual address space\n\t%dk used (%dk text, %dk smem)\n",
	       	btok(nswap), btok(nswap - avas), 
		btok(swapu.total_used), btok(swapu.txt), 
		btok(swapu.smem));
	printf("\t%dk free, %dk wasted, %dk missing\n",
		btok(free), btok(swapu.wasted), 
		btok(nswap - (swapu.total_used + free)));

	printf("avail: ");
	j = 0;
	while (rmalloc(swapmap, swapfrag) != 0)
		j++;
	if (j) printf("%d*%dk ", j, btok(swapfrag));
	free = 0;
	for (me = (struct mapent *)(swapmap+1);
	    me < (struct mapent *)&swapmap[nswapmap]; me++)
		free += me->m_size;
	printf("%d*1k\n", btok(free));
}

/*
 * Allocate 'size' units from the given
 * map. Return the base of the allocated space.
 * In a map, the addresses are increasing and the
 * list is terminated by a 0 size.
 *
 * Algorithm is first-fit.
 *
 * This routine knows about the interleaving of the swapmap
 * and handles that.
 */
long
rmalloc(mp, size)
	register struct map *mp;
	register long size;
{
	register struct mapent *ep = (struct mapent *)(mp+1);
	register int addr;
	register struct mapent *bp;
	swblk_t first, rest;

	if (size <= 0 || size > swapfrag)
		return (0);
	/*
	 * Search for a piece of the resource map which has enough
	 * free space to accomodate the request.
	 */
	for (bp = ep; bp->m_size; bp++) {
		if (bp->m_size >= size) {
			/*
			 * If allocating from swapmap,
			 * then have to respect interleaving
			 * boundaries.
			 */
			if (nswdev > 1 &&
			    (first = swapfrag - bp->m_addr%swapfrag) < bp->m_size) {
				if (bp->m_size - first < size)
					continue;
				addr = bp->m_addr + first;
				rest = bp->m_size - first - size;
				bp->m_size = first;
				if (rest)
					rmfree(mp, rest, addr+size);
				return (addr);
			}
			/*
			 * Allocate from the map.
			 * If there is no space left of the piece
			 * we allocated from, move the rest of
			 * the pieces to the left.
			 */
			addr = bp->m_addr;
			bp->m_addr += size;
			if ((bp->m_size -= size) == 0) {
				do {
					bp++;
					(bp-1)->m_addr = bp->m_addr;
				} while ((bp-1)->m_size = bp->m_size);
			}
			if (addr % CLSIZE)
				return (0);
			return (addr);
		}
	}
	return (0);
}

/*
 * Free the previously allocated space at addr
 * of size units into the specified map.
 * Sort addr into map and combine on
 * one or both ends if possible.
 */
rmfree(mp, size, addr)
	struct map *mp;
	long size, addr;
{
	register struct mapent *firstbp;
	register struct mapent *bp;
	register int t;

	/*
	 * Both address and size must be
	 * positive, or the protocol has broken down.
	 */
	if (addr <= 0 || size <= 0)
		goto badrmfree;
	/*
	 * Locate the piece of the map which starts after the
	 * returned space (or the end of the map).
	 */
	firstbp = bp = (struct mapent *)(mp + 1);
	for (; bp->m_addr <= addr && bp->m_size != 0; bp++)
		continue;
	/*
	 * If the piece on the left abuts us,
	 * then we should combine with it.
	 */
	if (bp > firstbp && (bp-1)->m_addr+(bp-1)->m_size >= addr) {
		/*
		 * Check no overlap (internal error).
		 */
		if ((bp-1)->m_addr+(bp-1)->m_size > addr)
			goto badrmfree;
		/*
		 * Add into piece on the left by increasing its size.
		 */
		(bp-1)->m_size += size;
		/*
		 * If the combined piece abuts the piece on
		 * the right now, compress it in also,
		 * by shifting the remaining pieces of the map over.
		 */
		if (bp->m_addr && addr+size >= bp->m_addr) {
			if (addr+size > bp->m_addr)
				goto badrmfree;
			(bp-1)->m_size += bp->m_size;
			while (bp->m_size) {
				bp++;
				(bp-1)->m_addr = bp->m_addr;
				(bp-1)->m_size = bp->m_size;
			}
		}
		goto done;
	}
	/*
	 * Don't abut on the left, check for abutting on
	 * the right.
	 */
	if (addr+size >= bp->m_addr && bp->m_size) {
		if (addr+size > bp->m_addr)
			goto badrmfree;
		bp->m_addr -= size;
		bp->m_size += size;
		goto done;
	}
	/*
	 * Don't abut at all.  Make a new entry
	 * and check for map overflow.
	 */
	do {
		t = bp->m_addr;
		bp->m_addr = addr;
		addr = t;
		t = bp->m_size;
		bp->m_size = size;
		bp++;
	} while (size = t);
	/*
	 * Segment at bp is to be the delimiter;
	 * If there is not room for it 
	 * then the table is too full
	 * and we must discard something.
	 */
	if (bp+1 > mp->m_limit) {
		/*
		 * Back bp up to last available segment.
		 * which contains a segment already and must
		 * be made into the delimiter.
		 * Discard second to last entry,
		 * since it is presumably smaller than the last
		 * and move the last entry back one.
		 */
		bp--;
		printf("%s: rmap ovflo, lost [%d,%d)\n", mp->m_name,
		    (bp-1)->m_addr, (bp-1)->m_addr+(bp-1)->m_size);
		bp[-1] = bp[0];
		bp[0].m_size = bp[0].m_addr = 0;
	}
done:
	return;
badrmfree:
	printf("bad rmfree\n");
}

/*
 *
 * klseek taken from source for ps with slight modifications.
 *
 */

klseek(fd, loc, off)
	int fd;
	unsigned long loc;
	int off;
{
#ifdef vax
	if( kflg && Sysmap == 0)
		{/* initialize Sysmap */

		sizeSysmap = nl[SSYSSIZE].n_value * sizeof( struct pte);
		if((Sysmap = (struct pte *)calloc( sizeSysmap, 1)) == 
		(struct pte *) NULL) {
			perror("calloc: klseek");
			exit(1);
		}
		lseek( fc, clear( nl[SSYSMAP].n_value), 0);
		if( read( fc, Sysmap, sizeSysmap) != sizeSysmap)
			{
			printf( "Can't read system page table from %s\n",
				fcore);
			exit(1);
			}
		}
	if( kflg && (loc&0x80000000))
		{/* do mapping for kernel virtual addresses */
		struct pte *ptep;

		loc &= 0x7fffffff;
		ptep = &Sysmap[btop(loc)];
		if( (char *)ptep - (char *)Sysmap > sizeSysmap)
			{
			printf( "no system pte for %s\n", loc);
			exit(1);
			}
		if( ptep->pg_v == 0)
			{
			printf( "system pte invalid for %x\n", loc);
			exit(1);
			}
		loc = (off_t)((loc&PGOFSET) + ptob(ptep->pg_pfnum));
		}
	(void) lseek(fd, (long)loc, off);
#endif vax
#ifdef mips
	/* mkphys handles mapping on mips*/
	if (kflg && (loc & 0x80000000)) loc = mkphys(loc);
	(void) lseek(fd, loc, off);
#endif mips
}
#ifdef mips
/*
 * "addr"  is a kern virt addr and does not correspond
 * To a phys addr after zipping out the high bit..
 * since it was valloc'd in the kernel.
 *
 * We return the phys addr by simulating kernel vm (/dev/kmem)
 * when we are reading a crash dump.
 */
#include <machine/cpu.h>
off_t
mkphys(addr)
	unsigned long addr;
{
	register off_t o;
	unsigned long addr2=addr;

	if (IS_KSEG0(addr)) {
		return(K0_TO_PHYS(addr));
	}
	if (IS_KSEG1(addr)) {
		return(K1_TO_PHYS(addr));
	}
	o = addr & PGOFSET;
	addr = 4 * btop(addr2 - K2BASE);
	addr = getsys((unsigned)nl[SSYSMAP].n_value + addr);
	addr = (addr & PG_PFNUM) << (PGSHIFT - PTE_PFNSHIFT) | o;
	return(addr);
}


getsys(loc)
	register unsigned loc;
{
	register unsigned seekloc = 0;
	int word;

	if (IS_KSEG0(loc)) {
		seekloc = K0_TO_PHYS(loc);
	}
	if (IS_KSEG1(loc)) {
		seekloc = K1_TO_PHYS(loc);
	}
	lseek(fc, seekloc, 0);
	read(fc, &word, sizeof(word));
	return(word);
}
#endif mips

/*
 * smp_stat
 *
 * Compute statistics on smp lock struct.
 * Calculate the average number of spins needed to acquire the lock and
 * the percentage of locks that were failed to be acquired.
 *
 * "line" is used to represent the tty line for example in a case where
 * there are many stats to be printed out under one title line.
 * setting "line" to -1 will prevent the use of this parameter.
 */
smp_stat(line,spin,lost,won)
	int line;
	unsigned int spin, lost,won;
{
	float avg,pct;

	if (smp_title) {
		if (line >= 0) 
			printf("LINE     WON       LOST       SPUN   AVGSPIN   %%LOST   NORM\n");
		else
			printf("     WON       LOST       SPUN    AVGSPIN   %%LOST   NORM\n");
		smp_title = 0;
	}
	if ((won != 0) || (lost != 0)) {
		if (lost != 0)
			avg = (float)spin/(float)lost;
		else	avg = 0;
		if (won != 0)
			pct = 100*((float)lost/(float)won);
		else	pct = 0;
		if (line >= 0)
			printf("%4d",line);
		printf("%8d   %8d   %8d   %6.3f   %6.3f   %6.3f\n",
			won, lost, spin, avg, pct, avg*pct);
	}
}

