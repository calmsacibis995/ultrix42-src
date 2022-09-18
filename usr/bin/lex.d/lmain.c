#ifndef lint
static char sccsid[] = "@(#)lmain.c	4.1	(Ultrix)	7/17/90";
#endif

# include "ldefs.c"
# include "once.c"
/*
 *	slr001 
 *	These two include files were added
 */
# include "ncform.h"
# include "nrform.h"

	/* lex [-[drcyvntf]] [file] ... [file] */

/************************************************************************
 *									*
 *			Copyright (c) 1983 by				*
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

	/* Copyright 1976, Bell Telephone Laboratories, Inc.,
	    written by Eric Schmidt, August 27, 1976   */
/************************************************************************
 *
 *			Modification History
 *
 *	Stephen Reilly, 07-Nov-83
 * 001-	The file ncform and nrform are no longer read in as file.  These
 *	template files are now contained in a date structure. This reason
 *	for this change is to satisify the Binary kit restrictions.
 * 002- Rodriguez, Aug-88
 *	Added numerous casts.
 * 003-	Jon Reeves/Dave Long, 20-Nov-88
 *	Removed unnecessary argument from yyparse call to aid portability.
 * 004 - Wendy Rannenberg 04-May-1989
 *       8 bit cleaned. Interface to yacc remained unchanged.
 *	 Problem with user specified output table size specification
 *	 required introducing variable ZCHHALF. All uses of 
 *	 unsigned chars introduced by Andy Gadsby (IED) were left in
 *	 with the exception of yy* references. CTYPE routines isalpha,
 *	 isprint, isdigit are now used both here and in the lex
 *	 library.
 *
 ***********************************************************************/

unsigned char *myalloc();

main(argc,argv)
  int argc;
  char **argv; {
	register int i;
	unsigned char **t, *p;
# ifdef DEBUG
#include <signal.h>
	signal(SIGBUS,buserr);
	signal(SIGSEGV,segviol);
# endif
	while (argc > 1 && argv[1][0] == '-' ){
		i = 0;
		while(argv[1][++i]){
			switch (argv[1][i]){
# ifdef DEBUG
				case 'd': debug++; break;
				case 'y': yydebug = TRUE; break;
# endif
				case 'r': case 'R':
					ratfor=TRUE; break;
				case 'c': case 'C':
					ratfor=FALSE; break;
				case 't': case 'T':
					fout = stdout;
					errorf = stderr;
					break;
				case 'v': case 'V':
					report = 1;
					break;
				case 'f': case 'F':
					optim = FALSE;
					break;
				case 'n': case 'N':
					report = 0;
					break;
				default:
					usage();
					exit(1);	
				}
			}
		argc--;
		argv++;
		}
	sargc = argc;
	sargv = argv;
	if (argc > 1){
		fin = fopen(argv[++fptr], "r");		/* open argv[1] */
		sargc--;
		sargv++;
		}
	else fin = stdin;
	if(fin == NULL)
		error ("Can't read input file %s",argc>1?argv[1]:"standard input");
	gch();
		/* may be gotten: def, subs, sname, schar, ccl, dchar */
	get1core();
		/* may be gotten: name, left, right, nullstr, parent */
	scopy("INITIAL",sp);
	sname[0] = sp;
	sp += slength("INITIAL") + 1;
	sname[1] = 0;
	if(yyparse()) exit(1);	/* error return code */
		/* may be disposed of: def, subs, dchar */
	free1core();
		/* may be gotten: tmpstat, foll, positions, gotof, nexts, nchar, state, atable, sfall, cpackflg */
	get2core();
	ptail();
	mkmatch();
# ifdef DEBUG
	if(debug) pccl();
# endif
	sect  = ENDSECTION;
	if(tptr>0)cfoll(tptr-1);
# ifdef DEBUG
	if(debug)pfoll();
# endif
	cgoto();
# ifdef DEBUG
	if(debug){
		printf("Print %d states:\n",stnum+1);
		for(i=0;i<=stnum;i++)stprt(i);
		}
# endif
		/* may be disposed of: positions, tmpstat, foll, state, name, left, right, parent, ccl, schar, sname */
		/* may be gotten: verify, advance, stoff */
	free2core();
	get3core();
	layout();
		/* may be disposed of: verify, advance, stoff, nexts, nchar,
			gotof, atable, ccpackflg, sfall */
# ifdef DEBUG
	free3core();
# endif

/*
 *				Slr001
 *
 *		No longer used because of Binary kit restrictions
 *
 *	if (ZCH>NCH) cname="/usr/lib/lex/ebcform";
 *
 *	fother = fopen(ratfor?ratname:cname,"r");
 *	if(fother == NULL)
 *		error("Lex driver missing, file %s",ratfor?ratname:cname);
 *	while ( (i=getc(fother)) != EOF)
 *	 	putc(i,fout);
 */

/*
 *			SLR001
 *
 *	Based on the switch we will read the appropriate date structure
 *	that contains the lexical anal. template
 *
 *      unsigned char ** typecast added for clean compile  (wr 6/89)
 */
	for(t = ratfor ? (unsigned char**)nrformlin : (unsigned char**)ncformlin; *t; *t++ )
 	    {
	    p = *t;
	    while ( *p != '\0' )
		putc( *p++,fout );
	    };


/*	fclose(fother);
 */
	fclose(fout);
	if(
# ifdef DEBUG
		debug   ||
# endif
			report == 1)statistics();
	fclose(stdout);
	fclose(stderr);
	exit(0);	/* success return code */
	}
get1core(){
	register int i, val;
	register char *p;
ccptr =	ccl = myalloc(CCLSIZE,sizeof(*ccl));
pcptr = pchar = myalloc(pchlen, sizeof(*pchar));
	def = (unsigned char **) myalloc(DEFSIZE,sizeof(*def));
	subs = (unsigned char **) myalloc(DEFSIZE,sizeof(*subs));
dp =	dchar = myalloc(DEFCHAR,sizeof(*dchar));
	sname = (unsigned char **) myalloc(STARTSIZE,sizeof(*sname));
sp = 	schar = myalloc(STARTCHAR,sizeof(*schar));
	if(ccl == 0 || def == 0 || subs == 0 || dchar == 0 || sname == 0 || schar == 0)
		error("Too little core to begin");
	}
free1core(){
	cfree(def,DEFSIZE,sizeof(*def));
	cfree(subs,DEFSIZE,sizeof(*subs));
	cfree(dchar,DEFCHAR,sizeof(*dchar));
	}
get2core(){
	register int i, val;
	register char *p;
	gotof = (int *) myalloc(nstates,sizeof(*gotof));
	nexts = (int *) myalloc(ntrans,sizeof(*nexts));
	nchar = myalloc(ntrans,sizeof(*nchar));
	state = (int **) myalloc(nstates,sizeof(*state));
	atable = (int *) myalloc(nstates,sizeof(*atable));
	sfall = (int *) myalloc(nstates,sizeof(*sfall));
	cpackflg = myalloc(nstates,sizeof(*cpackflg));
	tmpstat = myalloc(tptr+1,sizeof(*tmpstat));
	foll = (int **)myalloc(tptr+1,sizeof(*foll));
nxtpos = positions = (int *)myalloc(maxpos,sizeof(*positions));
	if(tmpstat == 0 || foll == 0 || positions == 0 ||
		gotof == 0 || nexts == 0 || nchar == 0 || state == 0 || atable == 0 || sfall == 0 || cpackflg == 0 )
		error("Too little core for state generation");
	for(i=0;i<=tptr;i++)foll[i] = 0;
	}
free2core(){
	cfree(positions,maxpos,sizeof(*positions));
	cfree(tmpstat,tptr+1,sizeof(*tmpstat));
	cfree(foll,tptr+1,sizeof(*foll));
	cfree(name,treesize,sizeof(*name));
	cfree(left,treesize,sizeof(*left));
	cfree(right,treesize,sizeof(*right));
	cfree(parent,treesize,sizeof(*parent));
	cfree(nullstr,treesize,sizeof(*nullstr));
	cfree(state,nstates,sizeof(*state));
	cfree(sname,STARTSIZE,sizeof(*sname));
	cfree(schar,STARTCHAR,sizeof(*schar));
	cfree(ccl,CCLSIZE,sizeof(*ccl));
	}
get3core(){
	register int i, val;
	register char *p;
	verify = (int *)myalloc(outsize,sizeof(unsigned char *));
	advance = (int *)myalloc(outsize,sizeof(*advance));
	stoff = (int *)myalloc(stnum+2,sizeof(*stoff));
	if(verify == 0 || advance == 0 || stoff == 0)
		error("Too little core for final packing");
	}
# ifdef DEBUG
free3core(){
	cfree(advance,outsize,sizeof(*advance));
	cfree(verify,outsize,sizeof(*verify));
	cfree(stoff,stnum+1,sizeof(*stoff));
	cfree(gotof,nstates,sizeof(*gotof));
	cfree(nexts,ntrans,sizeof(*nexts));
	cfree(nchar,ntrans,sizeof(*nchar));
	cfree(atable,nstates,sizeof(*atable));
	cfree(sfall,nstates,sizeof(*sfall));
	cfree(cpackflg,nstates,sizeof(*cpackflg));
	}
# endif
unsigned char *
myalloc(a,b)
  int a,b; {
	register int i;
	i = (int)calloc(a, b);
	if(i==0)
		warning("OOPS - calloc returns a 0");
	else if(i == -1){
# ifdef DEBUG
		warning("calloc returns a -1");
# endif
		return(0);
		}
	return((unsigned char *)i);
	}
# ifdef DEBUG
buserr(){
	fflush(errorf);
	fflush(fout);
	fflush(stdout);
	fprintf(errorf,"Bus error\n");
	if(report == 1)statistics();
	fflush(errorf);
	}
segviol(){
	fflush(errorf);
	fflush(fout);
	fflush(stdout);
	fprintf(errorf,"Segmentation violation\n");
	if(report == 1)statistics();
	fflush(errorf);
	}
# endif

yyerror(s)
unsigned char *s;
{
	fprintf(stderr, "%s\n", s);
}

usage()
{
	fprintf(stderr, "usage: lex [-tvfn] [file ...] \n");
}
