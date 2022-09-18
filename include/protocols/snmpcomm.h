/*
#ifndef lint
static	char	*sccsid = "@(#)snmpcomm.h	4.1	(ULTRIX)	7/2/90";
#endif lint
*/
/************************************************************************
 *									*
 *			Copyright (c) 1989 by				*
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
THIS SOFTWARE IS THE CONFIDENTIAL AND PROPRIETARY PRODUCT OF NYSERNET,
INC.  ANY UNAUTHORIZED USE, REPRODUCTION, OR TRANSFER OF THIS SOFTWARE
IS STRICTLY PROHIBITED.  (C) 1988 NYSERNET, INC.  (SUBJECT TO 
LIMITED DISTRIBUTION AND RESTRICTED DISCLOSURE ONLY.)  ALL RIGHTS RESERVED.
*/

/************************************************************************
 *			Modification History				*
 *
 * 03/09/89	R. Bhanukitsiri
 *		Initial Release.
 *									*
 ************************************************************************/

/*******************************************************************************
**
**			comm.h
**
** Header file containing other includes, structures and definitions
** needed for the communications functions.
**
**
*******************************************************************************/

#include <stdio.h>			/* for various definitions */
#include <sys/types.h>			/* definitions of sys types */
#include <sys/socket.h>			/* socket definitions */
#include <netinet/in.h>			/* internet definitions */
#include <netdb.h>			/* to access net database */
#include <errno.h>			/* for extended error codes */

#define HLEN			32	/* max length of local host name */

/* global variables */
extern int s;				/* socket file descriptor */
extern struct sockaddr_in local;	/* local communications information */
extern int errno;			/* global errno */
extern unsigned short snmpcommport;	/* SNMP communication port */

/* function definitions for communications table functions */
extern struct sockaddr_in *comminfo();	/* get communications information */
extern short commadd();			/* add a table entry */
extern short commrm();			/* remove a table entry */
