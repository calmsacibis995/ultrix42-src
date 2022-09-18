#ifndef	lint
static char *sccsid = "@(#)cons_sw_data.c	4.4      (ULTRIX)  9/7/90";
#endif	lint

/************************************************************************
 *									*
 *			Copyright (c) 1989, 1990 by			*
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
 * Modification History
 *
 * 06-Sep-90 - Randall Brown
 *	Removed entries for 'cfb' and 'pm', they are now folded into fb driver
 *
 * 03-Apr-90 - Tim Burke
 * 	Added console support for Mipsmate (DS5100).
 *
 * 27-Feb-90 - Philip Gapuzte
 *	Added entries for DS5000_100 (3MIN) console ssc.
 *
 * 04-Dec-89 - Sam Hsu
 *	Added entries for DS5000 graphics accelerators to vcons_init.
 *
 * 29-Oct-89 - Randall Brown
 *
 *	Added the c_init entry to each switch entry.  Also added the 
 *	vcons_init table.
 *
 * 20-May-89 - Randall Brown
 *
 *	created file.
 *
 */
#include "../machine/cons_sw.h"
#include "../../machine/common/cpuconf.h"


extern int nocons();

#if defined(DS3100) || defined(DS5000)
int dcopen(), dcclose(), dcread();
int dcwrite(), dcioctl(), dcstart();
int dcstop(), dcselect(), dcputc(), dcgetc();
int dcprobe(), dcintr(), dc_cons_init();
extern struct tty dc_tty[];
#endif DS3100

#if defined(DS5100)
int mdcopen(), mdcclose(), mdcread();
int mdcwrite(), mdcioctl(), mdcstart();
int mdcstop(), ttselect(), mdcputc(), mdcgetc();
int mdcprobe(), mdcintr(), mdc_cons_init();
extern struct tty mdc_tty[];
#endif DS5100

#if defined(DS5000_100) 
int sccopen(), sccclose(), sccread();
int sccwrite(), sccioctl(), sccstart();
int sccstop(), sccselect(), sccputc(), sccgetc();
int sccprobe(), sccintr(), scc_cons_init();
extern struct tty scc_tty[];
#endif DS5000_100

#if defined(DS5400) || defined(DS5800) || defined(DS5500)
int ssc_cnopen(), ssc_cnclose(), ssc_cnread();
int ssc_cnwrite(), ssc_cnioctl(), ssc_cnrint();
int ssc_cnxint(), ssc_cnstart(), ssc_cnputc();
int ssc_cngetc(),ttselect();
extern struct tty cons[];
#endif

struct cons_sw cons_sw[] =
{	/* no system */
#ifdef DS3100
	{	/* PMAX - DECstation 3100 */
	  DS_3100,		dcopen,			dcclose,
	  dcread,	  	dcwrite,		dcioctl,
	  dcintr,		dcintr,			dcstart,
	  dcstop,		dcselect,	        dcputc,
	  dcprobe,		dcgetc,			dc_cons_init,
	  dc_tty,
	},
#endif DS3100

#ifdef DS5100
	{	/* MIPSMATE - DECsystem 5100 */
	  DS_5100,		mdcopen,		mdcclose,
	  mdcread,	  	mdcwrite,		mdcioctl,
	  mdcintr,		mdcintr,		mdcstart,
	  mdcstop,		ttselect,	        mdcputc,
	  mdcprobe,		mdcgetc,		mdc_cons_init,
	  mdc_tty,
	},
#endif DS5100

#ifdef DS5400
	{	/* MIPSFAIR */
	  DS_5400,		ssc_cnopen,		ssc_cnclose,
	  ssc_cnread,		ssc_cnwrite,		ssc_cnioctl,
	  ssc_cnrint,		ssc_cnxint,		ssc_cnstart,
	  nocons,		ttselect,		ssc_cnputc,
	  nocons,		ssc_cngetc,		nocons,
	  cons,
	},
#endif DS5400

#ifdef DS5800
	{	/* ISIS */
	  DS_5800,		ssc_cnopen,		ssc_cnclose,
	  ssc_cnread,		ssc_cnwrite,		ssc_cnioctl,
	  ssc_cnrint,		ssc_cnxint,		ssc_cnstart,
	  nocons,		ttselect,		ssc_cnputc,
	  nocons,		ssc_cngetc,		nocons,
	  cons,
	},
#endif DS5800

#ifdef DS5000
	{	/* 3MAX - DECstation 5000 */
	  DS_5000,		dcopen,			dcclose,
	  dcread,	  	dcwrite,		dcioctl,
	  dcintr,		dcintr,			dcstart,
	  dcstop,		dcselect,	        dcputc,
	  dcprobe,		dcgetc,			dc_cons_init,
	  dc_tty,
	},
#endif DS5000

#ifdef DS5500
	{	/* MIPSFAIR - 2 */
	  DS_5500,		ssc_cnopen,		ssc_cnclose,
	  ssc_cnread,		ssc_cnwrite,		ssc_cnioctl,
	  ssc_cnrint,		ssc_cnxint,		ssc_cnstart,
	  nocons,		ttselect,		ssc_cnputc,
	  nocons,		ssc_cngetc,		nocons,
	  cons,
	},
#endif DS5500

#ifdef DS5000_100 
	{	/* 3MIN */
	  DS_5000_100,		sccopen,		sccclose,
	  sccread,	  	sccwrite,		sccioctl,
	  sccintr,		sccintr,		sccstart,
	  sccstop,		sccselect,	        sccputc,
	  sccprobe,		sccgetc,		scc_cons_init,
	  scc_tty,
	},
#endif DS5000_100 

	{	/* always need 0 to be last entry */
	  0,			0,			0,
	  0,			0,			0,
	  0,			0,			0,
	  0,			0,			0,
	  0,			0,			0,
	  0,
        }
};	

#include "gq.h"
#if NGQ > 0
int gq_cons_init();
#endif

#include "ga.h"
#if NGA > 0
int ga_cons_init();
#endif

#include "fb.h"
#if NFB > 0
int fb_cons_init();
#endif

(*vcons_init[])() = {
#if NGQ > 0
    gq_cons_init,
#endif

#if NGA > 0
    ga_cons_init,
#endif

#if NFB > 0
    fb_cons_init,
#endif

    0			/* we must have a 0 entry to end the loop */
    };

