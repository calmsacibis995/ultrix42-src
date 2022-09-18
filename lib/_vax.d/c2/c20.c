#ifndef lint
static char *sccsid = "@(#)c20.c	4.1	ULTRIX	11/23/87";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1985 by				*
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
 *			Modification History
 *
 * 006  Victoria Holt, 26-Aug-86
 *	Bug fix in remove_branch_to_return() to fix an infinite loop.
 *
 * 005	David Metsky, 13-May-1986
 *	Added code to ignore the -f flag that came from the new fortran
 *	compiler.   Incorperated the code that resulted from the -f flag, 
 *	which uses the correct move length.
 *
 * 004  Victoria Holt, 08-May-86
 *	Added support for .stab directives (see STAB).  Modified
 *	getline() so that it does not interpret a string containing
 *	a colon as a label.
 *	
 * 003	David L Ballenger, 06-Mar-1986
 *	Add code to remove_branch_to_return() to detect empty inifinite
 *	loops, i.e. unconditional branch which branches to itself as in
 *	"for (;;);".
 *
 * 002	David L Ballenger, 25-Feb-1986
 *	Add code to align labels under the appropriate conditions,
 *	remove branches to returns, and uneccessary branches.
 *
 *	David L Ballenger, 3-Jul-1985
 * 001	Allocate optimal size buffers for stdin and stdout.  This can 
 *	make c2 run faster and also fixes problems with interactions 
 *	between c2's allocation routine and stdio's use of malloc().
 *
 *	Based on:  c20.c	4.6 (Berkeley) 8/11/83
 *
 ************************************************************************/

/*
 *	 C object code improver
 */

#include "c2.h"
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>

int ioflag;
long	isn	= 2000000;
struct optab *oplook();
struct optab *getline();
long lgensym[10] =
  {100000L,200000L,300000L,400000L,500000L,600000L,700000L,800000L,900000L,1000000L};

static int
	n_aligns = 0;		/* # of alignments inserted */
static void
	remove_branch_to_return(),
	align_labels();

#define	STDERR_BUFSIZ 132	/* Make stderr line buffered with a buffer
				 * this size. Which should be large enough for
				 * most error messages.
				 */
/* get_buffer()
 *
 * 	Routine to allocate optimal size buffers for files.
 */
static void
get_buffer(file)
	FILE		*file;
{
	struct stat	stat;

	/* Call fstat() to get optimal buffer size for file.  BUFSIZE is
	 * the default.
	 */
	if ( (fstat(fileno(file),&stat) != 0) ||  (stat.st_blksize <= 0) )
		stat.st_blksize = BUFSIZ;

	/* Call setvbuf() to set up buffer.  Pass a NULL buffer pointer
	 * so setvbuf will dynamically allocate the buffer.
	 */
	if (setvbuf(file,NULL,_IOFBF,stat.st_blksize) != 0) {
		fprintf(stderr, "Optimizer: out of space\n");
		exit(1);
	}
}

#define ALLOCSIZE 4096
struct node *
alloc(an)
{
	register int n;
	register char *p;

	n = an;
	n+=sizeof(char *)-1;
	n &= ~(sizeof(char *)-1);
	if (lasta+n >= lastr) {
		if (sbrk(ALLOCSIZE) == -1) {
			fprintf(stderr, "Optimizer: out of space\n");
			exit(1);
		}
		lastr += ALLOCSIZE;
	}
	p = lasta;
	lasta += n;
	return(p);
}

main(argc, argv)
char **argv;
{
	register int niter, maxiter, isend;
	int nflag,infound;

	nflag = 0; infound=0; argc--; argv++;

	/* Make stderr line buffered, so that error messages from
	 * concurrent processes don't get mixed in with ours.
	 */
	setvbuf(stderr,NULL,_IOLBF,STDERR_BUFSIZ);

	while (argc>0) {/* get flags */
		if (**argv=='+') debug++;
		else if (**argv=='-') {
			switch((*argv)[1]) {
				case 'i':
					ioflag++;
					break;
				case 'n':
					nflag++;
					break;
				case 'f':         /* ignore -f  005 */
					break;
				case '\000':
					/* Place holder for input file spec
					 */
					infound++;
					break;
				default:
					fprintf(stderr,
						"C2: invalid option %s\n",
						*argv);
					break;
			}
		} else if (infound==0) {
			if (freopen(*argv, "r", stdin) ==NULL) {
				fprintf(stderr,"C2: can't find %s\n", *argv);
				exit(1);
			}
			++infound;
		} else if (freopen(*argv, "w", stdout) ==NULL) {
			fprintf(stderr,"C2: can't create %s\n", *argv);
			exit(1);
		}
		argc--; argv++;
	}
	/* Set up buffers for stdin and stdout.  Note that this must
	 * be done before setting up the information for c2's own memory
	 * allocation routine.  Stdin and stdout will be fully buffered with
	 * an "optimal" buffer size.
	 */
	get_buffer(stdin);
	get_buffer(stdout);

	/* Set up information for c2's memory allocation scheme.
	 */
	lasta = lastr = sbrk(2);
	opsetup();
	lasta = firstr = lastr = alloc(0);
	maxiter = 0;
	do {
		isend = input();
		niter = 0;
		bmove();
		do {
			refcount();
			do {
				iterate();
				clearreg();
				niter++;
			} while (nchange);
			comjump();
			rmove();
		} while (nchange || jumpsw());
		/* 
		 * Do some final optimizations:
		 * 	- remove branches to returns
		 *	- align certain labels.
		 *	- use ACBx, AOBxxx, and SOBxxx instructions
		 *	  where appopriate
		 *	- get rid of unused labels.
		 */
		remove_branch_to_return();
		align_labels();
		addsob();
		refcount();
		/*
		 * Output the code
		 */
		output();
		if (niter > maxiter)
			maxiter = niter;
		lasta = firstr;
	} while (isend);
	if (nflag) {
		fprintf(stderr,"%d labels aligned\n",n_aligns);
		fprintf(stderr,"%d iterations\n", maxiter);
		fprintf(stderr,"%d jumps to jumps\n", nbrbr);
		fprintf(stderr,"%d inst. after jumps\n", iaftbr);
		fprintf(stderr,"%d jumps to .+1\n", njp1);
		fprintf(stderr,"%d redundant labels\n", nrlab);
		fprintf(stderr,"%d cross-jumps\n", nxjump);
		fprintf(stderr,"%d code motions\n", ncmot);
		fprintf(stderr,"%d branches reversed\n", nrevbr);
		fprintf(stderr,"%d redundant moves\n", redunm);
		fprintf(stderr,"%d simplified addresses\n", nsaddr);
		fprintf(stderr,"%d loops inverted\n", loopiv);
		fprintf(stderr,"%d redundant jumps\n", nredunj);
		fprintf(stderr,"%d common seqs before jmp's\n", ncomj);
		fprintf(stderr,"%d skips over jumps\n", nskip);
		fprintf(stderr,"%d sob's added\n", nsob);
		fprintf(stderr,"%d redundant tst's\n", nrtst);
		fprintf(stderr,"%d jump on bit\n", nbj);
		fprintf(stderr,"%d field operations\n", nfield);
		fprintf(stderr,"%dK core\n", ((unsigned)lastr+01777) >> 10);
	}
	putc('\n',stdout);
	fflush(stdout); exit(0);
}

input()
{
	register struct node *p, *lastp;
	struct optab *op; register char *cp1;
	static struct optab F77JSW = {".long", T(JSW,1)};

	lastp = &first;
	for (;;) {
	  top:
		op = getline();
		if (debug && op==0) fprintf(stderr,"? %s\n",line);
		switch (op->opcode&0377) {
	
		case LABEL:
			p = alloc(sizeof first);
			if (isdigit(line[0]) && (p->labno=locdef(line)) ||
			  (line[0] == 'L') && (p->labno=getnum(line+1))) {
				p->combop = LABEL;
				if (p->labno<100000L && isn<=p->labno) isn=1+p->labno;
				p->code = 0;
			} else {
				p->combop = DLABEL;
				p->labno = 0;
				p->code = copy(line);
			}
			break;
	
		case LGEN:
			if (*curlp!='L' && !locuse(curlp)) goto std;
			op= &F77JSW;
		case JBR:
			if (op->opcode==T(JBR,RET) || op->opcode==T(JBR,RSB)) goto std;
		case CBR:
		case JMP:
		case JSW:
		case SOBGEQ: case SOBGTR: case AOBLEQ: case AOBLSS: case ACB:
			p = alloc(sizeof first);
			p->combop = op->opcode; p->code=0; cp1=curlp;
			if ((!isdigit(*cp1) || 0==(p->labno=locuse(cp1))) &&
			  (*cp1!='L' || 0==(p->labno = getnum(cp1+1)))) {/* jbs, etc.? */
				while (*cp1++); while (*--cp1!=',' && cp1!=curlp);
				if (cp1==curlp ||
				  (!isdigit(*++cp1) || 0==(p->labno=locuse(cp1))) &&
				  (*cp1!='L' || 0==(p->labno=getnum(cp1+1))))
					p->labno = 0;
				else *--cp1=0;
				p->code = copy(curlp);
			}
			if (isn<=p->labno) isn=1+p->labno;
			break;

		case MOVA:
			p=alloc(sizeof first);
			p->combop=op->opcode; p->code=0; cp1=curlp+1;
			if (cp1[-1]=='L' || isdigit(cp1[-1])) {
				while (*cp1++!=','); *--cp1=0;
				if (0!=(p->labno=locuse(curlp)) ||
					0!=(p->labno=getnum(curlp+1))) p->code=copy(cp1+1);
				else {*cp1=','; p->code=copy(curlp);}
			} else {p->code=copy(--cp1); p->labno=0;}
			break;

		case SET:
		case COMM:
		case LCOMM:
		case STAB:
			printf("%s\n",line); 
			goto top;

		case BSS:
		case DATA:
			for (;;) {
				printf("%s%c",line,(op->opcode==LABEL ? ':' : '\n'));
				if (op->opcode==TEXT) goto top;
				if (END==(op=getline())->opcode) {/* dangling .data is bad for you */
					printf(".text\n");
					break;
				}
			}

		std:
		default:
			p = alloc(sizeof first);
			p->combop = op->opcode;
			p->labno = 0;
			p->code = copy(curlp);
			break;

		}
		p->forw = 0;
		p->back = lastp;
		p->pop = op;
		lastp->forw = p;
		lastp = p;
		p->ref = 0;
		if (p->op==CASE) {
			char *lp; int ncase;
			lp=curlp; while (*lp++); while (*--lp!='$'); ncase=getnum(lp+1);
			if (LABEL!=(getline())->opcode) abort(-2);
			do {
				if (WGEN!=(getline())->opcode) abort(-3);
				p = alloc(sizeof first); p->combop = JSW; p->code = 0;
				lp=curlp; while(*lp++!='-'); *--lp=0; p->labno=getnum(curlp+1);
				if (isn<=p->labno) isn=1+p->labno;
				p->forw = 0; p->back = lastp; lastp->forw = p; lastp = p;
				p->ref = 0; p->pop=0;
			} while (--ncase>=0);
		}
		if (op->opcode==EROU)
			return(1);
		if (op->opcode==END)
			return(0);
	}
}

struct optab *
getline()
{
	register char *lp;
	register c;
	Boolean instring;
	static struct optab OPLABEL={"",LABEL};
	static struct optab OPEND={"",END};

	lp = line;
	instring = false;
	while (EOF!=(c=getchar()) && isspace(c));
	while (EOF!=c) {
		if (c == '"') {
		    instring = (instring == true) ? false : true;
		}
		if (c==':' && !instring) {
			*lp++ = 0;
			return(&OPLABEL);
		}
		if (c=='\n') {
			*lp++ = 0;
			return(oplook());
		}
		*lp++ = c;
		c = getchar();
	}
	*lp++ = 0;
	return(&OPEND);
}

long
getnum(p)
register char *p;
{
	register c; int neg; register long n;

	n = 0; neg=0; if (*p=='-') {++neg; ++p;}
	while (isdigit(c = *p++)) {
		 c -= '0'; n *= 10; if (neg) n -= c; else n += c;
	}
	if (*--p != 0)
		return(0);
	return(n);
}

locuse(p)
register char *p;
{
	register c; int neg; register long n;

	if (!isdigit(p[0]) || p[1] != 'f' && p[1] != 'b' || p[2]) return(0);
	return (lgensym[p[0] - '0'] - (p[1] == 'b'));
}

locdef(p)
register char *p;
{

	if (!isdigit(p[0]) || p[1]) return(0);
	return (lgensym[p[0] - '0']++);
}

/* isloop
 *
 * Routine to determine if a label node is the top of a loop.  It does this
 * by searching forward from the label node, and if it finds a branch
 * statement that refers to it then it assumes that it is a loop.
 */
static struct node *
isloop(label)
	register struct node	*label;
{
	register struct node	*next = label;
	register int		branch_seen = 0;

	while (next = next->forw) {
		if ((next->op == CBR) || (next->combop == JBR)) {
			if (next->ref == label) {
				/*
				 * This looks like a loop.
				 */
		   		return(1);
			} else {
				/* Remember that we saw a branch
				 */
				branch_seen = 1;
			}
		} else if ((next->op == JBR) && (branch_seen == 0)) {
			/*
			 * We've hit a RET or RSB, and there is no way
			 * to branch around it. This can't be a loop,
			 * at least not an iterative one.  There may be
			 * a back reference to label somewhere ahead of
			 * this point due to comjump(), but that isn't
			 * the kind of loop we're looking for.
			 */
			return(0);
		}
	}
	return(0);
}

/* align_labels
 *
 * Routine to align labels in what is hoped is a optimal manner, i.e.:
 *
 *	- labels which have a high enough reference count, i.e. the
 *	  first statement after a series of:
 *
 *		if () {}
 *		else if () {}
 *		else if () {}
 *
 *	- labels which are the tops of loops
 *
 * Labels following unconditional branches are not handled here.  The
 * assembler handles them when it decides what do to with jbr and jxxx
 * instructions.  Labels after RETs and RSBs are handled here.
 */
static void
align_labels()
{
	register struct node *prev;
	register struct node *label;

	/* Loop through all the statements looking for labels.
	 */
	prev = &first;
	while (label = prev->forw) {

		if ( (label->op == LABEL) && (prev->combop != JBR) ) {
			/*
			 * Align this label?
			 */
			if ((prev->op == JBR)		/* after RET/RSB */
			    || (label->refc > 2)	/* if else if seq */
			    || (isloop(label))		/* loop */
			    ) {
		   		/* Insert an .align 2 between the
				 * previous statement and the label.
				 * This will longword align the label.
				 */
				register struct node *align;

				align =  alloc(sizeof(*align));
				prev->forw = align;
				align->back = prev;
				align->forw = label;
				label->back = align;
				align->combop = ALIGN;
				if (prev->op == JBR) {
					/* Pad with HALTs
					 */
					align->code = "2";
				} else {
					/* Pad with NOPs since we might
					 * drop through.
					 */
					align->code = "2,1";
				}
				align->pop = 0;
				align->refc = 0;
				align->ref = NULL;
				align->labno = 0;
				prev = align;
				n_aligns++;
			}
		}
		prev = prev->forw;
	}
}

output()
{
	register struct node *t;
	int casebas;

	t = &first;
	while (t = t->forw) switch (t->op) {

	case END:
		fflush(stdout);
		return;

	case LABEL:
		printf("L%d:", t->labno);
		continue;

	case DLABEL:
		printf("%s:", t->code);
		continue;

	case CASE:
		casebas=0;

	default: std:
		if (t->pop==0) {/* must find it */
			register struct optab *p;
			for (p=optab; p->opstring[0]; ++p)
				if (p->opcode==t->combop) {t->pop=p; break;}
		}
		printf("\t%s", t->pop->opstring);
		if (t->code) printf("\t%s", t->code);
		if (t->labno!=0) printf("%cL%d\n",
							(t->code ? ',' : '\t'),
							t->labno);
		else printf("\n");
		continue;

	case MOVA:
		if (t->labno==0) goto std;
		printf("mova%c\tL%d,%s\n","bwlq"[t->subop-BYTE],t->labno,t->code);
		continue;

	case JSW:
		if (t->subop!=0) {/* F77JSW */
			printf(".long\tL%d\n",t->labno); continue;
		}
		if (casebas==0) printf("L%d:\n",casebas=isn++);
		printf(".word	L%d-L%d\n", t->labno, casebas);
		continue;
 	case MOV:				/* 005 */
 		if (t->forw) if(t->forw->op == CBR) goto std;
 		if (*t->code == '$') goto std;
 		if (t->subop == FFLOAT)
 			{
 			printf("movl\t%s\n", t->code);
 			continue;
 			}
 		if (t->subop == DFLOAT || t->subop == GFLOAT)
 			{
 			printf("movq\t%s\n", t->code);
 			continue;
 			}
 		if (t->subop == HFLOAT)
 			{
 			printf("movo\t%s\n", t->code);
 			continue;
 			}
		goto std;

	}
}

char *
copy(ap)
char *ap;
{
	register char *p, *np;
	char *onp;
	register n;
	int na;

	na = nargs();
	p = ap;
	n = 0;
	if (*p==0)
		return(0);
	do
		n++;
	while (*p++);
	if (na>1) {
		p = (&ap)[1];
		while (*p++)
			n++;
	}
	onp = np = alloc(n);
	p = ap;
	while (*np++ = *p++);
	if (na>1) {
		p = (&ap)[1];
		np--;
		while (*np++ = *p++);
	}
	return(onp);
}

#define	OPHS	560
struct optab *ophash[OPHS];

opsetup()
{
	register struct optab *optp, **ophp;
	register int i,t;

	for(i=NREG+5;--i>=0;) regs[i]=alloc(C2_ASIZE);
	for (optp = optab; optp->opstring[0]; optp++) {
		t=7; i=0; while (--t>=0) i+= i+optp->opstring[t];
		ophp = &ophash[i % OPHS];
		while (*ophp++) {
/*			fprintf(stderr,"\ncollision: %d %s %s",
/*				ophp-1-ophash,optp->opstring,(*(ophp-1))->opstring);
*/
			if (ophp > &ophash[OPHS])
				ophp = ophash;
		}
		*--ophp = optp;
	}
}

struct optab *
oplook()
{
	register struct optab *optp,**ophp;
	register char *p,*p2;
	register int t;
	char tempop[20];
	static struct optab OPNULL={"",0};

	for (p=line, p2=tempop; *p && !isspace(*p); *p2++= *p++); *p2=0; p2=p;
	while (isspace(*p2)) ++p2; curlp=p2;
	t=0; while(--p>=line) t += t+*p; ophp = &ophash[t % OPHS];
	while (optp = *ophp) {
		if (equstr(tempop,optp->opstring)) return(optp);
		if ((++ophp) >= &ophash[OPHS]) ophp = ophash;
	}
	curlp = line;
	return(&OPNULL);
}

refcount()
{
	register struct node *p, *lp;
	struct node *labhash[LABHS];
	register struct node **hp;

	for (hp = labhash; hp < &labhash[LABHS];)
		*hp++ = 0;
	for (p = first.forw; p!=0; p = p->forw)
		if (p->op==LABEL) {
			labhash[p->labno % LABHS] = p;
			p->refc = 0;
		}
	for (p = first.forw; p!=0; p = p->forw) {
		if (p->combop==JBR || p->op==CBR || p->op==JSW || p->op==JMP
		  || p->op==SOBGEQ || p->op==SOBGTR || p->op==AOBLEQ || p->op==AOBLSS
		  || p->op==ACB || (p->op==MOVA && p->labno!=0)) {
			p->ref = 0;
			lp = labhash[p->labno % LABHS];
			if (lp==0 || p->labno!=lp->labno)
			for (lp = first.forw; lp!=0; lp = lp->forw) {
				if (lp->op==LABEL && p->labno==lp->labno)
					break;
			}
			if (lp) {
				hp = nonlab(lp)->back;
				if (hp!=lp) {
					p->labno = hp->labno;
					lp = hp;
				}
				p->ref = lp;
				lp->refc++;
			}
		}
	}
	for (p = first.forw; p!=0; p = p->forw)
		if (p->op==LABEL && p->refc==0
		 && (lp = nonlab(p))->op && lp->op!=JSW)
			decref(p);
}

iterate()
{
	register struct node *p, *rp, *p1;

	nchange = 0;
	for (p = first.forw; p!=0; p = p->forw) {

		if ((p->op==JBR||p->op==CBR||p->op==JSW) && p->ref) {
			rp = nonlab(p->ref);
			if (rp->op==JBR && rp->labno && p->labno!=rp->labno) {
				nbrbr++;
				p->labno = rp->labno;
				decref(p->ref);
				rp->ref->refc++;
				p->ref = rp->ref;
				nchange++;
			}
		}
		/* If a conditional branch is followed by an unconditional
		 * branch to the same location, the conditional can be
		 * eliminated.
		 */
		if (p->op==CBR) {
			p1 = nonlab(p->forw);
			if (p1->combop == JBR) {
				if (p->ref == p1->ref) {
					decref(p->ref);
					delnode(p);
					p = p1->back;
					nbrbr++;
					nchange++;
					continue;
				}
			}
		}
#ifndef COPYCODE
		if (p->op==CBR && (p1 = p->forw)->combop==JBR) {/* combop: RET problems */
#else
		if (p->op==CBR && (p1 = p->forw)->combop==JBR &&
		    p->ref) {/* combop: RET problems */
#endif
			rp = p->ref;
			do
				rp = rp->back;
			while (rp->op==LABEL);
			if (rp==p1) {
				decref(p->ref);
				p->ref = p1->ref;
				p->labno = p1->labno;
#ifdef COPYCODE
				if (p->labno == 0)
					p->code = p1->code;
#endif
				p1->forw->back = p;
				p->forw = p1->forw;
				p->subop = revbr[p->subop];
				p->pop=0;
				nchange++;
				nskip++;
			}
		}

		if (p->op==JBR || p->op==JMP) {
			
			while (p->forw && p->forw->op!=LABEL && p->forw->op!=DLABEL
				&& p->forw->op!=EROU && p->forw->op!=END
				&& p->forw->op!=ALIGN
				&& p->forw->op!=0 && p->forw->op!=DATA) {
				nchange++;
				iaftbr++;
				if (p->forw->ref)
					decref(p->forw->ref);
				p->forw = p->forw->forw;
				p->forw->back = p;
			}
			rp = p->forw;
			while (rp && rp->op==LABEL) {
				if (p->ref == rp) {
					p->back->forw = p->forw;
					p->forw->back = p->back;
					p = p->back;
					decref(rp);
					nchange++;
					njp1++;
					break;
				}
				rp = rp->forw;
			}
			xjump(p);
			p = codemove(p);
		}
	}
}

/* remove_branch_to_return
 *
 * Routine to remove uncontional branches which just branch to a
 * return. They are just replaced with a return.
 */
static void
remove_branch_to_return()
{
	register struct node	
		*ptr,		/* Pointer to current instruction */
		*dest,		/* destination instruction for branch */
		*nextdest;	/* temporary for comparison */
	int count;

	for ( ptr = first.forw; ptr != NULL; ptr = ptr->forw ) {
		if (ptr->combop == JBR || ptr->combop==JMP) {
			
			/* We have an unconditional jump/branch to a
			 * label.  Find the instruction after the
			 * label(s), and if it is a return, convert 
			 * the jump/branch to the appropriate
			 * return and decrease the reference to the
			 * label.
			 */

			/*
			 * Search through a list of unconditional
			 * branches.  These should have been 
			 * eliminated, but just in case.
			 */
			nextdest = ptr;
			dest = nonlab(ptr->ref);
			while ((dest != nextdest)	/* L1:  jmp L1 */
			    && (dest->combop == JBR
			    ||  dest->combop == JMP)) {
				nextdest = dest;
				dest = nonlab(dest->ref);
			}
			if ((dest->combop == T(JBR,RET))
			    || (dest->combop == T(JBR,RSB))
			   ) {
			   	/* The unconditional branch/jump is to a
				 * return, so just change it to a return.
				 */
				ptr->labno = 0;
				ptr->combop = dest->combop;
				ptr->pop = dest->pop;
				decref(ptr->ref);
				ptr->ref = NULL;
				nbrbr++;
			}
		}

	}
}

xjump(p1)
register struct node *p1;
{
	register struct node *p2, *p3;
	int nxj;

	nxj = 0;
	if ((p2 = p1->ref)==0)
		return(0);
	for (;;) {
		while ((p1 = p1->back) && p1->op==LABEL);
		while ((p2 = p2->back) && p2->op==LABEL);
		if (!equop(p1, p2) || p1==p2)
			return(nxj);
		p3 = insertl(p2);
		p1->combop = JBR;
		p1->pop=0;
		p1->ref = p3;
		p1->labno = p3->labno;
		p1->code = 0;
		nxj++;
		nxjump++;
		nchange++;
	}
}

struct node *
insertl(op)
register struct node *op;
{
	register struct node *lp;

	if (op->op == LABEL) {
		op->refc++;
		return(op);
	}
	if (op->back->op == LABEL) {
		op = op->back;
		op->refc++;
		return(op);
	}
	lp = alloc(sizeof first);
	lp->combop = LABEL;
	lp->labno = isn++;
	lp->ref = 0;
	lp->code = 0;
	lp->refc = 1;
	lp->back = op->back;
	lp->forw = op;
	op->back->forw = lp;
	op->back = lp;
	return(lp);
}

struct node *
codemove(ap)
struct node *ap;
{
	register struct node *p1, *p2, *p3;
	struct node *t, *tl;
	int n;

	p1 = ap;
/* last clause to avoid infinite loop on partial compiler droppings:
	L183:	jbr L179
	L191:	jbr L179
			casel r0,$0,$1
	L193:	.word L183-L193
			.word L191-L193
	L179:	ret
*/
	if (p1->op!=JBR || (p2 = p1->ref)==0 || p2==p1->forw)
		return(p1);
	while (p2->op == LABEL)
		if ((p2 = p2->back) == 0)
			return(p1);
	if (p2->op!=JBR && p2->op!=JMP)
		goto ivloop;
	p2 = p2->forw;
	p3 = p1->ref;
	while (p3) {
		if (p3->op==JBR || p3->op==JMP) {
			if (p1==p3)
				return(p1);
			ncmot++;
			nchange++;
			p1->back->forw = p2;
			p1->forw->back = p3;
			p2->back->forw = p3->forw;
			p3->forw->back = p2->back;
			p2->back = p1->back;
			p3->forw = p1->forw;
			decref(p1->ref);
			return(p2);
		} else
			p3 = p3->forw;
	}
	return(p1);
ivloop:
	if (p1->forw->op!=LABEL)
		return(p1);
	p3 = p2 = p2->forw;
	n = 16;
	do {
		if ((p3 = p3->forw) == 0 || p3==p1 || --n==0)
			return(p1);
	} while (p3->op!=CBR || p3->labno!=p1->forw->labno);
	do 
		if ((p1 = p1->back) == 0)
			return(ap);
	while (p1!=p3);
	p1 = ap;
	tl = insertl(p1);
	p3->subop = revbr[p3->subop];
	p3->pop=0;
	decref(p3->ref);
	p2->back->forw = p1;
	p3->forw->back = p1;
	p1->back->forw = p2;
	p1->forw->back = p3;
	t = p1->back;
	p1->back = p2->back;
	p2->back = t;
	t = p1->forw;
	p1->forw = p3->forw;
	p3->forw = t;
	p2 = insertl(p1->forw);
	p3->labno = p2->labno;
#ifdef COPYCODE
	if (p3->labno == 0)
		p3->code = p2->code;
#endif
	p3->ref = p2;
	decref(tl);
	if (tl->refc<=0)
		nrlab--;
	loopiv++;
	nchange++;
	return(p3);
}

comjump()
{
	register struct node *p1, *p2, *p3;

	for (p1 = first.forw; p1!=0; p1 = p1->forw)
		if (p1->op==JBR && ((p2 = p1->ref) && p2->refc > 1
				|| p1->subop==RET || p1->subop==RSB))
			for (p3 = p1->forw; p3!=0; p3 = p3->forw)
				if (p3->op==JBR && p3->ref == p2)
					backjmp(p1, p3);
}

backjmp(ap1, ap2)
struct node *ap1, *ap2;
{
	register struct node *p1, *p2, *p3;

	p1 = ap1;
	p2 = ap2;
	for(;;) {
		while ((p1 = p1->back) && p1->op==LABEL);
		p2 = p2->back;
		if (equop(p1, p2)) {
			p3 = insertl(p1);
			p2->back->forw = p2->forw;
			p2->forw->back = p2->back;
			p2 = p2->forw;
			decref(p2->ref);
			p2->combop = JBR; /* to handle RET */
			p2->pop=0;
			p2->labno = p3->labno;
#ifdef COPYCODE
			p2->code = 0;
#endif
			p2->ref = p3;
			nchange++;
			ncomj++;
		} else
			return;
	}
}
