/*	@(#)mbuf.h	4.4	(ULTRIX)	1/22/91	*/

/************************************************************************
 *									*
 *			Copyright (c) 1985 by				*
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

/************************************************************************
 *			Modification History				*
 *									*
 *	Matt Thomas - 12/06/89						*
 *		Conditionize kmalloc.h for KERNEL/not KERNEL		*
 *									*
 *	Matt Thomas - 07/17/89						*
 *		Change ifnetptr field into a union so it can		*
 *		be used genericly by other modules.			*
 *									*
 *	R. Bhanukitsiri - 02/03/89					*
 *		Reflect V3.2 source pool changes.			*
 *									*
 *	Larry Palmer - 15-Jan-88					*
 *		Changes for malloced mbufs.				*
 *									*
 *	Larry Cohen -10/09/86						*
 *		fix MFREE - wakeup on address of mfree instead of mfree *
 *									*
 *	Jeff Chase - 03/12/86						*
 *		Changes for "type 2" mbufs:				*
 *			New MCLGET macro with different usage		*
 *			Changes to MFREE macro				*
 *									*
 *	Larry Cohen  -	09/16/85					*
 * 		Add 43bsd alpha tape changes  				*
 *									*
 ************************************************************************/

/*
 * Copyright (c) 1982 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)mbuf.h	6.5 (Berkeley) 6/8/85
 */


#ifndef _MBUF_
#define _MBUF_
#define NEWCL
#ifdef KERNEL
#include "../h/ansi_compat.h"
#include "../h/kmalloc.h"
#else
#include <ansi_compat.h>
#include <sys/kmalloc.h>
#endif
/*
 * Constants related to memory allocator.
 */
#define	MSIZE		128	/* size of an mbuf */
#define	MMINOFF		20 	/* 5*sizeof(long) - 5 things in mbuf header */
#define	MMAXOFF		(MSIZE)	/* offset where data ends */
#define	MLEN		(MSIZE-MMINOFF)	/* mbuf data length */

#define M_CLTYPE1	1	/* Defined mbuf cluster types */
#define M_CLTYPE2	2	/* nfs.... */

/*
 * Macros for type conversion
 */


/* network/kmalloc cluster number to virtual address, and back */
#define	cltom(x) ((struct mbuf *)((int)kmembase + ((x) << PGSHIFT)))
#define	mtocl(x) (((int)x - (int)kmembase) >> PGSHIFT)

/*
 * Address in mbuf to mbuf head. NOTE: calls to "dtom" must be preceded
 * by a call to m_pullup() to ensure that the mbuf is NOT a cluster-type
 * mbuf, since dtom will not properly determine the address of the header
 * of a cluster-type mbuf.
 */
#define	dtom(x)		((struct mbuf *)(((int)(x) & ~(MSIZE-1))))

/* mbuf head, to typed data */
#define	mtod(x,t)	((t)(((x)->m_off > MMAXOFF) ? (int)(x)->m_off : (int)(x) + (x)->m_off))

/* The mbuf */
#if defined(__vax) || (defined(__mips) && defined(__LANGUAGE_C))
struct mbuf {
	struct	mbuf *m_next;		/* next buffer in chain */
	unsigned long	m_off;		/* offset of data */
	short	m_len;			/* amount of data in this mbuf */
	short	m_type;			/* mbuf type (0 == free) */
	struct	mbuf *m_act;		/* link in higher-level mbuf list */
	union {
		struct ifnet *mun1_ifp;	/* pointer to ifnet structure */
		int mun1_context;	/* user-defined context field */
	} m_un1;
	union {
		u_char	mun_dat[MLEN];	/* data storage */
		struct {
			short	mun_cltype;	/* "cluster" type */
			caddr_t	mun_clptr; 	/* cluster head */
			int	(*mun_clfun)();
			int	mun_clarg;
			int	(*mun_clswp)();
		} mun_cl;
	} m_un;
};

#define m_ifp	m_un1.mun1_ifp
#define m_context m_un1.mun1_context
#define	m_dat	m_un.mun_dat
#define	m_cltype m_un.mun_cl.mun_cltype
#define	m_clptr m_un.mun_cl.mun_clptr
#define	m_clfun	m_un.mun_cl.mun_clfun
#define	m_clarg	m_un.mun_cl.mun_clarg
#define	m_clswp	m_un.mun_cl.mun_clswp
#endif /* defined(__vax) || (defined(__mips) && defined(__LANGUAGE_C)) */

/* mbuf types */
/* See malloc.h */
#define	MT_FREE		KM_FREEL	/* should be on free list */
#define	MT_DATA		KM_MBUF		/* dynamic (data) allocation */

#define MT_HEADER	MT_DATA 	/* Gone tomorrow! */

#define	MT_SOCKET	KM_SOCKET	/* socket structure */
#define	MT_PCB		KM_PCB		/* protocol control block */
#define	MT_RTABLE	KM_RTABLE	/* routing tables */
#define	MT_HTABLE	KM_HTABLE	/* IMP host tables */
#define	MT_ATABLE	KM_ATABLE	/* address resolution tables */
#define	MT_SONAME	KM_SONAME	/* socket name */
#define	MT_ZOMBIE	KM_ZOMBIE	/* zombie proc status */
#define	MT_SOOPTS	KM_SOOPTS	/* socket options */
#define	MT_FTABLE	KM_FTABLE	/* fragment reassembly header */
#define	MT_RIGHTS	KM_RIGHTS	/* access rights */
#define MT_OPT		KM_OPT		/* DECnet optional data */
#define	MT_IFADDR	KM_IFADDR	/* interface address */
#define	MT_ACCESS	KM_ACCESS	/* access control info */

/* flags to m_get */
#define	M_DONTWAIT	KM_NOWAIT
#define	M_WAIT		KM_NOARG


/* flags to m_pgalloc */
#define	MPG_MBUFS	0		/* put new mbufs on free list */
#define	MPG_CLUSTERS	1		/* put new clusters on free list */
#define	MPG_SPACE	2		/* don't free; caller wants space */

#define NCLBYTES	1024	/* How we try to fragment ether traffic */
#ifdef	__mips
#define M_CLUSTERSZ	4096		/* Size of clusters */
#endif /*	__mips */
#ifdef	__vax
#define	M_CLUSTERSZ	(2*CLBYTES)	/* Size of clusters */
#endif /*	__vax */
#define	M_NETPAD	32		/* Size for LLC and MAC headers */

/* length to m_copy to copy all */
#define	M_COPYALL	1000000000

#define	MGET(m, i, t) \
	KM_ALLOC(m, struct mbuf *, sizeof(struct mbuf), t, i); \
		if((m)) { \
		(m)->m_type = t; (m)->m_act = 0;\
		(m)->m_off = MMINOFF; (m)->m_len = MLEN; \
		(m)->m_next = 0; \
		}

#define	MCLGET(m, p) \
	KM_ALLOC((p), struct mbuf *, M_CLUSTERSZ, KM_CLUSTER, KM_NOWAIT); \
	if((p)) { \
		(m)->m_cltype = M_CLTYPE1; (m)->m_clptr = (caddr_t) p; \
		m->m_off = (int)(p); m->m_len = M_CLUSTERSZ; \
	}

#define MCLPUT(m, p) \
		(m)->m_cltype = M_CLTYPE1; (m)->m_clptr = (caddr_t) p; \
		m->m_off = (int)(p); m->m_len = M_CLUSTERSZ; \
	
#define	MFREE(m, n) \
	  if ((m)->m_off > MSIZE) { \
		if (m->m_cltype == M_CLTYPE1) { \
			KM_FREE((m)->m_clptr, KM_CLUSTER); \
		} else if (m->m_cltype == M_CLTYPE2) \
			(*m->m_clfun)(m->m_clarg); \
		else \
			panic("m_free has bad m_cltype"); \
	  } \
	  (n) = (m)->m_next; \
	  KM_FREE(m, m->m_type);

/*
 * Mbuf statistics. Kept for purely historical reasons; no longer
 * referenced by ULTRIX utilities.
 */
#if defined(__vax) || (defined(__mips) && defined(__LANGUAGE_C))
struct mbstat {
	short	m_mbufs;	/* mbufs obtained from page pool */
	short	m_clusters;	/* clusters obtained from page pool */
	short	m_clfree;	/* free clusters */
	short	m_drops;	/* times failed to find space */
	short	m_mtypes[256];	/* type specific mbuf allocations */
};

#ifdef	KERNEL
extern	struct mbuf mbutl[];		/* virtual address of net free mem */
extern	char kmembase[];		/* virtual address of net free mem */
extern	struct pte kmempt[];		/* page tables to map Netutl */
struct	mbstat mbstat;
struct	mbuf *m_get(),*m_getclr(),*m_free(),*m_copy(),*m_pullup();
caddr_t	m_clalloc();
#endif /* KERNEL */
#endif /* defined(__vax) || (defined(__mips) && defined(__LANGUAGE_C)) */
#endif /* _MBUF_ */
