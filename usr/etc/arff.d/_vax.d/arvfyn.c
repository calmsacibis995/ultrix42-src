#ifndef lint
static  char    *sccsid = "@(#)arvfyn.c	4.1  (ULTRIX)        7/2/90";
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

    Verify intended action with user.

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

/*
extern
    exit (),                      Terminate program.
    perror ();                    Print error message.
*/

extern char
    * ttyname ();               /*  Get tty name.                   */

extern FILE
    * fopen ();                 /*  Open a file.                    */

extern char
    * pgmnam;                   /*  Program's name.                 */

extern int
    isatty ();                  /*  Check if a file is a tty.       */

int arvfyn (msg)

char
    * msg;

/*********************************************************************

FUNCTIONAL DESCRIPTION:

    Asks the user a question and insists on a yes/no answer.

    If stderr is not a terminal, the question is not asked and a 'yes'
    reply is assumed.  If stderr is a terminal, the question is writ-
    ten to stderr and input read from stderr, until:

      o  a 'Y' or 'y' is entered as the first character on a line;
      o  a 'N' or 'n' is entered as the first character on a line;
      o  an end of file is encountered.

    If end of file is encountered, a 'no' reply is assumed.

FORMAL PARAMETERS:

    Question.rt.r - The question to ask the user.

IMPLICIT INPUTS:

    None.

IMPLICIT OUTPUTS:

    None.

ROUTINE VALUE:

    0 - The user replied "no" or gave an end of file.
    1 - The user replied "yes" or stderr is not a terminal.

SIDE EFFECTS:

    May exit with an error message.

*********************************************************************/

{
    char
	* inname;

    FILE
	* infile;

    int
	rchar,
	reply;

    /*
     *  Prompted read to stderr.
     */

    if (isatty (fileno (stderr)))
    {
	/*
	 *  Open stderr's device for input.
	 */

	inname = ttyname (fileno (stderr));
	if ((FILE *) NULL == (infile = fopen (inname, "r")))
	{
	    fprintf (stderr, "%s: ", pgmnam);
	    perror (inname);
	    exit (1);
	}

	/*
	 *  Ask until get an answer.
	 */

	reply = -1;
	while (-1 == reply)
	{
	    /*
	     *  Issue prompt.
	     */

	    fputs (msg, stderr);

	    /*
	     *  Get reply.
	     */

	    if (EOF == (rchar = getc (infile)))
	    {
		reply = 0;
	    }
	    else
	    {
		switch (rchar)
		{
		case 'y':
		case 'Y':
		    reply = 1;
		    break;

		case 'n':
		case 'N':
		    reply = 0;
		    break;

		default:
		    /*
		     *  Flush to end of line.
		     */

		    while ((EOF != (rchar = getc (infile))) &&
			   ('\n' != rchar))
		    ;
		    if (EOF == rchar)
		    {
			reply = 0;
		    }
		}
	    }
	}

	/*
	 *  Reply is gotten.
	 */

	fclose (infile);
    }
    else
    {
	/*
	 *  Stderr is not a terminal.  Assume permission granted.
	 */

	reply = 1;
    }

    return (reply);
}
