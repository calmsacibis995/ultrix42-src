/*
 *	@(#)if_scs_data.c	4.2	(ULTRIX)	11/14/90
 */

/************************************************************************
 *									*
 *                      Copyright (c) 1984 - 1989 by                    *
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
/* Modification History:
 *
 *	28-Aug-1990	Larry Cohen
 *		stop using NBPG define.  use 512 instead so that mips and
 *		vax have the same mtu size.  Also because the mips page
 *		is so large that the mtu calculation does not fit in a short.
 *
 *	14-May-1989	Todd M. Katz		TMK0001
 *		1. Include header file ../vaxmsi/msisysap.h.
 *		2. Use the ../machine link to refer to machine specific header
 *		   files.
 *
 *	4-Apr-1989 - Larry Cohen
 *		Add smp support.  
 *
 *	22-July-88 - Created by Larry Cohen
 */

#include "../h/types.h"
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/ksched.h"
#include "../h/time.h"
#include "../h/errlog.h"
#include "../h/mbuf.h"
#include "../h/errno.h"
#include "../h/ioctl.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/vmmac.h"
#include "../h/buf.h"
#include "../h/kernel.h"
#include "../h/kmalloc.h"
#include "../h/proc.h"

#include "../machine/cpu.h"
#ifdef vax
#include "../machine/mtpr.h"
#endif vax
#include "../machine/pte.h"

#include "../io/scs/sca.h"
#include "../io/scs/scaparam.h"
#include "../io/scs/scamachmac.h"
#include "../io/ci/cippdsysap.h"
#include "../io/ci/cisysap.h"
#include "../io/msi/msisysap.h"
#include "../io/bi/bvpsysap.h"
#include "../io/gvp/gvpsysap.h"
#include "../io/uba/uqsysap.h"
#include "../net/net/if.h"
#include "../net/net/netisr.h"
#include "../net/net/route.h"
#include "../net/netinet/in.h"
#include "../net/netinet/in_var.h"
#include "../net/netinet/in_systm.h"
#include "../net/netinet/ip.h"
#include "../net/netinet/ip_var.h"
#include "../net/netinet/if_ether.h"
#include "../io/sysap/sysap.h"
#include "../io/sysap/if_scs.h"

/* SCSNET_MAXHOSTS is defined in ../io/scs/scaparam.h */

/*
 * IMPORTANT:  in ../machine/spt.s enough ptes are currently allocated for
 * 32 hosts with the following transfer limits.  
 * IF SCSNET_MAXHOSTS  or any of the transfer limits increases then 
 * scsmempt should also be increased.
 */

/* block transfer limits */
#define SCSNET_XFERS 5		/* max simultaneous block xfers per site */
#define SCSNET_XFER_SIZ 16	/* max block xfer size = 17*512 */
#define SCSNET_XMITS  (SCSNET_MAXHOSTS * SCSNET_XFERS)  /* total transmits */
#define SCSNET_RECVS SCSNET_XMITS /* we can recv. everything sent to us */
#define SCSNET_XPTES (SCSNET_XMITS * SCSNET_XFER_SIZ)

#define SCSNET_BLOCK_SIZE (SCSNET_XFER_SIZ * 512)   /* max block xfer size */

#define SCSNET_TIMEO	(20*hz)	/* watch dog timer interval (secs) */

#define SCSNET_MAXRETRIES 100  /* number of times to retry a connect
			        * if remote sysap is busy 
				*/

#define SCSNET_MIN_MSG_SIZ 256  /* scs messages must be at least this size
				 *   to handle protocol headers
				 */ 


#ifdef BINARY
extern struct scsnet_cntl  scsnet_xfers[];  /* xmit and recv control block */
extern struct scsnet_cntl  scsnet_recvs[];

extern int scsnet_maxretries;
extern struct _netsystem netsystems[];
/* 
 * knownsystems is the list of systems we have successfully connected to.
 * knownsystems is indexed off of the internet host number.
 */
extern struct _netsystem *knownsystems[];
extern int scsnet_min_msg_siz;
extern int scsnet_block_size;

#else BINARY

struct scsnet_cntl  scsnet_xfers[SCSNET_XMITS];
struct scsnet_cntl  scsnet_recvs[SCSNET_RECVS];  

int scsnet_maxretries = SCSNET_MAXRETRIES;
struct _netsystem  netsystems[SCSNET_MAXHOSTS];
struct _netsystem *knownsystems[SCSNET_MAXHOSTS];
int scsnet_min_msg_siz=SCSNET_MIN_MSG_SIZ;
int scsnet_block_size = SCSNET_BLOCK_SIZE;

#endif BINARY

