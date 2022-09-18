/*
 *	@(#)bvpreg.h	4.1	(ULTRIX)	7/2/90
*/

/************************************************************************
 *									*
 *			Copyright (c) 1987 by				*
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
 *	BVP register definitions
 */
volatile struct	bvpregs {

	volatile unsigned long	bvp_pc;		/* Port Control Register*/
	volatile unsigned long	bvp_ps;		/* Port Status Register	*/
	volatile unsigned long	bvp_pe;		/* Port Error Register	*/
	volatile unsigned long	bvp_pd;		/* Port Data Register	*/
};

/*
 *	Port Control Register Definitions
 */

#define	BVP_PC_OWN	0x00000080	/* Port Control Ownership bit	*/

/*
 *	Port Intructions (used in Port Control  register)
 */

#define	BVP_CMD_PINIT	1	/* Port Init			*/
#define	BVP_CMD_ENAB	2	/* Enable Queue Processing	*/
#define	BVP_CMD_RPIV	3	/* Read Port Interrupt Vector	*/
#define	BVP_CMD_SHUT	4	/* Shutdown Port		*/
#define	BVP_CMD_MAINT	5	/* Enter MAINT state		*/
#define	BVP_CMD_CQNE	6	/* Command Queue Not Empty	*/
#define	BVP_CMD_FQNE	7	/* Free Queue Not Empty		*/
#define	BVP_CMD_PSQRY	8	/* Port Status Query		*/
#define	BVP_CMD_ENTER	9	/* Enable Transient Error Report*/
#define	BVP_CMD_DSTER	10	/* Disable Transient Error Report*/
#define	BVP_CMD_RESTART	11	/* Restart port from Stop state	*/

/*
 * Port Status Register Definitions
 */

#define	BVP_PS_OWN	0x80000000	/* Ownership bit		*/
#define	BVP_PS_RSPQ	0x40000000	/* Response to Port Status Query */
#define	BVP_PS_STD	0x20000000	/* Self Test Done		*/
#define	BVP_PS_ACC	0x10000000	/* Adapter Can Communicate	*/
#define	BVP_PS_XSTP	0x08000000	/* Extendend Self Test Done	*/
#define	BVP_PS_ERL	0x04000000	/* Error Lost			*/
#define	BVP_PS_FQE	0x02000000	/* Free Queue Empty		*/
#define	BVP_PS_RSQ	0x00000080	/* Response Queued		*/
#define	BVP_PS_SUME	0x00000040	/* Summary Error		*/

#define	BVP_PS_PST	0x00070000	/* Port State mask		*/
#define	BVP_PS_ETYPE	0x0000FF00	/* Error Type mask		*/

/*
 *	Port States
 */

#define	BVP_PSTATE_UNDF	0x00010000	/* Undefined			*/
#define	BVP_PSTATE_INIT	0x00020000	/* Initialized			*/
#define	BVP_PSTATE_ENAB	0x00040000	/* Enabled			*/
#define	BVP_PSTATE_STOP	0x00060000	/* Stopped			*/
#define	BVP_PSTATE_MAIN	0x00070000	/* Maintenance			*/


/*
 *	Port Error Types
 */

#define	BVP_ETYPE_TBI	0x00000100	/* Transient BI Error		*/
#define	BVP_ETYPE_EXC	0x00000200	/* Adapter Exception		*/
#define	BVP_ETYPE_NFBI	0x00000300	/* Non-Fatal BI Error		*/
#define	BVP_ETYPE_FBI	0x00000400	/* Fatal BI Error		*/
#define	BVP_ETYPE_DSE	0x00000500	/* Data Structure Error		*/
#define	BVP_ETYPE_PLE	0x00000600	/* Port Logical Error		*/
#define	BVP_ETYPE_AHE	0x00000700	/* Adapter Hard Error		*/
