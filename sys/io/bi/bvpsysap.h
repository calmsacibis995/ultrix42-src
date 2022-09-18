/*
	@(#)bvpsysap.h	4.1	(ULTRIX)	7/2/90
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

/*	BVP Constants
 */

#define	BVP_NOLOG	32		/* Number of logout entries	*/
#define	BVP_LEV_14	1		/* IPL 14  for PIV		*/
#define	BVP_LEV_15	2		/* IPL 15  for PIV		*/
#define	BVP_LEV_16	4		/* IPL 16  for PIV		*/
#define	BVP_LEV_17	8		/* IPL 17  for PIV		*/

/*	BVP flag word definitions
 */

#define	BVP_TIM		1		/* Timer on/off			*/


/* BVP Data Structure Definitions.
 */
typedef struct _bvp_ssplpib	{	/* BVP SSP local port information   */
	u_long	bvp_type;		/* Port device type		    */
	u_short	bvp_flags;		/* Flag word			    */
    } BVPSSPLPIB;
