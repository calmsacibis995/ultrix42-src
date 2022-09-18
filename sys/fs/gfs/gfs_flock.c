#ifndef lint
static char *sccsid = "@(#)gfs_flock.c	4.1	ULTRIX	7/2/90";
#endif

/************************************************************************
 *									*
 *			Copyright (c) 1986 by				*
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

/************************************************************************
 *
 *			Modification History
 *
 *	Fred Glover	1/26/88
 *	Move the gfs_flock routines to ufs_flock.c. These routines are now
 *	used to support only the kernel based UFS only Sys-V locking 
 *	functionality
 *
 *	Fred Glover	1/12/88
 *	modified Sys-V locking routines s.t. the validity checks were
 *	removed from these routines, and added to the fcntl interface
 *	routines in gfs_descrip.c.
 *
 *	Paul Shaughnessy (prs), 15-Sept-86
 *	Changed DEADLOCK index to DELLOCK.
 *
 *	koehler 11 Sep 86
 *	unregisterized dev_t
 *
 *	Stephen Reilly, 09-Sept-85
 *	Created to handle the lockf call.
 *
 ***********************************************************************/

