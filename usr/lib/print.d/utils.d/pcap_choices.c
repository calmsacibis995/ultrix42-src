#ifndef lint
static char *sccsid = "@(#)pcap_choices.c	4.1	ULTRIX	7/2/90";
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
 * pcap_choices.c -- strings for multi-choice capabilities in printcap
 */

/* SCCS history beginning
 * ***************************************************************
 *                -- Revision History --
 * ***************************************************************
 * 
 * 1.1  25/04/88 -- thoms
 * date and time created 88/04/25 18:51:55 by thoms
 * 
 * ***************************************************************
 * 
 * 1.2  12/05/88 -- thoms
 * Added strings for datatype parameter.
 * This is temporary, should use shared module with lpr
 * 
 * 
 * ***************************************************************
 * 
 * 1.3  17/05/88 -- thoms
 * Moved pcap related functions from printjob.c
 * Added functions to call check_args in argstrings.c
 * 
 * 
 * ***************************************************************
 * 
 * 1.4  20/05/88 -- thoms
 * Generalised pcap_get_and_check().
 * Added check for overflow of pcap string buffer.
 * 
 * 
 * ***************************************************************
 * 
 * 1.5  02/06/88 -- thoms
 * Simplified code to allow any -D<datatype>
 * 
 * 
 * ***************************************************************
 *
 * 1.6  19/07/88 -- thoms
 * Added copyright notice and modification history
 * Changed names to agree with code review,
 *
 * ***************************************************************
 *
 * 1.7 28/07/88 -- thoms
 * Modified PS_choices:
 * 	"lps_v2" selects pt_lps_v2
 * 	"LPS" selects pt_lps_v3
 *
 * ***************************************************************
 *
 * 1.8  01/09/88 -- thoms
 * Added to CT_choices to enable use of Berkeley compatible output filters
 *
 * ***************************************************************
 *
 * 1.9  07/09/88 -- thoms
 * Made new output filter code default: change to CT_choices
 *
 * ****************************************************************
 *
 * 1.10 16/10/88 -- thoms
 * Amended CT_choices and PS_choices, see connection.h lp.h
 *
 * ****************************************************************
 *
 * 1.11  2/08/89 -- Giles Atkinson
 * Add v4.0 to UV_choices
 *
 * SCCS history end
 */


#include "lp.h"


char *UV_choices[] = {
	/* check this matches enum ultrix_version_code (lp.h) */
	"3.0", "psv1.0", "4.0", 0,
};

char *CT_choices[] = {
	/* check this matches enum connection_type_e (connection.h) */
	"dev", "lat", "remote", "network", "tcp", "dqs", 0,
};

char *PS_choices[] = {
	/* check this matches enum printer_type_e (lp.h) */
	"non_PS", "LN03R", "LPS", 0,
};

/****************************************************************/
/*
 * functions to handle 0 terminated char * lookup arrays (as above)
 */
int strlookup(strtab, str)
     char **strtab;
     char *str;
{
	register char **p;
	for (p = strtab; *p; p++) {
		if (!strcmp(*p, str)) {
			return (p - strtab);
		}
	}
	return -1;
}

void
strtabprint(fp, strtab)
     FILE *fp;
     char **strtab;
{
	register char **p;
	if (*(p = strtab)) fprintf(fp, "%s", *p);
	for (p++ ; *p; p++) fprintf(fp, ", %s", *p);
	fprintf(fp, "\n");
}

/****************************************************************/
/*
 * function to call parameter lookup routine in argstrings.c
 */
int my_check_arg(arg, opt_num, canon)
     char *arg;
     int opt_num;
     char **canon;
{
	char *junk;
	int num;

	if (opt_num >= 0 && opt_num < num_opts)
	    return check_arg(arg, opt_num, (canon) ? canon : &junk);

	switch (opt_num) {
	    case as_numerical:
		num = atoi(arg);
		return ((num <= 0 || num >= MAXARG) ? -2 : 0);
		
	    case as_string:
		return 0;

	    default:
		log("Unknown string argument type");
		exit(1);
	}
}

/*
 * pcap_get_and_check -- get and check capability or return default
 */
char *pcap_get_and_check(cap, default_str, which_set)
     char *cap;
     char *default_str;
     int which_set;
{
	char *retval;

	if (retval = pgetstr(cap, &bp)) {
		if (bp >= bp_lim) {
			log("pgetstr overflowed buffer");
			exit(1);
		}
	} else
	    retval = default_str;

	if (retval && my_check_arg(retval, which_set, &retval) < 0) {
		log("%s capability: illegal value %s", cap, retval);
		exit(1);
	}
	return retval;
}

/*
 * pcap_get_and_canon -- as above but not a fatal error if the
 *	string is not found in the argstrings array
 */
char *pcap_get_and_canon(cap, default_str, which_set)
     char *cap;
     char *default_str;
     int which_set;
{
	char *retval;

	if (retval = pgetstr(cap, &bp)) {
		if (bp >= bp_lim) {
			log("pgetstr overflowed buffer");
			exit(1);
		}
	} else
	    retval = default_str;

	if (retval) {
		/* just expands it up if found */
		my_check_arg(retval, which_set, &retval);
	}
	return retval;
}
