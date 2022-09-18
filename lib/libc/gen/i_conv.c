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

#ifndef lint
static char Sccsid[] = "@(#)i_conv.c	4.1	(ULTRIX)	7/3/90";
#endif

#include "i_defs.h"
#include "i_errno.h"

/*
 * i_conv -- code conversion for one character
 *
 * SYNOPSIS:
 *	i_char
 *	i_conv(c, cnvtab)
 *	i_char c;
 *	cnv_tab *cnvtab;
 *
 * DESCRIPTION:
 *	i_conv converts the character c according to conversion cnvtab.
 *
 * RETURN:
 *	converted character if successful,
 *	otherwise the code c, with i_errno set.
 */
i_char
i_conv(c, cnvtab)
i_char c;
cnv_tab *cnvtab;
{

	/*
	 * perform a quick check of parameters given
	 */
	if (cnvtab == (cnv_tab *)0
	    ||
	    cnvtab->cnv_hdr == (cv_head *)0
	    ||
	    cnvtab->cnv_hdr->cv_type != CNV_COD)
	{
		i_errno = I_EICNV;
		return((i_char)c);
	}

	/*
	 * do the conversion
	 */
	if (c < cnvtab->cnv_prp->prp_nbspl ||
	    (c = cv_indx(c, cnvtab->cnv_prp)) != I_ERROR)
		return (cnvtab->cnv_cod[c]);

	/*
	 * cv_indx will have set i_errno = I_EICOD;
	 */
	return((i_char)c);
}
