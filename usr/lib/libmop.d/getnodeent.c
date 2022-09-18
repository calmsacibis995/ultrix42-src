#ifndef lint
static	char	*sccsid = "@(#)getnodeent.c	4.1	(ULTRIX)	7/2/90";
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
#include <stdio.h>
#include <ctype.h>
#include "vio.h"
#include <sys/socket.h>
#include <sys/errno.h>

#define MAXADDRSIZE 4
#define MAXNAMESIZE 6

extern int errno;
int _dn_dbase = ND_VOLATILE;
static char *dbname[] = { "/usr/lib/dnet/nodes_v", "/usr/lib/dnet/nodes_p" };
static char *dbmode[] = { "r", "r" };
static int isopen[2];
VFILE *_dn_vfile[2];
struct dnet_nodeent *_dn_cur_nodep[2];
struct dnet_nodeent *_dn_end_nodep[2];
unsigned short db_version[2];
unsigned short db_vrsnumber = ND_VERSION;

static struct nodeent node;
static u_char *parm = NULL;
static char nodeaddr[MAXADDRSIZE];
static char nodename[MAXNAMESIZE + 1];


/*
 *		s e t n o d e e n t
 *
 * Open the node database
 *
 * Returns:		0 on success,
 *			-1 on error
 *
 * Inputs:
 *	flag		= indicator if file is to be kept open (ignored)
 *	
 */
setnodeent(flag)
int flag;
{
	struct db_version vse;

	if (isopen[_dn_dbase] == 0)
	{
		/*
		 * if the file is open, close it
		 */
		if (_dn_vfile[_dn_dbase] != NULL)
		{
			_vclose(_dn_vfile[_dn_dbase]);
			_dn_vfile[_dn_dbase] = NULL;
		}
		/*
		 * open the node database file
		 */
		if ((_dn_vfile[_dn_dbase] = _vopen(dbname[_dn_dbase], dbmode[_dn_dbase])) == NULL)
		return(-1);
	}
	isopen[_dn_dbase]++;
	/*
	 * set the current postion in the file to the start, and
	 * set the pointer to the end of the database
	 */
	if ( _dn_vfile[_dn_dbase]->v_size == 0 && *dbmode[_dn_dbase] == 'w' )
	{
		vse.vs_esiz = sizeof(struct db_version);
		vse.vrsn_number = db_vrsnumber;
		vse.reserved = 0;
		if ( write(_dn_vfile[_dn_dbase]->v_fd, &vse, sizeof(vse)) < 0 )
		{
			endnodeent();
			return(-1);
		}
		endnodeent();
		return(setnodeent(flag));
	}
	else if ( (((struct dnet_nodeent *) (_dn_vfile[_dn_dbase]->v_base))->dn_esiz) < sizeof(struct dnet_nodeent) &&
	     (db_version[_dn_dbase] = ((struct db_version *) (_dn_vfile[_dn_dbase]->v_base))->vrsn_number) == db_vrsnumber )
	{
		_dn_cur_nodep[_dn_dbase] = (struct dnet_nodeent *)(_dn_vfile[_dn_dbase]->v_base + sizeof(struct db_version));
		_dn_end_nodep[_dn_dbase] = (struct dnet_nodeent *)(_dn_vfile[_dn_dbase]->v_base + _dn_vfile[_dn_dbase]->v_size);
		return(NULL);
	}
	else
	{
		endnodeent();
		errno = EFAULT;
		return(-1);
	}
}

/*
 *		s e t n o d e e n t w
 *
 * Open the node database, for write access
 *
 * Returns:		0 on success,
 *			-1 on error
 *
 * Inputs:
 *	flag		= indicator if file is to be kept open (ignored)
 *	
 */
setnodeentw(flag)
int flag;
{
	int status;
	*dbmode[_dn_dbase] = 'w';	/* change mode for open to write */
	status = setnodeent(flag);
	*dbmode[_dn_dbase] = 'r';	/* reset mode for future opens to read */
	return(status);
}

/*
 *		e n d n o d e e n t
 *
 * Terminate access to the node database
 *
 * Returns:		None
 *
 * Inputs:		None
 *	
 */
endnodeent()
{
	if (--isopen[_dn_dbase] == 0)
	{
		/*
		 * only close the file if it is open
		 */
		if (_dn_vfile[_dn_dbase] != NULL)
		{
			_vclose(_dn_vfile[_dn_dbase]);
			_dn_vfile[_dn_dbase] = NULL;
		}
	}
}


/*
 *		r e s e t n o d e e n t
 *
 * Reread the node database
 *
 * Returns:		0 on success,
 *			-1 on error
 *
 * Inputs:
 *	
 */
resetnodeent()
{
	/*
	 * open the node database file
	 */
	if (_vreload(_dn_vfile[_dn_dbase]) < 0)
		return(-1);

	/*
	 * set the current postion in the file to the start, and
	 * set the pointer to the end of the database
	 */
	if ( (((struct dnet_nodeent *) (_dn_vfile[_dn_dbase]->v_base))->dn_esiz) < sizeof(struct dnet_nodeent) &&
	     (db_version[_dn_dbase] = ((struct db_version *) (_dn_vfile[_dn_dbase]->v_base))->vrsn_number) == db_vrsnumber )
	{
		_dn_cur_nodep[_dn_dbase] = (struct dnet_nodeent *)(_dn_vfile[_dn_dbase]->v_base + sizeof(struct db_version));
		_dn_end_nodep[_dn_dbase] = (struct dnet_nodeent *)(_dn_vfile[_dn_dbase]->v_base + _dn_vfile[_dn_dbase]->v_size);
		return(NULL);
	}
	else
	{
		endnodeent();
		errno = EFAULT;
		return(-1);
	}
}

/*
 *		s e t n o d e d b
 *
 * Set the node database file to use
 *
 * Returns:		None
 *
 * Inputs:
 *	dbase		= Database file to use:
 *				0 = volatile database
 *				1 = permanent database
 *
 */
setnodedb(dbase)
int dbase;
{
	_dn_dbase = dbase;
}

/*
 *		g e t n o d e d b a s e
 *
 * Get the index of the current node database file
 *
 * Returns:		Index of the currently active node database
 *
 * Inputs:		None
 */
getnodedbase()
{
	return (_dn_dbase);
}

/*
 *		g e t n o d e d b
 *
 * Get the name of the current node database file
 *
 * Returns:		Pointer to the name string
 *
 * Inputs:		None
 *
 */
char *getnodedb()
{
	return(dbname[_dn_dbase]);
}

/*
 *		g e t n o d e e n t
 *
 * Get the next (or first) entry in the node database.
 *
 * Returns:		Pointer to node entry (if found), or NULL
 *
 * Inputs:		None
 *
 */
struct nodeent *getnodeent()
{
	/*
	 * if the file is not open, try to open it
	 */
	if (_dn_vfile[_dn_dbase] == NULL)
	{
		if (setnodeent(0) == -1)
			return(NULL);
	}
	/*
	 * if not at the end of the file, return the next node entry
	 */
	if (_dn_cur_nodep[_dn_dbase] < _dn_end_nodep[_dn_dbase])
	{
		format_node(_dn_cur_nodep[_dn_dbase]);
		_dn_cur_nodep[_dn_dbase] = (struct dnet_nodeent *) ( (u_char *) _dn_cur_nodep[_dn_dbase] + _dn_cur_nodep[_dn_dbase]->dn_esiz );
		return(&node);
	}
	return(NULL);
}

/*
 *		g e t n o d e b y n a m e
 *
 * Get a node entry with a matching name
 *
 * Returns:		Pointer to node entry (if found), or NULL
 *
 * Inputs:
 *	name		= pointer to name
 *
 */
struct nodeent *getnodebyname(name)
char *name;
{
	register struct dnet_nodeent *file_nodep;
	register struct dnet_nodeent *endp;
	register char *up_name;
	register size = sizeof(file_nodep->dn_name);
	char *node_to_upper();
	struct dnet_nodeent *savecur = _dn_cur_nodep[_dn_dbase];
	struct dnet_nodeent *saveend = _dn_end_nodep[_dn_dbase];

	if (strlen(name) > size)
	{
		errno = ENAMETOOLONG;
		return(NULL);
	}
	up_name = node_to_upper(name);

	/*
	 * read the node database
	 */
	if (setnodeent(0) == -1)
		return(NULL);

	file_nodep = _dn_cur_nodep[_dn_dbase];
	endp = _dn_end_nodep[_dn_dbase];

	/*
	 * indicate no match found (yet)
	 */
	node.n_addr = NULL;

	/*
	 * look through database for match on name
	 */
	for (; file_nodep < endp; file_nodep = (struct dnet_nodeent *) ((unsigned char *) file_nodep +  file_nodep->dn_esiz) )
	{
		if (*(file_nodep->dn_name) == *up_name)
		{
			if (strncmp(file_nodep->dn_name, up_name, size) == 0)
			{
				if ( (errno = format_node(file_nodep)) != NULL )
				{
					endnodeent();
					_dn_cur_nodep[_dn_dbase] = savecur;
					_dn_end_nodep[_dn_dbase] = saveend;
					return(NULL);
				}
				break;
			}
		}
	}

	/*
	 * close the database and restore the current database pointers.
	 */
	endnodeent();
	_dn_cur_nodep[_dn_dbase] = savecur;
	_dn_end_nodep[_dn_dbase] = saveend;

	/*
	 * if no match found, return NULL, else format and return nodeent
	 */
	if (node.n_addr == NULL)
	{
		errno = EADDRNOTAVAIL;
		return(NULL);
	}

	return(&node);
}

/*
 *		g e t n o d e b y a d d r
 *
 * Get a node entry with a matching address
 *
 * Returns:		Pointer to node entry (if found), or NULL
 *
 * Inputs:
 *	addr		= pointer to address
 *	len		= length of address in bytes
 *	type		= AF_DECnet
 *
 */
struct nodeent *getnodebyaddr(addr, len, type)
char *addr;
int len;
int type;
{

	register struct dnet_nodeent *file_nodep;
	register struct dnet_nodeent *endp;
	register u_short address = *(u_short *)addr;
	struct dnet_nodeent *savecur = _dn_cur_nodep[_dn_dbase];
	struct dnet_nodeent *saveend = _dn_end_nodep[_dn_dbase];

	/*
	 * if not looking for DECnet address, return error
	 */
	if ((type != AF_DECnet) || (len != sizeof(address)))
	{
		errno = EPROTONOSUPPORT;
		return(NULL);
	}
	/*
	 * read the node database
	 */
	if (setnodeent(0) == -1)
		return(NULL);

	file_nodep = _dn_cur_nodep[_dn_dbase];
	endp = _dn_end_nodep[_dn_dbase];

	/*
	 * indicate no match found (yet)
	 */
	node.n_addr = NULL;
	/*
	 * look through database form matching address
	 */
	for (; file_nodep < endp; file_nodep = (struct dnet_nodeent *) ((unsigned char *) file_nodep +  file_nodep->dn_esiz) )
	{
		if (file_nodep->dn_addr == address)
		{
			if ( (errno = format_node(file_nodep)) != NULL )
			{
				endnodeent();
				_dn_cur_nodep[_dn_dbase] = savecur;
				_dn_end_nodep[_dn_dbase] = saveend;
				return(NULL);
			}
			break;
		}
	}
	/*
	 * close the database and restore the access pointerS
	 */
	endnodeent();
	_dn_cur_nodep[_dn_dbase] = savecur;
	_dn_end_nodep[_dn_dbase] = saveend;
	/*
	 * if no match found, return NULL, else format and return nodeent
	 */
	if (node.n_addr == NULL)
	{
		errno = EADDRNOTAVAIL;
		return(NULL);
	}
	return(&node);
}

/*
 *		f o r m a t _ n o d e
 *
 * This routine is called to build a "nodeent" structure from the
 * internal node file format.
 *
 * Returns:		node entry build in "node"
 *
 * Inputs:
 *	dnet_nodeent	= pointer to node structure in internal format
 *
 */
static format_node(dnet_nodeent)
register struct dnet_nodeent *dnet_nodeent;
{
	unsigned char *malloc();

	/*
	 * free any existing allocated memory
	 */
	if ( parm )
		free( parm );

	bcopy(dnet_nodeent->dn_name, nodename, sizeof(dnet_nodeent->dn_name));
	nodename[sizeof(dnet_nodeent->dn_name)] = NULL;
	node.n_name = nodename;
	node.n_length = sizeof(u_short);
	node.n_addr = nodeaddr;
	node.n_addrtype = AF_DECnet;
	*((u_short *)node.n_addr) = dnet_nodeent->dn_addr;

	/*
	 * pass back any parameters associated with node entry.
	 */
	if ( dnet_nodeent->dn_esiz > sizeof(struct dnet_nodeent) )
	{
		if ( ! (parm = malloc(dnet_nodeent->dn_esiz)) )
		{
			return(ENOBUFS);
		}
		bcopy((unsigned char *) dnet_nodeent + sizeof(struct dnet_nodeent), parm, dnet_nodeent->dn_esiz);
		node.n_params = parm;
		*(parm + (dnet_nodeent->dn_esiz - sizeof(struct dnet_nodeent))) = NODE_END_PARAM;
	}
	else
	{
		node.n_params = NULL;
	}
	return(NULL);
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
