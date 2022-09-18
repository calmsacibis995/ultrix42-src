/*
 *	@(#)gvpscs.h	4.1	(ULTRIX)	7/2/90
 */

/************************************************************************
 *                                                                      *
 *                      Copyright (c) 1988 - 1989 by                    *
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
 *		Generic Vaxport Port Driver
 *
 *   Abstract:	This module contains Generic Vaxport Port Driver( GVP )
 *		data structure definitions visible to SCS.
 *
 *   Creator:	Todd M. Katz	Creation Date:	November 20, 1985
 *
 *   Modification History:
 *
 *   20-May-1989	Pete Keilty
 *	Added support for mips risc cpu's volatile to register pointer.
 *
 *   06-Dec-1988	Todd M. Katz		TMK0003
 *	Removed from structure definition GVPPCCB( union type ) MSI specific
 *	fields.
 *
 *   21-Aug-1988	Todd M. Katz		TMK0002
 *	Replace spare longword in GVPPCCB with field rspq_remerr( port response
 *	queue remove error ).
 *
 *   02-Jun-1988	Ricky S. Palmer
 *	Removed the struct entry for the PQB.
 *
 *   29-Jan-1988	Ricky S. Palmer
 *	Added struct entries for msi in both the Port Queue Block
 *	and PCCB struct definitions.
 *
 *   08-Jan-1988	Todd M. Katz		TMK0001
 *	Formated module, revised comments, increased robustness, made GVP
 *	completely independent from underlying port drivers, restructured code
 *	paths, and added SMP support.
 */

/* Generic Vaxport Data Structure Definitions.
 */
typedef struct _gvppqb	{		/* Generic Vaxport Port Queue Block  */
    struct _gvpbq cmdq0;		/* Command queue head priority 0     */
    struct _gvpbq cmdq1;		/* Command queue head priority 1     */
    struct _gvpbq cmdq2;		/* Command queue head priority 2     */
    struct _gvpbq cmdq3;		/* Command queue head priority 3     */
    struct _gvpbq rspq;			/* Response queue head		     */
    union	  	   {		/* Implementation dependent fields   */
	struct _cipqb	   ci;		/*  CI specific fields of PQB	     */
	struct _bvp_ssppqb bvp;		/*  BVP SSP specific fields of PQB   */
    } type;
} GVPPQB;

typedef struct _gvppq_info {		/* Generic Vaxport Port Queue	     */
					/*  Information		 	     */
    struct _gvpbq *header;		/* Port maintenance queue pointer    */
    volatile unsigned long *creg;	/* Port maintenance q ctrl reg ptr   */
    u_long	  cmask;		/* Port maintenance queue ctrl mask  */
    u_long	  error;		/* Port maintenance q insertion error*/
} GVPPQ_INFO;

typedef	struct _gvppccb	{		/* Generic Vaxport PCCB Fields	     */
    struct _gvppqb pqb;			/* Port Queue Block		     */
    struct _gvppq_info pmaintq;		/* Port maintenance queue information*/
    struct _gvppq_info pblockq;		/* Port block transfer queue info    */
    struct _gvppq_info pcommq;		/* Port communication queue info     */
    struct _gvppq_info pcontrolq;	/* Port control queue information    */
    struct _gvppq_info pdfreeq;		/* Port datagram free queue info     */
    struct _gvppq_info pmfreeq;		/* Port message free queue info	     */
    u_long	   dfreeq_remerr;	/* Port datagram free q remove error */
    u_long	   mfreeq_remerr;	/* Port message free q remove error  */
    u_long	   rspq_remerr;		/* Port response queue remove error  */
    void	   ( *qtransition )();	/* Queue transition routine address  */
    union	   	    {		/* Implementation dependent fields   */
	struct _cipccb	    ci;		/*  CI specific fields of PCCB	     */
	struct _bvp_ssppccb bvp;	/*  BVP SSP specific fields of PCCB  */
    } type;
} GVPPCCB;

typedef union _gvppb	{		/* Generic Vaxport PB Fields	     */
    struct _cipb	ci;		/* CI specific fields of PB	     */
} GVPPB;
