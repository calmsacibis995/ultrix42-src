#ifndef	lint
static char *sccsid = "@(#)dli_proto.c	4.3	ULTRIX	11/14/90";
#endif	lint

/*
 * Program dli_proto.c,  Module DLI 
 *
 * Copyright (C) 1985 by
 * Digital Equipment Corporation, Maynard, Mass.
 *
 * This software is furnished under a license and may be used and copied
 * only  in  accordance  with  the  terms  of such  license and with the
 * inclusion of the above copyright notice. This software or  any  other
 * copies thereof may not be provided or otherwise made available to any
 * other person. No title to and ownership of  the  software  is  hereby
 * transferred.
 *
 * The information in this software is subject to change without  notice
 * and  should  not be  construed  as  a commitment by Digital Equipment
 * Corporation.
 *
 * Digital assumes no responsibility for the use or  reliability  of its
 * software on equipment which is not supplied by Digital.
 *
 *
 * Networks & Communications Software Engineering
 *
 * IDENT HISTORY:
 *
 * 1.00 10-Jul-1985
 *      DECnet-ULTRIX   V1.0
 *
 * 2.00 18-Apr-1986
 *		DECnet-Ultrix	V2.0
 *
 * 5.00 09-Oct-1989
 *		DECnet-Ultrix	V5.0
 *
 * Added sysid and point-to-point support
 *
 */


#include "../../h/param.h"
#include "../../h/systm.h"
#include "../../h/socket.h"
#include "../../h/protosw.h"
#include "../../h/domain.h"
#include "../../h/mbuf.h"
#include "../../h/smp_lock.h"
#include "../../h/cpudata.h"

#include "../../net/net/if.h"

#include "../../net/netinet/in.h"
#include "../../net/netinet/if_ether.h"

#include "../../net/netdnet/dli_var.h"
#include "dli.h"


/*
 * Definitions of protocols supported in the DLI domain.
 */
extern int dli_usrreq(),dli_init(),dli_ifoutput(),dli_ifinput(),dli_ifioctl(),dli_slowtimo(), dli_ifstate(), dli_ctloutput();

#ifdef WDD
extern int dm_usrreq(),   dm_init(),         dm_ctloutput();

extern int wda_usrreq(),  wda_ifinput(),     wda_ctloutput();
extern int wda_ifstate(), wda_input(),       wda_init();

#ifdef IPH
extern int iph_init(),    iph_ctlinput(),    iph_input();
#endif IPH

#if defined(WDD_DECHDLC) || defined(WDD_HDLC) || defined(WDD_HDLC_NOXID)
extern int hdlc_init(),   hdlc_ctloutput(),  hdlc_usrreq(),  hdlc_input();
extern int hdlc_output(), hdlc_ifinput(),    hdlc_ifoutput(),hdlc_ifioctl();

extern int hdlcm_init(),  hdlcm_ctloutput(), hdlcm_usrreq(), hdlcm_output();
#endif WDD_DECHDLC

#if defined(WDD_LAPB)
extern int lapb_init(),   lapb_ctloutput(),  lapb_usrreq(),  lapb_input();
extern int hdlc_output(), lapb_ifinput(),    lapb_ifioctl();

extern int lapbm_init(),  lapbm_ctloutput(), lapbm_usrreq();
#endif WDD_LAPB

#if defined(WDD_LLC2)
extern int llc2_init(),   llc2_ctloutput(),  llc2_usrreq(),  llc2_input();
extern int llc2_output(), llc2_ifinput(),    llc2_ifoutput(),llc2_ifioctl();

extern int llc2m_init(),  llc2m_ctloutput(), llc2m_usrreq();
#endif WDD_LLC2
#endif WDD

#ifdef OSI
extern int csmacd_init(),csmacd_ctloutput(),csmacd_input(),csmacd_output(),
	   csmacd_slowtimo();
extern int fddi_init(),fddi_ctloutput(),fddi_input(),fddi_output(),
	   fddi_slowtimo();
extern int dloop_init(),dloop_ctloutput(),dloop_input(),dloop_output();
extern int ddcmp_init(),ddcmp_ctloutput(),ddcmp_input(),ddcmp_output(),ddcmp_ifioctl(),ddcmp_ifstate();
#endif

u_short dli_ifq_maxlen = 32;
#if NDLI==0
u_int dli_maxline = 1;
struct dli_line dli_ltable[1]; 		/* DLI line table */
struct dli_line *dli_ltableptrs[1];	/* pointers to each lte */
#else
u_int dli_maxline = NDLI * 16;
struct dli_line dli_ltable[NDLI * 16]; 		/* DLI line table */
struct dli_line *dli_ltableptrs[NDLI * 16];	/* pointers to each lte */
#endif

struct protosw dlisw[] =
{
    { SOCK_DGRAM,     &dlidomain,       DLPROTO_DLI,     PR_ATOMIC | PR_ADDR,
      0,              0,       		0,               dli_ctloutput,
      dli_usrreq,     dli_init,         0,               dli_slowtimo,
      0,              dli_ifoutput,     dli_ifinput,     dli_ifioctl,
      dli_ifstate
    },
#ifdef OSI
    { SOCK_DGRAM,     &dlidomain,       DLPROTO_CSMACD,  PR_ATOMIC | PR_ADDR,
      csmacd_input,   csmacd_output,    0,               csmacd_ctloutput,
      0,              csmacd_init,      0,               csmacd_slowtimo,
      0,              0,                0,               0,
      0
    },
    { SOCK_DGRAM,     &dlidomain,       DLPROTO_FDDI,    PR_ATOMIC | PR_ADDR,
      fddi_input,     fddi_output,      0,               fddi_ctloutput,
      0,              fddi_init,        0,               fddi_slowtimo,
      0,              0,                0,               0,
      0
    },
    { SOCK_DGRAM,     &dlidomain,       DLPROTO_LOOP,	 PR_ATOMIC | PR_ADDR,
      0,              dloop_output,     0,               dloop_ctloutput,
      0,              dloop_init,       0,               0,
      0,              0,                0,               0,
      0
    },
    { SOCK_DGRAM,     &dlidomain,       DLPROTO_DDCMP,	PR_ATOMIC | PR_ADDR,
      ddcmp_input,    ddcmp_output,     0, 		ddcmp_ctloutput,
      0,              ddcmp_init,       0,              0,
      0,              0,                0,    		ddcmp_ifioctl,
      ddcmp_ifstate
    },
#endif
#ifdef WDD
#if defined(WDD_DECHDLC) || defined(WDD_HDLC) || defined(WDD_HDLC_NOXID)
    { SOCK_SEQPACKET, &dlidomain,       DLPROTO_HDLC,    PR_ATOMIC       |
                                                         PR_CONNREQUIRED |
                                                         PR_WANTRCVD,
      hdlc_input,     hdlc_output,      0,               hdlc_ctloutput,
      hdlc_usrreq,    hdlc_init,        0,               0,
      0,              hdlc_ifoutput,    hdlc_ifinput,    hdlc_ifioctl,
      0
    },
#endif
#if defined(WDD_LLC2)
    { SOCK_SEQPACKET, &dlidomain,       DLPROTO_LLC2,    PR_ATOMIC       |
                                                         PR_CONNREQUIRED |
                                                         PR_WANTRCVD,
      llc2_input,     llc2_output,      0,               llc2_ctloutput,
      llc2_usrreq,    llc2_init,        0,               0,
      0,              0,    		llc2_ifinput,    llc2_ifioctl,
      0
    },
#endif WDD_LLC2
#if defined(WDD_LAPB)
    { SOCK_SEQPACKET, &dlidomain,       DLPROTO_LAPB,    PR_ATOMIC       |
                                                         PR_CONNREQUIRED |
                                                         PR_WANTRCVD,
      lapb_input,     hdlc_output,      0,               lapb_ctloutput,
      lapb_usrreq,    lapb_init,        0,               0,
      0,              0,    		lapb_ifinput,    lapb_ifioctl,
      0
    },
#endif WDD_LAPB
    { SOCK_DGRAM,     &dlidomain,       DLPROTO_WDA,     PR_ATOMIC | PR_ADDR,
      wda_input,      0, 		0,               wda_ctloutput,
      wda_usrreq,     wda_init,         0,               0,
      0,              0,                wda_ifinput,     0,
      wda_ifstate
    },
#ifdef IPH
      {SOCK_DGRAM,    &dlidomain,       DLPROTO_IPH,    PR_ATOMIC | PR_ADDR,
      iph_input,      0,                iph_ctlinput,   0,
      0,              iph_init,         0,              0,
      0,              0,                0,    		0,
      0
    },
#endif IPH
    { SOCK_STREAM,    &dlidomain,       DLPROTO_DM,      PR_ATOMIC,
      0,              0,                0,               dm_ctloutput,
      dm_usrreq,      dm_init,          0,               0,
      0,              0,                0,               0,
      0
    },
#if defined(WDD_DECHDLC) || defined(WDD_HDLC) || defined(WDD_HDLC_NOXID)
    { SOCK_STREAM,    &dlidomain,       DLPROTO_HDLCM,   PR_ATOMIC,
      0,              0,                0,               hdlcm_ctloutput,
      hdlcm_usrreq,   hdlcm_init,       0,               0,
      0,              0,                0,               0,
      0
    },
#endif WDD_DECHDLC
#if defined(WDD_LAPB)
    { SOCK_SEQPACKET, &dlidomain,       DLPROTO_LAPBM,   PR_ATOMIC,
      0,              0,                0,               lapbm_ctloutput,
      lapbm_usrreq,   lapbm_init,       0,               0,
      0,              0,                0,               0,
      0
    },
#endif WDD_LAPB
#if defined(WDD_LLC2)
    { SOCK_SEQPACKET, &dlidomain,       DLPROTO_LLC2M,   PR_ATOMIC,
      0,              0,                0,               llc2m_ctloutput,
      llc2m_usrreq,   llc2m_init,       0,               0,
      0,              0,                0,               0,
      0
    },
#endif WDD_LLC2
#endif WDD
};

struct domain dlidomain =
    { AF_DLI,  "DLI", 0, 0, 0, dlisw, &dlisw[sizeof(dlisw)/sizeof(dlisw[0])] };

