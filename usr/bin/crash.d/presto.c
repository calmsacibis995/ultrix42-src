#ifndef lint
static char *sccsid = "@(#)presto.c	4.1    (ULTRIX)        4/11/91";
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
#include	<sys/smp_lock.h>
#include	<sys/gnode_common.h>
#include	<ufs/ufs_inode.h>
#include	<sys/gnode.h>
#include	<sys/buf.h>
#include 	<sys/presto.h>
#include <stdio.h>


char *token();

c_presto(c)
	char *c;
{
	char *arg;
	int i;
	int praddr,prsize,prattached,pr_machineid,prnbufs,pr_flags, pr_bufs;
	struct presto_status prstatus;
	struct buf *prbufs;

	arg = token();
	
	if (symsrch("_praddr") == NULL) {
		printf("Presto is not configured.\n");
		return;
	}

	readsym(symsrch("_praddr"), &praddr, sizeof(praddr));
	readsym(symsrch("_pr_flags"), &pr_flags, sizeof(pr_flags));
	readsym(symsrch("_prsize"), &prsize, sizeof(prsize));
	readsym(symsrch("_prattached"), &prattached, sizeof(prattached));
	readsym(symsrch("_pr_machineid"), &pr_machineid, sizeof(pr_machineid));
	readsym(symsrch("_prnbufs"), &prnbufs, sizeof(prnbufs));
	readsym(symsrch("_prbufs"), &pr_bufs, sizeof(pr_bufs));


	printf("Presto:  addr 0x%8x size %d (nbufs %d) attached %d machine id %d flags:",
	       praddr,prsize,prnbufs,prattached,pr_machineid);
	printf("%s\n",
	pr_flags & PR_BOUNCEIO ? " bounceio" : "");

	readsym(symsrch("_prstatus"), &prstatus, sizeof(prstatus));
	printf("presto_status: state %s%s%s dirty %d clean %d inval %d\n",
	       (prstatus.pr_state == PRDOWN) ? "down":"",
	       (prstatus.pr_state == PRUP) ? "up":"",
	       (prstatus.pr_state == PRERROR) ? "*error*":"",
	       prstatus.pr_ndirty,prstatus.pr_nclean,prstatus.pr_ninval);

	if (arg != NULL && (strcmp(arg,"-buf") == 0)) {
		prbufs = (struct buf *)malloc(prnbufs*sizeof(struct buf));
		if (readmem(prbufs, pr_bufs, sizeof(struct buf)*prnbufs)
		    != (sizeof(struct buf)*prnbufs)) {
			perror("presto cache read");
			return(0);
		}
	
		printbufhd();
		for (i=0;i<prnbufs;i++) {
			printf("%4d ",i);
			printbuf(&prbufs[i]);
		}
		arg = token();
	}
	if (arg != NULL && (strcmp(arg,"-dev") == 0)) {
		pr_prtab();
		arg = token();
	}
}

/*
 * Prtab structure - kernel data structure kept per presto-ized major device.
 */
struct prtab {
	u_int pt_bmajordev;		/* 0: block major device numer */
	int (*pt_strategy)();		/* 4: major dev strategy routine */
	int (*pt_ready)();		/* 8: major dev `ready' routine */
	struct prbits pt_bounceio;	/* 12: per minor dev bounceio bits */
	struct prbits pt_enabled; 	/* 44: per minor enabled bits */
	struct prbits pt_error; 	/* 76: per minor dev error bits */
	struct prbits pt_flushing; 	/* 108: per minor dev flushing bits */
	                                /* 140 bytes long */
};


pr_prtab()
{
	int i, n;
	struct prtab *pt, *ptt, entry, *prtabs[100];
	int  pr_nprdev;

	readsym(symsrch("_pr_nprdev"), &pr_nprdev, sizeof(pr_nprdev));
	printf("nprdev = %d\n",pr_nprdev);

	readsym(symsrch("_prtabs"), prtabs, 
		sizeof(struct prtab *)*pr_nprdev);
	
	for (n=0; n<pr_nprdev;n++) {
		ptt=prtabs[n];
		if (ptt==NULL)
			continue;

		if (readmem(&entry, (int *)ptt, sizeof(struct prtab))
		    != (sizeof(struct prtab))) {
			perror("presto prtab read");
			return(0);
		}
	
		printf(" slot %d major dev: %d  ",n,entry.pt_bmajordev);
		praddr(entry.pt_strategy);
		printf("\n");
		printf("\tEnable:   ");
		for (i=0;i<256;i++) 
			if (isset(entry.pt_enabled.bits,i))
				printf("%3d ",i);
		printf("\n");
		printf("\tBounceIo: ");
		for (i=0;i<256;i++) 
			if (isset(entry.pt_bounceio.bits,i))
				printf("%3d ",i);
		printf("\n");
		printf("\tError:    ");
		for (i=0;i<256;i++) 
			if (isset(entry.pt_error.bits,i))
				printf("%3d ",i);
		printf("\n");
		printf("\tFlushing: ");
		for (i=0;i<256;i++) 
			if (isset(entry.pt_flushing.bits,i))
				printf("%3d ",i);
		printf("\n");
	}
}
