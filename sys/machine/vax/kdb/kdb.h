
/*
 * @(#)kdb.h	4.1	( ULTRIX )	7/2/90
 */

/************************************************************************
 *									*
 *			Copyright (c) 1984 by				*
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
/************************************************************************
 *			Modification History				*
 *									*
 *	11-13-87	George Mathew		
 *		Created this file
 *	
 *	03-24-88	George Mathew
 *		Moved definitions for MAXSYMBOLS and SYMBOL_SIZE to this *
 *		file from setup.c
 *
 ************************************************************************/

/* IP interrupt requests  */
#define KDB_ENTER 1
#define KDB_LEAVE 2
extern int kdb_req_cpu,kdb_intr_req,kdb_slavehold;

/* the kdb symbol table and strings tables depend on these values */

#define	MAXSYMBOLS	8192
#define	SYMBOL_SIZE	12
