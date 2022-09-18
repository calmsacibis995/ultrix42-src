#ifndef lint
static	char	*sccsid = "@(#)main.c	4.2	(ULTRIX)	9/10/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1983,86 by			*
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

/*-----------------------------------------------------------------------
 *
 *	Modification History
 *
 * 30-Aug-90 -- skc
 *      Added initialization of shadow device table pointer to NULL.
 * 
 * 20-Dec-89 -- Paul Grist
 *      corrected and added comments for clarification while adding
 *      VMEbus support to config.
 *
 * 17-Oct-89 -- Randall Brown
 *	Added check on DS3100 and DS5000 to see if 'dc' device was
 *	configured.
 *
 * 19-July-89 -- robin
 *	added mips based changes to support vax baseed devices
 *
 * 24-Feb-89 -- map (Mark Parenti)
 *	Remove last remnants of xos.
 *
 * 10-Feb-89 -- map (Mark Parenti)
 *	Use upmachinename to create output directory
 *
 * 12-Apr-87 -- fred (Fred Canter)
 *	Added xos variable. Search device table and set xos if
 *	pseudo-device xos configured.
 *
 * 15-Apr-86 -- afd
 *	Removed reference to MACHINE_MVAX
 *	Initialize "emulation_instr" to 0.
 *
 * 05-Mar-86 -- jrs
 *	Modified to support configuration of bi based devices
 *
 *-----------------------------------------------------------------------
 */

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "y.tab.h"
#include "config.h"

#ifndef TRUE
#define	TRUE	(1)
#endif

char *progname;


/*
 * Config builds a set of files for building a UNIX
 * system given a description of the desired system.
 */
main(argc, argv)
	int argc;
	char **argv;
{
	register struct device *dp;
	extern char *optarg;
	extern int optind;
	int optc,errflg;

	while ((optc=getopt(argc, argv, "kps")) != EOF) {
		switch(optc) {

		case 'k':
			kdebug = TRUE;
			break;
		case 'p':
			profiling = TRUE;
			break;
		case 's':
			source = TRUE;
			break;
		case '?':
			errflg = TRUE;
			break;
		}
	}
	if(((argc-optind) != 1) || errflg) {	/* no error and 1 arg left */
		fprintf(stderr, "usage: config [-kps] sysname\n");
		exit(2);
	}
	PREFIX = argv[optind];
	if (freopen(argv[optind], "r", stdin) == NULL) {
		perror(argv[optind]);
		exit(2);
	}
	dtab = NULL;
	highuba = -1;
	extrauba = 0;
	emulation_instr = 0;
	confp = &conf_list;
        shad_tabp = NULL;
	if (yyparse())
		exit(3);
	switch (machine) {

	case MACHINE_VAX:
		dec_ioconf();		/* Print ioconf.c */
		ubglue();		/* Create ubglue.s */
		break;

	case MACHINE_SUN:
		sun_ioconf();
		break;
	
	case MACHINE_MIPS:
		dec_ioconf();		/* Print ioconf.c  */
		/* Check to see that dc is declared in config file */
		if (is_cpu_declared("DS3100") || is_cpu_declared("DS5000"))
		    if (!isconfigured("dc")) {
			printf("config file error: must specify device dc\n");
			exit(1);
		    }
		ubglue();		/* Create scb_vec.c */
		break;

	default:
		printf("Specify machine type, e.g. ``machine vax or mips''\n");
		exit(1);
	}
	makefile();			/* build Makefile */
	headers();			/* make a lot of .h files */
	swapconf();			/* swap config files */
	printf("Don't forget to run \"make depend\"\n");
	exit(0);
}

/*
 * get_word
 *	returns EOF on end of file
 *	NULL on end of line
 *	pointer to the word otherwise
 */
char *
get_word(fp)
	register FILE *fp;
{
	static char line[80];
	register int ch;
	register char *cp;

	while ((ch = getc(fp)) != EOF)
		if (ch != ' ' && ch != '\t')
			break;
	if (ch == EOF)
		return ((char *)EOF);
	if (ch == '\n')
		return (NULL);
	cp = line;
	*cp++ = ch;
	while ((ch = getc(fp)) != EOF) {
		if (isspace(ch))
			break;
		*cp++ = ch;
	}
	*cp = 0;
	if (ch == EOF)
		return ((char *)EOF);
	(void) ungetc(ch, fp);
	return (line);
}

/*
 * prepend the path to a filename
 */
#define PATH_EXTRA 20

char *
path(file)
	char *file;
{
	register char *cp;
	register struct stat tmpbuf;
	char buf[1024];

	cp = malloc((unsigned)(strlen(PREFIX)+strlen(file)+PATH_EXTRA));

	switch (machine) {

	case MACHINE_VAX:
		(void) strcpy(cp, "../../VAX");
		break;

	case MACHINE_SUN:
		(void) strcpy(cp, "../../SUN");
		break;
	
	case MACHINE_MIPS:
		(void) strcpy(cp, "../../MIPS");
		break;
	}

	/* (void) strcat(cp, machinename); */
	(void) strcat(cp, "/");
	(void) strcat(cp, PREFIX);
	if(stat(cp, &tmpbuf)) {
		(void) sprintf(buf,"/bin/mkdir %s",cp);
		(void) system(buf);
	}
	(void) strcat(cp, "/");
	(void) strcat(cp, file);
	return (cp);
}
