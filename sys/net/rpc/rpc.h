/*	@(#)rpc.h	4.1	(ULTRIX)	7/2/90	*/

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

/*
 *	Portions of this software have been licensed to 
 *	Digital Equipment Company, Maynard, MA.
 *	Copyright (c) 1986 Sun Microsystems, Inc.  ALL RIGHTS RESERVED.
 */

/*
 *
 *   Modification history:
 *
 * 01-May-90 -- thomas
 * DECwest ANSI mt 1990 May 01
 * Change __RPC.H__ to __RPC_H__ for ANSI
 *
 * 02-Mar-87 -- logcher
 *	Merged in diskless changes, added new includes
 */

/*
 * rpc.h, Just includes the billions of rpc header files necessary to 
 * do remote procedure calling.
 *
 */
#ifndef __RPC_H__
#define __RPC_H__

#ifdef KERNEL
#include "../h/types.h"
#include "../rpc/types.h"		/* some typedefs */
#include "../netinet/in.h"
#include "../h/time.h"			/* for auth_des.h */

/* external data representation interfaces */
#include "../rpc/xdr.h"		/* generic (de)serializer */

/* Client side only authentication */
#include "../rpc/auth.h"		/* generic authenticator (client side) */

/* Client side (mostly) remote procedure call */
#include "../rpc/clnt.h"		/* generic rpc stuff */

/* semi-private protocol headers */
#include "../rpc/rpc_msg.h"	/* protocol for rpc messages */
#include "../rpc/auth_unix.h"	/* protocol for unix style cred */
#include "../rpc/auth_des.h"	/* protocol for des style cred */

/* Server side only remote procedure callee */
#include "../rpc/svc.h"		/* service manager and multiplexer */
#include "../rpc/svc_auth.h"	/* service side authenticator */
#else /* KERNEL */

#include <sys/types.h>
#include <rpc/types.h>		/* some typedefs */
#include <netinet/in.h>

/* external data representation interfaces */
#include <rpc/xdr.h>		/* generic (de)serializer */

/* Client side only authentication */
#include <rpc/auth.h>		/* generic authenticator (client side) */

/* Client side (mostly) remote procedure call */
#include <rpc/clnt.h>		/* generic rpc stuff */

/* semi-private protocol headers */
#include <rpc/rpc_msg.h>	/* protocol for rpc messages */
#include <rpc/auth_unix.h>	/* protocol for unix style cred */
/* #include <rpc/auth_des.h>	/* protocol for des style cred */

/* Server side only remote procedure callee */
#include <rpc/svc.h>		/* service manager and multiplexer */
#include <rpc/svc_auth.h>	/* service side authenticator */
#endif /* KERNEL */
#endif /* __RPC_H__ */
