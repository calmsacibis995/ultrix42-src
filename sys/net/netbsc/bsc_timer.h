/*	@(#)bsc_timer.h	4.1	(ULTRIX)	7/2/90	*/

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
/*	bsc_timer.h					83/07/29	*/

/*	
 * Definitions of the BSC timers.  These timers are counted
 * down PR_SLOWHZ times a second.
 */
int MAX_RETRY;
#define LINE_BID_RETRY 	 20		/* Bid for the line a max of 20 times.
					 * Note that this is generous.
					 * It's here because of the 730.  May
					 * want to change to spec which
				  	 * requires 7 max retries.   */
#define SEND_BLK_RETRY   20		/* Try to resend a block 20 times. */
#define BSC_LINGERTIME	120		/* set socket linger time at 2 min */
#define	BSC_NTIMERS	2
#define OFF		NULL		/* flag to indicate that timer is
					 * turned off			*/

int	BSC_REXMT_TIMER;		/* retransmit */
int	BSC_READ_TIMER;			/* timer set when reading data */

#define	BSC_DATA_TIMER	( 1*PR_SLOWHZ)		/* keep alive - 45 secs */
#define	BSC_CTRL_TIMER	( 1*PR_SLOWHZ)		

