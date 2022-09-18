#ifndef lint
static char Sccsid[] = "@(#)i_ldstr.c	4.1 (ULTRIX) 7/3/90";
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
 * i_ldstr -- load a string table
 *
 * SYNOPSIS:
 *	str_tab *
 *	i_ldcnv(strname, ip)
 *	char *strname;
 *	ip *ip;
 *
 * DESCRIPTION:
 *	i_ldstr reads the requested string table from the database.
 *
 * RETURN:
 *	handle of string table if successful
 *	(str_tab *)0 otherwise, with i_errno set.
 */

/*
 * Modification History
 * ~~~~~~~~~~~~~~~~~~~~
 *
 *
 * 002  DECwest ANSI 4.9.1 vk 023, 05 Jan 1990
 *      Use __opnsknrd (in i_subr.c) instead of sknrd.
 *      This is because setlocale now does not keep
 *      pi->in_name always open.
 * 001  David Lindner Tue Dec 19 12:25:01 EST 1989
 *      - Added prefix name to collation table name, so setlocale could
 *        pick it out of the database.
 *
 */

#include <i_defs.h>
#include <i_errno.h>
#include <strings.h>

/*
 * macro to allocate memory
 */
#define new(var, type, size)	((var = (type *)malloc(size)) != (type *)0)


str_tab *
i_ldstr(strname, ip)
char *strname;
intl *ip;
{
	register str_tab *sp;	/* this is what we load			*/
	register st_head *shp;	/* loop var for string table headers	*/
	register int i;		/* loop var				*/
	i_dbhead *hdr;		/* for faster access			*/
	char *nambuf;		/* temporary name buffer DJL 		*/

	/*
	 * DJL
	 * modify name for collation table
	 */
	if (strcmp(FRM_DEF, strname))
	{
		new(nambuf, char, sizeof(strname)+sizeof(FRM_PRFX)+1);
		sprintf(nambuf, "%s%s", FRM_PRFX, strname);
		strname = nambuf;
	}

	/*
	 * quick check for argument
	 */
	if (ip == (intl *)0)
	{
		i_errno = I_EINTL;
		return((str_tab *)0);
	}
	hdr = &(ip->in_dbhead);

	for (shp = ip->in_sthead, i = 0; i < hdr->i_nbrst; i++, shp++)
	{
		if (strcmp(strname, shp->st_name) == 0)
		{
			/*
			 * found string table. Allocate and load it.
			 */
			if(!new(sp, str_tab, shp->st_siz))
			{
				i_errno = I_ENMEM;
				return((str_tab *)0);
			}

                        if (!__opnsknrd(ip, hdr->i_sttab + shp->st_offst,
				   (char *)sp, shp->st_siz))
			{
				i_errno = I_EBADF;
				free((char *)sp);
				return((str_tab *)0);
			}

			return(sp);
		}
	}

	/*
	 * named string table does not exist
	 */
	i_errno = I_EISTI;
	return((str_tab *)0);
}
