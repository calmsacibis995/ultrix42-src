/*
 * static	char	*sccsid = "@(#)sh_data.c	4.1	(ULTRIX)	7/2/90";
 */

/************************************************************************
 *									*
 *			Copyright (c) 1985, 1986, 1987 by		*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any	other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   This software is  derived	from  software	received  from	the	*
 *   University    of	California,   Berkeley,   and	from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is	subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or	reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/*
 * sh_data.c
 *
 * Modification history
 *
 * MicroVAX 2000 serial line expander data file
 *
 * 12-Jun-89 - dws
 *
 *	Added trusted path support.
 *
 * 30-May-89	darrell
 *	Added include of ../../machine/common/cpuconf.h -- cpu types
 *	were moved there.
 *
 * 11-Aug-87 - Tim Burke
 *
 *	Added exec.h to list of include files for compatibility mode check 
 *	stored in the upper 4 bits of the magic number.
 *
 * 29-Jan-87 -- tim (Tim Burke)
 *
 *	Added definition of shdsr, a variable used to define the type of 
 *	modem control that is being followed.
 *	Derived from dhu_data.c (delta 1.7).
 *
 *   2-Jul-86  -- fred (Fred Canter)
 *	Created this data file for the first pass MicroVAX 2000
 *	serial line expander (8 line SLU) driver.
 *	Derived from dhu_data.c (delta 1.6).
 *
 *   4-Dec-86  -- fred (Fred Canter)
 *	Modified this data file for the real MicroVAX 2000
 *	serial line expander (8 line SLU) driver.
 *
 */

#include	"sh.h"

#include "../machine/pte.h"

#include "bk.h"
#include "../h/param.h"
#include "../h/conf.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/ioctl.h"
#include "../h/tty.h"
#include "../h/map.h"
#include "../h/buf.h"
#include "../h/vm.h"
#include "../h/kernel.h"
#include "../h/devio.h"
#include "../h/exec.h"
#include "../h/sys_tpath.h"

#include "../machine/cpu.h"
#include "../../machine/common/cpuconf.h"
#include "../io/uba/ubareg.h"
#include "../io/uba/ubavar.h"
#include "../io/uba/shreg.h"

#include "../h/bk.h"
#include "../h/clist.h"
#include "../h/file.h"
#include "../h/uio.h"

#ifdef BINARY

extern	struct	uba_device *shinfo[];
extern	struct	sh_softc sh_softc[];
extern	short	shsoftCAR[];
extern	short	shdefaultCAR[];

extern	struct	tty sh_tty[];
extern	u_char	shmodem[];
extern	struct  timeval	shtimestamp[];
extern	int	sh_cnt;

extern	int	nNSH;
extern int shdsr;

#else BINARY

struct	uba_device *shinfo[NSH];
struct	sh_softc sh_softc[NSH];
short	shsoftCAR[NSH];
short	shdefaultCAR[NSH];

struct	tty sh_tty[NSH*8];	    /* one tty structure per line */
u_char	shmodem[NSH*8];		    /* to keep track of modem state */
struct	timeval shtimestamp[NSH*8]; /* to keep track of CD transient drops */
int	sh_cnt	= NSH*8;	    /* total number of sh lines   */
int	nNSH	= NSH;		    /* total number of sh modules */

#ifdef NODSR
int shdsr = 0;			    /* a "0" here means ignore DSR */
#else NODSR
int shdsr = 1;			    /* a "1" here means follow DECSTD52 */
#endif NODSR

#endif BINARY
