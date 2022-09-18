/*	@(#)kmalloc.h	4.8	(ULTRIX)	2/28/91	*/

/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	malloc.h	7.3 (Berkeley) 7/15/87
 *
 * History:
 *
 * 9 Sep 90	Sinkewicz
 *		Added new type of memory - KM_WAN for x.25.
 *
 * 4 Sep 90	dlh
 *		new type of memory which can be allocated:  KM_VECTOR -
 *		memory for structures which deal which vector processor 
 *		support
 *
 * 31 Aug 90	paradis
 *		Added KM_SPU
 *
 * 30 Dec 89	bp
 *		Add state in flags specifically for internal usage 
 *		for pageout and sched.
 *
 * 10-Dec-89 -- Matt Thomas
 *		Add new type KM_LAT (actually change KM_FREE7 to KM_LAT).
 *
 * 25 Jul 89 -- chet
 *	Add new type KM_BUFCACHE
 *
 * 15-Jun-89	bp
 *		Removed uncessary smp_lock in KM_REALLOC and
 *		fixed merge problem.
 *
 * 12-Jun-89	bp
 *		Added changes for enhanced kernel memory alloctor
 *
 *  12-Jun-89		gopal
 *		Added a new macro KM_REALLOC for dynamic swap. Also
 *		added a new memory block type KM_DMAP.
 *
 *  4-May-89		Giles Atkinson
 *		Two new memory block types
 *
 *  3-Feb-89		jmartin
 *		PMAX/3.0 merge
 *
 * 15 Dec 88		jmartin
 *		SMP locking for shared memory data structures (cf.
 *		h/shm.h) Integration of v3.0 shared memory changes,
 *		i.e. KM_ALLOC of per-process shared memory
 *		structures.
 *
 * 31-Aug-88		jmartin
 *		Call sleep_check() from KM_ALLOC macro it KM_NOWAIT
 *		is not set.
 *
 *  9-Jun-88		Joe Amato
 *		Moved validation of address being freed to top of 
 *		KM_FREE macro.
 *
 * 27-Apr-88		Joe Amato
 *		Added IS_KMEM_VA() macro.
 *		Bumped size of elements of kmemusage structure
 *		Changed GUARDPAGES from 2 to CLSIZE
 *
 * 15-Jan-88		Larry Palmer
 *		Add defines for malloced network.
 *
 * 17-Dec-1987		Todd M.Katz
 *		Place braces( {} ) around the KMEMSTATS versions of KM_FREE
 *		and KM_MEMDUP.
 */

#ifndef _KMALLOC_
#define _KMALLOC_

#ifdef KERNEL
#include "../h/ansi_compat.h"
#include "../h/smp_lock.h"
#else
#include <ansi_compat.h>
#endif


/*
 * memfree "flg" parameter options
 */
#define KMF_NODETACH	0		/* PFs are reclaimable */
#define KMF_DETACH	1		/* PFs are probably not reclaimable*/
#define KMF_UAREA	2		/* PFs are to be placed on "u" list*/

/*
 * malloc "flags" parameter options
 */
#define KM_NOARG	0x0000	/* could sleep */
#define KM_NOWAIT	0x0001	/* can't sleep */
#define KM_CLEAR	0x0002	/* zero memory */
#define KM_CONTIG	0x0004	/* physically contiguous */
#define KM_CALL		0x0008	/* only do calls */
#define	KM_NOCACHE	0x0010	/* MIPS flag to set n bit of pte(s) */
#define	KM_WIRED	0x0020	/* Wired map */
#define	KM_NWSYS	0x2000	/* can't wait */
#define	KM_SSYS		0x4000	/* sched or pageout */
#define	KM_INDEX	0x8000	/* Internal flag for index calculation */

#define	KM_NOW_CL	( KM_NOWAIT | KM_CLEAR )
#define	KM_NOW_CL_CA	( KM_NOWAIT | KM_CLEAR | KM_CALL )
#define	KM_NOW_CL_CO_CA	( KM_NOWAIT | KM_CLEAR | KM_CONTIG | KM_CALL )

/*
 * Types of memory to be allocated
 */
#define KM_FREEL	0	/* should be on free list */
#define KM_MBUF		1	/* mbuf */
#define KM_DEVBUF	2	/* device driver memory */
#define	KM_PCB		3	/* protocol control block */
#define	KM_ZOMBIE	4	/* zombie proc status */
#define KM_NAMEI	5	/* namei path name buffer */
#define KM_GPROF	6	/* kernel profiling buffer */
#define KM_TEMP		7	/* misc temporary data buffers */
#define KM_DECNET	8 	/* DECnet opts */
#define KM_MOUNT	9	/* Mount/fsdata pointer */
#define KM_NFS		10	/* NFS using this allocation */
#define KM_CRED		11	/* Credential allocation */
#define KM_SYSPROC	12	/* sysprocs */
#define KM_RPC		13	/* RPC */
#define KM_INTSTK	14	/* Interrupt Stack */
#define KM_RMAP		15	/* resource maps */
#define KM_SCA		16	/* sca: static (sysap/scs/ppd/pd) */
#define KM_SCABUF	17	/* sca: pd buffers (dg,msg,cmd packets) */
#define KM_CDRP		18	/* sca sysap: class driver request packet) */
#define KM_XOS		19	/* for xos, will go away */
#define KM_SOCKET	20	/* Sockets */
#define KM_ACCESS	21 	/* Decnet Access Rights	*/
#define KM_RTABLE	22	/* Route entries */
#define KM_HTABLE	23	/* IMP host table */
#define KM_FTABLE	24	/* Fragment reassembly queues */
#define KM_IFADDR	25	/* Interface addresses */
#define KM_SOOPTS	26	/* Socket options */
#define KM_SONAME	27	/* Socket names */
#define KM_CLUSTER	28	/* Large mbufs */
#define KM_RIGHTS	29	/* Rights */
#define KM_ATABLE	30	/* Arp tables */
#define KM_TXTSW	31	/* text swap block array */
#define KM_SHMSEG	32	/* per proc shem segs */
#define KM_LMF          33      /* LMF license records */
#define KM_EXIT_ACTN    34      /* Per-process exit_actn structs */
#define KM_DMAP		35	/* dynamic map for swap */
#define KM_BUFCACHE     36      /* Filesystem buffer cache allocation */
#define KM_LAT		37	/* LAT structures (softc's, slots, vc's,...) */
#define KM_PFILT	38	/* packetfilter descriptors */
#define KM_SPU		39	/* VAX 9000 console memory request */
#define KM_DEBUG	40	/* ptrace/procxmt communication */
#define KM_VECTOR	41	/* vector data stuctures */
#define KM_WAN          42	/* x.25 */
#define KM_NOFILE	43	/* Overflow buffer for open file descriptors */
#define KM_FREE3	44
#define KM_FREE4	45
#define KM_FREE5	46
#define KM_FREE6	47
#define KM_FREE7	48
#define KM_FREE8	49
#define KM_FREE9	50
#define KM_LAST		51	/* KEEP THIS LAST AS IT IS AN ARRAY SIZE */

/*
 * Constants for setting the parameters of the kernel memory allocator.
 *
 * 2 ** MINBUCKET is the smallest unit of memory that will be
 * allocated. It must be at least large enough to hold a pointer.
 *
 * Units of memory less or equal to MAXALLOCSAVE will permanently
 * allocate physical memory; requests for these size pieces of
 * memory are quite fast. Allocations greater than MAXALLOCSAVE must
 * always allocate and free physical memory; requests for these
 * size allocations should be done infrequently as they will be slow.
 * Constraints: CLBYTES <= MAXALLOCSAVE <= 2 ** (MINBUCKET + 14)
 * and MAXALLOCSIZE must be a power of two. MAXBUCKETSAVE must be < 19
 * Here are the buckets:	BUCKET	  SIZE
 *				 4	    16
 *				 5	    32
 *				 6	    64
 *				 7	   128
 *				 8	   256
 *				 9	   512
 *				10	  1024 (  1 KB)
 *				11	  2048 (  2 KB)
 *				12	  4096 (  4 KB)
 *				13	  8192 (  8 KB)
 *				14	 16384 ( 16 KB)
 *				15	 32768 ( 32 KB)
 *				16	 65536 ( 64 KB)
 *				17	131072 (128 KB)
 *				18	262144 (256 KB)
 *				19	ALL GREATER THAN 256 KB
 */
#define KM_REQSIZ	4096

#define MINBUCKET	4			/* 4 => min alloc of 16 bytes */
#define MAXBUCKETSAVE	15			/* 32 KB in bucket 15 */
#define MAXALLOCSAVE	(1<<MAXBUCKETSAVE)	/* 32 KB */
#define KMBUCKET	CLSHIFT			/* bucket of clusters */
#define NBUCKET         (MINBUCKET+16)
#define GUARDPAGES	(CLSIZE)		/*we zero CLSIZE trailing ptes*/
#define powerof2(x)     ((((x)-1)&(x))==0)	/* 2**x */
#define	KMNCBUCKET	(NBUCKET)		/* 
						 * Imaginary bucket for
						 * KM_NOCAHCE.
						 */

#if (defined(__vax) && !defined(LOCORE)) || (defined(__mips) && defined(__LANGUAGE_C))

struct kmemusage {
	union {
		struct {
					/* Active head */
			struct kmemelement *_ukmem_ku_hele;
					/* Active tail */
			struct kmemelement *_ukmem_ku_tele;
					/* Bucket Index */
			unsigned short  _ukmem_ku_index;
					/* Reference count */
			short _ukmem_ku_refcnt;
		} _ukmem_active;
		struct {		/* On dead kmemusage list */
					/* Processor TB purge */
			unsigned long _ukmem_ku_pmask;
					/* Have guard ptes */ 
			char _ukmem_ku_guard;
		} _ukmem_free;
	} _ukmem;
	struct kmemusage *ku_nku;	/* Next kmemusage */
	unsigned long   ku_pagecnt;	/* Free Count */
};

#define ku_hele		_ukmem._ukmem_active._ukmem_ku_hele
#define ku_tele		_ukmem._ukmem_active._ukmem_ku_tele
#define ku_index	_ukmem._ukmem_active._ukmem_ku_index
#define ku_refcnt	_ukmem._ukmem_active._ukmem_ku_refcnt
#define ku_pmask	_ukmem._ukmem_free._ukmem_ku_pmask
#define ku_guard	_ukmem._ukmem_free._ukmem_ku_guard

/*
 * Each memory element on a buckets free list
 * is cast to a kernel memory element
 */

struct kmemelement {
	struct kmemelement *ke_fl;
	struct kmemelement *ke_bl;
};

/*
 * Set of buckets for each size of memory block that is retained
 */

struct kmembuckets {
	struct kmemelement *kb_efl, *kb_ebl;	/* Dlist of kmemelement  */
	struct kmemusage *kb_kup;		/* Kmemusage structure */
	short   kb_total;			/* Total kmemusage allocated */
	short   kb_hwm;				/* High water mark */
};

/*
 * Argument passed to km_hwmscan to indicate what
 * degree of trimming of buckets is to be done.
 */

#define	KM_HWMSOFT	(0)			/* Trim to lotsfree */
#define	KM_HWMHARD	(1)			/* Trim all to high water */


#define MINALLOCSIZE	(1 << MINBUCKET)
#define BUCKETINDX(size) \
	(size) <= (MINALLOCSIZE * 128) \
		? (size) <= (MINALLOCSIZE * 8) \
			? (size) <= (MINALLOCSIZE * 2) \
				? (size) <= (MINALLOCSIZE * 1) \
					? (MINBUCKET + 0) \
					: (MINBUCKET + 1) \
				: (size) <= (MINALLOCSIZE * 4) \
					? (MINBUCKET + 2) \
					: (MINBUCKET + 3) \
			: (size) <= (MINALLOCSIZE* 32) \
				? (size) <= (MINALLOCSIZE * 16) \
					? (MINBUCKET + 4) \
					: (MINBUCKET + 5) \
				: (size) <= (MINALLOCSIZE * 64) \
					? (MINBUCKET + 6) \
					: (MINBUCKET + 7) \
		: (size) <= (MINALLOCSIZE * 2048) \
			? (size) <= (MINALLOCSIZE * 512) \
				? (size) <= (MINALLOCSIZE * 256) \
					? (MINBUCKET + 8) \
					: (MINBUCKET + 9) \
				: (size) <= (MINALLOCSIZE * 1024) \
					? (MINBUCKET + 10) \
					: (MINBUCKET + 11) \
			: (size) <= (MINALLOCSIZE * 8192) \
				? (size) <= (MINALLOCSIZE * 4096) \
					? (MINBUCKET + 12) \
					: (MINBUCKET + 13) \
				: (size) <= (MINALLOCSIZE * 16384) \
					? (MINBUCKET + 14) \
					: (MINBUCKET + 15)

/*
 * Turn virtual addresses into kmem map indicies
 */
#define kmemxtob(index)	(kmembase + (index) * NBPG)
#define btokmemx(addr)	((((char *)(addr)) - kmembase) / NBPG)
#define btokup(addr)	((struct kmemusage *)(&kmemusage[(((char *)(addr)) - kmembase) >> CLSHIFT]))
#define	kuptova(KUP) 	(kmembase + (((KUP) - kmemusage) << CLSHIFT)) 
#define	kuptokmemx(KUP)	(((KUP) - kmemusage) << CLSIZELOG2)
#define IS_KMEM_VA(va)  ((va) >= kmembase && (va) < kmemlimit)

/*
 * Queue Manipulation Macro
 */


#define	KM_RMELEMENTS(KUP) {			\
	if ((KUP)->ku_hele == (KUP)->ku_tele) {	\
		_remque((KUP)->ku_tele); \
	} \
	else { \
		register struct kmemelement *M_hele, *M_tele; \
		M_hele = (KUP)->ku_hele; \
		M_tele = (KUP)->ku_tele; \
		M_hele->ke_bl->ke_fl = M_tele->ke_fl; \
		M_tele->ke_fl->ke_bl = M_hele->ke_bl; \
	} \
}


/*
 * Macro versions for the usual cases of malloc/free
 */


#ifdef	__mips
#define KM_MEMDUP(space) {						\
	(void) km_memdup((space));					\
}
#endif /*	__mips */

#ifdef	__vax
#define	KM_MEMDUP(space) {						\
	(void) _km_memdup((space));					\
}
#endif /*	__vax */



#ifdef	__vax
#define KM_ALLOC(space, cast, size, type, flags) { 			\
	if ((flags) & (KM_CALL|KM_CONTIG|KM_NOCACHE) || 		\
		(size) > MAXALLOCSAVE) 					\
		(space) = (cast) km_alloc((size),(type),(flags));	\
	else 								\
		(space) = (cast) _km_alloc((type),(flags)|KM_INDEX,	\
				BUCKETINDX((size)));			\
}
#endif /*	__vax */

#ifdef	__mips
#define	KM_ALLOC(space, cast, size, type, flags) {			\
	if ((size) > MAXALLOCSAVE) 					\
		(space) = (cast) km_alloc((size),(type),(flags));	\
 	else (space) = 							\
	(cast) km_alloc(BUCKETINDX((size)),(type),(flags)|KM_INDEX);	\
}
#endif /*	__mips */



#define KM_REALLOC(space, cast, size, osize, type, flags ) { \
	register int oaddr, indx, oindx; \
	register struct kmemusage *M_kd; \
	register long M_sp = splimp(); \
	M_kd = btokup((space));\
	oindx = M_kd->ku_index;\
	oaddr = (int)(space); \
	indx = BUCKETINDX((size));\
	if(indx != oindx || \
	   (indx > MAXBUCKETSAVE && clrnd(rbtop((size))) != M_kd->ku_pagecnt)){\
		KM_ALLOC((space), cast, size, type, flags); \
		if((space) != NULL){\
			if((size) > (osize)) \
				blkcpy(oaddr, (int)(space), (osize)); \
			else \
				blkcpy(oaddr, (int)(space), (size)); \
			KM_FREE(oaddr, (type)); \
		} \
	} else {\
		if(size > osize) \
			blkclr(((caddr_t)(space)) + osize, (size - osize));\
	} \
	splx(M_sp);\
}


#ifdef	__mips
#define	KM_FREE(addr, type) {						\
	km_free((addr), (type));					\
}
#endif /*	__mips */

#ifdef	__vax
#define	KM_FREE(addr, type) {						\
	_km_free((type), (addr));					\
}
#endif /*	__vax */




/*
 * Debug flags for use with kmdebug and km_debug system call
 */

#define	KM_DFLAGS	0x00ffffff	/* Debug flags */
#define	KM_DDMZ		0x00000001	/* Dead Memory Zone detection */
#define	KM_DLEAK	0x00000002	/* Memory leak detection */
#define	KM_DSANITY	0x00000004	/* Do integrety checking of lists */
#define	KM_DCOOKIE	0x00000008	/* Look for untouched cookie */
#define	KM_DBOOTONLY	0x0000000f	/* Boot only flags */
#define	KM_DGETSYSPTE	0x00000010	/* get_sys_pte failures */
#define	KM_DHISTORY	0x00000020	/* Maintain history */
#define	KM_DSTYPE	0x00000040	/* Maintain type statistics */
#define	KM_DSBUCKET	0x00000080	/* Maintain bucket statistics */
#define	KM_DPKMFIND	0x00000100	/* Print km_find entries */
#define	KM_DPSCANPTE	0x00000200	/* Print km_scan_pte entries */
#define	KM_DPHWMSCAN	0x00000400	/* Print km_hwmscan entries */
#define	KM_DPDEADPTE	0x00000800	/* Print km_dead_pte entries */
#define KM_DPPTEPURGE	0x00001000	/* Print when km_scan_pte purges */
#define	KM_DPHWMTRIM	0x00002000	/* Print when bucket is trimmed */
#define	KM_DPFINDSTOLEN	0x00004000	/* Print when km_find stole */

#define	KM_DDISABLE	0x00010000	/* Disable stealing and trimming */

#define	KM_DSTATE	0xff000000	/* State flags */
#define	KM_DTIMERON	0x01000000	/* The timer is on */
#define	KM_DTIMEROFF	0x02000000	/* The timer is off */
#define	KM_DNOMEM	0x04000000	/* Faked no memory for debug */
#define	KM_DNOPTE	0x08000000	/* Faked no pte for debug */


#define	KM_DNOUSER	(KM_DBOOTONLY|KM_DSTATE)
#define	KM_DFIND	(KM_DNOMEM|KM_DNOPTE)
#define	KM_DPATTERN	0xfeebeecd

/*
 * Debug system calls.
 */


#define	KM_SSET			1	/* Set kmdebug  flags */
#define	KM_SCLEAR		2	/* Clear kmdebug flags */

struct km_dallocate {			/* Used to control allocations */
	struct km_dallocate 
			*kd_next;	/* Next one */
	caddr_t 	kd_va;		/* What the allocator returned */
};

					/* Controlled by process */
#define	KM_SPALLOCATE		3	/* Allocate kmem memory */
#define	KM_SPFREE		4	/* Free kmem memory */
#define	KM_SPFALL		5	/* 
					 * Free all memory allocated 
					 * by KM_SALLOCATE
					 */

					/* Timer driven */
struct km_stallocate {			/* Timer allocator structure */
	int 		kd_msecs;	/* Milli second delay */
	unsigned short 	kd_ssize;	/* Starting size */
	unsigned short 	kd_lsize;	/* Ending size */
	unsigned short 	kd_count;	/* Passes before freeing */
};
					/* Controlled by timer event */
#define	KM_STALLOCATE		6	/* Allocate */
#define	KM_STSTOP		7	/* Stop timer */

#define	KM_SSTEALMEM		8	/* Steal freemem */
#define	KM_SRETURNMEM		9	/* Return stolen memory */

#define	KM_SAWIRED		10	/* Allocate wired */
#define	KM_SAUNWIRED		11	/* Allocate unwired */
#define	KM_SFREE		12	/* Free allocated memory */



struct kmemd_request {			/* History of allocations */
	int size;			/* Size of request */
	int type;			/* Type of request */
	int flags;			/* Flags passed */
};

struct kmemd_ts {			/* Type statistics */
	long	ts_inuse;		/* Number of packets in use */
	long	ts_total;		/* Total packets allocated */
	long	ts_maxused;		/* Maximum in use */
	long	ts_allocfail;		/* km_alloc failures */
};

struct kmemd_bs {			/* Bucket statistics */
	long	bs_total;		/* Total allocated */
	long	bs_inuse;		/* Number in use */
	long	bs_maxused;		/* Maximim in use */
	long	bs_hwtrim;		/* High water mark trims */
	long	bs_stolen;		/* Stolen in km_find */
	long	bs_allocfail;		/* km_alloc failures */
};

struct kmemd_gs {			/* Global statistics */
	long	gs_tbsync;		/* TB syncs */
	long	gs_hwscan;		/* High water mark scans */
	long	gs_hwhits;		/* Number of times something trimmed */
	long	gs_hwmisses;		/* Number of time we got nothing */
	long	gs_kmfind;		/* Calls to km_find */
	long	gs_bstolen;		/* Stolen by km_find */
	long	gs_lowmap;		/* schedcpu low kmemmap hwm trim */
	long	gs_hardmem;		/* kmem_alloc hwm trim */
};


#ifdef KERNEL
extern struct pte kmempt[];
extern struct pte ekmempt[];
extern struct kmemusage *kmemusage;
extern struct kmemusage *uwkmemusage;
extern char kmembase[];
extern char kmemlimit[];
extern caddr_t km_alloc();
extern void km_free();
extern int kmemu[];
extern int kmemneedmap, kmemneedwmap;
extern int km_wmapsize;
struct lock_t lk_buckets;
extern short kmbucket_hwm;
extern short kmemhwm[MAXBUCKETSAVE+1];
#ifdef	B_PROTECTION
extern struct kmembuckets *bucket;
extern struct kmembuckets *wbucket;
#else
extern struct kmembuckets bucket[MAXBUCKETSAVE+1];
extern struct kmembuckets wbucket[MAXBUCKETSAVE+1];
#endif /*	B_PROTECTION */
extern int kmdebug;
extern int kmdebugboot;
#ifdef	KMEM_DEBUG
extern struct kmemd_ts kmemd_ts[KM_LAST];
extern struct kmemd_bs kmemd_bs[MAXBUCKETSAVE+1];
extern struct kmemd_gs kmemd_gs;
extern struct kmemd_request kmemd_requests[KM_REQSIZ];
extern int kmemd_request_location;
#endif /*	KMEM_DEBUG */
#endif /* KERNEL */
#endif /* defined(__vax) && !defined(LOCORE)||(defined(__mips) && defined(__LANGUAGE_C)) */
#endif /* _KMALLOC_ */
