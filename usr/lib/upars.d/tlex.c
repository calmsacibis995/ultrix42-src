/*		@(#)tlex.c	4.1				7/2/90	*/
/*
 * Program tlex.c,  Module 
 *
 *									
 *			Copyright (c) 1985 by			
 *		Digital Equipment Corporation, Maynard, MA
 *			All rights reserved.		
 *						
 *
 * This software is furnished under a license and may be used and copied
 * only  in  accordance  with  the  terms  of such  license and with the
 * inclusion of the above copyright notice. This software or  any  other
 * copies thereof may not be provided or otherwise made available to any
 * other person. No title to and ownership of  the  software  is  hereby
 * transferred.
 *
 * The information in this software is subject to change without  notice
 * and  should  not be  construed  as  a commitment by Digital Equipment
 * Corporation.
 *
 * Digital assumes no responsibility for the use or  reliability  of its
 * software on equipment which is not supplied by Digital.
 *
 *
 * Program tlex.c,  Module 
 *
 *
 * Networks & Communications Software Engineering
 *
 * IDENT HISTORY:
 *
 * 1.00 10-Jul-1985
 *     DECnet Ultrix V1.0
 *
 *
 */

/*******************************************************************************
**
**
**	tlex.c
**
**
**	Run time text lexer for upars system
**
**
*******************************************************************************/

#include <ctype.h>
#include "upars.h"
#include "keydefs.h"





u_BOOL	 _u_tlex();






extern	void	 _u_init();
extern  u_BOOL	 _u_cmp();
extern	void	 _u_blankson();
extern	void	 _u_blanksoff();
extern	void	 _u_text();
extern	void	 _u_binary();
extern unsigned char	 _u_getchar();
extern unsigned char	*_u_nextchar();
extern u_BOOL	 _u_incr();

extern char	*get_keyword();
extern short	 get_keylen();



extern u_BOOL	 u_parse_flag;


extern unsigned char	*u_cc;	/* master buffer pointer */
extern short	 u_left;	/* number of characters left in buffer */
extern unsigned char	 u_text[]; /* Latest token matched. Ends with '\0' for */
				/* easy copys and compares . . . */
extern short	 u_length;	/* length of token in u_text, minus '\0' */
extern long	 u_intval;	/* value of integer for u_DECIMAL, et al */
extern u_BOOL	 u_blanks;

/*
**	SCCS def
*/

static char	 TLEX[] = "	@(#)tlex.c	4.1			7/2/90	";



u_BOOL	 _u_tlex(key)
short	 key;
{
	u_BOOL	 temp;
	int	 i, j;
	unsigned char	*cc;

	switch(key)

	{
	    case u_$NULL:
		return (TRUE);

	    case u_$ERROR:
		return (TRUE);

	    case u_$EOS:
		if (!u_left)
			return (TRUE);
		else
			return (FALSE);

	    case u_$DIGIT:
		if (isdigit((char) _u_getchar()))
			if (_u_incr(1))
				return (TRUE);
		return (FALSE);

	    case u_$CHAR:
		if (isalpha((char) _u_getchar()))
			if (_u_incr(1))
				return (TRUE);
		return (FALSE);

	    case u_$BLANK:
		if (isspace((char) _u_getchar()))
			if (_u_incr(1))
				return (TRUE);
		return (FALSE);

	    case u_$ANY:
		temp = u_blanks;
		_u_blankson();
		i = _u_incr(1);
		u_blanks = temp;
		return (i);


	    case u_$STRING:
		if (!u_left)
			return (FALSE);

		temp = u_blanks;
		_u_blankson();
		for  (cc = u_cc, i = 0; i < u_left; cc++, i++)	
		{
			if (!isalnum((char) *cc))
				if (((char) *cc != '_'))
					if (!((char) *cc == '$'))
						break;
		}
		if (!i)
		{
			u_blanks = temp;
			return (FALSE);
		}
		if ((char) *cc == ' ')
		{
			u_blanks = temp;
			VOID _u_incr(i);
			return (TRUE);
		}
		else if ((i == u_left) || ispunct((char) *cc))
		{
			VOID _u_incr(i);
			u_blanks = temp;
			return (TRUE);
		}
		
		u_blanks = temp;
		return (FALSE);
	

			

	    case u_$DECIMAL:
		if (!u_left)
			return (FALSE);

		temp = u_blanks;
		_u_blankson();
		for  (cc = u_cc, i = 0; i < u_left; cc++, i++)	
		{
			if (!isdigit((char) *cc))
				break;
		}
		if (!i)
		{
			u_blanks = temp;
			return (FALSE);
		}
		if ((char) *cc == ' ')
		{
			u_blanks = temp;
			VOID _u_incr(i);
			u_intval = atol(u_text);
			return(TRUE);
		}
		else if ((i == u_left) || ispunct((char) *cc))
		{
			VOID _u_incr(i);
			u_intval = atol(u_text);
			u_blanks = temp;
			return (TRUE);
		}

		u_blanks = temp;
		return (FALSE);

	    case u_$HEX:
		if (!u_left)
			return (FALSE);

		temp = u_blanks;
		_u_blankson();
		for  (cc = u_cc, i = 0; i < u_left; cc++, i++)	
		{
			if (!isxdigit((char) *cc))
				break;
		}
		if (!i)
		{
			u_blanks = temp;
			return (FALSE);
		}
		if ((char) *cc == ' ')
		{
			u_blanks = temp;
			VOID _u_incr(i);
			sscanf(u_text, "%x", &u_intval);
			return(TRUE);
		}
		else if ((i == u_left) || ispunct((char) *cc))
		{
			VOID _u_incr(i);
		 	sscanf(u_text, "%x", &u_intval);
			u_blanks = temp;
			return (TRUE);
		}

		u_blanks = temp;
		return (FALSE);

	    case u_$OCTAL:
		if (!u_left)
			return (FALSE);

		temp = u_blanks;
		_u_blankson();
		for  (cc = u_cc, i = 0; i < u_left; cc++, i++)	
		{
			if (!(j = isdigit((char) *cc)) || (*cc > '7'))
				break;
		}
		if (!i)
		{
			u_blanks = temp;
			return (FALSE);
		}
		if ((char) *cc == ' ')
		{
			u_blanks = temp;
			VOID _u_incr(i);
			sscanf(u_text, "%o", &u_intval);
			return(TRUE);
		}
		else if ((i == u_left) || ispunct((char) *cc))
		{
			VOID _u_incr(i);
		 	sscanf(u_text, "%o", &u_intval);
			u_blanks = temp;
			return (TRUE);
		}

		u_blanks = temp;
		return (FALSE);



	    default:
		if (_u_cmp(key))
			if (_u_incr(get_keylen(key)))
				return (TRUE);
		return (FALSE);
	}
}


