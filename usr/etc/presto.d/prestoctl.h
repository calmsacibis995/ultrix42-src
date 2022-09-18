/*	@(#)prestoctl.h	4.1	(ULTRIX)	10/8/90	*/

/************************************************************************
 *									*
 *			Copyright (c) 1990 by				*
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
 *	Portions of this software have been licensed to 
 *	Digital Equipment Company, Maynard, MA.
 *	Copyright (c) 1990 Legato Systems, Inc.  ALL RIGHTS RESERVED.
 */

/*
 *
 *   Modification history:
 *
 *  23 Aug 90 -- chet
 *	Added this file; it was derived from Legato sources.
 *
 */

#include <rpc/types.h>

bool_t xdr_battery();
bool_t xdr_prstates();
bool_t xdr_io();
bool_t xdr_presto_status();
bool_t xdr_presto_modstat();

struct presto_fs_status {
	bool_t pfs_prestoized;
	prstates pfs_state;
	bool_t pfs_enabled;
	bool_t pfs_bounceio;
	bool_t pfs_unused;
};
typedef struct presto_fs_status presto_fs_status;
bool_t xdr_presto_fs_status();

struct presto_get_fs_status {
	bool_t succeeded;
	union {
		char *errmsg;
		presto_fs_status status;
	} presto_get_fs_status_u;
};
typedef struct presto_get_fs_status presto_get_fs_status;
bool_t xdr_presto_get_fs_status();

#define PRESTOCTLPROG ((u_long)390100)
#define PRESTOCTLVERS ((u_long)3)
#define PRESTOCTL_GETSTATE ((u_long)1)
extern presto_modstat *prestoctl_getstate_3();
#define PRESTOCTL_SETBYTES ((u_long)2)
extern presto_modstat *prestoctl_setbytes_3();
#define PRESTOCTL_TOGGLE ((u_long)3)
extern presto_modstat *prestoctl_toggle_3();
#define PRESTOCTL_GET_FS_STATUS ((u_long)4)
extern presto_get_fs_status *prestoctl_get_fs_status_3();
