/*	@(#)msdup_user.h	4.1	(ULTRIX)	2/19/91	*/

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
 *
 *   Facility:	Systems Communication Architecture
 *		Diagnostic/Utilities Protocol (DUP) User Application
 *
 *   Abstract:	This module contains definitions relavent to users of
 *		DUP support.
 *
 *   Author:	Andrew J. Moskal	Creation Date:	dd-mmn-yyyy
 *
 *   Modification History:
 *
 */

/*
 *   Conditional check to include this header file only once.
 */
#ifndef MSDUP_user.h
#define MSDUP_user.h	0		/* Disable subsequent includes */

/*
 *  Include DUP specific definitions
 */
#ifdef	KERNEL
#include "../io/sysap/msdup_pgm.h"	/* DUP program file/header defs      */
#include "../io/sysap/msdup_ioctl.h"	/* DUP I/O control function defs     */
#include "../io/sysap/msdup_msg.h"	/* DUP command/response message defs */
#include "../io/sysap/msdup_proto.h"
#include "../io/sysap/msdup_errs.h"
#else	KERNEL
#include <io/sysap/msdup_pgm.h>		/* DUP program file/header defs      */
#include <io/sysap/msdup_ioctl.h>	/* DUP I/O control function defs     */
#include <io/sysap/msdup_msg.h>		/* DUP command/response message defs */
#include <io/sysap/msdup_proto.h>
#include <io/sysap/msdup_errs.h>
#endif	KERNEL

/*
 *  Local constants
 */
#define	MAX_STD_SIZE	MAX_SDI_SIZE

/*
 *  DUP Standard Dialogue message format
 */
typedef struct _msdup_std_msg {
    struct {
	u_short	msg_num		: 12;	/*  Message number                   */
	u_short	msg_typ		:  4;	/*  Message type                     */
    }			msg_id;		/* Message identifier                */
    u_char		msg_txt[ MAX_STD_SIZE ];
					/* Message text                      */
} MSDUP_STD_MSG;			/* DUP Standard Dialogue message     */

/*
 *  DUP Standard Dialogue message types
 */
#define	MSG_QUESTION		1	/* Question                          */
#define MSG_DEFAULT_QUESTION	2	/* Default question                  */
#define	MSG_INFORMATION		3	/* Information                       */
#define	MSG_TERMINATION		4	/* Termination                       */
#define	MSG_FATAL_ERROR		5	/* Fatal Error                       */
#define	MSG_SPECIAL		6	/* Special                           */


#endif	MSDUP_user.h
