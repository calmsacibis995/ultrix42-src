/*	@(#)vmsystm.h	4.2  (ULTRIX)        9/4/90     */

/*
 * Miscellaneous virtual memory subsystem variables and structures.
 */

#ifdef KERNEL
#include "../h/ansi_compat.h"
#else
#include <ansi_compat.h>
#endif

#ifdef KERNEL
int	freemem;		/* remaining blocks of free memory */
int	avefree;		/* moving average of remaining free blocks */
int	avefree30;		/* 30 sec (avefree is 5 sec) moving average */
int	deficit;		/* estimate of needs of new swapped in procs */
int	nscan;			/* number of scans in last second */
#ifndef __mips
int	multprog;		/* current multiprogramming degree */
#else /* __mips */
extern	int multprog;		/* current multiprogramming degree */
#endif /* __mips */
int	desscan;		/* desired pages scanned per second */

/* writable copies of tunables */
#ifndef __mips
int	maxpgio;		/* max paging i/o per sec before start swaps */
int	maxslp;		/* max sleep time before very swappable */
int	lotsfree;		/* max free before clock freezes */
int	minfree;		/* minimum free pages before swapping begins */
int	desfree;		/* no of pages to try to keep free via daemon */
int	saferss;		/* no pages not to steal; decays with slptime */
#else /* mips */
extern	int maxpgio;		/* max paging i/o per sec before start swaps */
extern	int maxslp;		/* max sleep time before very swappable */
extern	int lotsfree;		/* max free before clock freezes */
extern	int minfree;		/* minimum free pages before swapping begins */
extern	int desfree;		/* no of pages to try to keep free via daemon */
extern	int saferss;		/* no pages not to steal; decays with slptime */
extern int slowscan;		/* slowest scan rate, clusters/second */
extern int fastscan;		/* fastest scan rate, clusters/second */
#endif /* __mips */
#endif /* KERNEL */

/*
 * Fork/vfork accounting.
 */
struct	forkstat
{
	int	cntfork;
	int	cntvfork;
	int	sizfork;
	int	sizvfork;
};
#ifdef KERNEL
struct	forkstat forkstat;
#endif /* KERNEL */

/*
 * Swap kind accounting.
 */
struct	swptstat
{
#ifdef __mips
	int	dpteasy;	/* easy data pt swaps */
	int	spteasy;	/* easy stack pt swaps */
	int	dptexpand;	/* data pt expansion swaps */
	int	sptexpand;	/* stack pt expansion swaps */
	int	dptshrink;	/* data pt shrinking swaps */
	int	sptshrink;	/* stack pt shrinking swaps */
#endif /* __mips */
#ifdef __vax
	int	pteasy;		/* easy pt swaps */
	int	ptexpand;	/* pt expansion swaps */
	int	ptshrink;	/* pt shrinking swaps */
	int	ptpack;		/* pt swaps involving spte copying */
#endif /* __vax */
};
#ifdef KERNEL
struct	swptstat swptstat;
#endif /* KERNEL */
