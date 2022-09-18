/*
 *		@(#)sgtty.h	4.2	(ULTRIX)	9/4/90
 *		sgtty.h	4.1	83/05/03
 */

/************************************************************************
 *									*
 *			Copyright (c) 1985 by				*
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
/************************************************************************
 *			Modification History				*
 *									*
 *	David L Ballenger, 28-Mar-1985					*
 * 0001	Merge in definitions for System V compatibility.		*
 *									*
 ************************************************************************/


/*
 * Structure for stty and gtty system calls.
 */



#ifndef _SGTTY_
#define	_SGTTY_

#ifdef KERNEL
#include "../h/ansi_compat.h"
#else
#include <ansi_compat.h>
#endif

#ifndef __SYSTEM_FIVE
/*
 * For regular ULTRIX systems the sgttyb structure is the same as the
 * IOCTL_sgttyb structure
 */
#define sgttyb_ULTRIX sgttyb

#else /* SYSTEM_FIVE */
/*
 * For System V emulation, the sgttyb structure that the user code sees is
 * different from the one needed to form the second argument for ioctl().
 */
struct sgttyb {
	char	sg_ispeed;		/* input speed */
	char	sg_ospeed;		/* output speed */
	char	sg_erase;		/* erase character */
	char	sg_kill;		/* kill character */
	int	sg_flags;		/* mode flags *** System V ****/
};
#endif /* __SYSTEM_FIVE */
/*
 * sgttyb structure used in creating the second argument for ioctl().
 * This is needed for System V emulation support.
 */
struct sgttyb_ULTRIX {
	char	sg_ispeed;		/* input speed */
	char	sg_ospeed;		/* output speed */
	char	sg_erase;		/* erase character */
	char	sg_kill;		/* kill character */
	short	sg_flags;		/* mode flags */
};

#ifndef	_IOCTL_
#include <sys/ioctl.h>
#endif /*  _IOCTL_ */

#endif /* _SGTTY_ */
