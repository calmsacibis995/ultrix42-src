#ifndef lint
static char Sccsid[] = "@(#)i_ldprp.c	4.1 (ULTRIX) 7/3/90";
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
 * i_ldprp -- load a property table
 *
 * SYNOPSIS:
 *	prp_tab *
 *	i_ldprp(prpname, ip)
 *	char *prpname;
 *	intl *ip;
 *
 * DESCRIPTION:
 *	i_ldprp reads the requested property table from the database, if not
 *	already incore.
 *
 * RETURN:
 *	handle of property table if successful
 *	(prp_tab *)0 otherwise, with i_errno set.
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
 * 001  David Lindner Tue Dec 19 12:25:01 EST 1989
 *      - Added prefix name to property table name, so setlocale could
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


prp_tab *
i_ldprp(prpname, ip)
char *prpname;
intl *ip;
{
	register prp_tab *pp;		/* loop var + temp uses		*/
	register pr_head *php;		/* loop var + temp uses		*/
	register int i;			/* loop var			*/
	i_dbhead *hdr;			/* to shorten access time	*/
	char *nambuf;			/* temp name buffer DJL 001	*/

	/*
	 * DJL 001
	 * modify name for collation table
	 */
	if (strcmp(PRP_DEF, prpname))
	{
		new(nambuf, char, sizeof(prpname)+sizeof(PRP_PRFX)+1);
		sprintf(nambuf, "%s%s", PRP_PRFX, prpname);
		prpname = nambuf;
	}

	/*
	 * quick check for arguments
	 */
	if (ip == (intl *)0)
	{
		i_errno = I_EINTL;
		return((prp_tab *)0);
	}

	/*
	 * first look in list of loaded property tables whether resident
	 */
	for (pp = ip->in_propt; pp; pp = pp->prp_nxt)
		if (strcmp(pp->prp_nam, prpname) == 0)
			return(pp);

	hdr = &ip->in_dbhead;

	/*
	 * property table not resident. Get it from the database
	 */
	for (php = ip->in_prhead, i = 0; i < hdr->i_nbrpr; i++, php++)
	{
		if (strcmp(php->pr_name, prpname) == 0)
		{
			/*
			 * Allocate memory for header and data and read data.
			 */
			if (!new(pp, prp_tab, sizeof(prp_tab) + hdr->i_prtsz))
			{
				i_errno = I_ENMEM;
				return((prp_tab *)0);
			}

                        if (!__opnsknrd(ip, hdr->i_prtab + php->pr_offst,
				   (char *)(pp->prp_tbl), hdr->i_prtsz))
			{
				i_errno = I_EBADF;
				free((char *)pp);
				return((prp_tab *)0);
			}

			strcpy(pp->prp_nam, php->pr_name);
			pp->prp_nbspl = hdr->i_nbspl;
			pp->prp_nbdbl = hdr->i_nbdbl;
			pp->prp_dblt  = ip->in_dblt;

			/*
			 * chain into list of loaded tables
			 */
			pp->prp_nxt = ip->in_propt;
			ip->in_propt = pp;
			return(pp);
		}
	}

	/*
	 * property table specified does not exist
	 */
	i_errno = I_EIPRP;
	return((prp_tab *)0);
}
