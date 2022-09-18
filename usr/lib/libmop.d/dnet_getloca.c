#ifndef lint
static	char	*sccsid = "@(#)dnet_getloca.c	4.1	7/2/90";
#endif lint

/*
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

#include <stdio.h>
#include <sys/types.h>			/* system type definitions */
#include <sys/socket.h>			/* socket definitions */
#include <netdnet/dn.h>
#include <netdnet/nsp_addr.h>

/*
 *	d n e t _ g e t l o c a d d
 *
 * This routine is called  to get the address of the local node.
 *
 * Returns:		pointer to address of local node or
 *			NULL if error
 *
 * Inputs:
 *	which		= 0 if volatile database to be used
 *			= 1 if permanent database to be used
 */
struct dn_naddr *dnet_getlocadd(which)
int which;
{
	static struct dn_naddr naddr = { sizeof(unsigned short) };
	int sock;
	int kaddress;
	struct dn_naddr *address = NULL;
	FILE *fp;
	char *cp, buf[20];

	if (which == 0)
	{
		/*
		 * volatile database - read from kernel
		 */
		sock = socket(AF_DECnet, SOCK_SEQPACKET, 0);
		if (sock != -1)
		{
			if (ioctl(sock, SIOCGNETADDR, &kaddress) != -1)
			{
				*(unsigned short *)naddr.a_addr = (unsigned short)kaddress;
				address = &naddr;
			}
			close(sock);
		}
	}
	else
	{
		/*
		 * permanent database - read from file
		 */
		if (fp = fopen("/usr/lib/dnet/exeadd_p", "r"))
		{
			if (fgets(buf, sizeof(buf), fp))
			{
				if (cp = (char *)index(buf, '\n'))
					*cp = NULL;
				if (address = (struct dn_naddr *)dnet_addr(buf))
				{
					naddr = *address;
					address = &naddr;
				}
			}
			fclose(fp);
		}
	}
	return(address);
}

/*
 *		g e t n o d e a d d
 *
 * This routine is called to get the local node address. It returns
 * the volatile database's address
 *
 * Returns:		pointer to node address or
 *			NULL if error
 *
 * Inputs:		None
 *
 */
struct dn_naddr *getnodeadd()
{
	return(dnet_getlocadd(0));
}
