
#ifndef lint
static	char	*sccsid = "@(#)join.c	4.1	(ULTRIX)	7/17/90";
#endif lint

/*	join F1 F2 on stuff */
/*
 *			Copyright (c) 1985 by
 *		Digital Equipment Corporation, Maynard, MA
 *			All rights reserved.
 *								
 *	This software is furnished under a license and may be used and
 *	copied  only  in accordance with the terms of such license and
 *	with the  inclusion  of  the  above  copyright  notice.   This
 *	software  or  any  other copies thereof may not be provided or
 *	otherwise made available to any other person.  No title to and
 *	ownership of the software is hereby transferred.		
 *								
 *	This software is  derived  from  software  received  from  the
 *	University    of   California,   Berkeley,   and   from   Bell
 *	Laboratories.  Use, duplication, or disclosure is  subject  to
 *	restrictions  under  license  agreements  with  University  of
 *	California and with AT&T.					
 *								
 *	The information in this software is subject to change  without
 *	notice  and should not be construed as a commitment by Digital
 *	Equipment Corporation.					
 *								
 *	Digital assumes no responsibility for the use  or  reliability
 *	of its software on equipment which is not supplied by Digital.
*/

#include	<stdio.h>
#include	<locale.h>		/* internationalization */
#define F1 0
#define F2 1
#define	NFLD	50	/* max field per line */
#define comp() cmp(ppi[F1][j1],ppi[F2][j2])

FILE *f[2];
char buf[2][BUFSIZ];	/*input lines */
char *ppi[2][NFLD];	/* pointers to fields in lines */
char *s1,*s2;
int	j1	= 1;	/* join of this field of file 1 */
int	j2	= 1;	/* join of this field of file 2 */
int	olist[2*NFLD];	/* output these fields */
int	olistf[2*NFLD];	/* from these files */
int	no;	/* number of entries in olist */
int	sep1	= ' ';	/* default field separator */
int	sep2	= '\t';
char*	null	= "";
int	unpub1;
int	unpub2;
int	aflg;

main(argc, argv)
char *argv[];
{
	int i;
	int n1, n2;
	long top2, bot2;
	long ftell();

	while (argc > 1 && argv[1][0] == '-') {
		if (argv[1][1] == '\0')
			break;
		switch (argv[1][1]) {
		case 'a':
			switch(argv[1][2]) {
			case '1':
				aflg |= 1;
				break;
			case '2':
				aflg |= 2;
				break;
			default:
				aflg |= 3;
			}
			break;
		case 'e':
			null = argv[2];
			argv++;
			argc--;
			break;
		case 't':
			sep1 = sep2 = argv[1][2];
			break;
		case 'o':
			for (no = 0; no < 2*NFLD; no++) {
				if (argv[2][0] == '1' && argv[2][1] == '.') {
					olistf[no] = F1;
					olist[no] = atoi(&argv[2][2]);
				} else if (argv[2][0] == '2' && argv[2][1] == '.') {
					olist[no] = atoi(&argv[2][2]);
					olistf[no] = F2;
				} else
					break;
				argc--;
				argv++;
			}
			break;
		case 'j':
			if (argv[1][2] == '1')
				j1 = atoi(argv[2]);
			else if (argv[1][2] == '2')
				j2 = atoi(argv[2]);
			else
				j1 = j2 = atoi(argv[2]);
			argc--;
			argv++;
			break;
		case '1':		/* -1 x == same as -j1 x */
			j1 = atoi(argv[2]);
			argc--;
			argv++;
			break;
		case '2':		/* -1 x == same as -j2 x */
			j2 = atoi(argv[2]);
			argc--;
			argv++;
			break;
		}
		argc--;
		argv++;
	}
	for (i = 0; i < no; i++)
		olist[i]--;	/* 0 origin */
	if (argc != 3)
error("usage: join [-an] [-e string] [-jn m] [-1 m] [-2 m] [-o list] [-tc] \n             file1 file2");

	    /* Internationalization:
		  setlocale(LC_foo, "") will set foo locale to value of
		  environment var LC_foo, if valid locale, else to value of
		  environment var LANG, if valid locale.  */
	(void) setlocale(LC_CTYPE, "");
	(void) setlocale(LC_COLLATE, "");

	j1--;
	j2--;	/* everyone else believes in 0 origin */
	s1 = ppi[F1][j1];
	s2 = ppi[F2][j2];
	if (argv[1][0] == '-')
		f[F1] = stdin;
	else if ((f[F1] = fopen(argv[1], "r")) == NULL)
		error("can't open %s", argv[1]);
	if ((f[F2] = fopen(argv[2], "r")) == NULL)
		error("can't open %s", argv[2]);

#define get1() n1=input(F1)
#define get2() n2=input(F2)
	get1();
	bot2 = ftell(f[F2]);
	get2();
	while(n1>0 && n2>0 || aflg!=0 && n1+n2>0) {
		if(n1>0 && n2>0 && comp()>0 || n1==0) {
			if(aflg&2) output(0, n2);
			bot2 = ftell(f[F2]);
			get2();
		} else if(n1>0 && n2>0 && comp()<0 || n2==0) {
			if(aflg&1) output(n1, 0);
			get1();
		} else /*(n1>0 && n2>0 && comp()==0)*/ {
			while(n2>0 && comp()==0) {
				output(n1, n2);
				top2 = ftell(f[F2]);
				get2();
			}
			fseek(f[F2], bot2, 0);
			get2();
			get1();
			for(;;) {
				if(n1>0 && n2>0 && comp()==0) {
					output(n1, n2);
					get2();
				} else if(n1>0 && n2>0 && comp()<0 || n2==0) {
					fseek(f[F2], bot2, 0);
					get2();
					get1();
				} else /*(n1>0 && n2>0 && comp()>0 || n1==0)*/{
					fseek(f[F2], top2, 0);
					bot2 = top2;
					get2();
					break;
				}
			}
		}
	}
	return(0);
}

input(n)		/* get input line and split into fields */
{
	register int i, c;
	char *bp;
	char **pp;

	bp = buf[n];
	pp = ppi[n];
	if (fgets(bp, BUFSIZ, f[n]) == NULL)
		return(0);
	for (i = 0; ; i++) {
		if (sep1 == ' ')	/* strip multiples */
			while ((c = *bp) == sep1 || c == sep2)
				bp++;	/* skip blanks */
		else
			c = *bp;
		if (c == '\n' || c == '\0')
			break;
		*pp++ = bp;	/* record beginning */
		while ((c = *bp) != sep1 && c != '\n' && c != sep2 && c != '\0')
			bp++;
		*bp++ = '\0';	/* mark end by overwriting blank */
			/* fails badly if string doesn't have \n at end */
	}
	*pp = 0;
	return(i);
}

output(on1, on2)	/* print items from olist */
int on1, on2;
{
	int i;
	char *temp;

	if (no <= 0) {	/* default case */
		printf("%s", on1? ppi[F1][j1]: ppi[F2][j2]);
		for (i = 0; i < on1; i++)
			if (i != j1)
				printf("%c%s", sep1, ppi[F1][i]);
		for (i = 0; i < on2; i++)
			if (i != j2)
				printf("%c%s", sep1, ppi[F2][i]);
		printf("\n");
	} else {
		for (i = 0; i < no; i++) {
			temp = ppi[olistf[i]][olist[i]];
			if(olistf[i]==F1 && on1<=olist[i] ||
			   olistf[i]==F2 && on2<=olist[i] ||
			   *temp==0)
				temp = null;
			printf("%s", temp);
			if (i == no - 1)
				printf("\n");
			else
				printf("%c", sep1);
		}
	}
}

error(s1, s2, s3, s4, s5)
char *s1;
{
	fprintf(stderr, "join: ");
	fprintf(stderr, s1, s2, s3, s4, s5);
	fprintf(stderr, "\n");
	exit(1);
}

cmp(s1, s2)
char *s1, *s2;
{
	return(strcmp(s1, s2));
}
