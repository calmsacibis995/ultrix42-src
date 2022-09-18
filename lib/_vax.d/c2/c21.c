#ifndef lint
static char *sccsid = "@(#)c21.c	4.1	ULTRIX	7/3/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1984-1989 by			*
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
 *                     Modification History                             *
 *									*
 * 0008 Jon Reeves, 01-Mar-1989						*
 *	Change dest() to invalidate wild stores sooner; fixed		*
 *	bypass of test when it immediately followed a store to -n(fp)	*
 *	(Change from 4.3 code.)						*
 *									*
 * 0007 David L Ballenger, 10-Jun-1986					*
 *	Fix bug in redunbr() which didn't completely update the	label	*
 *	information when changing a branch to go to a new label.	*
 *	This could cause the branch to go to an undefined label.	*
 *									*
 * 0006	David L Ballenger, 02-Mar-1986					*
 *	Do further corrections on the bitopt() routine fixed previously	*
 *	do correctly distinguish when bixx operations can be optimized. *
 *	The previous fix introduced problems with index mode.		*
 *									*
 * 0005	David L Ballenger, 24-Feb-1986					*
 *	Fix problem reported by SPR ICA-679 in which a bitb was		*
 *	incorrectly being converted to jlbc.  Also fix problem		*
 *	found in the QVSS driver in which a jbs/jbc was incorrectly	*
 *	converted into a jbss/jbcc.					*
 *									*
 *	David L. Ballenger, 06-Nov-1984					*
 * 0004 Modify byondrd() to handle MFPR instructions correctly, i.e.	*
 *	to recognize that it is a 2 operand instruction.		*
 *									*
 * 	David L. Ballenger, 10-Oct-1984					*
 * 0003 Modify compat() so that GFLOAT, HFLOAT, and OCTA types are	*
 *	handled correctly, specifically so that GFLOAT and FFLOAT are	*
 *	correctly determined to be different types.			*
 *                                                                      *
 *	David L. Ballenger, 19-Jul-1984					*
 * 0002 Fix problems in bicopt with optimizing BICL and preceding	*
 *	instructions.							*
 *	- don't allow a CVT of a float type preceding a BICL to be	*
 *	  deleted.							*
 *	- don't allow a CVT or MOVZ with an auto(in|de)cremented source *
 *	  to be delete unless the source of the instruction replacing	*
 *	  it is the same size.						*
 *	- don't replace a BICL with an EXTZV unless a preceding ASHL	*
 *	  and/or CVT/MOVZ was deleted.					*
 *									*
 *	David L. Ballenger, 24-May-1984					*
 * 0001	Fix problems with converting EXT[Z]V instructions to CVT{B|W}L	*
 *	or MOVZ{B|W}L instructions.  An extract with a bit offset of 16	*
 *	and length 16 was not being converted with the proper alignment.*
 *	Also an extract with an offset of 8 and length 16 was not being	*
 *	converted at all.						*
 *                                                                      *
 ************************************************************************/

/*
 * C object code improver-- second part
 */

#include "c2.h"
#include <stdio.h>
#include <ctype.h>
#include <varargs.h>
#include <strings.h>

#define NUSE 6
#define BITS_PER_UNIT 8		/* The bits per addressable unit is used to
				 * calculate the addressable unit offset
				 * for converting EXT instructions to CVT
				 * or MOVZ instructions.    		0001
				 */
int ioflag;
int biti[NUSE] = {1,2,4,8,16,32};
int bitsize[] = {	/* index by type codes */
	0,		/* 0	not allocated */
	8,		/* 1	BYTE */
	16,		/* 2	WORD */
	32,		/* 3	LONG */
	32,		/* 4	FFLOAT */
	64,		/* 5	DFLOAT */
	64,		/* 6	QUAD */
	0,		/* 7	OP2 */
	0,		/* 8	OP3 */
	0,		/* 9	OPB */
	0,		/* 10	OPX */
	64,		/* 11	GFLOAT */
	128,		/* 12	HFLOAT */
	128		/* 13	OCTA */
};
int pos,siz; long f; /* for bit field communication */
struct node *uses[NUSE]; /* for backwards flow analysis */
char *lastrand; /* last operand of instruction */
struct node *bflow();
struct node *bicopt();
char *findcon();

/* new_operands
 *
 * Routine to reformat the new operands when an instruction or instruction
 * sequence is optimized.  Accepts a variable list of 2 or more arguments.
 * The first argument is the pointer to the node which is getting the new
 * operands, the second argument is the format string and the rest of the
 * arguments are as specified in the format string as in printf().
 *
 */
static void
new_operands(va_alist)
	va_dcl
{
	va_list		args;
	register struct node	*ptr;
	char			*format;
	char			operands[C2_ASIZE];
	register int		n_chars;

	va_start(args);
	/*
	 * Get the node pointer and format, then call vsprintf() with
	 * the format and the remaining arguments.
	 */
	ptr = va_arg(args,struct node *);
	format = va_arg(args,char *);
	n_chars = vsprintf(operands,format,args);
	if (n_chars > C2_ASIZE) {
		fputs("c2: buffer overflow in new_operands()\n",stderr);
		exit(1);
	}
	/* Allocate a buffer for the string, copy the new operands to it
	 * and return a pointer to it.
	 */
	ptr->code = alloc(n_chars+1);
	(void)strcpy(ptr->code,operands);
}

redun3(p,split) register struct node *p; int split; {
/* check for 3 addr instr which should be 2 addr */
	if (OP3==((p->subop>>4)&0xF)) {
		if (split) splitrand(p);
		if (equstr(regs[RT1],regs[RT3])
		  && (p->op==ADD || p->op==MUL || p->op==BIS || p->op==XOR)) {
			register char *t=regs[RT1]; regs[RT1]=regs[RT2]; regs[RT2]=t;
		}
		if (equstr(regs[RT2],regs[RT3])) {
			p->subop=(p->subop&0xF)|(OP2<<4); p->pop=0;
			lastrand=regs[RT2]; *regs[RT3]=0; return(1);
		}
	} return(0);
}

bmove() {
	register struct node *p, *lastp; register char *cp1,*cp2; register int r;
	refcount();
	for (p=lastp= &first; 0!=(p=p->forw); lastp=p);
	clearreg(); clearuse();
	for (p=lastp; p!= &first; p=p->back) {
	if (debug) {
		printf("Uses:\n");
		for (r=NUSE;--r>=0;) if (uses[r])
			printf("%d: %s\n",r,uses[r]->code? uses[r]->code:"");
		printf("-\n");
	}
	r=(p->subop>>4)&0xF;
	if (OP2==r && (cp1=p->code, *cp1++)=='$' && *cp1++=='0' && *cp1++==',' &&
			!source(cp1)) {/* a no-op unless MUL or DIV */
		if (p->op==MUL) {p->op=MOV; p->subop&=0xF; p->pop=0;}
		else if (p->op==DIV) fprintf(stderr,"c2: zero divide\n");
		else {delnode(p); redunm++; continue;}
	}
	if (OP3==r && 0!=redun3(p,1)) {newcode(p); redunm++;}
	switch (p->op) {
	case LABEL: case DLABEL:
		for (r=NUSE; --r>=0;)
			if (uses[r]) p->ref=(struct node *) (((int)p->ref)|biti[r]);
		break;
	case CALLS:
		clearuse(); goto std;
	case 0:
		clearuse(); break;
	case SUB:
		if ((p->subop&0xF)!=LONG) goto std; cp1=p->code;
		if (*cp1++!='$') goto std; splitrand(p);
		if (equstr(regs[RT2],"fp") && !indexa(regs[RT1])) {/* address comp. */
			char buf[C2_ASIZE]; cp2=buf; *cp2++='-'; 
			cp1=regs[RT1]+1; while (*cp2++= *cp1++); --cp2;
			cp1="(fp),"; while (*cp2++= *cp1++); --cp2;
			cp1=regs[RT3]; while (*cp2++= *cp1++);
			p->code=copy(buf); p->combop=T(MOVA,LONG); p->pop=0;
		} else if (*cp1++=='-' && 0<=(r=getnum(cp1))) {
			p->op=ADD; p->pop=0; *--cp1='$'; p->code=cp1;
		} goto std;
	case ADD:
		if ((p->subop&0xF)!=LONG) goto std; cp1=p->code;
		if (*cp1++!='$') goto std; splitrand(p);
		if (isstatic(cp1) && (r=isreg(regs[RT2]))>=0 && r<NUSE && uses[r]==p->forw)
		{
			/* address comp:
			**	addl2	$_foo,r0  \	movab	_foo[r0],bar
			**	movl	r0,bar	  /
			*/
			register struct	node	*pnext = p->forw;
			char	buf[C2_ASIZE];

			if (pnext->op == MOV && pnext->subop == LONG)
			{
				cp1 = &regs[RT1][1]; cp2 = &buf[0];
				while (*cp2++ = *cp1++) ; cp2--;
				splitrand(pnext);
				if (r == isreg(regs[RT1]))
				{
					delnode(p); p = pnext;
					p->op = MOVA; p->subop = BYTE;
					p->pop = 0;
					cp1 = regs[RT1]; *cp2++ = '[';
					while (*cp2++ = *cp1++) ; cp2--;
					*cp2++ = ']'; *cp2++ = ',';
					cp1 = regs[RT2];
					while (*cp2++ = *cp1++) ;
					p->code = copy(buf);
				}
			}
		}
		else
		if (equstr(regs[RT2],"fp") && !indexa(regs[RT1])) {/* address comp. */
			cp2=cp1-1; cp1=regs[RT1]+1; while (*cp2++= *cp1++); --cp2;
			cp1="(fp)"; while (*cp2++= *cp1++); *--cp2=',';
			p->combop=T(MOVA,LONG); p->pop=0;
		} else if (*cp1++=='-' && 0<=(r=getnum(cp1))) {
			p->op=SUB; p->pop=0; *--cp1='$'; p->code=cp1;
		}
		/* fall thru ... */
	case CASE:
	default: std:
		p=bflow(p); break;
	case MUL:
	{
		/*
		** Change multiplication by constant powers of 2 to
		**	shifts.
		*/
		splitrand(p);
		if (regs[RT1][0] != '$' || regs[RT1][1] == '-') goto std;
		if ((r = ispow2(getnum(&regs[RT1][1]))) < 0) goto std;
		switch (r)
		{
		case 0:		/* mull3 $1,x,y */
			if (p->subop == U(LONG,OP3))
			{
				if (equstr(regs[RT2], regs[RT3]))
				{
					delnode(p); p = p->forw;
				}
				else
				{
					p->op = MOV; p->subop = LONG;
					p->pop = 0; newcode(p); nchange++;
				}
			}
			else
			if (p->subop == U(LONG,OP2))
			{
				delnode(p); p = p->forw;
			}
			goto std;

		case 1:		/* mull2 $2,x */
			if (p->subop == U(LONG, OP2) && !source(regs[RT2]))
			{
				strcpy(regs[RT1], regs[RT2]);
				p->op = ADD; p->pop = 0; newcode(p); nchange++;
			}
			goto std;
		}
		if(p->subop==U(LONG,OP3)||(p->subop==U(LONG,OP2)&&!source(regs[RT2])))
		{
			if (p->subop == U(LONG,OP2))
				strcpy(regs[RT3], regs[RT2]);
			sprintf(regs[RT1], "$%d", r);
			p->op = ASH; p->subop = LONG;
			p->pop = 0; newcode(p); nchange++;
		}
		goto std;
	}
	case ASH:
	{
		/* address comp:
		**	ashl	$1,bar,r0  \	movl	bar,r0
		**	movab	_foo[r0]   /	movaw	_foo[r0]
		**
		**	ashl	$2,r0,r0   \	moval	_foo[r0]
		**	movab	_foo[r0]   /
		*/
		register struct	node	*pf;
		register int	shfrom, shto;
		long	shcnt;
		char	*regfrom;

		splitrand(p);
		if (regs[RT1][0] != '$') goto std;
		if ((shcnt = getnum(&regs[RT1][1])) < 1 || shcnt > 3) goto std;
		if ((shfrom = isreg(regs[RT2])) >= 0)
			regfrom = copy(regs[RT2],"]");
		if ((shto = isreg(regs[RT3])) >= 0 && shto<NUSE)
		{
			int	regnum;

			if (uses[shto] != (pf = p->forw)) goto ashadd;
			if (pf->op != MOVA && pf->op != PUSHA) goto ashadd;
			if (pf->subop != BYTE) goto ashadd;
			splitrand(pf);
			if (!indexa(regs[RT1])) goto std;
			cp2 = regs[RT1];
			if(!isstatic(cp2)) goto std;
			while (*cp2++ != '[') ;
			if (*cp2++ != 'r' || !isdigit(*cp2)) goto std;
			regnum = *cp2++ - '0';
			if (isdigit(*cp2))
			{
				if (cp2[1] != ']') goto std;
				regnum *= 10; regnum += *cp2 - '0';
			}
			if (regnum != shto) goto std;
			if (shfrom >= 0)	/* ashl $N,r*,r0 */
			{
				delnode(p);
				if (shfrom != shto)
				{
					uses[shto] = NULL; splitrand(pf);
					cp2=regs[RT1]; while (*cp2++!='[');
					cp1=regfrom; while (*cp2++= *cp1++);
					newcode(pf);
				}
			}
			else
			{
				p->op = MOV; splitrand(p);
				strcpy(regs[RT1], regs[RT2]);
				strcpy(regs[RT2], regs[RT3]);
				regs[RT3][0] = '\0';
				p->pop = 0; newcode(p);
			}
			switch (shcnt)
			{
			case 1:	pf->subop = WORD; break;
			case 2:	pf->subop = LONG; break;
			case 3:	pf->subop = QUAD; break;
			}
			redunm++; nsaddr++; nchange++;
		}
		goto std;
ashadd:
		/* at this point, RT2 and RT3 are guaranteed to be simple regs*/
		if (shcnt == 1 && equstr(regs[RT2], regs[RT3]))
		{
			/*
			** quickie:
			**	ashl	$1,A,A	>	addl2	A,A
			*/
			p->op = ADD; p->subop = U(LONG,OP2); p->pop = 0;
			strcpy(regs[RT1], regs[RT2]); regs[RT3][0] = '\0';
			newcode(p); nchange++;
		}
		goto std;
	}

	case EXTV:
	case EXTZV:
	{
		/* bit tests:
		**	extv	A,$1,B,rC  \
		**	tstl	rC	    >	jbc	A,B,D
		**	jeql	D	   /
		**
		** also byte- and word-size fields:
		**	extv	$n*8,$8,A,B	>	cvtbl	n+A,B
		**	extv	$n*16,$16,A,B	>	cvtwl	n+A,B
		**	extzv	$n*8,$8,A,B	>	movzbl	n+A,B
		**	extzv	$n*16,$16,A,B	>	movzwl	n+A,B
		*/
		register struct	node	*pf;	/* forward node */
		register struct	node	*pn;	/* next node (after pf) */
		int	flen;			/* field length */

		splitrand(p);
		if (regs[RT2][0] != '$') goto std;
		if ((flen = getnum(&regs[RT2][1])) < 0) goto std;
		if (flen == 1)
		{
			register int	extreg;		/* reg extracted to */

			extreg = isreg(regs[RT4]);
			if (extreg < 0 || extreg >= NUSE) goto std;
			if ((pf = p->forw)->op != TST) goto std;
			if (uses[extreg] && uses[extreg] != pf) goto std;
			splitrand(pf);
			if (extreg != isreg(regs[RT1])) goto std;
			if ((pn = pf->forw)->op != CBR) goto std;
			if (pn->subop != JEQ && pn->subop != JNE) goto std;
			delnode(p); delnode(pf);
			pn->subop = (pn->subop == JEQ) ? JBC : JBS;
			for(cp2=p->code; *cp2++!=',';);
			for(cp1=cp2;     *cp1++!=',';);
			while (*cp1!=',') *cp2++= *cp1++; *cp2='\0';
			pn->code = p->code; pn->pop = NULL;
			uses[extreg] = NULL;
		}
		else
		if (flen == 8 || flen == 16)
		{
			/* If the extract is a byte or word length we may be
			 * able to change it to a CVT or MOVZ instruction.
			 */

			register int	boff;	/* bit offset */
			register int	coff;	/* chunk (byte or word) offset*/

			/* If the bit offset is not immediate mode (i.e. $16)
			 * then we can't convert it.
			 */
			if (regs[RT1][0] != '$') goto std;

			/* Get the value of the bit offset and if it is < 0
			 * then we can't change it.
			 */
			if ((boff = getnum(&regs[RT1][1])) < 0) goto std;

			/* If the bit offset is not on an addressable unit 
			 * boundary, then we can't change it
			 */
			if ((boff % BITS_PER_UNIT) != 0) goto std; /*0001*/

			/* Calculate the chunk (addressable unit offset)
			 */
			coff = boff / BITS_PER_UNIT ;		   /*0001*/

			/* If the chunk is not zero and the extract is from
			 * a register, then we can't change it since the
			 * CVT or MOVZ can only act on the low byte or word
			 * of the register. The function isreg returns the
			 * register number or -1 if the argument is not a
			 * register.
			 */
			if (coff && (isreg(regs[RT3]) >= 0)) goto std;

			/* Determine whether to do a signed (CVT) or unsigned
			 * MOVZ operation and if the source is a byte or a
			 * word.
			 */
			p->op = (p->op == EXTV) ? CVT : MOVZ;
			p->subop = U((flen == 8 ? BYTE : WORD), LONG);

			/* Set up the source operand string in regs[RT1]
			 */
			if (coff == 0)
				/* If the addressable offset is 0 then just
				 * copy the source from RT3.
				 */
				strcpy(regs[RT1], regs[RT3]);
			else
				/* Change the source operand string to use the
				 * chunk.  If the source uses general register
				 * addressing generate "coff(Rn)" otherwise 
				 * generate "coff+address".
				 */
				sprintf(regs[RT1], "%d%s%s", coff, 
					regs[RT3][0]=='(' ? "":"+",
					regs[RT3]);

			/* Move the destination from RT4 to RT2 and set the
			 * RT3 and RT4 to null strings since we no longer need
			 * them to generate the instruction.
			 */
			strcpy(regs[RT2], regs[RT4]);
			regs[RT3][0] = '\0'; regs[RT4][0] = '\0';
			p->pop = 0; newcode(p);
		}
		nchange++;
		goto std;
	}

	case CMP:
	{
		/* comparison to -63 to -1:
		**	cmpl	r0,$-1	>	incl	r0
		**	jeql	...
		**
		**	cmpl	r0,$-63	>	addl2	$63,r0
		**	jeql	...
		*/
		register int	num;
		register int	reg;
		register struct	node	*regp = p->back;

		if (p->forw->op != CBR) goto std;
		if (p->forw->subop != JEQ && p->forw->subop != JNE) goto std;
		splitrand(p);
		if (strncmp(regs[RT2], "$-", 2) != 0) goto std;
		reg = r = isreg(regs[RT1]);
		if (r < 0) goto std;
		if (r < NUSE && uses[r] != 0) goto std;
		if (r >= NUSE && regp->op == MOV && p->subop == regp->subop)
		{
			if (*regp->code != 'r') goto std;
			reg = regp->code[1] - '0';
			if (isdigit(regp->code[2]) || reg >= NUSE || uses[reg])
				goto std;
		}
		if (r >= NUSE) goto std;
		if (reg != r)
			sprintf(regs[RT1], "r%d", reg);
		if ((num = getnum(&regs[RT2][2])) <= 0 || num > 63) goto std;
		if (num == 1)
		{
			p->op = INC; regs[RT2][0] = '\0';
		}
		else
		{
			register char	*t;

			t=regs[RT1];regs[RT1]=regs[RT2];regs[RT2]=t;
			p->op = ADD; p->subop = U(p->subop, OP2);
			for (t = &regs[RT1][2]; t[-1] = *t; t++) ;
		}
		p->pop = 0; newcode(p);
		nchange++;
		goto std;
	}

	case JSB:
		if (equstr(p->code,"mcount")) {uses[0]=p; regs[0][0]= -1;}
		goto std;
	case JBR: case JMP:
		clearuse();
		if (p->subop==RET || p->subop==RSB) {uses[0]=p; regs[0][0]= -1; break;}
		if (p->ref==0) goto std;	/* jmp (r0) */
		/* fall through */
	case CBR:
		if (p->ref->ref!=0) for (r=NUSE;--r>=0;)
			if (biti[r] & (int)p->ref->ref) {uses[r]=p; regs[r][0]= -1;}
	case EROU: case JSW:
	case TEXT: case DATA: case BSS: case ALIGN: case WGEN: case END: ;
	}
	}
	for (p= &first; p!=0; p=p->forw)
		if (p->op==LABEL || p->op==DLABEL) p->ref=0;	/* erase our tracks */
}

rmove()
{
	register struct node *p, *lastp;
	register int r;
	int r1;

	clearreg();
	for (p=first.forw; p!=0; p = p->forw) {
	lastp=p;
	if (debug) {
		printf("Regs:\n");
		for (r=0; r<NREG; r++)
			if (regs[r][0]) {
				r1=regs[r][0];
				printf("%d: %d%d %s\n", r, r1&0xF, r1>>4, regs[r]+1);
			}
		printf("-\n");
	}
	switch (p->op) {

	case CVT:
		splitrand(p); goto mov;

	case MOV:
		splitrand(p);
		if ((r = findrand(regs[RT1],p->subop)) >= 0) {
			if (r == isreg(regs[RT2]) && p->forw->op!=CBR) {
				delnode(p); redunm++; break;
			}
		}
mov:
		repladdr(p);
		r = isreg(regs[RT1]);
		r1 = isreg(regs[RT2]);
		dest(regs[RT2],p->subop);
		if (r>=0) {
			if (r1>=0) savereg(r1, regs[r]+1, p->subop);
			else if (p->op!=CVT) savereg(r, regs[RT2], p->subop);
		} else if (r1>=0) savereg(r1, regs[RT1], p->subop);
		else if (p->op!=CVT) setcon(regs[RT1], regs[RT2], p->subop);
		break;

/* .rx,.wx */
	case MFPR:
	case COM:
	case NEG:
/* .rx,.wx or .rx,.rx,.wx */
	case ADD:
	case SUB:
	case BIC:
	case BIS:
	case XOR:
	case MUL:
	case DIV:
	case ASH:
	case MOVZ:
/* .rx,.rx,.rx,.wx */
	case EXTV:
	case EXTZV:
	case INSV:
		splitrand(p);
		repladdr(p);
		dest(lastrand,p->subop);
		if (p->op==INSV) ccloc[0]=0;
		break;

/* .mx or .wx */
	case CLR:
	case INC:
	case DEC:
		splitrand(p);
		dest(lastrand,p->subop);
		if (p->op==CLR)
			if ((r = isreg(regs[RT1])) >= 0)
				savereg(r, "$0", p->subop);
			else
				setcon("$0", regs[RT1], p->subop);
		break;

/* .rx */
	case TST:
	case PUSH:
		splitrand(p);
		lastrand=regs[RT1+1]; /* fool repladdr into doing 1 operand */
		repladdr(p);
		if (p->op==TST && equstr(lastrand=regs[RT1], ccloc+1)
		  && ((0xf&(ccloc[0]>>4))==p->subop || equtype(ccloc[0],p->subop))
		  &&!source(lastrand)) {
			delnode(p); p = p->back; nrtst++; nchange++;
		}
		setcc(lastrand,p->subop);
		break;

/* .rx,.rx,.rx */
	case PROBER:
	case PROBEW:
	case CASE:
	case MOVC3:
/* .rx,.rx */
	case MTPR:
	case CALLS:
	case CMP:
	case BIT:
		splitrand(p);
		/* fool repladdr into doing right number of operands */
		if (p->op==CASE || p->op==PROBER || p->op==PROBEW) lastrand=regs[RT4];
/*		else if (p->op==CMPV || p->op==CMPZV) lastrand=regs[RT4+1]; */
		else if (p->op==MOVC3) lastrand=regs[RT1];
		else lastrand=regs[RT3];
		repladdr(p);
		if (p->op==CALLS || p->op==MOVC3) clearreg();
		if (p->op==BIT) bitopt(p);
		ccloc[0]=0; break;

	case CBR:
		if (p->subop>=JBC) {
			splitrand(p);
			if (p->subop<JBCC) lastrand=regs[RT3]; /* 2 operands can be optimized */
			else lastrand=regs[RT2]; /* .mb destinations lose */
			repladdr(p);
		}
		ccloc[0] = 0;
		break;

	case JBR:
		redunbr(p);

/* .wx,.bb */
	case SOB:

	default:
		clearreg();
	}
	}
}

char *byondrd(p) 

/* Return pointer to register which is "beyond last read/modify operand"
 */
	register struct node *p;
{
	if (OP2==(p->subop>>4))
		return(regs[RT3]);
	switch (p->op) {
		
		/* 1 operand instructions
		 */
		case JSB:
		case TST:
		case INC:
		case DEC:
		case PUSHA:
		case PUSH:	return(regs[RT2]);
		
		/* 2 operand instructions 
		 */
		case MFPR:	/* DLB0004 */
		case MTPR:
		case BIT:
		case CMP:
		case CALLS:	return(regs[RT3]);

		/* 3 operand instructions
		 */
		case PROBER:	
		case PROBEW:
		case CASE:
		case MOVC3:	return(regs[RT4]);
	}
	return(lastrand);
}

struct node *
bflow(p)
register struct node *p;
{
	register char *cp1,*cp2,**preg; register int r;
	int flow= -1;
	struct node *olduse=0;
	splitrand(p);
	if (p->op!=PUSH && p->subop && 0<=(r=isreg(lastrand)) && r<NUSE && uses[r]==p->forw) {
	if (equtype(p->subop,regs[r][0])
	|| ((p->op==CVT || p->op==MOVZ)
			 && 0xf&regs[r][0] && compat(0xf&(p->subop>>4),regs[r][0]))) {
		register int r2;
		if (regs[r][1]!=0) {/* send directly to destination */
			if (p->op==INC || p->op==DEC) {
				if (p->op==DEC) p->op=SUB; else p->op=ADD;
				p->subop=(OP2<<4)+(p->subop&0xF); /* use 2 now, convert to 3 later */
				p->pop=0;
				cp1=lastrand; cp2=regs[RT2]; while (*cp2++= *cp1++); /* copy reg */
				cp1=lastrand; *cp1++='$'; *cp1++='1'; *cp1=0;
			}
			cp1=regs[r]+1; cp2=lastrand;
			if (OP2==(p->subop>>4)) {/* use 3 operand form of instruction */
				p->pop=0;
				p->subop += (OP3-OP2)<<4; lastrand=cp2=regs[RT3];
			}
			while (*cp2++= *cp1++);
			if (p->op==MOVA && p->forw->op==PUSH) {
				p->op=PUSHA; *regs[RT2]=0; p->pop=0;
			} else if (p->op==MOV && p->forw->op==PUSH) {
				p->op=PUSH ; *regs[RT2]=0; p->pop=0;
			}
			delnode(p->forw);
			if (0<=(r2=isreg(lastrand)) && r2<NUSE) {
				uses[r2]=uses[r]; uses[r]=0;
			}
			redun3(p,0);
			newcode(p); redunm++; flow=r;
		} else if (p->op==MOV && p->forw->op!=EXTV && p->forw->op!=EXTZV) {
			/* superfluous fetch */
			int nmatch;
			char src[C2_ASIZE];
	movit:
			cp2=src; cp1=regs[RT1]; while (*cp2++= *cp1++);
			splitrand(p->forw);
			if (p->forw->op != INC && p->forw->op != DEC)
				lastrand=byondrd(p->forw);
			nmatch=0;
			for (preg=regs+RT1;*preg!=lastrand;preg++)
				if (r==isreg(*preg)) {
				cp2= *preg; cp1=src; while (*cp2++= *cp1++); ++nmatch;
			}
			if (nmatch==1) {
				if (OP2==(p->forw->subop>>4) && equstr(src,regs[RT2])) {
					p->forw->pop=0;
					p->forw->subop += (OP3-OP2)<<4; cp1=regs[RT3];
					*cp1++='r'; *cp1++=r+'0'; *cp1=0;
				}
				delnode(p); p=p->forw;
				if (0<=(r2=isreg(src)) && r2<NUSE) {
					uses[r2]=uses[r]; uses[r]=0;
				}
				redun3(p,0);
				newcode(p); redunm++; flow=r;
			} else splitrand(p);
		}
	} else if (p->op==MOV && (p->forw->op==CVT || p->forw->op==MOVZ)
		&& p->forw->subop&0xf 	/* if base or index, then forget it */
		&& compat(p->subop,p->forw->subop) && !source(cp1=regs[RT1])
		&& !indexa(cp1)) goto movit;
	}
	/* adjust 'lastrand' past any 'read' or 'modify' operands. */
	lastrand=byondrd(p);
	/* a 'write' clobbers the register. */
	if (0<=(r=isreg(lastrand)) && r<NUSE
	|| OP2==(p->subop>>4) && 0<=(r=isreg(regs[RT2])) && r<NUSE && uses[r]==0) {
		/* writing a dead register is useless, but watch side effects */
		switch (p->op) {
		case ACB:
		case AOBLEQ: case AOBLSS: case SOBGTR: case SOBGEQ: break;
		default:
			if (uses[r]==0) {/* no direct uses, check for use of condition codes */
				register struct node *q=p;
				while ((q=nonlab(q->forw))->combop==JBR) q=q->ref;	/* cc unused, unchanged */
				if (q->op!=CBR) {/* ... and destroyed */
					preg=regs+RT1;
					while (cp1= *preg++) {
						if (cp1==lastrand) {redunm++; delnode(p); return(p->forw);}
						if (source(cp1) || equstr(cp1,lastrand)) break;
					}
				}
			}
			flow=r;
		}
	}
	if (0<=(r=flow)) {olduse=uses[r]; uses[r]=0; *(short *)(regs[r])=0;}
		/* these two are here, rather than in bmove(),
		/* because I decided that it was better to go for 3-address code
		/* (save time) rather than fancy jbxx (save 1 byte)
		/* on sequences like  bisl2 $64,r0; movl r0,foo
		*/
	if (p->op==BIC) {p=bicopt(p); splitrand(p); lastrand=byondrd(p);}
	if (p->op==BIS) {bixprep(p,JBSS);           lastrand=byondrd(p);}
	/* now look for 'read' or 'modify' (read & write) uses */
	preg=regs+RT1; 
	while (*(cp1= *preg++)) {
		/* check for  r  */
		if (lastrand!=cp1 && 0<=(r=isreg(cp1)) && r<NUSE && uses[r]==0) {
			uses[r]=p; cp2=regs[r]; *cp2++=p->subop;
			if (p->op==ASH && preg==(regs+RT1+1)) cp2[-1]=BYTE; /* stupid DEC */
			if (p->op==MOV || p->op==PUSH || p->op==CVT || p->op==MOVZ || p->op==COM || p->op==NEG) {
				if (p->op==PUSH) cp1="-(sp)";
				else {
					cp1=regs[RT2];
					if (0<=(r=isreg(cp1)) && r<NUSE && uses[r]==0)
						uses[r]=olduse; /* reincarnation!! */
					/* as in  addl2 r0,r1;  movl r1,r0;  ret  */
					if (p->op!=MOV) cp1=0;
				}
				if (cp1) while (*cp2++= *cp1++);
				else *cp2=0;
			} else *cp2=0;
			continue;
		}
		/* check for (r),(r)+,-(r),[r] */
		do if (*cp1=='(' || *cp1=='[') {/* get register number */
			char t;
			cp2= ++cp1; while (*++cp1!=')' && *cp1!=']'); t= *cp1; *cp1=0;
			if (0<=(r=isreg(cp2)) && r<NUSE && (uses[r]==0 || uses[r]==p)) {
				uses[r]=p; regs[r][0]=(*--cp2=='[' ? OPX<<4 : OPB<<4);
			}
			*cp1=t;
		} while (*++cp1);
	}
	/* pushax or movax possibility? */
	cp1=regs[RT1];
	if (*cp1++=='$' && isstatic(cp1) && natural(regs[RT1])) {
		if (p->combop==T(MOV,LONG)) {
			if (regs[RT1][1]=='L' && 0!=(p->labno=getnum(regs[RT1]+2))) {
				cp1=p->code; while (*cp1++!=','); p->code= --cp1;
			}
			p->combop=T(MOVA,LONG); ++p->code; p->pop=0;
		} else if (p->combop==T(PUSH,LONG)) {
			p->combop=T(PUSHA,LONG); ++p->code; p->pop=0;
		} else if ((p->combop&0xFFFF)==T(ADD,U(LONG,OP3))
				 && 0<=(r=isreg(regs[RT2]))) {
			cp1=cp2=p->code; ++cp1;
			do *cp2++= *cp1; while (*cp1++!=','); cp2[-1]='[';
			do *cp2++= *cp1; while (*cp1++!=','); cp2[-1]=']';
			if (!equstr(regs[RT3],"-(sp)")) p->combop=T(MOVA,BYTE);
			else {p->combop=T(PUSHA,BYTE); *cp2=0;}
			if (uses[r]==0) {uses[r]=p; regs[r][0]=OPX<<4;}
			p->pop=0;
		}
	}
	return(p);
}

ispow2(n) register long n; {/* -1 -> no; else -> log to base 2 */
	register int log;
	if (n==0 || n&(n-1)) return(-1); log=0;
	for (;;) {n >>= 1; if (n==0) return(log); ++log; if (n== -1) return(log);}
}

bitopt(p) register struct node *p; {
	/* change "bitx $<power_of_2>,a" followed by JEQ or JNE
	/* into JBC or JBS.  watch out for I/O registers. (?)
	/* assumes that 'splitrand' has already been called.
	*/
	register char	*opr_1 = regs[RT1],
			*opr_2 = regs[RT2];
	register int	bit_number;

	if ((p->forw->combop != T(CBR,JEQ))
	    && (p->forw->combop != T(CBR,JNE)))
		/*
		 * If the next operation is not jeql or jneq then it
		 * can't be changed.
		 */
		return;

	if (*opr_1 != '$')
		/*
		 * If the src is not immediate then don't optimize.
		 */
		return;

	/* Get the value of the mask and convert it to a bit number.
	 */
	bit_number = ispow2(getnum(&opr_1[1]));
	if ( (bit_number < 0) || (bit_number>=bitsize[p->subop]))
		/* If it is not a power of two, ie. single bit then don't
		 * optimize.  If the bit is greater than the highest bit in
		 * the in the entity to be tested, then don't attempt to
		 * optimize, since this might mask a bug in someone's code.
		 * Just let it go through and the assembler will complain.
		 */
		return;

	if ( !okio(opr_2) )
		/*
		 * If the operand being tested might be an I/O register,
		 * then do nothing.
		 */
		return;

	if (p->subop == LONG) {
		/*
		 * Index mode can be used with a LONG operand, only if
		 * this will be converted to a jbl[cs].
		 */
		if ((bit_number != 0) && indexa(opr_2)) {
			return ;
		}
	} else if (source(opr_2)) {
		/*
		 * Auto{in,de}crement mode can be used only with LONG
		 * operands
		 */
		return ;
	} else if ((p->subop != BYTE) && indexa(opr_2)) {
		/*
		 * Index mode can be used only with BYTE operands, except
		 * for the case mentioned above.
		 */
		return;
	}

	/* Now try to optimize.
	 */

	if (bit_number==0 && (p->subop==LONG || !indexa(opr_2))) {
		/*
		 * bitx	$1,dst	\
		 *		 -  jlbx dst, label
		 * jxxx label	/
		 *
		 * The $1 is thrown away, the dst becomes the first operand
		 * and the label is either the label from the jxxx or an
		 * internally generated label which will be based on the
		 * value in p->forw->labno.
		 */
		if (p->forw->code)
			new_operands(p,"%s,%s",opr_2,p->forw->code);
		else
			new_operands(p,"%s",opr_2);
		p->forw->subop += JLBC-JBC;
		p->forw->pop=0;
	} else {
		/*
		 * bitx	$n,dst	\
		 *		 -  jbx $bit_number,dst,label
		 * jxxx label	/
		 *
		 * Reformat the operands appropriately.
		 */
		if (p->forw->code)
			new_operands(p,
				     "$%d,%s,%s",
				     bit_number,opr_2,p->forw->code);
		else
			new_operands(p,"$%d,%s",bit_number,opr_2);
	}
	p->combop = p->forw->combop+((JBC-JEQ)<<8);
	p->labno = p->forw->labno;
	delnode(p->forw);
	p->pop=0;
	nbj++;
	}

isfield(n) register long n; {/* -1 -> no; else -> position of low bit */
	register int pos; register long t;
	t= ((n-1)|n) +1;
	if (n!=0 && (0==t || 0==n || 0<=ispow2(t))) {
		pos=0; while(!(n&1)) {n >>= 1; ++pos;} return(pos);
	} else return(-1);
}

bixprep(p,bix) register struct node *p; {
/* initial setup, single-bit checking for bisopt, bicopt.
/* return: 0->don't bother any more; 1->worthwhile trying
*/
	register char *cp1,*cp2;
	splitrand(p); cp1=regs[RT1]; cp2=regs[RT2];
	if (*cp1++!='$' || 0>(pos=isfield(f=getnum(cp1)))
	  || !okio(cp2) || indexa(cp2) || source(cp2) || !okio(lastrand)) return(0);
	f |= f-1; if (++f==0) siz=32-pos; else siz=ispow2(f)-pos;
	return(1);
}


struct node *
bicopt(p) register struct node *p; {

/* Try to replace a BICLn instruction with a MOVZxL or EXTZV instruction.  If
 * the BICLn is preceded by an ASH or {CVTxL|MOVZxL} or a {CVTxL|MOVZxl} / 
 * ASH these may also be optimized out.  Done as part of 'bflow'.
 */
	register char *cp1,*cp2;
	register struct node *back;	/* previous node		*/
	int r;			/* register number			*/
	int deleted = 0 ;	/* ASH, CVT, or MOVZ deleted?		*/
	register int mov_src ;	/* If non-zero, indicates size of the
				 * MOVZxL which can replace the BICL.
				 */
	char src[C2_ASIZE];	/* source of BICL		*/
	/* Do some preliminary tests to see if we can optimize the BIC
	 */
	if (!bixprep(p,JBCC)) return(p);
	if ((f!=0)			/* doesn't isolate low order bits */
	    || (p->subop&0xF)!=LONG	/* result isn't long, ie. BICL	  */
	    || *(regs[RT2])=='$')	/* src of BICL3 is immediate	  */
		return(p);

	siz=pos; pos=0;

	/* Save source of BICL */
	cp1=regs[RT2]; cp2=src; while (*cp2++= *cp1++);

	/* If the preceding instruction is an ASH see if it can be deleted.
	 */
	back = p->back;
	if (back->op==ASH) {/* try for more */
		splitrand(back); cp1=regs[RT1]; cp2=regs[RT3];
		if (*cp1++=='$' && okio(regs[RT2]) && okio(cp2) 
                    && *(regs[RT2])!='$'
                    && !indexa(regs[RT2])
		    && 0>(f=getnum(cp1)) && equstr(src,cp2)
		    && 0<=(r=isreg(cp2)) && r<NUSE) 
                {/* a good ASH */
			pos -= f;
			cp1=regs[RT2]; cp2=src; while (*cp2++= *cp1++);
			delnode(back);
			back = p->back; /* get new previous node */
			deleted++;
		}
	}
	/* "pos" and "siz" are known.  Find out if we can turn
	 * the BICL into a MOVZxL or an EXTZV. (0002)
	 */
	if (pos == 0) {
		if (siz == 8)       mov_src = BYTE; /*MOVZBL*/
		else if (siz == 16) mov_src = WORD; /*MOVZWL*/
		else                mov_src = 0;    /*EXTZV */
	} else mov_src = 0 ;			    /*EXTZV */

	/* If the previous instruction is now a CVT or a MOVZ
	 * we may be able to delete it.
	 */
	if (back->op==CVT || back->op==MOVZ) {

		/* What is the source operand type for the CVT or MOVZ?
		 */
		register int src_type = back->subop & 0xF ;

		/* Get the operand strings for the instruction
		 */
		splitrand(back); 
		cp1=regs[RT1]; 	/* source operand address */
		cp2=regs[RT2];	/* destination "     "    */

		/* We can delete the instruction if:
		 *   - dst is the same as src of the BIC
		 *   - src isn't in I/O space
		 *   - src isn't indexing mode or auto(in|de)crement mode
		 *     unless the mov_src is the same as the src_type (0002)
		 *   - src isn't a floating point type (0002)
		 *   - dst is a usable register
		 *   - the bit field fits within the src & dst
		 */
		if (equstr(src,cp2) && okio(cp1) 
		  && src_type != FFLOAT && src_type != DFLOAT
		  && src_type != GFLOAT && src_type != HFLOAT
		  && ((!indexa(cp1) && !autoid(cp1)) || mov_src == src_type)
		  && 0<=(r=isreg(cp2)) && r<NUSE
		  && bitsize[src_type]>=(pos+siz)
		  && bitsize[back->subop>>4]>=(pos+siz)) {

			/* We can get rid of the instruction.  Use the src of
			 * this instruction for the src of the MOVZ or EXTZV
			 * which replaces it.
			 */
			cp1=regs[RT1]; cp2=src; while (*cp2++= *cp1++);
			delnode(back);
			deleted++;
		}
	}
	splitrand(p);	/* Retrieve destination of BICL into "lastrand".
			 */
	if (mov_src) {	
		/* Replace the BICL with a MOVZxL instruction.
		 */
		p->combop = T(MOVZ,U(mov_src,LONG));
		sprintf(line,"%s,%s",src,lastrand);

	} else if (deleted) {
		/* Replace the BICL with an EXTZV instruction.
		 */
		p->combop = T(EXTZV,LONG);
		sprintf(line,"$%d,$%d,%s,%s",pos,siz,src,lastrand);

	} else	/* None of the previous instructions could be deleted, and
		 * replacing a BICL with an EXTZV would not be "optimal".
		 * (0002)
		 */
		return(p);

	p->pop=0;
	p->code = copy(line);
	nfield++;
	return(p);
}

jumpsw()
{
	register struct node *p, *p1;
	register t;
	int nj;

	t = 0;
	nj = 0;
	for (p=first.forw; p!=0; p = p->forw)
		p->seq = ++t;
	for (p=first.forw; p!=0; p = p1) {
		p1 = p->forw;
		if (p->op == CBR && p1->op==JBR && p->ref && p1->ref
		 && abs(p->seq - p->ref->seq) > abs(p1->seq - p1->ref->seq)) {
			if (p->ref==p1->ref)
				continue;
			p->subop = revbr[p->subop];
			p->pop=0;
			t = p1->ref;
			p1->ref = p->ref;
			p->ref = t;
			t = p1->labno;
			p1->labno = p->labno;
			p->labno = t;
#ifdef COPYCODE
			if (p->labno == 0) {
				t = p1->code; p1->code = p->code; p->code = t;
			}
#endif
			nrevbr++;
			nj++;
		}
	}
	return(nj);
}

addsob()
{
	register struct node *p, *p1, *p2, *p3;

	for (p = &first; (p1 = p->forw)!=0; p = p1) {
	if (p->combop==T(DEC,LONG) && p1->op==CBR) {
		if (abs(p->seq - p1->ref->seq) > 8) continue;
		if (p1->subop==JGE || p1->subop==JGT) {
			if (p1->subop==JGE) p->combop=SOBGEQ; else p->combop=SOBGTR;
			p->pop=0;
			p->labno = p1->labno; delnode(p1); nsob++;
		}
	} else if (p->combop==T(INC,LONG)) {
		if (p1->op==LABEL && p1->refc==1 && p1->forw->combop==T(CMP,LONG)
		  && (p2=p1->forw->forw)->combop==T(CBR,JLE)
		  && (p3=p2->ref->back)->combop==JBR && p3->ref==p1
		  && p3->forw->op==LABEL && p3->forw==p2->ref) {
			/* change	INC LAB: CMP	to	LAB: INC CMP */
			p->back->forw=p1; p1->back=p->back;
			p->forw=p1->forw; p1->forw->back=p;
			p->back=p1; p1->forw=p;
			p1=p->forw;
			/* adjust beginning value by 1 */
				p2=alloc(sizeof first); p2->combop=T(DEC,LONG);
				p2->pop=0;
				p2->forw=p3; p2->back=p3->back; p3->back->forw=p2;
				p3->back=p2; p2->code=p->code; p2->labno=0;
		}
		if (p1->combop==T(CMP,LONG) && (p2=p1->forw)->op==CBR) {
			register char *cp1,*cp2;
			splitrand(p1); if (!equstr(p->code,regs[RT1])) continue;
			if (abs(p->seq - p2->ref->seq)>8) {/* outside byte displ range */
				if (p2->subop!=JLE) continue;
				p->combop=T(ACB,LONG);
				new_operands(p,"%s,$1,%s",regs[RT2],p->code);
				p->pop=0;
				p->labno = p2->labno;
				delnode(p2);
				delnode(p1);
				nsob++;
			} else if (p2->subop==JLE || p2->subop==JLT) {
				p->combop = (p2->subop==JLE) ? AOBLEQ:AOBLSS;
				new_operands(p,"%s,%s",regs[RT2],p->code);
				p->pop=0;
				p->labno = p2->labno;
				delnode(p2);
				delnode(p1);
				nsob++;
			}
		}
	}
	}
}

abs(x)
{
	return(x<0? -x: x);
}

equop(p1, p2)
register struct node *p1;
struct node *p2;
{
	register char *cp1, *cp2;

	if (p1->combop != p2->combop)
		return(0);
	if (p1->op>0 && p1->op<MOV)
		return(0);
	if (p1->op==MOVA && p1->labno!=p2->labno) return(0);
	cp1 = p1->code;
	cp2 = p2->code;
	if (cp1==0 && cp2==0)
		return(1);
	if (cp1==0 || cp2==0)
		return(0);
	while (*cp1 == *cp2++)
		if (*cp1++ == 0)
			return(1);
	return(0);
}

delnode(p) register struct node *p; {
	p->back->forw = p->forw;
	p->forw->back = p->back;
}

decref(p)
register struct node *p;
{
	if (p && --p->refc <= 0) {
		nrlab++;
		delnode(p);
	}
}

struct node *
nonlab(ap)
struct node *ap;
{
	register struct node *p;

	p = ap;
	while (p && p->op==LABEL)
		p = p->forw;
	return(p);
}

clearuse() {
	register struct node **i;
	for (i=uses+NUSE; i>uses;) *--i=0;
}

clearreg() {
	register short **i;
	for (i=regs+NREG; i>regs;) **--i=0;
	conloc[0] = 0; ccloc[0] = 0;
}

savereg(ai, s, type)
register char *s;
{
	register char *p, *sp;

	sp = p = regs[ai];
	if (source(s)) /* side effects in addressing */
		return;
	/* if any indexing, must be parameter or local */
	/* indirection (as in "*-4(fp)") is ok, however */
	*p++ = type;
	while (*p++ = *s)
		if (*s=='[' || *s++=='(' && *s!='a' && *s!='f') {*sp = 0; return;}
}

dest(s,type)
register char *s;
{
	register int i;

	(void) source(s); /* handle addressing side effects */
	if (!natural(s)) {
		/* wild store, everything except constants vanishes */
		for (i=NREG; --i>=0;)
			if (regs[i][1] != '$')
				*(short *)(regs[i]) = 0;
		conloc[0] = 0; ccloc[0] = 0;
		return;
	}
	if ((i = isreg(s)) >= 0) {
		/* if register destination, that reg is a goner */
		*(short *)(regs[i]) = 0;
		switch(type & 0xF){
		case DFLOAT:	/* clobber two at once */
			/*FALLTHROUGH*/
		case GFLOAT:
			*(short *)(regs[i+1]) = 0;
			break;
		case HFLOAT:	/* clobber four at once */
			*(short *)(regs[i+1]) = 0;
			*(short *)(regs[i+2]) = 0;
			*(short *)(regs[i+3]) = 0;
			break;
		}
		switch((type>>4)&0xF){
		case DFLOAT:	/* clobber two at once */
			/*FALLTHROUGH*/
		case GFLOAT:
			*(short *)(regs[i+1]) = 0;
			break;
		case HFLOAT:	/* clobber four at once */
			*(short *)(regs[i+1]) = 0;
			*(short *)(regs[i+2]) = 0;
			*(short *)(regs[i+3]) = 0;
			break;
		}
	}
	for (i=NREG; --i>=0;)
		if (regs[i][1]=='*' && equstr(s, regs[i]+2))
			*(short *)(regs[i]) = 0; /* previous indirection through destination is invalid */
	while ((i = findrand(s,0)) >= 0) /* previous values of destination are invalid */
		*(short *)(regs[i]) = 0;
	if (*conloc && equstr(conloc, s))
		conloc[0] = 0;
	setcc(s, type); /* natural destinations set condition codes */
}

splitrand(p) struct node *p; {
/* separate operands at commas, set up 'regs' and 'lastrand' */
register char *p1, *p2; register char **preg;
preg=regs+RT1;
if (p1=p->code) while (*p1) {
	lastrand=p2= *preg++;
	while (*p1) if (','==(*p2++= *p1++)) {--p2; break;}
	*p2=0;
}
while (preg<(regs+RT1+5)) *(*preg++)=0;
}

compat(have, want)

/* See if the the operand types are compatible. "have" contains the source
 * and destination types for an instruction (which we may be trying to
 * replace).  "want" contains the operand type we want as a result.  DLB003
 */

{	register int	wanted_type,	/* Type we want to end with	*/
			src_type,	/* Source type of instruction	*/
			dst_type;	/* Destination type of "	*/

	/* Get "wanted_type" from "want" and if it is 0 then we have
	 * wildcard request so anything matches.
	 */
	if ( (wanted_type = (want & 0xF)) == 0)
		return(1);

	/* Extract the source and destination tyes from "have".  If the
	 * destination type is wildcard or one of the OPx types then make
	 * it the same as the source.				DLB003
	 */
	src_type=have&0xF;
	dst_type=((have>>4)&0xF);
	if (dst_type==0 || (dst_type>=OP2 && dst_type<=OPX))
		dst_type=src_type;

	/* If the wanted_type is not BYTE, WORD, or LONG, then the
	 * types are compatible only if all three are the same.	DLB003
	 */
	if (wanted_type>=FFLOAT)
		return(dst_type==wanted_type && 
		       src_type==wanted_type);

	/* Otherwise, the types are compatible only if the the destination
	 * type is a BYTE, WORD, or LONG and both the destination and
	 * sources types are larger than the wanted type.	DLB003
	 */
	return(dst_type<FFLOAT &&
	       dst_type>=wanted_type &&
	       src_type>=wanted_type );
}

equtype(t1,t2) {return(compat(t1,t2) && compat(t2,t1));}

findrand(as, type)
char *as;
{
	register char **i;
	for (i = regs+NREG; --i>=regs;) {
		if (**i && equstr(*i+1, as) && compat(**i,type))
			return(i-regs);
	}
	return(-1);
}

isreg(s)
register char *s;
{
	if (*s++!='r' || !isdigit(*s++)) return(-1);
	if (*s==0) return(*--s-'0');
	if (*(s-1)=='1' && isdigit(*s++) && *s==0) return(10+*--s-'0');
	return(-1);
}

check()
{
	register struct node *p, *lp;

	lp = &first;
	for (p=first.forw; p!=0; p = p->forw) {
		if (p->back != lp)
			abort(-1);
		lp = p;
	}
}

source(ap)
char *ap;
{
	register char *p1, *p2;

	p1 = ap;
	p2 = p1;
	if (*p1==0)
		return(0);
	while (*p2++ && *(p2-1)!='[');
	if (*p1=='-' && *(p1+1)=='('
	 || *p1=='*' && *(p1+1)=='-' && *(p1+2)=='('
	 || *(p2-2)=='+') {
		while (*p1 && *p1++!='r');
		if (isdigit(*p1++))
			if (isdigit(*p1)) *(short *)(regs[10+*p1-'0'])=0;
			else *(short *)(regs[*--p1-'0'])=0;
		return(1);
	}
	return(0);
}

newcode(p) struct node *p; {
	register char *p1,*p2,**preg;
	preg=regs+RT1; p2=line;
	while (*(p1= *preg++)) {while (*p2++= *p1++); *(p2-1)=',';}
	*--p2=0;
	p->code=copy(line);
}

repladdr(p)
struct node *p;
{
	register r;
	register char *p1, *p2;
	char **preg; int nrepl;

	preg=regs+RT1; nrepl=0;
	while (lastrand!=(p1= *preg++))
		if (!source(p1) && 0<=(r=findrand(p1,p->subop))) {
			*p1++='r'; if (r>9) {*p1++='1'; r -= 10;} *p1++=r+'0'; *p1=0;
			nrepl++; nsaddr++;
		}
	if (nrepl) newcode(p);
}

/* movedat()
/* {
/* 	register struct node *p1, *p2;
/* 	struct node *p3;
/* 	register seg;
/* 	struct node data;
/* 	struct node *datp;
/* 
/* 	if (first.forw == 0)
/* 		return;
/* 	datp = &data;
/* 	for (p1 = first.forw; p1!=0; p1 = p1->forw) {
/* 		if (p1->op == DATA) {
/* 			p2 = p1->forw;
/* 			while (p2 && p2->op!=TEXT)
/* 				p2 = p2->forw;
/* 			if (p2==0)
/* 				break;
/* 			p3 = p1->back;
/* 			p1->back->forw = p2->forw;
/* 			p2->forw->back = p3;
/* 			p2->forw = 0;
/* 			datp->forw = p1;
/* 			p1->back = datp;
/* 			p1 = p3;
/* 			datp = p2;
/* 		}
/* 	}
/* 	if (data.forw) {
/* 		datp->forw = first.forw;
/* 		first.forw->back = datp;
/* 		data.forw->back = &first;
/* 		first.forw = data.forw;
/* 	}
/* 	seg = -1;
/* 	for (p1 = first.forw; p1!=0; p1 = p1->forw) {
/* 		if (p1->op==TEXT||p1->op==DATA||p1->op==BSS) {
/* 			if (p1->op == seg || p1->forw&&p1->forw->op==seg) {
/* 				p1->back->forw = p1->forw;
/* 				p1->forw->back = p1->back;
/* 				p1 = p1->back;
/* 				continue;
/* 			}
/* 			seg = p1->op;
/* 		}
/* 	}
/* }
*/

redunbr(p)
register struct node *p;
{
	register struct node *p1;
	register char *ap1;
	char *ap2;

	if ((p1 = p->ref) == 0)
		return;
	p1 = nonlab(p1);
	if (p1->op==TST) {
		splitrand(p1);
		savereg(RT2, "$0", p1->subop);
	} else if (p1->op==CMP)
		splitrand(p1);
	else
		return;
	if (p1->forw->op==CBR) {
		ap1 = findcon(RT1, p1->subop);
		ap2 = findcon(RT2, p1->subop);
		p1 = p1->forw;
		if (compare(p1->subop, ap1, ap2)) {
			nredunj++;
			nchange++;
			decref(p->ref);
			p->ref = p1->ref;
			p->labno = p1->labno;
#ifdef COPYCODE
			if (p->labno == 0)
				p->code = p1->code;
			if (p->ref)
#endif
				p->ref->refc++;
		}
	} else if ( p1->op==TST 
		    && equstr(regs[RT1],ccloc+1) 
		    && equtype(ccloc[0],p1->subop)
		  ) {
		/* The results of the TST aren't used, so we can just
		 * branch past it.  Insert a new label in front of the
		 * TST, dereference the old label, and set up the branch
		 * to go to the new label.
		 */
		p1=insertl(p1->forw);
		decref(p->ref);
		p->ref = p1;
		p->labno = p1->labno;
		nrtst++; nchange++;
	}
}

char *
findcon(i, type)
{
	register char *p;
	register r;

	p = regs[i];
	if (*p=='$')
		return(p);
	if ((r = isreg(p)) >= 0 && compat(regs[r][0],type))
		return(regs[r]+1);
	if (equstr(p, conloc))
		return(conval+1);
	return(p);
}

compare(op, acp1, acp2)
char *acp1, *acp2;
{
	register char *cp1, *cp2;
	register n1;
	int n2;	int sign;

	cp1 = acp1;
	cp2 = acp2;
	if (*cp1++ != '$' || *cp2++ != '$')
		return(0);
	n1 = 0; sign=1; if (*cp2=='-') {++cp2; sign= -1;}
	while (isdigit(*cp2)) {n1 *= 10; n1 += (*cp2++ - '0')*sign;}
	n2 = n1;
	n1 = 0; sign=1; if (*cp1=='-') {++cp1; sign= -1;}
	while (isdigit(*cp1)) {n1 *= 10; n1 += (*cp1++ - '0')*sign;}
	if (*cp1=='+')
		cp1++;
	if (*cp2=='+')
		cp2++;
	do {
		if (*cp1++ != *cp2)
			return(0);
	} while (*cp2++);
	cp1 = n1;
	cp2 = n2;
	switch(op) {

	case JEQ:
		return(cp1 == cp2);
	case JNE:
		return(cp1 != cp2);
	case JLE:
		return(((int)cp1) <= ((int)cp2));
	case JGE:
		return(((int)cp1) >= ((int)cp2));
	case JLT:
		return(((int)cp1) < ((int)cp2));
	case JGT:
		return(((int)cp1) > ((int)cp2));
	case JLO:
		return(cp1 < cp2);
	case JHI:
		return(cp1 > cp2);
	case JLOS:
		return(cp1 <= cp2);
	case JHIS:
		return(cp1 >= cp2);
	}
	return(0);
}

setcon(cv, cl, type)
register char *cv, *cl;
{
	register char *p;

	if (*cv != '$')
		return;
	if (!natural(cl))
		return;
	p = conloc;
	while (*p++ = *cl++);
	p = conval;
	*p++ = type;
	while (*p++ = *cv++);
}

equstr(p1, p2)
register char *p1, *p2;
{
	do {
		if (*p1++ != *p2)
			return(0);
	} while (*p2++);
	return(1);
}

setcc(ap,type)
char *ap;
{
	register char *p, *p1;

	p = ap;
	if (!natural(p)) {
		ccloc[0] = 0;
		return;
	}
	p1 = ccloc;
	*p1++ = type;
	while (*p1++ = *p++);
}

okio(p) register char *p; {/* 0->probable I/O space address; 1->not */
	if (ioflag && (!natural(p) || 0>getnum(p))) return(0);
	return(1);
}

indexa(p) register char *p; {/* 1-> uses [r] addressing mode; 0->doesn't */
	while (*p) if (*p++=='[') return(1);
	return(0);
}

autoid(p)
	register char *p ;	/* operand address string */

/* See if operand addressing mode uses auto(in|de)crement.
 * Return 1 for yes, 0 for no.
 */
{
	if (*p == '-' && *(p+1) == '(') return(1) ; /* autodecrement	  */
	while (*p) p++;				    /* find end of string */
	if (*--p == '+' && *--p == ')') return(1) ; /* autoincrement	  */
	return(0) ;				    /* neither		  */
}

natural(p)
register char *p;
{/* 1->simple local, parameter, global, or register; 0->otherwise */
	if (*p=='*' || *p=='(' || *p=='-'&&*(p+1)=='(' || *p=='$'&&getnum(p+1))
		return(0);
	while (*p++);
	p--;
	if (*--p=='+' || *p==']' || *p==')' && *(p-2)!='a' && *(p-2)!='f')
		return(0);
	return(1);
}

/*
** Tell if an argument is most likely static.
*/

isstatic(cp)
register char	*cp;
{
	if (*cp == '_' || *cp == 'L' || (*cp++ == 'v' && *cp == '.'))
		return (1);
	return (0);
}
