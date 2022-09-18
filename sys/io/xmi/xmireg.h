/*
 * @(#)xmireg.h	4.4	ULTRIX	9/4/90
 */

/************************************************************************
 *									*
 *			Copyright (c) 1988 by				*
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
 * Revision History
 *
 * 03-Aug-1990	rafiey (Ali Rafieymehr)
 *	Added dtype define for XJA (VAX9000).
 *
 * 06-Jun-1990	Pete Keilty
 *	1. Modified xmi_reg padding to 17k a must for CIKMF(dash).
 *	2. Added XMI_CIKMF for preliminary support.
 *
 * 11-Apr-1990  Joe Szczypek
 *      Added dtype defines for XMP processor, XBI+ adapter, and XMA2.
 *
 * 08-Dec-1989	Pete Keilty
 *	1. Modified xmi_reg padding to 16k a must for CIXCD.
 *	2. Added 4 new defines for XMI_LEVEL14, XMI_LEVEL15, XMI_LEVEL16
 *	   & XMI_LEVEL17.
 *	3. XMINODE_SIZE is now 16k for VAX.
 *
 * 08-Dec-1989	jaw
 * 	device change for rigel.
 *
 *   19-Sep-1989	Pete Keilty
 *	Add XCD support, remove XCB.
 *
 * 20-Jul-89	rafiey (Ali Rafieymehr)
 *	Added support for XMI devices.
 *
 *   03-May-1988	Todd M. Katz
 *	1. Define XMI device type register fields.
 *	2. Add support for the CIXCB XMI to CI communications port by
 *	   adding the XMI device type XMI_CIXCB.
 *	3. Add placeholder macros for SCB_XMI_LWOFFSET() and
 *	   SCB_XMI_VEC_ADDR().
 */

struct xmi_reg {
	unsigned int  	xmi_dtype;		/* device type register */
	unsigned int  	xmi_xbe;		/* bus error status */
	unsigned int	xmi_fadr;		/* fail address on error */
	unsigned int	xmi_gpr;		/* general purpose register */
#ifdef KERNEL
#include "../h/ansi_compat.h"
#else
#include <ansi_compat.h>
#endif

#ifdef __vax
	char	xmi_pad[17392];			/* pad to 17k byte XMI node
						   space mapping size. */
#endif /* __vax */
#ifdef __mips
	char	xmi_pad[524272];		/* pad to XMI node space
						size, since we don't map it
						on mips systems */
#endif /* __mips */
};

#ifdef	KERNEL
extern struct xmi_reg xminode[];
#endif /*  KERNEL */


/*  xmi flags */
#define XMIF_SST 0x1			/* do node reset before call init */
#define XMIF_SET_HEIE 0x2		/* if set don't enable HES */
#define XMIF_DEVICE 0x4			/* is a device in the config file */
#define XMIF_CONTROLLER 0x8		/* is a controller in config file */
#define XMIF_ADAPTER  0x10		/* adapters...uba's etc */
#define XMIF_NOCONF 0x1000		/* Isn't config'd */

/* XMI device type register */

#define	XMIDTYPE_TYPE	0x0000ffff	/* XMI device type field */
#define	XMIDTYPE_REV	0xffff0000	/* XMI device revision field */

					/* XMI device types defined */
#define	XMI_XCP		0x8001		/* CVAX processor */
#define	XMI_XRP		0x8082		/* Rigel processor */
#define XMI_X3P		0x8081		/* ISIS/R3000 processor set */
#define XMI_XMP         0x8080          /* Mariah processor */
#define XMI_XMA		0x4001		/* XMA memory */
#define XMI_XMA2        0x804001        /* XMA2 memory */
#define XMI_XBI		0x2001		/* XMI to BI adapter */
#define XMI_XBIPLUS     0x2002          /* Enhanced XMI to BI adapter */
#define	XMI_XJA		0x1001		/* VAX9000 XJA */
#define XMI_XNA		0x0c03		/* XMI to NI adapter */
#define	XMI_CIXCD	0x0c05		/* XMI to CI adapter */
#define	XMI_KDM		0x0C22		/* XMI to SI adapter */
#define	XMI_CIKMF	0x0810		/* XMI to CI adapter - DUAL ports */

/* Bus error definitions */

#define XMI_ES		0x80000000	/* */
#define XMI_NRST	0x40000000	/* */
#define XMI_NHALT	0x20000000	/* */
#define XMI_XBAD	0x10000000	/* */
#define XMI_CC		0x08000000	/*  */
#define XMI_XFAULT	0x04000000	/*  */
#define XMI_WEI		0x02000000	/*  */
#define XMI_IPE		0x01000000	/*  */
#define XMI_PE		0x00800000	/* */
#define XMI_WSE		0x00400000	/*  */
#define XMI_RIDNAK	0x00200000	/* */
#define XMI_WDNAK	0x00100000	/*  */
#define XMI_CRD		0x00080000	/*  */
#define XMI_NRR		0x00040000	/*  */
#define XMI_RSE		0x00020000	/*  */
#define XMI_RER		0x00010000	/*  */
#define XMI_CNAK	0x00008000	/*  */
#define XMI_TE		0x00004000	/*  */
#define XMI_TTO		0x00002000	/*  */
#define XMI_NSES	0x00001000	/*  */
#define XMI_ETF		0x00000800	/*  */
#define XMI_STF		0x00000400	/*  */
#define XMI_FCID	0x000003F0	/*  */
#define XMI_FCMD	0x0000000F	/*  */

#define LEVEL14 0x100
#define LEVEL15 0x140
#define LEVEL16 0x180
#define LEVEL17 0x1c0
#define XMIVECSIZE 0x40
#define XMIEINT_XMIVEC  0x50
#define XMI_LEVEL14 0x10000
#define XMI_LEVEL15 0x20000
#define XMI_LEVEL16 0x40000
#define XMI_LEVEL17 0x80000

struct xmisw {

	short	xmi_type;		/* bi device type */
	char	*xmi_name;		/* name of the device*/
	int	(**probes)();		/* funtions to probe at boot time */
	int	(*xmi_reset)();		/* reset routine for device */
	short	xmi_flags;		
};
extern struct xmisw xmisw[];

struct xmidata {
	struct xmidata *next;
	int xminum;
	struct xmi_reg *xmivirt;
	struct xmi_reg *xmiphys;
	struct xmi_reg *cpu_xmi_addr; 
	int (**xmivec_page)();
	int xminodes_alive;
	int xmiintr_dst;
	int xmi_err_cnt;
	unsigned int xmiilast_err_time;
	struct {
		struct xmisw *pxmisw;
		int xmierr;
		int xmierr1;
	} xmierr[16];
};

extern struct xmidata *head_xmidata;
extern struct xmidata *get_xmi();
#define MAX_XMI_NODE	16
#ifdef	__vax
#define XMINODE_SIZE	17408	/* should be 512k but using 17k
				to reduce space used by pte's,
				17k needed for CIXCD & CIKMF*/
#endif /* __vax */
#ifdef __mips
#define XMINODE_SIZE	4096	/* Not used - and not mapped on mips */
#endif /* __mips */


#define SCB_XMI_LWOFFSET(xmi_nodenum,level) \
	((xmi_nodenum << 2) | level)

#define SCB_XMI_VEC_ADDR(xmidata,xminumber,xmi_nodenum,level) \
	((xmidata->xmivec_page)+(((xmi_nodenum << 2) | level)/4))

#define SCB_XMI_ADDR(xmidata) \
	((xmidata->xmivec_page))

#define XMA2_MASK       0x80ffff /* If bit 23 set of XMI device revision field
				    module is XMA2.  Note other bits can be set
				    in this field.  Must be masked out. Low 16
				    bits match original XMA value */




