/*
 * static  char    *sccsid = "@(#)pfilt.h	4.3  (ULTRIX)	4/4/91";
 */
/************************************************************************
 *									*
 *			Copyright (c) 1988, 1989 by			*
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
 *   University	of   California,   Berkeley,   and   from   Bell	*
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

/* ---------------------------------------------------------------------
 * Common code for processing received ethernet packets
 * Derived from:
 *      enet.h	Stanford	25 April 1983
 */
/*
 *  Ethernet definitions needed for user processes
 *
 **********************************************************************
 * HISTORY
 *
 * $Log:	pfilt.h,v $
 * Revision 1.2  90/05/22  15:35:55  mogul
 * Added EIOCALLOWCOPYALL ioctl and ENCOPYALL mode bit.
 * 
 * Revision 1.1  90/05/18  17:31:15  mogul
 * Initial revision
 * 
 * Revision 1.5  89/08/09  11:04:17  mogul
 * Added definition for ENSF_MULTICAST
 * 
 * Revision 1.4  89/07/31  18:04:32  mogul
 * Moved definition of generic prefix from enet.c to enet.h
 * for use by outside programs.
 * 
 * Revision 1.3  89/07/31  17:57:56  mogul
 * EIOCALLOWPROMISC and EIOCMAXBACKLOG are in-out, so that we
 * can see what the setting is.
 * 
 * Revision 1.2  89/07/26  19:18:49  mogul
 * Added new ioctls: EIOCALLOWPROMISC, EIOCMAXBACKLOG,
 * 	EIOCSRTIMEOUT, EIOCGRTIMEOUT, EIOCSETIF
 * 
 * Uttam Shikarpur Nadig/Nashua,NH
 * Changed name of this header file from enet.h to pfilt.h
 *
 * Revision 1.8  87/01/26  15:04:06  mogul
 * Added EIOCTRUNCATE ioctl to support truncation of received packets.
 * 
 * Revision 1.7  86/09/16  17:10:56  mogul
 * Automatic inclusion of RCS log messages in history comments.
 * 
 *
 * 13 July 1986		Jeff Mogul	Stanford
 *	Added ENALIGN macro and ENALIGMENT constant.
 *	Added special stack actions to push common constants:
 *		1, 0xFFFF, 0x00FF, 0xFF00
 *	Added EIOCIFNAME ioctl to get interface name.
 *	Added ENNONEXCL mode for non-exclusive reception of packets.
 *	Added EIOCMFREE ioctl, returns number of free minor devices
 *	Added BATCH, TSTAMP, and PROMISC modes.
 *	Defined enstamp struct returned with packets in BATCH and TSTAMP modes.
 *
 * 7 October 1985	Jeff Mogul	Stanford
 *	Added EIOCMFREE ioctl to indicate # of free minor devices;
 *	may or may not be useful.
 * 17 October 1984	Jeff Mogul	Stanford
 *	Added ENF_CAND, ENF_COR, ENF_CNAND, and ENF_CNOR, short-circuit
 *	operators, to make filters run faster.
 *	All evaluate "(*sp++ == *sp++)":
 *	ENF_CAND: returns false immediately if result is false, otherwise
 *		continue
 *	ENF_COR: returns true immediately if result is true, otherwise
 *		continue
 *	ENF_CNAND: returns true immediately if result is false, otherwise
 *		continue
 *	ENF_CNOR: returns false immediately if result is true, otherwise
 *		continue
 *	Also added ENF_NEQ to complement ENF_EQ
 *
 * 10 November 1983	Jeffrey Mogul	Stanford
 *	Slight restructuring for support of 10mb ethers;
 *	added the EIOCDEVP ioctl and associated definitions
 *	and removed the EIOCMTU ioctl (subsumed by EIOCDEVP)
 *
 * 25-Apr-83	Jeffrey Mogul	Stanford
 *	Began conversion to 4.2BSD.  This involves removing all
 *	references to the actual hardware.
 *	Incompatible change: ioctl encodings!
 *	Added EIOCMTU ioctl to get MTU (max packet size).
 *	Most previous history comments removed.
 *	Definitions of interest only to kernel now are in enetdefs.h
 *
 * 10-Aug-82  Mike Accetta (mja) at Carnegie-Mellon University
 *	Added EIOCMBIS and EIOCMBIC definitions, and new ENHOLDSIG mode
 *	bit and ENPRIVMODES defintions (V3.05e). [Last change before
 *	4.2BSD conversion starts.]
 *
 * 22-Feb-80  Rick Rashid (rfr) at Carnegie-Mellon University
 *	Rewritten for multiple simultaneous opens with filters (V1.05).
 *
 * 18-Jan-80  Mike Accetta (mja) at Carnegie-Mellon University
 *      Created (V1.00).
 *
 **********************************************************************
 */
#ifdef KERNEL
#include "../h/ioctl.h"
#else
#include <sys/ioctl.h>
#endif	/* KERNEL */

/* implementation constant */
#define	ENALIGNMENT	(sizeof(int))	/* padding boundary for BATCH reads */
#define	ENALIGNMASK	(ENALIGNMENT-1)	/* assumes ENALIGNMENT is power of 2 */

/* This macro takes a length and pads it, if necessary */
#define	ENALIGN(x) (((x)&ENALIGNMASK)==0?x:(ENALIGNMENT+((x)&(~ENALIGNMASK))))

#define ENMAXFILTERS	40		/* maximum filter short words */

/*
 *  filter structure for SETF
 */
struct enfilter
{
    u_char  enf_Priority;		/* priority of filter */
    u_char  enf_FilterLen;		/* length of filter command list */
    u_short enf_Filter[ENMAXFILTERS];	/* the filter command list */
};

/*  set/get parameters, set filter ioctl commands  */
#define	EIOCSETP	_IOW('E',100, struct eniocb)
#define	EIOCGETP	_IOR('E',101, struct eniocb)
#define	EIOCSETF	_IOW('E',102, struct enfilter)
#define	EIOCENBS	_IOW('E',103, int)
#define	EIOCINHS	_IO('E',104)
#define	EIOCSETW	_IOW('E',105, u_int)
#define	EIOCFLUSH	_IO('E',106)
#define	EIOCALLOCP	_IO('E',107)
#define	EIOCDEALLOCP	_IO('E',108)
#define	EIOCMBIS	_IOW('E',109, u_short)
#define	EIOCMBIC	_IOW('E',110, u_short)
#define	EIOCDEVP	_IOR('E',111, struct endevp)
#define	EIOCMFREE	_IOR('E',112, int)
#define	EIOCIFNAME	_IOR('E',113, struct ifreq)
#define	EIOCTRUNCATE	_IOW('E',114, int)
#define	EIOCALLOWPROMISC	_IOWR('E',115, int)
#define	EIOCMAXBACKLOG	_IOWR('E',116, int)
#define	EIOCSRTIMEOUT	_IOW('E',117, struct timeval)
#define	EIOCGRTIMEOUT	_IOR('E',117, struct timeval)
#define	EIOCSETIF	_IOW('E',118, struct ifreq)
#define	EIOCALLOWCOPYALL	_IOWR('E',119, int)

/*
 *  Bits in mode word modified by EIOCMBIS and EIOCMBIC.
 */
#define	ENHOLDSIG	0x0001	/* don't disable signal after sending */
#define	ENBATCH		0x0002	/* group as many packets into single read */
#define	ENTSTAMP	0x0004	/* prepend packet with a header containing
				   time stamp and other stuff */
#define	ENPROMISC	0x0008	/* accept promiscuously received packets */
#define	ENNONEXCL	0x0010	/* non-exclusive reception */
#define	ENCOPYALL	0x0020	/* hear all packets addressed to this host */

#define	ENPRIVMODES	\
	(~(ENHOLDSIG|ENBATCH|ENTSTAMP|ENPROMISC|ENNONEXCL|ENCOPYALL))

/*
 *  We now allow specification of up to MAXFILTERS (short) words of a filter
 *  command list to be applied to incoming packets to determine if
 *  those packets should be given to a particular open ethernet file.
 *  
 *  Each open enet file specifies the filter command list via iocontrl.
 *  Each filter command list specifies a sequences of actions which leave a
 *  boolean value on the top of an internal stack.  Each word of the
 *  command list specifies an action from the set {PUSHLIT, PUSHZERO,
 *  PUSHWORD+N} which respectively push the next word of the stack, zero,
 *  or word N of the incoming packet on the stack, and a binary operator
 *  from the set {EQ, LT, LE, GT, GE, AND, OR, XOR} which operates on the
 *  top two elements of the stack and replaces them with its result.  The
 *  special action NOPUSH and the special operator NOP can be used to only
 *  perform the binary operation or to only push a value on the stack.
 *  
 *  If the final value of the filter operation is true, then the packet is
 *  accepted for the open file which specified the filter.
 *  
 */

/*  these must sum to 16!  */
#define	ENF_NBPA	10			/* # bits / action */
#define	ENF_NBPO	6			/* # bits / operator */

/*  binary operators  */
#define ENF_NOP	(0<<ENF_NBPA)
#define	ENF_EQ	(1<<ENF_NBPA)
#define	ENF_LT	(2<<ENF_NBPA)
#define	ENF_LE	(3<<ENF_NBPA)
#define	ENF_GT	(4<<ENF_NBPA)
#define ENF_GE	(5<<ENF_NBPA)
#define	ENF_AND	(6<<ENF_NBPA)
#define	ENF_OR	(7<<ENF_NBPA)
#define	ENF_XOR	(8<<ENF_NBPA)
#define	ENF_COR	(9<<ENF_NBPA)
#define	ENF_CAND	(10<<ENF_NBPA)
#define	ENF_CNOR	(11<<ENF_NBPA)
#define	ENF_CNAND	(12<<ENF_NBPA)
#define	ENF_NEQ		(13<<ENF_NBPA)

/*  stack actions  */
#define	ENF_NOPUSH	0
#define	ENF_PUSHLIT	1	
#define	ENF_PUSHZERO	2
#define	ENF_PUSHONE	3
#define	ENF_PUSHFFFF	4
#define	ENF_PUSH00FF	5
#define	ENF_PUSHFF00	6
#define	ENF_PUSHWORD	16

/*
 *  parameter buffer structure for GETP and SETP
 */
struct eniocb
{
    u_char en_addr;		/* ethernet address (RO) */
    u_char en_maxfilters;	/* max filter words available (RO) */
    u_char en_maxwaiting;	/* max queued input packets (RO) */
    u_char en_maxpriority;	/* max filter priority for this file (RO) */
    long   en_rtout;		/* receive timeout in (jiffies) (RW) */
};

/*
 * parameter structure for EIOCDEVP (get device parameters)
 */

#define	EN_MAX_ADDR_LEN	8	/* maximum bytes in a hardware address */

struct endevp {
	u_char	end_dev_type;	/* device type, codes below */
	u_char	end_addr_len;	/* length (bytes) of a hardware address */
	u_short	end_hdr_len;	/* length of a hardware packet header */
	u_short	end_MTU;	/* maximum packet size (bytes) */
	u_char	end_addr[EN_MAX_ADDR_LEN];
				/* hardware address for this unit */
	u_char	end_broadaddr[EN_MAX_ADDR_LEN];
				/* hardware-supported broadcast address */
};

/*
 * Stamp structure comes before each packet in BATCH or TSTAMP modes.
 * Some of the fields might not be valid for all devices.
 */
struct enstamp {
	u_short ens_stamplen;	/* number of bytes in this stamp struct */
	u_short ens_flags;	/* see below				*/
	u_short	ens_count;	/* number of bytes in packet		*/
				/*	(not counting this stamp struct) */
	u_short	ens_dropped;	/* number of packets dropped for this	*/
				/*	filter since previous packet	*/
				/*	(valid only for last-match filter) */
	u_long	ens_ifoverflows;
				/* number of packets missed by interface */
				/*	(cumulative)			*/
	struct timeval ens_tstamp;
				/* time packet was received (more or less) */
};

/* Flag bit definitions for ens_flags */
#define	ENSF_PROMISC	001	/* received promiscuously */
#define ENSF_BROADCAST	002	/* received as a broadcast */
#define	ENSF_TRAILER	004	/* packet was encapsulated in trailer format */
#define ENSF_MULTICAST	010	/* received as a multicast */

/* Ethernet Device Type codes */

#define	ENDT_3MB	3	/* Xerox Experimental Ethernet */
#define	ENDT_BS3MB	1	/* Xerox Experimental Ethernet/byteswapped */
#define	ENDT_10MB	2	/* Xerox-DEC-Intel Standard Ethernet */
#define	ENDT_FDDI	4	/* FDDI */

#define	ENDT_MIN	1	/* minimum defined device type code */
#define	ENDT_MAX	4	/* maximum defined device type code */

#define	ENGENPREFIX	"pf"	/* prefix for "generic" interface names */
