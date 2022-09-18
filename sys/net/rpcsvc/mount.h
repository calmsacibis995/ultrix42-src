/*	@(#)mount.h	4.2	(ULTRIX)	2/12/91	*/

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
 * 21-Jul-89 -- lebel
 *  Changed exports structure to accomodate allowing exports
 *  options at the directory level.  Removed expdir structure.
 *
 * 09-Mar-88 -- logcher
 *	Added a pointer to the new expdir structure in exports and
 *	added the expdir structure which allows the mountd to use
 *	a 2x2 matrix of exports rather than 1x1.
 *
 * 14-Jul-87 -- logcher
 *	Changed ex_flags in exports structure from short to u_int for 
 *	continuity with m_flags in mount structure.
 */

#define MOUNTPROG 100005
#define MOUNTPROC_MNT 1
#define MOUNTPROC_DUMP 2
#define MOUNTPROC_UMNT 3
#define MOUNTPROC_UMNTALL 4
#define MOUNTPROC_EXPORT 5
#define MOUNTPROC_EXPORTALL 6
#define MOUNTVERS_ORIG 1
#define MOUNTVERS 1

#ifndef svc_getcaller
#define svc_getcaller(x) (&(x)->xp_raddr)
#endif

bool_t xdr_path();
bool_t xdr_fhandle();
bool_t xdr_fhstatus();
bool_t xdr_mountlist();
bool_t xdr_exports();

struct mountlist {		/* what is mounted */
	char *ml_name;
	char *ml_path;
	struct mountlist *ml_nxt;
};

struct fhstatus {
	int fhs_status;
	fhandle_t fhs_fh;
};

/*
 * List of exported directories
 * An export entry with ex_groups
 * NULL indicates an entry which is exported to the world.
 */
struct exports {
	dev_t		 ex_dev;	      /* dev of directory */
	ino_t		 ex_ino;	     
	u_long		 ex_gen;	      
	char		 *ex_name;	      /* name of directory */
	struct groups	 *ex_groups;  /* groups allowed to mount this entry */
	struct exports	 *ex_next;    /* next entry for this dev */
	short		  ex_rootmap;	  /* id to map root requests to */
	u_int		  ex_flags;	      /* bits to mask off file mode */
	struct exports   *ex_devptr;  /* pointer to next dev entry if this is
									 the top entry for this dev */
};

struct groups {
	char		*g_name;
	struct groups	*g_next;
/* private to mountd to identify hostnames */
	int 		g_hbit;
};
