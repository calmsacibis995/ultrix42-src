/*	@(#)connection.h	4.1      ULTRIX 7/2/90 */

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
 * connection.h -- Connection object header file
 *
 * Description:
 *	This is the header file for the connection object
 *	The implementation is in connection.c
 *
 *	The externally available calls are declared extern here
 */

/* SCCS history beginning
 * ***************************************************************
 *                -- Revision History --
 * ***************************************************************
 * 
 * 1.1  02/06/88 -- thoms
 * date and time created 88/06/02 17:45:58 by thoms
 * 
 * ***************************************************************
 *
 * 1.2 12/07/88 -- thoms
 * Added copyright notice and modification history.
 * Improved the comments, made a few name changes for BL1
 *
 * ***************************************************************
 *
 * 1.3  21/07/88 -- thoms
 * cx_open and cx_close routines now return status.
 *
 * ***************************************************************
 *
 * 1.4  01/09/88 -- thoms
 * Added new connections types to use Berkeley compatible filters
 *
 * ***************************************************************
 *
 * 1.5  07/09/88 -- thoms
 * Made Berkeley style output filters the default
 *
 * ****************************************************************
 *
 * 1.6  16/10/88 -- thoms
 * Removed dev_2.2 and lat_2.2 (used old style filter restart)
 * Added tcp for TCP Printserver support
 *
 * SCCS history end
 */

/*
 * This enumeration is used for the connection type
 * which is stored in the connection object on initialisation
 */
enum connection_type_e {
	con_dev, con_lat, con_remote, con_network, con_tcp, con_dqs
};

/*
 * This enumeration is used to record the connection state
 * Note that the start/stop state is maintained by the
 * filter object, if there is one and therefore does not
 * need to be tracked by the connection object as well
 */
enum cx_state_e { cxs_closed, cxs_open, };

/*
 * This structure is the implementation of the connection object
 */
struct connection {
	enum cx_state_e cx_state; /* connection state */
	int cx_out_fd;		/* output file descriptor (was ofd) */
	int cx_pr_fd;		/* printer file descriptor (was pfd) */
	FCP cx_output_filter;	/* the output filter */
	enum connection_type_e cx_type;
};

typedef struct connection *CXP;	/* short hand for object pointer */

/*
 * Structure for table of functions implementing operations
 * on connection object
 */
struct cx_fns {
	int (*cxf_open)(/* CXP */);	/* Open the connection */
	int (*cxf_close)(/* CXP */);	/* Close the connection */
	int (*cxf_stop)(/* CXP */);	/* Stop & bypass the filter */
	int (*cxf_start)(/* CXP */);	/* Restart the filter */
};

/*
 * These are the exported calls available on a connection object
 */

extern void cx_init(/* CXP cxp; enum connection_type_e connection_type */);
extern void cx_delete(/* CXP cxp; int on_heap */);

extern int cx_open(/* CXP cxp */);
extern int cx_close(/* CXP cxp */);
extern int cx_stop(/* CXP cxp */);
extern int cx_start(/* CXP cxp */);
