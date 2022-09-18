#ifndef lint
static	char	*sccsid = "@(#)sizer.c	4.2  (ULTRIX)        10/15/90";
#endif lint
/************************************************************************
 *									*
 *			Copyright (c) 1987 by				*
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
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/


/************************************************************************
 *
 * Name: sizer.c
 *
 *	Usage:
 *	sizer [-s][-c][-b][-r][-wt][-wu][-k kfile] [-n sysnam] [-t timezone]
 *
 *	-b	Create the boot commands for the chosen system disk
 *	-c	Write the cpu type (only) to standard output
 *	-k	Use the next argument as an alternate kernel file
 *	-n	The next arguement is the system name
 *	-r	Write the root device (only) to standard output
 *	-t	The next argument is the "timezone" string for config
 *	-s	Write the cpu subtype (only) to standard output
 *	-wt	Write workstation display type field to standard output
 *	-wu	Write workstation display units field to standard output
 *
 * Modification History
 *
 * Oct 15, 1990 - Paul Grist
 *      Bugfix for error checking on sizer commands with arguments,
 *      the old check resulted in a seg fault on mips because of
 *      a null pointer situation.
 * 
 * Dec 15, 1989 - Alan Frechette
 *	Added 2 more options "-wt" and "-wu" to display the workstation
 *	display type field and the workstation display units field.
 *
 * May 10, 1989 - Alan Frechette
 *	Added an error message for the name list type "NL_roottype"
 *	which was added to determine if a network boot was performed.
 *
 * Feb 12, 1989 - Alan Frechette
 *	New sizer code which supports multiple architectures.
 *      This file is a complete redesign and rewrite of the 
 *	original V3.0 sizer code by Tungning Cherng.
 *
 ***********************************************************************/

#include "sizer.h"

char *sysname;
char *tzone;
char *altfile;

main(argc, argv)
char **argv;
int argc;
{

	int kflag, tflag;
	char option;

	tzone = altfile = sysname = NULL;

    	getargs(argv, argc, &option, &kflag, &tflag);

	nlist((kflag) ? altfile : "/vmunix", nl);

	if(nl[NL_cpu].n_type == N_UNDF && 
		nl[NL_tz].n_type == N_UNDF &&
			nl[NL_rootdev].n_type == N_UNDF)
		quitonerror (-1);

	if((kmem = open("/dev/kmem", 0)) < 0)
		quitonerror (-2);

	if(signal(SIGINT,SIG_IGN) != SIG_IGN)
		signal(SIGINT,SIG_IGN);

	switch(option) {
	case 'n': 
		getconfig(sysname, tzone);
		break;

	case 'c': 
		getcpu(DISPLAY);
		break;

	case 'r': 
		getroot(DISPLAY);
		break;

	case 'b':
		getcpu(NODISPLAY);
		getboot();
		break;

	case 's':
		getsubcpu(DISPLAY);
		break;

	case 'w':
		getwsinfo(tflag);
		break;

	default: 
		break;
	}  
	exit(0);
}

/****************************************************************
*								*
* getargs 							*
*								*
* Parse the command line arguments.				*
*								*
****************************************************************/
getargs(argv, argc, option, kflag, tflag)
char **argv;
int argc;
char *option;
int *kflag;
int *tflag;
{

	int k;
	int error;

	*kflag = *option = error = 0;
	for(k=1; (k<argc && !error); k++) {
	    	switch(argv[k][1]) {
		case 'k': 	/* -k: use alternate file */
			k++;
			*kflag = 1;
			if(argc == 2)
				error++;
	    		else
		    		altfile = argv[k];
	    		break;

		case 'n': 	/* -n: system name */
	    		k++;
			*option = 'n';
	    		if(argc == 2)
				error++;
	    		else
		    		sysname = argv[k];
	   		break;

		case 'r': 	/* -r: display root device */
			*option = 'r';
			break;

		case 'b': 	/* -b: create boot stuff */
			*option = 'b';
			break;

		case 'c': 	/* -c: display cpu type */
			*option = 'c';
			break;

		case 's': 	/* -s: display cpu subtype */
			*option = 's';
			break;

		case 't':	/* -t: get time zone for -n */
			k++;
			if(argc == 2)
				error++;
			else
				tzone = argv[k];
			break;

		case 'w':	/* -w: get workstation display info */
			*option = 'w';
			if(argv[k][2] == 't')
				*tflag = 1;
			else if(argv[k][2] == 'u')
				*tflag = 2;
			else
				error++;
			break;

		default: 
			error++;
			break;
		}
	}
	if(error || argc == 1) {
		usage();
		exit(1);
	}
}

/****************************************************************
* quitonerror							*
*								*
* If an error occurs control is passed to this routine to 	*
* allow for a graceful exit.					*
*								*
****************************************************************/
quitonerror(code)
int code;
{

	switch(code) {
	case -1: 
		fprintf(stderr, "No namelist found.\n");
		break;

	case -2: 
		fprintf(stderr, "Cannot read kernel memory.\n");
		break;

	case -3: 
		fprintf(stderr, "Too many errors (ABORTING).\n");
		break;

	case -4: 
		fprintf(stderr, "Cannot get the cpu information.\n");
		break;

	case -5: 
		fprintf(stderr, "Cannot get the physical memory size.\n");
		break;

	case -6: 
		fprintf(stderr, "Cannot get the timezone information.\n");
		break;

	case -7: 
		fprintf(stderr, "Cannot get the rpb boot information.\n");
		break;

	case -8:
		fprintf(stderr, "Cannot open the console boot command file.\n");
		break;

	case -9:
		fprintf(stderr, "Cannot read UNIBUS/QBUS memory.\n");
		break;

	case -10:
		fprintf(stderr, "Cannot create the MAKEDEVICES File.\n");
		break;

	case -11:
		fprintf(stderr, "Cannot create the CONFIG File.\n");
		break;

	case -12:
		fprintf(stderr, 
			"Cannot get the root, swap, or dump information.\n");
		break;

	case -13:
		fprintf(stderr, "Cannot get the workstation display type.\n");
		break;

	case -14:
		fprintf(stderr, "Cannot get the roottype information.\n");
		break;

	default: 
		break;
	}
	exit(-1);
}

/****************************************************************
*								*
* usage 							*
*								*
* Display usage message.					*
*								*
****************************************************************/
usage()
{

	fprintf(stderr,"\nUsage: sizer\n");
	fprintf(stderr,"-b\t\t Create a boot command file.\n");
	fprintf(stderr,"-s\t\t Returns cpu subtype.\n");
	fprintf(stderr,"-c\t\t Returns cpu type.\n");
	fprintf(stderr,"-r\t\t Returns root device.\n");
	fprintf(stderr,"-k image\t Use image instead of /vmunix.\n");
	fprintf(stderr,"-n name \t Create a config file using this name.\n");
	fprintf(stderr,"-t timezone\t Use timezone in the config file.\n");
	fprintf(stderr,"-wt\t\t Returns workstation display type.\n");
	fprintf(stderr,"-wu\t\t Returns workstation display units.\n\n");
}
