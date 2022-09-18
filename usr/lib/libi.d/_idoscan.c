#ifndef lint
static char Sccsid[] = "@(#)_idoscan.c	4.1 (ULTRIX) 7/2/90";
#endif

/************************************************************************
 *									*
 *			Copyright (c) 1987,1988,1989 by			*
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
 *
 *  File name:		_idoscan.c
 *
 *
 *  Source file description:
 *	This files implement the base functionality for X/OPEN internationalised
 *	scanf routines (scanf(), fscanf(), sscanf() and nl_*scanf()) allowing
 *	argument switching and the specification of precision flags as
 *	arguments passed as parameters.
 *
 *	A call to scanf() will pass the internationalised format string
 *	to _idoscan() which fakes up a new format string & argument stack
 *	which is then passed to _doscan() in the normal way.
 *
 *  Functions:
 *	_idoscan()
 *
 */

/*
 * Modification history
 * ~~~~~~~~~~~~~~~~~~~~
 *
 * 001	David Lindner Wed Nov 15 11:10:42 EST 1989
 *	- Removed vax/mips ifdefs so varargs structure could be implemented
 *	  architecture independent.
 *	- Added arg_addr structure so varargs structure could be implemented
 *	  similarly as in _idoprnt.
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

static char types[] = "douxXscfeEgG]DiIUOpn";
static char others[] = "h+#- *.Ll1234567890[";

/*
 * _idoscan() -- internationalised _doscan().
 *
 * SYNOPSIS:
 *	int
 *	_idoscan(buf, format, args)
 *	char   *format,
 *	va_list args;
 *	FILE *buf;
 *
 * DESCRIPTION:
 *	Acts a filter between nl_scanf() (& nl_sscanf() etc) converting
 *	the internationalised format to the standard _doscan() format.
 *
 * RETURN:
 *	Returns either the normal return value from _doscan() on a successful
 *	call or on failure zero with i_errno set to I_EICSC.
 *
 */

int
_idoscan(buf, format, args)
char *format;
va_list args;
FILE *buf;
{
	int	intl = 1;		/* i18n format flag		*/
	int	arg;			/* arg number			*/
	int	max_param=0;		/* max arg number used		*/
	int	i;			/* loop variable		*/
	char	c, num, *index();

	char   *arg_addr[NL_ARGMAX+1];	/* pointer to args on stack	*/
	char	newformat[MAX_LEN];	/* new format string created	*/
	char	newargs[MAX_ARG_MEM];	/* new stack frame for _idoprnt	*/

	/*
	 * pointers used to form new strings
	 */
	register char	*fptr, *oldptr, *newptr, *newargptr;

	fptr = format;

	/*
	 * scan along format string to detect the parameters to be passed
	 * to _idoscan().
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
			 * scanf or if it uses the internationalisation
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
			 * reject call as a non-internationalised
			 * call.
			 */
			if (*fptr++ != '$') {
				intl = 0;
				i_errno = I_EICSC;
				break;
			}

			/*
			 * check that the parameter number used is not zero
			 */
			if (arg == 0) {
				intl = 0;
				i_errno = I_EICSC;
				break;
			}

			/*
			 * DJL 001
			 * get the number of the highest parameter used
			 */

			if (arg > max_param)
				max_param = arg;

			while (index(others, *fptr) != (char *)0) {
				if (*fptr++ == '[') {
					while(*fptr != ']' && *fptr != '\0') {
						fptr++;
					}
				}
			}
			if (index(types, *fptr) == (char *)0) {
				intl = 0;
				i_errno = I_EICSC;
				break;
			}
		}
	}


	/*
	 * if this is a non-internationalised call then just
	 * pass arguments to _doscan().
	 */
	if (!intl) {
		return(_doscan(buf, format, args));
	}

	/*
	 * DJL 001
	 * now compute the address of each of the arguments on the stack
	 */
	
	for (i = 1; i <= max_param; i++) {
		arg_addr[i] = (char *)&va_arg(args, int);
	}

	/*
	 * now create new format string and stack frame to pass to
	 * _idoscan().
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

			while(index(others, *oldptr)) {
				if (*oldptr == '[') {
					do {
						*newptr++ = *oldptr++;
					} while (*oldptr != ']');
				}
				else
					*newptr++ = *oldptr++;
			}
			*newptr++ = *oldptr++;		/* pick up type */

			/*
			 * DJL 001
			 * now add item to new stack frame
			 */
			*((int *)newargptr) = *((int *)(arg_addr[arg]));
			newargptr += S_WORD;
		}
	}
	*newptr = '\0';
	return(_doscan(buf, newformat, newargs));
}
