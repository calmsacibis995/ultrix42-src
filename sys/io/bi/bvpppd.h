/*
	@(#)bvpppd.h	4.1	(ULTRIX)	7/2/90
*/

/************************************************************************
 *									*
 *			Copyright (c) 1987 by				*
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

/* 	Port Info Block
 */
struct	bvp_port_info {

	PCCB	*pc_ptr;	/* Pointer to PCCB			*/

};

struct	bvp_sw {
		u_long	type;		/* BVP Adapter type		*/
		u_long	offset;		/* Address offset		*/
		u_char	errlog_typ;	/* Error log type		*/
};

struct	bvp_cmd	{
		u_long	command;	/* Port command			*/
};

