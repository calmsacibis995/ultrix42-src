/* @(#)prot_time.h	4.1  (ULTRIX)        7/2/90     */


/****************************************************************
 *								*
 *  Licensed to Digital Equipment Corporation, Maynard, MA	*
 *		Copyright 1985 Sun Microsystems, Inc.		*
 *			All rights reserved.			*
 *								*
 ****************************************************************/
/**/
/*
 *	Modification history:
 *	~~~~~~~~~~~~~~~~~~~~
 *
 *	revision			comments
 *	--------	-----------------------------------------------
 *
 *	18-Jan-88	fries
 *			Added Header and Copyright notice.
 *
 *	
 */

/* 
 * This file consists of all timeout definition used by rpc.lockd
 */

#define MAX_LM_TIMEOUT_COUNT	1
#define OLDMSG			30		/* counter to throw away old msg */
#define LM_TIMEOUT_DEFAULT 	15
#define LM_GRACE_DEFAULT 	3
int 	LM_TIMEOUT;
int 	LM_GRACE;
int	grace_period;
