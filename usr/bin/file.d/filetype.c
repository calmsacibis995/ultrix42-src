#ifndef lint
static	char	*sccsid = "@(#)filetype.c	4.3	(ULTRIX)	1/22/91";
#endif lint
/************************************************************************
 *									*
 *			Copyright (c) 1989 by				*
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
/************************************************************************
 *			Modification History				*
 * 001 Richard Hart, Oct. 21, 1987					*
 *     Copied from 4.3 BSD code:					*
 *		file.c 4.12	(Berkeley) 11/17/85			*
 * 002 Richard Hart, Oct. 21, 1987					*
 *     Added named pipes for Sys V support, and other things in the	*
 *     current Ultrix file.c						*
 * 003 Richard Hart, Oct. 22, 1987					*
 *     Added use of /etc/magic, like Sys V filecommand			*
 * 004 Richard Hart, Nov. 5, 1987					*
 *     Now uses sys/exec.h for support of a.out magic numbers		*
 * 005 Ricky Palmer, Aug. 5, 1988					*
 *     Ifdef'ed for vax/mips.						*
 * 006 Jon Reeves, Nov 12, 1988						*
 *     Fixed ifdefs to allow mode code to work.				*
 * 007 Tim Burke, June 12, 1989						*
 *     Added check for the tape density of DEV_38000BPI which is used   *
 *     on the TA90 tape drive.  Print out "loader" if the tape has a	*
 *     media loader.							*
 * 008 Tim Burke, Sep 13, 1989						*
 *	Added the following TA90 densities: DEV_38000_CP, DEV_76000BPI  *
 *	and DEV_76000_CP.						*
 * 009 Bill Dallas, Jul 05,1990						*
 *	Added the following QIC densities DEV_8000_BPI, DEV_10000_BPI	*
 *	and DEV_16000_BPI.						*
 * 010 Robin Miller, January 8, 1991					*
 *	Added additional tape density checks for DEV_54000_BPI (TZK08)	*
 *	and DEV_61000_BPI (TLZ04).  Also fixed compiler warning error	*
 *	by adding '(struct matcher *) 0' on return from fre_match().	*
 *									*
 ************************************************************************/

/*
 * filetype - determine type of file
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/devio.h>
#include <stdio.h>
#include <ctype.h>
#ifdef vax
#include <a.out.h>
#endif vax
#include "filetype.h"

/*
**	Types
*/

#define	BYTE	0
#define	SHORT	2
#define	LONG	4
#define	STR	8

/*
**	Opcodes
*/

#define	EQ	0
#define	GT	1
#define	LT	2
#define	STRC	3	/* string compare */
#define	ANY	4
#define	SUB	64	/* or'ed in */

/*
**	Misc
*/

#define	NENT	200
#define	BSZ	128
#define	FBSZ	512
#define	reg	register

/*
**	Structure of magic file entry
*/

struct	entry	{
	char	e_level;	/* 0 or 1 */
	long	e_off;		/* in bytes */
	long	e_retcode;	/* major and minor type info to return */
	char	e_type;
	char	e_opcode;
	union	{
		long	num;
		char	*str;
	}	e_value;
	char	*e_str;
};

typedef	struct entry	Entry;

Entry	*mtab;
extern char	*mfile;
extern char	*sys_errlist[];

struct devget devget;
int	errno;
int	sys_nerr;
int in;
int i  = 0;
char buf[BUFSIZ];
char *execmodes();

char *troff[] = {	/* new troff intermediate lang */
	"x","T","res","init","font","202","V0","p1",0};
char *fort[] = {
	"function","subroutine","common","dimension","block","integer",
	"real","data","double",0};
char *asc[] = {
	"chmk","mov","tst","clr","jmp",0};
char *cstart[] = {
	"#include", "#ifndef", "static", "stdio.h", 0};
char *c[] = {
	"int","char","float","double","struct","extern","include",0};
char *as[] = {
	"globl","byte","align","text","data","comm",0};
char *sh[] = {
	"fi", "elif", "esac", "done", "export",
	"readonly", "trap", "PATH", "HOME", 0 };
char *csh[] = {
	"alias", "breaksw", "endsw", "foreach", "limit",  "onintr",
	"repeat", "setenv", "source", "path", "home", 0 };
int	ifile = -1;
int maxexecfn = 2;

#define MASH(x, y)	(((x)<<16) + (y))

long
filetype(file, pflg)
char *file;
int pflg;
{
	int j,nl;
	char ch;
	int minornum, majornum;
	long retnum;
	struct stat mbuf;
	char slink[MAXPATHLEN + 1];
	struct devget *pdevget = &devget;

	if (ifile >= 0)
		close(ifile);		/* close any file that was opened on a previous call */
	ifile = -1;
	if (lstat(file, &mbuf) < 0) {
		if (pflg) printf("%s\n",
		    (unsigned)errno < sys_nerr? sys_errlist[errno]: "Cannot stat");
		return(MASH(UNKNOWN, UNKNOWN));
	}

	switch (mbuf.st_mode & S_IFMT) {

	case S_IFLNK:
		if (pflg) printf("symbolic link");
		j = readlink(file, slink, sizeof slink - 1);
		if (j >= 0) {
			slink[j] = '\0';
			if (pflg) printf(" to %s", slink);
		}
		if (pflg) printf("\n");
		return(MASH(SLINK, UNKNOWN));

	case S_IFDIR:
		minornum = STANDARD;
		if (mbuf.st_mode & S_ISVTX)
			if (pflg) printf("append-only ");
			minornum = APPENDONLY;
		if (pflg) printf("directory\n");
		return(MASH(DIRECTORY, minornum));

	case S_IFPORT: 
		if (pflg) printf("port (named pipe\n");
		return(MASH(NAMEDPIPE, UNKNOWN));

	case S_IFSOCK:
		if (pflg) printf("socket\n");
		return(MASH(SOCKET, UNKNOWN));

	case S_IFBLK:
		if (pflg) printf("block special (%d/%d)\n", major(mbuf.st_rdev), minor(mbuf.st_rdev));
		return(MASH(SPECIAL, BLOCK));

	case S_IFCHR:
		if (!pflg)
			return(MASH(SPECIAL, CHARACTER));
		printf("character special (%d/%d)", major(mbuf.st_rdev), minor(mbuf.st_rdev));
                ifile = open(file, (O_RDONLY|O_NDELAY));
                if(ifile < 0)
                        ifile = open(file, (O_WRONLY|O_NDELAY));
                if(ifile < 0) {
                    printf("\n");
                    return;
                }
                if(ioctl(ifile,DEVIOCGET,(char *) pdevget) < 0) {
                    printf("\n");
                } else {
                    if((strcmp(DEV_UNKNOWN,pdevget->interface) == 0) ||
                       (pdevget->ctlr_num == -1)) {
                            printf(" ");
                    } else {
                            printf(" %s #%d ", pdevget->interface,
                                   pdevget->ctlr_num);
                    }
                    if(!(strcmp(DEV_UNKNOWN,pdevget->device) == 0)) {
                            printf("%s ", pdevget->device);
                    }
                    switch(pdevget->category) {
                    case DEV_TAPE:
                            printf("tape ");
                            break;
                    case DEV_DISK:
                            printf("disk ");
                            break;
                    case DEV_TERMINAL:
                            printf("terminal ");
                            break;
                    case DEV_PRINTER:
                            printf("printer ");
                            break;
                    case DEV_SPECIAL:
                            printf("special_device ");
                            break;
                    default:
                            printf("unknown ");
                            break;
                    }
                    if(!(pdevget->slave_num == -1)) {
                            printf("#%d ", pdevget->slave_num);
                    }
                    if(pdevget->soft_count || pdevget->hard_count) {
                        printf("errors = %d/%d ", pdevget->soft_count,
                           pdevget->hard_count);
                    }
                    if(pdevget->stat & DEV_OFFLINE) {
                        printf("offline ");
                    } else if(pdevget->stat & DEV_WRTLCK) {
                        printf("write-locked ");
                    }
                    if((pdevget->category == DEV_TAPE) &&
                       !(pdevget->stat & DEV_OFFLINE)) {            
                        if(pdevget->category_stat & DEV_LOADER) {
                                printf("loader ");
			}
                        if(pdevget->category_stat & DEV_800BPI) {
                                printf("800_bpi\n");
                        } else if(pdevget->category_stat & DEV_1600BPI) {
                                printf("1600_bpi\n");
                        } else if(pdevget->category_stat & DEV_6250BPI) {
                                printf("6250_bpi\n");
                        } else if(pdevget->category_stat & DEV_6666BPI) {
                                printf("6666_bpi\n");
                        } else if(pdevget->category_stat & DEV_8000_BPI) {
                                printf("8000_bpi\n");
                        } else if(pdevget->category_stat & DEV_10000_BPI) {
                                printf("10000_bpi\n");
                        } else if(pdevget->category_stat & DEV_10240BPI) {
                                printf("10240_bpi\n");
                        } else if(pdevget->category_stat & DEV_16000_BPI) {
                                printf("16000_bpi\n");
                        } else if(pdevget->category_stat & DEV_38000BPI) {
                                printf("38000_bpi\n");
                        } else if(pdevget->category_stat & DEV_38000_CP) {
                                printf("38000_bpi compacted\n");
                        } else if(pdevget->category_stat & DEV_76000BPI) {
                                printf("76000_bpi\n");
                        } else if(pdevget->category_stat & DEV_76000_CP) {
                                printf("76000_bpi compacted\n");
                        } else if(pdevget->category_stat & DEV_54000_BPI) {
                                printf("54000_bpi\n");
                        } else if(pdevget->category_stat & DEV_61000_BPI) {
                                printf("61000_bpi\n");
                        } else {
				/*
				 * Density unknown.
				 */
                                printf("unspecified density\n");
                        }
                    } else if(pdevget->category == DEV_TERMINAL) {
                        if(pdevget->category_stat & DEV_MODEM) {
                                printf("modem_control ");
                                if(pdevget->category_stat & DEV_MODEM_ON) {
                                        printf("on\n");
                                } else {
                                        printf("off\n");
                                }
                        } else {
                                printf("\n");
                        }
                    } else {
                        printf("\n");
                    }
                }
        return;
        }

	ifile = open(file, 0);
	if(ifile < 0) {
		if (pflg) printf("%s\n",
			(unsigned)errno < sys_nerr? sys_errlist[errno]: "Cannot read");
		return(MASH(UNKNOWN, UNKNOWN));
	}
	in = read(ifile, buf, BUFSIZ);
	if(in == 0){
		if (pflg) printf("empty\n");
		return(MASH(EMPTY, UNKNOWN));
	}
	if (retnum = ckmtab(&mbuf, pflg))
		return(retnum);

	if (buf[0] == '\037' && buf[1] == '\235') {
		if (buf[2]&0x80 && pflg)
			printf("block ");
		if (pflg) printf("compressed %d bit code data\n", buf[2]&0x1f);
		return(MASH(COMPRESSED, UNKNOWN));
	}
	if (mbuf.st_size % 512 == 0) {	/* it may be a PRESS file */
		lseek(ifile, -512L, 2);	/* last block */
		if (read(ifile, buf, BUFSIZ) > 0 && *(short *)buf == 12138) {
			if (pflg) printf("PRESS file\n");
			return(MASH(PRESS, UNKNOWN));
		}
	}
	for(i=0; i < in; i++)
		if(buf[i]&0200){
			if (pflg) printf("data\n"); 
			return(MASH(DATA, UNKNOWN));
		};

	/* check for various types of ascii text files now using keyword lists */
	if (cprog() == 1) {
		if (pflg) printf("c program text");
		retnum = CPROG;
		 /* 	checkgarbage();  */
		goto outa;
		}
	else if (fortprog() == 1) {
		if (pflg) printf("fortran program text");
		retnum = FORTPROG;
		goto outa;
		}
notfort:
	i=0;
	if(ascom() == 0)goto notas;
	j = i-1;
	if(buf[i] == '.'){
		i++;
		if(lookup(as) == 1){
			if (pflg) printf("assembler program text"); 
			retnum = ASSEMBLER;
			goto outa;
		}
		else if(buf[j] == '\n' && isalpha(buf[j+2])){
			if (pflg) printf("roff, nroff, or eqn input text");
			retnum = NROFF;
			goto outa;
		}
	}
	while(lookup(asc) == 0){
		if(ascom() == 0)goto notas;
		while(buf[i] != '\n' && buf[i++] != ':')
			if(i >= in)goto notas;
		while(buf[i] == '\n' || buf[i] == ' ' || buf[i] == '\t')if(i++ >= in)goto notas;
		j = i-1;
		if(buf[i] == '.'){
			i++;
			if(lookup(as) == 1){
				if (pflg) printf("assembler program text"); 
				retnum = ASSEMBLER;
				goto outa; 
			}
			else if(buf[j] == '\n' && isalpha(buf[j+2])){
				if (pflg) printf("roff, nroff, or eqn input text");
				retnum = NROFF;
				goto outa;
			}
		}
	}
	if (pflg) printf("assembler program text");
	retnum = ASSEMBLER;
	goto outa;
notas:
	if (troffint(buf, in)) {
		if (pflg) printf("troff intermediate output text");
		retnum = TROFFINT;
	}
	else if (shell(buf, in, sh)) {
		if (pflg) printf("%sshell commands", execmodes(&mbuf, buf));
		retnum = SHELL;
	}
	else if (shell(buf, in, csh)) {
		if (pflg) printf("%sc-shell commands", execmodes(&mbuf, buf));
		retnum = CSHELL;
	}
	else if (english(buf, in)) {
		if (pflg) printf("English text");
		retnum = ENGLISH;
	}
	else {
		if (pflg) printf("ASCII text");
		retnum = UNKNOWN;
	}
outa:
	while(i < in)
		if((buf[i++]&0377) > 127){
			if (pflg) printf(" with garbage\n");
			return(MASH(ASCIIwGARBAGE, retnum));
		}
	/* if next few lines in then read whole file looking for nulls ...
		while((in = read(ifile,buf,BUFSIZ)) > 0)
			for(i = 0; i < in; i++)
				if((buf[i]&0377) > 127){
					if (pflg) printf(" with garbage\n");
					return(MASH(ASCIIwGARBAGE, retnum));
				}
		/*.... */
	if (pflg) printf("\n");
	return(MASH(ASCII, retnum));
}

mkmtab(cflg)
reg	int	cflg;
{
	reg	Entry	*ep;
	reg	FILE	*fp;
	reg	int	lcnt = 0;
	auto	char	buf[BSZ];
	auto	Entry	*mend;
	auto majnum, minnum;
	char *strchr();

	ep = (Entry *) calloc(sizeof(Entry), NENT);
	if(ep == NULL) {
		fprintf(stderr, "file: no memory for magic table.\n");
		exit(2);
	}
	mtab = ep;
	mend = &mtab[NENT];
	fp = fopen(mfile, "r");
	if(fp == NULL) {
		fprintf(stderr, "file: cannot open magic file <%s>.\n", mfile);
		exit(2);
	}
	while(fgets(buf, BSZ, fp) != NULL) {
		reg	char	*p = buf;
		reg	char	*p2, *p3;
		reg	char	opc;
		int count;

		if(*p == '\n' || *p == '#')
			continue;
		lcnt++;
			

			/* LEVEL */
		if(*p == '>') {
			ep->e_level = 1;
			p++;
		}
			/* OFFSET */
		p2 = strchr(p, '\t');
		if(p2 == NULL) {
			if(cflg)
				fprintf(stderr, "file: magic format error, no tab after %son line %d\n", p, lcnt);
			continue;
		}
		*p2++ = NULL;
		ep->e_off = atolo(p);
		while(*p2 == '\t')
			p2++;
			/* TYPE */
		p = p2;
		p2 = strchr(p, '\t');
		if(p2 == NULL) {
			if(cflg)
				fprintf(stderr, "file: magic format error, no tab after %son line %d\n", p, lcnt);
			continue;
		}
		*p2++ = NULL;
		if(*p == 's') {
			if(*(p+1) == 'h')
				ep->e_type = SHORT;
			else
				ep->e_type = STR;
		} else if (*p == 'l')
			ep->e_type = LONG;
		while(*p2 == '\t')
			*p2++;
			/* OP-VALUE */
		p = p2;
		p2 = strchr(p, '\t');
		if(p2 == NULL) {
			if(cflg)
				fprintf(stderr, "file: magic format error, no tab after %son line %d\n", p, lcnt);
			continue;
		}
		*p2++ = NULL;
		if(ep->e_type != STR) {
			opc = *p++;
			switch(opc) {
			case '=':
				ep->e_opcode = EQ;
				break;

			case '>':
				ep->e_opcode = GT;
				break;

			case '<':
				ep->e_opcode = LT;
				break;

			case 'x':
				ep->e_opcode = ANY;
				break;

			default:
				p--;
			}
		}
		if(ep->e_opcode != ANY) {
			if(ep->e_type != STR)
				ep->e_value.num = atolo(p);
			else {
				ep->e_value.str = (char *)malloc(strlen(p) + 1);
				p3 = ep->e_value.str;
				while (*p != '\0') 
				    if (*p == '\\')
					switch (*++p) {
					case 'n':
						*p3++ = '\n';
						p++;
						break;
					case 't':
						*p3++ = '\t';
						p++;
						break;
					case 'b':
						*p3++ = '\b';
						p++;
						break;
					case 'r':
						*p3++ = '\r';
						p++;
						break;
					case 'f':
						*p3++ = '\f';
						p++;
						break;
					case '0':
					case '1':
					case '2':
					case '3':
					case '4':
					case '5':
					case '6':
					case '7':
					case '8':
					case '9':
						{
						int value = 0,
						    base = 10;

						while (*p >= '0' && *p <= '9') {
							if (*p == '0' && value == 0) base = 8;
							value = value*base + (*p++ - '0');
							}
						*p3++ = (unsigned char)value;
						break;
						}
					default:
						*p3++ = *p++;
				}
			   else
				*p3++ = *p++;
			*p3 = '\0';
			p3 = (char *)fre_comp(ep->e_value.str);
			if (p3 == 0) fprintf(stderr, "file: bad reg exp in magic file: %s\n", ep->e_value.str);
			else {
				free(ep->e_value.str);
				ep->e_value.str = p3;
			}
			}
		}
		while(*p2 == '\t')
			*p2++;
			/* MAJOR/MINOR TYPE NUMBERS */
		if (sscanf(p2, "%d,%d", &majnum, &minnum) < 2)
			fprintf(stderr, "file: invalid major,minor numbers in magic file.\n");
		ep->e_retcode = MASH(majnum, minnum);
		p2 = strchr(p2, '\t');
		while(*p2 == '\t')
			*p2++;

			/* STRING */

		ep->e_str = (char *)malloc(strlen(p2) + 1);
		p = ep->e_str;
		while(*p2 != '\n') {
			if(*p2 == '%')
				ep->e_opcode |= SUB;
			*p++ = *p2++;
		}
		*p = NULL;
		ep++;
		if(ep >= mend) {
			fprintf(stderr, "file: magic table overflow.\n");
			exit(2);
		}
	}
	ep->e_off = -2L*(long)maxexecfn;
}

printmtab()
{
reg Entry *ep;

	printf("level   offset  type    opcode  major,minor	value   string\n");
	for(ep = mtab; ep->e_off > -2L*(long)maxexecfn; ep++) {
		printf("%d\t%d\t%d\t%d\t%d,%d\t",
			ep->e_level, ep->e_off, ep->e_type, ep->e_opcode, 
			(short)(ep->e_retcode>>16),
			(short)(ep->e_retcode & 0177777));
		if (ep->e_type == STR) {
			printstr(ep->e_value.str, 50);
			printf("\t");
			}
		else
			printf("%#lo\t", ep->e_value.num);
		printf("%s", ep->e_str);
		if (ep->e_opcode & SUB)
			printf("\tsubst");
		printf("\n");
	}
}

printstr(p, n)
unsigned char *p;
int n;
{

	register unsigned char *sp;
	register int c;

	for (sp = p, c = 0; *sp != '\0' && c++ < n; sp++)
		if (isprint(*sp)) printf("%c", *sp);
		else if (*sp == '\n') printf("\\n");
		else if (*sp == '\r') printf("\\r");
		else if (*sp == '\t') printf("\\t");
		else if (*sp == '\b') printf("\\b");
		else if (*sp == '\b') printf("\\f");
		else printf("\\%#o", *sp);
}

long
atolo(s)
reg	char	*s;
{
	reg	char	c;
	reg	char	*fmt = "%ld";
	auto	long	j = 0L;

	if(*s == '0') {
		s++;
		if(*s == 'x') {
			s++;
			fmt = "%lx";
		} else
			fmt = "%lo";
	}
	sscanf(s, fmt, &j);
	return(j);
}

char *
execmodes(mbuf, fbuf)
struct stat *mbuf;
char *fbuf;
{
	static char msg_buf[26];

	msg_buf[0] = '\0';
	if (mbuf->st_mode & S_ISUID)
		strncat(msg_buf, "setuid ", 7);
	if (mbuf->st_mode & S_ISGID)
		strcat(msg_buf, "setgid ");
	if (mbuf->st_mode & S_ISVTX)
		strcat(msg_buf, "sticky ");
	return(msg_buf);
}

#ifdef vax
char *
symtable(mbuf, fbuf)
struct stat *mbuf;
char *fbuf;
{
	struct exec ex;
	struct stat stb;

	ex = *(struct exec *)fbuf;
	if (fstat(ifile, &stb) < 0)
		return("");

	if (((int *)buf[4])!= 0)
 	if ((int)N_STROFF(ex)+sizeof(off_t) > stb.st_size)
		return ("version 7 style symbol table");
	return ("");
}
#else !vax
char *
symtable(mbuf, fbuf)
struct stat *mbuf;
char *fbuf;
{
	return("");
}
#endif vax
char *(*execfns[])() = { symtable, symtable, execmodes };

long
ckmtab(mbuf, pflg)
struct stat *mbuf;
int pflg;
{

	reg	Entry	*ep;
	reg	char	*p;
	reg	int	lev1 = 0;
	int fn;
	long retcode;

	auto	union	{
		long	l;
		char	ch[4];
		}	val, revval;
	static	char	init = 0;
	struct matcher {
		char *start, *finish;
		} *match;
	char pbuf[256]; /* these fixed length arrays are bad. fix then sometime */
	char sbuf[256];
	char *sptr;
	int slen = 0;
	int match_len;

	if(!init) {
		mkmtab(0);
		init = 1;
	}
	for(ep = mtab; ep->e_off > -2L * (long)maxexecfn; ep++) {
		if(lev1) {
			if(ep->e_level != 1)
				break;
			if (pflg && (slen > 0)) putchar(' ');
			slen = 0;
		} else if(ep->e_level == 1)
			continue;

		if (pflg && ep->e_off < 0L) {
			fn = -1 * ep->e_off;
			if (fn > maxexecfn)
				sprintf(sbuf, ep->e_str, " ???? ");
			else
				sprintf(sbuf, ep->e_str, (char *)(*execfns[fn])(mbuf, buf));
			slen = strlen(sbuf);
			printf(sbuf);
			continue;
			}
			
		p = &buf[ep->e_off];
		switch(ep->e_type) {
		case STR:
		{
	
			if((match = (struct matcher *)fre_exec(p,ep->e_value.str)) <= (struct matcher *) 0)
				continue;
			match_len = match->finish - match->start;
			strncpy(pbuf, match->start, match_len);
			pbuf[match_len] = '\0';
			break;
		}

		case BYTE:
			val.l = (long)(*(unsigned char *) p);
			break;

		case SHORT:
			val.l = (long)(*(unsigned short *) p);
			break;

		case LONG:
			val.l = (*(long *) p);
			break;
		}

		if (ep->e_type != STR)
		switch(ep->e_opcode & ~SUB) {
		case EQ:
			if(val.l != ep->e_value.num)
				continue;
			break;
		case GT:
			if(val.l <= ep->e_value.num)
				continue;
			break;

		case LT:
			if(val.l >= ep->e_value.num)
				continue;
			break;
		}
		if (pflg)
			if(ep->e_opcode & SUB) { /* we really need the SysV printf here to return */
					         /* the number of characters printed.		*/
				sprintf(sbuf, ep->e_str, (ep->e_type == STR) ? (int)pbuf : val.l);
				sptr = sbuf;
				}
			else
				sptr=ep->e_str;
		slen = strlen(sptr);
		printf(sptr);
		retcode = ep->e_retcode;
		lev1 = 1;
	}
	if(lev1) {
		if (pflg) putchar('\n');
		return(retcode);
	}
	return(0);
}

int ci;

cprog()
{
	int j, nl;
	char ch;

	ci = 0;

	if (lookup(cstart) == 1)
		return(1);
	if(ccom() == 0) return(0);

	while(buf[ci] == '#'){
		j = ci;
		while(buf[ci++] != '\n'){
			if(ci - j > 255)
				return(0);
			if(ci >= in) return(0);
		}
		if(ccom() == 0) return(0);
	}
check:
	if(lookup(c) == 1){
		if (buf[0] == '%' && buf[1] == '!') return(0);	/* actually postscript */
		while((ch = buf[ci++]) != ';' && ch != '{')if(ci >= in) return(0);
		return(1);
	}
	nl = 0;
	while(buf[ci] != '('){
		if(buf[ci] <= 0)
			return(0);
		if(buf[ci] == ';'){
			ci++; 
			goto check; 
		}
		if(buf[ci++] == '\n')
			if(nl++ > 6) return(0);
		if(ci >= in) return(0);
	}
	while(buf[ci] != ')'){
		if(buf[ci++] == '\n')
			if(nl++ > 6) return(0);
		if(ci >= in) return(0);
	}
	while(buf[ci] != '{'){
		if(buf[ci++] == '\n')
			if(nl++ > 6) return(0);
		if(ci >= in) return(0);
	}
	if (buf[0] == '%' && buf[1] == '!') return(0);	/* actually postscript */
	return(1);
}

ccom(){
	char cc;
	while((cc = buf[ci]) == ' ' || cc == '\t' || cc == '\n')if(ci++ >= in)return(0);
	if(buf[ci] == '/' && buf[ci+1] == '*'){
		ci += 2;
		while(buf[ci] != '*' || buf[ci+1] != '/'){
			if(buf[ci] == '\\')ci += 2;
			else ci++;
			if(ci >= in)return(0);
		}
		if((ci += 2) >= in)return(0);
	}
	if(buf[ci] == '\n')if(ccom() == 0)return(0);
	return(1);
}

int
fortprog()
{
	int i = 0;

	while(buf[i] == 'c' || buf[i] == '#'){
		while(buf[i++] != '\n')if(i >= in) return(0);
	}
	if(lookup(fort) == 1)
		return(1);
	return(0);
}

ascom(){
	while(buf[i] == '/'){
		i++;
		while(buf[i++] != '\n')if(i >= in)return(0);
		while(buf[i] == '\n')if(i++ >= in)return(0);
	}
	return(1);
}

sccs() {
	reg int i;

	if(buf[0] == 1 && buf[1] == 'h')
		for(i=2; i<=6; i++)
			if(isdigit(buf[i])) continue;
			else return(0);
	else
		return(0);
	return(1);
}

troffint(bp, n)
char *bp;
int n;
{
	int k;

	i = 0;
	for (k = 0; k < 6; k++) {
		if (lookup(troff) == 0)
			return(0);
		if (lookup(troff) == 0)
			return(0);
		while (i < n && buf[i] != '\n')
			i++;
		if (i++ >= n)
			return(0);
	}
	return(1);
}
lookup(tab)
char *tab[];
{
	char r;
	int k,j,l;
	while(buf[i] == ' ' || buf[i] == '\t' || buf[i] == '\n')i++;
	for(j=0; tab[j] != 0; j++){
		l=0;
		for(k=i; ((r=tab[j][l++]) == buf[k] && r != '\0');k++);
		if(r == '\0')
			if(buf[k] == ' ' || buf[k] == '\n' || buf[k] == '\t'
			    || buf[k] == '{' || buf[k] == '/' || buf[k] == '>'){
				i=k;
				return(1);
			}
	}
	return(0);
}

english (bp, n)
char *bp;
{
# define NASC 128
	int ct[NASC], j, vow, freq, rare;
	int badpun = 0, punct = 0;
	if (n<50) return(0); /* no point in statistics on squibs */
	for(j=0; j<NASC; j++)
		ct[j]=0;
	for(j=0; j<n; j++)
	{
		if (bp[j]<NASC)
			ct[bp[j]|040]++;
		switch (bp[j])
		{
		case '.': 
		case ',': 
		case ')': 
		case '%':
		case ';': 
		case ':': 
		case '?':
			punct++;
			if ( j < n-1 &&
			    bp[j+1] != ' ' &&
			    bp[j+1] != '\n')
				badpun++;
		}
	}
	if (badpun*5 > punct)
		return(0);
	vow = ct['a'] + ct['e'] + ct['i'] + ct['o'] + ct['u'];
	freq = ct['e'] + ct['t'] + ct['a'] + ct['i'] + ct['o'] + ct['n'];
	rare = ct['v'] + ct['j'] + ct['k'] + ct['q'] + ct['x'] + ct['z'];
	if (2*ct[';'] > ct['e']) return(0);
	if ( (ct['>']+ct['<']+ct['/'])>ct['e']) return(0); /* shell file test */
	return (vow*5 >= n-ct[' '] && freq >= 10*rare);
}

shell(bp, n, tab)
	char *bp;
	int n;
	char *tab[];
{

	i = 0;
	do {
		if (buf[i] == '#' || buf[i] == ':')
			while (i < n && buf[i] != '\n')
				i++;
		if (++i >= n)
			break;
		if (lookup(tab) == 1)
			return (1);
	} while (i < n);
	return (0);
}
