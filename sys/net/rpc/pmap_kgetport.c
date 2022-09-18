#ifndef lint
static	char	*sccsid = "@(#)pmap_kgetport.c	4.3	(ULTRIX)	4/25/91";
#endif
/************************************************************************
 *									*
 *			Copyright (c) 1986, 1988 by			*
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
 * pmap_kgetport.c
 * Kernel interface to pmap rpc service.
 *
 * Copyright (C) 1986, Sun Microsystems, Inc.
 */

/*
 *
 *   Modification history:
 *
 * 08 Mar 91 -- dws
 *	Initialize root credential in pmap_kgetport().
 *
 * 10-Jun-88 -- jaw 
 * 	add parameter to ISSIG for SMP.... this makes going to stop
 *	state atomic.
 *
 * 02-Mar-87 -- logcher
 *	New file for diskless 
 */

#include "../h/types.h"
#include "../h/smp_lock.h"
#include "../rpc/types.h"
#include "../rpc/xdr.h"
#include "../rpc/auth.h"
#include "../rpc/clnt.h"
#include "../rpc/rpc_msg.h"
#include "../rpc/pmap_prot.h"
#include "../h/time.h"
#include "../h/socket.h"
#include "../net/if.h"
#include "../h/param.h"
#include "../netinet/in.h"
#include "../h/user.h"
#include "../h/proc.h"

#define retries 4
static struct timeval tottimeout = { 1, 0 };


/*
 * Find the mapped port for program,version.
 * Calls the pmap service remotely to do the lookup.
 *
 * The 'address' argument is used to locate the portmapper, then
 * modified to contain the port number, if one was found.  If no
 * port number was found, 'address'->sin_port returns unchanged.
 *
 * Returns:	 0  if port number successfully found for 'program'
 *		-1  (<0) if 'program' was not registered
 *		 1  (>0) if there was an error contacting the portmapper
 */
int
pmap_kgetport(address, program, version, protocol)
	struct sockaddr_in *address;
	u_long program;
	u_long version;
	u_long protocol;
{
	u_short port = 0;
	register CLIENT *client;
	struct pmap parms;
	int error = 0, i;
	struct sockaddr_in tmpaddr;
	struct ucred *tmpcred, *savecred;

	/* Set up credential */
        tmpcred = crdup(u.u_cred);
	savecred = u.u_cred;
	u.u_cred = tmpcred;
	u.u_uid = 0;
	u.u_gid = 0;
	for (i=1; i<NGROUPS; i++)
		u.u_groups[i] = NOGROUP;
	
	/* copy 'address' so that it doesn't get trashed */
	tmpaddr = *address;

	tmpaddr.sin_port = htons(PMAPPORT);
	client = clntkudp_create(&tmpaddr, PMAPPROG, PMAPVERS, 
		 retries, u.u_cred);
	if (client != (CLIENT *)NULL) {
		parms.pm_prog = program;
		parms.pm_vers = version;
		parms.pm_prot = protocol;
		parms.pm_port = 0;  /* not needed or used */
		if (CLNT_CALL(client, PMAPPROC_GETPORT, xdr_pmap, &parms,
		    xdr_u_short, &port, tottimeout) != RPC_SUCCESS){
			error = 1;	/* error contacting portmapper */
		} else if (port == 0) {
			error = -1;	/* program not registered */
		} else {
			address->sin_port = htons(port);	/* save the port # */
		}
		AUTH_DESTROY(client->cl_auth);
		CLNT_DESTROY(client); 
	}

	/*
	 * Reset credentials
	 */
	u.u_cred = savecred;
	crfree(tmpcred);
	return (error);
}

/*
 * getport_loop -- kernel interface to pmap_kgetport()
 *
 * Talks to the portmapper using the sockaddr_in supplied by 'address',
 * to lookup the specified 'program'.
 *
 * Modifies 'address'->sin_port by rewriting the port number, if one
 * was found.  If a port number was not found (ie, return value != 0),
 * then 'address'->sin_port is left unchanged.
 *
 * If the portmapper does not respond, prints console message (once).
 * Retries forever, unless a signal is received.
 *
 * Returns:	 0  the port number was successfully put into 'address'
 *		-1  (<0) the requested process is not registered.
 *		 1  (>0) the portmapper did not respond and a signal occurred.
 */
getport_loop(address, program, version, protocol)
	struct sockaddr_in *address;
	u_long program;
	u_long version;
	u_long protocol;
{
	register int pe = 0;
	register int i = 0;

	/* sit in a tight loop until the portmapper responds */
	while ((i = pmap_kgetport(address, program, version, protocol)) > 0) {

		/* test to see if a signal has come in */
		if (ISSIG(u.u_procp,0)) {
			printf("Portmapper not responding; giving up\n");
			goto out;		/* got a signal */
		}
		/* print this message only once */
		if (pe++ == 0) {
			printf("Portmapper not responding; still trying\n");
		}
	}				/* go try the portmapper again */

	/* got a response...print message if there was a delay */
	if (pe != 0) {
		printf("Portmapper ok\n");
	}
out:
	return(i);	/* may return <0 if program not registered */
}
