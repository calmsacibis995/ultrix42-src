#ifndef	lint
static char *sccsid = "@(#)dli_open.c	4.3	ULTRIX	11/9/90";
#endif	lint

/*
 * Program dli_opent.c,  Module DLI 
 *
 * Copyright (C) 1985 by
 * Digital Equipment Corporation, Maynard, Mass.
 *
 * This software is furnished under a license and may be used and copied
 * only  in  accordance  with  the  terms  of such  license and with the
 * inclusion of the above copyright notice. This software or  any  other
 * copies thereof may not be provided or otherwise made available to any
 * other person. No title to and ownership of  the  software  is  hereby
 * transferred.
 *
 * The information in this software is subject to change without  notice
 * and  should  not be  construed  as  a commitment by Digital Equipment
 * Corporation.
 *
 * Digital assumes no responsibility for the use or  reliability  of its
 * software on equipment which is not supplied by Digital.
 *
 *
 * Networks & Communications Software Engineering
 *
 * IDENT HISTORY:
 *
 * 1.00 10-Jul-1985
 *      DECnet-ULTRIX   V1.0
 *
 */

#include "../../h/param.h"
#include "../../h/systm.h"
#include "../../h/mbuf.h"
#include "../../h/socket.h"
#include "../../h/socketvar.h"
#include "../../h/protosw.h"
#include "../../h/errno.h"
#include "../../h/smp_lock.h"
#include "../../h/cpudata.h"

#include "../../net/net/if.h"


#include "../../net/netinet/in.h"
#include "../../net/netinet/if_ether.h"

#include "../../net/netdnet/dli_var.h"

extern struct dli_line dli_ltable[];

extern struct lock_t lk_dli;



/*
 *		d l i _ o p e n
 *
 * Process a DLI open line request.
 *
 * Returns:		error code if error, otherwise NULL.
 *
 * Inputs:
 *	so		= Pointer to the socket for this request.
 */
dli_open( so )
register struct socket *so;
{
	
	register int i = 0;

	smp_lock(&lk_dli, LK_RETRY);
	while ( dli_ltable[i].dli_so != NULL && i < dli_maxline ) i++;
	if ( i == dli_maxline )
	{
		smp_unlock(&lk_dli);
		return(ENOBUFS);
	}
	dli_ltable[i].dli_so = so;
	so->so_pcb = ( caddr_t ) &dli_ltable[i];
	smp_unlock(&lk_dli);
	return(NULL);

}
