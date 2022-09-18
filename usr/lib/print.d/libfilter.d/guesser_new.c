#ifndef lint
static char *sccsid = "@(#)guesser_new.c	4.1      ULTRIX 	10/16/90";
#endif

/************************************************************************
 *									*
 *			Copyright (c) 1990 by				*
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
 * File:	guesser_new.c
 * Author:	Adrian Thoms (thoms@wessex)
 * Description:
 *	This file exports the interface to the new file guesser
 *	It maps across the major and minor file type numbers produced
 *	by the fdtype() function into the much smaller and simpler set
 *	of types understood by the switch code in the filters.
 *
 * Modification History:
 */
#include "guesser.h"
#include "filetype.h"

int determinefile();

int in;
int globi;
char	filestorage[HOW_MUCH_TO_CHECK+1];		/* first chunk of pipe	*/

int
determinefile(fd)
	int fd;
{
	long mashed_type;
	register int major, minor;
	register int file_type;

	binary_mkmtab();
#ifdef TESTING
	mashed_type = fdtype(fd, 0, &in, filestorage, HOW_MUCH_TO_CHECK, PRINT);
#else
	mashed_type = fdtype(fd, 0, &in, filestorage, HOW_MUCH_TO_CHECK, NOPRINT);
#endif
	major = MAJOR(mashed_type);
	minor = MINOR(mashed_type);
#ifdef TESTING
	printf("File type (%d, %d)\n", major, minor);
#endif
	switch(major) {
	case UNKNOWN:
	default:
		file_type = TEXT_FILE; /* When in doubt, print it */
		break;

	case SLINK:
	case DIRECTORY:
		/* case APPENDONLY: */
		/* case STANDARD: */
	case NAMEDPIPE:
	case SOCKET:
	case SPECIAL:
		/* case BLOCK: */
		/* case CHARACTER: */
	case EMPTY:
		file_type = EMPTY_FILE;
		break;

	case ASCII:
	case ASCIIwGARBAGE:
		switch(minor) {
		case SCCS:
			file_type = DATA_FILE;
			break;

		case SHELL:
		case BSHELL:
		case CSHELL:
		case CPROG:
		case FORTPROG:
		case ASSEMBLER:
		case NROFF:
		case TEXT:
			file_type = TEXT_FILE;
			break;

		case CPIOARCHIVE:/* also used under DATA */
		case TROFFINT:
		case POSTSCRIPT:
			file_type = DATA_FILE;
			break;

		case COMMANDS:
		case ENGLISH:
			file_type = TEXT_FILE;
			break;
		}
		break;

	case PRESS:
		file_type = DATA_FILE;
		break;

	case DATA:
		switch(minor) {
		case DDIF:
			file_type = DATA_FILE;
			break;

		case CAT_TROFF:
			file_type = CAT_FILE;
			break;

		case X_IMAGE:
			file_type = XIMAGE_FILE;
			break;

		case COMPACTED:
		case COMPRESSED:
		case UUENCODED:
		case PACKED:
			file_type = DATA_FILE;
			break;

		case LN03:
			file_type = ANSI_FILE;
			break;
		}
		break;

	case EXECUTABLE:
		/* case PDP11SA: */
		/* case E411: */
		/* case E410: */
		/* case E413: */
		/* case PDP430: */
		/* case PDP431: */
		/* case PDP450: */
		/* case PDP451: */
	case ARCHIVE:
		/* case VERYOLD: */
		/* case OLDARCH: */
		/* case STANDARD: */
		/* case RANLIB: */
		file_type = DATA_FILE;
		break;
	}
	return file_type;
}

#ifdef TESTING
#include <stdio.h>

static char *file_types[] = {
	"EMPTY_FILE",		/* 0 */
	"EXECUTABLE_FILE",	/* 1 */
	"ARCHIVE_FILE",		/* 2 */
	"DATA_FILE",		/* 3 */
	"TEXT_FILE",		/* 4 */
	"CTEXT_FILE",		/* 5 */
	"ATEXT_FILE",		/* 6 */
	"RTEXT_FILE",		/* 7 */
	"FTEXT_FILE",		/* 8 */
	"CAT_FILE",		/* 9 */
	"XIMAGE_FILE",		/* 10 */
	"POSTSCRIPT_FILE",	/* 11 */
	"ANSI_FILE"		/* 12 */
};

#define NFILE_TYPES	(sizeof(file_types)/sizeof(file_types[0]))

FILE *input;

main(argc, argv)
int argc;
char *argv[];
{
	char *file;
	if (argc == 1) {
		/* ghastly kludge */
		argv[1] = "-"; argc=2;
	}
	while (--argc) {
		int file_type;

		file = *++argv;
		if (!strcmp(file, "-")) {
			input = stdin;
			file="<stdin>";
		} else {
			input=fopen(file, "r");
			if (input == NULL) {
				fprintf(stderr, "Can't open %s\n", file);
				continue;
			}
		}

		file_type=determinefile(fileno(input));
		if ((unsigned) file_type > NFILE_TYPES) {
			fprintf(stderr, "File %s is of file_type %d\n", file, file_type);
		} else {
			fprintf(stderr, "File %s is of file_type %s\n", file, file_types[file_type]);
		}

	}

}

#endif
