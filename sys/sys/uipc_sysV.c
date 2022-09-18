#ifndef lint
static  char    *sccsid = "@(#)uipc_sysV.c	4.3  (ULTRIX)        9/10/90";
#endif

/************************************************************************
 *									*
 *			Copyright (c) 1984 by				*
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
 *
 *   Modification history:
 *
 * 09 Aug 90 -- sekhar
 *	added latent support for memory mapped devices on vaxes.
 *
 * 30 Jul 90 -- sekhar
 *	in ipcget() include IPC_MMAP bit for key equivalance.
 *
 * 23 Apr 90 -- sekhar
 * 	Modifiied ipcget() for memory mapped devices: store IPC_MMAP in 
 * 	mode in ipc_permissions structure if it is set in the flags 
 *	(mips only).
 *
 * 11 Sep 86 -- koehler
 *	ushort cannot be a register - change
 *
 * 11 Nov 85 -- depp
 *	Removed all conditional compiles for System V IPC.
 *
 * 09 Apr 85 -- depp
 *	New File
 *	This file contains the common System V IPC routines
 *
 */

#include "../h/errno.h"		
#include "../h/param.h"		
#include "../h/dir.h"		
#include "../h/user.h"		
#include "../h/quota.h"		
#include "../h/proc.h"		
#include "../h/seg.h"		
#include "../h/ipc.h"		

/*
**	Common IPC routines.
*/

/*
**	Check message, semaphore, or shared memory access permissions.
**
**	This routine verifies the requested access permission for the current
**	process.  Super-user is always given permission.  Otherwise, the
**	appropriate bits are checked corresponding to owner, group, or
**	everyone.  Zero is returned on success.  On failure, u.u_error is
**	set to EACCES and one is returned.
**	The arguments must be set up as follows:
**		p - Pointer to permission structure to verify
**		mode - Desired access permissions
*/

ipcaccess(p, mode)
register struct ipc_perm	*p;
ushort				mode;
{
	if(u.u_uid == 0)
		return(0);
	if(u.u_uid != p->uid && u.u_uid != p->cuid) {
		mode >>= 3;
		if(u.u_gid != p->gid && u.u_gid != p->cgid)
			mode >>= 3;
	}
	if(mode & p->mode)
		return(0);
	u.u_error = EACCES;
	return(1);
}

/*
**	Get message, semaphore, or shared memory structure.
**
**	This routine searches for a matching key based on the given flags
**	and returns a pointer to the appropriate entry.  A structure is
**	allocated if the key doesn't exist and the flags call for it.
**	The arguments must be set up as follows:
**		key - Key to be used
**		flag - Creation flags and access modes
**		base - Base address of appropriate facility structure array
**		cnt - # of entries in facility structure array
**		size - sizeof(facility structure)
**		status - Pointer to status word: set on successful completion
**			only:	0 => existing entry found
**				1 => new entry created
**	Ipcget returns NULL with u.u_error set to an appropriate value if
**	it fails, or a pointer to the initialized entry if it succeeds.
*/

struct ipc_perm *
ipcget(key, flag, base, cnt, size, status)
register struct ipc_perm	*base;
int				cnt,
				flag,
				size,
				*status;
long				key;
{
	register struct ipc_perm	*a;	/* ptr to available entry */
	register int			i;	/* loop control */

	if(key == IPC_PRIVATE) {
		for(i = 0;i++ < cnt;
			base = (struct ipc_perm *)(((char *)base) + size)) {
			if(base->mode & IPC_ALLOC)
				continue;
			goto init;
		}
		u.u_error = ENOSPC;
		return(NULL);
	} else {
		for(i = 0, a = NULL;i++ < cnt;
			base = (struct ipc_perm *)(((char *)base) + size)) {
			if(base->mode & IPC_ALLOC) {
				if(base->key == key) {
					if ((flag & (IPC_SYSTEM | IPC_MMAP)) != 
					    (base->mode & (IPC_SYSTEM | IPC_MMAP)))
						continue;
					if((flag & (IPC_CREAT | IPC_EXCL)) ==
						(IPC_CREAT | IPC_EXCL)) {
						u.u_error = EEXIST;
						return(NULL);
					}
					if((flag & 0777) & ~base->mode) {
						u.u_error = EACCES;
						return(NULL);
					}
					*status = 0;
					return(base);
				}
				continue;
			}
			if(a == NULL)
				a = base;
		}
		if(!(flag & IPC_CREAT)) {
			u.u_error = ENOENT;
			return(NULL);
		}
		if(a == NULL) {
			u.u_error = ENOSPC;
			return(NULL);
		}
		base = a;
	}
init:
	*status = 1;
	base->mode = IPC_ALLOC | (flag & (0777 | IPC_SYSTEM | IPC_MMAP));
	base->key = key;
	base->cuid = base->uid = u.u_uid;
	base->cgid = base->gid = u.u_gid;
	return(base);
}
