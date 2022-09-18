#ifndef lint
static  char    *sccsid = "@(#)rtwmat.c	4.1  (ULTRIX)        7/2/90";
#endif lint

/************************************************************************
 *                                                                      *
 *                      Copyright (c) 1984 by                           *
 *              Digital Equipment Corporation, Maynard, MA              *
 *                      All rights reserved.                            *
 *                                                                      *
 *   This software is furnished under a license and may be used and     *
 *   copied  only  in accordance with the terms of such license and     *
 *   with the  inclusion  of  the  above  copyright  notice.   This     *
 *   software  or  any  other copies thereof may not be provided or     *
 *   otherwise made available to any other person.  No title to and     *
 *   ownership of the software is hereby transferred.                   *
 *                                                                      *
 *   The information in this software is subject to change  without     *
 *   notice  and should not be construed as a commitment by Digital     *
 *   Equipment Corporation.                                             *
 *                                                                      *
 *   Digital assumes no responsibility for the use  or  reliability     *
 *   of its software on equipment which is not supplied by Digital.     *
 *                                                                      *
 ************************************************************************/


/*
FACILITY:

    RT-11 volume manipulation.

ABSTRACT:

    Does RT-11 wild card matching.

ENVIRONMENT:

    PRO/VENIX user mode.
    ULTRIX-11 user mode.
    ULTRIX-32 user mode.

AUTHOR: Brian Hetrick, CREATION DATE: 1 March 1985.

MODIFIED BY:

	Brian Hetrick, 01-Mar-85: Version 1.0
  000 - Original version of module.

*********************************************************************/

/*
 * INCLUDE FILES:
 */

#include <stdio.h>
#include <ctype.h>

/*
 * TABLE OF CONTENTS:
 */

/*
 *  fmatch - Explicit wild carding match.
 *  rtwmat - Implicit wild carding match.
 */

/*
 * MACROS:
 */

/*
 * EQUATED SYMBOLS:
 */

/*
 * OWN STORAGE:
 */

/*
 * EXTERNAL REFERENCES:
 */

static int fmatch (str1, str2)

char
    * str1;

register char
    * str2;

/*********************************************************************

FUNCTIONAL DESCRIPTION:

    Match a full wild card specification to a file specification.

FORMAL PARAMETERS:

    Wild_card_spec.rt.r - The wild card specification.  No  parts  may
	be omitted.  Case sensitive.
    File_spec.rt.r - The file specification.  Case sensitive.

IMPLICIT INPUTS:

    None.

IMPLICIT OUTPUTS:

    None.

ROUTINE VALUE:

    0 - The wild card specification  and  file  specification  do  not
	match.
    1 - The wild card specification and file specification match.

SIDE EFFECTS:

    None.

*********************************************************************/

{
    register int
	patchar,
	rtnval;

#ifdef DEBUG
    fprintf (stderr, "wmatch\\fmatch:  match \"%s\" and \"%s\"\n",
	str1, str2);
#endif

    rtnval = 1;

    /*
     *  See if the wild card pattern matches the file name.
     */

    while ((0 != rtnval) && ('\0' != (patchar = * str1 ++)))
    {
	switch (patchar)
	{
	case '%':
	    /*
	     *  Match any one character except period.
	     */

	    if (('\0' == * str2) || ('.' == * str2 ++))
	    {
		rtnval = 0;
	    }
	    break;

	case '*':
	    /*
	     *  Match zero or more characters except for period.
	     */

	    rtnval = 0;
	    do
	    {
		rtnval = fmatch (str1, str2);
	    } while ((0 == rtnval) && ('\0' != * str2) && ('.' != * str2 ++));

	    /*
	     *  At this point, we have determined the entire match.
	     */

	    goto end_fmatch;

	default:
	    /*
	     *  Match the character.
	     */

	    if (patchar != * str2 ++)
	    {
		if(patchar != '.')
		rtnval = 0;
	    }
	}
    }

    /*
     *  Do not allow excess at end of name.
     */
    if(*str2 == '.')
	*str2++;

    if ('\0' != * str2)
    {
	rtnval = 0;
    }

    /*
     *  That's it.
     */

end_fmatch:

#ifdef DEBUG
    fprintf (stderr, "fmatch returns %d\n", rtnval);
#endif

    return (rtnval);
}

int rtwmat (pattern, name)

char
    * pattern,
    * name;

/*********************************************************************

FUNCTIONAL DESCRIPTION:

    Checks whether a file name matches a specified wild card pattern.

    Three types of "wild card" patterns are permitted:

     o  An RT-11 file specification, e.g., "foobar.for";
     o  An RT-11 wild card specification, e.g, "foobar.*" or "%.c";
     o  A  long  native  file  specification,  e.g.,   "Makefile"   or
	"old.rtwmatch.c"

    An RT-11 file specification consists of one to six characters fol-
    lowed by a period followed by zero to three characters.

    The wild card pattern consists of an optional file name wild  card
    specification optionally followed by a period and a file extension
    wild card specification.  The character % matches  any  one  char-
    acter;  the character * matches zero or more characters.  An omit-
    ted file name wild card specification is equivalent to  a  *  file
    name  specification;   an  omitted  period and file extension wild
    card specification is equivalent to a .* specification.  The  wild
    card  pattern  may have a prepended UNIX-style path specification,
    which is ignored.

    Thus, "" as a wild card specification matches any file  name,  but
    "."  as  a  wild  card specification matches only files with blank
    file name extensions.  "A" is equivalent to "A.*", and  ".FOR"  is
    equivalent to "*.FOR".  These actions are compatible with those of
    the RT-11 DIRECTORY command.

    A long native file specification violates the length  restrictions
    of  an RT-11 file name.  If the long native file specification has
    more than six characters before the first period, the first six of
    these are taken to be the RT-11 file name and the next three of of
    these  are taken to be the RT-11 file name extension.  If the long
    native file name has one to six characters before the  first  per-
    iod, this is taken to be the RT-11 file name and up to three char-
    acters  before the next period are taken to be the RT-11 file name
    extension.

    All of the above may have prepended native path information.  This
    information is ignored.  File/wild card specifications not falling
    into one of the above categories are considered to never match.

FORMAL PARAMETERS:

    Wild_card_pattern.rt.r - The wild card pattern.
    File_name.rt.r - The file name.

IMPLICIT INPUTS:

    None.

IMPLICIT OUTPUTS:

    None.

ROUTINE VALUE:

    0 - The wild card pattern does not match the name.
    1 - The wild card pattern matches the name.

SIDE EFFECTS:

    None.

*********************************************************************/

{
    char
	outpat [11],    /*  Constructed RT-11 wild card pattern.    */
	* outptr,       /*  Pointer into constructed RT-11 wild     */
			/*   card pattern.                          */
	patchr,         /*  Pattern character.                      */
	* patptr1,      /*  Pattern pointer.                        */
	* patptr2;      /*  Pattern pointer.                        */

    int
	elmsiz,         /*  Number of characters in this part.      */
	numper,         /*  Number of periods encountered.          */
	pattyp;         /*  Pattern type.  0 = RT-11 name without   */
			/*   wild card characters, 1 = RT-11 wild   */
			/*   card specification, 2 = long native    */
			/*   file specification.                    */

#ifdef DEBUG
    fprintf (stderr, "wmatch: comparing \"%s\" and \"%s\"\n",
	pattern, name);
#endif

    /*
     *  Skip over prepended native path information.
     */

    patptr1 = pattern;
    patptr2 = pattern;
    while ('\0' != (patchr = * patptr1 ++))
    {
	if ('/' == patchr)
	{
	    patptr2 = patptr1;
	}
    }
    pattern = patptr2;

    /*
     *  Construct RT-11 wild card pattern.
     */

    outptr = & outpat [0];
    elmsiz = 0;         /*  Size of file name element.              */
    numper = 0;         /*  Number of periods.                      */
    pattyp = 0;         /*  0 = RT-11 file spec, 1 = RT-11 wild     */
			/*   spec, 2 = long native spec.            */

    while ('\0' != (patchr = * pattern ++))
    {
	switch (patchr)
	{
	case '.':

	    /*
	     *  The encountered character is a period.
	     *
	     *  If the spec is a native file spec,  period  terminates
	     *  it.
	     */

	    if (2 == pattyp)
	    {
		goto end_pattern;
	    }

	    /*
	     *  The spec is an RT-11 spec.  Period  is  legal  iff  no
	     *  period has been encountered before.
	     */

	    if (numper > 0)
	    {
		return (0);
	    }

	    /*
	     *  If no file name was specified, assume *.
	     */

	    if (0 == elmsiz)
	    {
		* outptr ++ = '*';
		elmsiz ++;
	    }

	    /*
	     *  Store the period and reset indicators.
	     */

	    * outptr ++ = '.';
	    numper ++;
	    elmsiz = 0;

	    /*
	     *  So much for periods.
	     */

	    break;

	case '*':
	case '%':

	    /*
	     *  The specified character is a wild card character.
	     *
	     *  These are legal only in RT-11 specs.
	     */

	    if (2 == pattyp)
	    {
		return (0);
	    }

	    /*
	     *  The spec type is RT-11 with wild cards.
	     */

	    pattyp = 1;

	    /*
	     *  Fall through to store the character.
	     */

	default:

	    /*
	     *  Some character to be stored.
	     */

	    /*
	     *  Check length restrictions.  Seven or  more  characters
	     *  in  the  name  field is illegal in RT-11 specs.  If no
	     *  wild characters have been specified, this implies that
	     *  the spec is a long native spec.
	     */

	    if ((pattyp < 2) && (0 == numper) && (6 <= elmsiz))
	    {
		if (1 == pattyp)
		{
		    return (0);
		}

		pattyp = 2;
		* outptr ++ = '.';
		numper ++;
		elmsiz = 0;
	    }

	    /*
	     *  Four or more characters in the extension field are il-
	     *  legal in RT-11 wild card specs.  Otherwise,  the  spec
	     *  is a long native file spec, and thereby terminated.
	     */

	    if ((0 != numper) && (3 <= elmsiz))
	    {
		if (1 == pattyp)
		{
		    return (0);
		}

		goto end_pattern;
	    }

	    /*
	     *  Convert the pattern character to uppercase, and  store
	     *  it.
	     */

	    if (isascii (patchr) && islower (patchr))
	    {
		patchr = toupper (patchr);
	    }
	    * outptr ++ = patchr;
	    elmsiz ++;
	}
    }

end_pattern:

    /*
     *  A totally null file spec implies a * for the file name.
     */

    if ((0 == numper) && (0 == elmsiz))
    {
	* outptr ++ = '*';
    }

    /*
     *  RT-11 specs with no period to delimit extension implies .* for
     *  the extension.  Note that native file names always result in a
     *  period in the RT-11 wild card specification.
     */

    if (0 == numper)
    {
/*        * outptr ++ = '.';            */
/*        * outptr ++ = '*';            NO WE DONT LIKE THIS    */

    }

    /*
     *  Store the trailing NUL for the pattern.
     */

    * outptr ++ = '\0';

    /*
     *  The full pattern has been constructed.  Try the match.
     */

#ifdef DEBUG
    fprintf (stderr, "Resulting RT-11 wild card spec: %s\n", outpat);
#endif

    return (fmatch (outpat, name));
}
