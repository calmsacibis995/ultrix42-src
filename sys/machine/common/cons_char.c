#ifdef lint
static char *sccsid = "@(#)cons_char.c	4.1	ULTRIX	7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1984,85,86,88,89 by		*
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

/***********************************************************************
 *
 * Modification History: cons_char.c
 *
 */

#include "../machine/cons.h"
#ifdef vax
#include "../machine/mtpr.h"
#endif /* vax */

/*
 * These routines are used to get characters and strings from the console
 * terminal.  Minimal character processing is performed.
 */

#if defined (MVAX) || defined (VAX420)

/*
 * Get a character from a microvax console.  Grab it from the qdss if a 
 * graphics device is being used.
 *
 * Note that for the VAX case this will strip the input character to only
 * 7 bits.  This mask should probably be changed to 0377 to allow 8-bit chars
 * on input.
 */
getchar()
{
	register c;
	extern (*v_consgetc)();
	if( v_consgetc ) {
		c = (*v_consgetc)();
	} else {
		while ((mfpr(RXCS)&RXCS_DONE) == 0)
			;
		c = mfpr(RXDB)&0177;
	}
	gotchar(c);
}

#else
/*
 * Get a character from the console.  For VAX systems this consists of spinning
 * on a register until a character is available; otherwise the cngetc routine
 * is called which calls the console getc routine as defined in the cpu switch
 * table.
 *
 * Note that for the VAX case this will strip the input character to only
 * 7 bits.  This mask should probably be changed to 0377 to allow 8-bit chars
 * on input.
 */
getchar()
{
	register c;
#ifdef VAX
	while ((mfpr(RXCS)&RXCS_DONE) == 0)
		;
	c = mfpr(RXDB)&0177;
#else VAX
	c = cngetc();
#endif VAX
	gotchar(c);
}
#endif 

/*
 * A character has been received from the console.  Call cnputc to echo the
 * character back out onto the console.  Map the carriage return character
 * into the newline character.
 */
gotchar(c)
	register c;
{
	if (c == '\r')
		c = '\n';
        cnputc(c);
	return (c);
}

/*
 * Get a string from the console terminal.  This routine essentially loops
 * until a newline or carriage return is read in to terminate the string.
 *
 * A limited amount of character processing is performed; such as handling
 * erase and kill characters.
 *
 * Note that this routine also needs some work to make it 8-bit clean.
 */
gets(cp)
	char *cp;
{
	register char *lp;
	register c;

	lp = cp;
	for (;;) {
		c = getchar() & 0177;
		switch (c) {
		case 0021:
		case 0023:
			continue;	/* Ignore ^Q and ^S.		  */
		case '\n':
		case '\r':
			*lp++ = '\0';
			return;
		case '\b':		/* Handle erase character,	  */
		case '#':
		case '\177':
			lp--;		/* backup over the previous char, */
			if (lp < cp)	/* but not past the beginning,    */
				lp = cp;
			continue;	/* toss out the erase char itself.*/
		case '@':		/* Handle kill by tossing out the */
		case 'u'&037:		/* pending input and outputing a  */
			lp = cp;	/* newline to start over.	  */
			cnputc('\n');
			continue;
		default:
			*lp++ = c;	/* Store character in array.	  */
		}
	}
}
