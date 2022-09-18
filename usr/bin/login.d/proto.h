/*	@(#)proto.h	4.1	(ULTRIX)	7/17/90 */

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
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

#define PASSWORD_AGING	"-e"

/* Protocol opcodes */
#define INITIALIZE 	0
#define GETNAME		1
#define GETENAME	2
#define NAME		3
#define GETPWD		4
#define GETEPWD		5
#define PASSWD		6
#define CHZPWD		7
#define CHGPWD		8
#define VALIDNXIT	9
#define ERROR		10
#define	ERRORNXIT	11
#define ACKNOWLEDGE	12
#define VALID		100

/* Version info sent as part of INITIALIZE message */
#define VERSION		"2.2"

#define REQDATASIZ	4096

struct req {
	short opcode;
	short length;
	char data[REQDATASIZ];
};
	
typedef struct req REQ;

#ifdef DEBUG
#define	SENDREQ(req, op, d, l)	\
	{ \
	req->opcode = op;  \
	req->length = l; \
	if(d != NULL) \
		bcopy(d, req->data, req->length); \
	write(outfd, (char *)req, req->length+(sizeof req->opcode)+(sizeof req->length)); \
	}
#else
#define	SENDREQ(req, op, buf, l)	\
	{ \
	req->opcode = op;  \
	req->length = l; \
	if(buf != NULL) \
		bcopy(buf, req->data, req->length); \
	write(1, (char *)req, req->length+(sizeof req->opcode)+(sizeof req->length)); \
	}
#endif

#ifdef DEBUG
#define	GETREQ(req, c)	\
	{ \
	req->opcode = -1; \
	c = read(infd, req, (sizeof req->opcode)+(sizeof req->length)); \
	if(req->length > 0 && c > 0) \
		c = read(infd, req->data, req->length); \
	}
#else
#define	GETREQ(req, c)	\
	{ \
	c = read(0, req, (sizeof req->opcode)+(sizeof req->length)); \
	if(req->length > 0 && c > 0) \
		c = read(0, req->data, req->length); \
	}
#endif DEBUG

#define RANDOM_WORDS	5
#define NAMESIZ		40

typedef struct {
	char		length;
	char		data[NAMESIZ];
} STRING;

typedef struct {
	char		length;
	STRING		passwd;
	STRING		phonetic;
} PASSWORDPAIRS;

typedef struct {
	long		count;
	PASSWORDPAIRS	pairs[RANDOM_WORDS];
	char		pad[4];
} LISTOFPAIRS;

typedef struct {
	long		count;			/* # of passwords */
	STRING		passwords[2];		/* passwords */
	char		pad[4];
} LISTOFPASSWORDS;
