#ifndef lint
static char *sccsid = "@(#)main.c	4.4      (ULTRIX)        4/11/91";
#endif

/************************************************************************
 *									*
 *			Copyright (c) 1988-1989 by			*
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
 * History:
 *
 * 02-Mar-90  Janet Schank
 *      Changed Nscsi to Nscsibus.
 *
 */

#include	"crash.h"
#include	"cmd.h"
#undef JB_PC	/* these are defined in pcb.h - already loaded in crash.h */
#undef JB_S0
#undef JB_S1
#undef JB_S2
#undef JB_S3
#undef JB_S4
#undef JB_S5
#undef JB_S6
#undef JB_S7
#undef JB_SP
#undef JB_S8
#undef NJBREGS
#include	<setjmp.h>
#include	<ctype.h>
#include	<sys/buf.h>
#include	<sys/ioctl.h>
#include	<sys/param.h>
#include	<sys/map.h>
#include	<sys/file.h>
#include	<sgtty.h>
#include	<signal.h>
#include	<sys/varargs.h>
#include	<sys/proc.h>


struct winsize win;
int		mem;
char		*namelist_default = "/vmunix";
char		*dumpfile_default = "/dev/mem";
char		*namelist=NULL;
char		*dumpfile=NULL;
#define LINESIZE 256
char		line[LINESIZE], *p = line;
char		sbuf[BUFSIZ];		/* buffer for stdout */
int		tok;
jmp_buf		jmp;

struct	Symbol   Swap, Sys, Panic, Text, Sptbase, Callout, Lbolt, 
		Region, Usrptma, Usrpt, Cmap, Buffree, Gfree, Arptab,
		Dnlcache, Cmhash, Mscp_classb, Tmscp_classb,
		Mscp_utable, Tmscp_utable, Sz_softc, Nscsibus,
		Scs_config_db, Ports;

static int __terminal;
static int __stopresponse = 0;
static struct sgttyb ttybsave;
static struct sgttyb ttybnew;


#define HISTSIZE 20	/* default getenv can change it */
int histsize = HISTSIZE;
char *lastline;				/* malloc buffer for history */
int lines_out = 0;
int map_to_do;
int batch=0;

char origline[LINESIZE];
char *getenv(),*malloc();

main(argc, argv)
	int	argc;
	char	**argv;
{
	char	*token();
	long	atol();
	register  struct  tsw	*tp;
        register  char  *c;
	register  int i;
	int	cpu, detail, cnt, units, r;
	struct	Symbol	*nmsrch(), *symgrep(), *search(), *sp;
	int	sigint();
	int	die();
	int	core();
	int	suspend();
	int	cont();
	extern	showsyms();
	struct	prmode	*prptr;
	extern	unsigned  ttycnt;
	extern	unsigned scan_vaddr();
	unsigned addr;
	int index;
	int done;
	int mbuf_flag;
	

	while (argc > 1) {
		if (argv[1][0] == '-') {
			switch(argv[1][1]) {
				case 'b':
					batch++;
					__stopresponse = 1;
					break;

					
	default:
					usage();
			}
		}
		else {
			if (dumpfile == NULL)
			dumpfile = argv[1];
			else if (namelist == NULL)
				namelist = argv[1];
	}
		argc--;
		argv++;
	}
	if (namelist == NULL)
		namelist = namelist_default;
	if (dumpfile == NULL)
		dumpfile = dumpfile_default ;
	printf("namelist: %s core: %s\n",namelist,dumpfile);
	if(__terminal = (isatty(0) && isatty(1))) {
		(void)ioctl(1, TIOCGWINSZ, (char *)&win); /* get winsize */
		if (win.ws_row == 0) win.ws_row = 24;	/* set a default for the poor fool */
		setbuf(stdout,sbuf);
		(void)ioctl(0, TIOCGETP, (char *)&ttybsave);
		ttybnew = ttybsave;
		signal(SIGHUP, die);
		signal(SIGSEGV, core);
		signal(SIGILL, core);
		signal(SIGBUS, core);
		signal(SIGCONT, cont);
		signal(SIGTSTP, suspend);
	} else 
		lastline = NULL;

	/* set up history buffer */
	c = getenv("HISTSIZE");
	if (c)	histsize = atoi(c);
	lastline = malloc((unsigned)(histsize*LINESIZE));
	if (!lastline) {
		fprintf(stderr,"Can't get history buffer\n");
		exit(1);
	}
	bzero(lastline,histsize*LINESIZE);
	init();

	printf("Kernel version: \n");
	if ((sp = nmsrch("version")) == NULL) {
		printf("*Unknown*\n");
	} else {
		prod(sp->s_value, 1, "s");
		printf("\n");
	}

	(void)setjmp(jmp);

	for (;;) {
		__stopresponse = 0;
		done = 0;
		fflush(stdout);
		while((c = token()) == NULL);
			for(tp = t; tp->t_action != 0; tp++) {
			if(strncmp(tp->t_nm, c, strlen(c)) == 0) {
				(*tp->t_action)(c);
				break;
			}
			
		}
		if (tp->t_action == NULL)
			printf("Unknown command: %s\n",c);
	}
}

sigint(sig,code,scp)
	int sig,code;
	struct sigcontext *scp;
{
	char  *token();

#ifdef lint
	if((sig == 0) ||
	(code == 0) ||
	(scp == NULL));
#endif
	p = line;
	tok = 1;
	line[0] = '\0';
	lines_out = 0;
	(void)ioctl(0, TIOCSETP, (char *)&ttybsave);	
	putchar('\n');
	if(*p)
		while(token() != NULL);
	longjmp(jmp, 1);
}

char	*
token()
{
	register  char  *cp;
	register int i, num;

	for(;;) {
		switch(*p) {
		case '\0':
		case '\n':
			if(*p == '\n') {
				p++;
				tok = 0;
			}
			if(tok != 0) {
				tok = 0;
				return(NULL);
			}
			prompt("> ");
			lines_out = 0;
			p = line;
			if(fgets(line, LINESIZE, stdin) == NULL)
				return("quit");
			bcopy(line,origline,LINESIZE);
			/* turn #\n into #0 (# is short for #0 a redo! */
			if (line[0] == '#' && line[1] == '\n') {
				line[1] = '0';
				line[2] = '\0';
			}
			if (line[0] == '#') {
				/* histlist  #h command */
				if (line[1] == 'h') {
					pr_history();
					line[0] = '\0';
					continue;
				}
				/* assume number like #5 */
				num = atoi(&line[1]);
				if (num >= 0 && num <= histsize-1 &&
					*(lastline+num*LINESIZE) != NULL) {
					bcopy(lastline+num*LINESIZE,
						line,LINESIZE);
					bcopy(lastline+num*LINESIZE,
						origline,LINESIZE);
					/* do history for hist command */
					for(i=histsize-2;i>=0;i--) {
						bcopy(lastline+i*LINESIZE,
						lastline+(i+1)*LINESIZE,
						LINESIZE);
					}
					bcopy(line,lastline,LINESIZE);
					/* hist invoke */
					printf("%s",line);
					fflush(stdout);
					continue;
				}
				else {
					printf("bad history number\n");
					line[0] = '\0';
					fflush(stdout);
					continue;
				}
			}
			if(line[0] == '!') {
				system(&line[1]);
				line[0] = '\0';
				continue;
			}
			/* do history for normal commands */
			for(i=histsize-2;i>=0;i--) {
				bcopy((char *)lastline + i * LINESIZE,
				    (char *)lastline + (i + 1) * LINESIZE,
				    LINESIZE);
			}
			bcopy(origline,lastline,LINESIZE);
			continue;

		case ' ':
		case '\t':
			p++;
			continue;

		default:
			tok++;
			cp = p;
			while(*p!=' ' && *p!='\t' && *p!='\n')
				p++;
			*p++ = '\0';
			return(cp);
		}
	}
}

prompt(str)
	char *	str;
{
	while(*str)
		putc(*str++, stderr);
}

#ifndef lint
printf(fmt, va_alist)
	char *fmt;
#ifdef vax
	unsigned va_alist;
{
	char *ap = (char *) &va_alist;
#endif vax
#ifdef mips
	va_dcl
{
	va_list(ap);
#endif mips
	register char *ptr = fmt;
	char c;
	int i;
	va_start(ap);
	if (!batch&&!__stopresponse && __terminal && 
	    lines_out >= win.ws_row-1) {
		_doprnt(
		"<cr> for next line, q to quit, ^d to stop pagination, <sp> to continue ",
		NULL, stdout);
		fflush(stdout);
#ifdef notdef
		putchar('s');putchar('p');putchar('a');putchar('c');
		putchar('e');putchar(' ');putchar('t');putchar('o');
		putchar(' ');putchar('g');putchar('o');putchar(' ');
		putchar('o');putchar('n');putchar(':');
		fflush(stdout);
#endif
		ttybnew.sg_flags |= CBREAK;
		ttybnew.sg_flags &= ~ECHO;
		(void)ioctl(0, TIOCSETP, (char *)&ttybnew);
		read(0,&c,1);
		for(i = 0; i < 72; i++) 
			putchar(010);
		for(i = 0; i < 72; i++)
			putchar(' ');
		for(i = 0; i < 72; i++) 
			putchar(010);
		(void)ioctl(0, TIOCSETP, (char *)&ttybsave);	
		(void)ioctl(0, TIOCFLUSH, (char *)FREAD);
		switch(c) {
			case '\r' :
			case '\n' :
				lines_out--;
				break;
			case 'q' :
			case 'Q' :
				putchar('\n');
				if(*p)
					while(token() != NULL);
				longjmp(jmp, 2);
				/*NOTREACHED*/
			case '\04' :
				__stopresponse++;
				break;
			default :
				lines_out = 0;
		
		}
		fflush(stdout);
	}
	while (*ptr) { if (*ptr == '\n') lines_out++; ptr++; }
#ifndef SYSTEM_FIVE

	/* Call _doprnt() and then return EOF for failure and 0 for success.
	 */
	_doprnt(fmt, ap, stdout);
	return(ferror(stdout)? EOF: 0);
#else

	/* For the System V environment, call _doprnt() then return EOF
	 * for failure and the number of characters transmitted by _doprnt()
	 * for success.
	 */
	int n_chars;
	n_chars = _doprnt(fmt, ap, stdout);
	return(ferror(stdout)? EOF: n_chars);
#endif
}
#endif

die() {
	(void)ioctl(0, TIOCSETP, (char *)&ttybsave);
	fflush(stdout);
	exit(1);
}

core() {
	fflush(stdout);
	signal(SIGIOT, SIG_DFL);
	(void)ioctl(0, TIOCSETP, (char *)&ttybsave);	
	kill(getpid(), SIGIOT);
}

suspend() {
	signal(SIGTSTP, SIG_DFL);
	(void)ioctl(0, TIOCSETP, (char *)&ttybsave);
	putchar('\n');
	fflush(stdout);
	kill(getpid(), SIGTSTP);
}

cont() {
	signal(SIGTSTP, suspend);
	if(*p)
		while(token() != NULL);
	longjmp(jmp, 2);
}


pr_history() {
	register int i;
	
	for(i = 0; i < histsize; i++) {
		if (*(lastline + i * LINESIZE))
			printf("%d %s", i, lastline + i * LINESIZE);
	}
	fflush(stdout);
}	


c_quit(c)
	char *c;
{
	char *arg;
	int index;
	unsigned int addr;

	if(__terminal)
		(void)ioctl(0, TIOCSETP, (char *)&ttybsave);
	exit(0);
	
}
usage() 
{
	fatal("usage: crash  [ dump ]  [ namelist ]");
}	
