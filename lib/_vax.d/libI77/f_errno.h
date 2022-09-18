/*
*	@(#)f_errno.h	1.2	(ULTRIX)	1/16/86
*/

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
*
*			Modification History
*
*	David Metsky		10-Jan-86
*
* 001	Replaced old version with BSD 4.3 version as part of upgrade.
*
*	Based on:	f_errno.h	5.2		7/30/85
*
*************************************************************************/

/*
 * f77 I/O error definitions
 */

#include	<errno.h>

extern int errno;
extern int f_nerr;

#define F_ER		100	/* base offset of f77 error numbers */

#define F_ERFMT		100	/* error in format */
#define F_ERUNIT	101	/* illegal unit number */
#define F_ERNOFIO	102	/* formatted i/o not allowed */
#define F_ERNOUIO	103	/* unformatted i/o not allowed */
#define F_ERNODIO	104	/* direct i/o not allowed */
#define F_ERNOSIO	105	/* sequential i/o not allowed */
#define F_ERNOBKSP	106	/* can't backspace file */
#define F_ERBREC	107	/* off beginning of record */
#define F_ERSTAT	108	/* can't stat file */
#define F_ERREPT	109	/* no * after repeat count */
#define F_EREREC	110	/* off end of record */
#define F_ERTRUNC	111	/* truncation failed */
#define F_ERLIO		112	/* incomprehensible list input */
#define F_ERSPACE	113	/* out of free space */
#define F_ERNOPEN	114	/* unit not connected */
#define F_ERRICHR	115	/* invalid data for integer format term */
#define F_ERLOGIF	116	/* invalid data for logical format term */
#define F_ERNEWF	117	/* 'new' file exists */
#define F_EROLDF	118	/* can't find 'old' file */
#define F_ERSYS		119	/* opening too many files or unknown system error */
#define F_ERSEEK	120	/* requires seek ability */
#define F_ERARG		121	/* illegal argument */
#define F_ERNREP	122	/* negative repeat count */
#define F_ERILLOP	123	/* illegal operation for channel or device */
#define F_ERRFCHR	124	/* invalid data for d,e,f, or g format term */
#define F_ERNMLIST	125	/* illegal input for namelist */

#define F_MAXERR	(f_nerr + F_ER)
