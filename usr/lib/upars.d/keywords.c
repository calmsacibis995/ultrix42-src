/*	@(#)keywords.c	4.1				7/2/90		      */
/*
 * Program keywords.c,  Module 
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
 * Program keywords.c,  Module 
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
**	keywords.c
**
**	keyword table manager for UPARS compiler
**
**
**
**	These routines handle the keywork table functions for the UPARS 
**	compiler.
**
*******************************************************************************/

#include <stdio.h>
#include "upars.h"
#include "keywords.h"


void	 add_keyword();
void	 key_init(); 
/*
**	SCCS def
*/

static  char	KEYWORDS[] = "    @(#)keywords.c	4.1				7/2/90 "; 


extern  u_BOOL	u_parse_flag;
extern  u_BOOL	u_debug;
extern  char	defines_file_name[];

FILE	*define_file;


/*******************************************************************************
**
**	add_keyword
**
**	Whenever the upars compiler reaches a state where it needs to add
**	a keyword to it's keyword table, like in a statement such as
**		keyword		XYZZY	5
**	it calls add_keyword with "XYZZY" as the keyword, and 5 as the off
**	set.  The defs for the keyword table, search keyword, etc., are
**	contained in "keywords.h", which is shared with the user's
**	parser by way of the upars library, at user run_time.
*/

void	 add_keyword(keyword, id, offset)
char	*keyword;
char	*id;
short	 offset;
{
	short 	 true_offset;

	if (search_keyword(keyword))
		errprint("Duplicate keyword");
	
	else if (u_parse_flag)
		true_offset = ++max_key;
	
	else
		true_offset = offset + key_offset;

	if (true_offset > NUM_KEYWORDS)
		errprint("Keyword out of range");

	else
	{
		keyword_table[true_offset] = (char *) malloc(strlen(keyword)+1);
		strcpy(keyword_table[true_offset], keyword);
		if (!u_parse_flag)
			max_key = true_offset > max_key ? true_offset : max_key;
		if (!u_parse_flag)
			fprintf(define_file, "#define\t%s\t\t%ld\n", keyword,
				offset);
		else
			fprintf(define_file, "#define\t%s\t\t%s\n", keyword, id);

#if DEBUG
		printf("in add_keyword: offset - %d true_offset %d",
			offset, true_offset);
		printf("  max_key - %d\n", max_key);
#endif
	}
}

/*******************************************************************************
**
**	key_init
**
**
**	Provides a way for the upars compiler to call init_keywords,
**	contained in keywords.h, open the defines output file containing
**	the keyword definitions, and output the include of <stdio.h> and
**	the run-time parse flag (FALSE for text parse, TRUE for binary
**	parse.
*/

void	 key_init(what)
u_BOOL	 what;
{
	init_keywords(what);

	if ((define_file = fopen(defines_file_name, "w")) == NULL)
	{
		perror("Could not open u_DEFINES.h");
		exit(1);
	}
			
	fprintf(define_file, "#include <stdio.h>\n");
	fprintf(define_file, "\n\n#define u_PARSE_FLAG\t%d\n", u_parse_flag);
}


/*******************************************************************************
**
**	keyword_dump
**
**	At the end of the upars compiler's parse, it calls this to finish
**	off keyword_table processing.  After the keyword definitions,
**	it outputs the includes of the two upars .h files.
*/



void	 keyword_dump()
{
	fprintf(define_file, "\n#include <upars.h>\n");
	fprintf(define_file, "#include <utables.h>\n");

#if DEBUG
	int	 i;

	for (i=u_$NULL; i<=max_key; i++)
		printf("\nkeyword  %d     %s", i, keyword_table[i]);
#endif
}
