/*
#ifndef lint
static	char	*sccsid = "@(#)snmperrs.h	4.1	(ULTRIX)	7/2/90";
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
**			snmperrs.h
**
** Return codes (mostly errors) for NYSERNet snmp implementation 
**
**
*******************************************************************************/
/* 0 through -9 are general errors */
#define	GENSUC			0	/* generic success */
#define SNMP_OK			0	/* the only non-error code */
#define EMALLOC			-1	/* malloc error */
#define GENERR			-2	/* generic error */

/* -10 through -29 are errors from the parser and packet builder */
#define NEGINBUF		-10	/* input buffer of negative length */
#define NEGOUTBUF		-11	/* output buffer of negative length */
#define NOINBUF			-12	/* no input buffer */
#define NOOUTBUF		-13	/* no outbuf buffer */
#define TYP_UNKNOWN		-14	/* unknown mesg type */
#define LENERR			-15	/* unexpected end of input message */
#define UNSPLEN			-16	/* indefinite length type */
#define OUTERR			-17	/* no more output buffer */
#define TYP_MISMATCH		-18	/* wrong type */
#define TOOMANYVARS		-19	/* too many variables in message */
#define TOOLONG			-20	/* asn structure too long */
#define TYPERR			-21	/* asn structure wrong for type */
#define PKTLENERR		-22	/* max size of packet exceeded */
#define TYPELONG		-23	/* value is too long for type */
#define NO_SID			-24	/* no session id */
/* -30 to -39 are communications table function errors */
#define TBLFULL			-30	/* table full */
#define NOSUCHID		-31	/* no such request id */
/* -40 to -59 are communications function errors */
#define UNINIT_SOCK		-40	/* uninitialized socket */
#define NODEST			-41	/* no destination */
#define NOSID			-42	/* no session id provided */
#define BADSIDLEN		-43	/* no session id length */
#define NOSVC			-44	/* service requested unavailable */
#define REQID_UNKNOWN		-45	/* unknown request id */
#define SND_TMO			-46	/* send timeout */
#define NORECVBUF		-47	/* no receive buffer */
#define NOREQID			-48	/* no request id provided */
#define NOSOCK			-49	/* couldn't open socket */
#define BINDERR			-50	/* bind error */
#define SND_ERR			-51	/* generic send error */
#define NOHNAME			-52	/* no host name */
#define NOHADDR			-53	/* no local host address */
#define BADVERSION		-54	/* bad protocol version */
#define	RCV_ERR			-55	/* generic receive error */
/* -60  to -69 are variable converter errors */
#define NOINITFL		-60	/* no varcvt initialization file */
#define VARBOUNDS		-61	/*numeric variable not in legal bounds*/
#define NOBUF			-62	/* buffer for conversion non-existent */
#define NONUM			-63	/* no numeric correspondent */
#define NONAME			-64	/* no name for conversion */
#define CONVUNKNOWN		-65	/* unknown conversion */
#define VARUNKNOWN		-66	/* unknown variable */
/* -70 to -79 are authentication function errors */
#define NOMSGBUF		-70	/* buffers are missing */
