#ifndef lint
static	char	*sccsid = "@(#)_idoprnt.c	4.1 (ULTRIX) 7/2/90";
#endif lint

/************************************************************************
 *									*
 *	 	      Copyright (c) 1987, 1988, 1989 by			*
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
 *
 *  File name:		_idoprnt.c
 *
 *
 *  Source file description:
 *	This files implements the base functionality for X/OPEN internationised
 *	printf routines (nl_printf(), nl_fprintf(), nl_sprintf()) allowing
 *	argument switching and the specification of precision flags as
 *	arguments passed as parameters.
 *
 *	A call to nl_printf() will pass the internationised format string
 *	to _idoprnt() which fakes up a new format string & argument stack
 *	which is then passed to _doprnt() in the normal way.
 *
 *  Functions:
 *	_idoprnt()
 *
 */

/*
 * Modification history
 * ~~~~~~~~~~~~~~~~~~~~
 *
 * 003	David Lindner Thu Dec 28 13:34:56 EST 1989
 *	- Added arg_align macro to align integers and doubles on
 *	  their correct boundaries.
 *
 * 002	Lie Min Hioe Thu Nov 16th, 1989
 *	- if the greatest value of precision is larger than max_param,
 *	  max_param will be set to the value of prec. Modified the code
 *	  so that the substitued value of precision field can be > 9.
 *	  Commented incorrect codes.
 *
 * 001	David Lindner Wed Nov 15 10:49:45 EST 1989
 *	- Removed vax/mips ifdefs. Modified traversal through arg
 *	  structure so it would use architecture independent macros
 *	  given in varargs.
 *
 * 000	Martin Hills, 08-Feb-1988
 *	- Created.
 *
 */

#include <stdio.h>
#include <ctype.h>
#include <limits.h>
#include <i_errno.h>
#include <varargs.h>

#define S_WORD	sizeof(int)
#define D_WORD	sizeof(double)
#define MAX_LEN		1000	/* maximum of new format string 	*/
#define MAX_ARG_MEM	500	/* maximum size of new args stack	*/

/*
 * DJL 003
 * Definition of arg_align macro that aligns integers on integer 
 * boundaries, and doubles on double boundaries. Is a no-op for vax
 * since this is done automatically.
 *
 */
#ifdef vax
#define arg_align(list, mode)  ;
#endif /* vax */
#ifdef mips
#define arg_align(list, mode)  list = (char *) \
	        (sizeof(mode) > 4 ? ((int)list + 2*8 - 1) & -8 \
				  : ((int)list + 2*4 - 1) & -4) - (sizeof(mode))
#endif /* mips */


static char types[] = "douxXscfeEgGDiIUOpn";
static char others[] = "h+#- *.Ll1234567890";

/*
 * _idoprnt -- internationised _doprnt.
 *
 * SYNOPSIS:
 *	int
 *	_idoprnt(format, args, buf)
 *	char    *format,
 *	va_list  args;
 *	FILE buf;
 *
 * DESCRIPTION:
 *	Acts a filter between printf() (& sprintf(), nl_* etc) converting
 *	the internationised format to the standard _doprnt() format.
 *
 * RETURN:
 *	Returns either the normal return value from _doprnt() on a successful
 *	call or on failure zero with i_errno set to I_EICPR.
 *
 */

int
_idoprnt(format, args, buf)
char *format;
va_list args;
FILE *buf;
{
	int	intl = 1;		/* i18n format flag		*/
	int	i;			/* loop variable		*/
	int	max_param = 0;		/* max arg number used		*/
	int	arg, prec;		/* arg & precision numbers	*/
	char	c, num, *index();

	char	arg_type[NL_ARGMAX + 1];/* the type of each arg		*/
	char	arg_size[NL_ARGMAX + 1];/* the size of each arg		*/
	char   *arg_addr[NL_ARGMAX + 1];/* pointer to args on stack	*/

	char	newformat[MAX_LEN];	/* new format string created	*/
	char	newargs[MAX_ARG_MEM];	/* new stack frame for _idoprnt	*/

	/*
	 * pointers used to form new strings
	 */
	char	*type, *fptr, *oldptr, *newptr, *argptr, *newargptr;
	char	*pstr, str [MAX_LEN];

	fptr = format;

	/*
	 * scan along format string to detect the parameters to be passed
	 * to _idoprnt()
	 */
	while ((c = *fptr++)) {

		if (c == '%') {
			/*
		 	 * check for %%, if so skip onto next char
			 */
			if (*fptr == '%') {
				fptr++;
				continue;
			}

			/*
			 * check to see if this call is an an ordinary
			 * printf or if it uses the internationistion
			 * features.
			 */
			num = *fptr++;
			if (!isdigit(num)) {
				intl = 0;
				break;
			}
			arg = num - '0';

			/*
			 * get number of arg to be printed and
			 * if it is not terminated by a '$' then
			 * reject call as a non-internationised
			 * call.
			 */
			if (*fptr++ != '$') {
				intl = 0;
				i_errno = I_EICPR;
				break;
			}

			/*
			 * check that the parameter number used is not zero
			 */
			if (arg == 0) {
				intl = 0;
				i_errno = I_EICPR;
				break;
			}

			/*
			 * get the number of the highest parameter used.
			 */
			if (arg > max_param)
				max_param = arg;

			/*
			 * if the precision fields are specified in
			 * international format, then subsitute value
			 * for parameter no.
			 */
			while (index(others, *fptr) != (char *)0) {
				if (*fptr++ == '*') {
					num = *fptr++;
					if (!isdigit(num)) {
						intl = 0;
						i_errno = I_EICPR;
						break;
					}
					if (*fptr++ != '$') {
						intl = 0;
						i_errno = I_EICPR;
						break;
					}

					prec = (int)(num - '0');

			/*
			 * LMH 002
			 * get the number of the highest parameter used.
			 */
					if (prec > max_param) 
						max_param = prec;

					/*
					 * check types
					 */
/*							LMH 002		wrong!!!
					if (arg_type[prec] != 'd' &&
					    arg_type[prec] != '\0') {
						intl = 0;
						i_errno = I_EICPR;
						break;
					}
*/
					arg_type[prec] = 'd';
					arg_size[prec] = S_WORD;
				}
			}
			if ((type = index(types, *fptr)) == (char *)0) {
				intl = 0;
				i_errno = I_EICPR;
				break;
			}

			arg_type[arg] = *fptr;
			switch (arg_type[arg]) {

				/*
				 * single word args.
				 */
				case 'd':
				case 'o':
				case 'u':
				case 'x':
				case 'X':
				case 's':
				case 'c':
				case 'D':
				case 'i':
				case 'I':
				case 'U':
				case 'O':
				case 'p':
				case 'n':
					arg_size[arg] = S_WORD;
					break;

				/*
				 * double word args
	 			 */
				case 'f':
				case 'e':
				case 'E':
				case 'g':
				case 'G':
					arg_size[arg] = D_WORD;
					break;

				/*
				 * illegal args
				 */
				default:
					intl = 0;
					i_errno = I_EICPR;
					break;
			}
		}
	}


	/*
	 * if this is a non-internationalised call then just
	 * pass arguments to _doprnt().
	 */
	if (!intl) {
		return(_doprnt(format, args, buf));
	}

	/*
	 * DJL 001
	 * now compute the address of each of the args on the stack
	 */
	for (i = 1; i <= max_param; i++)
		if (arg_size[i] == S_WORD)
			arg_addr[i] = (char *)&va_arg(args, int);
		else
			arg_addr[i] = (char *)&va_arg(args, double);


	/*
	 * now create new format string and stack frame to pass to
	 * _idoprnt().
	 */
	oldptr = format;		/* pointer to old format string	*/
	newptr = newformat;		/* pointer to new format string */
	newargptr = newargs; 		/* pointer to new stack frame	*/

	while ((c = *oldptr++)) {
		*newptr++ = c;
		if (c == '%') {
			if (*oldptr == '%') {
				*newptr++ = *oldptr++;
				continue;
			}
			num = *oldptr++;
			arg = num - '0';
			oldptr++;

			/*
			 * handle extra characters and expand *<digit>$
			 * precision arguments into new format string.
			 */
			while(index(others, *oldptr)) {
				if (*oldptr == '*') {
					oldptr++;
					num = *oldptr++;
					prec = num - '0';
					oldptr++;
					if (arg_type[prec] != 'd') {
						i_errno = I_EICPR;
						return(0);
					}
/*							LMH 002	    wrong !!!
					sprintf(newptr++, "%d", *arg_addr[prec]);
*/
					pstr = str;
					sprintf(str, "%d", *arg_addr[prec]);
					while (*pstr) 
						*newptr++ = *pstr++;
				}
				else
					*newptr++ = *oldptr++;
			}
			*newptr++ = *oldptr++;		/* pick up type */

			/*
			 * now add item to new stack frame
			 */
			if (arg_size[arg] == S_WORD) {
				arg_align(newargptr, int);
				*((int *)newargptr) = *((int *)(arg_addr[arg]));
				newargptr += S_WORD;
			} else {
				arg_align(newargptr, double);
				*((double *)newargptr) = *((double *)(arg_addr[arg]));
				newargptr += D_WORD;
			}
		}
	}
	*newptr = '\0';
	return(_doprnt(newformat, newargs, buf));
}
