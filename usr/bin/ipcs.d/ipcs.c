#ifndef lint
static	char	*sccsid = "@(#)ipcs.c	4.2	(ULTRIX)	10/8/90";
#endif lint

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
/*
 *
 *   Modification history:
 *
 *   8 Dec 89 -- jmartin
 *	Fix "-k" option computation of physical address for new MIPS PTE
 *	format.
 *
 *  08 Jan 88 -- depp
 *      Fixed [-C corefile] [-N namelist] (System V like)options and 
 *      added [-k namelist corefile] (BSD like) option
 *      cleaned up commented out code.
 *
 *  30 Apr 85 -- depp
 *	New file to perform system maintenance on System V IPC
 *
 */

/*
**	ipcs - IPC status
**	Examine and print certain things about message queues, semaphores,
**		and shared memory.
*/

#include	<machine/pte.h>

#include	<sys/param.h>
#include	<sys/vm.h>
#define KERNEL
#include	<sys/ipc.h>
#include	<sys/msg.h>
#include	<sys/sem.h>
#undef KERNEL
#include	<sys/shm.h>
#include	<nlist.h>
#include	<fcntl.h>
#include	<sys/time.h>
#include	<grp.h>
#include	<pwd.h>
#include	<stdio.h>
#include        <sys/vmmac.h>

#define	TIME	0
#define	MSG	1
#define	SEM	2
#define	SHM	3
#define	MSGINFO	4
#define	SEMINFO	5
#define	SHMINFO	6
#define SSYSMAP	7
#define SSYSSIZE 8

struct nlist    nl[] =
{			/* name list entries for IPC facilities */
    { "_time" },{ "_msgque" },{ "_sema" },{ "_smem" },{ "_msginfo" },
    { "_seminfo" }, { "_sminfo" }, 
    { "_Sysmap" },
    { "_Syssize" },
    { NULL },
};


#define NAMELIST "/vmunix"
#define COREFILE "/dev/kmem"

char    chdr[] = "T     ID     KEY        MODE       OWNER    GROUP",

 /* common header format */
        chdr2[] = "  CREATOR   CGROUP",

       *name = NAMELIST,
       *mem = COREFILE,

        /* c option header format */
        opts[] = "abchmopqstC:N:k:";/* allowable options for getopt */
extern char *optarg;		/* arg pointer for getopt */

int     bflg,		/* biggest size: segsz on m; qbytes on q; nsems on s */
        cflg,		/* creator's login and group names */
        mflg,		/* shared memory status */
        oflg,		/* outstanding data: nattch on m; cbytes, qnum on q */
        pflg,		/* process id's: lrpid, lspid on q; cpid, lpid on m */
        qflg,		/* message queue status */
        sflg,		/* semaphore status */
        tflg,		/* times: atime, ctime, dtime on m; ctime,
				   rtime, stime on q; ctime, otime on s */
	nflg,		/* indicator that a particular IPC capability is
				   currently not being used */
        err;		/* option error count */

extern int  optind;		/* option index for getopt */

extern char *ctime ();
extern struct group *getgrgid ();
extern struct passwd   *getpwuid ();
extern long lseek ();

char	*kmemf, *memf, *swapf, *nlistf;
int	kmem, kflg, swap = -1;

#define clear(x) ((int)(x) & 0x7fffffff)

main (argc, argv)
int     argc;			/* arg count */
char  **argv;			/* arg vector */
{
    register int    i,		/* loop control */
                    md,		/* memory file file descriptor */
                    o;		/* option flag */
    time_t time;		/* date in memory file */
    struct shmid_ds mds;	/* shared memory data structure */
    struct shminfo  shminfo;	/* shared memory information structure */
    struct msqid_ds qds;	/* message queue data structure */
    struct msginfo  msginfo;	/* message information structure */
    struct semid_ds sds;	/* semaphore data structure */
    struct seminfo  seminfo;	/* semaphore information structure */

    /* Go through the options and set flags. */
    while ((o = getopt (argc, argv, opts)) != EOF)
	switch (o)
	{
	    case 'a': 
		bflg = cflg = oflg = pflg = tflg = 1;
		break;
	    case 'b': 
		bflg = 1;
		break;
	    case 'c': 
		cflg = 1;
		break;
	    case 'C': 
		mem = optarg;
		kflg = 1;
		break;
	    case 'h':
		err++;
		break;
	    case 'k':
		name = optarg;
		if((mem = argv[optind++]) == (char *)0 || (mem[0] == '-')) {
		    printf("%s: -k option requires both namelist and corefile\n"
			   , argv[0]);
		    err++;
		}
		kflg = 1;
		break;
	    case 'm': 
		mflg = 1;
		break;
	    case 'N': 
		name = optarg;
		break;
	    case 'o': 
		oflg = 1;
		break;
	    case 'p': 
		pflg = 1;
		break;
	    case 'q': 
		qflg = 1;
		break;
	    case 's': 
		sflg = 1;
		break;
	    case 't': 
		tflg = 1;
		break;
	    case '?': 
		err++;
		break;
	}
    if (err || (optind < argc))
    {
	fprintf (stderr,
		"usage:  ipcs [-abcmopqst] [-k namelist corefile] [-C corefile] [-N namelist]\n");
	exit (1);
    }
    if ((mflg + qflg + sflg) == 0)
	mflg = qflg = sflg = 1;

    /* Check out namelist and open memory/core file. */
    nlist (name, nl);
    openfiles();

    /* retrieve time */
    if (!nl[TIME].n_value)
    {
	fprintf (stderr, "ipcs:  no namelist\n");
	exit (1);
    }
    klseek (kmem, (long) nl[TIME].n_value, 0);

    reade (kmem, &time, sizeof (time));

    printf ("\nIPC status from %s as of %s", mem, ctime (&time));

    /* Print Message Queue status report. */
    if (qflg)
    {
	if (nl[MSG].n_value)
	{
	    i = 0;
	    klseek (kmem, (long) nl[MSGINFO].n_value, 0);
	    reade (kmem, &msginfo, sizeof (msginfo));
	    klseek (kmem, (long) nl[MSG].n_value, 0);
	    printf ("Message Queues:\n%s%s%s%s%s%s\n", chdr,
		    cflg ? chdr2 : "",
		    oflg ? " CBYTES  QNUM" : "",
		    bflg ? " QBYTES" : "",
		    pflg ? " LSPID LRPID" : "",
		    tflg ? "   STIME    RTIME    CTIME " : "");
	} else
	{
	    i = msginfo.msgmni;
	    printf ("Message Queue facility not in system.\n\n");
	}
	nflg = 1;
	while (i < msginfo.msgmni)
	{
	    reade (kmem, &qds, sizeof (qds));
	    if (!(qds.msg_perm.mode & IPC_ALLOC))
	    {
		i++;
		continue;
	    }
	    hp ('q', "SRrw-rw-rw-", &qds.msg_perm, i++, msginfo.msgmni);
	    if (oflg)
		printf ("%7u%6u", qds.msg_cbytes, qds.msg_qnum);
	    if (bflg)
		printf ("%7u", qds.msg_qbytes);
	    if (pflg)
		printf ("%6u%6u", qds.msg_lspid, qds.msg_lrpid);
	    if (tflg)
	    {
		tp (qds.msg_stime);
		tp (qds.msg_rtime);
		tp (qds.msg_ctime);
	    }
	    printf ("\n");
	    nflg = 0;
	}
	if (nflg)
		printf("*** No message queues are currently defined ***\n\n");
	else
		printf("\n");
    }

    /* Print Shared Memory status report. */
    if (mflg)
    {
	if (nl[SHM].n_value)
	{
	    i = 0;
	    klseek (kmem, (long) nl[SHMINFO].n_value, 0);
	    reade (kmem, &shminfo, sizeof (shminfo));
	    klseek (kmem, (long) nl[SHM].n_value, 0);
	    printf ("Shared Memory\n%s%s%s%s%s%s\n", chdr,
		cflg ? chdr2 : "",
		oflg ? " NATTCH" : "",
		bflg ? "  SEGSZ" : "",
		pflg ? "  CPID  LPID" : "",
		tflg ? "   ATIME    DTIME    CTIME " : "");
	}
	else
	{
	    i = shminfo.shmmni;
	    printf ("Shared Memory facility not in system.\n\n");
	}
	nflg = 1;
	while (i < shminfo.shmmni)
	{
	    reade (kmem, &mds, sizeof (mds));
	    if (!(mds.shm_perm.mode & IPC_ALLOC))
	    {
		i++;
		continue;
	    }
	    hp ('m', "DCrw-rw-rw-", &mds.shm_perm, i++, shminfo.shmmni);
	    if (oflg)
		printf ("%7u", mds.shm_nattch);
	    if (bflg)
		printf ("%7d", mds.shm_segsz);
	    if (pflg)
		printf ("%6u%6u", mds.shm_cpid, mds.shm_lpid);
	    if (tflg)
	    {
		tp (mds.shm_atime);
		tp (mds.shm_dtime);
		tp (mds.shm_ctime);
	    }
	    printf ("\n");
	    nflg = 0;
	}
	if (nflg)
		printf("*** No shared memory segments are currently defined ***\n\n");
	else
		printf("\n");
    }

    /* Print Semaphore facility status. */
    if (sflg)
    {
	if (nl[SEM].n_value)
	{
	    i = 0;
	    klseek (kmem, (long) nl[SEMINFO].n_value, 0);
	    reade (kmem, &seminfo, sizeof (seminfo));
	    klseek (kmem, (long) nl[SEM].n_value, 0);
	    printf ("Semaphores\n%s%s%s%s\n", chdr,
		cflg ? chdr2 : "",
		bflg ? " NSEMS" : "",
		tflg ? "   OTIME    CTIME " : "");
	}
	else
	{
	    i = seminfo.semmni;
	    printf ("Semaphore facility not in system.\n\n");
	}
	nflg = 1;
	while (i < seminfo.semmni)
	{
	    reade (kmem, &sds, sizeof (sds));
	    if (!(sds.sem_perm.mode & IPC_ALLOC))
	    {
		i++;
		continue;
	    }
	    hp ('s', "--ra-ra-ra-", &sds.sem_perm, i++, seminfo.semmni);
	    if (bflg)
		printf ("%6u", sds.sem_nsems);
	    if (tflg)
	    {
		tp (sds.sem_otime);
		tp (sds.sem_ctime);
	    }
	    printf ("\n");
	    nflg = 0;
	}
	if (nflg)
		printf("*** No semaphores are currently defined ***\n\n");
	else
		printf("\n");
    }
    exit (0);
}

struct pte *Sysmap = 0;

klseek(fd, loc, off)
	int fd;
	long loc;
	int off;
{
	static int	sizeSysmap;
	int a;

#ifdef vax
	/* initialize Sysmap */
	if( kflg && Sysmap == 0) {
		sizeSysmap = nl[SSYSSIZE].n_value * sizeof( struct pte);
		Sysmap = (struct pte *)calloc( sizeSysmap, 1);
		if ((lseek( kmem, (long)clear( nl[SSYSMAP].n_value), 0)) < 0) {
		    perror("ipcs: can't seek system page table");
		    exit(1);
		}

		if( (a = read( kmem, Sysmap, sizeSysmap)) != sizeSysmap) {
		    cantread( "system page table", kmemf);
		    exit(1);
		}
	}

	/* do mapping for system VA space */
	if( kflg && (loc&0x80000000)) {
		struct pte *ptep;

		loc &= 0x7fffffff;
		ptep = &Sysmap[btop(loc)];
		if( (char *)ptep - (char *)Sysmap > sizeSysmap)
			{
			printf( "no system pte for %s\n", loc);
			exit(1);
			}
		if( ptep->pg_v == 0) {
			printf( "system pte invalid for %x\n", loc);
			exit(1);
		}
		loc = (off_t)((loc&PGOFSET) + ptob(ptep->pg_pfnum));
	}
	(void) lseek(fd, (long)loc, off);
#endif vax
#ifdef mips
	/* mkphys handles mapping on mips*/
	(void) lseek(fd, mkphys(loc), off);
#endif mips
}

openfiles()
{

	kmemf = mem;
	kmem = open(kmemf, 0);
	if (kmem < 0) {
		perror(kmemf);
		exit(1);
	}
}

/*
**	reade - read with error exit
*/

reade (f, b, s)
int     f;			/* fd */
char    *b;			/* buffer address */
int     s;			/* size */
{
    if (read (f, b, s) != s)
    {
	perror ("ipcs:  read error");
	printf ("%x\n", b);
	exit (1);
    }
}

/*
**	hp - common header print
*/

hp (type, modesp, permp, slot, slots)
char    type,			/* facility type */
       *modesp;			/* ptr to mode replacement characters */
register struct ipc_perm   *permp;/* ptr to permission structure */
int     slot,			/* facility slot number */
        slots;			/* # of facility slots */
{
    register int    i,		/* loop control */
                    j;		/* loop control */
    register struct group  *g;	/* ptr to group group entry */
    register struct passwd *u;	/* ptr to user passwd entry */

    printf ("%c%7d %10d ", type, slot + slots * permp -> seq, permp -> key);
    for (i = 02000; i; modesp++, i >>= 1)
	printf ("%c", (permp -> mode & i) ? *modesp : '-');
    if ((u = getpwuid (permp -> uid)) == NULL)
	printf ("%9d", permp -> uid);
    else
	printf ("%9.8s", u -> pw_name);
    if ((g = getgrgid (permp -> gid)) == NULL)
	printf ("%9d", permp -> gid);
    else
	printf ("%9.8s", g -> gr_name);
    if (cflg)
    {
	if ((u = getpwuid (permp -> cuid)) == NULL)
	    printf ("%9d", permp -> cuid);
	else
	    printf ("%9.8s", u -> pw_name);
	if ((g = getgrgid (permp -> cgid)) == NULL)
	    printf ("%9d", permp -> cgid);
	else
	    printf ("%9.8s", g -> gr_name);
    }
}

/*
**	tp - time entry printer
*/

tp (time)
time_t time;			/* time to be displayed */
{
    register struct tm *t;	/* ptr to converted time */

    if (time)
    {
	t = localtime (&time);
	printf (" %2d:%2.2d:%2.2d", t -> tm_hour, t -> tm_min, t -> tm_sec);
    }
    else
	printf (" no-entry");
}

cantread(what, fromwhat)
	char *what, *fromwhat;
{

	fprintf(stderr,
		"ipcs: error reading %s from %s\n", what, fromwhat);
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
	unsigned addr;
{
	register off_t o;
	unsigned addr2=addr;

	if (!kflg)
		return(addr);
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
	int word = 0;

	if (IS_KSEG0(loc)) {
		seekloc = K0_TO_PHYS(loc);
	}
	if (IS_KSEG1(loc)) {
		seekloc = K1_TO_PHYS(loc);
	}
	lseek(kmem, seekloc, 0);
	read(kmem, &word, sizeof (word));
	return (word);
}
#endif mips
