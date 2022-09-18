/*	@(#)msdup_errs.h	4.1	(ULTRIX)	2/19/91	*/

/************************************************************************
 *                                                                      *
 *                      Copyright (c) 1989 by                           *
 *              Digital Equipment Corporation, Maynard, MA              *
 *                      All rights reserved.                            *
 *                                                                      *
 *   This software is furnished under a license and may be used and     *
 *   copied  only  in accordance with the terms of such license and     *
 *   with the  inclusion  of  the  above  copyright  notice.   This     *
 *   software  or  any  other copies thereof may not be provided or     *
 *   otherwise made available to any other person.  No title to and     *
 *   ownership of the software is hereby transferred.                   *
 *                                                                      *
 *   The information in this software is subject to change  without     *
 *   notice  and should not be construed as a commitment by Digital     *
 *   Equipment Corporation.                                             *
 *                                                                      *
 *   Digital assumes no responsibility for the use  or  reliability     *
 *   of its software on equipment which is not supplied by Digital.     *
 *                                                                      *
 ************************************************************************/
/*
 *   Facility:	Systems Communications Architecture
 *		Diagnostic/Utilities Protocol (DUP) Class Driver
 *
 *   Abstract:	This file contains the routines specific to the DUP
 *		class driver.
 *
 *   Author:    Mayank Prakash		    5/24/90
 *
 *   $Header$
 *
 *   $Log$
 *
 */

#ifndef MSDUP_errs.h
#define MSDUP_errs.h 0

#define	SIG_NONE -1

typedef struct _msdup_error {
  u_int	category : 3;
  u_int	severity : 2;
  u_int		 : 11;
  u_int	code	 : 16;
} MSDUP_ERROR;

/* Error categories */
#define ERR_CAT_OS	0	/* Standard Ultrix errors		*/
#define ERR_CAT_RET	1	/* Errors in establishing/breaking conn.*/
#define ERR_CAT_CRE	2	/* Errors in message transmission.	*/
#define ERR_CAT_ADR	3	/* Connection management errors.	*/
#define ERR_CAT_DUP	4	/* Error codes defined by DUP		*/

/* Severity of errors. */
#define ERR_FATAL	1	/* Fatal error.				*/
#define ERR_NOCONN	2	/* Connection broken/not established	*/
#define ERR_CONT	3	/* General info. Can continue.		*/

/* Codes signifying no-error condition. */
#define NOERR_CAT_RET		RET_SUCCESS
#define NOERR_CAT_CRE		CRE_CONN_DONE
#define NOERR_CAT_ADR		ADR_SUCCESS
#define NOERR_CAT_DUP		0
#define NOERR_CAT_OS		0
#define	END_ERR_CODES		-1

#define MSDUP_BADCMDREF		11
#define MSDUP_BADOPCODE		12
#define MSDUP_BADCMD		13
#define MSDUP_NOSLEEPERS	14
#define MSDUP_NOTIMERS		15
#define MSDUP_IO_COLLISION	16
#define MSDUP_NODATA		17
#define MSDUP_MSGINCMP		18
#define MSDUP_BUFOVFL		19
#define MSDUP_BBLFAIL		20
#define MSDUP_TMO_NODUST	21
#define MSDUP_NOPROGRESS	22
#define MSDUP_NOREQBS		23
#define MSDUP_BADBHANDLE	24
#define MSDUP_NOMSGBUF		25
#define MSDUP_NOSDIBUFS		26
#define	MSDUP_DATATMO		27
#define	MSDUP_ACTIVE		28
#define	MSDUP_NOACTIVE		29
#define	MSDUP_BABBLEBEG		30
#define	MSDUP_BABBLEEND		31
#define MSDUP_BADMSGBUF		32
#define MSDUP_MSGTOOLONG	33
#define	ADR_SUCCESS		 1	/* Normal or success		     */


#endif /* MSDUP_errs.h */
