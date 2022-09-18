#ifndef lint
static  char    *sccsid = "@(#)arwcid.c	4.1  (ULTRIX)        7/2/90";
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

    Report on RT-11 specifications that do not match anything.

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

#include <stdio.h>

/*
 * TABLE OF CONTENTS:
 */

/*
 *  arwcin - Initialize wild card table.
 *  arwcmt - Notify of match against wild card.
 *  arwcrp - Report on unmatched wild cards.
 */

/*
 * MACROS:
 */

/*
 * EQUATED SYMBOLS:
 */

/*
 *  Maximum number of wild cards.
 */

#define MAXWCNUM        128

/*
 * OWN STORAGE:
 */

static char
    wctabl [MAXWCNUM / 8];      /*  Wild card usage bit vector.     */

/*
 * EXTERNAL REFERENCES:
 */

extern char
    * pgmnam;                   /*  Program's name.                 */

arwcin (numwcs)

int
    numwcs;

/*********************************************************************

FUNCTIONAL DESCRIPTION:

    Initializes the table of wild card matches.

FORMAL PARAMETERS:

    Number_of_wild_cards.rg.v - The number of wild cards in  the  com-
	mand to arff which may be matched.

IMPLICIT INPUTS:

    None.

IMPLICIT OUTPUTS:

    wctabl - The bitvector of wild card match indicators.

ROUTINE VALUE:

    None.

SIDE EFFECTS:

    None.

*********************************************************************/

{
    int
	i;

#ifdef DEBUG
    fprintf (stderr, "arwcid\\arwcin: initializing count %d\n", numwcs);
#endif

    if (numwcs > MAXWCNUM)
    {
	fprintf (stderr, "%s: Too many file arguments, %d maximum\n",
	    pgmnam, MAXWCNUM);
	exit (1);
    }

    for (i = 0; i < (numwcs + 7) / 8; i ++)
    {
	wctabl [i] = 0;
    }
}

arwcmt (wcnum)

int
    wcnum;

/*********************************************************************

FUNCTIONAL DESCRIPTION:

    Records a wild card as having a match on the RT-11 volume.

FORMAL PARAMETERS:

    Wild_card_number.rg.v - The zero-based index into  the  wild  card
	vector of the wild card that is matched.

IMPLICIT INPUTS:

    wctabl - The bitvector of wild card match indicators.

IMPLICIT OUTPUTS:

    wctabl - The bitvector of wild card match indicators.

ROUTINE VALUE:

    None.

SIDE EFFECTS:

    None.

*********************************************************************/

{
#ifdef DEBUG
    fprintf (stderr, "arwcid\\arwcmt: Match on %d\n", wcnum);
#endif

    wctabl [wcnum / 8] |= (1 << (wcnum % 8));
}

arwcrp (numwc, wc)

int
    numwc;

char
    * * wc;

/*********************************************************************

FUNCTIONAL DESCRIPTION:

    Reports on the wild cards that were never matched.

FORMAL PARAMETERS:

    Number_of_wild_cards.rg.v - The number of wild cards.
    Wild_cards.rt.ra - An array of pointers  to  ASCIZ  strings  whose
	text is the wild card specifications.

IMPLICIT INPUTS:

    wctabl - The bitvector of wild card match indicators.

IMPLICIT OUTPUTS:

    None.

ROUTINE VALUE:

    None.

SIDE EFFECTS:

    For each wild card that was never matched, prints a message.

*********************************************************************/

{
    int
	i;

#ifdef DEBUG
    fprintf (stderr, "arwcid\\arwcrp: report on %d wildcards\n", numwc);
#endif

    for (i = 0; i < numwc; i ++)
    {
	if (0 == (wctabl [i / 8] & (1 << (i % 8))))
	{
	    fprintf (stderr, "%s: Not matched: %s\n", pgmnam, wc [i]);
	}
    }
}
