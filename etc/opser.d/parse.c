
#ifndef lint
static	char	*sccsid = "@(#)parse.c	4.1  (ULTRIX)        7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1984 by				*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any	other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or	reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/
/*
 * parse.c
 *
 *	static char Sccsid[] = "@(#)parse.c     1.0     08/06/86";
 *
 *	Modification History:
 *	
 *	8/03/86 - dike
 *		Created parse file.
 *
 */
#include "opser.h"

extern char last_typed;
extern char *args[];

int parse(instr,commands,pntr)
char *instr,*commands[];
int pntr;
{
	int i,first_arg;
	char c,**cmd;
	
	i = 0;
	while(isspace(instr[i])) i++;
	while((!isspace(instr[i])) && (instr[i] != '\0')) i++;
	instr[i] = '\0';
	if(i == pntr) i = -2;
	first_arg = i+1;
	for(i = 0, cmd = commands; *cmd; *cmd++, i++){
	    /*take length of shortest
	     *string; match to commands
	     *in commands[].
	     */
#if defined(vax) || defined(mips)
	    if(strncmp(*cmd, instr,
		       (strlen(*cmd)<strlen(instr))
			?strlen(*cmd):strlen(instr)) == 0)
#endif
#ifdef ultrix11
	    if(strncmp(*cmd, &instr,
		       (strlen(*cmd)<strlen(&instr))
			?strlen(*cmd):strlen(&instr)) == 0)
#endif ultrix11
		break;
	}
	args[0] = NULL;
	if(first_arg != -1) get_args(&instr[first_arg]);
	return(i);
}
