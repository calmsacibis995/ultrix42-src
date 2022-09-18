#ifndef lint
static char *sccsid = "@(#)map.c	4.1	(ULTRIX)	7/17/90";
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

#include <stdio.h>
#include <sys/param.h>
#include <sys/dir.h>
#include <sys/map.h>
#include <nlist.h> 

char	*fcore	= "/dev/kmem";
char	*fnlist	= "/vmunix";
int	fc = -1;
long lseek();
char *calloc();
void nlist(),exit(),free();

struct nlist nl[] = {
#define	SWAPMAP	0
#ifdef vax
	{ "_swapmap", 0, '\0', 0, 0 },
#endif
#ifdef mips
	{ "_swapmap", 0, 0, 0 },
#endif

#define ARGMAP 1
#ifdef vax
	{ "_argmap", 0, '\0', 0, 0 },
#endif
#ifdef mips
	{ "_argmap", 0, 0, 0 },
#endif

#define KERNELMAP 2
#ifdef vax
	{ "_kernelmap", 0, '\0', 0, 0 },
#endif
#ifdef mips
	{ "_kernelmap", 0, 0, 0 },
#endif
#define MBMAP 3
#ifdef vax
	{ "_mbmap", 0, '\0', 0, 0 },
#endif
#ifdef mips
	{ "_mbmap", 0, 0, 0 },
#endif
#define DMEMMAP 4
#ifdef vax
	{ "_dmemmap", 0, '\0', 0, 0 },
#endif
#ifdef mips
	{ "_dmemmap", 0, 0, 0 },
#endif
#define MSGMAP 5
#ifdef vax
	{ "_msgmap", 0, '\0', 0, 0 },
#endif
#ifdef mips
	{ "_msgmap", 0, 0, 0 },
#endif
#define SEMMAP 6
#ifdef vax
	{ "_semmap", 0, '\0', 0, 0 },
#endif
#ifdef mips
	{ "_semmap", 0, 0, 0 },
#endif
#define QB_MAP 7
#ifdef vax
	{ "_qb_map", 0, '\0', 0, 0 },
#endif
#ifdef mips
	{ "_qb_map", 0, 0, 0 },
#endif
#define UB_MAP 8
#ifdef vax
	{ "_cabase", 0, '\0', 0, 0 },
#endif
#ifdef mips
	{ "_cabase", 0, 0, 0 },
#endif
#ifdef vax
	{ "", 0, '\0', 0, 0 }
#endif
#ifdef mips
	{ "", 0, 0, 0 }
#endif
};

#define NUM_MAPS (sizeof(nl)/sizeof(struct nlist) - 1)
int num_maps = NUM_MAPS;
extern int map_to_do;

char *map_names[NUM_MAPS] = {
	"Swap Map      (0)",
	"Argument Map  (1)",
	"Kernel Map    (2)",
	"Mbuf Map      (3)",
	"Dmem Map      (4)",
	"Message Map   (5)",
	"Semaphore Map (6)",
	"Qbus Map      (7)",
	"Unibus Map    (8)"
};

domap()
{
	register struct mapent *bp, *xbp, *abp;
	register int nin,nmap,nused=0,resavail=0;
	struct map header;
	int openslot=0;

	nlist(fnlist, nl);
	if (fc == -1) fc = open(fcore,0,0);
	if (fc == -1) {
		printf("can't open %s\n",fcore);
		return;
	}
	if (nl[map_to_do].n_type == 0) {
		(void) fprintf(stderr,"no namelist for %s\n", map_names[map_to_do]);
		return;
	}
	if (map_to_do < MSGMAP) {
		/* indirect addresses */
		abp = (struct mapent *)getint(nl[map_to_do].n_value);
	}
	else {
		/* direct addresses */
		abp = (struct mapent *)nl[map_to_do].n_value;
	}
	if (lseek(fc,(long)abp,0) != (long) abp) {
		(void) fprintf(stderr,"seek error\n");
		exit(1);
	}
	if (read(fc, (char *)&header, sizeof(struct map))
		!= sizeof(struct map)) {
		(void) fprintf(stderr,"error reading header\n");
		exit(1);
	}
	nmap = (header.m_limit - abp);
	nmap--;
	nin = nmap * sizeof(struct map);
	xbp = (struct mapent *)calloc((unsigned int)nmap,sizeof(struct mapent));
	if (read(fc, (char *)xbp, nin) != nin) {
		(void) fprintf(stderr,"error reading map\n");
		exit(1);
	}
	for (bp = xbp; bp < &xbp[nmap]; bp++) {
		if (bp->m_size) {
			resavail += bp->m_size;
			nused++;
		}
		else {
			openslot++;
		}
	}
	(void) printf("Map Name = %s\n",map_names[map_to_do]);
	(void) printf(
	    "Number of map entries = %d, Number in use = %d, or %g%%\n",
		nmap,nused,(double)nused * 100. / (double)nmap);
	(void) printf("Total Resources in map = %d\n", resavail);
	(void) printf("Openslots in map = %d\n", openslot);
	(void) printf("Resource Size\tResource Start\n");
	for (bp = xbp; bp < &xbp[nmap]; bp++) {
		if (bp->m_size) {
			(void) printf("%d\t\t%d\n",bp->m_size,bp->m_addr);
		}
	}
	free((char *)xbp);
}

getint(loc)
	unsigned long loc;
{
	int word;

	if(lseek(fc,(long)loc,0) != (long)loc) {
		(void) fprintf(stderr,"seek error in getint\n");
		exit(1);
	}
	if (read(fc, (char *)&word, sizeof (word)) != sizeof(word)){
		(void) fprintf(stderr,"read error in getint\n");
		exit(1);
	}
	return (word);
}
