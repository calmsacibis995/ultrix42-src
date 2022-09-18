/*@(#)printsym.c	4.2  Ultrix  11/9/90*/

/************************************************************************
 *									*
 *			Copyright (c) 1986, 1987 by			*
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
 *									*
 *			Modification History				*
 *									*
 *  008	- Added support for vectors. Made print of function 	*
 *		  more informative.										*
 *		  (L Miller, 18JAN90)									*
 *									*
 *	007 - Changed default language back to "C" from assembler;	*
 *	      apparently a typo in 003.  Also remerged 4.3 code to	*
 *	      preserve spacing.						*
 *	      (jlr, July 13, 1987)					*
 *									*
 *	006 - Added stub version of prth (H float output).		*
 *	      Ugly, but it beats printing it as complex.		*
 *	      (Jon Reeves, April 14, 1987)				*
 *									*
 *	005 - Merged in 4.3 changes.					*
 *	      (vjh, April 29, 1986)					*
 *									*
 *	004 - Pushretval() now returns a boolean value.  If false,	*
 *	      prints "??".  Pushretval() used to panic when value was	*
 *	      inaccessible (eg. FORTRAN arrays).			*
 *	      (vjh, July 18, 1985)					*
 *									*
 *	003 - Updated all calls to findlanguage() to call with		*
 *	      LanguageName constant, rather than with a filename	*
 *	      suffix.							*
 *	      (vjh, June 22, 1985)					*
 *									*
 *	002 - Modified psym() to understand new Symbol format for	*
 *	      register variables (symvalue.raddr.<fields>).		*
 *	      (vjh, June 22, 1985)					*
 *									*
 *	001 - Uncommented the code that prints "ARRAY" when 		*
 *	      displaying the values of parameters with the "where"	*
 *	      command.							*
 *	      (Victoria Holt, May 2, 1985)				*
 *									*
 ************************************************************************/

/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char sccsid[] = "@(#)printsym.c	2.1	ULTRIX	4/24/89";
#endif not lint

/*
 * Printing of symbolic information.
 */

#include "defs.h"
#include "symbols.h"
#include "languages.h"
#include "printsym.h"
#include "tree.h"
#include "eval.h"
#include "mappings.h"
#include "process.h"
#include "runtime.h"
#include "machine.h"
#include "names.h"
#include "keywords.h"
#include "main.h"

#ifndef public
/* When ANSI comes around, this should be a long float. jlr006*/
struct hfloat{
	long exponent;
	long frac1;
	long frac2;
	long frac3;
};
#endif

/*
 * Maximum number of arguments to a function.
 * This is used as a check for the possibility that the stack has been
 * overwritten and therefore a saved argument pointer might indicate
 * to an absurdly large number of arguments.
 */

#define MAXARGSPASSED 20

/*
 * Return a pointer to the string for the name of the class that
 * the given symbol belongs to.
 */

private String clname[] = {
    "bad use", "constant", "type", "variable", "array", "@dynarray",
    "@subarray", "fileptr", "record", "field",
    "procedure", "function", "funcvar",
    "ref", "pointer", "file", "set", "range", "label", "withptr",
    "scalar", "string", "program", "improper", "variant",
    "procparam", "funcparam", "module", "tag", "common", "extref", "typeref"
};

public String classname(s)
Symbol s;
{
    return clname[ord(s->class)];
}

/*
 * Note the entry of the given block, unless it's the main program.
 */

public printentry(s)
Symbol s;
{
    if (s != program and isactive(s)) {
	printf("\nentering %s ", classname(s));
	printname(stdout, s);
	printf("\n");
    }
}

/*
 * Note the exit of the given block
 */

public printexit(s)
Symbol s;
{
    if (s != program) {
	printf("leaving %s ", classname(s));
	printname(stdout, s);
	printf("\n\n");
    }
}

/*
 * Note the call of s from t.
 */

public printcall(s, t)
Symbol s, t;
{
    printf("calling ");
    printname(stdout, s);
    printparams(s, nil);
    while (isinline(t)) {
        t = container(t);
    }
    printf(" from %s ", classname(t));
    printname(stdout, t);
    printf("\n");
}

/*
 * Note the return from s.  If s is a function, print the value
 * it is returning.  This is somewhat painful, since the function
 * has actually just returned.
 */

public printrtn(s)
Symbol s;
{
    register Symbol t;
    register int len;
    Boolean isindirect;

    printf("returning ");
    if (s->class == FUNC && (!istypename(s->type,"void"))) {
	len = size(s->type);
	if (canpush(len)) {
	    t = rtype(s->type);
	    isindirect = (Boolean) (t->class == RECORD or t->class == VARNT);
	    if (pushretval(len, isindirect)) {
	        printval(s->type);
	    } else {
	        printf("??");
	    }
	    putchar(' ');
	} else {
	    printf("(value too large) ");
	}
    }
    printf("from ");
    printname(stdout, s);
    printf("\n");
}

/*
 * Print the values of the parameters of the given procedure or function.
 * The frame distinguishes recursive instances of a procedure.
 *
 * If the procedure or function is internal, the argument count is
 * not valid so we ignore it.
 */

public printparams(f, frame)
Symbol f;
Frame frame;
{
    Symbol param;
    int n, m, s;

    n = nargspassed(frame);
    if (isinternal(f)) {
	n = 0;
    }
    printf("(");
    param = f->chain;
    if (param != nil or n > 0) {
	m = n;
	if (param != nil) {
	    for (;;) {
		s = psize(param) div sizeof(Word);
		if (s == 0) {
		    s = 1;
		}
		m -= s;
		if (showaggrs) {
		    printv(param, frame);
		} else {
		    printparamv(param, frame);
		}
		param = param->chain;
	    if (param == nil) break;
		printf(", ");
	    }
	}
	if (m > 0) {
	    if (m > MAXARGSPASSED) {
		m = MAXARGSPASSED;
	    }
	    if (f->chain != nil) {
		printf(", ");
	    }
	    for (;;) {
		--m;
		printf("0x%x", argn(n - m, frame));
	    if (m <= 0) break;
		printf(", ");
	    }
	}
    }
    printf(")");
}

/*
 * Test if a symbol should be printed.  We don't print files,
 * for example, simply because there's no good way to do it.
 * The symbol must be within the given function.
 */

public Boolean should_print(s)
Symbol s;
{
    Boolean b;
    register Symbol t;

    switch (s->class) {
	case VAR:
	case FVAR:
	    if (isparam(s)) {
		b = false;
	    } else {
		t = rtype(s->type);
		if (t == nil) {
		    b = false;
		} else {
		    switch (t->class) {
			case FILET:
			case SET:
			case BADUSE:
			    b = false;
			    break;

			default:
			    b = true;
			    break;
		    }
		}
	    }
	    break;

	default:
	    b = false;
	    break;
    }
    return b;
}

/*
 * Print out a parameter value.
 *
 * Since this is intended to be printed on a single line with other information
 * aggregate values are not printed.
 */

public printparamv (p, frame)
Symbol p;
Frame frame;
{
    Symbol t;

    t = rtype(p->type);
    switch (t->class) {
	case ARRAY:
	case DYNARRAY:
	case SUBARRAY:
	    t = rtype(t->type);
	    if (compatible(t, t_char)) {
		printv(p, frame);
	    } else {
		printf("%s = (...)", symname(p));
	    }
	    break;

	case RECORD:
	    printf("%s = (...)", symname(p));
	    break;

	default:
	    printv(p, frame);
	    break;
    }
}

/*
 * Print the name and value of a variable.
 */

public printv(s, frame)
Symbol s;
Frame frame;
{
    Address addr;
    int len;

    if (isambiguous(s) and ismodule(container(s))) {
	printname(stdout, s);
	printf(" = ");
    } else {
	printf("%s = ", symname(s));
    }
    if (isvarparam(s) and not isopenarray(s)) {
	rpush(address(s, frame), sizeof(Address));
	addr = pop(Address);
    } else {
	addr = address(s, frame);
    }
    len = size(s);
    if (not canpush(len)) {
	printf("*** expression too large ***");
    } else if (isreg(s)) {
	push(Address, addr);
	printval(s->type);
    } else {
	rpush(addr, len);
	printval(s->type);
    }
}

/*
 * Print out the name of a symbol.
 */

public printname(f, s)
File f;
Symbol s;
{
    if (s == nil) {
	fprintf(f, "(noname)");
    } else if (s == program) {
	fprintf(f, ".");
    } else if (isredirected() or isambiguous(s)) {
	printwhich(f, s);
    } else {
	fprintf(f, "%s", symname(s));
    }
}

/*
 * Print the fully specified variable that is described by the given identifer.
 */

public printwhich(f, s)
File f;
Symbol s;
{
    printouter(f, container(s));
    fprintf(f, "%s", symname(s));
}

/*
 * Print the fully qualified name of each symbol that has the same name
 * as the given symbol.
 */

public printwhereis(f, s)
File f;
Symbol s;
{
    register Name n;
    register Symbol t;

    checkref(s);
    n = s->name;
    t = lookup(n);
    printwhich(f, t);
    t = t->next_sym;
    while (t != nil) {
	if (t->name == n) {
	    putc(' ', f);
	    printwhich(f, t);
	}
	t = t->next_sym;
    }
    putc('\n', f);
}

private printouter(f, s)
File f;
Symbol s;
{
    Symbol outer;

    if (s != nil) {
	outer = container(s);
	if (outer != nil and outer != program) {
	    printouter(f, outer);
	}
	fprintf(f, "%s.", symname(s));
    }
}

public printdecl(s)
Symbol s;
{
    Language lang;

    checkref(s);
    if (s->language == nil or s->language == primlang) {
	lang = findlanguage(ASSEMBLER);
    } else {
	lang = s->language;
    }
    (*language_op(lang, L_PRINTDECL))(s);
}

/*
 * Straight dump of symbol information.
 */

public psym(s)
Symbol s;
{
    printf("name\t%s\n", symname(s));
    printf("lang\t%s\n", language_name(s->language));
    printf("class\t%s\n", classname(s));
    printf("level\t%d\n", s->level);
    printf("type\t0x%x", s->type);
    if (s->type != nil and s->type->name != nil) {
	printf(" (%s)", symname(s->type));
    }
    printf("\nchain\t0x%x", s->chain);
    if (s->chain != nil and s->chain->name != nil) {
	printf(" (%s)", symname(s->chain));
    }
    printf("\nblock\t0x%x", s->block);
    if (s->block->name != nil) {
	printf(" (");
	printname(stdout, s->block);
	putchar(')');
    }
    putchar('\n');
    switch (s->class) {
	case TYPE:
	    printf("size\t%d\n", size(s));
	    break;

	case VAR:
	case REF:
	    if (s->level >= 3) {
		printf("address\t0x%x\n", s->symvalue.offset);
	    } else if (s->level < 0) {
	        printf("register = %d, indir = %d, disp = %d\n",
		    s->symvalue.raddr.reg, s->symvalue.raddr.indirect,
		    s->symvalue.raddr.displacement);
	    } else {
		printf("offset\t%d\n", s->symvalue.offset);
	    }
	    printf("size\t%d\n", size(s));
	    break;

	case RECORD:
	case VARNT:
	    printf("size\t%d\n", s->symvalue.offset);
	    break;

	case FIELD:
	    printf("offset\t%d\n", s->symvalue.field.offset);
	    printf("size\t%d\n", s->symvalue.field.length);
	    break;

	case PROG:
	case PROC:
	case FUNC:
	    printf("address\t0x%x\n", s->symvalue.funcv.beginaddr);
	    if (isinline(s)) {
		printf("inline procedure\n");
	    }
	    if (nosource(s)) {
		printf("does not have source information\n");
	    } else {
		printf("has source information\n");
	    }
	    break;

	case RANGE:
	    prangetype(s->symvalue.rangev.lowertype);
	    printf("lower\t%d\n", s->symvalue.rangev.lower);
	    prangetype(s->symvalue.rangev.uppertype);
	    printf("upper\t%d\n", s->symvalue.rangev.upper);
	    break;

	default:
	    /* do nothing */
	    break;
    }
}

private prangetype(r)
Rangetype r;
{
    switch (r) {
	case R_CONST:
	    printf("CONST");
	    break;

	case R_ARG:
	    printf("ARG");
	    break;

	case R_TEMP:
	    printf("TEMP");
	    break;

	case R_ADJUST:
	    printf("ADJUST");
	    break;
    }
}

/*
 * Print out in a formatted manner.  The stack contains a list of 
 * arguments evaluated in reverse order with the first being
 * the format string.  The parameter passed to the function is the
 * first in a series of Nodes describing the stack contents.
 */

public print_formatted(n, argv)
Node n;
char **argv;
{
    if(n->value.printform.lang == nil || n->value.printform.lang == primlang) 
	    (*language_op(findlanguage("ASSEMBLER"), L_PRINTF))(n, argv);
    else
	    (*language_op(n->value.printform.lang, L_PRINTF))(n, argv);
}

static Vquad mask_reg;

public getregbit(n)
Integer n;
{
	if(n > 31) {
		return(mask_reg.val[0] & (1 << (n - 32)));
	}
	else {
		return(mask_reg.val[1] & (1 << n));
	}
}

/*
 * Print out the value on top of the stack according to the given type.
 * If it is not a vector register or is $vmr then do nothing.
 */

public printval_with_mask(t, op, tt)
Symbol t;
Operator op;
Node tt;
{
    Symbol s;
	Symbol a = t->type;
    Stack *savesp, *newsp;
    Symbol eltype;
    long elsize;
	integer bit_position = 0, i;
	Vquad *xvmr;
	char val0[10], val1[10], *str;

    checkref(t);
    if (t->class == TYPEREF) {
		resolveRef(t);
    }
	if(tt == nil) {
		xvmr = (Vquad *)vreg(VMR);
		mask_reg.val[0] = xvmr->val[0];
		mask_reg.val[1] = xvmr->val[1];
	}
	else {
		mask_reg.val[0] = 0;
		mask_reg.val[1] = 0;
		str = (char *)tt->value.arg[0];
		if(strpbrk(str, "Xx.+- ")) {
			sp -= sizeof(struct Vreg);
			error("illegal mask");
		}
		i =strlen(str);
		if(i > 8) {
			strcpy(val1, str + i - 8);
			if(i < 17) {
				strncpy(val0, str, i - 8);
			}
			else {
				sp -= sizeof(struct Vreg);
				error("illegal mask");
			}
			if(sscanf(val1, "%lx", &mask_reg.val[1]) != 1) {
				sp -= sizeof(struct Vreg);
				error("illegal mask");
			}
			if(sscanf(val0, "%lx", &mask_reg.val[0]) != 1) {
				sp -= sizeof(struct Vreg);
				error("illegal mask");
			}
		}
		else {
			mask_reg.val[0] = 0;
			if(sscanf(str, "%lx", &mask_reg.val[1]) != 1) {
				sp -= sizeof(struct Vreg);
				error("illegal mask");
			}
		}
	}
	savesp = sp;
    sp -= (size(a));
    newsp = sp;
    eltype = rtype(a->type);
    elsize = size(eltype);
  	for (sp += elsize; sp <= savesp; sp += 2*elsize, bit_position++) {
		if(op == O_TMASK) {
			if(getregbit(bit_position)) {
				printf("%s[%d] : ", ident(t->name), bit_position);
				printval(eltype);
				printf("\n");
			}
			else {
				pop(Vquad);
				continue;
			}
		}
		else {
			if(getregbit(bit_position)) {
				pop(Vquad);
				continue;
			}
			else {
				printf("%s[%d] : ", ident(t->name), bit_position);
				printval(eltype);
				printf("\n");
			}
		}
  	}
  	sp = newsp;
	return;
}

public void arith_exception_decode(r)
long r;
{
	if(r & 0x2f) {
		printf("(arithmetic exception ");
		if(r & 0x20) {
			printf(", integer overflow");
		}
		if(r & 0x8) {
			printf(", floating overflow");
		}
		if(r & 0x4) {
			printf(", floating reserved operand");
		}
		if(r & 0x2) {
			printf(", floating divide by zero");
		}
		if(r & 0x1) {
			printf(", floating underflow");
		}
		printf(", dst register mask is 0x%02x",
					 (r >> 16) & 0xff);
		printf(")");
	} else {
		printf("no arithmetic exceptions ");
	}
	
}

public Boolean vexception_decode(r)
Vquad *r;
{
	if(!(r->val[0] & 0x3ff0)) {
		if(r->val[0] & 0x8000) {
			if(r->val[0] & 0xf) {
				printf("(reserved operand ");
				if(r->val[0] & 0x8) {
					printf(", overflow");
				}
				if(r->val[0] & 0x2) {
					printf(", divide by zero");
				}
				if(r->val[0] & 0x1) {
					printf(", underflow");
				}
				printf(")");
				return(true);
			}
		}
	}
	return(false);
}

/*
 * Print out the value on top of the stack according to the given type.
 */

public printval(t)
Symbol t;
{
    Symbol s;
	Vquad value;
	double r;
	long l;

    checkref(t);
    if (t->class == TYPEREF) {
	resolveRef(t);
    }
    switch (t->class) {
	case PROC:
	case FUNC:
	    s = pop(Symbol);
	    printf("\"%s\"", symname(s));
		if(t->class == FUNC) {
			printf(" is a function");
		}
		else {
			printf(" is a procedure");
		}
	    break;

	case TYPE:
		if(streq(ident(t->name), "$vquad")) {
	        popn(sizeof(Vquad), &r);
			if(!vexception_decode(&r)) {
				printf("(");
	        	prtreal(r);
				printf(")");
			}
			return;
		}
		if(streq(ident(t->name), "vint")) {
	        popn(sizeof(Vquad), &value);
			if(!vexception_decode(&value)) {
	        	printf("%d", value.val[0]);
			}
			return;
		}
		if(streq(ident(t->name), "vhex")) {
	        popn(sizeof(Vquad), &value);
			if(!vexception_decode(&value)) {
	        	printf("(0x%X,0x%X)", value.val[0], value.val[1]);
			}
			return;
		}
		if(streq(ident(t->name), "vfloat")) {
	        popn(sizeof(Vquad), &value);
			if(!vexception_decode(&value)) {
				printf("(");
	        	prtreal(value.val[0]);
				printf(",");
	        	prtreal(value.val[1]);
				printf(")");
			}
			return;
		}
		/* if not the special case of TYPE, fall thru to RANGE: */
	case RANGE:
		if(streq(ident(t->type->name), "vint")) {
	        popn(sizeof(Vquad), &value);
			if(!vexception_decode(&value)) {
	        	printf("%d", value.val[0]);
			}
			return;
		} else {
			if(streq(ident(t->type->name), "vhex")) {
	        	popn(sizeof(Vquad), &value);
				if(!vexception_decode(&value)) {
	        		printf("(0x%X,0x%X)", value.val[0], value.val[1]);
				}
				return;
			} else {
				if(streq(ident(t->type->name), "vfloat")) {
	        		popn(sizeof(Vquad), &value);
					if(!vexception_decode(&value)) {
						printf("(");
	        			prtreal(value.val[0]);
						printf(",");
	        			prtreal(value.val[1]);
						printf(")");
					}
					return;
				} else {
					if(streq(ident(t->type->name), "$vquad")) {
	        			popn(sizeof(Vquad), &r);
						if(!vexception_decode(&r)) {
							printf("(");
	        				prtreal(r);
							printf(")");
						}
						return;
					}
				}
			}
		}
		/* if not the special case of RANGE, fall thru to default: */
	
	default:
  		if(isvreg(t) && t->symvalue.raddr.reg == VAER) {
			l = pop(long);
	   		arith_exception_decode(l);
			return;
    	}
  		if(isvreg(t) && t->symvalue.raddr.reg == VMR) {
			value = pop(Vquad);
	   		printf("0x%04x %04x %04x %04x", 
					(value.val[0] >> 16) & 0x0ffff,
					(value.val[0] ) & 0x0ffff,
					(value.val[1] >> 16) & 0x0ffff,
					(value.val[1] ) & 0x0ffff);
			return;
    	}
	    if (t->language == nil) {
			error("unknown language");
	    } else if (t->language == primlang) {
			(*language_op(findlanguage(C), L_PRINTVAL))(t);
	    } else {
			(*language_op(t->language, L_PRINTVAL))(t);
	    }
	    break;
    }
}

/*
 * Print out the value of a record, field by field.
 */

public printrecord(s)
Symbol s;
{
    Symbol f;

    if (s->chain == nil) {
	error("record has no fields");
    }
    printf("(");
    sp -= size(s);
    f = s->chain;
    if (f != nil) {
	for (;;) {
	    printfield(f);
	    f = f->chain;
	if (f == nil) break;
	    printf(", ");
	}
    }
    printf(")");
}

/*
 * Print out a field.
 */

private printfield(f)
Symbol f;
{
    Stack *savesp;
    register int off, len;

    printf("%s = ", symname(f));
    savesp = sp;
    off = f->symvalue.field.offset;
    len = f->symvalue.field.length;
    sp += ((off + len + BITSPERBYTE - 1) div BITSPERBYTE);
    printval(f);
    sp = savesp;
}

/*
 * Print out the contents of an array.
 * Haven't quite figured out what the best format is.
 *
 * This is rather inefficient.
 *
 * The "2*elsize" is there since "printval" drops the stack by elsize.
 */

public printarray(a)
Symbol a;
{
    Stack *savesp, *newsp;
    Symbol eltype;
    long elsize;
    String sep;

    savesp = sp;
    sp -= (size(a));
    newsp = sp;
    eltype = rtype(a->type);
    elsize = size(eltype);
    printf("(");
    if (eltype->class == RECORD or eltype->class == ARRAY or
      eltype->class == VARNT) {
	sep = "\n";
	putchar('\n');
    } else {
	sep = ", ";
    }
    for (sp += elsize; sp <= savesp; sp += 2*elsize) {
	if (sp - elsize != newsp) {
	    fputs(sep, stdout);
	}
	printval(eltype);
    }
    sp = newsp;
    if (streq(sep, "\n")) {
	putchar('\n');
    }
    printf(")");
}

/*
 * Print out the value of a real number in Pascal notation.
 * This is, unfortunately, different than what one gets
 * from "%g" in printf.
 */

public prtreal(r)
double r;
{
    extern char *index();
    char buf[256];

    sprintf(buf, "%g", r);
    if (buf[0] == '.') {
	printf("0%s", buf);
    } else if (buf[0] == '-' and buf[1] == '.') {
	printf("-0%s", &buf[1]);
    } else {
	printf("%s", buf);
    }
    if (index(buf, '.') == nil) {
	printf(".0");
    }
}

/* jlr006
 * Print out an "H" format floating point number.
 * For now, punt and just dump it in hex.  Should snarf Fortran's
 * routine, or wait until vcc handles both dbx and long float.
 */

public prth(h)
struct hfloat h;
{
    printf("0x%04x%04x%04x%04x", 
	h.exponent, h.frac1, h.frac2, h.frac3);
}							

/*
 * Print out a character using ^? notation for unprintables.
 */

public printchar(c)
char c;
{
    if (c == 0) {
	putchar('\\');
	putchar('0');
    } else if (c == '\n') {
	putchar('\\');
	putchar('n');
    } else if (c > 0 and c < ' ') {
	putchar('^');
	putchar(c - 1 + 'A');
    } else if (c >= ' ' && c <= '~') {
	putchar(c);
    } else {
	printf("\\0%o",c);
    }
}

/*
 * Print out a value for a range type (integer, char, or boolean).
 */

public printRangeVal (val, t)
long val;
Symbol t;
{
    if (t == t_boolean->type or istypename(t->type, "boolean")) {
	if ((boolean) val) {
	    printf("true");
	} else {
	    printf("false");
	}
    } else if (t == t_char->type or istypename(t->type, "char")) {
	if (varIsSet("$hexchars")) {
	    printf("0x%lx", val);
	} else {
	    putchar('\'');
	    printchar(val);
	    putchar('\'');
	}
    } else if (varIsSet("$hexints")) {
	printf("0x%lx", val);
    } else if (t->symvalue.rangev.lower >= 0) {
	printf("%lu", val);
    } else {
	printf("%ld", val);
    }
}

/*
 * Print out an enumerated value by finding the corresponding
 * name in the enumeration list.
 */

public printEnum (i, t)
integer i;
Symbol t;
{
    register Symbol e;

    e = t->chain;
    while (e != nil and e->symvalue.constval->value.lcon != i) {
	e = e->chain;
    }
    if (e != nil) {
	printf("%s", symname(e));
    } else {
	printf("%d", i);
    }
}

/*
 * Print out a null-terminated string (pointer to char)
 * starting at the given address.
 */

public printString (addr, quotes)
Address addr;
boolean quotes;
{
    register Address a;
    register integer i, len;
    register boolean endofstring;
    union {
	char ch[sizeof(Word)];
	int word;
    } u;

    if (varIsSet("$hexstrings")) {
	printf("0x%x", addr);
    } else {
	if (quotes) {
	    putchar('"');
	}
	a = addr;
	endofstring = false;
	while (not endofstring) {
	    dread(&u, a, sizeof(u));
	    i = 0;
	    do {
		if (u.ch[i] == '\0') {
		    endofstring = true;
		} else {
		    printchar(u.ch[i]);
		}
		++i;
	    } while (i < sizeof(Word) and not endofstring);
	    a += sizeof(Word);
	}
	if (quotes) {
	    putchar('"');
	}
    }
}
