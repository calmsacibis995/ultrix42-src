/*	list.h -
 *		header file for use with list routines in list.c
 *
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
 *	The information in this software is subject to change  without
 *	notice  and should not be construed as a commitment by Digital
 *	Equipment Corporation.					
 *								
 *	Digital assumes no responsibility for the use  or  reliability
 *	of its software on equipment which is not supplied by Digital.
 *
 *	sccsid "@(#)list.h	4.1	(ULTRIX)	7/2/90"
 *
 *	Modifications:
 *
 *	000	19-apr-1989	ccb
 *		added this comment
 *	001	14-jun-1989	ccb
 *		declare ListFree()
*/

/*	DataTypes
*/

typedef struct ListT {
	struct ListT	*l_next;	/* linkage pointer */
	char		*l_data;	/* trash data pointer */
} ListT;

/*	Function Types
*/
extern ListT	*ListAppend();
extern void	ListFree();


