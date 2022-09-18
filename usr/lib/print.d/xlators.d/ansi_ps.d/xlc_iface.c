#ifndef lint
static char SccsId[] = "  @(#)xlc_iface.c	4.1   LPS_ULT_TRN   7/2/90";
#endif

/* file: xlc_iface.c
 *
 *	COPYRIGHT  (c)  DIGITAL  EQUIPMENT CORPORATION 1987,
 *	1988, 1989.  ALL RIGHTS RESERVED.
 *
 *	THIS SOFTWARE IS FURNISHED UNDER A LICENSE AND MAY BE
 *	USED AND COPIED ONLY IN ACCORDANCE  WITH THE TERMS OF
 *	SUCH  LICENSE  AND  WITH  THE  INCLUSION OF THE ABOVE
 *	COPYRIGHT  NOTICE.  THIS SOFTWARE OR ANY OTHER COPIES
 *	THEREOF   MAY  NOT  BE  PROVIDED  OR  OTHERWISE  MADE
 *	AVAILABLE  TO  ANY  OTHER  PERSON.  NO  TITLE  TO AND
 *	OWNERSHIP  OF  THE  SOFTWARE  IS  HEREBY TRANSFERRED.
 *
 *	THE INFORMATION IN THIS SOFTWARE IS SUBJECT TO CHANGE
 *	WITHOUT  NOTICE  AND SHOULD  NOT BE  CONSTRUED  AS A
 *	COMMITMENT  BY  DIGITAL EQUIPMENT CORPORATION.
 *
 *	DIGITAL  ASSUMES  NO RESPONSIBILITY  FOR THE  USE  OR
 *	RELIABILITY  OF ITS SOFTWARE ON EQUIPMENT THAT IS NOT
 *	SUPPLIED BY DIGITAL.
 *
 */



/*
 *-----------------------------------------------------------
 *
 *  begin edit_history
 *
 *   8-JAN-1988 08:58 mhw
 *      Initial version
 *
 *   5-FEB-1988 14:32 mhs
 *      Make code conditionally compiled so that LATTICE version
 *      doesn't have problems with VAX-specific code.
 *
 *   8-FEB-1988 16:24  bf
 *      Changed return value fo ransi_input()
 * 
 *   2-DEC-1988 14:46 ejs
 *	The macros M$PS_CHAR and M$PS_SCHAR are now available.
 *	The macro M$PS_SCHAR has a check for eight bit and will execute it
 *	first, using the macro M$PS_CHAR for performance reasons.
 *
 *  20-MAR-1989 C. Peters
 *	Added cast operator '(int *)' before '&f' for Ultrix port.
 *
 *
 *  24-APR-1989 14:41 C. Peters
 *	Changed definition of oprintf to use varargs.h for portability.
 *
 *  end edit_history
 *
 *-----------------------------------------------------------
 */



/*
 *-----------------------------------------------------------
 *
 *   begin description
 *
 *   Filename:  xlc_iface.c
 *
 *   I/O Interface for xltest
 *
 *   end description
 *
 *-----------------------------------------------------------
 */



/* Translator edit history
 *
 * file:	xlc_iface.c - Translator OS interface routines
 * created:	ajb	 24-FEB-1986 09:59:42 
 * edit:
 *		araj	 24-JUL-1986 09:36:43 
 *			Made all variables Global, so 
 *			the code can be duplicated in the 
 *			fast loop of XLC_MAIN, and XLC_CODEGEN.
 *
 *			Removed EOF_STATE, as it is 
 *			the responsalibity of the caller to 
 *			insure proper termination upon EOF.
 *
 *			Note, in fact these changes were made
 *			during the previous attempt at 
 *			optimization (Last week of May), 
 *			but were kept from interfering with 
 *			Functionality development.
 *			The optimized file is now being 
 *			transfered to main stream.
 *
 *		araj	 24-JUL-1986 10:55:16 
 *			Changed CHECK_BUFF_OF into M$....
 *
 *		gh	 31-JUL-1986 14:29:07 
 *			Changed declaration of obuf_sum to UBYTE *
 *
 *		araj	 27-MAR-1987 21:22:53 
 *			removed ps_schar from list of routines, as
 *			it was moved to codegen long ago
 */



/* begin include file */

#include <varargs.h>
#include "portab.h"
#include "xlc_iface.hc"
#include "trn$.hc"
#include "xlm_io.hc"
#include "cpsys.hc"
#include "caglobal.hc"

/* end include files */



/************************************************************************
 * This module includes the following routines:
 * --------------------------------------------
 * iface_init ()
 * ansi_input ()
 * ps_flush ()
 * ps_char ()
 * ps_str ()
 ************************************************************************/



/*
 * External variables referenced:
 */

/*
 * External functions referenced:
 */

/*
 * Internal variable declarations:
 */



/*----------*/

READONLY VOID iface_init (getr, user_arg_g, putr, user_arg_p)
DEF (*getr) ();
DEF user_arg_g;
DEF (*putr) ();
DEF user_arg_p;
   {
    /*
     * Save in local statics
     */
    getr_loc = getr;
    user_g = user_arg_g;
    putr_loc = putr;
    user_p = user_arg_p;

    /*
     * Find out where the output buffer is going to go
     */
    obuf_len = 0;
    (*putr_loc) (&obuf_len, &obuf_loc, user_p);
    obuf_ptr = obuf_loc;
    obuf_sum = obuf_loc + obuf_len;

    /*
     * Set up the input length so that the first invocation of
     * ansi_input will read in a buffer
     */
    ibuf_len = 0;
   }



/*----------*/

READONLY DEFAULT ansi_input () 
   {
    /*
     * Read in a buffer if there isn't any data left
     */
    if (ibuf_len == 0) 
       {
        (*getr_loc) (&ibuf_len, &ibuf_loc, user_g);
	if (ibuf_len == 0) 
	   {
	    return(EOF);
	   }
	ibuf_ptr = ibuf_loc;
       }

    /*
     * Return the next character in the input buffer
     */
    ibuf_len--;
    return((DEFAULT)*ibuf_ptr++);
   }



/*
 * Flush any remaining data that is in the output buffer.  This routine should
 * only be called as the last translator outcall.
 */

READONLY VOID ps_flush () 
   {
    if (obuf_ptr != obuf_loc) 
       {
        obuf_len = obuf_ptr - obuf_loc;
	(*putr_loc) (&obuf_len, &obuf_loc, user_p);
       }
   }



/*----------*/

READONLY VOID ps_char (c)
UBYTE c;
   {
   M$PS_CHAR(c);
   }



/*
 *	This printf is specific to the ansii to postsctipt translator.  This
 *	printf makes direct outcalls to the caller of the translator.  This
 *	routine is standalone in that it doesn't require support from the
 *	C run time library.
 *
 *	The following oprintf commands are supported, with brackets [] 
 *	denoting optional parameters.  Note that this a very small subset
 *	of the real printf.
 *
 *	%s	Print variable length string.  String is null terminated.
 *
 *	%c	Print character.
 *
 *	%d	Print signed longword in decimal.
 *
 *	%x	Print unsigned longword in hex.
 *
 *	%%	Print a percentage sign.
 */

oprintf(va_alist)
va_dcl
{
    va_list arg_ptr;
  
    char *format;
    int	number;
    char c, ch;
    char *str;

    /*
     * Set up the pointers into the parameter list
     */
    va_start(arg_ptr);

    format = va_arg(arg_ptr, char *);

    /*
     * Visit each character on the format string
     */
    while (*format) 
       {
	/*
	 * Character is a regular, non escaped character
	 */
	if (*format != '%') 
	   {
	    ps_char (*format++);
	    continue;
	   }

	/*
	 * Character is an format specifier
	 */
	format++;
	switch (c = *format++) 
	   {
	    /* single character */
	    case 'c':
	    case 'C':
                     {
		       ch = va_arg(arg_ptr, char);
                       ps_schar(ch);
		       break;
                     }
	    case 'x':
	    case 'X':
                     {
                       number = va_arg(arg_ptr, int);
		       convert16 (number);
		       break;
                     }
	    case 'd':
	    case 'D':
                     {
		       number = va_arg(arg_ptr, int);
		       if (number < 0) 
			  {
			   ps_char ('-');
			   convert10 (-number);
			  }
		       else 
			  {
			   convert10 (number);
			  }
		       break;
                     }
	    case 'S':
	    case 's':
                     {
                       str = va_arg(arg_ptr, char *);
		       ps_str (str);
		       break;
                     }
	    default:
		       ps_schar (c);
		       break;
	   }
       }
     va_end(arg_ptr);
   }



/*----------*/

VOID convert16 (number)
int number;
   {
    WORD i;
    char c;

    for (i = 7; i >= 0; i--) 
       {
	c = (number >> (i * 4)) & 0x0f;
	c += '0';
	ps_char ((c > '9') ? (c + 7) : c);
       }
   }



/*----------*/

VOID convert10 (number)
int number;
   {
    WORD i;
    char digits [12];

    /*
     * Convert to ascii
     */
    i = 0;
    do {
	digits [i] = (number % 10) + '0';
	number /= 10;
	i++;
       } while (number);

    /*
     * Send out the conversion
     */
    while (--i >= 0) 
       {
	ps_char (digits [i]);
       }
   }



/*----------*/

VOID ps_schar (c)
UBYTE c;
   {
    /* If the character is \ ( ) or blob, PS will not understand, 
     * use octal
     */
    if ( (c == '\\') || (c == '(') || (c == ')') )
       {
	ps_char ('\\');
	ps_char ( c );		/* output the character, if PS escapable */
        return;
       }

    /* If not C0/C1, and GL/GR enabled, print GR code as is 
     * or if GL (not C0/C1, and less than 0x80)
     * print as is
     */
    if((c >= 0x20) && (c  <= 0x7F))
       {
	ps_char (c);
	return;
       }

    /* If CR/LF/FF print as is */
    if ( (c == '\n') ||
	 (c == '\r') ||
	 (c == '\f')
       )
       {
	ps_char (c);
	return;
       }

    /* None of the above, use octal */
    ps_char ('\\');
    ps_char (((c >> 6) & 7) + '0');
    ps_char (((c >> 3) & 7) + '0');
    ps_char (((c >> 0) & 7) + '0');
   }



/*----------*/

VOID tconvert16 (number)
int number;
   {
    int	i;
    char c;

    for (i = 7; i >= 0; i--) 
       {
	c = (number >> (i * 4)) & 0x0f;
	c += '0';
	ps_char ((c > '9') ? (c+7) : c);
       }
   }



/*----------*/

VOID tconvert10 (number)
int number;
   {
    int	i;
    char digits [12];

    /*
     * Convert to ascii
     */
    i = 0;
    do {
	digits [i] = number % 10 + '0';
	number /= 10;
	i++;
       } while (number);

    /*
     * Send out the conversion
     */
    while (--i >= 0) 
       {
	ps_char (digits [i]);
       }
   }



/*----------*/

VOID ps_str (s)
PUB s;
   {
    while (*obuf_ptr++ = *s++)
       {
	/* Stuff the character into the output buffer */
		
	/*
	 * Check for a buffer overflow or buffer full
	 */
	M$CHECK_BUFF_OF();
       }
    obuf_ptr--;
   }

