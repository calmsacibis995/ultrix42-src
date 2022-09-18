

#ifndef lint
static char *sccsid = "@(#)dohist.c	4.1	ULTRIX	7/17/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1984,1987 by			*
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
 *
 *   Modification history:
 *
 * 29Nov84 -- depp
 *	Added to method of terminating comments a '.' on the 1st column.
 *
 * 12-Nov-87 -- Tim Burke
 *	Look for the correct end-of-file character instead of assuming that
 *	it will be ^D.
 *
 */

# include	"../hdr/defines.h"
# include	"../hdr/had.h"
# include 	<sgtty.h>


extern char *Mrs;
extern int Domrs;

char	Cstr[RESPSIZE];
char	Mstr[RESPSIZE];
char	*savecmt();	/* function returning character ptr */

dohist(file)
char *file;
{
	char line[BUFSIZ];
	struct sgttyb tty[3];
	struct tchars tchar;	/* Terminal's special characters */
	int doprmt;
	register char *p;
	char eofstr[3];
	FILE *in, *fdfopen();
	extern char *Comments, *getresp();

	in = xfopen(file,0);
	while ((p = fgets(line,sizeof(line),in)) != NULL)
		if (line[0] == CTLCHAR && line[1] == EUSERNAM)
			break;
	if (p != NULL) {
		while ((p = fgets(line,sizeof(line),in)) != NULL)
			if (line[3] == VALFLAG && line[1] == FLAG && line[0] == CTLCHAR)
				break;
			else if (line[1] == BUSERTXT && line[0] == CTLCHAR)
				break;
		if (p != NULL && line[1] == FLAG) {
			Domrs++;
		}
	}
	fclose(in);
	doprmt = 0;
	if (gtty(0,tty) >= 0) {
		doprmt++;
		if (ioctl(0, TIOCGETC, &tchar) < 0) 
			tchar.t_eofc = 04;	/* Control-D */
		eofstr[0] = ' ';
		if (tchar.t_eofc <= 037) {
			eofstr[0] = '^';
			eofstr[1] = tchar.t_eofc + 'A' - 1;
		}
		else
			eofstr[1] = tchar.t_eofc;
		eofstr[2] = '\0';
	}
	if (Domrs && !Mrs) {
		if (doprmt)
			printf("MRs? (%s to end)\n",eofstr);
		Mrs = getresp(" ",Mstr);
	}
	if (Domrs)
		mrfixup();
	if (!Comments) {
		if (doprmt)
			printf("Comments? (%s to end)\n",eofstr);
		sprintf(line,"\n");
		Comments = getresp(line,Cstr);
	}
}

char *
getresp(repstr,result)
char *repstr;
char *result;
{
	char line[BUFSIZ];
	register int done, sz;
	register char *p;
	extern char	had_standinp;
	extern char	had[26];
	char *editresult();

	result[0] = 0;
	done = 0;
	/*
	save old fatal flag values and change to
	values inside ()
	*/
	FSAVE(FTLEXIT | FTLMSG | FTLCLN);
	if ((had_standinp && (!HADY || (Domrs && !HADM)))) {
		Ffile = 0;
		fatal("standard input specified w/o -y and/or -m keyletter (de16)");
	}
	/*
	restore the old flag values and process if above
	conditions were not met
	*/
	FRSTR();
	sz = sizeof(line) - size(repstr);
	while (fgets(line,sz,stdin) != NULL && line[0] != '.') 
		if( line[0] == '~' && line[1] == 'e' ) {
			editresult( result );
			printf("Continue:\n");
		} else {	
			p = strend(line);
			if (*--p == '\n') 
				copy(repstr,p);
			else
				fatal("line too long (co18)");

			if ((size(line) + size(result)) > RESPSIZE)
				fatal("response too long (co19)");
			strcat(result,line);
		}

	putchar('\n');
	return(result);
}


char	*Qarg[NVARGS];
char	**Varg = Qarg;

valmrs(pkt,pgm)
struct packet *pkt;
char *pgm;
{
	extern char *Sflags[];
	register int i;
	int st;
	register char *p;
	char *auxf();

	Varg[0] = pgm;
	Varg[1] = auxf(pkt->p_file,'g');
	if (p = Sflags[TYPEFLAG - 'a'])
		Varg[2] = p;
	else
		Varg[2] = Null;
	if ((i = fork()) < 0) {
		fatal("cannot fork; try again (co20)");
	}
	else if (i == 0) {
		for (i = 4; i < 15; i++)
			close(i);
		execvp(pgm,Varg);
		exit(1);
	}
	else {
		wait(&st);
		return(st);
	}
}


mrfixup()
{
	register char **argv, *p, c;
	char *ap, *stalloc();

	argv = &Varg[PWVSTART];
	p = Mrs;
	NONBLANK(p);
	for (ap = p; *p; p++) {
		if (*p == ' ' || *p == '\t') {
			if (argv >= &Varg[(NVARGS - 1)])
				fatal("too many MRs (co21)");
			c = *p;
			*p = 0;
			*argv = stalloc(size(ap));
			copy(ap,*argv);
			*p = c;
			argv++;
			NONBLANK(p);
			ap = p;
		}
	}
	--p;
	if (*p != ' ' && *p != '\t')
		copy(ap,*argv++ = stalloc(size(ap)));
	*argv = 0;
}


# define STBUFSZ	500

char *
stalloc(n)
register int n;
{
	static char stbuf[STBUFSZ];
	static int stind = 0;
	register char *p;

	p = &stbuf[stind];
	if (&p[n] >= &stbuf[STBUFSZ])
		fatal("out of space (co22)");
	stind += n;
	return(p);
}


char *
savecmt(p)
register char *p;
{
	register char	*p1, *p2;
	char *fmalloc();
	int	ssize, nlcnt;

	nlcnt = 0;
	for (p1 = p; *p1; p1++)
		if (*p1 == '\n')
			nlcnt++;
/*
 *	ssize is length of line plus mush plus number of newlines
 *	times number of control characters per newline.
*/
	ssize = (strlen(p) + 4 + (nlcnt * 3)) & (~1);
	p1 = fmalloc(ssize);
	p2 = p1;
	while (1) {
		while(*p && *p != '\n')
			*p1++ = *p++;
		if (*p == '\0') {
			*p1 = '\0';
			return(p2);
		}
		else {
			p++;
			*p1++ = '\n';
			*p1++ = CTLCHAR;
			*p1++ = COMMENTS;
			*p1++ = ' ';
		}
	}
}

/*
 *	This routine will apply the editor of your choice to the
 *	result string.
 */

char *
editresult( result )
char *result;
{

	int fd;				/* file descriptor for the tmp file */
	char tmpname[64];		/* generated file name		    */	
	char *editor;			/* editor of your choice	    */
	int status;			/* wait status			    */
	int len;			/* return length for read	    */
	int child;			/* return from fork call	    */
	char *getenv();


	/*
	 *	setup the temp file and editor names
	 */
		
	sprintf(tmpname,"/tmp/sccsed%d",getpid());
	if( ( editor=getenv("EDITOR") ) == NULL )
		editor="vi";
	
	/*
	 *	Create the temp file and write the result buffer to it.
	 *	( Would normally use stdio but the rest of the code doesn't.)
	 */

	if( (fd=creat(tmpname,0666)) < 0 ) {
		fatal("cannot create temp file for editor (30)");
		return(result);
	}
	write(fd, result, strlen(result)); 
	close( fd );

	/*
	 *	Run the editor
	 */
	
	if( (child=fork()) < 0  )
		fatal("cannot fork; try again (co20)");
	
	if( child )
		wait( &status );
	else {
		execlp( editor, editor, tmpname, 0 );
		fatal("can not execute the editor (27)");
		return(result);
	}
	
	/*
	 *	If the status is okay read the results buffer back
	 */
	if( (status & 0377) ) {
		fatal("editor returned bad status (28)");
		return(result);
	}
	if( (fd=open( tmpname, 0 )) < 0 ) {
		fatal("could not open editor temp file (29)");
		return(result);
	}

	len = read(fd,result,RESPSIZE); 
	close( fd );
	unlink( tmpname );
	if( len < RESPSIZE || (len == RESPSIZE && result[len-2] == '\n') )
		return(result);
	else {
		fatal("response too long (co19)");
		return(result);
	}

}
