/*
 * @(#)dc_data.c	4.1	(ULTRIX)	8/9/90
 */

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
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/*
 * Modification History: dc_data.c
 *
 * 04-Jul-90	Randall Brown
 *	Created file.
 */

#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/ioctl.h"
#include "../h/tty.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/map.h"
#include "../h/buf.h"
#include "../h/vm.h"
#include "../h/conf.h"
#include "../h/file.h"
#include "../h/uio.h"
#include "../h/kernel.h"
#include "../h/devio.h"
#include "../../machine/common/cpuconf.h"
#include "../h/exec.h"
#include "../h/kmalloc.h"
#include "../h/sys_tpath.h"
#include "../io/uba/ubavar.h"	/* auto-config headers */

#include "../machine/cpu.h"
#include "../io/tc/dc7085reg.h"
#include "../io/tc/slu.h"
#include "../io/tc/vsxxx.h"
#include "../io/tc/xcons.h"

#include "dc.h"

#ifdef BINARY

extern struct	tty	dc_tty[];	/* tty structure		*/
extern struct	slu	slu;		/* serial line communcation function pointers */

extern	u_char	dcmodem[];
extern 	u_char	dcmodem_active[];
extern	int	dc_modem_line[];
extern	struct	timeval	dctimestamp[];

extern 	u_short	dc_brk[];
extern 	int	dc_modem_ctl;

extern	char	dcsoftCAR[];
extern	char	dcdefaultCAR[];
extern	int	nNDCLINE;
extern	int	dc_cnt;
extern	int	brk_start[], brk_stop[];

extern	struct 	uba_device *dcinfo[];

extern	struct 	dc_softc dc_softc[];

/*
 * definitions of the modem status register and transmit control register
 *
 * The bit placement of these registers are different from PMAX to 3MAX
 * these variables get set to where the appropriate signal is in the
 * register.
 */
extern	short	dc_rdtr[], dc_rrts[], dc_rcd[];
extern	short	dc_rdsr[], dc_rcts[], dc_xmit[];

#else 

struct	tty dc_tty[NDC * NDCLINE];	/* tty structure		*/
struct	slu	slu;			/* Serial Line communication function pointers */

u_char	dcmodem[NDC * NDCLINE];		/* keeps track of modem state */
u_char 	dcmodem_active[NDC] = 0;
int	dc_modem_line[NDC * NDCLINE];
struct	timeval dctimestamp[NDC * NDCLINE];
u_short	dc_brk[NDC];
int	dc_modem_ctl;	/* holds whether we use full or limited modem control */

char	dcsoftCAR[NDC];
char	dcdefaultCAR[NDC];
int	nNDCLINE = NDC * NDCLINE;
int	dc_cnt = NDC * NDCLINE;
int	brk_start[NDC * NDCLINE], brk_stop[NDC * NDCLINE];

struct uba_device *dcinfo[NDC];

struct dc_softc dc_softc[NDC];

/*
 * definitions of the modem status register and transmit control register
 *
 * The bit placement of these registers are different from PMAX to 3MAX
 * these variables get set to where the appropriate signal is in the
 * register.
 */
short	dc_rdtr[NDC * NDCLINE], dc_rrts[NDC *NDCLINE], dc_rcd[NDC * NDCLINE];
short	dc_rdsr[NDC * NDCLINE], dc_rcts[NDC * NDCLINE], dc_xmit[NDC * NDCLINE];

#endif




