/************************************************************************
 *									*
 *			Copyright (c) 1987,1988 by			*
 *		Digital Equipment Corporation, Maynard, MA		*
 *		            Bull, France				*
 *			   Siemens AG, FR Germany			*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under license and may be used and	*
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

#ifndef lint
static char Sccsid[] = "@(#)form_lang.c	4.1	(ULTRIX)	7/3/90";
#endif

/*
 * form_lang -- split up an environ variable
 *
 * int form_lang(env_var, lang, terr, code, table)
 *	char *env_var;		 environ variable text   (size NL_LANGMAX + 1)
 *	char *lang;		 language name string	 (size NL_LANGMAX + 1)
 *	char *terr;		 territory name string   (size NL_LANGMAX + 1)
 *	char *code;		 codeset name string	 (size NL_LANGMAX + 1)
 *	char *table;		 table name string	 (size NL_LANGMAX + 1)
 *
 * DESCRIPTION:
 *	splits up the given name into it's components
 *	the values are checked for correct length
 *	the buffers have to be as long as described above
 *
 * RETURNS:
 *    lang:	language name string
 *    terr:	territory name string
 *    code:	codeset name string
 *    table:	table name string
 *
 * RETURNVALUES:
 *     0:		all is OK
 *    -1:		given env_var not correct (to long or not set)
 */

#include "limits.h"

#ifdef NOVOID
typedef char void;	/* for Siemen's MX */
#endif

/*
 * for portability recognise that strchr sometimes is called index
 */
int
form_lang(env_var, lang, terr, code, table)
char *env_var;		/* environ variable text (size NL_LANGMAX + 1)  	*/
char *lang;		/* language name string	 (size NL_LANGMAX + 1)	*/
char *terr;		/* territory name string (size NL_LANGMAX + 1)    */
char *code;		/* codeset name string	 (size NL_LANGMAX + 1)	*/
char *table;		/* table name_string     (size NL_LANGMAX + 1)    */
{
    char *index();
    char *strcpy();
    char *savecp, *cp;

    code[0] = '\0';	/* clear return fields				*/
    terr[0] = '\0';
    lang[0] = '\0';
    if (table)
    	table[0] = '\0';

    if (env_var == (char *)0)
	return -1;

    /* 
     * first get hold of any table component iff table is a valid pointer
     */
    if (table && (cp = index(env_var, '@')) != (char *)0) {

	savecp = cp++;

	if (strlen(cp) > NL_LANGMAX || strlen(env_var) > NL_LANGMAX)
	    return -1;

	(void)strcpy(table, cp);
	/* 
	 * place a null terminator where the @ was, copy string and then 
	 * replace the @ sign
	 */
	*savecp = '\0';	
        (void)strcpy(lang, env_var);
	*savecp = '@';
    } else
        (void)strcpy(lang, env_var);

    /* split LANG into lang, terr, code */

    if ((env_var = index(lang, '.')) != (char *)0) {
	*env_var++ = '\0';
	(void)strcpy(code, env_var);
    }

    if ((env_var = index(lang, '_')) != (char *)0) {
	*env_var++ = '\0';
	(void)strcpy(terr, env_var);
    }

    return 0;
}
