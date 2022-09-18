#ifndef lint
static char *sccsid = "@(#)grep.c	4.1	(ULTRIX)	7/2/90";
#endif

/*
 * grep -- print lines matching (or not matching) a pattern
 *
 *	status returns:
 *		0 - ok, and some matches
 *		1 - ok, but no matches
 *		2 - some error
 */

/* Modifications:
 * 	01 -	Teoman Topcubasi, 13-May-87
 *		Changed static handling of RE's to dynamic handling, related
 *		problem report ICA-05890
 *
 *	02 -	Teoman Topcubasi, 16-Feb-88
 *		Eight bit cleaned grep.
 *
 *	03 -	Lie-Min Hioe, 06-July-89
 *		Fixed the problem of not reading the option list properly.
 *
 *	04 -	Lie-Min Hioe, 06-July-89
 *		Eight bit cleaned grep. Changed char into unsigned char
 *		grep did not operate correctly in an 8-bit transparent manner
 *		for regular expressions related to character class [].
 */

#include <stdio.h>
#include <ctype.h>

#define	CBRA	1
#define	CCHR	2
#define	CDOT	4
#define	CCL	6
#define	NCCL	8
#define	CDOL	10
#define	CEOF	11
#define	CKET	12
#define	CBRC	14
#define	CLET	15
#define	CBACK	18

#define	STAR	01

#define	LBSIZE	BUFSIZ
#define	ESIZE	2048
#define	NBRA	9

int	cursize = ESIZE;
unsigned char	*expbuf, *start, *start1;
long	lnum;
unsigned char	linebuf[LBSIZE+1];
char	ybuf[ESIZE];
int	bflag;
int	lflag;
int	nflag;
int	cflag;
int	vflag;
int	nfile;
int	hflag	= 1;
int	sflag;
int	yflag;
int	wflag;
int	retcode = 0;
int	circf;
long	tln;
int	nsucc;
unsigned char	*braslist[NBRA];
unsigned char	*braelist[NBRA];
unsigned char	bittab[] = {
	1,
	2,
	4,
	8,
	16,
	32,
	64,
	128
};

char *malloc(), *realloc();

main(argc, argv)
char **argv;
{
	char	*cp;

	while (--argc > 0 && (++argv)[0][0]=='-') {

		cp = argv[0] + 1;
		while (*cp) 

		switch (*cp++) {

		case 'i':
		case 'y':
			yflag++;
			continue;

		case 'w':
			wflag++;
			continue;

		case 'h':
			hflag = 0;
			continue;

		case 's':
			sflag++;
			continue;

		case 'v':
			vflag++;
			continue;

		case 'b':
			bflag++;
			continue;

		case 'l':
			lflag++;
			continue;

		case 'c':
			cflag++;
			continue;

		case 'n':
			nflag++;
			continue;

		case 'e':
			--argc;
			++argv;
			goto out;

		default:
			errexit("grep: unknown flag\n", (char *)NULL);
			continue;
		}
	}
out:
	if (argc<=0)
		exit(2);
	expbuf = start = (unsigned char *) malloc(cursize); /* 01 */
	if (yflag) {
		register char *p, *s;
		for (s = ybuf, p = *argv; *p; ) {
			if (*p == '\\') {
				*s++ = *p++;
				if (*p)
					*s++ = *p++;
			} else if (*p == '[') {
				while (*p != '\0' && *p != ']')
					*s++ = *p++;
			} else if (islower(*p)) {
				*s++ = '[';
				*s++ = toupper(*p);
				*s++ = *p++;
				*s++ = ']';
			} else
				*s++ = *p++;
			if (s >= ybuf+ESIZE-5)
				errexit("grep: argument too long\n", (char *)NULL);
		}
		*s = '\0';
		*argv = ybuf;
	}
	compile(*argv);
	nfile = --argc;
	if (argc<=0) {
		if (lflag)
			exit(1);
		execute((char *)NULL);
	} else while (--argc >= 0) {
		argv++;
		execute(*argv);
		fflush(stdout);		/* flush on a per file basis */
	}
	exit(retcode != 0 ? retcode : nsucc == 0);
}

compile(astr)
unsigned char *astr;
{
	register c;
	register unsigned char *ep, *sp;
	unsigned char *cstart;
	unsigned char *lastep;
	int cclcnt;
	unsigned char bracket[NBRA], *bracketp;
	int closed;
	unsigned char numbra;
	char neg;

	ep = expbuf;
	sp = astr;
	lastep = 0;
	bracketp = bracket;
	closed = numbra = 0;
	if (*sp == '^') {
		circf++;
		sp++;
	}
	if (wflag)
		*ep++ = CBRC;
	for (;;) {
		if (ep >= &expbuf[cursize])
			goto cerror;
		if ((c = *sp++) != '*')
			lastep = ep;
		switch (c) {

		case '\0':
			if (wflag)
				*ep++ = CLET;
			*ep++ = CEOF;
			return;

		case '.':
			*ep++ = CDOT;
			continue;

		case '*':
			if (lastep==0 || *lastep==CBRA || *lastep==CKET ||
			    *lastep == CBRC || *lastep == CLET)
				goto defchar;
			*lastep |= STAR;
			continue;

		case '$':
			if (*sp != '\0')
				goto defchar;
			*ep++ = CDOL;
			continue;

		case '[':
			if(&ep[17] >= &expbuf[cursize]) {
	/* 01 */		start1 = (unsigned char *)realloc(start, (2 * cursize));
				ep = start1 + (ep - start);
				start = start1;
				cursize = 2 * cursize;
			}
			*ep++ = CCL;
			neg = 0;
			if((c = *sp++) == '^') {
				neg = 1;
				c = *sp++;
			}
			cstart = sp;
			do {
				if (c=='\0')
					goto cerror;
				if (c=='-' && sp>cstart && *sp!=']') {
					for (c = sp[-2]; c<*sp; c++)
						ep[c>>3] |= bittab[c&07];
					sp++;
				}
				ep[c>>3] |= bittab[c&07];
			} while((c = *sp++) != ']');
			if(neg) {
				for(cclcnt = 0; cclcnt < 16; cclcnt++)
					ep[cclcnt] ^= -1;
				ep[0] &= 0376;
			}

			ep += 16;

			continue;

		case '\\':
			if((c = *sp++) == 0)
				goto cerror;
			if(c == '<') {
				*ep++ = CBRC;
				continue;
			}
			if(c == '>') {
				*ep++ = CLET;
				continue;
			}
			if(c == '(') {
				if(numbra >= NBRA) {
					goto cerror;
				}
				*bracketp++ = numbra;
				*ep++ = CBRA;
				*ep++ = numbra++;
				continue;
			}
			if(c == ')') {
				if(bracketp <= bracket) {
					goto cerror;
				}
				*ep++ = CKET;
				*ep++ = *--bracketp;
				closed++;
				continue;
			}

			if(c >= '1' && c <= '9') {
				if((c -= '1') >= closed)
					goto cerror;
				*ep++ = CBACK;
				*ep++ = c;
				continue;
			}

		defchar:
		default:
			*ep++ = CCHR;
			*ep++ = c;
		}
	}
    cerror:
	errexit("grep: Regular Expression error\n", (char *)NULL);
}

execute(file)
unsigned char *file;
{
	register unsigned char *p1, *p2;
	register c;

	if (file) {
		if (freopen(file, "r", stdin) == NULL) {
			perror(file);
			retcode = 2;
		}
	}
	lnum = 0;
	tln = 0;
	for (;;) {
		lnum++;
		p1 = linebuf;
		while ((c = getchar()) != '\n') {
			if (c == EOF) {
				if (cflag) {
					if (nfile>1)
						printf("%s:", file);
					printf("%D\n", tln);
					fflush(stdout);
				}
				return;
			}
			*p1++ = c;
			if (p1 >= &linebuf[LBSIZE-1])
				break;
		}
		*p1 = '\0';
		p1 = linebuf;
		p2 = expbuf;
		if (circf) {
			if (advance(p1, p2))
				goto found;
			goto nfound;
		}
		/* fast check for first character */
		if (*p2==CCHR) {
			c = p2[1];
			do {
				if (*p1!=c)
					continue;
				if (advance(p1, p2))
					goto found;
			} while (*p1++);
			goto nfound;
		}
		/* regular algorithm */
		do {
			if (advance(p1, p2))
				goto found;
		} while (*p1++);
	nfound:
		if (vflag)
			succeed(file);
		continue;
	found:
		if (vflag==0)
			succeed(file);
	}
}

advance(lp, ep)
register unsigned char *lp, *ep;
{
	register unsigned char *curlp;
	unsigned char c;
	unsigned char *bbeg;
	int ct;

	for (;;) switch (*ep++) {

	case CCHR:
		if (*ep++ == *lp++)
			continue;
		return(0);

	case CDOT:
		if (*lp++)
			continue;
		return(0);

	case CDOL:
		if (*lp==0)
			continue;
		return(0);

	case CEOF:
		return(1);

	case CCL:
			/* & 0177 used to be here */
		c = *lp++;
		if(ep[c>>3] & bittab[c & 07]) {
			ep += 16;
			continue;
		}
		return(0);
	case CBRA:
		braslist[*ep++] = lp;
		continue;

	case CKET:
		braelist[*ep++] = lp;
		continue;

	case CBACK:
		bbeg = braslist[*ep];
		if (braelist[*ep]==0)
			return(0);
		ct = braelist[*ep++] - bbeg;
		if(!bcmp(bbeg, lp, ct)) {
			lp += ct;
			continue;
		}
		return(0);

	case CBACK|STAR:
		bbeg = braslist[*ep];
		if (braelist[*ep]==0)
			return(0);
		ct = braelist[*ep++] - bbeg;
		curlp = lp;
		while(!bcmp(bbeg, lp, ct))
			lp += ct;
		while(lp >= curlp) {
			if(advance(lp, ep))	return(1);
			lp -= ct;
		}
		return(0);


	case CDOT|STAR:
		curlp = lp;
		while (*lp++);
		goto star;

	case CCHR|STAR:
		curlp = lp;
		while (*lp++ == *ep);
		ep++;
		goto star;

	case CCL|STAR:
		curlp = lp;
		do {
				/* & 0177 used to be here */
			c = *lp++;
		} while(ep[c>>3] & bittab[c & 07]);
		ep += 16;
		goto star;

	star:
		if(--lp == curlp) {
			continue;
		}

		if(*ep == CCHR) {
			c = ep[1];
			do {
				if(*lp != c)
					continue;
				if(advance(lp, ep))
					return(1);
			} while(lp-- > curlp);
			return(0);
		}

		do {
			if (advance(lp, ep))
				return(1);
		} while (lp-- > curlp);
		return(0);

	case CBRC:
		if (lp == expbuf)
			continue;
#define	uletter(c)	(isalpha(c) || (c) == '_')
		if (uletter(*lp) || isdigit(*lp))
			if (!uletter(lp[-1]) && !isdigit(lp[-1]))
				continue;
		return (0);

	case CLET:
		if (!uletter(*lp) && !isdigit(*lp))
			continue;
		return (0);

	default:
		errexit("grep: Regular Expression error\n", (char *)NULL);
	}
}

succeed(f)
unsigned char *f;
{
	nsucc = 1;
	if (sflag)
		return;
	if (cflag) {
		tln++;
		return;
	}
	if (lflag) {
		printf("%s\n", f);
		fflush(stdout);
		fseek(stdin, 0l, 2);
		return;
	}
	if (nfile > 1 && hflag)
		printf("%s:", f);
	if (bflag) {
		printf("%u:", (ftell(stdin) - strlen(linebuf) - 1)/512);
	}
	if (nflag)
		printf("%ld:", lnum);
	printf("%s\n", linebuf);
}

errexit(s, f)
unsigned char *s, *f;
{
	fprintf(stderr, s, f);
	exit(2);
}
