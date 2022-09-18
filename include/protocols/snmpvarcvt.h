/*
#ifndef lint
static	char	*sccsid = "@(#)snmpvarcvt.h	4.1	(ULTRIX)	7/2/90";
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

/*****************************************************************************
**
**			varcvt.h
**
** This is the header file for the translator that converts SNMP
** variables from symbolic to numeric form and vice-versa. This converter
** will convert variables of form:
**
**	<symbolic prefix><dot notation IP address> to 
**	<numeric prefix><32 bit IP address>;		(SYMTONUM conversion)
**
** and
**
**	<numeric prefix><32 bit IP address> to
**	<symbolic prefix><dot notation IP address>	(NUMTOSYM conversion)
**
**
************************************************************************/
/* conversion flags */
#define SYMTONUM		1	/* convert symbolic to numeric */
#define NUMTOSYM		2	/* convert numeric to symbolic */

#define VARINITFL		"/etc/snmp.variables"

extern short varcvt();
