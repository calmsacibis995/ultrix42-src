/*
 * @(#)if_dmv_data.c	4.2	(ULTRIX)	9/4/90 
 */

/************************************************************************
 *									*
 *			Copyright (c) 1986 by				*
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
 * 05-Feb-89 -- R. Bhanukitsiri
 *		Reflect V3.2 source pool changes.
 *	
 * 28-jan-86 -- ejf  report threshold errors only once.
 *
 * 18-apr-86 -- ejf  cloned from if_dmc_data.c
 */

#include "dmv.h"

#include "../machine/pte.h"
#include "../machine/psl.h"

#include "../machine/psl.h"
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mbuf.h"
#include "../h/buf.h"
#include "../h/ioctl.h"                 /* must precede tty.h */
#include "../h/tty.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/vmmac.h"
#include "../h/errno.h"

#include "../net/net/if.h"
#include "../net/net/netisr.h"
#include "../net/net/route.h"
#include "../net/netinet/in.h"
#include "../net/netinet/in_systm.h"
#include "../net/netinet/ip.h"
#include "../net/netinet/ip_var.h"

#include "../machine/cpu.h"
#include "../machine/mtpr.h"
#include "../io/netif/if_uba.h"
#include "../io/netif/if_dmv.h"
#include "../io/uba/ubareg.h"
#include "../io/uba/ubavar.h"

#include "../h/time.h"
#include "../h/kernel.h"


struct dmv_cntdata {
	u_char cntlo;			/* counter in bsel 4 */
	u_char cnthi;			/* counter in bsel 5 */
};

struct  dmv_command {
	char	qp_cmd:4,		/* command & 22 bit mode */
		qp_maint:1,		/* request maintenance mode */
		qp_diag:1,		/* run diagnostics - not used */
		qp_boot:1,		/* boot - not used */
		qp_cmd_unused:1;
	char	qp_trib;	/* tributary address */
	union {
		u_short	qp_qbaddrl;	/* low order buffer address */
		u_short qp_ctldata;	/* data field for control command */
		struct dmv_cntdata qp_cntdata;	/* counters from control response */
	} qp_sel4;
	union {
		u_short qp_qbaddrh;	/* high order buffer address */
		u_short qp_mode_field;	/* mode function commands */
		u_short qp_ctl_field;	/* control command field */
	} qp_sel6;
	short	qp_cc;		/* character count || XMEM */
	struct dmv_command *qp_next;	/* next command on queue */
};
#define qp_lowaddr	qp_sel4.qp_qbaddrl
#define qp_data		qp_sel4.qp_ctldata
#define qp_cntlo	qp_sel4.qp_cntdata.cntlo
#define qp_cnthi	qp_sel4.qp_cntdata.cnthi

#define qp_hiaddr	qp_sel6.qp_qbaddrh
#define HIADDR_MASK	0x3f
#define qp_mode		qp_sel6.qp_mode_field
#define	MODE_MASK	0x7
#define qp_ctl		qp_sel6.qp_ctl_field
#define CTL_MASK	0x33ff
#define KEY_MASK	0x1f
#define EVENT_MASK	0xff

#define CC_MASK	0x3fff


/*
 * The dmvuba structures generalize the ifuba structure
 * to an arbitrary number of recieve and transmit buffers.
 */

struct	ifxmt {
	struct ifrw x_ifrw;		/* mapping imfo */
	struct pte x_map[IF_MAXNUBAMR];	/* output base pages */
	short  x_xswapd;		/* mask of clusters swapped */
	struct mbuf *x_xtofree;		/* pages being dma'd out */
};
struct  dmvuba {
        short   ifu_uban;               /* uba number */
        short   ifu_hlen;               /* local net header length */
	struct	uba_regs *ifu_uba;	/* uba regs, in vm */
        struct  ifrw ifu_r[NRCV];       /* receive information */
        struct  ifxmt ifu_w[NXMT];      /* transmit information */
                                /* these should only be pointers */
        short   ifu_flags;              /* used during uballoc's */
};

struct dmvbufs {
        int     ubinfo;                 /* from uballoc */
        short   cc;                     /* buffer size */
        short   flags;                  /* access control */
#define DBUF_OURS       0               /* buffer is available */
#define DBUF_DMVS       1               /* buffer claimed by somebody */
#define DBUF_XMIT       4               /* transmit buffer */
#define DBUF_RCV        8               /* recieve buffer */
};

struct	dmvbufres {
	char	*buffers;		/* buffers for packets */
	u_short nrcv;			/* number of outstanding rx buffers */
	u_short nxmt;			/* number of outstanding tx buffers */
	u_short ntot;			/* total number of outstanding bufrs */
	u_short ncmds;			/* number of command buffers */
	u_short dmrdev:1;		/* set if device is dmr */
};


#ifdef	BINARY

struct  uba_device *dmvinfo[];

extern u_char	dmv_instack[NDMV];

extern struct dmv_softc {
        short   sc_oused;               /* output buffers currently in use */
        short   sc_iused;               /* input buffers given to DMV */
        short   sc_flag;                /* flags */
	int	sc_nticks;		/* seconds since last interrupt */
	long	sc_ztime;		/* seconds since ctrs last zeroed */
	struct	ctrreq sc_errctrs;	/* error counters from device */
	short	sc_ctrmask;		/* bit mask of fetched counters */
        struct  ifnet sc_if;            /* network-visible interface */
	struct	ifstate sc_dmvcs;	/* current state of dmv */
	struct	dmvbufres sc_bufres;	/* buffer resources */
        struct  dmvbufs sc_rbufs[NRCV]; /* recieve buffer info */
        struct  dmvbufs sc_xbufs[NXMT]; /* transmit buffer info */
        struct  dmvuba sc_ifuba;        /* UNIBUS resources */
        int     sc_ubinfo;              /* UBA mapping info for base table */
	int	sc_errors[5];		/* non-fatal error counters */
#	define sc_datck	sc_errors[0]
#	define sc_timeo sc_errors[1]
#	define sc_nobuf sc_errors[2]
#	define sc_disc  sc_errors[3]
#	define sc_dmverrmsg  sc_errors[4]
	/* command queue stuff */
	struct	dmv_command sc_cmdbuf[NCMDS];
	struct	dmv_command *sc_qhead;	/* head of command queue */
	struct	dmv_command *sc_qtail;	/* tail of command queue */
	struct	dmv_command *sc_qactive;	/* command in progress */
	struct	dmv_command *sc_qfreeh;	/* head of list of free cmd buffers */
	struct	dmv_command *sc_qfreet;	/* tail of list of free cmd buffers */
	/* end command queue stuff */
} dmv_softc[];

extern struct dmv_base {
        short   d_base[128];            /* DMV base table */
} dmv_base[];

extern int nNDMV;

#else	BINARY

struct  uba_device *dmvinfo[NDMV];

u_char	dmv_instack[NDMV];

struct dmv_softc {
        short   sc_oused;               /* output buffers currently in use */
        short   sc_iused;               /* input buffers given to DMV */
        short   sc_flag;                /* flags */
	int	sc_nticks;		/* seconds since last interrupt */
	long	sc_ztime;		/* seconds since ctrs last zeroed */
	struct	ctrreq sc_errctrs;	/* error counters from device */
	short	sc_ctrmask;		/* bit mask of fetched counters */
        struct  ifnet sc_if;            /* network-visible interface */
	struct	ifstate sc_dmvcs;	/* current state of dmv */
	struct	dmvbufres sc_bufres;	/* buffer resources */
        struct  dmvbufs sc_rbufs[NRCV]; /* recieve buffer info */
        struct  dmvbufs sc_xbufs[NXMT]; /* transmit buffer info */
        struct  dmvuba sc_ifuba;        /* UNIBUS resources */
        int     sc_ubinfo;              /* UBA mapping info for base table */
	int	sc_errors[5];		/* non-fatal error counters */
#	define sc_datck	sc_errors[0]
#	define sc_timeo sc_errors[1]
#	define sc_nobuf sc_errors[2]
#	define sc_disc  sc_errors[3]
#	define sc_dmverrmsg  sc_errors[4]
	/* command queue stuff */
	struct	dmv_command sc_cmdbuf[NCMDS];
	struct	dmv_command *sc_qhead;	/* head of command queue */
	struct	dmv_command *sc_qtail;	/* tail of command queue */
	struct	dmv_command *sc_qactive;	/* command in progress */
	struct	dmv_command *sc_qfreeh;	/* head of list of free cmd buffers */
	struct	dmv_command *sc_qfreet;	/* tail of list of free cmd buffers */
	/* end command queue stuff */
} dmv_softc[NDMV];

struct dmv_base {
        short   d_base[128];            /* DMV base table */
} dmv_base[NDMV];

int nNDMV = NDMV;

#endif	BINARY
