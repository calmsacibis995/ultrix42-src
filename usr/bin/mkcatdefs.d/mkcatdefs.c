#ifndef lint
static	char	*sccsid = "@(#)mkcatdefs.c	4.1	(ULTRIX)	12/6/90";
#endif lint

/************************************************************************
 *									*
 *         Copyright (c) Digital Equipment Corporation, 1990		*
 *									*
 *   All Rights Reserved.  Unpublished rights  reserved  under  the	*
 *   copyright laws of the United States.				*
 *									*
 *   The software contained on this media  is  proprietary  to  and	*
 *   embodies  the  confidential  technology  of  Digital Equipment	*
 *   Corporation.  Possession, use, duplication or dissemination of	*
 *   the  software and media is authorized only pursuant to a valid	*
 *   written license from Digital Equipment Corporation.		*
 *									*
 *   RESTRICTED RIGHTS LEGEND   Use, duplication, or disclosure  by	*
 *   the U.S. Government is subject to restrictions as set forth in	*
 *   Subparagraph (c)(1)(ii)  of  DFARS  252.227-7013,  or  in  FAR	*
 *   52.227-19, as applicable.						*
 *									*
 ************************************************************************/
/**/

/*
 * @OSF_COPYRIGHT@
 */
/*
 * HISTORY
 * $Log:	mkcatdefs.c,v $
 * Revision 1.4  90/10/07  16:45:30  devrcs
 * 	Added EndLog Marker.
 * 	[90/09/28  14:53:40  gm]
 * 
 * Revision 1.3  90/09/13  12:17:27  devrcs
 * 	remove setlocale decl
 * 	[90/08/29  13:09:24  mbrown]
 * 
 * 	Cleanup and update with changes in Golden code.
 * 	[90/08/24  10:54:39  bet]
 * 
 * Revision 1.2  90/05/13  20:40:06  devrcs
 * 	new for OSF/1
 * 	[90/05/03  13:03:48  mbrown]
 * 
 * $EndLog$
 *
 * #if !defined(lint) && !defined(_NOIDENT)
 * static char rcsid[] = "@(#)$RCSfile: mkcatdefs.c,v $ $Revision: 1.4 $ (OSF) $Date: 90/10/07 16:45:30 $";
 * #endif
 *
 * COMPONENT_NAME: CMDMSG
 *
 * FUNCTIONS: main, mkcatdefs, incl, chkcontin 
 *
 * ORIGINS: 27
 *
 * IBM CONFIDENTIAL -- (IBM Confidential Restricted when
 * combined with the aggregated modules for this product)
 * OBJECT CODE ONLY SOURCE MATERIALS
 * (C) COPYRIGHT International Business Machines Corp. 1988, 1989
 * All Rights Reserved
 *
 * US Government Users Restricted Rights - Use, duplication or
 * disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
 *
 * mkcatdefs.c	1.16  com/cmd/msg,3.1,9013 3/1/90 15:39:05
 */

#include <stdio.h>
#include <locale.h>
#ifdef KJI
#include <NLctype.h>
#else
#include <ctype.h>
#endif
#include <sys/dir.h>
#include "nl_types.h"  /*wendy*/
#include <limits.h>
#include <string.h>
#include "msgfac_msg.h"
#define	 MSGSTR(N,S)	catgets(errcatd,MS_MKCATDEFS,N,S)

#define MAXLINELEN NL_TEXTMAX
#define KEY_START '$'
#define MAXIDLEN 64
#ifdef _D_NAME_MAX
#define MDIRSIZ _D_NAME_MAX
#else
#define MDIRSIZ 14
#endif

/*
 * EXTERNAL PROCEDURES CALLED: descopen, descclose, descset, descgets,
 *                             descerrck, insert, nsearch
 */

char *descgets();
nl_catd errcatd;
static int errflg = 0;
static int setno = 1;
static int msgno = 1;
static int symbflg = 0;
static int inclfile = 1;
FILE *outfp;
FILE *msgfp;
static char inname [PATH_MAX];
static char outname [PATH_MAX];
static char catname [PATH_MAX];
char *mname;

/*
 * NAME: main
 *
 * FUNCTION: Make message catalog defines.
 *
 * EXECUTION ENVIRONMENT:
 *  	User mode.
 *
 * NOTES:  Invoked by:
 *         mkcatdefs <name> <msg_file>
 *
 *  	Results are 1) Creates header file <name>.h.
 *                  2) Displays message file to stdout. The message file is 
 *                     ready to be used as input to gencat.
 *
 *   	mkcatdefs takes a message definition file and produces
 *  	a header file containing #defines for the message catalog,
 * 	the message sets and the messages themselves.  It also
 *  	produces a new message file which has the symbolic message set and
 *  	message identifiers replaced by their numeric values (in the form
 *  	required by gencat).
 *
 * DATA STRUCTURES: Effects on global data structures -- none.
 *
 * RETURNS: 1 - error condition
 */

main (argc, argv) 
int argc;
char *argv[];

{
    register int i;
    register char *cp;
    int count;
    char *t;

    setlocale (LC_ALL,"");
    errcatd = catopen(MF_MSGFAC,0);

    /* usage: handle multiple files; -h option has to be at the end */
    if (argc < 3) {
	fprintf (stderr, MSGSTR(MKCATUSAGE,"mkcatdefs: Usage: %s catname msg_file [msg_file...] [-h]\n"), argv [0]);	
	exit (0);
    }

    /* check if  include file should be created; -h is the last argument */
    if (argv[argc-1][0] == '-' && argv[argc-1][1] == 'h') 
		inclfile = 0;

    /* open header output file */
    if (inclfile) {
	mname = argv [1];
	if ((strlen((t = strrchr(mname,'/')) ? t + 1 : mname) +
             sizeof("_msg.h") - 1) > MDIRSIZ) {
		fprintf (stderr, MSGSTR(MNAMTOOLONG, "mkcatdefs: catname too long\n"));
		exit (1);
	}
    	sprintf (outname, "%s_msg.h", mname);
	if (strrchr(mname,'/'))
	    mname = strrchr(mname,'/') + 1;
        sprintf (catname, "%s.cat", mname);
    	if ((outfp = fopen (outname, "w")) == NULL) {
		fprintf (stderr, MSGSTR(MKCATOPN, "mkcatdefs: Cannot open %s\n"), outname);								/*MSG*/
		exit (1);
	} else  {
    		/* convert name to upper case */
    		for (cp=mname; *cp; cp++)
			if (islower (*cp))
	    			*cp = _toupper (*cp);
			else if (!isupper (*cp) && !isdigit (*cp))
				*cp = '_';

                incl ("#ifndef _H_%s_MSG \n", mname);
                incl ("#define _H_%s_MSG \n", mname);
    		incl ("#include <limits.h>\n");
    		incl ("#include <nl_types.h>\n");
    		incl ("#define MF_%s \"%s\"\n\n", mname, catname);
	}
    } else sprintf (outname, "msg.h");


    /* open new msg output file */
    msgfp = stdout;

/* if message descriptor files were specified then process each one in turn */
         
    if (inclfile == 0 )
        count = argc - 1;
    else
        count = argc;
    for (i = 2; i < count; i++) {
    /* open input file */
    	sprintf (inname, "%s", argv[i]);
	if (strcmp(inname,"-") == 0) {
		strcpy(inname,"stdin");
		descset(stdin);       /* input from stdin if no source files */
		mkcatdefs(inname);
	} else	{
		if (descopen(inname) < 0) {
			fprintf (stderr, MSGSTR(MKCATOPN,"mkcatdefs: Cannot open %s\n"), inname);							/*MSG*/
			errflg = 1;
		} else  {
			mkcatdefs (inname);
			descclose();
		}
	}
    }
    incl ("#endif \n");

    if (inclfile) {
    	fflush (outfp);
    	if (ferror (outfp)) {
		fprintf (stderr, MSGSTR(WRITERRS,"mkcatdefs: There were write errors on file %s\n"), outname);						/*MSG*/
		errflg = 1;
	}
    	fclose (outfp);
    }

    if (errflg) {
	fprintf (stderr, MSGSTR(ERRFND,"mkcatdefs: Errors found: no %s created\n"), outname);								/*MSG*/
	if (inclfile)  unlink(outname);
    } else {
	   if (inclfile) {
		if (symbflg)
			fprintf (stderr, MSGSTR(HCREAT,"mkcatdefs: %s created\n"), outname);
	   	else {
			fprintf (stderr, MSGSTR(NOSYMB,"mkcatdefs: No symbolic identifiers; no %s created\n"), outname);				/*MSG*/
			unlink (outname);
		}
   	   } 
	   else 
                fprintf(stderr,MSGSTR(NOHDR,"mkcatdefs: no %s created\n"), outname);                                      				/*MSG*/
    }
    exit (errflg);
}

/*
 * NAME: mkcatdefs
 *
 * FUNCTION: Make message catalog definitions.
 *
 * EXECUTION ENVIRONMENT:
 *  	User mode.
 *
 * RETURNS: None
 */


mkcatdefs (fname)
char *fname;

	/*---- fname: message descriptor file name ----*/

{
    char msgname [PATH_MAX];
    char line [MAXLINELEN];
    register char *cp;
    register char *cpt;
    register int m;
    register int n;
    int contin = 0;


    /* put out header for include file */
    incl ("\n\n/* The following was generated from %s. */\n\n",fname);

    /* process the message file */
    while (descgets (line, MAXLINELEN) ) {
#ifdef KJI
	for (cp=line; isspace (*cp); cp += NLchrlen(cp));
        /* find first nonblank character */
#else
	for (cp=line; isspace (*cp); cp++);		  
	/* find first nonblank character */
#endif
	    if (*cp == KEY_START) {
		cp++;
		if (isspace (*cp)) {
#ifdef KJI
			for (; isspace (*cp); cp += NLchrlen(cp)); 
#else
			for (; isspace (*cp); cp++);
#endif
		    sscanf (cp, "%s", msgname);
		    if ((m = nsearch(msgname)) > 0) {
			fprintf (msgfp, "$ %d", m);
			cp += strlen(msgname);
			fprintf (msgfp, "%s", cp);
		    } else
		    	fputs (line, msgfp);
		    continue; /* line is a comment */
		}
		if (strncmp (cp, "set", 3) == 0 && isspace (cp[3])) {
    		    char setname [MAXIDLEN];

		    sscanf (cp+4, "%s", setname);
		    incl ("\n/* definitions for set %s */\n", setname);
		    if (isdigit(setname[0])) {
			    cpt = setname;
			    do  {
				if (!isdigit(*cpt)) {
				   fprintf(stderr,MSGSTR(ZEROINV, "mkcatdefs: %s is an invalid identifier\n"), setname);
					errflg = 1;
					break;
				}
			    }   while (*++cpt);
			n = atoi (setname);
			if (n >= setno)
			    	setno = n;
		        else {
				if (n = 0)
				   fprintf(stderr,MSGSTR(ZEROINV, "mkcatdefs: %s is an invalid identifier\n"), setname);	
				else
				   fprintf(stderr,MSGSTR(INVLDSET, "mkcatdefs: set # %d already assigned or sets not in ascending sequence\n"), n);
				errflg = 1;
			}
		    } else  {
			    cpt = setname;
			    do  {
				if ((!isalpha(*cpt)) && (!isdigit(*cpt)) && 
				    (*cpt != '_'))    {
					fprintf(stderr,MSGSTR(ZEROINV, "mkcatdefs: %s is an invalid  identifier\n"), setname);
					errflg = 1;
					break;
				}
			    }   while (*++cpt);
			incl ("#define %s %d\n\n", setname, setno);
		        symbflg = 1;
		    }
		    fprintf (msgfp,"$delset");
		    fprintf (msgfp," %d\n", setno);
		    fprintf (msgfp,"%.4s", line);
		    fprintf (msgfp," %d\n", setno++);
		    msgno = 1;
		    continue;
		} else {
		     /* !!!other command */
		}
	    } else
		if (contin) {
#ifdef KJI
		    if (!chkcontin(line))
#else
		    if (line[strlen(line) - 2] != '\\')
#endif
			contin = 0;
		} else if (setno > 1) { /* set must have been seen first */
    		    char msgname [MAXIDLEN];

		    msgname [0] = '\0';
		    if (sscanf (cp, "%s", msgname) && msgname [0] )
			if (isalpha (msgname[0])) {
			    cpt = msgname;
			    do  {
				if ((!isalpha(*cpt)) && (!isdigit(*cpt)) && 
				    (*cpt != '_'))    {
					fprintf(stderr,MSGSTR(ZEROINV, "mkcatdefs: %s is an invalid identifier\n"), msgname);	
					errflg = 1;
					break;
				}
			    }   while (*++cpt);
			    cp += strlen(msgname);
			    fprintf (msgfp,"%d %s", msgno,cp);
			    incl ("#define %s %d\n", msgname, msgno);
			    symbflg = 1;
#ifdef KJI
			    if (chkcontin(line))
#else
		    	    if (line[strlen(line) - 2] == '\\')
#endif
				contin = 1;
			    if(insert(msgname,msgno++) < 0) {
				fprintf(stderr,MSGSTR(MULTOPN, "mkcatdefs: name %s used more than once\n"), msgname); 
				errflg = 1;
			    }
			    continue;
			} else if (isdigit (msgname[0])){
			    cpt = msgname;
			    do  {
				if (!isdigit(*cpt)) {
					fprintf(stderr,MSGSTR(INVTAG, "mkcatdefs: invalid syntax in %s\n"), line);
					errflg = 1;
					break;
				}
			    }   while (*++cpt);
			    n = atoi (msgname);
			    if ((n >= msgno) || (n == 0 && msgno == 1))
				msgno = n + 1;
			    else {
				 errflg = 1;
				 if (n == 0)
				    fprintf(stderr,MSGSTR(ZEROINV, "mkcatdefs: %s is an invalid identifier\n"), msgno);
				 else if (n == msgno) 
					fprintf(stderr,MSGSTR(MULTNR, "mkcatdefs: message id %s already assigned to identifier\n"), msgname);		/*MSG*/
				      else
					fprintf(stderr,MSGSTR(NOTASC, "mkcatdefs: source messages not in ascending sequence\n"));			/*MSG*/
			    }
			}
#ifdef KJI
		    if (chkcontin(line))
#else
		    if (line[strlen(line) - 2] == '\\')
#endif
			contin = 1;
		}
	fputs (line, msgfp);
    }

    /* make sure the operations read/write operations were successful */
    if (descerrck() == -1) {
	fprintf (stderr, MSGSTR(READERRS, "mkcatdefs: There were read errors on file %s\n"), inname);							/*MSG*/
	errflg = 1;
    }
}

/*
 * NAME: incl
 *
 * FUNCTION: Output strings to file.
 *
 * EXECUTION ENVIRONMENT:
 *  	User mode.
 *
 * RETURNS: None
 */

incl(a, b, c) 
char *a; char *b; char *c;

	/*
	  a - pointer to "printf" format
	  b - pointer to optional "printf" arg
	  c - pointer to optional "printf" arg
	*/

{
	if (inclfile) fprintf (outfp, a, b, c);
}


#ifdef KJI


/*
 * NAME: chkcontin
 *
 * FUNCTION: Check for continuation line.
 *
 * EXECUTION ENVIRONMENT:
 *  	User mode.
 *
 * RETURNS: 0 - not a continuation line.
 *          1 - continuation line.
 */

chkcontin(line) 
char *line;

	/*
	  line - pointer to line to be checked
	*/

{
	register char *ptr, *eptr;

	ptr = line;
	eptr = &line[strlen(line) - 2];
	while (*ptr && ptr < eptr)
		ptr += NLchrlen(ptr);
	if (ptr == eptr && *ptr == '\\')
		return (1);
	return (0);
}

#endif

