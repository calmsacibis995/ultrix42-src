#ifndef lint
/*static char	*sccsid = "@(#)elwindow.h	4.1	(ULTRIX)	7/2/90";*/
#endif /* lint */
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
*
* File Name: elwindow.h
*
* Modification history: 
*
*	4/7/86 - bjg
*		Initial Creation; moved from usr.lib/liberrlog to /usr/include
*
*
*/
#define ALL -1
struct options {
	short class;
	short type;
	short ctldevtyp;
	short num;
	short unitnum;
};
struct s_params {
	int indx;
	int toslen;
	struct sockaddr_un tos;
	char scktname[32];
};

