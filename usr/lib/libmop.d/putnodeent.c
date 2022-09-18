#ifndef lint
static	char	*sccsid = "@(#)putnodeent.c	4.1	(ULTRIX)	7/2/90";
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

#include <netdnet/dnetdb.h> 
#include <netdnet/node_params.h>
#include <ctype.h>
#include <stdio.h>
#include "vio.h"
#include <sys/socket.h>
#include <sys/errno.h>

extern int errno;
extern VFILE *_dn_vfile[];
extern int _dn_dbase;
extern struct dnet_nodeent *_dn_cur_nodep[];
extern struct dnet_nodeent *_dn_end_nodep[];

/*
 *		r e m n o d e b y n a m e
 *
 * Remove an node entry which matches a specified name
 *
 * Returns:		0 on success,
 *			-1 on failure
 *
 * Inputs:
 *	name		= Pointer to name to match
 *
 */
remnodebyname(name)
char *name;
{
	register struct dnet_nodeent *file_nodep;
	register struct dnet_nodeent *endp;
	register size = sizeof(file_nodep->dn_name);
	register char *up_name;
	char *node_to_upper();
	int status;

	if (strlen(name) > size)
	{
		errno = ENAMETOOLONG;
		return(-1);
	}
	up_name = node_to_upper(name);
	/*
	 * open the node database file, for writing
	 */
	if (setnodeentw(0) == -1)
		return(-1);

	file_nodep = _dn_cur_nodep[_dn_dbase];
	endp = _dn_end_nodep[_dn_dbase];
	/*
	 * assume node not found
	 */
	errno = EADDRNOTAVAIL;
	status = -1;
	/*
	 * search database matching on name
	 */
	for (; file_nodep < endp; file_nodep = (struct dnet_nodeent *) ((unsigned char *) file_nodep +  file_nodep->dn_esiz) )
	{
		if (*(file_nodep->dn_name) == *up_name)
		{
			if (bcmp(file_nodep->dn_name, up_name, size) == 0)
			{
				status = remove_node(file_nodep);
				break;
			}
		}
	}
	/*
	 * reset and close the database
	 */
	if (status != -1)
	{
		status = resetnodeent();
	}
	endnodeent();
	return(status);
}

/*
 *		r e m n o d e b y a d d r
 *
 * Remove an node entry which matches a specified address
 *
 * Returns:		0 on success,
 *			-1 on failure
 *
 * Inputs:
 *	addr		= pointer to address
 *	len		= length of address in bytes
 *	type		= AF_DECnet
 *
 */
remnodebyaddr(addr, len, type)
char *addr;
int len;
int type;
{
	register struct dnet_nodeent *file_nodep;
	register struct dnet_nodeent *endp;
	register u_short address = *(u_short *)(addr);
	int status;

	/*
	 * if not removing a DECnet address, return error
	 */
	if ((type != AF_DECnet) || (len != sizeof(address)) || address == 0)
	{
		errno = EPROTONOSUPPORT;
		return(-1);
	}
	/*
	 * open the node database file, for writing
	 */
	if (setnodeentw(0) == -1)
		return(-1);

	file_nodep = _dn_cur_nodep[_dn_dbase];
	endp = _dn_end_nodep[_dn_dbase];
	/*
	 * assume node not found
	 */
	errno = EADDRNOTAVAIL;
	status = -1;
	/*
	 * seach the database, matching on address
	 */
	for (; file_nodep < endp; file_nodep = (struct dnet_nodeent *) ((unsigned char *) file_nodep +  file_nodep->dn_esiz) )
	{
		if (file_nodep->dn_addr == address)
		{
			status = remove_node(file_nodep);
			break;
		}
	}
	/*
	 * reset and close the database
	 */
	if (status != -1)
	{
		status = resetnodeent();
	}
	endnodeent();
	return(status);
}

/*
 *		r e m o v e _ n o d e
 *
 * Update the node database file, deleting the node entry specified.
 *
 * Returns:		0 on success
 *			-1 on failure
 *
 * Inputs:
 *	dnet_nodeent	= Pointer to node entry in memory
 *
 */
remove_node(dnet_nodeent)
struct dnet_nodeent *dnet_nodeent;
{
	int pos = ((caddr_t)dnet_nodeent - _dn_vfile[_dn_dbase]->v_base);
	int size = (_dn_vfile[_dn_dbase]->v_size - (pos + dnet_nodeent->dn_esiz));
	dnet_nodeent = (struct dnet_nodeent *) ( (u_char *) dnet_nodeent + dnet_nodeent->dn_esiz );
	return(write_nodes(dnet_nodeent, size, pos, 1));
}

/*
 *		w r i t e _ n o d e s
 *
 * Write the specified node entries to the specified position in the file
 *
 * Returns:		0 on success
 *			-1 on failure
 *
 * Inputs:
 *	nodep		= Pointer to start of node entries to be written
 *	size		= size of list (in bytes)
 *	offset		= offset into file to write the entries
 *	truncate	= if not 0, trucate file after write
 *
 */
write_nodes(nodep, size, offset, truncate)
struct dnet_nodeent *nodep;
int size;
int offset;
int truncate;
{
	int status;
	/*
	 * seek to the offset in the file
	 */
	status = lseek(_dn_vfile[_dn_dbase]->v_fd, offset, 0);
	/* 
	 * if the seek succeeded, write the node entries
	 */
	if (status != -1)
		status = write(_dn_vfile[_dn_dbase]->v_fd, nodep, size);
	/*
	 * if the write succeeded, and if requested, truncate the file
	 */
	if ((status != -1) && (truncate != 0))
		status = ftruncate(_dn_vfile[_dn_dbase]->v_fd, offset + size);
	return(status);
}

/*
 *		a d d n o d e e n t
 *
 * Insert an node entry in the node database, sorted by address
 *
 * Returns:		0 on success,
 *			-1 on failure
 *
 * Inputs:
 *	nodeent		= pointer to node entry to be added
 *
 */
addnodeent(nodeent)
struct nodeent *nodeent;
{
	char *malloc();
	struct dnet_nodeent *dnet_nodeent;
	register struct dnet_nodeent *file_nodep;
	register struct dnet_nodeent *endp;
	register u_short address = *(u_short *)(nodeent->n_addr);
	char *up_name, *node_to_upper();
	unsigned short parm_size;
	int status;
	int pos;
	int size;

	/*
	 * check the format of the entry to be added
	 */
	if ((strlen(nodeent->n_name) > sizeof(dnet_nodeent->dn_name)) ||
	(nodeent->n_length != sizeof(dnet_nodeent->dn_addr)) ||
	(nodeent->n_addrtype != AF_DECnet) ||
	( address == 0  && strlen(nodeent->n_name) == 0 ))
	{
		errno = EAFNOSUPPORT;
		return(-1);
	}
	if ( nodeent->n_params )
	{ 
		if ( ! (parm_size = get_parmblk_siz(nodeent->n_params)) )
		{
			errno = EAFNOSUPPORT;
			return(-1);
		}
	}
	else
	{
		parm_size = 0;
	}
	up_name = node_to_upper(nodeent->n_name);

	/*
	 * open the node database file, for writing
	 */
	if (setnodeentw(0) == -1)
		return(-1);

	file_nodep = _dn_cur_nodep[_dn_dbase];
	endp = _dn_end_nodep[_dn_dbase];

	/*
	 * scan database while addresses are lower than the one being added
	 */
	for (; file_nodep < endp; file_nodep = (struct dnet_nodeent *) ((unsigned char *) file_nodep +  file_nodep->dn_esiz) )
	{
		if (file_nodep->dn_addr >= address)
		break;
	}

	/*
	 * if an entry with the same address (except for 0) is found:
	 *	return an error.
	 */
	if (file_nodep < endp && file_nodep->dn_addr == address && address != 0)
	{
		endnodeent();
		errno = EADDRINUSE;
		return(-1);
	}

	/*
	 * build the node entry and write it to the file,
	 * then write the remainder of the nodeentries out
	 */
	if ( ! (dnet_nodeent = (struct dnet_nodeent *) malloc( sizeof(struct dnet_nodeent) + parm_size)) )
	{
		errno = ENOBUFS;
		return(-1);
	}
	dnet_nodeent->dn_esiz = sizeof(struct dnet_nodeent) + parm_size;
	dnet_nodeent->dn_addr = address;
	strncpy(dnet_nodeent->dn_name, up_name, sizeof(dnet_nodeent->dn_name));
	if ( parm_size )
	{
		bcopy(nodeent->n_params, (unsigned char *) (dnet_nodeent) + sizeof(struct dnet_nodeent), parm_size);
	}
	pos = ((caddr_t)file_nodep - _dn_vfile[_dn_dbase]->v_base);
	status = write_nodes(dnet_nodeent, dnet_nodeent->dn_esiz, pos, 0);
	if (status != -1)
	{
		size = (_dn_vfile[_dn_dbase]->v_size) - pos;
		pos += dnet_nodeent->dn_esiz;
		status = write_nodes(file_nodep, size, pos, 1);
	}
	/*
	 * reset and close the database
	 */
	if (status != -1)
	{
		status = resetnodeent();
	}
	endnodeent();
	return(status);
}


/*
 *		g e t _ p a r m b l k _ s i z
 *
 * Calculate size of parameter block
 *
 * Input:		pointer to parameter block
 *
 * Returns:		size of parameter block
 *
 *	
 */
get_parmblk_siz(pblk)
register unsigned char *pblk;
{
	int size = 0;
	unsigned short parm_size;

	while ( *(u_short *) &pblk[size] != NODE_END_PARAM &&  size < NODE_MAX_PSIZ )
	{
		parm_size = *(u_short *) &pblk[size + NODE_CSIZ];
		size += parm_size + NODE_PARM_HDRSIZ;
	}

	if ( size > NODE_MAX_PSIZ )
	{
		return(NULL);
	}
	else
	{
		return(size);
	}
}

/*
 *		n o d e _ t o _ u p p e r
 *
 * Converts a supplied node name to upper case.
 *
 * Returns:		Pointer to upper case node name
 *
 * Inputs:
 *	name		= supplied name (may be any case)
 *
 */
static char *node_to_upper(name)
register char *name;
{
	static char up_name[ND_MAXNODE + 1];
	register char *cp = up_name;
	register char c;

	bzero(up_name, sizeof(up_name));
	while (c = *name++)
	{
		if (islower(c))
			c = toupper(c);
		*cp++ = c;
	}
	return(up_name);
}
