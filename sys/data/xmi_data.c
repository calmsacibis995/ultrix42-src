#ifndef lint
static char *sccsid = "@(#)xmi_data.c	4.2	ULTRIX	8/3/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1984,1985,1986,1987,1988 by	*
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

/* ------------------------------------------------------------------------
 * Modification History:
 *
 *   03-Aug-90		rafiey (Ali Rafieymehr)
 *	Added XJA (VAX9000) entry. Also removed unnecessary
 *	code and comments for allocation of xminode which was taking a 
 *	lot of kernel BSS space (specially for multiple XMIs).
 *
 *   06-Jun-1990	Pete Keilty
 *	Added preliminary support for CIKMF.
 *
 *   05-Jun-1990	Joe Szczypek
 *	Removed XMA2.  Entry for XMA will handle XMA2 (both have
 *	same device id).
 *
 *   01-May-1990        Joe Szczypek
 *      Included XMP, XBI+, and XMA2.
 *
 *   08-Dec-1989	Fix support for rigel
 *
 *   18-Aug-1989	Pete Keilty
 *	Change CIXCB to CIXCD.
 *
 *   20-Jul-89		rafiey (Ali Rafieymehr)
 *	Included XNA, and KDM in xmisw[].
 *
 *   14-Mar-1989	Mark A. Parenti
 *	Add support for HSM.
 *
 *   16-Jun-1989	Darrell A. Dunnuck
 *	Removed cpup as an arg passed to functions.
 *
 *   05-Aug-1988	Todd M. Katz
 *	Refer to function xminotconf() instead of to function binotconf()
 *	when no CIs are configured.
 *
 *   05-May-1988	Todd M. Katz
 *	Add support for the CIXCB XMI to CI communications port.
 *
 * ------------------------------------------------------------------------
 */

#include "../machine/pte.h"
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/map.h"
#include "../h/buf.h"
#include "../machine/cpu.h"
#include "../io/bi/bireg.h"
#include "../io/bi/buareg.h"
#include "../io/xmi/xmireg.h"
#include "../io/uba/ubavar.h"
#include "vaxbi.h"
#include "xmi.h"

int xminotconf();

noxmireset()
{
	/* no bi reset routine */

}

#ifdef VAX6200
int xcpinit();
#else	VAX6200
#define xcpinit xminotconf
#endif	VAX6200

#ifdef VAX9000
int xjainit();
#else	VAX9000
#define xjainit xminotconf
#endif	VAX9000

#ifdef	DS5800
int x3p_init();
#else	DS5800
#define x3p_init xminotconf
#endif	DS5800

#ifdef VAX6400
int xrpinit();
#else
#define xrpinit xminotconf
#endif

#if	defined(VAX6200) || defined(DS5800) || defined(VAX6400)
int xmainit();
#else
#define xmainit xminotconf
#endif

#if NVAXBI > 0
int xbiinit();
#else
#define xbiinit xminotconf
#endif

#include "ci.h"
#if NCI > 0
int     xmiciinit();
#else
#define xmiciinit xminotconf
#endif

#include "kdm.h"
#if NKDM > 0
int     kdminit();
#else
#define kdminit xminotconf
#endif

#include "xna.h"
#if NXNA > 0
int xnaprobe(), xnaattach();
#else
#define xnaprobe xminotconf
#endif

int nNXMI = NXMI;


int (*xcpprobes[])() = {  xcpinit,0};
int (*xrpprobes[])() = {  xrpinit,0};
int (*x3p_probes[])() = { x3p_init,0};
int (*xmaprobes[])() = {  xmainit,0};
int (*xbiprobes[])() = {  xbiinit,0};
int (*xcixmiprobes[])() = {  xmiciinit,0};
int (*xkdmprobes[])() = {  kdminit,0};
int (*xnaxmiprobes[])() = {  xnaprobe,0};
int (*xjaprobes[])() = {  xjainit,0};


struct xmisw xmisw [] =
{

	{ XMI_XCP, 	"xcp",		xcpprobes ,	noxmireset,	
	  XMIF_NOCONF},
	{ XMI_XRP, 	"xrp",		xrpprobes ,	noxmireset,	
	  XMIF_NOCONF},
	{ XMI_XMP, 	"xmp",		xrpprobes ,	noxmireset,	
	  XMIF_NOCONF},
	{ XMI_X3P,	"x3p",		x3p_probes,	noxmireset,
	  XMIF_NOCONF},
	{ XMI_XMA, 	"xma",		xmaprobes ,	noxmireset,	
	  XMIF_NOCONF},
	{ XMI_XBI, 	"xbi",		xbiprobes ,	noxmireset,	
	  XMIF_ADAPTER},
	{ XMI_XBIPLUS, 	"xbi+",		xbiprobes ,	noxmireset,	
	  XMIF_ADAPTER},
	{ XMI_CIXCD, 	"cixcd",	xcixmiprobes ,	noxmireset,	
	  XMIF_ADAPTER|XMIF_SST},
	{ XMI_CIKMF, 	"cikmf",	xcixmiprobes ,	noxmireset,	
	  XMIF_ADAPTER},
	{ XMI_KDM, 	"kdm",		xkdmprobes ,	noxmireset,	
	  XMIF_ADAPTER|XMIF_SST},
	{ XMI_XNA, 	"xna",		xnaxmiprobes ,	noxmireset,	
	  XMIF_DEVICE|XMIF_SST},
	{ XMI_XJA, 	"xja",		xjaprobes ,	noxmireset,	
	  XMIF_NOCONF},

	{ 0 }
};

int nxmitypes = sizeof (xmisw) / sizeof (xmisw[0]);

struct xmidata *head_xmidata;


xminotconf(nxv,nxp,xminumber,xminode,xmidata) 
struct xmi_nodespace *nxv;
struct xmi_nodespace *nxp;
int 	xminumber;
int 	xminode;
struct xmidata *xmidata;

{
	struct xmisw *pxmisw;	

	pxmisw = xmidata->xmierr[xminode].pxmisw;

	printf("%s at xmi%x node %x option not configured!\n",
		pxmisw->xmi_name,xminumber,xminode);
		
}

