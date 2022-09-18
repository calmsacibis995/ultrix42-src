/*	@(#)lex_utils.c	1.10				7/16/86				      */
/*
 * Program lex_utils.c,  Module 
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
 * Program lex_utils.c,  Module 
 *
 *
 * Networks & Communications Software Engineering
 *
 * IDENT HISTORY:
 *
 *
 * 1.00 10-Jul-1985
 *     DECnet Ultrix V1.0
 *
 */

/*******************************************************************************
**
**
**	lex_utils.c
**
**
**	Run time lexer common routines for upars system
**
**
*******************************************************************************/


#include <stdio.h>
#include <ctype.h>
#include "upars.h"
#include "keywords.h"

void	 _u_init();
static void	 _u_keys();
void	 _u_backup();
short	 get_keylen();
void	 _u_blankson();
void	 _u_blanksoff();
void	 _u_parse_text();
void	 _u_copy();
void	 _u_parse_binary();
unsigned char	 _u_getchar();
unsigned char	*_u_nextchar();
u_BOOL	 _u_incr();
u_BOOL	 _u_cmp();



/*
**	SCCS def
*/

static char	LEX_UTILS[] = "     @(#)lex_utils.c	1.10         7/16/86";



extern	u_BOOL	 u_parse_flag;

#define  u_MAX_BUFFER  256

unsigned char	*u_cc;			/* master buffer pointer */
unsigned char	*u_backcc;	/* last master buffer pointer */
short	 u_left;		/* number of characters left in buffer */

unsigned char	 u_text[u_MAX_BUFFER];	/* Latest token matched. Ends with '\0' for */
				/* easy strcpys, strcmps, etc.    */
short	 u_length;		/* length of token in u_text, minus '\0' */
long	 u_intval;		/* integer value of u_text, for u_$DECIMAL, etc */

u_BOOL	 u_blanks = TRUE;
u_BOOL 	 u_text_init = FALSE;

unsigned char	 u_0 = 0;

short	u_keylen[NUM_KEYWORDS];
short	text_key;

FILE	*key_file;





static void  _u_keys(keyword_table_name)
char 	*keyword_table_name;
{

	if ((key_file = fopen(keyword_table_name, "r")) == NULL)
	{
		perror("keyword table");
		exit(1);
	}

	for (text_key = u_TEXT_OFFSET; text_key <= NUM_KEYWORDS; text_key++)
	{
		if (!(text_names[text_key] = (char *) malloc(u_NAME_LENGTH)))
		{
			fprintf(stderr, "Fatal error - No memory for keyword table");
			exit(1);
		}

		if (fscanf(key_file, "%s %d", text_names[text_key],
			      &u_keylen[text_key]) == EOF)
			break;
	}

	if (text_key > NUM_KEYWORDS)
	{
		fprintf(stderr, "Too many keywords in keyword.h\n");
		exit(1);
	}
		
	fclose(key_file);
}





short	 get_keylen(key)
short	 key;
{
	return (u_keylen[key]);
}




void	 _u_init(buffer, buflen)
unsigned char	*buffer;
int	 buflen;
{
	u_cc = buffer;
	u_left = buflen;
	*u_text = '\0';
	u_length = 0;
}





u_BOOL	 _u_incr(how_much)
short	 how_much;
{
	short	 i;
	unsigned char	*text;

	if (how_much  > u_left)
		return (FALSE);
	else
		u_backcc = u_cc;

	if ((!u_blanks) && (!u_parse_flag))
	{
		text = u_cc + how_much;
		while ((!isspace((char) *text++)) && (how_much < u_left))
			how_much++;
	}

	text = u_text;
	while (text < u_text + how_much)
		*text++ = *u_cc++;

	u_length = how_much;
	u_text[u_length] = '\0';

	u_left -= how_much;

	VOID	_u_nextchar();
	
	return (TRUE);
}




void	 _u_backup(how_much)
short	 how_much;
{
		u_cc -= how_much;
		u_left += how_much;
}


u_BOOL	 _u_cmp(key)
short	 key;
{
	unsigned char	 temp[u_MAX_BUFFER], *cc, *cd;
	int	 i;

	strcpy(temp, get_keyword(key));

	for (cc = temp, cd = _u_nextchar(), i = 0; i < get_keylen(key); i++)
	{
		*cc = isupper(*cd) && islower(*cc) ? toupper(*cc) :
		      islower(*cd) && isupper(*cc) ? tolower(*cc) : *cc;

		if (*cc++ != *cd++)
			return (FALSE);	
	}

	while ( !u_blanks ? !isspace(*cd) && ((cd - u_cc) != u_left)
			  : i++ < strlen(temp))
	{
		*cc = isupper(*cd) && islower(*cc) ? toupper(*cc) :
	      	      islower(*cd) && isupper(*cc) ? tolower(*cc) : *cc;
	
		if (*cc++ != *cd++)
			return (FALSE);
	}

	return (TRUE);
}



	
	


void	 _u_parse_text(keyword_table_name)
char	 *keyword_table_name;
{
	int i;

	u_parse_flag = FALSE;
	if (!u_text_init)
	{
	    _u_keys(keyword_table_name);
	    u_text_init = TRUE;
	}
	max_key = text_key;
	for (i=u_$NULL; i<max_key; i++)
	    keyword_table[i] = text_names[i];
}


void	 _u_parse_binary()
{
	int i;

	u_parse_flag = TRUE;
	u_blanks = TRUE;
	max_key = u_BYTE_OFFSET;
	for (i=u_$NULL; i<max_key; i++)
	    keyword_table[i] = text_names[i];
}



void	 _u_blankson()
{
	u_blanks = TRUE;
}



void	 _u_blanksoff()
{
	u_blanks = FALSE;
}


unsigned char	*_u_nextchar()
{
	if (u_blanks)
		return (u_cc);
	else
		while (u_left > 0)
		{
			if (isspace(*u_cc))
			{
				u_cc++;
				u_left--;
			}
			else
				return (u_cc);
		}
	return (&u_0);
}


	


unsigned char	 _u_getchar()
{
	return (*_u_nextchar());
}




void	 _u_copy(where)
unsigned char	*where;
{
	unsigned char	*s;
	int		 i;

	s = u_text;
	for (i = 0; i < (u_length <= 4 ? u_length : 4); i++)
		*where++ = *s++;
}

