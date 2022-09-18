#ifndef lint
static char Sccsid[] = "@(#)i_ldcol.c	4.1 (ULTRIX) 7/3/90";
#endif

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

/*
 * i_ldcol -- load a collation table
 *
 * SYNOPSIS:
 *	col_tab *
 *	i_ldcol(colname, ip)
 *	char *colname;
 *	intl *ip;
 *
 * DESCRIPTION:
 *	i_ldcol reads the requested collation table from the database. If an
 *	additional property table is required, that is loaded too.
 *
 * RETURN:
 *	handle of collation table if successful
 *	(col_tab *)0 otherwise, with i_errno set.
 */

/*
 * Modification History
 * ~~~~~~~~~~~~~~~~~~~~
 * 
 * 002  DECwest ANSI 4.9.1 vk 023, 05 Jan 1990
 *      Use __opnsknrd (in i_subr.c) instead of sknrd.
 *      This is because setlocale now does not keep
 *      pi->in_name always open.
 *
 * 001	David Lindner Tue Dec 19 12:25:01 EST 1989
 *	- Added prefix name to collation table name, so setlocale could
 *	  pick it out of the database.
 *
 */

#include <i_defs.h>
#include <i_errno.h>
#include <strings.h>

/*
 * macro to allocate memory
 */
#define new(var, type, size)	((var = (type *)malloc(size)) != (type *)0)


col_tab *
i_ldcol(colname, ip)
char *colname;
intl *ip;
{
	register col_tab *cp;	/* temp uses				*/
	register int i;		/* loop var 				*/
	cl_head  *chp;		/* loop var and temp uses		*/
	i_dbhead *hdr;		/* for efficient access to the header	*/
	char *nambuf;		/* temporary name buffer DJL 001	*/

	/*
	 * DJL 001
	 * modify name for collation table
	 */
	if (strcmp(COL_DEF, colname))
	{
		new(nambuf, char, sizeof(colname)+sizeof(COL_PRFX)+1);
		sprintf(nambuf, "%s%s", COL_PRFX, colname);
		colname = nambuf;
	}

	/*
	 * quick check for arguments
	 */
	if (ip == (intl *)0)
	{
		i_errno = I_EINTL;
		return((col_tab *)0);
	}
	hdr = &(ip->in_dbhead);

	for (chp = ip->in_clhead, i = 0; i < hdr->i_nbrcl; i++, chp++)
	{
		if (strcmp(colname, chp->cl_name) == 0)
		{
			/*
			 * found collation table. allocate memory and load it
			 */
			if(!new(cp, col_tab, sizeof(col_tab) + hdr->i_cltsz))
			{
				i_errno = I_ENMEM;
				return((col_tab *)0);
			}

                        if (!__opnsknrd(ip, hdr->i_cltab + chp->cl_offst,
				   (char *)(cp->col_col), hdr->i_cltsz))
			{
				i_errno = I_EBADF;
				free((char *)cp);
				return((col_tab *)0);
			}

			/*
			 * load the corresponding property table
			 */
			if (*(chp->cl_pnam) == '\0')
				strcpy(chp->cl_pnam, PRP_DEF);

			if ((cp->col_prp = i_ldprp(chp->cl_pnam, ip)) == (prp_tab *)0)
			{
				free((char *)cp);
				return((col_tab *)0);
			}

			return(cp);
		}
	}

	/*
	 * collation table not found
	 */
	i_errno = I_EICOL;
	return((col_tab *)0);
}
