#ifdef lint
static char sccsid[] = "@(#)ultrix_main.c	4.1	ULTRIX	7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1988 by				*
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

/*
 * ultrix_main.c -- Ultrix main for regi_ps and tek4014_ps xlator
 *
 * Description:
 *	This is a calling harness used by both the regis
 *	and tek4014 to PostScript translators.
 *
 *	It is based on the calling harness for the ANSI
 *	to PostScript translator.
 *
 *	The command line arguments (or defaults) are used
 *	to build the VMS-style item list which is passed
 *	to the translator proper.
 */

/* SCCS history beginning
 * ***************************************************************
 *                -- Revision History --
 * ***************************************************************
 * 
 * 1.1  31/05/88 -- thoms
 * date and time created 88/05/31 19:54:57 by thoms
 * 
 * ***************************************************************
 *
 * 1.2  19/07/88 -- thoms
 * Added copyright notice and modification history
 *
 * 1.3  1/08/88 -- maxwell
 * corrected call to page_size_lookup to use expanded pagesize rather
 * than addrev.
 *
 * SCCS history end
 */


/*
 * Translator options
 * ------------------
 * 
 *  -F <page_size>	a,a3,a4,a5,b,b4,b5,executive,legal,
 *				c4,c5,dl,10x13,9x12,bus
 *	 			(all except dl are implemented)	default is a4
 *  -O <orientation>	landscape,portrait
 *	 			default is portrait
 *  -s			inhibit final showpage
 *				needed for multi-page printing
 * 
 *  The order of the options is unimportant.
 */

#include "portab.h"
#include <stdio.h>
#include "trn$.h"
#include "argstrings.h"
#include "page_sizes.h"

static char	*arg0;  /* program name */

/* ITEM Declaration (Itemlist members are ITEMS) */
/*-----------------------------------------------*/
typedef struct {
	UWORD	length;
	UWORD	code;
	ULONG	address;
} ITEM;

static ITEM	itemlist [20];

/* initialise mode variables to default values */

static BOOLEAN	noshowlastpage = 0;
static UBYTE	orientation = trn$k_page_portrait;

static PAGE_SIZE *page_size=0;

main (argc, argv)
int     argc;
char    *argv [];
{
	static char *options = "F:O:v:es87gf";
	extern int optind;	/* in getopt */
	extern char *optarg;	/* in getopt */

	/* forward declarations */
	extern void PUT_XLBUF();
	extern void GET_XLBUF();
	static int build_item_list();

	char	*dummy;
	int	fin = 0, fout = 1; /* descriptors default to stdin/stdout */
	int	opt;

	arg0 = argv[0];

	/* parse command line options */

	init_args();

	while ((opt = getopt(argc,argv,options)) != EOF) {
		switch (opt) {

		    case 'F':
			if (check_arg(optarg,as_page_sizes,&dummy)  !=  0) {
				fatal("invalid argument %s",optarg);
			}
			if (!(page_size=page_size_lookup(dummy))) {
				fatal("not yet implemented %s",optarg);
			}
			break;
		    case 'O':
			if (check_arg(optarg,as_orientations,&dummy)  !=  0)
			    fatal("invalid argument %s",optarg);
			else switch(optarg[0]) {

			    case 'l':
				orientation = trn$k_page_landscape;
				break;
			    case 'p':
				orientation = trn$k_page_portrait;
				break;
			}
			break;
		    case 'v':
			break;
		    case 'e':
			break;
		    case 's':
			noshowlastpage++;
			break;
		    case '8':
			break;
		    case 'g':
			break;
		    case '?':
			exit(2);
		}
	}
	if (!page_size && !(page_size=page_size_lookup("a"))) {
		fatal("Don't know about size a paper\n");
	}
	build_item_list();

#ifdef DEBUG
	print_item_list();
#endif

	TRANSLATOR(GET_XLBUF, fin, PUT_XLBUF, fout, itemlist);

	close (fin);
	close (fout);
}


static int build_item_list()
{
	WORD	cur_i = 0;

	itemlist[cur_i].code = trn$_page_orientation;
	itemlist[cur_i++].address = orientation;

	/* The numbers for the itemlist data has to be given in decipoint.  */
	/* (There are 72 decipoints to the inch and 72/25.4 decipoints)     */


	itemlist[cur_i].code = trn$_page_height;
	itemlist[cur_i++].address = page_size->sz_height;

	itemlist[cur_i].code = trn$_page_width;
	itemlist[cur_i++].address = page_size->sz_width;

	if (noshowlastpage)  {
		itemlist[cur_i].code = trn$_page_fragment;
		itemlist[cur_i++].address = trn$k_page_fragment;
	}
	itemlist[cur_i].code = trn$_end_of_list;
}

#ifdef DEBUG
print_item_list()
{
	int i;
	for(i=0; itemlist[i].code != trn$_end_of_list; i++) {
		fprintf(stderr, "code %d address %d\n",
			itemlist[i].code,itemlist[i].address);
	}
}
#endif

/*****************************************************************/
/* fatal error - print message on stderr                         */
/*****************************************************************/

/*VARARGS1*/
fatal(msg, a1, a2, a3)
	char *msg;
{
	fprintf(stderr,"%s: ", arg0);
	fprintf(stderr,msg, a1, a2, a3);
	fputc('\n',stderr);
	exit(2);
}
