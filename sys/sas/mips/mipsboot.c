/*
 * mipsboot.c
 */

#ifndef lint
static char *sccsid = "@(#)mipsboot.c	4.2	(ULTRIX)	10/9/90";
#endif lint
/************************************************************************
 *									*
 *			Copyright (c) 1985 by				*
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

/* 
 * Revision History
 *
 * jas - Added entrypt.h.  It contains rex callbacks.  Modified code to
 *       use rex callbacks if envp indicates new console present.
 */

#include "../h/param.h"
#include "../machine/mips/cpu.h"
#include "../machine/mips/entrypt.h"

#define stop prom_restart
#define printf _prom_printf

char    *imagename = "ultrixboot";

/*
 * Functional Discription:
 *
 * Inputs:
 *	none
 *
 * Outputs:
 *	none
 *
 */

int prom_io;

int ub_argc;
char **ub_argv;
char **ub_envp;
char **ub_vector;

extern int rex_base;  /* used by REX console */
extern int rex_magicid; /* used by REX console */

main (argc,argv,envp,vector)
int argc;
char **argv, **envp, **vector;
{
	int     io;
	char	*boot;

	ub_argc = argc;	/* save prom's args for kernel */
	ub_argv = argv;	/* save prom's args for kernel */
	ub_envp = envp;	/* save prom's args for kernel */
	ub_vector = vector; /* save prom's args for kernel */

	rex_magicid = (int)envp;

	if((int)envp == REX_MAGIC) {
		rex_base = (int)vector;
		if(rex_bootinit() < 0) {
			printf("binit fld\n");
			exit();
		}
	}
	else {
		rex_base = 0;
		boot = (char *)prom_getenv("boot");
		if ((prom_io = _prom_open(boot, 0)) < 0 )	{
			printf("dev open failed\n");
			exit();
		}
	}

	if ((io = open (imagename, 0)) < 0) {	/* Open the image */
		printf("can't open %s\n", imagename);
		exit();
	}

	load_image (io);	
	exit();
}

exit()
{
        if(!rex_base) 
		_prom_close(prom_io);
	stop();
}

