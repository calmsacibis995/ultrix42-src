/*	@(#)vio.h	4.1	(ULTRIX)	7/2/90	*/

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

#include <sys/types.h>			/* include system data types */

#define MAX_VFILES 2			/* max number of files open at once */

/*
 * define vfile structure
 */
struct vfile {
	char *v_name;			/* name of file */
	char v_mode;			/* mode of file open ("r" or "w") */
	u_char v_flags;			/* flags for i/o operations */
	int v_fd;			/* file descriptor */
	caddr_t v_base;			/* base of memory allocation */
	u_long v_size;			/* size of memory allocation */
};
/*
 * v_flags
 */
#define VF_MODIFIED	(1<<0)		/* file has been modified */

#define VFILE struct vfile

VFILE *_vopen();
