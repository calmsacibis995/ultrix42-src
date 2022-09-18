/*		@(#)keywords.h	1.6				7/16/86	*/	
/*
 * Program keywords.h ,  Module 
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
 * Program keywords.h ,  Module 
 *
 *
 * Networks & Communications Software Engineering
 *
 * IDENT HISTORY:
 *
 *
 * 1.00  10-Jul-1985
 *     DECnet Ultrix V1.0
 *
 */

/*********************************************************************************
**
**	keywords.c
**
**	insert file for keyword table, definitions, and constants.
**
**
**
**	This file contains constants, definitions, and routines 
**      for UPARS keywords processing.	It is included by both the UPARS 
**      compiler, and by the run-time lexing utilities.
**
*******************************************************************************/



#include "keydefs.h"



/*******************************************************************************
**
**	These routines handle basic keyword table functions for the UPARS 
**	compiler and run_time library.
**
*******************************************************************************/


short  	 search_keyword();
char	*get_keyword();
void	 init_keywords();



static short	 key_offset;		/* contains user keywords offset */
static short	 max_key;		/* highest keyword number */


/*
**	The compile and run-time keyword table
*/

typedef	char *KEYWORD_ENTRY;	/* one for \0, ok? */

#define	NUM_KEYWORDS	256

static KEYWORD_ENTRY	keyword_table[NUM_KEYWORDS];



/*
**	Compile and run-time initializers of keyword_table
*/

KEYWORD_ENTRY	byte_names[u_BYTE_OFFSET] = {
		"",
		"$ERROR",
		"$NULL",
		"$EOM",
		"$IMAGE",
		"$PARAM",		
		"$BYTE",
		"$SKIP",
		"$MATCH" } ;



KEYWORD_ENTRY	text_names[NUM_KEYWORDS] = {
		"",
		"$NULL",
		"$ERROR",
		"$EOS",
		"$DIGIT",
		"$CHAR",
		"$BLANK",
		"$ANY",
		"$STRING",
		"$DECIMAL",
		"$HEX",
		"$OCTAL",
		"$LABEL" } ;

/*******************************************************************************
**
**	search_keyword
**
**	Given some string which may be a token, seartch the keyword table
**	for it, abbreviated maybe, and return the keyword's token number
**	as the result.  If no match, return FALSE
*/

short  	 search_keyword(string)
char	*string;
{
	int i;

	for (i = u_$NULL; i <= max_key; i++)
		if (keyword_table[i])
			if (!strcmp(keyword_table[i], string))
				return (i);	
	return (FALSE);
}



/*******************************************************************************
**
**	get_keyword
**
**	Given the keyword number, return the string
**
*/


char	*get_keyword(offset)
short	 offset;
{
	if ((offset >= u_$NULL) && (offset <= max_key))
		return(keyword_table[offset]);
	else
	{
		fprintf(stderr, "Compiler error - bad offset to get_keyword");
		exit(1);
	}
}


/*******************************************************************************
**
**	init_keywords
**
**	Depending on the value of u_parse_flag passed to it as what, it
**	decides whether to load the default values for a text or byte
**	parsing keyword table.  Then it does the load.
*/


void 	 init_keywords(what)
u_BOOL	 what;
{
	int i;


	if (what)
		key_offset = u_BYTE_OFFSET;
	else
		key_offset = u_TEXT_OFFSET;

	max_key = key_offset;

	/*
	**	Now initialize compiler's keyword table 
	*/

	for (i=u_$NULL; i<key_offset; i++)
		keyword_table[i] = what ? byte_names[i] : text_names[i];


}

