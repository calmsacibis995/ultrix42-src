/*
#ifndef lint
static	char	*sccsid = "@(#)snmpasn.h	4.1	(ULTRIX)	7/2/90";
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
**			asn.h
**
** Definitions to be used by the ASN.1 parser and packet builder
**
**
*******************************************************************************/
/* Message types */
#define GETREQID	'\240'		/* get-request */
#define GETNXTID	'\241'		/* get-next-request */
#define GETRSPID	'\242'		/* get-response */
#define SETREQID	'\243'		/* set-request */
#define TRAPID		'\244'		/* trap */

/* real ASN.1 'primitives' */
#define INTID		0x02		/* integer */
#define STRID		0x04		/* string */
#define NULLID		0x05		/* null */
#define OBJID		0x06		/* object identifier */
#define SEQID		0x30		/* sequence */

/* SMI defined 'primitives' */
#define IPID		0x40		/* IpAddress */
#define COUNTERID	0x41		/* Counter */
#define GAUGEID		0x42		/* Gauge */
#define TIMEID		0x43		/* TimeTicks */
#define OPAQUEID	0x44		/* Opaque */

/* misc. other definitions */
#define ISLONG		0x80		/* to mask out all but 8th bit */
#define LENMASK		0x7f		/* to mask out 8th bit */
#define INDFTYP		(char)('\200')	/* the indefinite type */

/* function definitions */
extern short snmpparse();		/* parser functions */
extern char * varlst();
extern char * var();
extern char * vval();
extern char * pdu();
extern char * trap();
extern char * prseint();
extern char * prsestr();
extern char * prsenull();
extern char * prseobj();
extern char * prseipadd();
extern char * prsecntr();
extern char * prsegauge();
extern char * prsetime();
extern char * prsestr();
extern char * prseopaque();
extern char * asnlen();
extern char * otherlen();
extern char * getsubident();
extern short snmpbld();
extern char * bldpdu();
extern char * bldtrp();
extern char * bldvarlst();
extern char * bldvar();
extern char * bldval();
extern char * bldint();
extern char * bldstr();
extern char * bldobj();
extern char * bldnull();
extern char * bldipadd();
extern char * bldcntr();
extern char * bldgauge();
extern char * bldtime();
extern char * bldopqe();
extern char * bldlen();
