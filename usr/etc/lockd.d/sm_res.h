/* @(#)sm_res.h	4.1  (ULTRIX)        7/2/90     */


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

struct stat_res{
	res res_stat;
	union {
		sm_stat_res stat;
		int rpc_err;
	}u;
};
#define sm_stat 	u.stat.res_stat
#define sm_state 	u.stat.state

