/*      @(#)crash.h	4.1     (ULTRIX)       7/17/90      */

/************************************************************************
 *									*
 *			Copyright (c) 1988-1989 by			*
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
 * History:
 *
 * 02-Mar-90  Janet Schank
 *      Changed Nscsi to Nscsibus.
 *
 */

#include  <sys/types.h>
#include  <sys/param.h>
#include  <signal.h>
#include  <sys/dir.h>
#include  <sys/user.h>
#include  <stdio.h>
#include  <strings.h>
#include  <ctype.h>
#include  <rpc/rpc.h>

#define	VIRT_MEM	0x3fffffff
#define	SYM_VALUE(ptr)	((ptr)->s_value & VIRT_MEM)
#define	FMT	"%8.8x"
#define	SWAPPED	1	/* Returned by getuarea if process is swapped */

struct	tsw	{
	char	*t_nm;
	int	(*t_action)();
	char	*t_dsc;
} ;

struct	prmode	{
	char	*pr_name;
	int	pr_sw;
} ;

struct	Symbol	{
	char	*s_name;
	unsigned	s_value;
	int	s_type;
	int	s_class;
};
/*
 * Simple values for s_type.
 */
#define	S_UNDF	0x0		/* undefined */
#define	S_ABS	0x2		/* absolute */
#define	S_TEXT	0x4		/* text */
#define	S_DATA	0x6		/* data */
#define	S_BSS	0x8		/* bss */
#define	S_COMM	0x12		/* common (internal to ld) */
#define	S_FN	0x1f		/* file name symbol */

#define	S_EXT	01		/* external bit, or'ed in */
#define	S_TYPE	0x1e		/* mask for all the type bits */


extern	int	mem;		/* fd for core file (mem or dumpfile) */
extern	int	sptptr;		/* offset of SPT in core file */
extern	int	sptlen;		/* length of system page table */
extern 	int	firstfree;	/* PFN of first free physical page */

struct	uarea {
	union {
		struct user u;
		char upages[UPAGES][NBPG];
	} ustuff
};
extern struct uarea u;
#define U u.ustuff.u

/* These point to symbol table entries for various kernel structures. */
extern struct Symbol	Swap, Sys, Panic, Text, Sptbase, Callout, Lbolt, 
			Region, Usrptma, Usrpt, Cmap, Buffree, Gfree,
			Arptab, Dnlcache, Cmhash, Mscp_classb, Tmscp_classb,
			Mscp_utable, Tmscp_utable, Sz_softc, Nscsibus,
			Scs_config_db, Ports ;
			
/* The tab table keeps information about the kernel tables. */

struct tabsum {
	char *name;	/* name of table described by this entry */
	unsigned first;	/* first vaddr occupied by the table */
	unsigned last;	/* last vaddr occupied by the table */
	int size;	/* size of an entry in the table */
	int ents;	/* number of entries in the table */
};

/* Some indices for the tab table */
#define PROC_T		 0
#define MOUNT_T		 1
#define FILE_T		 2
#define GNODE_T		 3
#define BUF_T		 4
#define TEXT_T		 5
#define CALLOUT_T	 6
#define CMAP_T		 7
#define BUFH_T		 8
#define ARP_T		 9
#define BUFFER_T	10
#define DNLC_T		11
#define CMHASH_T	12
#define TABSUM_MAX	13

extern struct tabsum tab[TABSUM_MAX];	/* defined in misc.c */

/* The map table keeps information about kernel page table maps. */

struct mapsum {
	char *name;	/* name of spt described by this entry */
	char *descrip;	/* description of map, eg "mbuf pool" */
	unsigned first;	/* first vaddr mapped by this spt */
	unsigned last;	/* last vaddr mapped by this spt */
};

/* Some indices for the map table */
#define MBUF_SPT	0
#define DMEM_SPT	1
#define	MAPSUM_MAX	2

extern struct mapsum map[MAPSUM_MAX];	/* defined in misc.c */

struct tabloc {
	int index;
	int offset;
	struct tabsum *tab;
	unsigned addr;
	char tabname[60];
};

#define	DIRECT	2
#define	OCTAL	3
#define	DECIMAL	4
#define	CHAR	5
#define	WRITE	6
#define	BYTE	8
#define	LDEC	9
#define	LOCT	10
#define	HEX	11
#define STRING	12
#define GNODE	13

/* memory flags */
#define		MIMSK	0x00ff	/* mask for input format */
#define		MPFN	0x0001	/* use PFN rather than CMAP index */
#define         MADDR   0x0002  /* use system address */
#define         MBLKNO  0x0004  /* use blkno */
#define		MFMSK	0xff00  /* mask for print format */
#define		MONE	0x0100	/* dump only 1 cmap entry */
#define		MFREE_CRASH	0x0200	/* dump CMAP free list */
#define		MHASH	0x0400  /* dump CMAP hash list */

struct	dstk	{
	int	r0;
	int	r1;
	int	r2;
	int	r3;
	int	r4;
	int	r5;
	int	r6;
	int	r7;
	int	r8;
	int	r9;
	int	r10;
	int	r11;
	int	r12;
	int	r13;
	int	ipl;
	int	mapen;
	int	pcbb_x;
	int	stkptr;
} ;

struct	glop	{
	int	g_x0;
	int	g_x1;
	int	g_r0;
	int	g_r1;
	int	g_r2;
	int	g_r3;
	int	g_r4;
	int	g_r5;
	int	g_sp;
	unsigned  int  g_ka6;
} ;

/*
 * These are defined in /sys/netinet/if_ether.c.
 */
#define	ARPTAB_BSIZ	15
#define	ARPTAB_NB	29
#define ARPTAB_SIZE	ARPTAB_BSIZ * ARPTAB_NB

#define MAXCLIENTS	36

struct buckets {
	char	 	*addr;
	int		index;
	struct 	buckets	*next;
};

#define GNODEBUCKETS	13		/* should be prime */
#define FILEBUCKETS	17
#define MOUNTBUCKETS	7
#define BUFBUCKETS	53
#define PROCBUCKETS	23

#define HASH(addr, buckets)		((u_int) (addr) % (buckets))

extern struct buckets *gnodebuckets[];
extern struct buckets *filebuckets[];
extern struct buckets *mountbuckets[];
extern struct buckets *bufbuckets[];
extern struct buckets *procbuckets[];

extern struct gnode *gnodetab;
extern struct file *filetab;
extern struct mount *mounttab;
extern struct buf *buftab;
extern struct proc *proctab;




