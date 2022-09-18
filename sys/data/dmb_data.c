/*
 *	@(#)dmb_data.c	4.1	(ULTRIX)	7/2/90
 */

/************************************************************************
 *									*
 *			Copyright (c) 1986-1988 by			*
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
 * dmb_data.c
 *
 * Modification history
 *
 * DMB32 data file
 *
 * 13-Jul-89 - Randall Brown
 *
 *	Added support for MIPS based machines.
 *
 * 12-Jun-89 - dws
 *
 *	Added trusted path support.
 *
 * 22-Mar-88 - Tim Burke
 *	Added lock structure for the dmb device.  One lock per board.
 *
 * 29-Dec-87 - Tim Burke
 *
 *	Expand number of lines from 8 to 16 to enable usage of the DHB mux.
 *
 * 11-Aug-87 - Tim Burke
 *
 *	Added exec.h to list of include files for compatibility mode check 
 *	stored in the upper 4 bits of the magic number.
 *
 * 29-Jan-87 - Tim Burke
 *
 *	Added definition of dmbdsr, a variable used to define the type of 
 *	modem control that is being followed.
 *
 * 26-Apr-86 - ricky palmer
 *
 *	Added "devio.h" to include list. V2.0
 *
 */

#include	"dmb.h"

#include "../machine/pte.h"

#ifdef vax
#include "bk.h"
#endif vax
#include "../h/param.h"
#include "../h/conf.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/ioctl.h"
#include "../h/tty.h"
#include "../h/map.h"
#include "../h/buf.h"
#include "../h/vm.h"
#include "../h/vmmac.h"
#include "../h/bk.h"
#include "../h/clist.h"
#include "../h/file.h"
#include "../h/uio.h"
#include "../h/kernel.h"
#include "../h/devio.h"
#include "../h/exec.h"
#include "../h/proc.h"
#include "../h/sys_tpath.h"

#include "../machine/scb.h"
#ifdef vax
#include "../machine/mtpr.h"
#endif vax
#include "../machine/cpu.h"
#include "../io/uba/ubareg.h"
#include "../io/uba/ubavar.h"
#include "../io/bi/dmbreg.h"

struct	uba_device *dmbinfo[NDMB];
#define NUMLINES 16

#ifdef BINARY

extern	struct	uba_device *dmbinfo[];
extern	struct	dmb_softc dmb_softc[];
extern  struct  lock_t lk_dmb[];

extern	struct	tty dmb_tty[];
extern	char	dmb_dma[];
extern	char	dmbsoftCAR[];
extern	char	dmbdefaultCAR[];
extern	u_char dmbmodem[];
extern	struct timeval dmbtimestamp[];
extern	u_short dmb_numchars[];

extern	int	nNDMB;
extern	int	ndmb;
extern	int	dmbdsr;
extern  int	dmb_lines[];

extern	struct dmbl_softc dmbl_softc[];

#else BINARY

struct  lock_t lk_dmb[NDMB];
struct	dmb_softc dmb_softc[NDMB];
struct	tty dmb_tty[NDMB*NUMLINES];
char	dmb_dma[NDMB*NUMLINES];
char	dmbsoftCAR[NDMB];
char	dmbdefaultCAR[NDMB];
struct dmbl_softc dmbl_softc[NDMB];
u_char dmbmodem[NDMB*NUMLINES];
struct timeval dmbtimestamp[NDMB*NUMLINES];
u_short dmb_numchars[NDMB*NUMLINES];

int	nNDMB = NDMB;
int	ndmb = NDMB*NUMLINES;

int	dmb_lines[NDMB];

#ifdef NODSR
int dmbdsr = 0;		/* A "0" here means ignore DSR */
#else NODSR
int dmbdsr = 1;		/* A "1" here means follow DS52 DSR signals */
#endif NODSR

#endif BINARY
