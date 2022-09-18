#ifndef lint
static char *sccsid = "@(#)n10.c	4.1	(ULTRIX)	7/17/90";
#endif lint

/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */
/* $Header: n10.c,v 1.6 86/11/12 19:09:56 dce Exp $ */

#include "tdef.h"
#include <sgtty.h>
extern
#include "d.h"
extern
#include "v.h"
extern
#include "tw.h"
/*
nroff10.c

Device interfaces
*/

extern int lss;
extern char obuf[];
extern char *obufp;
extern int xfont;
extern int esc;
extern int lead;
extern int oline[];
extern int *olinep;
extern int ulfont;
extern int esct;
extern int sps;
extern int ics;
extern int ttysave;
extern struct sgttyb ttys;
extern char termtab[];
extern int ptid;
extern int waitf;
extern int pipeflg;
extern int eqflg;
extern int hflg;
extern int tabtab[];
extern int ascii;
extern int xxx;
int dtab;
int bdmode;
int plotmode;

static void tab_parse();

ptinit(){
	register int i;

	if(((i=open(termtab,0)) < 0) && (i=open("/usr/lib/term/tablpr",0)) < 0){
		prstr("Cannot open ");
		prstr(termtab);
		prstr("\n");
		exit(-1);
	}

	/*
	 * Read in ASCII format terminal table
	 */

	tab_parse(i);
	(void)close(i);

	sps = EM;
	ics = EM*2;
	dtab = 8 * t.Em;
	for(i=0; i<16; i++)tabtab[i] = dtab * (i+1);
	if(eqflg)t.Adj = t.Hor;

}
twdone(){
	obufp = obuf;
	oputs(t.twrest);
	flusho();
	if(pipeflg){
		close(ptid);
		wait(&waitf);
	}
	if(ttysave != -1) {
		ttys.sg_flags = ttysave;
		stty(1, &ttys);
	}
}
ptout(i)
int i;
{
	*olinep++ = i;
	if(olinep >= &oline[LNSIZE])olinep--;
	if((i&CMASK) != '\n')return;
	olinep--;
	lead += dip->blss + lss - t.Newline;
	dip->blss = 0;
	esct = esc = 0;
	if(olinep>oline){
		move();
		ptout1();
		oputs(t.twnl);
	}else{
		lead += t.Newline;
		move();
	}
	lead += dip->alss;
	dip->alss = 0;
	olinep = oline;
}
ptout1()
{
	register i, k;
	register char *codep;
	extern char *plot();
	int *q, w, j, phyw;

	for(q=oline; q<olinep; q++){
	if((i = *q) & MOT){
		j = i & ~MOTV;
		if(i & NMOT)j = -j;
		if(i & VMOT)lead += j;
		else esc += j;
		continue;
	}
	if((k = (i & CMASK)) <= 040){
		switch(k){
			case ' ': /*space*/
				esc += t.Char;
				break;
		}
		continue;
	}
	codep = t.codetab[k-32];
	w = t.Char * (*codep++ & 0177);
	phyw = w;
	if(i&ZBIT)w = 0;
	if(*codep && (esc || lead))move();
	esct += w;
	if(i&074000)xfont = (i>>9) & 03;
	if(*t.bdon & 0377){
		if(!bdmode && (xfont == 2)){
			oputs(t.bdon);
			bdmode++;
		}
		if(bdmode && (xfont != 2)){
			oputs(t.bdoff);
			bdmode = 0;
		}
	}
	if(xfont == ulfont){
		for(k=w/t.Char;k>0;k--)oput('_');
		for(k=w/t.Char;k>0;k--)oput('\b');
	}
	while(*codep != 0){
		if(*codep & 0200){
			codep = plot(codep);
			oputs(t.plotoff);
			oput(' ');
		}else{
			if(plotmode)oputs(t.plotoff);
			*obufp++ = *codep++;
			if(obufp == (obuf + OBUFSZ + ascii - 1))flusho();
/*			oput(*codep++);*/
		}
	}
	if(!w)for(k=phyw/t.Char;k>0;k--)oput('\b');
	}
}
char *plot(x)
char *x;
{
	register int i;
	register char *j, *k;

	if(!plotmode)oputs(t.ploton);
	k = x;
	if((*k & 0377) == 0200)k++;
	for(; *k; k++){
		if(*k & 0200){
			if(*k & 0100){
				if(*k & 040)j = t.up; else j = t.down;
			}else{
				if(*k & 040)j = t.left; else j = t.right;
			}
			if(!(i = *k & 037))return(++k);
			while(i--)oputs(j);
		}else oput(*k);
	}
	return(k);
}
move(){
	register k;
	register char *i, *j;
	char *p, *q;
	int iesct, dt;

	iesct = esct;
	if(esct += esc)i = "\0"; else i = "\n\0";
	j = t.hlf;
	p = t.right;
	q = t.down;
	if(lead){
		if(lead < 0){
			lead = -lead;
			i = t.flr;
		/*	if(!esct)i = t.flr; else i = "\0";*/
			j = t.hlr;
			q = t.up;
		}
		if(*i & 0377){
			k = lead/t.Newline;
			lead = lead%t.Newline;
			while(k--)oputs(i);
		}
		if(*j & 0377){
			k = lead/t.Halfline;
			lead = lead%t.Halfline;
			while(k--)oputs(j);
		}
		else { /* no half-line forward, not at line begining */
			k = lead/t.Newline;
			lead = lead%t.Newline;
			if (k>0) esc=esct;
			i = "\n";
			while (k--) oputs(i);
		}
	}
	if(esc){
		if(esc < 0){
			esc = -esc;
			j = "\b";
			p = t.left;
		}else{
			j = " ";
			if(hflg)while((dt = dtab - (iesct%dtab)) <= esc){
				if(dt%t.Em || dt==t.Em)break;
				oput(TAB);
				esc -= dt;
				iesct += dt;
			}
		}
		k = esc/t.Em;
		esc = esc%t.Em;
		while(k--)oputs(j);
	}
	if((*t.ploton & 0377) && (esc || lead)){
		if(!plotmode)oputs(t.ploton);
		esc /= t.Hor;
		lead /= t.Vert;
		while(esc--)oputs(p);
		while(lead--)oputs(q);
		oputs(t.plotoff);
	}
	esc = lead = 0;
}
ptlead(){move();}
dostop(){
	char junk;

	flusho();
	read(2,&junk,1);
}


/*
 * Table parser. This command reads from the given file descriptor, and
 * parses the data into the terminal driver structure t.
 */

static int tabgetnum();
static char *tabgetstr();
static int get_character();

static int Eof_seen;			/* Set upon EOF			*/
static char String_space[8192];		/* Space for strings		*/
static char *String_place;		/* Current string place		*/
static char Empty_string[3];		/* 3 nulls			*/

static void
tab_parse(fd)
int fd;
{

	int tab_no;		/* Code table index			*/

	/*
	 * The following values are initialized for reading the table.
	 * Eof_seen is 1 when EOF is reached, and all subsequent integers
	 * are 0s, and strings will all be set to point to Empty_string, which
	 * is a string of 3 nulls (control, character, null terminator).
	 */

	Eof_seen = 0;
	String_place = &String_space[0];

	/*
	 * Read in the numbers.
	 */

	t.bset = tabgetnum(fd);
	t.breset = tabgetnum(fd);
	t.Hor = tabgetnum(fd);
	t.Vert = tabgetnum(fd);
	t.Newline = tabgetnum(fd);
	t.Char = tabgetnum(fd);
	t.Em = tabgetnum(fd);
	t.Halfline = tabgetnum(fd);
	t.Adj = tabgetnum(fd);

	/*
	 * Now, the known strings (everything but codetab).
	 */

	t.twinit = tabgetstr(fd);
	t.twrest = tabgetstr(fd);
	t.twnl = tabgetstr(fd);
	t.hlr = tabgetstr(fd);
	t.hlf = tabgetstr(fd);
	t.flr = tabgetstr(fd);
	t.bdon = tabgetstr(fd);
	t.bdoff = tabgetstr(fd);
	t.ploton = tabgetstr(fd);
	t.plotoff = tabgetstr(fd);
	t.up = tabgetstr(fd);
	t.down = tabgetstr(fd);
	t.right = tabgetstr(fd);
	t.left = tabgetstr(fd);

	/*
	 * Now, read each code table element.
	 */

	tab_no = 0;
	while (tab_no < CTABSIZE) {
		t.codetab[tab_no] = tabgetstr(fd);
		tab_no++;
	}
}

/*
 * The subroutine tabgetnum() reads a line from the given file descriptor,
 * and converts it to a number. The number is returned. An invalid number
 * does not cause an error message.
 */

static int
tabgetnum(fd)
int fd;
{
	
	int accum;	/* Accumulator					*/
	int c;		/* Input character				*/

	if (Eof_seen) {
		return 0;
	}

	/*
	 * Read past any comments.
	 */

	if ((c = get_character(fd)) == 0) {
		Eof_seen = 1;
		return 0;
	}
	while (c == '#') {
		while (c != '\n') {
			if ((c = get_character(fd)) == 0) {
				Eof_seen = 1;
				return 0;
			}
		}
		if ((c = get_character(fd)) == 0) {
			Eof_seen = 1;
			return 0;
		}
	}

	/*
	 * Get each digit.
	 */

	accum = 0;
	while (c != '\n') {
		accum *= 10;
		accum += c - '0';
		if ((c = get_character(fd)) == 0) {
			Eof_seen = 1;
			return accum;
		}
	}
	return accum;
}

/*
 * The subroutine tabgetstr() reads a line from the given file descriptor,
 * converts all escaped octal characters (\xxx) to corresponding characters,
 * converts \n, \b, \t, \r, and \f to the appropriate characters,
 * and returns a pointer to the null-terminated string.
 *
 * Note that \xxx is interpreted as "at most 3 digits", so \01x is interpreted
 * as \01 and x, but \011 is interpreted as \011.
 */

static char *
tabgetstr(fd)
int fd;
{

	int accum;	/* Value accumulator			*/
	int c;		/* Input character			*/
	char *retval;	/* Pointer to current string space	*/

	if (Eof_seen) {
		return Empty_string;
	}

	String_place++;
	retval = String_place;

	/*
	 * Read past any comments.
	 */

	if ((c = get_character(fd)) == 0) {
		Eof_seen = 1;
		return 0;
	}
	while (c == '#') {
		while (c != '\n') {
			if ((c = get_character(fd)) == 0) {
				Eof_seen = 1;
				return 0;
			}
		}
		if ((c = get_character(fd)) == 0) {
			Eof_seen = 1;
			return 0;
		}
	}

	/*
	 * Read characters up to a newline. Empty string isn empty line is a
	 * special case.
	 */

	if (c == '\n') {
		return Empty_string;
	}

	while (c != '\n') {
		if (c == '\\') {
			accum = 0;
			if ((c = get_character(fd)) == 0) {
				Eof_seen = 1;
				*String_place = (accum & 0xff);
				String_place++;
				*String_place = '\0';
				return retval;
			}
			if (c < '0' || c > '7') {
				switch (c) {

				case 'n':
					c = '\n';
					break;

				case 'r':
					c = '\r';
					break;

				case 'b':
					c = '\b';
					break;

				case 't':
					c = '\t';
					break;

				case 'f':
					c = '\f';
					break;
				}
				*String_place = c;
				String_place++;
				if ((c = get_character(fd)) == 0) {
					Eof_seen = 1;
					*String_place = '\0';
					return retval;
				}
				continue;
			}

			accum *= 8;
			accum += c - '0';

			/*
			 * Second digit
			 */

			if ((c = get_character(fd)) == 0) {
				Eof_seen = 1;
				*String_place = (accum & 0xff);
				String_place++;
				*String_place = '\0';
				return retval;
			}

			if (c < '0' || c > '7') {
				*String_place = (accum & 0xff);
				String_place++;
				continue;
			}

			accum *= 8;
			accum += c - '0';

			/*
			 * Third digit
			 */

			if ((c = get_character(fd)) == 0) {
				Eof_seen = 1;
				*String_place = (accum & 0xff);
				String_place++;
				*String_place = '\0';
				return retval;
			}

			if (c < '0' || c > '7') {
				*String_place = (accum & 0xff);
				String_place++;
				continue;
			}

			accum *= 8;
			accum += c - '0';
			*String_place = (accum & 0xff);
			String_place++;
		} else {	/* c != '\'	*/
			*String_place = c;
			String_place++;
		}

		if ((c = get_character(fd)) == 0) {
			Eof_seen = 1;
			*String_place = '\0';
			return retval;
		}
	}

	*String_place = '\0';
	return retval;
}

/*
 * The subrutine get_character() reads a character from the given file
 * descriptor and returns the character or a '\0' (for EOF). If the character
 * is null or non-ASCII, an error message is printed and nroff exits.
 */

static int
get_character(fd)
int fd;
{

	char c;		/* Input character			*/

	if (read(fd, &c, sizeof(char)) != sizeof(char)) {
		return '\0';
	}

	if (c == '\0' || (c &0x80)) {
		prstr("Null or non-ASCII character in terminal table file ");
		prstr(termtab);
		prstr("\n");
		exit(1);
	}

	return (int) (c & 0xff);
}
