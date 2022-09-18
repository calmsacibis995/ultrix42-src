/*
 * @(#)uu_data.c	4.1	(ULTRIX)	7/2/90
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
#include "uu.h"


#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/buf.h"
#include "../h/conf.h"
#include "../h/time.h"
#include "../h/kernel.h"
#include "../h/errno.h"
#include "../h/file.h"

#include "../machine/cpu.h"
#include "../machine/nexus.h"
#include "../machine/rsp.h"

#include "../io/uba/ubavar.h"
#include "../io/uba/ubareg.h"
#include "../io/uba/uureg.h"

#define	NTUBLK	512		/* number of blocks on a TU58 cassette */
#define	WRV     01              /* bit in minor dev => write w. read verify */
#define	NDPC	02		/* drives per controller */
#define	NUX	NDPC * NUU	/* number of drives */
#define	NUUQ	02		/* # of block which can be queued up */
#define	UMASK	01		/* unit number mask */
#define UUIPL	0x14		/* ipl level to use */


#ifdef	BINARY

extern	struct packet uucmd[];	/* a command sent to the TU58 */
extern	struct packet uudata[];	/* a command or data returned from TU58 */
extern	struct buf uitab[];		/* buffer queue headers */

/*
 * Driver soft carrier structure
 */
struct uu_softc {
	u_char	*tu_rbptr;	/* pointer to buffer for read */
	int	tu_rcnt;	/* how much to read */
	u_char	*tu_wbptr;	/* pointer to buffer for write */
	int	tu_wcnt;	/* how much to write */
	int	tu_state;	/* current state of tansfer operation */
	int	tu_flag;	/* read in progress flag */
	char	*tu_addr;	/* real buffer data address */
	int	tu_count;	/* real requested count */
	int	tu_serrs;	/* count of soft errors */
	int	tu_cerrs;	/* count of checksum errors */
	int	tu_herrs;	/* count of hard errors */
	char    tu_dopen[2];	/* drive is open */
} uu_softc[];
extern	struct	uba_device	*uudinfo[];
extern	static char uu_pcnt[];		/* pee/vee counters, one per drive */

extern	int	nNUU;
extern	int	nNUX;
extern	int	nNUUQ;
	
#else

struct packet uucmd[NUU];	/* a command sent to the TU58 */
struct packet uudata[NUU];	/* a command or data returned from TU58 */
struct buf uitab[NUU];		/* buffer queue headers */

/*
 * Driver soft carrier structure
 */
struct uu_softc {
	u_char	*tu_rbptr;	/* pointer to buffer for read */
	int	tu_rcnt;	/* how much to read */
	u_char	*tu_wbptr;	/* pointer to buffer for write */
	int	tu_wcnt;	/* how much to write */
	int	tu_state;	/* current state of tansfer operation */
	int	tu_flag;	/* read in progress flag */
	char	*tu_addr;	/* real buffer data address */
	int	tu_count;	/* real requested count */
	int	tu_serrs;	/* count of soft errors */
	int	tu_cerrs;	/* count of checksum errors */
	int	tu_herrs;	/* count of hard errors */
	char    tu_dopen[2];	/* drive is open */
} uu_softc[NUU];
struct	uba_device	*uudinfo[NUU];
static char uu_pcnt[NUX];		/* pee/vee counters, one per drive */

int	nNUU = NUU;
int	nNUX = NUX;
int	nNUUQ = NUUQ;

#endif
