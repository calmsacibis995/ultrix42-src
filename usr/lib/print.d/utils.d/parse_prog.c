#ifndef lint
static char *sccsid = "@(#)parse_prog.c	4.1	ULTRIX	7/2/90";
#endif

/************************************************************************
 *									*
 *			Copyright (c) 1988 by				*
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


/*
 * parse_prog.c -- Test program for filter argument collection
 *	and execution code
 *	Also tests escape replacement code
 * NOTE:
 *	This code is not part of the running system and is
 *	purely for use as a confidence check on the filter pipeline
 *	building code.
 */

/* SCCS history beginning
 * ***************************************************************
 *                -- Revision History --
 * ***************************************************************
 * 
 * 1.1  10/03/88 -- thoms
 * date and time created 88/03/10 14:43:09 by thoms
 * 
 * ***************************************************************
 * 
 * 1.2  06/05/88 -- thoms
 * Brought up to date so that it links cleanly.
 * 
 * 
 * ***************************************************************
 *
 * 1.3  21/07/88 -- thoms
 * Added copyright notice modification history 
 *
 * ***************************************************************
 *
 * 1.4  28/07/88 -- thoms
 * Fixed to call fc_plumb_and_run instead of fc_run which is now obsolete
 *
 * ***************************************************************
 *
 * 1.5  09/09/88 -- thoms
 * Removed duplicate functions which are now available in separate objects
 *
 * SCCS history end
 */


#include "lp.h"

char *name = "me";
char *printer = "printer";
int DU = 0;

test_parse(str)
char *str;
{
	struct filter_chain fc;
	struct escapes es;

static char * es_percent=	"%";
static char * es_one=		"one";
static char * es_two=		"two";
static char * es_three=		"three";
static char * es_four=		"four";
static char * es_null=		"";
	static struct es_pair escape_pairs[] = {
		{ '%', &es_percent },
  		{ '1', &es_one },
		{ '2', &es_two },
		{ '3', &es_three },
		{ '4', &es_four },
		{ '0', &es_null },
		{ '\0', 0 }
	};
	char *argv[10];
	char *p;

	int filter_pid;

	fc_init(&fc);
	es_init(&es, escape_pairs);

	for (p = str; p; ) {
		struct filter_chain prog;
		fc_init(&prog);
		p = parse_prog(&prog, p);
		fc_add_args_v(&fc, prog.fc_argv[0]);
		fc_delete(&prog, 0);
		fc_end_filter(&fc);
	}
	if (!do_escapes(&fc, &es)) dlog(0, "no escapes found");

	fc_plumb_and_run(&fc, DOABORT, 0, 1, 2);

	{
		union wait status = fc_wait(&fc);
		dlog(0, "status %d", status.w_retcode);
	}

	fc_delete(&fc, 0);
}


main(argc, argv)
int argc;
char **argv;
{
	register i;
	DB = 0;
	for (i = 1; i <argc; i++) {
		test_parse(argv[i]);
	}
}

