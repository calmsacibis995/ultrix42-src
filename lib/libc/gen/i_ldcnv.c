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
/************************************************************************
 *			Modification History
 *
 * 001  DECwest ANSI 4.9.1 vk 023, 05 Jan 1990
 *      Use __opnsknrd (in i_subr.c) instead of sknrd.
 *      This is because setlocale now does not keep
 *      pi->in_name always open.
 */


#ifndef lint
static char Sccsid[] = "@(#)i_ldcnv.c	4.1	(ULTRIX)	7/3/90";
#endif

#include "i_defs.h"
#include "i_errno.h"

/*
 * macro to allocate memory
 */
#define new(var, type, size)	((var = (type *)malloc(size)) != (type *)0)

/*
 * i_ldcnv -- load a conversion table
 *
 * SYNOPSIS:
 *	cnv_tab *
 *	i_ldcnv(cnvname, ip)
 *	char *cnvname;
 *	intl *ip;
 *
 * DESCRIPTION:
 *	i_ldcnv reads the requested conversion table from the database.
 *	uses binary search as the number of conversion tables may be quite
 *	high.
 *
 * RETURN:
 *	handle of conversion table if successful
 *	(cnv_tab *)0 otherwise, with i_errno set.
 */
cnv_tab *
i_ldcnv(cnvname, ip)
char *cnvname;
intl *ip;
{
	register int c;			/* temp uses			*/
	register cv_head *mid;		/* used for binary search	*/
	cv_head *low;			/* lower limit of binary search	*/
	cv_head *high;			/* upper limit of binary search	*/
	i_dbhead *hdr;			/* for faster access		*/

	/*
	 * quick check for arguments
	 */
	if (ip == (intl *)0)
	{
		i_errno = I_EINTL;
		return((cnv_tab *)0);
	}

	/*
	 * set up for binary search
	 */
	hdr = &(ip->in_dbhead);
	low = ip->in_cvhead;
	high = &ip->in_cvhead[hdr->i_nbrcv - 1];

	while (low <= high)
	{
		mid = low + (high - low) / 2;

		if ((c = strcmp(mid->cv_name, cnvname)) == 0)
		{
			register cnv_tab *cp;

			/*
			 * found conversion table. Allocate and load it
			 */
			c = mid->cv_type;

			if(!new(cp, cnv_tab, sizeof(cnv_tab) + mid->cv_size))
			{
				i_errno = I_ENMEM;
				return((cnv_tab *)0);
			}

			if (!__opnsknrd(ip, hdr->i_cvtab + mid->cv_offst,
				   ((c == CNV_STR) ? (char *)cp->cnv_str
						   : (char *)cp->cnv_cod),
				   mid->cv_size))
			{
				i_errno = I_EBADF;
				free((char *)cp);
				return((cnv_tab *)0);
			}

			cp->cnv_hdr = mid;
			cp->cnv_prp = ip->in_prdflt;
			return(cp);
		}
		else if (c < 0)
			low = mid + 1;
		else
			high = mid - 1;
	}

	/*
	 * specified conversion table does not exist
	 */
	i_errno = I_EICNV;
	return((cnv_tab *)0);
}
