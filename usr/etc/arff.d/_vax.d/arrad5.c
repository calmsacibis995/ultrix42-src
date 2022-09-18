#ifndef lint
static  char    *sccsid = "@(#)arrad5.c	4.1  (ULTRIX)        7/2/90";
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

    RADIX 50 to/from ASCII conversion.

ENVIRONMENT:

    PRO/VENIX user mode.
    ULTRIX-11 user mode.
    ULTRIX-32 user mode.

AUTHOR: Brian Hetrick, CREATION DATE: 1 March 1985.

MODIFIED BY:

	Brian Hetrick, 01-Mar-85: Version 1.0
  000 - Original version of module.

*/

/*
 * INCLUDE FILES:
 */

#include "arff.h"
#include <stdio.h>
#include <ctype.h>

/*
 * TABLE OF CONTENTS:
 */

/*
 *  arr50a - Convert from RADIX 50 to ASCII.
 *  arar50 - Convert from ASCII to RADIX 50.
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

static char
    r50atab [050] =
       {' ', 'A', 'B', 'C', 'D', 'E', 'F', 'G',
	'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
	'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W',
	'X', 'Y', 'Z', '$', '.',   0, '0', '1',
	'2', '3', '4', '5', '6', '7', '8', '9'};

/*
 * EXTERNAL REFERENCES:
 */

extern
    exit (),            /*  Terminate program.                      */
    fprintf ();         /*  Print message to file.                  */

extern char
    * pgmnam;           /*  Program name.                           */

arr50a (r50, asc, len)

rad50
    * r50;

char
    * asc;

int
    len;

/*********************************************************************

FUNCTIONAL DESCRIPTION:

    Translates RADIX 50 into ASCII.

FORMAL PARAMETERS:

    RADIX_50.rlw.r - The array of rad50 words to  be  translated  into
	ASCII.
    ASCII.wt.r - The resulting ASCII string.  NOT  terminated  with  a
	NUL.
    Number_characters.rg.v - The  number  of  characters  in  the  two
	strings.

IMPLICIT INPUTS:

    r50atab - The array of ASCII equivalents for RADIX 50 codes.

IMPLICIT OUTPUTS:

    None.

ROUTINE VALUE:

    None.

SIDE EFFECTS:

    May exit with an error code.

*********************************************************************/

{
    register int
	ascii,
	i;

    register rad50
	accum;

    /*
     *  Check length.
     */

    if ((len < 0) || (0 != (len % 3)))
    {
	fprintf (stderr, "%s: Bad RADIX 50 conversion length\n", pgmnam);
	exit (1);
    }

    /*
     *  Decode each RADIX 50 word.
     */

    for (len /= 3; len > 0; len --, asc += 3)
    {
	/*
	 *  Decode the characters from right to left.
	 */

	accum = * r50 ++;

	for (i = 2; i >= 0; i --)
	{
	    /*
	     *  Get the ASCII value and adjust the accumulator  to
	     *  hold only the remaining characters.
	     */

	    ascii = r50atab [accum % 050];
	    accum /= 050;

	    /*
	     *  If the character is valid, store it;  else abort.
	     */

	    if (ascii != 0)
	    {
		asc [i] = ascii;
	    }
	    else
	    {
		fprintf (stderr, "%s: Bad RADIX 50 value\n", pgmnam);
		exit (1);
	    }
	}

	/*
	 *  If the accumulator is not zero, then the word was  not
	 *  a valid RADIX50 code after all.
	 */

	if (accum != 0)
	{
	    fprintf (stderr, "%s: Bad RADIX 50 value\n", pgmnam);
	    exit (1);
	}
    }

    /*
     *  That's all.
     */
}

arar50 (asc, r50, len)

char
    * asc;

rad50
    * r50;

int
    len;

/*********************************************************************

FUNCTIONAL DESCRIPTION:

    Converts an ASCII string to a RADIX 50 string.

FORMAL PARAMETERS:

    ASCII_string.rt.r - The ASCIZ string to be  converted.   Lowercase
	is considered to be equivalent to uppercase.  Legal characters
	are A-Z, 0-9, period, and dollar sign.
    RADIX50_string.wuw.r - The RADIX 50 equivalent.
    String_length.rg.v - The number of characters in the two  strings.
	If this is not a multiple of three, ASCII_string is considered
	to  be  extended with blanks to a multiple of three.  RADIX50_
	string must have one element for each three characters.

IMPLICIT INPUTS:

    None.

IMPLICIT OUTPUTS:

    None.

ROUTINE VALUE:

    None.

SIDE EFFECTS:

    May exit with an error message.

*********************************************************************/

{
    register rad50
	r50acc;

    register int
	i,
	r50chr;

    while (len > 0)
    {
	r50acc = 0;
	for (i = 0; i < 3; i ++)
	{
	    /*
	     *  Get the ASCII character.
	     */

	    if (len > 0)
	    {
		r50chr = (* asc ++) & CHARMASK;
		len --;
	    }
	    else
	    {
		r50chr = ' ';
	    }

	    /*
	     *  Get the RADIX 50 equivalent.
	     */

	    if (isascii (r50chr) && islower (r50chr))
	    {
		r50chr = toupper (r50chr);
	    }

	    if ((r50chr >= 'A') && (r50chr <= 'Z'))
	    {
		r50chr -= ('A' - 001);
	    }
	    else if ((r50chr >= '0') && (r50chr <= '9'))
	    {
		r50chr -= ('0' - 036);
	    }
	    else if (r50chr == ' ')
	    {
		r50chr = 000;
	    }
	    else if (r50chr == '.')
	    {
		r50chr = 034;
	    }
	    else if (r50chr == '$')
	    {
		r50chr = 033;
	    }
	    else
	    {
		fprintf (stderr, "%s: Invalid RADIX 50 character: %c\n",
		    pgmnam, r50chr);
		exit (1);
	    }

	    /*
	     *  Insert into accumulator.
	     */

	    r50acc *= 050;
	    r50acc += r50chr;
	}

	/*
	 *  Store the RADIX 50 word.
	 */

	* r50 ++ = r50acc;
    }
}
