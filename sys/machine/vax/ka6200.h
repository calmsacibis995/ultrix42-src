/*	@(#)ka6200.h	4.1	(ULTRIX)	7/2/90	*/

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
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/
/*
 * Modification History:
 *
 * 06-Jun-1990 	Pete Keilty
 *	Modified xcp_reg padding to 17k same as xmi_reg because CIKMF
 *	node space is 17k.
 *
 * 08-Dec-1989 	Pete Keilty
 *	Modified xcp_reg padding to 16k same as xmi_reg because CIXCD
 *	node space is 16k.
 */

#define	XMI_START_PHYS 	0x21800000


struct v6200csr {
	long 	csr1;

};

struct xcp_reg {
	unsigned int xcp_dtype;
	unsigned int xcp_xbe;
	unsigned int xcp_fadr;
	unsigned int xcp_gpr;
	unsigned int xcp_csr2;
	char	xcp_pad[17388];
};

/* definitions for CSR2 bits */
#define	CSR2_VBPE	0x80000000
#define	CSR2_TPE 	0x40000000
#define	CSR2_IQO 	0x20000000
#define	CSR2_WDPE	0x10000000
#define	CSR2_CFE	0x08000000
#define	CSR2_DTPE	0x04000000
#define	CSR2_LOCKOUT	0x00600000
#define CSR2_ERRORS	0xff000000

#define CSRV6200 0x20000000

char *ka6200_ip[16];

/*
 * Data structure to keep track of the previous machine check information.
 * One structure per processor.
 */
struct	xcp_machdep_data {
long	mchk_in_progress;	/* Flag set while we are running mcheck code */
long	time;			/* time stamp of the machine check */
long	code;			/* machine check code		   */
long	maddr;			/* most recent memory address	   */
long	istate1;		/* internal state 1		   */
long	istate2;		/* internal state 2		   */
long	pc;			/* program counter		   */
long	psl;			/* PSL				   */

struct el_xcpsoft xcpsoft;
};
