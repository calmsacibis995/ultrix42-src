/*
 * @(#)if_css_data.c	4.1	(ULTRIX)	7/2/90
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

#include "css.h"


/*
 * DEC/CSS IMP11-A ARPAnet IMP interface driver.
 * Since "imp11a" is such a mouthful, it is called
 * "css" after the LH/DH being called "acc".
 *
 * Configuration notes:
 *
 * As delivered from DEC/CSS, it
 * is addressed and vectored as two DR11-B's.  This makes
 * Autoconfig almost IMPOSSIBLE.  To make it work, the
 * interrupt vectors must be restrapped to make the vectors
 * consecutive.  The 020 hole between the CSR addresses is
 * tolerated, althought that could be cleaned-up also.
 *
 * Additionally, the TRANSMIT side of the IMP11-A has the
 * lower address of the two subunits, so the vector ordering
 * in the CONFIG file is reversed from most other devices.
 * It should be:
 *
 * device css0 ....  cssxint cssrint
 *
 * If you get it wrong, it will still autoconfig, but will just
 * sit there with RECIEVE IDLE indicated on the front panel.
 */

/************************************************************************
 *			Modification History				*
 *									*
 * 03-Feb-89	R. Bhanukitsiri						*
 *	Reflect V3.2 source pool changes.				*
 *									*
 ************************************************************************/

#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mbuf.h"
#include "../h/buf.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/vmmac.h"

#include "../net/net/if.h"
#include "../net/netimp/if_imp.h"

#include "../machine/cpu.h"
#include "../machine/mtpr.h"
#include "../io/netif/if_cssreg.h"
#include "../io/netif/if_uba.h"
#include "../io/uba/ubareg.h"
#include "../io/uba/ubavar.h"

/*
 * "Lower half" of IMP interface driver.
 *
 * Each IMP interface is handled by a common module which handles
 * the IMP-host protocol and a hardware driver which manages the
 * hardware specific details of talking with the IMP.
 *
 * The hardware portion of the IMP driver handles DMA and related
 * management of UNIBUS resources.  The IMP protocol module interprets
 * contents of these messages and "controls" the actions of the
 * hardware module during IMP resets, but not, for instance, during
 * UNIBUS resets.
 *
 * The two modules are coupled at "attach time", and ever after,
 * through the imp interface structure.  Higher level protocols,
 * e.g. IP, interact with the IMP driver, rather than the CSS.
 */

#ifdef	BINARY


extern	struct  css_softc {
        struct  ifnet *css_if;          /* pointer to IMP's ifnet struct */
        struct  impcb *css_ic;          /* data structure shared with IMP */
        struct  ifuba css_ifuba;        /* UNIBUS resources */
        struct  mbuf *css_iq;           /* input reassembly queue */
        short   css_olen;               /* size of last message sent */
        char    css_flush;              /* flush remainder of message */
} css_softc[];

extern	struct  uba_device *cssinfo[];
extern	int	nNCSS;
#else

struct  css_softc {
        struct  ifnet *css_if;          /* pointer to IMP's ifnet struct */
        struct  impcb *css_ic;          /* data structure shared with IMP */
        struct  ifuba css_ifuba;        /* UNIBUS resources */
        struct  mbuf *css_iq;           /* input reassembly queue */
        short   css_olen;               /* size of last message sent */
        char    css_flush;              /* flush remainder of message */
} css_softc[NCSS];

struct  uba_device *cssinfo[NCSS];

int	nNCSS = NCSS;
#endif
