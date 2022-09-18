/*
 *	@(#)scaparam.h	4.1	(ULTRIX)	7/2/90
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
 *
 *   Abstract:	This module contains global SCA parameters.
 *
 *   Creator:	Todd M. Katz	Creation Date:	January 09, 1988
 *
 *   Modification History:
 *
 *   06-Mar-1990	Pete Keilty
 *	Changed the number of DSSC_CREDITS from 2 to 10, this is the
 *	number returned from the RF & TF controllers.	
 *
 *   18-Sep-1989	Pete Keilty
 *	Added SCA errlogging levels.
 *
 *   15-Aug-1988	Todd M. Katz		TMK0004
 *	1. Completely revise the SCA Console Severity Levels to reflect
 *	   extensive changes in the SCA Event Severity Codes.
 *	2. Change SCA_SEVERITY such that only severe errors are console logged
 *	   by default.
 *
 *   13-Apr-1988	Todd M. Katz		TMK0003
 *	Add SCA error recovery severity level codes.
 *
 *   22-Jan-1988	Todd M. Katz		TMK0002
 *	Add SCA console logging severity levels.  Also modify computation of
 *	GVP_MAX_BDS.  The number of GVP buffer descriptors "associated" with
 *	each remote SYSAP now exceeds by 1 the maximum number of credits
 *	extended by the SYSAP.
 *
 *   10-Jan-1988	Todd M. Katz		TMK0001
 *	Formatted module and revised comments.
 */

					/* Console Logging Severity Levels   */
#define	SCA_LEVEL0		 0	/*  Level 0: Log all events	     */
#define	SCA_LEVEL1		 1	/*  Level 1: Log ES_[ W,RE,E,SE,FE ] */
#define	SCA_LEVEL2		 2	/*  Level 2: Log ES_[ RE ,E, SE, FE ]*/
#define	SCA_LEVEL3		 3	/*  Level 3: Log ES_[ E, SE, FE ]    */
#define	SCA_LEVEL4		 4	/*  Level 4: Log ES_[ SE, FE ]	     */
#define	SCA_LEVEL5		 5	/*  Level 5: Log ES_FE only	     */
#ifndef	SCA_SEVERITY
#define	SCA_SEVERITY	SCA_LEVEL4	/*  Default: Log severe/fatal errors */
#endif /*	SCA_SEVERITY */

					/* Error Logging Severity Levels     */
#define	SCA_ERRLOG0		 0	/*  Level 0: Log all events	     */
#define	SCA_ERRLOG1		 1	/*  Level 1: Log ES_[ W,RE,E,SE,FE ] */
#define	SCA_ERRLOG2		 2	/*  Level 2: Log ES_[ RE ,E, SE, FE ]*/
#define	SCA_ERRLOG3		 3	/*  Level 2: Log ES_[ E, SE, FE ]    */
#ifndef	SCA_ERRLOG
#define	SCA_ERRLOG	SCA_ERRLOG2	/*  Default: Errlog Logging          */
					/*  remote/error/severe/fatal errors */
#endif /*	SCA_ERRLOG */

					/* Error Recovery Severity Levels    */
#define	SCA_PANIC0		 0	/*  Level 0: Recover all errors	     */
#define	SCA_PANIC1		 1	/*  Level 1: Panic port failures     */
#define	SCA_PANIC2		 2	/*  Level 2: Panic port and open     */
					/*	     path failures	     */
#define	SCA_PANIC3		 3	/*  Level 3: Panic port and all path */
					/*	     failures		     */
#ifndef	SCA_PANIC
#define	SCA_PANIC	SCA_PANIC0	/*  Default: Recover all conditions  */
#endif /*	SCA_PANIC */

					/* Maximum Numbers of Paths/Hosts    */
#define	CIPPD_MAXPATHS		32	/*  CI PPD			     */
		 			/*  INET$RFC_790		     */
#define	SCSNET_MAXHOSTS	    ( CIPPD_MAXPATHS )

					/* Remote SYSAP Buffer Requirements  */
#define	BVPSSP_CREDITS		15	/*  BVPSSP Disk/Tape Server	     */
#define	DSSC_CREDITS		10	/*  DSSC Disk/Tape Server	     */
#define	HSC_CREDITS		25	/*  HSC Disk/Tape Server	     */
#define	SCSNET_CREDITS		 5	/*  INET$RFC_790		     */

/* Maximum Number of SCS Connections =
 *	SYS$DIRECTORY:			Listener + 4 Connections
 *	INET$RFC_790:			Listener + 1/Remote Host
 *	Disk and Tape Clients:		2 Listeners + 2/HSC Controller
 *						    + 1/UQ Controller
 *						    + 1/BVPSSP Controller
 *						    + 1/DSSC Controller
 *	MOP Client: 			1/HSC Controller
 */
#define	SCS_MAX_CONNS							\
    (( 1 + 4 )					  +			\
     ( 1 + ( SCSNET_MAXHOSTS * NSCSNET ))	  +			\
     ( 2 + ( 2 * NHSC ) + NUQ + NBVPSSP + NDSSC ) +			\
     NHSC )

/* Maximum Number of GVP Buffer Descriptors =
 * For HSC Disk & Tape Servers:   2/HSC Ctlr * ( Max HSC Credits + 1 )
 * For BVPSSP Disk/Tape Servers:  1/BVPSSP Ctlr * ( Max BVPSSP Credits + 1 )
 * For DSSC Disk/Tape Servers:    1/DSSC Ctlr * ( Max DSSC Credits + 1 )
 * For INET$RFC_790 Servers:      Max Remote Hosts * ( MAX SCSNET Credits + 1 )
 */
#define	GVP_MAX_BDS		    					\
    (( 2 * NHSC * ( HSC_CREDITS + 1 ))	 +				\
     ( NBVPSSP * ( BVPSSP_CREDITS + 1 )) +				\
     ( NDSSC * ( DSSC_CREDITS + 1 ))     +				\
     ( NSCSNET * (( SCSNET_CREDITS + 1 ) * SCSNET_MAXHOSTS )))
