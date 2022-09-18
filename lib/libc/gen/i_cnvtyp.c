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
static char Sccsid[] = "@(#)i_cnvtyp.c	4.1	(ULTRIX)	7/3/90";
#endif

#include "i_defs.h"
#include "i_errno.h"

/*
 * i_cnvtyp -- return type of a conversion
 *
 * SYNOPSIS:
 *	int
 *	i_cnvtyp(cnvtab)
 *	cnv_tab *cnvtab;
 *
 * DESCRIPTION:
 *	I_cnvtyp determines the type of the given conversion.
 *
 * RETURN:
 *	CNV_COD for code conversion
 *	CNV_STR for string conversion
 *	0 for bad/illegal conversion pointer.
 */
int
i_cnvtyp(cnvtab)
cnv_tab *cnvtab;
{
	if (cnvtab == (cnv_tab *)0 || cnvtab->cnv_hdr == (cv_head *)0)
	{
		i_errno = I_EICNV;
		return(0);
	}

	if (cnvtab->cnv_hdr->cv_type == CNV_COD)
		return(CNV_COD);

	if (cnvtab->cnv_hdr->cv_type == CNV_STR)
		return(CNV_STR);

	i_errno = I_EICNV;
	return(0);
}
