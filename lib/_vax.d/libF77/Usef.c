#ifndef lint
static char	*sccsid = "@(#)Usef.c	4.1	(ULTRIX)	7/3/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1986 by				*
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
 *   This software is  derived  from  software  received  from  the	*
 *   University    of   California,   Berkeley,   and   from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is  subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
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
*
*			Modification History
*
*	Sid Maxwell		14-Sep-88
*
* 002	Move ptr++ from top of loop to bottom, to include 1st element
*	of list and stop before the last (null).
*
*	David Metsky		10-Jan-86
*
* 001	Added from BSD 4.3 version as part of upgrade.
*
*	Based on:	Usef.c		1.2		8/1/85
*
*************************************************************************/

/*		returns '-f' if need to use -f to bypass C bug		*/

static char *needs_f[] = {
	"besj0_", "besj1_", "besjn_", "besy0_", "besy1_", "besyn_",
	"c_abs", "erf_", "erfc_", "r_abs", "r_acos", "r_asin",
	"r_atan", "r_atn2", "r_cos", "r_cosh", "r_exp", "r_imag",
	"r_int", "r_lg10", "r_log", "r_sign", "r_sin",
	"r_sinh", "r_sqrt", "r_tan", "r_tanh", "rand_", "random_",
	0,
	};

main(argc, argv)
int argc;
char **argv;
{
	char **ptr;

	argv++;
	ptr = needs_f;
	while( *ptr != 0 ) {
		if( strcmp( *ptr, *argv ) == 0 )
		{
			printf("-f");
			exit(0);
		}
		ptr++;
	}
	printf(" ");
	exit(0);
}
