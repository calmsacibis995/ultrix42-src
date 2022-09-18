#ifndef lint
static char *sccsid = "@(#)escapes.c	4.1	ULTRIX	7/2/90";
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
 * escapes.c -- escape replacement functions
 *
 * Description
 *	This code implements an escape object which is used
 *	to replace character pairs of the form %x by
 *	a string.
 *	The x's and the replacements are supplied in the
 *	form of a table when the escape object is initialised.
 *
 *	The first pair in the table defines the escape
 *	character.
 *	The replacement string is defined by a pointer
 *	to a pointer which makes it more generally useful
 *	for a very small efficiency hit.
 *
 *	This module only knows how to replace
 *	escapes in a struct filter_chain (see filter.[hc])
 *
 *	If you want to just do it to a simple char array
 *	just steal all but the outer for loop in do_descapes()
 *	in order to implement the replacement fn for a string.
 *
 * Restrictions
 * ------------
 *	MEMORY ALLOCATION:
 *	When strings are replaced they are assumed to have
 *	been malloced and are freed using free()
 *
 *	PORTABILITY:
 *	Note that we build the vsprintf arguments
 *	in stack argument layout, using a struct filter_chain.
 *	This is the same as an argv layout for
 *	many architectures, BUT NOT ALL!!!!
 *
 */

/* SCCS history beginning
 * ***************************************************************
 *                -- Revision History --
 * ***************************************************************
 * 
 * 1.1  10/03/88 -- thoms
 * date and time created 88/03/10 14:01:25 by thoms
 * 
 * ***************************************************************
 * 
 * 1.2  03/05/88 -- thoms
 * Fixed '\0' sentinel bug in lookup
 * 
 * 
 * ***************************************************************
 * 
 * 1.3  06/05/88 -- thoms
 * Fixed up obscure bug related to passing null pointer as replacement string.
 * Simplified to use static array for replacement.
 * Consequently if escapes are found replacement must be less than 127 bytes.
 * 
 * 
 * ***************************************************************
 * 
 * 1.4  12/05/88 -- thoms
 * Improved handling of unknown escapes, added es_delete().
 * 
 * 
 * ***************************************************************
 * 
 * 1.5  17/05/88 -- thoms
 * Increase size of temp area for single arg to BUFSIZ
 * 
 * 
 * ***************************************************************
 *
 * 1.6 15/07/88 -- thoms
 * Added copyright notice, modification history, improved comments
 *
 * SCCS history end
 */

#include "lp.h"

/*
 * This is a template for the error message
 *
 * The escape char is filled in to a copy
 * on initialisation and the unknown character is
 * filled in when the error is found
 */
static char unknown[] = "??:unknown escape character";

/*
 * es_init -- initialise an escapes object passing
 *	a table of escape pairs
 */
void
es_init(esp, pairs)
register ESP esp;
struct es_pair *pairs;
{
	register struct es_pair *p;

	esp->es_pair = pairs;
	for (p=pairs; p->ep_ch != '\0'; p++) {}
	esp->es_sentinel = p;

	esp->es_unknown = strsave(unknown);
	esp->es_unknown[0] = pairs[0].ep_ch;	/* the escape char */

	p->ep_repl = &esp->es_unknown;
}

/*
 * es_delete -- de-initialise an escapes object
 *
 * It is assumed that the struct escape itself is static
 */
void
es_delete(esp)
register ESP esp;
{
	free(esp->es_unknown);
}

/*
 * es_lookup -- lookup character ch in the table of pairs
 */
char *
es_lookup(esp, ch)
register ESP esp;
register int ch;
{
	register struct es_pair *p = esp->es_pair;
	esp->es_sentinel->ep_ch = ch;
	esp->es_unknown[1] = ch;
	for (; ; p++) {
		if (p->ep_ch == ch) {
			/* leave as we find for es_init */
			esp->es_sentinel->ep_ch = '\0';
			return *p->ep_repl;
		}
	}
	/*NOTREACHED*/
}

/*
 * do_argv_escapes -- do the escapes for a vector of strings
 */
static int
do_argv_escapes(argv, esp)
char **argv;
register ESP esp;
{
	register int found = 0;	/* Found at least one escape char */
	register char **pv;	/* pointer for stepping through argv */
	register int esc_char; /* remember what our escape char is */
	char arg_tmp[BUFSIZ];

	esc_char = esp->es_pair[0].ep_ch;

	for (pv=argv; *pv; pv++) {
		struct filter_chain sprintf_args;
		register char *p; /* pointer for arg parsing */
		register char *s; /* temporary pointer to repl. string */
		int local_found = 0; /* need to substitute this arg? */

		fc_init(&sprintf_args);

		for (p = *pv; *p; p++) {
			if (*p == esc_char && p[1]) {
				local_found = 1;
				found = 1;
				s = es_lookup(esp, p[1]);
				fc_add_arg(&sprintf_args, s);
				/*
				 * We replace the ESCx pair
				 * by %s for use in the vsprintf call
				 */
				*p++ = '%', *p = 's';
			}
		}
		fc_end_filter(&sprintf_args);
		if (local_found) {
			vsprintf(arg_tmp, *pv, sprintf_args.fc_argv[0]);
			free(*pv);
			*pv = strsave(arg_tmp);
		}
		fc_delete(&sprintf_args, 0);
	}
	return found;
}

/*
 * do_escapes -- replace escapes for filter description object
 */
int do_escapes(fcp, esp)
register FCP fcp;
register ESP esp;
{
	register int found = 0;	/* flag for escape found: no default args */
	register char ***pv;	/* pointer for stepping through argvs */

	if (!esp) return found;	/* no escapes, succeed gracefully */

	for (pv=fcp->fc_argv; pv < &fcp->fc_argv[fcp->fc_nf]; pv++) {
		if (do_argv_escapes(*pv, esp) == 1)
		    found = 1;
	}
	return found;
}
