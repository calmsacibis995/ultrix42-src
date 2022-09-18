/*	@(#)xlocal.c	1.3	*/
#include <stdio.h>
#include "mfile1"
#include "lmanifest"
/*
 * this file contains the functions local to CXREF
 * they put their output to outfp
 * the others contain lint's 1st pass with output thrown away
 * cgram.c has calls to these functions whenever a NAME is seen
 */

char infile[120];
FILE *outfp;

ref( i, line)
	int i, line;
{
#ifdef	FLEXNAMES

	fprintf(outfp, "R%s\t%05d\n",stab[i].sname,line);
#else
	fprintf(outfp, "R%.8s\t%05d\n",stab[i].sname,line);
#endif
}

def( i, line)
	int i, line;
{
	if (stab[i].sclass == EXTERN)
		ref(i, line);
	else
#ifdef	FLEXNAMES
	fprintf(outfp,"D%s\t%05d\n",stab[i].sname,line);
#else
	fprintf(outfp,"D%.8s\t%05d\n",stab[i].sname,line);
#endif
}


newf(i, line)
	int i, line;
{
#ifdef	FLEXNAMES
	fprintf(outfp,"F%s\t%05d\n",stab[i].sname, line);
#else
	fprintf(outfp,"F%.8s\t%05d\n",stab[i].sname, line);
#endif
}

/* Routine exname CXREF */
char *
exname( p ) char *p; {			
	/* make a name look like an external name in the local machine */

#ifndef FLEXNAMES
	static char text[NCHNAM+1];
#else
	static char text[BUFSIZ+1];
#endif

	register i;

	text[0] = '_';
#ifndef FLEXNAMES
	for( i=1; *p&&i<NCHNAM; ++i ){
#else
	for( i=1; *p; ++i ){
#endif
		text[i] = *p++;
		}

	text[i] = '\0';
#ifndef FLEXNAMES
	text[NCHNAM] = '\0';  /* truncate */
#endif

	return( text );
	}

/* Routine added CXREF */
NODE * addroreg(l)		
				/* OREG was built in clocal()
				 * for an auto or formal parameter
				 * now its address is being taken
				 * local code must unwind it
				 * back to PLUS/MINUS REG ICON
				 * according to local conventions
				 */
{
	cerror("address of OREG taken");
}
