#ifndef lint
static char sccsid[] = "@(#)tr.c	4.1	(ULTRIX)	7/17/90";
#endif

/*
 *  Modification History.
 *	April-12-1989, Pradeep Chetal (001)
 *	- Made changes for POSIX conformance.
 *	  Removed previous NUL restrictions. Now, tr(1) handles
 *	  NUL in string1, string2, and also input.
 */


/* static char *sccsid = "@(#)tr.c	4.1 (Berkeley) 10/1/80"; */
#include <stdio.h>
#include <values.h>

/* tr - transliterate data stream */
int	dflag = 0;
int	sflag = 0;
int	cflag = 0;
int	save  = -1; 
/*
 *  Comments for (001) change, Pradeep Chetal
 *	The following four arrays of shorts are used to store input-output
 *	mappings(1st two), deletions and squeezes. Previously since 
 *	NUL char was deleted from string1, string2, and stdin itself, these
 *	arrays never contained NUL character. Now they can. Also the next()
 *	function returned 0 when the string[12] was over but now 0 (NUL) can
 *	be a valid input char. So  next() now uses -1
 *	to show end of string. 
 */
short	code[256];
short	vect[256];
short	delete[256];
short	squeez[256];
struct string { int last, max; char *p; } string1, string2;

main(argc,argv)
char **argv;
{
	register i;
	int j;
	register c, d;
	short *compl;
	register complElements;	/* 001, # elements in compl array */
	int lastd;

	string1.last = string2.last = 0;
	string1.max = string2.max = 0;
	string1.p = string2.p = "";

	if(--argc>0) {
		argv++;
		if(*argv[0]=='-'&&argv[0][1]!=0) {
			while(*++argv[0])
				switch(*argv[0]) {
				case 'c':
					cflag++;
					continue;
				case 'd':
					dflag++;
					continue;
				case 's':
					sflag++;
					continue;
				}
			argc--;
			argv++;
		}
	}
	if(argc>0) string1.p = argv[0];
	if(argc>1) string2.p = argv[1];
	for(i=0; i<256; i++)
		code[i] = vect[i] = delete[i] = squeez[i] = MAXSHORT;
	if(cflag) {
		while((c = next(&string1)) != -1)
			vect[c&0377] = 1;
		j = complElements = 0;
		for(i=0; i<256; i++)
			if(vect[i]==MAXSHORT) {
			  	vect[j++] = i;
				complElements++;
			}
		compl = vect;
	}
	lastd = i = 0;
	for(;;){
		if(cflag)
		  	if (i < complElements)
			  	c = compl[i++];
			else
			  	c = -1;
		else 
		  	c = next(&string1);
		if(c==-1) break;	/* 001 */
		d = next(&string2);
		if(d==-1) d = lastd;	/* 001 */
		else lastd = d;
		squeez[d&0377] = 1;
		/*
		 * code[] contains input to output mapping.
		 * delete[] contains input to be deleted.
		 */
		code[c&0377] = d;
		if (dflag) delete[c&0377] = 1;
	}
	while((d = next(&string2)) != -1)	/* 001 */
		squeez[d&0377] = 1;
	for(i=0;i<256;i++) {
		if(code[i]==MAXSHORT) code[i] = i; /* 001, input = output if
						    not specified in string1 */
	}

	while((c=getc(stdin)) != EOF ) {
	  	if (delete[c&0377] != 1) {
			c = code[c&0377]&0377;
			if(!sflag || c!=save || (squeez[c&0377] != 1)) {
				putchar(save = c);
				if (ferror(stdout))
					exit(1);
			}
		}
	}
	exit(0);
}

/*
 * next() now returns -1 instead of NUL if the string
 * supplied to it finishes.
 */
next(s)
struct string *s;
{

again:
	if(s->max) {
		if(s->last++ < s->max)
			return(s->last);
		s->max = s->last = 0;
	}
	if(s->last && *s->p=='-') {
		nextc(s);
		s->max = nextc(s);
		if(s->max==-1) {	/* 001, '-' is last char in range */
		  	s->max = 0;	/* 001 */
			s->p--;
			return('-');
		}
		if(s->max < s->last)  {
			s->last = s->max-1;
			return('-');
		}
		goto again;
	}
	return(s->last = nextc(s));
}


/*
 * nextc() now returns -1 instead of NUL if the string
 * supplied to it finishes.
 */
nextc(s)
struct string *s;
{
	register c, i, n;
	
	/*
	 * Now 'i' is initialized outside because 'c' can be 
	 * NUL because of a NUL character in the input itself
	 * rather than referring to end of string input.
	 */
	i = 0;
	c = *s->p++;
	if(c=='\\') {
		n = 0;
		while(i<3 && (c = *s->p)>='0' && c<='7') {
			n = n*8 + c - '0';
			i++;
			s->p++;
		}
		if(i>0) c = n;
		else c = *s->p++;
	}
	if((c==0) && (i==0)) {
	  	*--s->p = 0;
		return(-1);
	}
	return(c&0377);
}
