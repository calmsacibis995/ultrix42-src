/* static char *sccsid = "@(#)lat.h	4.1.1.3	3/15/88"; */

/************************************************************************
 *									*
 *			Copyright (c) 1984 by				*
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

/************************************************************************
 *			Modification History				*
 *									*
 *	Larry Cohen  -	09/16/85					*
 * 		protosw and ETHERTYPE changes caused by subnet routing  *
 *									*
 ************************************************************************/

/*	lat.h	0.0	12/11/84	*/

#define LATPROTO_CTL		0	/* control protocol for socket call */
#define LATPROTO_TRACE		1	/* trace protocol for socket call */
#define LATBUFF_TRACE	    16384	/* trace default socket buffering */

/*
 * set/getsockopt calls for LAT control.
 * Note that for LAT_DIRMSG and LAT_STATE which operate on an individual
 * service class, the function codes are compound values of the form:
 *		((service_class) << 8) | LAT_xxx
 */

#define LAT_COUNTERS		1	/* read/zero LAT counters */
#define LAT_PARAMS		2	/* get/set LAT parameters */
#define LAT_DIRMSG		3	/* get/set LAT directory message */
#define LAT_STATE		4	/* get/set LAT state */
#define LAT_SERVPORT		5	/* get server/port */

/*
 * LAT counters data structure returned from LAT_COUNTERS request.
 */
struct lat_counters
    {
	u_int	lco_rcvframes;		/* received frames */
	u_int	lco_rcvdup;		/* received duplicates */
	u_int	lco_xmtframes;		/* transmitted frames */
	u_int	lco_rexmt;		/* retransmissions */
	u_int	lco_badmsg;		/* illegal messages received */
	u_int	lco_badslots;		/* illegal slots received */
    };

/*
 * LAT parameters data structure for LAT_PARAMS.
 */
struct lat_params
    {
	u_char	lpm_version;		/* LAT protocol version */
	u_char	lpm_eco;		/* LAT protocol eco */
	u_short	lpm_mtimer;		/* multicast timer interval */
    };

/*
 * LAT state structure for LAT_STATE.
 */
struct lat_state
    {
	u_char	lst_state;		/* class state */
    };
#define LST_OFF		0		/* class is turned off */
#define LST_RUNNING	1		/* class is active */

extern struct domain latdomain;

/*
 * LAT parameters data structure for LAT_SERVPORT.
 */
struct lat_servport
    {
	u_char	lsp_namelen;		/* LAT server name len */
	u_char	lsp_name[17];		/* LAT server name */
	u_char	lsp_portlen;		/* LAT server port name len */
	u_char	lsp_port[17];		/* LAT server port name */
	u_char	lsp_destlen;		/* LAT destination len */
	u_char	lsp_dest[17];		/* LAT destination */
    };
