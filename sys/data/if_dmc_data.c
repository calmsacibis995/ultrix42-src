/*
 * @(#)if_dmc_data.c	4.1	ULTRIX	7/2/90
 */

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
/*
 * 05-Feb-89 -- R. Bhanukitsiri
 *		Reflect V3.2 source pool changes.
 *	
 * 04-jan-86 -- ejf  Fixed DECnet error counters bug.  Counters were zeroed
 *			whenever line cycled.
 *
 * 18-apr-86 -- ejf  added DECnet support.
 *
 * 04-feb-86 -- jaw  get rid of biic.h.
 *
 * 11-jul-85 -- jaw
 *	fix bua/bda map registers.
 */


#include "dmc.h"

#include "../machine/pte.h"
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
#include "../io/netif/if_dmc.h"
#include "../io/uba/ubareg.h"
#include "../io/uba/ubavar.h"

#include "../h/time.h"
#include "../h/kernel.h"


struct  dmc_command {
	char	qp_cmd;		/* command */
	short	qp_ubaddr;	/* buffer address */
	short	qp_cc;		/* character cound || XMEM */
	struct dmc_command *qp_next;	/* next command on queue */
};

/*
 * The dmcuba structures generalize the ifuba structure
 * to an arbitrary number of recieve and transmit buffers.
 */

struct	ifxmt {
	struct ifrw x_ifrw;		/* mapping imfo */
	struct pte x_map[IF_MAXNUBAMR];	/* output base pages */
	short  x_xswapd;		/* mask of clusters swapped */
	struct mbuf *x_xtofree;		/* pages being dma'd out */
};
struct  dmcuba {
        short   ifu_uban;               /* uba number */
        short   ifu_hlen;               /* local net header length */
	struct	uba_regs *ifu_uba;	/* uba regs, in vm */
        struct  ifrw ifu_r[NRCV];       /* receive information */
        struct  ifxmt ifu_w[NXMT];      /* transmit information */
                                /* these should only be pointers */
        short   ifu_flags;              /* used during uballoc's */
};

struct dmcbufs {
        int     ubinfo;                 /* from uballoc */
        short   cc;                     /* buffer size */
        short   flags;                  /* access control */
#define DBUF_OURS       0               /* buffer is available */
#define DBUF_DMCS       1               /* buffer claimed by somebody */
#define DBUF_XMIT       4               /* transmit buffer */
#define DBUF_RCV        8               /* recieve buffer */
};

struct	dmcbufres {
	u_short nrcv;			/* number of outstanding rx buffers */
	u_short nxmt;			/* number of outstanding tx buffers */
	u_short ntot;			/* total number of outstanding bufrs */
	u_short ncmds;			/* number of command buffers */
	u_short dmrdev:1;		/* set if device is dmr */
};

#ifdef	BINARY

struct  uba_device *dmcinfo[];

extern struct dmc_softc {
        short   sc_oused;               /* output buffers currently in use */
        short   sc_iused;               /* input buffers given to DMC */
        short   sc_flag;                /* flags */
	int	sc_nticks;		/* seconds since last interrupt */
	long	sc_ztime;		/* seconds since ctrs last zeroed */
	u_char	sc_basectrs[DMCZ_SIZE];	/* zero base for dmc error counters */
	u_char	sc_errctrs[DMCZ_SIZE];	/* dmc error counters */
	long	sc_rxtxctrs[CTRZ_SIZE];	/* tx and rx counters */
        struct  ifnet sc_if;            /* network-visible interface */
	struct	ifstate sc_dmccs;	/* current state of dmc */
	struct	dmcbufres sc_bufres;	/* buffer resources */
        struct  dmcbufs sc_rbufs[NRCV]; /* recieve buffer info */
        struct  dmcbufs sc_xbufs[NXMT]; /* transmit buffer info */
        struct  dmcuba sc_ifuba;        /* UNIBUS resources */
        int     sc_ubinfo;              /* UBA mapping info for base table */
	int	sc_errors[4];		/* non-fatal error counters */
#	define sc_datck	sc_errors[0]
#	define sc_timeo sc_errors[1]
#	define sc_nobuf sc_errors[2]
#	define sc_disc  sc_errors[3]
	/* command queue stuff */
	struct	dmc_command sc_cmdbuf[NCMDS];
	struct	dmc_command *sc_qhead;	/* head of command queue */
	struct	dmc_command *sc_qtail;	/* tail of command queue */
	struct	dmc_command *sc_qactive;	/* command in progress */
	struct	dmc_command *sc_qfreeh;	/* head of list of free cmd buffers */
	struct	dmc_command *sc_qfreet;	/* tail of list of free cmd buffers */
	/* end command queue stuff */
} dmc_softc[];

extern struct dmc_base {
        short   d_base[128];            /* DMC base table */
} dmc_base[];

extern int nNDMC;

#else	BINARY

struct  uba_device *dmcinfo[NDMC];

struct dmc_softc {
        short   sc_oused;               /* output buffers currently in use */
        short   sc_iused;               /* input buffers given to DMC */
        short   sc_flag;                /* flags */
	int	sc_nticks;		/* seconds since last interrupt */
	long	sc_ztime;		/* seconds since ctrs last zeroed */
	u_char	sc_basectrs[DMCZ_SIZE];	/* zero base for dmc error counters */
	u_char	sc_errctrs[DMCZ_SIZE];	/* dmc error counters */
	long	sc_rxtxctrs[CTRZ_SIZE];	/* tx and rx counters */
        struct  ifnet sc_if;            /* network-visible interface */
	struct	ifstate sc_dmccs;	/* current state of dmc */
	struct	dmcbufres sc_bufres;	/* buffer resources */
        struct  dmcbufs sc_rbufs[NRCV]; /* recieve buffer info */
        struct  dmcbufs sc_xbufs[NXMT]; /* transmit buffer info */
        struct  dmcuba sc_ifuba;        /* UNIBUS resources */
        int     sc_ubinfo;              /* UBA mapping info for base table */
	int	sc_errors[4];		/* non-fatal error counters */
#	define sc_datck	sc_errors[0]
#	define sc_timeo sc_errors[1]
#	define sc_nobuf sc_errors[2]
#	define sc_disc  sc_errors[3]
	/* command queue stuff */
	struct	dmc_command sc_cmdbuf[NCMDS];
	struct	dmc_command *sc_qhead;	/* head of command queue */
	struct	dmc_command *sc_qtail;	/* tail of command queue */
	struct	dmc_command *sc_qactive;	/* command in progress */
	struct	dmc_command *sc_qfreeh;	/* head of list of free cmd buffers */
	struct	dmc_command *sc_qfreet;	/* tail of list of free cmd buffers */
	/* end command queue stuff */
} dmc_softc[NDMC];

struct dmc_base {
        short   d_base[128];            /* DMC base table */
} dmc_base[NDMC];

int nNDMC = NDMC;

#endif	BINARY
