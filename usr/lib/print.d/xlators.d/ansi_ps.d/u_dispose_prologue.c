#ifndef lint
static char *sccsid = "@(#)u_dispose_prologue.c	4.1      ULTRIX 7/2/90";
#endif

/************************************************************************
 *									*
 *			Copyright (c) 1989 by				*
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
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

#include <stdio.h>
#include "portab.h"
#include "capdl.hc"
#include "xlate.h"
#include "dbug.h"
#include "trn$.hc"
#include "xlc_font_dictionary.hc"
#include "xlc_font_init.hc"
#include "xlm_codegen.h" 
#include "xlc_codegen.hc" 
#include "xlc_graph.hc"
#include "xlm_io.hc"
#include "xlc_ps.hc"
#include "xlc_iface.hc"

#define PREAMBLE_FILE "/usr/lib/lpdfilters/preamble.ps"

char *preamble_file = PREAMBLE_FILE;

void sensible_ps_flush()
{
	if (obuf_ptr != obuf_loc) {
		obuf_len = obuf_ptr - obuf_loc;
		(*putr_loc) (&obuf_len, &obuf_loc, user_p);
	}
	obuf_ptr = obuf_loc;
	obuf_sum = obuf_loc + obuf_len;
}

u_dispose_prologue (decbind)
/*************************************************************************
 *
 * dispose_prologue () sends the ANSI prologue to the PS machine.  If
 * the prologue is being preloaded for future use, the Translator does
 * not "bind" to the PostScript operators.
 *
 ************************************************************************/
BOOLEAN decbind;
{
	/*
	 * Output the initial save if the prologue is going down to the
	 * PostScript machine with the file.
	 */
	/*
	 * To fix the invalidrestore problems.  The initial save has been
	 * moved to xlc_main ().
	 */
	/* if (decbind) ps_str ("save\n"); */

	/* Output the error handler only if ERROR_HANDLER is defined <>0 in dbug.h */
#if ERROR_HANDLER
	ps_str (str_error_handler); /* <--- FOR DEBUG ONLY!!! */
#endif

	/* Output the time stamp and version number of this prologue  */

	ps_str (str_time_date_and_version);

	/*
	 * If the prologue is going down to the PS machine with the file, then
	 * bind to all the PostScript operators defined in the prologue.
	 */

	if (decbind)
	    ps_str (str_decbind);
	else
	    ps_str (str_decbind_null);

	/* Now output the meat of the prologue */


	{
		int fd;
		int n;

		if ((fd = open(preamble_file, 0)) < 0) {
			fprintf(stderr, "ansi_ps: Can't open preamble file %s\n", preamble_file);
			exit(2);
		}
		sensible_ps_flush();

		while ((n=read(fd, obuf_loc, obuf_len)) > 0) {
			obuf_ptr += n;
			sensible_ps_flush();
		}
		if (n < 0) {
			perror("ansi_ps: reading preamble");
			exit(2);
		}
	}
}


