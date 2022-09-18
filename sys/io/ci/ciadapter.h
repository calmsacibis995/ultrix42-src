/*
 *	@(#)ciadapter.h	4.1	(ULTRIX)	7/2/90
 */

/************************************************************************
 *                                                                      *
 *                      Copyright (c) 1987 by                           *
 *              Digital Equipment Corporation, Maynard, MA              *
 *                      All rights reserved.                            *
 *                                                                      *
 *   This software is furnished under a license and may be used and     *
 *   copied  only  in accordance with the terms of such license and     *
 *   with the  inclusion  of  the  above  copyright  notice.   This     *
 *   software  or  any  other copies thereof may not be provided or     *
 *   otherwise made available to any other person.  No title to and     *
 *   ownership of the software is hereby transferred.                   *
 *                                                                      *
 *   The information in this software is subject to change  without     *
 *   notice  and should not be construed as a commitment by Digital     *
 *   Equipment Corporation.                                             *
 *                                                                      *
 *   Digital assumes no responsibility for the use  or  reliability     *
 *   of its software on equipment which is not supplied by Digital.     *
 *                                                                      *
 ************************************************************************
 *
 *
 *   Facility:	Systems Communication Architecture
 *		Computer Interconnect Port Driver
 *
 *   Abstract:	This module contains Computer Interconnect Port Driver( CI )
 *		constants, data structure definitions, and macros required
 *		to link CI ports and their driver with lower level machine
 *		specific portions of the Ultrix kernel.
 *
 *   Creator:	Todd M. Katz	Creation Date:	January 31, 1986
 *
 *   History:
 *
 *   Modification History:
 *
 *   06-Jun-1990	Pete Keilty
 *	1. Added preliminary support for CIKMF( dash ).
 *	2. Added new structure CIISR.
 *
 *   19-Sep-1989	Pete Keilty
 *	Change XCB to XCD.
 *
 *   27-Apr-1988	Todd M. Katz
 *	1. Add support for the CIXCB hardware port type by adding support for
 *	   XMI based ports to structure definition CIADAP through addition of
 *	   structure xmi to union ic and the creation of shorthand notations
 *	   Xminum and Xminode.
 *	2. Add the CIBCA BIIC Device Type Register Port Revision Field Mask
 *	   Bits constants.
 *
 *   08-Dec-1987	Todd M. Katz
 *	Formated module, revised comments, and generalized CIADAP
 *	definition to support multiple interconnect types.
 */

/*
 * Constants.
 */
					/* CIBCA BIIC Device Type Register   */
					/*  Port Revision Field Mask Bits    */
#define	CIBCA_DEV_BCABA	  0x400000	/* CIBCA-BA communications port	     */

#define	CI_ADAPSIZE		32	/* Size adapter I/O space( pages )   */

#define	NCI_SUPPORTED		 4	/* Number of CIs supported	     */

/* Data Structures.
 *
 * Adapter Interface Block Definition.
 *
 * NOTE: This structure must NOT be changed without changing locore.s which has
 *	 dependencies on:
 *
 *	 1. The size of the structure.
 *	 2. The position of "isr" within the structure.
 *	 3. The position of "pccb" within the structure.
 */
typedef	struct _ciadap	{		/* CI Adapter Interface Block	     */
    void	   ( *isr )(); 		/* Interrupt Service Routine address */
    struct _pccb   *pccb;		/* PCCB pointer			     */
    void	   ( *mapped_isr )();	/* Mapped port ISR addr( OPTIONAL )  */
    unsigned char  *phyaddr;		/* Adapter I/O space physical address*/
    unsigned char  *viraddr;		/* Adapter I/O space virtual address */
    struct pte	   *iopte;		/* Adapter I/O space PTE pointer     */
    unsigned short npages;		/* Size adapter I/O space( pages )   */
    unsigned char  icnum;		/* Interconnect number		     */
#define	Binum		icnum
#define	Sbinum		icnum
#define	Xminum		icnum
    unsigned char  nexnum;		/* Nexus/Node number		     */
#define	Binode		nexnum
#define	Sbinexus	nexnum
#define	Xminode		nexnum
    struct	{
	u_long	reset   : 1;		/* Adapter reset needed	 */
	u_long	reset_ip: 1;		/* Adapter reset in progress */
	u_long	pccb	: 1;		/* Adapter has control pccb	*/
	u_long		:28;
    } status;
    union	{			/* Interconnect dependent fields     */
	struct		{		/*  BI only fields( CIBCI/CIBCA )    */
	    u_long	*bityp;		/*   BIIC device type register	     */
	    u_long	*bictrl;	/*   BIIC control and status register*/
	    u_long	*bierr;		/*   BIIC error summary register     */
	    u_long	*biint_dst;	/*   BIIC interrupt destination mask */
	    u_long	*biint_ctrl;	/*   BIIC user interrupt control reg */
	    u_long	*bibci_ctrl;	/*   BIIC BCI control register	     */
	    u_long 	biic_int_ctrl;  /*   BIIC user int ctrl reg contents */
	    u_long 	biic_int_dst;	/*   BIIC int dst mask reg contents  */
	} bi;
	struct		{		/*  XMI only fields( CIXCD/CIKMF )   */
	    u_long	*xdev;		/*   XMI device type register	     */
	    u_long	*xbe;		/*   XMI bus error register	     */
	    u_long	*xfadrl;	/*   XMI failing address register low*/
	    u_long	*xfadrh;	/*   XMI failing address register hi */
	} xmi;
    } ic;
    union	{			/* Port dependent fields     */
	struct		{		/*  XMI only fields( CIXCD )	     */
	    u_long	*pidr;		/*   Port interrupt destination reg  */
	    u_long	*pvr;		/*   Port vector register	     */
	    u_long 	pid;		/*   Port int dst reg contents	     */
	    u_long 	pv;	 	/*   Port vector register contents   */
	} xcd;
	struct		{		/*  Adapter only fields( CIKMF )     */
	    u_long *xpcctl;	/*   XPC control register 	     */
	    u_long *xpccsr;	/*   XPC csr register 	     */
	    u_long *pidr1;	/*   Port int dst register 	     */
	    u_long *pidr2;	/*   Port int dst register 	     */
	    u_long *pidr3;	/*   Port int dst register 	     */
	    u_long *pvr1;	/*   Port vector register    */
	    u_long *pvr2;	/*   Port vector register    */
	    u_long *pvr3;	/*   Port vector register    */
	    u_long *piplr1;	/*   Port ipl 1 register    */
	    u_long *piplr2;	/*   Port ipl 2 register    */
	    u_long *piplr3;	/*   Port ipl 3 register    */
	    u_long pid1;	/*   Port int dst reg contents	     */
	    u_long pid2;	/*   Port int dst reg contents	     */
	    u_long pid3;	/*   Port int dst reg contents	     */
	    u_long pv1;		/*   Port vector register contents   */
	    u_long pv2;		/*   Port vector register contents   */
	    u_long pv3;		/*   Port vector register contents   */
	    u_long pipl1;	/*   Port ipl 1 register contents   */
	    u_long pipl2;	/*   Port ipl 2 register contents   */
	    u_long pipl3;	/*   Port ipl 3 register contents   */
	} kmf;
    } adap;
} CIADAP;

typedef	struct _ciisr	{		/* CI Interrupt Service Block	     */
					/* Used by interrupt vector routine  */
    void	   ( *isr )(); 		/* Interrupt Service Routine address */
    struct _pccb   *pccb;		/* PCCB pointer			     */
} CIISR;

#define	Bibci_ctrl	Ciadap->ic.bi.bibci_ctrl
#define	Bictrl		Ciadap->ic.bi.bictrl
#define	Bierr		Ciadap->ic.bi.bierr
#define	Biint_ctrl	Ciadap->ic.bi.biint_ctrl
#define	Biint_dst	Ciadap->ic.bi.biint_dst
#define	Bityp		Ciadap->ic.bi.bityp
#define	Biic_int_ctrl	ic.bi.biic_int_ctrl
#define	Biic_int_dst	ic.bi.biic_int_dst
#define	Kmf_pid1	adap.kmf.pid1
#define	Kmf_pid2	adap.kmf.pid2
#define	Kmf_pid3	adap.kmf.pid3
#define	Kmf_pv1		adap.kmf.pv1
#define	Kmf_pv2		adap.kmf.pv2
#define	Kmf_pv3		adap.kmf.pv3
#define	Kmf_pipl1	adap.kmf.pipl1
#define	Kmf_pipl2	adap.kmf.pipl2
#define	Kmf_pipl3	adap.kmf.pipl3
#define	Kmf_xpcctl	Ciadap->adap.kmf.xpcctl
#define	Kmf_xpccsr	Ciadap->adap.kmf.xpccsr
#define	Kmf_pidr1	Ciadap->adap.kmf.pidr1
#define	Kmf_pidr2	Ciadap->adap.kmf.pidr2
#define	Kmf_pidr3	Ciadap->adap.kmf.pidr3
#define	Kmf_pvr1	Ciadap->adap.kmf.pvr1
#define	Kmf_pvr2	Ciadap->adap.kmf.pvr2
#define	Kmf_pvr3	Ciadap->adap.kmf.pvr3
#define	Kmf_piplr1	Ciadap->adap.kmf.piplr1
#define	Kmf_piplr2	Ciadap->adap.kmf.piplr2
#define	Kmf_piplr3	Ciadap->adap.kmf.piplr3
#define	Xdev		Ciadap->ic.xmi.xdev
#define	Xbe		Ciadap->ic.xmi.xbe
#define	Xfadrh		Ciadap->ic.xmi.xfadrh
#define	Xfadrl		Ciadap->ic.xmi.xfadrl
#define	Xcd_pidr	Ciadap->adap.xcd.pidr
#define	Xcd_pvr		Ciadap->adap.xcd.pvr
#define	Xcd_pid		adap.xcd.pid
#define	Xcd_pv		adap.xcd.pv
