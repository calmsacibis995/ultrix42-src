#ifndef lint
static char *sccsid = "@(#)pfstat.c	4.2	(ULTRIX)	2/26/91";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1989 by				*
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
 * pfstat.c
 *
 * Print packet filter status & statistics: ULTRIX version
 *
 * HISTORY:
 * 8 August 1989	Jeffrey Mogul	DECWRL
 *	- Revised for ULTRIX 4.0 version
 *	A few new state bits; "hz" now read from kernel; name change;
 *	slight format change for AllDescriptors/LINK-QUEUE
 *
 * 29 November 1988	Jeffrey Mogul	DECWRL
 *	- Revised for ULTRIX 3.0 version
 *	Mostly this means that the state tables are dynamically
 *	allocated
 *
 * 8 October 1985	Jeff Mogul	Stanford
 *	- Revised for 4.3 version of driver.
 *
 * 17 October 1984	Jeff Mogul	Stanford
 *	- Prints RecvCount field; this comes out as part of
 * 	the "Filters" listing, even though it should probably be
 *	with the "AllDescriptors" list, because it fits better on
 *	the output lines.
 *
 * 3 October 1984	Jeff Mogul	Stanford
 *	- Prints state of "enOneCopy", if present
 *
 * 17 February 1984	Jeff Mogul	Stanford
 *	- Fixes from "decvax!genrad!grkermit!masscomp!clyde!watmath!arhwhite"
 *	to:
 *		make "k" option work (using core dumps instead of kmem);
 *
 * 15 February 1984	Jeff Mogul	Stanford
 *	- Added printout of underlying device name
 *
 * 20 December 1983	Jeffrey Mogul	Stanford
 *	- added unit number options ("01234567")
 *
 * 2 December 1983	Jeffrey Mogul	Stanford
 *	- added printout of new endevp structure, new "p" flag
 *
 * Jeffrey Mogul	Stanford	28 April 1983
 *
 * Derived from:
 *	static char *sccsid = "@(#)pstat.c	4.9 (Berkeley) 5/7/81";
 * and from
 *	modifications thereto by Mike Accetta @ CMU
 */

#define mask(x) (x&0377)
#ifdef	old
#define	clear(x) ((int)(x)&0x7FFFFFFF)
#else
#define	clear(x) (x)
#endif	old

#include <stdio.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <net/pfilt.h>
#include <net/pfiltdefs.h>
#include <sys/vm.h>
#include <machine/pte.h>

#include <net/if.h>

#include <nlist.h>


char   *fcore = "/dev/kmem";
char   *fnlist = "/vmunix";
int     fc;
int	hz;

struct nlist    nl[] = {

#define	SENSTATEPS	(0)
    {
	"_enStatePs"
    },
#define	SENQELTS	(1)
    {
	"_enQueueElts"
    },
#define	SENFREEQ	(2)
    {
	"_enFreeq"
    },
#define	SENUNITS	(3)
    {
	"_enUnits"
    },
#define	SENFREEQMIN 	(4)
    {
	"_enFreeqMin"
    },
#define	SENSCAVENGES	(5)
    {
	"_enScavenges"
    },
#define	SENDEBUG	(6)
    {
	"_enDebug"
    },
#define	SENXFREEQ	(7)
    {
	"_enXfreeq"
    },
#define	SENALLOC	(8)
    {
	"_enAllocCount"
    },
#define	SENETINFO	(9)
    {
	"_enet_info"
    },
#define	X_SYSMAP	(10)
    {
	"_Sysmap"
    },
#define	X_SYSSIZE	(11)
    {
	"_Syssize"
    },
#define	SENONECOPY	(12)
    {
	"_enOneCopy"
    },
#define	SENMAXMINORS	(13)
    {
	"_enMaxMinors"
    },
#define	SENALLOCMAP	(14)
    {
	"_enAllocMap"
    },
#define	SENDESCRIPTORS	(15)
    {
	"_enAllDescriptors"
    },
#define	SENUNITMAP	(16)
    {
	"_enUnitMap"
    },
#define	SENREORDERCOUNT	(17)
    {
	"_enReorderCount"
    },
#define	SENSCAVDROPS	(18)
    {
	"_enScavDrops"
    },

#define	SENPERPKTPROF	(19)
    {
	"_enPerPktProf"
    },

#define	SENPKTCOUNT	(20)
    {
	"_enPktCount"
    },

#define	SENPERFILTPROF	(21)
    {
	"_enPerFiltProf"
    },

#define	SENFILTCOUNT	(22)
    {
	"_enFiltCount"
    },

#define	SHZ		(23)
    {
	"_hz"
    },

    0,
};

int     kflg = 0;
int     Verbose = 0;
int     Counts = 0;
int     FreeStats = 0;
int     Descriptors = 0;
int     Filters = 0;
int     QueueElements = 0;
int 	Parameters = 0;
int	UnitMask = 0;

main (argc, argv)
char  **argv;
{
    register char  *argp;
    int     something = 0;
    char *progname=argv[0];

    argc--, argv++;
    while (argc > 0 && **argv == '-') {
	argp = *argv++;
	argp++;
	argc--;
	while (*argp++)
	    switch (argp[-1]) {

		case 'k': 
		    kflg++;
		    fcore = "/vmcore";
		    break;

		case 'v': 
		    Verbose++;
		    break;

		case 'c': 
		    Counts++;
		    something++;
		    break;

		case 's': 
		    FreeStats++;
		    something++;
		    break;

		case 'd': 
		    Descriptors++;
		    something++;
		    break;

		case 'f': 
		    Filters++;
		    something++;
		    break;

		case 'q': 
		    QueueElements++;
		    something++;
		    break;

		case 'p':
		    Parameters++;
		    something++;
		    break;

		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		    UnitMask |= (1<<(argp[-1] - '0'));
		    break;

		default:
		    fprintf(stderr,
		    	"Usage: %s [-kvcsdfqp01234567] [nlist [core]]\n", progname);
		    fprintf(stderr,
    "\t(c=Counts, s=FreeStats, d=Descriptors, f=filters, q=QueueElts,\n");
		    fprintf(stderr,
    "\t p=Parameters); 0-7 specifies unit\n");
		    exit(1);
	    }
    }
    if (!something) {		/* if none given then use all */
	Counts = FreeStats = Descriptors = Filters = QueueElements = 1;
	Parameters = 1;
    }
    if (argc > 0) {		/* different system image */
	fnlist = argv[0];
	argv++;
	argc--;
    }
    if (argc > 0)
	fcore = argv[0];

    if (UnitMask == 0)		/* no specific units, do them all */
    	UnitMask = 0xFFFFFFFF;

    if ((fc = open (fcore, 0)) < 0) {
	fprintf (stderr, "%s: cannot open %s\n", progname, fcore);
	exit (1);
    }

    nlist (fnlist, nl);
    if (nl[0].n_type == 0) {
	/* printf ("no namelist\n"); */
	printf("PACKETFILTER option is not configured in %s\n", fnlist);
	exit (1);
    }
    hz = nl[SHZ].n_value;

    doenet ();
}

/* copied from enet./sys/net/enet.c */
struct enet_info {
	struct ifnet *ifp;
};

#define	NEN	8	/* enough for a good long while XXX */

int    enMaxMinors;

doenet () {
    struct enPacket enQueueElts[ENPACKETS];
    struct enQueue  enFreeq;
    struct enState  enStateData[NEN];
    struct enState *enStatePs[NEN];
    struct enet_info enet_info[NEN];
    u_char enAllocMap[256];
    u_char enUnitMap[256];
    struct enOpenDescriptor *enAllDescriptors[256];
    struct enOpenDescriptor ThisDescriptor;
    int     enFreeqMin,
            enScavenges,
            enScavDrops,
            enDebug,
	    enUnits,
	    enOneCopy;
    int     enAllocCount, enReorderCount;
    int	    enPktCount, enFiltCount;
    struct timeval enPerPktProf, enPerFiltProf;
    register    loc;
    register struct enOpenDescriptor   *op;
    register struct enPacket   *p;
    register int    i;
    register struct enState *enStatep;
    int unit;

    klseek (fc, (long) nl[SENUNITS].n_value, 0);
    read (fc, &enUnits, sizeof (enUnits));
    klseek (fc, (long) nl[SENMAXMINORS].n_value, 0);
    read (fc, &enMaxMinors, sizeof (enMaxMinors));

    klseek (fc, (long) nl[SENSTATEPS].n_value, 0);
    read (fc, enStatePs, enUnits * sizeof (enStatePs));
    for (i = 0; i < enUnits; i++) {
	klseek (fc, (long) enStatePs[i], 0);
	read (fc, &(enStateData[i]), sizeof(struct enState));
    }
    klseek (fc, (long) nl[SENALLOCMAP].n_value, 0);
    read (fc, enAllocMap, enMaxMinors * sizeof(u_char));
    klseek (fc, (long) nl[SENUNITMAP].n_value, 0);
    read (fc, enUnitMap, enMaxMinors * sizeof(u_char));
    klseek (fc, (long) nl[SENDESCRIPTORS].n_value, 0);
    read (fc, enAllDescriptors,
    			enMaxMinors * sizeof(struct enOpenDescriptor *));

    klseek (fc, (long) nl[SENETINFO].n_value, 0);
    read (fc, enet_info, enUnits * sizeof (struct enet_info));
    klseek (fc, (long) nl[SENQELTS].n_value, 0);
    read (fc, enQueueElts, sizeof (enQueueElts));
    klseek (fc, (long) nl[SENFREEQ].n_value, 0);
    read (fc, &enFreeq, sizeof (enFreeq));
    klseek (fc, (long) nl[SENFREEQMIN].n_value, 0);
    read (fc, &enFreeqMin, sizeof (enFreeqMin));
    klseek (fc, (long) nl[SENSCAVENGES].n_value, 0);
    read (fc, &enScavenges, sizeof (enScavenges));
    klseek (fc, (long) nl[SENSCAVDROPS].n_value, 0);
    read (fc, &enScavDrops, sizeof (enScavDrops));
    klseek (fc, (long) nl[SENDEBUG].n_value, 0);
    read (fc, &enDebug, sizeof (enDebug));
    if (nl[SENALLOC].n_type) {
	klseek (fc, (long) nl[SENALLOC].n_value, 0);
	read (fc, &enAllocCount, sizeof (enAllocCount));
    }
    if (nl[SENREORDERCOUNT].n_type) {
	klseek (fc, (long) nl[SENREORDERCOUNT].n_value, 0);
	read (fc, &enReorderCount, sizeof (enReorderCount));
    }
    if (nl[SENONECOPY].n_type) {
	klseek (fc, (long) nl[SENONECOPY].n_value, 0);
	read (fc, &enOneCopy, sizeof (enOneCopy));
    }
    else {
	enOneCopy = -1;
    }
    if (nl[SENPERPKTPROF].n_type) {
	klseek (fc, (long) nl[SENPERPKTPROF].n_value, 0);
	read (fc, &enPerPktProf, sizeof (enPerPktProf));
    }
    if (nl[SENPKTCOUNT].n_type) {
	klseek (fc, (long) nl[SENPKTCOUNT].n_value, 0);
	read (fc, &enPktCount, sizeof (enPktCount));
    }
    if (nl[SENPERFILTPROF].n_type) {
	klseek (fc, (long) nl[SENPERFILTPROF].n_value, 0);
	read (fc, &enPerFiltProf, sizeof (enPerFiltProf));
    }
    if (nl[SENFILTCOUNT].n_type) {
	klseek (fc, (long) nl[SENFILTCOUNT].n_value, 0);
	read (fc, &enFiltCount, sizeof (enFiltCount));
    }

    if (FreeStats) {
	printf ("Scavenges:\t%d\t (%d packets dropped)\n",
			enScavenges,
			enScavDrops);
	if (nl[SENALLOC].n_type)
		printf ("Allocated:\t%d\n", enAllocCount);
	printf ("Freeq(%x): %d (Min %d) [%x,%x]\n",
		 (nl[SENFREEQ].n_value),
		enFreeq.enQ_NumQueued, enFreeqMin,
		 (enFreeq.enQ_F),  (enFreeq.enQ_B));
	if (enOneCopy >= 0)
	    printf("OneCopy:\t%d\n", enOneCopy);
	if (nl[SENREORDERCOUNT].n_type)
		printf ("Queue priority reorderings:\t%d\n", enReorderCount);
	for (i = 0; i < enMaxMinors; i++)
	    if (enAllocMap[i])
		enAllocCount++;
	printf("Space allocated for descriptors: %d bytes\n",
			enAllocCount * sizeof(struct enOpenDescriptor));
	if (nl[SENPERPKTPROF].n_type && nl[SENPKTCOUNT].n_type) {
	    if (enPktCount)
	      printf("uSecs/packet:\t%.0f",
		((enPerPktProf.tv_sec * 1000000.0) + enPerPktProf.tv_usec)/
			(enPktCount + 0.0));
	    else
	      printf("uSecs/packet:\tNo packets");
	    if (enFiltCount)
	      printf("\t\t");
	    else
	      printf("\n");
	}
	if (nl[SENPERFILTPROF].n_type && nl[SENFILTCOUNT].n_type) {
	    if (enFiltCount)
	       printf("uSecs/filter execution:\t%.0f\n",
		((enPerFiltProf.tv_sec * 1000000.0) + enPerFiltProf.tv_usec)/
			(enFiltCount + 0.0));
	    else
	      printf("uSecs/filter execution:\tNone executed\n");
	    if (enFiltCount && enPktCount)
	      printf("Avg. filters executed/packet:\t%.2f\n",
	      		(enFiltCount + 0.0)/(enPktCount + 0.0));
	}
	printf ("\n");
    }

    for (unit = 0; unit < enUnits; unit++) {
	enStatep = &(enStateData[unit]);
	
	if ((UnitMask & (1<<unit)) == 0)	/* skip this unit */
	    continue;
	
	if (enDesq.enQ_F == 0)	/* probably not needed any more */
	    continue;
	if (Counts || Parameters || Descriptors || Filters) {
	    printf ("\npf%d:\n\n", unit);
	}
	if (Counts) {
	    printf ("Xcnt:\t\t%-8d\tRcnt:\t\t%-8d\n", enXcnt, enRcnt);
	    printf ("Xdrops:\t\t%-8d\tRdrops:\t\t%-8d\n",
		    enXdrops, enRdrops);
	    printf ("Rmissed:\t%-8d\tRunwanted:\t%-8d\n",
	    		enRmissed, enRunwanted);
	    printf ("AllowPromisc:\t");
	    if (enStatep->ens_AllowPromisc) {
		printf("yes (%d)", enStatep->ens_PromiscCount);
	    }
	    else {
		printf("no     ");
	    }
	    printf ("\t\tAllowCopyAll:\t");
	    if (enStatep->ens_AllowCopyAll) {
		printf("yes (%d)", enStatep->ens_CopyAllCount);
	    }
	    else {
		printf("no     ");
	    }
	    printf ("\n");
	    printf("UserMaxWaiting:\t%d\n", enStatep->ens_UserMaxWaiting);
	    printf ("\n");
	}
	if (Parameters) {
	    struct endevp *devp = &enDevParams;
	    printf ("Device type: ");
	    switch (devp->end_dev_type) {
		case ENDT_3MB:
		     printf("3Mb");
		     break;
		case ENDT_BS3MB:
		     printf("byte-swapping 3Mb");
		     break;
		case ENDT_10MB:
		     printf("10Mb");
		     break;
		default:
		     printf("unknown (%d)", devp->end_dev_type);
		     break;
	    }
	    PrintIFP(enet_info[unit].ifp);
	    printf("\n");
	    printf("Address Length: %d\tHeader Length: %d\tMTU: %d\n",
    		devp->end_addr_len, devp->end_hdr_len, devp->end_MTU);
	    printf("Interface Address: ");
	    PrintAddress(devp->end_addr, devp->end_addr_len);
	    printf("\nBroadcast Address: ");
	    PrintAddress(devp->end_broadaddr, devp->end_addr_len);
	    printf("\n\n");
	}

	if (Descriptors) {
	    printf ("Desq(%x): %d/%d open files [%x,%x]:\n",
		     (((caddr_t)&enDesq
			- (caddr_t)enStatep + (caddr_t)enStatePs[unit])),
		    enDesq.enQ_NumQueued, enMaxMinors,
		     (enDesq.enQ_F),  (enDesq.enQ_B));
	    printf ("AllDescriptors:\n");
	    printf (" #    LOC   LINK-QUEUE  STATE WAIT-QUEUE  NQ'D    TOUT MODE  SIG   PROC(PID)");
	    printf ("\n");
	    op = &ThisDescriptor;
	    for (i = 0; i < enMaxMinors; i++) {
 		if ((enAllocMap[i] == 0) && !Verbose)
			continue;
		if (enUnitMap[i] != unit)
			continue;
		loc = (long)enAllDescriptors[i];
		if (loc == 0)
			continue;
		klseek(fc, loc, 0);
		read(fc, &ThisDescriptor, sizeof(ThisDescriptor));
#ifdef	vax
		printf ("%2d %8.1x", i, loc | 0x80000000);
#else
		printf ("%2d %8.1x", i, loc);
#endif	vax
		popdes (op);
	    }
	}

	if (Filters) {
	    printf ("\nFilters:\n");
	    printf (" #    COUNT  DROPS PRI LEN FILTER");
	    printf ("\n");
	    op = &ThisDescriptor;
	    for (i = 0; i < enMaxMinors; i++) {
	    loc = nl[SENDESCRIPTORS].n_value;
 		if ((enAllocMap[i] == 0) && !Verbose)
			continue;
		if (enUnitMap[i] != unit)
			continue;
		loc = (long)enAllDescriptors[i];
		if (loc == 0)
			continue;
		klseek(fc, loc, 0);
		read(fc, &ThisDescriptor, sizeof(ThisDescriptor));
		printf ("%2d ", i);
		printf("%8d ", op->enOD_RecvCount);
		printf("%6d ", op->enOD_Drops);
		pfilt (&op -> enOD_OpenFilter);
	    }
	}
    }
    if (QueueElements) {
	printf ("\n");
	printf ("QueueElts:\n");
	printf (
"   LOC        LINK-QUEUE     COUNT   REF FLAGS DROP    TIME");
	printf ("\n");
	loc = nl[SENQELTS].n_value;
	for (p = enQueueElts; p < &enQueueElts[ENPACKETS];
				p++, loc += sizeof (enQueueElts[0])) {
	    if ((Verbose == 0) && (p->enP_RefCount == 0))
		continue;
#ifdef	vax
	    printf ("%8.1x ", loc | 0x80000000);
#else
	    printf ("%8.1x ", loc);
#endif	vax
	    pqelt (p);
	}
    }
}

popdes (op)
register struct enOpenDescriptor   *op;
{
    static char *states[] = {
	" wait", "timed", " tout"
    };
    int     num;
    int     head;
    int	    to;

    num = (Verbose) ? ENMAXWAITING : op -> enOD_Waiting.enWQ_NumQueued;
    head = (Verbose) ? 0 : op -> enOD_Waiting.enWQ_Head;

#ifdef	TOO_WIDE
    printf (" %5x", clear (op -> enOD_Link.F));
    printf (" %5x", clear (op -> enOD_Link.B));
#else
    printf (" %8x  ", clear (op -> enOD_Link.F));
#endif	TOO_WIDE
    if ((int) op -> enOD_RecvState <= 2)
	printf (" %-5.5s ", states[(int) op -> enOD_RecvState]);
    else
	printf (" %3d   ", op -> enOD_RecvState);
    if (num)
	pwq (&op -> enOD_Waiting, head);
    else
	printf (" %-10s", "");
    printf (" %3d", op -> enOD_Waiting.enWQ_NumQueued);
    printf ("/%-3d", op -> enOD_Waiting.enWQ_MaxWaiting);
    to = op->enOD_Timeout;
    if ((to > hz*60) && (Verbose == 0)) {
	to /= (hz * 60);
	if (to > 60) {
	    to /= 60;
	    if (to > 999)
	    	printf(" long");
	    else
	    	printf(" %3dh", to);
	}	
	else
    	    printf(" %3dm", to);
    }
    else
	printf (" %4d", op -> enOD_Timeout);
    pmode(op->enOD_Flag, 6);	/* 6 = # of columns after leading space */
    printf (" %2d ", op -> enOD_SigNumb);
    printf (" %6x", clear (op -> enOD_SigProc));
    printf ("(%d)", op -> enOD_SigPid);
    printf ("\n");
    while (--num > 0) {
	PfiltNextWaitQueueIndex (head);
	printf ("%-29s", "");
	pwq (&op -> enOD_Waiting, head);
	printf ("\n");
    }
}


pwq (wq, idx)
register struct enWaitQueue *wq;
{

    int     tail = wq -> enWQ_Tail;

    PfiltPrevWaitQueueIndex (tail);
    printf (" %1s", (idx == wq -> enWQ_Head) ? "^" : " ");
    printf ("%8x", clear (wq -> enWQ_Packets[idx]));
    printf ("%s", (idx == tail) ? "$" : " ");

}

pfilt (fp)
register struct enfilter   *fp;
{

    int     w,
            arg,
            op,
            i;
    unsigned short *wp;

    printf ("%3d ", fp -> enf_Priority);
    printf ("%3d ", fp -> enf_FilterLen);

    i = 0;
    for (wp = &fp -> enf_Filter[0];
    			wp < &fp -> enf_Filter[fp -> enf_FilterLen]; wp++) {
	w = *wp;
	arg = (w & (0xffff >> ((sizeof (short) * 8) - ENF_NBPO)));
	op = (w & (0xffff << ENF_NBPA));
	switch (arg) {
	    default: 
		if (arg < ENF_PUSHWORD)
		    printf ("BADPUSH%d", arg);
		else
		    printf ("PUSHWORD+%d", arg - ENF_PUSHWORD);
		break;
	    case ENF_PUSHLIT: 
		printf ("PUSHLIT");
		break;
	    case ENF_PUSHZERO: 
		printf ("PUSHZERO");
		break;
	    case ENF_PUSHONE: 
		printf ("PUSHONE");
		break;
	    case ENF_PUSHFFFF: 
		printf ("PUSHFFFF");
		break;
	    case ENF_PUSH00FF: 
		printf ("PUSH00FF");
		break;
	    case ENF_PUSHFF00: 
		printf ("PUSHFF00");
		break;
	    case ENF_NOPUSH: 
		if (op == ENF_NOP)
		    printf ("NOP");
		break;
	}
	if (arg != ENF_NOPUSH && op != ENF_NOP)
	    printf ("|");
	switch (op) {
	    case ENF_AND: 
		printf ("AND");
		break;
	    case ENF_OR: 
		printf ("OR");
		break;
	    case ENF_XOR: 
		printf ("XOR");
		break;
	    case ENF_EQ: 
		printf ("EQ");
		break;
	    case ENF_NEQ: 
		printf ("NEQ");
		break;
	    case ENF_LT: 
		printf ("LT");
		break;
	    case ENF_LE: 
		printf ("LE");
		break;
	    case ENF_GT: 
		printf ("GT");
		break;
	    case ENF_GE: 
		printf ("GE");
		break;
	    case ENF_NOP: 
		break;
	    case ENF_COR:
	    	printf("COR");
		break;
	    case ENF_CAND:
	    	printf("CAND");
		break;
	    case ENF_CNOR:
	    	printf("CNOR");
		break;
	    case ENF_CNAND:
	    	printf("CNAND");
		break;
	    default: 
		printf ("OP%d", op);
		break;
	}
	printf (",");
	if (arg == ENF_PUSHLIT) {
	    i++;
	    printf ("%.1o,", *++wp);
	}
	if (++i >= 4) {
	    i = 0;
	    if (&wp[1] < &fp -> enf_Filter[fp -> enf_FilterLen])
		printf ("\n\t\t\t    ");
	}
    }
    printf ("\n");

}

pqelt (qp)
register struct enPacket   *qp;
{
    char *ctime();
    char *ctp;
    struct timeval *tvp = &(qp->enP_Stamp.ens_tstamp);

    printf (" %8x", clear (qp -> enP_Link.F));
    printf (" %8x", clear (qp -> enP_Link.B));
    printf (" %5d", qp -> enP_ByteCount);
    printf (" %5d", qp -> enP_RefCount);
    pflags( qp -> enP_Stamp.ens_flags, 6);
    printf (" %4d", qp -> enP_Stamp.ens_dropped);
    
    while (tvp->tv_usec >= 1000000) {
	tvp->tv_sec++;
	tvp->tv_usec -= 1000000;
    }
    ctp = ctime(&(tvp->tv_sec));
    ctp += 11;	/* skip day and date */
    ctp[8] = 0;	/* no timezone or year */
    printf (" %s.%06d", ctp, qp->enP_Stamp.ens_tstamp.tv_usec);
    
    printf ("\n");

}

PrintAddress(a, l)
unsigned char *a;
int l;
{
	if (l == 1) {	/* one-byte addresses in octal */
	    if (*a)
	    	printf("0");
	    printf("%o", *a);
	    return;
	}
	while (l-- > 0) {
	    printf("%02x", *a++);
	    if (l > 0)
	    	printf(":");
	}
}

PrintIFP(ifp)
struct ifnet *ifp;
{
	struct ifnet ifnet;
	char name[16];
	
	printf("\tInterface: ");

	if (ifp == 0) {
	    printf("[ifp == 0?]");
	    return;
	}
	
	klseek(fc, (long) ifp, 0);
	read(fc, &ifnet, sizeof(struct ifnet));
	
	klseek(fc, (long)ifnet.if_name, 0);
	read(fc, name, sizeof(name));
	name[15] = 0;
	
	printf("%s%d", name, ifnet.if_unit);
	
	if ((ifnet.if_flags&IFF_UP) == 0)
	    printf(" not up");
}

/*
 * Stolen from ps as is
 */
klseek(fd, loc, off)
	int fd;
	long loc;
	int off;
{

	/*
	 * If this is a dump, then the kernel isn't doing the page
	 * mapping for us.  Simulate it.
	 */
	if (kflg && (loc & 0x80000000) != 0) {
		long v;
		long addr;
		struct pte pte;

		loc &= 0x7fffffff;
		v = btop(loc);
 		if(v >= nl[X_SYSSIZE].n_value) {
 			printf("address botch %x\n", loc);
 			return;
 		}
 		addr = (long)((struct pte *)nl[X_SYSMAP].n_value + v);
 		lseek(fd, addr&0x7fffffff, 0);
 		if(read(fd, (char *)&pte, sizeof(pte)) != sizeof(pte)) {
 			printf("Error reading kmem for pte at %x\n", addr);
 			return;
 		}
 		if (pte.pg_v == 0 && (pte.pg_fod || pte.pg_pfnum == 0)) {
 			printf("pte bad for %x\n", addr);
 			return;
 		}
 		loc = (long)ptob(pte.pg_pfnum) + (loc & PGOFSET);
 	}
	(void) lseek(fd, (long)loc, off);
}

struct BSEnt {
	unsigned long Bit;
	char Char;
};

struct BSEnt ModeStrings[] = {
	{ ENHOLDSIG, 'H' },
	{ ENBATCH, 'B' },
	{ ENTSTAMP, 'T' },
	{ ENPROMISC, 'P' },
	{ ENNONEXCL, 'N' },
	{ ENCOPYALL, 'C' },
};

pmode(m, cols)
short m;
int cols;
{
	pbits(m, cols, ModeStrings,
		(sizeof(ModeStrings)/sizeof(struct BSEnt)));
}

struct BSEnt FlagStrings[] = {
	{ ENSF_PROMISC, 'P' },
	{ ENSF_BROADCAST, 'B' },
	{ ENSF_TRAILER, 'T' },
	{ ENSF_MULTICAST, 'M' },
};

pflags(f, cols)
u_short f;
int cols;
{
	pbits(f, cols, FlagStrings,
		(sizeof(FlagStrings)/sizeof(struct BSEnt)));
}

pbits(b, cols, table, nelts)
unsigned long b;
int cols;
struct BSEnt *table;
int nelts;
{
	char bstring[33];
	char *bsp = bstring;
	int i;
	int colsused = 0;
	int precols;
	
	for (i = 0; i < nelts; i++) {
	    if (b & table[i].Bit) {
		colsused++;
		*bsp++ = table[i].Char;
		b &= ~table[i].Bit;
	    }
	}
	if (b) {
	    colsused++;
	    *bsp++ = '?';
	}
	*bsp = 0;
	
	if (colsused > cols) {
	    printf(bstring);
	}
	else {
	    precols = (cols - colsused)/2;
	    cols = (cols - colsused) - precols;

	    printf(" %*s%s%*s", precols, "", bstring, cols, "");
	}
}
