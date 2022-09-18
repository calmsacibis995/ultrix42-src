/* sccsid =	@(#)dbm.h	4.1     ULTRIX        7/2/90	*/
  
/*
 *			Copyright (c) 1989 by
 *		Digital Equipment Corporation, Maynard, MA
 *			All rights reserved.
 *								
 *	This software is furnished under a license and may be used and
 *	copied  only  in accordance with the terms of such license and
 *	with the  inclusion  of  the  above  copyright  notice.   This
 *	software  or  any  other copies thereof may not be provided or
 *	otherwise made available to any other person.  No title to and
 *	ownership of the software is hereby transferred.		
 *								
 *	This software is  derived  from  software  received  from  the
 *	University    of   California,   Berkeley,   and   from   Bell
 *	Laboratories.  Use, duplication, or disclosure is  subject  to
 *	restrictions  under  license  agreements  with  University  of
 *	California and with AT&T.					
 *								
 *	The information in this software is subject to change  without
 *	notice  and should not be construed as a commitment by Digital
 *	Equipment Corporation.					
 *								
 *	Digital assumes no responsibility for the use  or  reliability
 *	of its software on equipment which is not supplied by Digital.
*/

/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	dbm.h	5.1 (Berkeley) 3/27/86
 */

#ifndef NULL
/*
 * this is not used, but is provided for backwards compatability 
 * in case someone may rely on this.
 */
#define	NULL	((char *) 0)
#endif

#include <ndbm.h>

datum	fetch();
datum	firstkey();
datum	nextkey();
