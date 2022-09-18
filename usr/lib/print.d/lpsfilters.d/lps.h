/* @(#)lps.h	4.2      LPS_ULT 	12/20/90 */
/*
 * lps.h -- Configuration information 
 *
 * Revision 1.b  90/07/13  00:05:00  augeri
 * Change PACKET_BUFFER_SIZE value back to 2*DECNET_MAX_FRAME
 * 
 * Revision 1.a  90/07/10  21:10:00  augeri
 * Change PACKET_BUFFER_SIZE value
 * 
 * Revision 1.9  89/02/07  11:00:00  reid
 * change version ID to WRL-1.0
 * 
 * Revision 1.8  88/12/07  14:27:34  reid
 * protected definitions of TRUE and FALSE with #ifdef TRUE/#ifdef FALSE
 * 
 * Revision 1.7  88/11/01  18:14:37  reid
 * Added declaration of LP_C_gets()
 * 
 * Revision 1.6  88/10/31  20:31:17  reid
 * Changed to manual recognition of setjmp.h loading
 * 
 * Revision 1.5  88/10/30  20:55:31  reid
 * Made more-aggressive test for SETJMP_LOADED for mips compilers.
 * 
 * Revision 1.4  88/08/28  22:58:37  reid
 * *** empty log message ***
 * 
 * Revision 1.3  88/08/28  22:28:51  reid
 * Add definition of PACKET_BLOCK_SIZE for (bletch) socket blocking.
 * 
 * Revision 1.2  88/08/15  00:10:19  reid
 * Be more clever about determining whether setjmp.h has been loaded.
 * 
 * Revision 1.1  88/08/06  01:00:35  reid
 * Initial revision
 * 
 *	    Copyright (c) Digital Equipment Corporation, 1991
 *	    All Rights Reserved.  Unpublished rights reserved
 *	    under the copyright laws of the United States.
 *
 *	    The software contained on this media is proprietary
 *	    to and embodies the confidential technology of 
 *	    Digital Equipment Corporation.  Possession, use,
 *	    duplication or dissemination of the software and
 *	    media is authorized only pursuant to a valid written
 *	    license from Digital Equipment Corporation.
 *
 *	    RESTRICTED RIGHTS LEGEND   Use, duplication, or 
 *	    disclosure by the U.S. Government is subject to
 *	    restrictions as set forth in Subparagraph (c)(1)(ii)
 *	    of DFARS 252.227-7013, or in FAR 52.227-19, as
 *	    applicable.
 *
 *		   PostScript is by Adobe Systems, Inc.
 */

/* #define DEBUG */
#define progVersion "WRL-1.0"
#define	SERVER_TIMEOUT_DEFAULT	60	/* seconds */
#define DEFAULT_LIB "/usr/local/lib/lps"
#define DEFAULT_BREAK "breakPage.psh"
#define DEFAULT_ERR   "errorPage.psh"
#define DEFAULT_TXT   "lptext"
#define DEFAULT_TMP   "/tmp"

/* Protocol information */
#define	OP_MAX_SIZE	12	/* max characters in an opcode */
#define PACKET_BUFFER_SIZE 	2*DECNET_MAX_FRAME
#define PACKET_BLOCK_SIZE	1024 /* max legal for DECNET */

/* Exit codes for client filter to signal lpd */

#define	EX_SUCCESS	0
#define	EX_TRYAGAIN	1
#define	EX_DISCARD	2

/* Error severity codes for internal message routines */
#define	LPS_SUCCESS	0
#define	LPS_WARN	1
#define	LPS_ERROR	2
#define LPS_FATAL	3

/* Utility definitions */
#ifndef TRUE
#define TRUE		1
#define	FALSE		0
#endif TRUE

/* global variables */
extern char progName[];
extern char errbuf[];
extern int debug;

#ifdef SETJMP_LOADED
extern jmp_buf write_error;
extern jmp_buf read_error;
extern int     jump_write_error;	
extern int     jump_read_error;	
#endif SETJMP_LOADED

/* global procedures */
extern int	LP_C_waitForReply();
extern int	LP_C_recv();
extern void	LP_C_waitForMsg();
extern void	LP_C_close();
extern char	*LP_C_gets();
extern char    *getvar();
extern char    *LPStime();
extern char    *smatch();

/* library procedures */
extern char *malloc();
