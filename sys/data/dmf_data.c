
/*
 * 	@(#)dmf_data.c	4.1	(ULTRIX)	7/2/90
 */

/************************************************************************
 *									*
 *			Copyright (c) 1985, 1986,1987 by		*
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
 * dmf_data.c
 *
 * Modification history
 *
 * DMF32 data file
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
 *	Added definition of dhudsr, a variable used to define the type of 
 *	modem control that is being followed.
 *
 * 11-Aug-87 - Tim Burke
 *
 *	Added exec.h to list of include files for compatibility mode check 
 *	stored in the upper 4 bits of the magic number.
 *
 * 15-Jul-88 - Tim Burke
 *
 *	Removed dmf_vector array because the dmf number is not passed into the
 * 	dmfprobe routine.
 *
 * 12-Jun-89 - dws
 *
 *	Added trusted path support.
 *
 */

#include	"dmf.h"
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
#include "../io/uba/dmfreg.h"

#ifdef BINARY

extern	struct	uba_device *dmfinfo[];
extern	struct	dmf_softc dmf_softc[];

struct	tty dmf_tty[];
char	dmf_dma[];
char	dmfsoftCAR[];
char	dmfdefaultCAR[];
extern	u_char dmfmodem[];
extern	struct timeval dmftimestamp[];

extern	int	nNDMF;
extern	int	ndmf;

struct dmfl_softc dmfl_softc[];
extern int cbase[];
extern int dmfdsr;

#else BINARY

int cbase[NUBA];		/* base address in unibus map */
struct	uba_device *dmfinfo[NDMF];
struct	dmf_softc dmf_softc[NDMF];
struct	tty dmf_tty[NDMF*8];
char	dmf_dma[NDMF*8];
char	dmfsoftCAR[NDMF];
char	dmfdefaultCAR[NDMF];
struct dmfl_softc dmfl_softc[NDMF];

u_char dmfmodem[NDMF*8];
struct timeval dmftimestamp[NDMF*8];
int	nNDMF = NDMF;
int	ndmf = NDMF*8;

#ifdef NODSR
int dmfdsr = 0;				/* "1" = follow DECSTD52, 0 = no DS52 */
#else NODSR
int dmfdsr = 1;				/* "1" = follow DECSTD52, 0 = no DS52 */
#endif NODSR

#endif BINARY
