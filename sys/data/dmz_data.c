
/*
 * 	@(#)dmz_data.c	4.1	(ULTRIX)	7/2/90
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
 * dmz_data.c
 *
 * Modification history
 *
 * DMZ32 data file
 *
 * 16-Jan-86 - Larry Cohen
 *
 *	Add full DEC standard 52 support.
 *
 * 14-Apr-86 - jaw
 *
 *	Remove MAXNUBA referances.....use NUBA only!
 *
 * 26-Apr-86 - ricky palmer
 *
 *	Added "devio.h" to include list. V2.0
 *
 * 29-Jan-87 - Tim Burke
 *
 *	Added definition of dmzdsr, a variable used to define the type of 
 *	modem control that is being followed.
 *
 * 11-Aug-87 - Tim Burke
 *
 *	Added exec.h to list of include files for compatibility mode check 
 *	stored in the upper 4 bits of the magic number.
 *
 * 12-Jun-89 - dws
 *
 *	Added trusted path support.
 *
 */

#include	"dmz.h"
#include	"uba.h"

#include "../machine/pte.h"

#include "bk.h"
#include "../h/param.h"
#include "../h/conf.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/ioctl.h"
#include "../h/tty.h"
#include "../h/map.h"
#include "../h/buf.h"
#include "../h/vm.h"
#include "../h/bk.h"
#include "../h/clist.h"
#include "../h/file.h"
#include "../h/uio.h"
#include "../h/kernel.h"
#include "../h/devio.h"
#include "../h/exec.h"
#include "../h/proc.h"
#include "../h/sys_tpath.h"

#include "../io/uba/ubareg.h"
#include "../io/uba/ubavar.h"
#include "../io/uba/dmzreg.h"

#ifdef BINARY

extern	struct	uba_device *dmzinfo[];
extern	struct	dmz_softc dmz_softc[];

struct	tty dmz_tty[];
char	dmz_dma[];
long	dmzsoftCAR[];
long	dmzdefaultCAR[];
extern	u_char dmzmodem[];
extern	struct timeval dmztimestamp[];

extern	int	nNDMZ;
extern	int	ndmz;
extern	int	cbase[];
extern  int 	dmzdsr;

#else BINARY

int cbase[NUBA];		/* base address in unibus map */
struct	uba_device *dmzinfo[NDMZ];
struct	dmz_softc dmz_softc[NDMZ];

struct	tty dmz_tty[NDMZ*24];
char	dmz_dma[NDMZ*24];
long	dmzsoftCAR[NDMZ];
long	dmzdefaultCAR[NDMZ];
u_char dmzmodem[NDMZ*24];
struct timeval dmztimestamp[NDMZ*24];

int	nNDMZ = NDMZ;
int	ndmz = NDMZ*24;

#ifdef NODSR
int dmzdsr = 0;				/* "0"=Don't follow DECSTD52 */
#else NODSR
int dmzdsr = 1;				/* "1"=Do follow DECSTD52 */
#endif NODSR

#endif BINARY
