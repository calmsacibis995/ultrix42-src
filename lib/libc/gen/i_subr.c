/************************************************************************
 *									*
 *			Copyright (c) 1987,1988 by			*
 *		Digital Equipment Corporation, Maynard, MA		*
 *		            Bull, France				*
 *			   Siemens AG, FR Germany			*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under license and may be used and	*
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
/*			Modification History
 *
 * 001  DECwest ANSI vk 022, 29 Dec 1989 - REMOVED thomas 6/06/90
 *      changed the declaration of size, a parameter of function
 *      sknrd to integer (from bit16). This is so that setlocale 
 *      can support 16 bit characters.
 * 002  DECwest ANSI 4.9.1 vk 023, 05 Jan 1990
 *      defined function __opnsknrd (open seek read close).
 *      all sknrd calls other than those in i_init have been 
 *      renamed to __opnsknrd. This is to make sure that setlocale
 *      does not keep file descriptors open.
 */
/*
 * This file contains central subroutines which have to be loaded for any
 * work with the database.
 */
#ifndef lint
static char Sccsid[] = "@(#)i_subr.c	4.1	(ULTRIX)	7/3/90";
#endif

#include "i_defs.h"
#include "i_errno.h"

/*
 * cv_indx -- get index of a double letter
 *
 * SYNOPSIS:
 *	int
 *	cv_indx(c, prptab)
 *	i_char c;
 *	prp_tab *prptab;
 *
 * DESCRIPTION:
 *	cv_index returns the index of the given character in the double letter
 *	table.
 *
 * RETURN:
 *	index of double letter if successful,
 *	I_ERROR otherwise, with i_errno set.
 */
int
cv_indx(c, prptab)
i_char c;
register prp_tab *prptab;
{
	register int i;			/* loop variable		    */
	register i_dblt	*dbp;		/* pointer into double letter table */

	/*
	 * quick check for arguments
	 */
	if (prptab == (prp_tab *)0)
	{
		i_errno = I_EIPRP;
		return(I_ERROR);
	}

	if (c < prptab->prp_nbspl)
		return((int)c);

	/*
	 * character really is double letter so look it up
	 */
	for (dbp = prptab->prp_dblt, i = 0; i < prptab->prp_nbdbl; i++, dbp++)
		if (dbp->symb == c)
			return((int)dbp->indx);

	i_errno = I_EICOD;
	return(I_ERROR);
}

/*
 * sknrd -- secure seek and read
 *
 * SYNOPSIS:
 *	int
 *	sknrd(file, pos, data, size)
 *	int file;
 *	long pos;
 *	char *data;
 *	bit16 size;
 *
 * DESCRIPTION:
 *	sknrd positions to pos in in the file and reads size bytes of data
 *	there, all while checking the systemcall return codes.
 *
 * RETURN:
 * 	1 for could successfully seek'n read
 *	0 otherwise
 */
int
sknrd(file, pos, data, size)
int file;
long pos;
char *data;
bit16 size;
{
	extern long lseek();

	return(lseek(file, pos, 0) == pos && read(file, data, (int)size) == (int)size);
}


/*
 * __opnsknrd -- open, secure seek and read, close
 *
 * SYNOPSIS:
 *	int
 *	__opnsknrd(pi, pos, data, size)
 *	intl *pi;
 *	long pos;
 *	char *data;
 *	bit16 size;
 *
 * DESCRIPTION:
 *      __opnsknrd first opens the file
 *	__opnsknrd positions to pos in in the file and reads size bytes of data
 *	there, all while checking the systemcall return codes.
 *      __opnsknrd also closes the file.
 *
 * RETURN:
 * 	1 for could successfully seek'n read
 *	0 otherwise
 */

#define READ            0       /* open flag for read only              */ 

int
__opnsknrd(pi, pos, data, size)
intl *pi;
long pos;
char *data;
bit16 size;
{
        int file;
	extern long lseek();

        if ((file = open(pi->in_name, READ)) < 0)
           return(0);

	if (!(lseek(file, pos, 0) == pos && read(file, data, (int)size) == (int)size))
           return(0);

        if (close(file) < 0)
           return(0);
        return(1);
}
