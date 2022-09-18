#ifdef lint
static char sccsid[] = "@(#)nmatch.c	1.2		(ULTRIX)	4/2/86";	
#endif

/************************************************************************
 *									*
 *			Copyright (c) 1986 by				*
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
 *   This software is  derived  from  software  received  from  the	*
 *   University    of   California,   Berkeley,   and   from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is  subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

static char *pat,lastmatch;

/*
 * Match attempts to match pattern with string.  Pattern
 * may contain operators or single character REs.  The following
 * are operators:
 *	-	Skip through string until the
 *		RE following the '-' is matched.
 *	$n	Same as above except the skipped string is
 *		saved in the nth argument after string (1<=n<=9).
 *	blank	Skip over white space in string.
 *	?x	Conditional.  Return(savecnt) if the last character matched
 *		in a [..] RE was x.
 *	'...'	Skip ahead looking for the string that matches
 *		the RE string in quotes
 *	\	Negates the affect of the above.
 * The following are single character REs:
 *	x	Any ascii character.  Must be matched by the
 *		next character in string.
 *	.	Matches any character in string.
 *	[...]	Matches any character contained in the brackets,
 *		- and ^ do not work.
 *	\	Negates the affect of the above.
 * Match goes until the end of pattern is reached or until
 * the string does not match the pattern.  The number of saved
 * strings ( from the $ op. ) is returned or -1 if there
 * is no more string but more pattern.
 */
nmatch(pattern,string,arg1)
char *pattern,*string,*arg1;
{
	register char *input;
	register char *save;
	register char *p;
	int savecnt;

	savecnt = 0;
	pat = pattern;
	input = string;
	while(*pat) {
		switch(*pat) {
		case '?':
			pat++;
			if(*pat++ == lastmatch)
				return(savecnt);
			break;
		case ' ':
			pat++;
			while(*input==' ' || *input=='\t')
				input++;
			if(*input == '\0')
				return(-1);
			break;
		case '-':
			pat++;
			while(rematch(*input++) < 0)
				if(*input == '\0')
					return(-1);
			break;
		case '$':
			pat++;
			save = (&arg1)[*pat++ - '1'];
			while(rematch(*input) < 0) {
				if(*input == '\0')
					return(-1);
				*save++ = *input++;
			}
			input++;
			savecnt++;
			*save = '\0';
			break;
		case '\'':
			pat++;
			p = pat;
			while(*pat != '\'') {
				pat = p;
				while(rematch(*input) == 0 && *pat != '\'')
					input++;
				if(*input == '\0')
					return(-1);
				if(pat == p)
					input++;
			}
			pat++;
			input++;
			break;
		case '\\':
			pat++;
		default:
			if(rematch(*input++) < 0)
				return(-1);
		}
	}
	return(savecnt);
}


/* Regular expression match.  Test if the RE at pat
 * matches the char c.  If it does advance pat past the
 * matched RE and return 0.  If it doesn't return -1.
 */
static
rematch(c)
register char c;
{
	register char *mark;
	register int flag;

	mark = pat;
	switch(*pat) {
	case '[':
		flag = 0;
		while(*pat != ']')
			if(*pat++ == c) {
				lastmatch = c;
				flag++;
			}
		if(flag) {
			pat++;
			return(0);
		}
		break;
	case '.':
		pat++;
		return(0);
	case '\\':
		pat++;
	default:
		if(*pat == c) {
			if(*pat)
				pat++;
			return(0);
		}
	}
	pat = mark;
	return(-1);
}

#include <stdio.h>
#include <pwd.h>

#define PERSONSIZE	40	/* max size of username */
/*
    tilde

    Return the original file name with possible tilde expansion.

	original = 	~person/mumble
    expands to
	expanded = /home_directory_of_person/mumble

    THIS ROUTINE ASSUMES THAT ITS ARGUMENT HAS BEEN ALLOCATED
    USING MALLOC AND FREEs ITS STORAGE IF NECESSARY FOR EXPANSION.

    Return with no expansion if "person" is invalid or there is
    no tilde, otherwise return expanded result.
*/
char *tilde (orig)
char *orig;
{
    extern struct passwd *getpwuid(), *getpwnam();

    char *getenv();
    register char *o, *p;
    register struct passwd *pw;
    char person[PERSONSIZE];
    char *new, *expand;

    /* No leading tilde? No translation. Return orig string */
    if (orig[0] != '~')
	return (orig);

    /*
     Copy everything after the tilde up to EOS or
     start of subdir name.
    */
    for (p = person, o = &orig[1]; *o && *o != '/'; *p++ = *o++);
    *p = '\0';

    if (person[0] == '\0')		/* Use current uid if no name */
	expand = getenv("HOME");	/* translate from environment */
    else
	{
	pw = getpwnam (person);		/* Else lookup on name */
	
	if (pw == NULL)			/* Not found? Return Error. */
	    return (orig);

	expand = pw->pw_dir;
	}

    /* place the translation into the orig name */
    new = (char *)strcpy(malloc (strlen(expand) + strlen (o) + 1), expand);

    strcat (new, o);		/* add the remainder of the spec */

    free(orig);			/* free the original string */

    return (new);		/* return the result */

}/* end tilde() */


