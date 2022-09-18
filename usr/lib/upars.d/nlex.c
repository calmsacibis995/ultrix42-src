/*	@(#)nlex.c	4.1				7/2/90				      */
/*
 * Program nlex.c,  Module 
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
 * Program nlex.c,  Module 
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
**	nlex.c
**
**
**	Run time binary lexer for upars system
**
**
*******************************************************************************/



#include <stdio.h>
#include "upars.h"
#include "keydefs.h"




u_BOOL	 _u_nlex();






extern	void	 _u_init();
extern	void	 _u_blankson();
extern	void	 _u_blanksoff();
extern	void	 _u_text();
extern	void	 _u_binary();
extern unsigned char	 _u_getchar();
extern unsigned char	*_u_nextchar();
extern u_BOOL	 _u_incr();



extern u_BOOL	 u_parse_flag;


extern unsigned char	*u_cc;	/* master buffer pointer */
extern short	 u_left;	/* number of characters left in buffer */
extern unsigned char	 *u_text; /* Latest token matched. Ends with '\0' for */
				/* easy copys and compares . . . */
extern short	 u_length;	/* length of token in u_text, minus '\0' */

/*
**	SCCS def
*/

static	char	 NLEX[] = "      @(#)nlex.c	4.1                    7/2/90";





u_BOOL	 _u_nlex(token, arg)
short	 token;
unsigned short	 arg;
{
	union	{
			unsigned short	du_2_s;
			unsigned char   du_2_c[2];
			}du_2;

	switch (token)
	{
	    case u_$NULL:
		return (TRUE);

	    case u_$ERROR:
		return (TRUE);

	    case u_$EOM:
		if (!u_left)
			return (TRUE);
		else
			return (FALSE);

	    case u_$IMAGE:
		if (*u_cc <= (char)arg)
			if (_u_incr(*u_cc + 1))
				return (TRUE);
		return (FALSE);

	    case u_$PARAM:
		du_2.du_2_c[0] = *u_cc++;
		du_2.du_2_c[1] = *u_cc--;
		if (du_2.du_2_s == arg)
		{
			VOID _u_incr(2);
			return (TRUE);
		}
		else
			return (FALSE);

	    case u_$BYTE:
		if (*u_cc == (unsigned char) arg)
		{
			VOID _u_incr(1);
			return (TRUE);

		}
		else
			return (FALSE);

	    case u_$SKIP:
		VOID _u_incr(1);
		return (TRUE);

	    case u_$MATCH:
		if (_u_incr(arg))
			return (TRUE);
		else
			return (FALSE);


	    default:
		fprintf(stderr, "Keyword number out of range - %d\n", token);
		exit(1);
	}
}



