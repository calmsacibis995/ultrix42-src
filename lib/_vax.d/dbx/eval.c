/************************************************************************
 *									*
 *			Copyright (c) 1986 by				*
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
 *  011	- Added support for vectors.							*
 *		  (L Miller, 18JAN90)									*
 *									*
 *	010 - Fixed 009: check for FIELD before isbitfield.		*
 *	      (jlr, April 13, 1989)					*
 *									*
 *	009 - Added code to handle bitfields to assign().		*
 *	      (Jon Reeves, January 21, 1989)				*
 *									*
 *	008 - Merged in 4.3 changes.					*
 *	      (vjh, April 29, 1986)					*
 *									*
 *	007 - Fixed another bug in trace.				*
 *	      (vjh, August 7, 1985)					*
 *									*
 *	006 - Fixed bugs/problems related to tracing (search for 006).	*
 *	      (vjh, July 23, 1985)					*
 *									*
 *	005 - Added test under O_CATCH and O_IGNORE in eval(), to	*
 *	      insure that the signal number is within the		*
 *	      appropriate range.					*
 *	      (vjh, June 14, 1985)					*
 *									*
 *	004 - Removed test after calling firstaddr() in eval(), under	*
 *	      the O_LIST case.  Testing for source is done by check()   *
 *	      immediately after the O_LIST node is built.		*
 *	      (vjh, May 30, 1985)					*
 *									*
 *	003 - Removed gripe().						*
 *	      (vjh, May 10, 1985)					*
 *									*
 *	002 - Added support for "delete *".				*
 *	      (vjh, April 25, 1985)					*
 *									*
 *	001 - Require a flag INHOUSE to be defined to allow commands 	*
 *	      "psym", "gripe", and "debug" to work.  These are all 	*
 *	      tools used in debugging dbx.				*
 *	      (Victoria Holt, April 15, 1985)				*
 *									*
 ************************************************************************/

/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/* Based on: "@(#)eval.c	5.3 (Berkeley) 6/21/85" */
#ifndef lint
static char sccsid[] = "@(#)eval.c	4.2	ULTRIX	11/9/90";
#endif not lint
/*
static char rcsid[] = "$Header: eval.c,v 1.5 84/12/26 10:39:08 linton Exp $";
*/
/*
 * Tree evaluation.
 */

#include "defs.h"
#include "tree.h"
#include "operators.h"
#include "debug.h"
#include "eval.h"
#include "events.h"
#include "symbols.h"
#include "scanner.h"
#include "source.h"
#include "object.h"
#include "main.h"
#include "mappings.h"
#include "process.h"
#include "runtime.h"
#include "machine.h"
#include <signal.h>

#ifndef public

#include "machine.h"

#define STACKSIZE 20000

typedef Char Stack;

#define push(type, value) { \
    ((type *) (sp += sizeof(type)))[-1] = (value); \
}

#define pop(type) ( \
    (*((type *) (sp -= sizeof(type)))) \
)

#define popn(n, dest) { \
    sp -= n; \
    bcopy(sp, dest, n); \
}

#define alignstack() { \
    sp = (Stack *) (( ((int) sp) + sizeof(int) - 1)&~(sizeof(int) - 1)); \
}

#endif

public Stack stack[STACKSIZE];
public Stack *sp = &stack[0];
public Boolean useInstLoc = false;

#define chksp() \
{ \
    if (sp < &stack[0]) { \
	panic("stack underflow"); \
    } \
}

#define poparg(n, r, fr) { \
    eval(p->value.arg[n]); \
    if (isreal(p->op)) { \
	if (size(p->value.arg[n]->nodetype) == sizeof(float)) { \
	    fr = pop(float); \
	} else { \
	    fr = pop(double); \
	} \
    } else if (isint(p->op)) { \
	r = popsmall(p->value.arg[n]->nodetype); \
    } \
}

#define Boolrep char	/* underlying representation type for booleans */

/*
 * Command-level evaluation.
 */

public Node topnode;

public topeval (p)
Node p;
{
    if (traceeval) {
	fprintf(stderr, "topeval(");
	prtree(stderr, p);
	fprintf(stderr, ")\n");
	fflush(stderr);
    }
    topnode = p;
    eval(p);
}

/*
 * Evaluate a parse tree leaving the value on the top of the stack.
 */

public eval(p)
register Node p;
{
    long r0, r1;
    double fr0, fr1;
    Address addr;
    long i, n;
    int len;
    Symbol s;
    Node n1, n2;
    Boolean b, callflag;
    File file;
    String str;
    Vreg v;

    checkref(p);
    if (traceeval) {
	fprintf(stderr, "begin eval %s\n", opname(p->op));
    }
    switch (degree(p->op)) {
	case BINARY:
	    poparg(1, r1, fr1);
	    poparg(0, r0, fr0);
	    break;

	case UNARY:
	    poparg(0, r0, fr0);
	    break;

	default:
	    /* do nothing */;
    }
    switch (p->op) {
	case O_SYM:
	    s = p->value.sym;
	    if (s == retaddrsym) {
			push(long, return_addr());
	    } else if (isvariable(s)) {
		    if (s != program and not isactive(container(s))) {
				error("\"%s\" is not active", symname(s));
		    }
			if (isvarparam(s) and not isopenarray(s)) {
		    	rpush(address(s, nil), sizeof(Address));
	        } else {
		    	push(Address, address(s, nil));
			}
		} else if (isblock(s)) {
		    push(Symbol, s);
	    } else if (isconst(s)) {
			eval(constval(s));
		} else {
		    error("can't evaluate a %s", classname(s));
		}
	    break;

	case O_VREG:
	    s = p->value.sym;
	    if (not (p->nodetype == t_addr)) {
	        /* Start vector support */
		if (not canpush(sizeof (struct Vreg))) {
		    error("expression too large to evaluate");
		} else {
		    chksp();
		    v = vreg(s->symvalue.raddr.reg);
		    switch(s->symvalue.raddr.reg)
		    {
			case VMR:
		    	    push(Vquad, *(Vquad *)v);
			    break;
			case VCR:
			case VAER:
			case VLR:
		    	    push(long, (long)v);
			    break;
		    default:
			    push(struct Vreg, *v);
			    break;
		    }
		}
		/* End of vector support */
	    } else {
	        push(Address, address(s, nil));
	    }
	    break;

	case O_LCON:
	case O_CCON:
	    r0 = p->value.lcon;
	    pushsmall(p->nodetype, r0);
	    break;

	case O_FCON:
	    push(double, p->value.fcon);
	    break;

	case O_SCON:
	    len = size(p->nodetype);
	    mov(p->value.scon, sp, len);
	    sp += len;
	    break;

	case O_INDEX:
	    s = p->value.arg[0]->nodetype;
	    p->value.arg[0]->nodetype = t_addr;
	    eval(p->value.arg[0]);
	    p->value.arg[0]->nodetype = s;
	    n = pop(Address);
	    eval(p->value.arg[1]);
	    evalindex(s, n, popsmall(p->value.arg[1]->nodetype));
	    break;

	case O_DOT:
	    s = p->value.arg[1]->value.sym;
	    eval(p->value.arg[0]);
	    n = pop(long);
	    push(long, n + (s->symvalue.field.offset div 8));
	    break;

	/*
	 * Get the value of the expression addressed by the top of the stack.
	 * Push the result back on the stack.
	 */

	case O_INDIR:
	case O_RVAL:
	    addr = pop(long);
	    if (addr == 0) {
			error("reference through nil pointer");
	    }
        len = size(p->nodetype);
	    /* Start vector support */
	    if (p->nodetype == t_vquad) {
	        push(Vquad, *(Vquad *) addr);
	    /* End of vector support */
	    } else
	        rpush(addr, len);
	    break;

	/*
	 * Move the stack pointer so that the top of the stack has
	 * something corresponding to the size of the current node type.
	 * If this new type is bigger than the subtree (len > 0),
	 * then the stack is padded with nulls.  If it's smaller,
	 * the stack is just dropped by the appropriate amount.
	 */
	case O_TYPERENAME:
		if(p->nodetype->type == t_vint ||
				p->nodetype->type->type == t_vint) { 
			if(p->value.arg[0]->nodetype->type->type != t_vquad) {
				if(streq(ident(p->value.arg[0]->nodetype->name), "$vmr")) {
					sp -= 8;
				}
				else {
					sp -= size(p->value.arg[0]->nodetype);
				}
				error("rename to vint illegal in this context");
			}
		}
		if(p->nodetype->type == t_vhex ||
				p->nodetype->type->type == t_vhex) { 
			if(p->value.arg[0]->nodetype->type->type != t_vquad) {
				if(streq(ident(p->value.arg[0]->nodetype->name), "$vmr")) {
					sp -= 8;
				}
				else {
					sp -= size(p->value.arg[0]->nodetype);
				}
				error("rename to vhex illegal in this context");
			}
		}
		if(p->nodetype->type == t_vfloat ||
				p->nodetype->type->type == t_vfloat) { 
			if(p->value.arg[0]->nodetype->type->type != t_vquad) {
				if(streq(ident(p->value.arg[0]->nodetype->name), "$vmr")) {
					sp -= 8;
				}
				else {
					sp -= size(p->value.arg[0]->nodetype);
				}
				error("rename to vfloat illegal in this context");
			}
		}
	    len = size(p->nodetype) - size(p->value.arg[0]->nodetype);
	    if (len > 0) {
			for (n = 0; n < len; n++) {
		    	*sp++ = '\0';
			}
	    } else if (len < 0) {
			sp -= len;
	    }
	    break;

	case O_COMMA:
	    eval(p->value.arg[0]);
	    if (p->value.arg[1] != nil) {
		eval(p->value.arg[1]);
	    }
	    break;

	case O_ITOF:
	    push(double, (double) r0);
	    break;

	case O_ADD:
	    push(long, r0+r1);
	    break;

	case O_ADDF:
	    push(double, fr0+fr1);
	    break;

	case O_SUB:
	    push(long, r0-r1);
	    break;

	case O_SUBF:
	    push(double, fr0-fr1);
	    break;

	case O_NEG:
	    push(long, -r0);
	    break;

	case O_NEGF:
	    push(double, -fr0);
	    break;

	case O_MUL:
	    push(long, r0*r1);
	    break;

	case O_MULF:
	    push(double, fr0*fr1);
	    break;

	case O_DIVF:
	    if (fr1 == 0) {
		error("error: division by 0");
	    }
	    push(double, fr0 / fr1);
	    break;

	case O_DIV:
	    if (r1 == 0) {
		error("error: div by 0");
	    }
	    push(long, r0 div r1);
	    break;

	case O_MOD:
	    if (r1 == 0) {
		error("error: mod by 0");
	    }
	    push(long, r0 mod r1);
	    break;

	case O_LT:
	    push(Boolrep, r0 < r1);
	    break;

	case O_LTF:
	    push(Boolrep, fr0 < fr1);
	    break;

	case O_LE:
	    push(Boolrep, r0 <= r1);
	    break;

	case O_LEF:
	    push(Boolrep, fr0 <= fr1);
	    break;

	case O_GT:
	    push(Boolrep, r0 > r1);
	    break;

	case O_GTF:
	    push(Boolrep, fr0 > fr1);
	    break;

	case O_GE:
	    push(Boolrep, r0 >= r1);
	    break;

	case O_GEF:
	    push(Boolrep, fr0 <= fr1);

	case O_EQ:
	    push(Boolrep, r0 == r1);
	    break;

	case O_EQF:
	    push(Boolrep, fr0 == fr1);
	    break;

	case O_NE:
	    push(Boolrep, r0 != r1);
	    break;

	case O_NEF:
	    push(Boolrep, fr0 != fr1);
	    break;

	case O_AND:
	    push(Boolrep, r0 and r1);
	    break;

	case O_OR:
	    push(Boolrep, r0 or r1);
	    break;

	case O_ASSIGN:
	    assign(p->value.arg[0], p->value.arg[1]);
	    break;

	case O_CHFILE:
	    if (p->value.scon == nil) {
		printf("%s\n", cursource);
	    } else {
		file = opensource(p->value.scon);
		if (file == nil) {
		    error("can't read \"%s\"", p->value.scon);
		} else {
		    fclose(file);
		    setsource(p->value.scon);
		}
	    }
	    break;

	case O_CONT:
	    cont(p->value.lcon);
	    printnews();
	    break;

	case O_LIST:
	    list(p);
	    break;

	case O_FUNC:
	    func(p->value.arg[0]);
	    break;

	case O_EXAMINE:
	    eval(p->value.examine.beginaddr);
	    r0 = pop(long);
		if(p->value.examine.beginaddr->op == O_QLINE) {
		 	r0 =  objaddr(r0, cursource);	
			if(r0 == NOADDR || r0 == ADDRNOEXEC){
					beginerrmsg();
					if (r0 == NOADDR)
		    			fprintf(stderr, "beyond end of file at line ");
					else
						fprintf(stderr, "no executable code found at line ");
					prtree(stderr, p->value.examine.beginaddr);
					enderrmsg();
				break;
			}
		}
	    if (p->value.examine.endaddr == nil) {
		n = p->value.examine.count;
		if (n == 0) {
		    printvalue(r0, p->value.examine.mode);
		} else if (streq(p->value.examine.mode, "i")) {
		    printninst(n, (Address) r0);
		} else {
		    printndata(n, (Address) r0, p->value.examine.mode);
		}
	    } else {
		eval(p->value.examine.endaddr);
		r1 = pop(long);
		if (streq(p->value.examine.mode, "i")) {
		    printinst((Address)r0, (Address)r1);
		} else {
		    printdata((Address)r0, (Address)r1, p->value.examine.mode);
		}
	    }
	    break;

	case O_TMASK:
	case O_FMASK:
	{
		Symbol t;
		
	    for (n1 = p->value.arg[0]; n1 != nil; n1 = n1->value.arg[1]) {
			t = n1->value.arg[0]->nodetype;
			if(((streq(ident(t->name), "$vquad")) ||
						(streq(ident(t->name), "$vmr")) ||
						(streq(ident(t->name), "$vcr")) ||
						(streq(ident(t->name), "$vaer")) ||
						(streq(ident(t->name), "$vlr")))) {
				error("incorrect symbol for mask operation");
			}
			if((p->value.arg[1]) && (p->value.arg[1]->op != O_SCON)) {
				error("illegal mask");
				break;
			}
			if(n1->value.arg[0]->op != O_VREG) {
				error("rename illegal for mask operation");
				break;
			}
			eval(n1->value.arg[0]);
			printval_with_mask(n1->value.arg[0]->nodetype, p->op,
													p->value.arg[1]);
	    }
	    putchar('\n');
	    break;
	}

	case O_PRINT:
	    for (n1 = p->value.arg[0]; n1 != nil; n1 = n1->value.arg[1]) {
		eval(n1->value.arg[0]);
		printval(n1->value.arg[0]->nodetype);
		putchar(' ');
	    }
	    putchar('\n');
	    break;

	case O_PRINTF:
#define MAXARGLIST 200
	    { 
			char *argv[MAXARGLIST];
	      	int i;
	      	long nsize;

			for (i=0, n1=p->value.arg[0]; n1!=nil; n1=n1->value.arg[1]) {
				if((i == 0) && (n1->value.arg[0]->op != O_SCON)) {
					error("missing format");
				}
		    	if(n1->nodetype->class == ARRAY) {
					argv[i++] = sp;
					eval(n1->value.arg[0]);
		    	} else if ((nsize = size(n1->value.arg[0]->nodetype))> 4) {
					if (nsize/4 > MAXARGLIST)
			    		error ("printf argument size (%d) exceeds max(%d)\n",
														  nsize, 4*MAXARGLIST);
					eval(n1->value.arg[0]);
					nsize = (nsize + 3) & ~3;	/* round */
					popn (nsize, &argv[i]);
					i += nsize / 4;
		    	} else {
					eval(n1->value.arg[0]);
					argv[i++] =(char *)popsmall(n1->value.arg[0]->nodetype);
		    	}
			}
			argv[i] = NULL;
			print_formatted(p, argv);
			for (i=0, n1=p->value.arg[0]; n1!=nil; i++,n1=n1->value.arg[1]){
		    	if(n1->nodetype->class == ARRAY) {
					sp -= size(n1->nodetype);
		    	}
			}
	    }
	    break;

	case O_PSYM:
#	    ifdef INHOUSE
	    	if (p->value.arg[0]->op == O_SYM) {
		    psym(p->value.arg[0]->value.sym);
	    	} else {
		    psym(p->value.arg[0]->nodetype);
	    	}
#	    else
		error("\"psym\" is not supported");
#	    endif
	    break;

	case O_QLINE:
	    eval(p->value.arg[1]);
	    break;

	case O_STEPV:
	    b = inst_tracing;
	    inst_tracing = (Boolean) (not p->value.step.source);
	    if (p->value.step.skipcalls) {
			nextv();
	    } else {
			stepv();
	    }
	    inst_tracing = b;
	    useInstLoc = (Boolean) (not p->value.step.source);
	    printnews();
	    break;

	case O_STEP:
	    b = inst_tracing;
	    inst_tracing = (Boolean) (not p->value.step.source);
	    if (p->value.step.skipcalls) {
		next();
	    } else {
		stepc();
	    }
	    inst_tracing = b;
	    useInstLoc = (Boolean) (not p->value.step.source);
	    printnews();
	    break;

	case O_SET:
		if(p->value.arg[0] == nil) {
	    	set(p->value.arg[0], p->value.arg[1]);
		} else {
			if(p->value.arg[0]->value.name->identifier[0] != '$') {
				switch(p->value.arg[1]->op) {
					case O_LCON:
						addr = p->value.arg[1]->value.lcon;
						break;

					case O_SYM:
						addr = address(p->value.arg[1]->value.sym, nil);
						break;

					case O_DOT:
						eval(p->value.arg[1]);
						addr= pop(long);
						break;
				
					default:
						error("expression must resolve to an address"); 
				}
						
    			s = insert(p->value.arg[0]->value.name);
    			s->language = findlanguage(ASSEMBLER);	/* RBN 2-27-90 */
    			s->class = VAR;
    			s->type = t_int;
    			s->level = program->level;
    			s->block = curblock;
				s->symvalue.offset = addr;
			} else {
	    		set(p->value.arg[0], p->value.arg[1]);
			}
		}
		break;

	case O_WHATIS:
	    if (p->value.arg[0]->op == O_SYM) {
		printdecl(p->value.arg[0]->value.sym);
	    } else {
		printdecl(p->value.arg[0]->nodetype);
	    }
	    break;

	case O_WHERE:
	    wherecmd();
	    break;

	case O_WHEREIS:
	    if (p->value.arg[0]->op == O_SYM) {
		printwhereis(stdout, p->value.arg[0]->value.sym);
	    } else {
		printwhereis(stdout, p->value.arg[0]->nodetype);
	    }
	    break;

	case O_WHICH:
	    if (p->value.arg[0]->op == O_SYM) {
		printwhich(stdout, p->value.arg[0]->value.sym);
	    } else {
		printwhich(stdout, p->value.arg[0]->nodetype);
	    }
	    putchar('\n');
	    break;
	
	case O_ALIAS:
	    n1 = p->value.arg[0];
	    n2 = p->value.arg[1];
	    if (n2 == nil) {
	    if (n1 == nil) {
		    alias(nil, nil, nil, false);
	    } else {
		    alias(n1->value.name, nil, nil, false);
	    }
	    } else if (n2->op == O_NAME) {
		str = ident(n2->value.name);
		alias(n1->value.name, nil, strdup(str), false);
	    } else {
		if (n1->op == O_COMMA) {
		    alias(
			n1->value.arg[0]->value.name,
			(List) n1->value.arg[1],
			n2->value.scon,
			false
		    );
		} else {
		    alias(n1->value.name, nil, n2->value.scon, false);
		}
	    }
	    break;

	case O_UNALIAS:
	    unalias(p->value.arg[0]->value.name);
	    break;

	case O_CALLPROC:
	    callproc(p, false);
	    break;

	case O_CALL:
	    callproc(p, true);
	    break;

	case O_CATCH:
	    if (p->value.lcon == 0) {
		printsigscaught(process);
	    } else {
		psigtrace(process, p->value.lcon, true);
	    }
	    break;

	case O_CD:
	    do_chdir(p->value.scon);
	    break;

	case O_RECORD:
	    set_record(p->value.scon);
	    break;

	case O_EDIT:
	    edit(p->value.scon);
	    break;

        case O_DEBUG:
#	    ifdef INHOUSE
            	debug(p);
#	    else
		error("\"debug\" is not supported");
#	    endif
	    break;

	case O_DOWN:
	    checkref(p->value.arg[0]);
	    assert(p->value.arg[0]->op == O_LCON);
	    down(p->value.arg[0]->value.lcon);
	    break;

	case O_DUMP:
	    if (p->value.arg[0] == nil) {
		dumpall();
	    } else {
		s = p->value.arg[0]->value.sym;
		if (s == curfunc) {
		    dump(nil);
		} else {
		    dump(s);
		}
	    }
	    break;

	case O_GRIPE:
	    error ("%s\n%s",
	        "please submit a written SPR, or",
	        "contact your nearest service representative");
	    break;

	case O_GETENV:
	    get_env(p->value.arg[0]);
	    break;

	case O_HELP:
	    help();
	    break;

	case O_IGNORE:
	    if (p->value.lcon == 0) {
		printsigsignored(process);
	    } else {
		psigtrace(process, p->value.lcon, false);
	    }
	    break;

	case O_RETURN:
	    if (p->value.arg[0] == nil) {
		rtnfunc(nil);
	    } else {
		assert(p->value.arg[0]->op == O_SYM);
		rtnfunc(p->value.arg[0]->value.sym);
	    }
	    break;

	case O_RUN:
	    run();
	    break;

	case O_SETENV:
	    set_env(p->value.arg[0], p->value.arg[1]);
	    break;

	case O_SEARCH:
	    search(p->value.arg[0]->value.lcon, p->value.arg[1]->value.scon);
	    break;

	case O_SOURCE:
	    setinput(p->value.scon);
	    break;

	case O_STATUS:
	    status();
	    break;

	case O_TRACE:
	case O_TRACEI:
	    trace(p);
	    break;

	case O_STOP:
	case O_STOPI:
	    stop(p);
	    break;

	case O_UNSET:
	    undefvar(p->value.arg[0]->value.name);
	    break;

	case O_UP:
	    checkref(p->value.arg[0]);
	    assert(p->value.arg[0]->op == O_LCON);
	    up(p->value.arg[0]->value.lcon);
	    break;

	case O_ADDEVENT:
	    addevent(p->value.event.cond, p->value.event.actions);
	    break;

	case O_DELETE:
	    n1 = p->value.arg[0];
	    if (n1 == nil) {
	        delallevents();
	    } else {
	        while (n1->op == O_COMMA) {
		    n2 = n1->value.arg[0];
		    assert(n2->op == O_LCON);
	            if (not delevent((unsigned int) n2->value.lcon)) {
		        warning("unknown event %ld", n2->value.lcon);
	            }
		    n1 = n1->value.arg[1];
	        }
	        assert(n1->op == O_LCON);
	        if (not delevent((unsigned int) n1->value.lcon)) {
		    warning("unknown event %ld", n1->value.lcon);
	        }
	    }
	    break;

	case O_ENDX:
	    endprogram();
	    break;

	case O_IF:
	    if (cond(p->value.event.cond)) {
		evalcmdlist(p->value.event.actions);
	    }
	    break;

	case O_ONCE:
	    event_once(p->value.event.cond, p->value.event.actions);
	    break;

	case O_PRINTCALL:
	    printcall(p->value.sym, whatblock(return_addr()));
	    break;

	/* O_PRINTENTRY node is basically the same as O_TRACEON.  However,
	 * it insures that *initial* routine entry is noted.  It has to
	 * be handled specially because initial entry is encountered as
 	 * a breakpoint, rather than single-stepping to it and knowing
	 * that your're at a calls/g instruction (see nextaddr(), 
	 * machine.c).  (006 - vjh)
	 */
	case O_PRINTENTRY:
	    if (not single_stepping) {  /* traceon() sets single_stepping  */
	        callflag = true;	/* so test here first.		   */
	    } else {
	        callflag = false;
	    }
	    traceon(p->value.trace.inst, p->value.trace.event,
		p->value.trace.actions);

	    /* Test callflag.  If we are single stepping already, then      */
	    /* no need to callnews(); it will be done by nextaddr() in      */
	    /* machine.c.  Otherwise, we got here by reaching a breakpoint, */
	    /* and we want to note the occasion. */
	    if (callflag) {
	        callnews(true);
	    }
	    break;

	case O_PRINTIFCHANGED:
	    printifchanged(p->value.arg[0]);
	    break;

	case O_PRINTRTN:
            printrtn(p->value.sym);
	    break;

	case O_PRINTSRCPOS:
	    getsrcpos();
	    if (p->value.arg[0] == nil) {
		printsrcpos();
		putchar('\n');
		printlines(curline, curline);
	    } else if (p->value.arg[0]->op == O_QLINE) {
		if ((p->value.arg[0]->value.arg[1]->value.lcon == 0) or 
							(p->value.arg[0]->value.arg[0] == nil)) {
		    printf("tracei: ");
		    printinst(pc, pc);
		} else {
		    if (canReadSource()) {
		    printf("trace:  ");
		    printlines(curline, curline);
		}
		}
	    } else {
		printsrcpos();
		printf(": ");
		eval(p->value.arg[0]);
		prtree(stdout, p->value.arg[0]);
		printf(" = ");
		printval(p->value.arg[0]->nodetype);
		putchar('\n');
	    }
	    break;

	case O_PROCRTN:
	    procreturn(p->value.sym);
	    break;

	case O_PWD:
		do_pwd();
	    break;

	case O_STOPIFCHANGED:
	    stopifchanged(p->value.arg[0]);
	    break;

	case O_STOPX:
	    isstopped = true;
	    break;

	case O_TRACEON:
	    traceon(p->value.trace.inst, p->value.trace.event,
		p->value.trace.actions);
	    break;

	case O_TRACEOFF:
	    traceoff(p->value.lcon);
	    break;

	default:
	    panic("eval: bad op %d", p->op);
    }
    if (traceeval) { 
	fprintf(stderr, "end eval %s\n", opname(p->op));
 }
}

/*
 * Evaluate a list of commands.
 */

public evalcmdlist(cl)
Cmdlist cl;
{
    Command c;

    foreach (Command, c, cl)
	evalcmd(c);
    endfor
}

/*
 * Push "len" bytes onto the expression stack from address "addr"
 * in the process.  If there isn't room on the stack, print an error message.
 */

public rpush(addr, len)
Address addr;
int len;
{
    if (not canpush(len)) {
	error("expression too large to evaluate");
    } else {
	chksp();
        dread(sp, addr, len);
	sp += len;
    }
}

/*
 * Check if the stack has n bytes available.
 */

public Boolean canpush(n)
Integer n;
{
    return (Boolean) (sp + n < &stack[STACKSIZE]);
}

/*
 * Push a small scalar of the given type onto the stack.
 */

public pushsmall(t, v)
Symbol t;
long v;
{
    register Integer s;

    s = size(t);
    switch (s) {
	case sizeof(char):
	    push(char, v);
	    break;

	case sizeof(short):
	    push(short, v);
	    break;

	case sizeof(long):
	    push(long, v);
	    break;

	default:
	    panic("bad size %d in popsmall", s);
    }
}

/*
 * Pop an item of the given type which is assumed to be no larger
 * than a long and return it expanded into a long.
 */

public long popsmall(t)
Symbol t;
{
    register integer n;
    long r;

    n = size(t);
    if (n == sizeof(char)) {
	if (t->class == RANGE and t->symvalue.rangev.lower >= 0) {
	    r = (long) pop(unsigned char);
	} else {
	    r = (long) pop(char);
	}
    } else if (n == sizeof(short)) {
	if (t->class == RANGE and t->symvalue.rangev.lower >= 0) {
	    r = (long) pop(unsigned short);
	} else {
	    r = (long) pop(short);
	}
    } else if (n == sizeof(long)) {
	    r = pop(long);
    } else {
	error("[internal error: size %d in popsmall]", n);
    }
    return r;
}

/*
 * Evaluate a conditional expression.
 */

public Boolean cond(p)
Node p;
{
    register Boolean b;

    if (p == nil) {
	b = true;
    } else {
	eval(p);
	b = (Boolean) pop(Boolrep);
    }
    return b;
}

/*
 * Return the address corresponding to a given tree.
 */

public Address lval(p)
Node p;
{
    if (p->op == O_RVAL) {
	eval(p->value.arg[0]);
    } else {
	eval(p);
    }
    return (Address) (pop(long));
}

/*
 * Process a trace command, translating into the appropriate events
 * and associated actions.
 */

public trace(p)
Node p;
{
    Node exp, place, cond;
    Node left;

    exp = p->value.arg[0];
    place = p->value.arg[1];
    cond = p->value.arg[2];
    if (exp == nil) {
	traceall(p->op, place, cond);
    } else if (exp->op == O_QLINE or exp->op == O_LCON) {
	traceinst(p->op, exp, cond);
    } else if (place != nil and place->op == O_QLINE) {
	traceat(p->op, exp, place, cond);
    } else {
	left = exp;
	if (left->op == O_RVAL or left->op == O_CALL) {
	    left = left->value.arg[0];
	}
	if (left->op == O_SYM and isblock(left->value.sym)) {
	    traceproc(p->op, left->value.sym, place, cond);
	} else {
	    tracedata(p->op, exp, place, cond);
	}
    }
}

/*
 * Set a breakpoint that will turn on tracing.
 */

private traceall(op, place, cond)
Operator op;
Node place;
Node cond;
{
    Symbol s;
    Node event;
    Command action;

    /* trace:  Special hack for FORTRAN when tracing entire program.
     * f77 startup prolog is without source information (/lib/crt0.o).
     * Setting a tmp. breakpoint at "program" (0x02) and then dissassembling
     * to the first source line consequently did not work:  the call to
     * main() was stepped over (nosource and canskip, nextaddr() in 
     * machine.c); since main() calls MAIN(), MAIN() was missed as well.
     * The hack is to make this appear as if the user has requested
     * "trace in MAIN".  (006 - vjh)
     */
    if (place == nil) {
        s = findproc("MAIN");
	if (s == nil) {
	    s = program;
	}
    } else {
        s = place->value.sym;
    }
    event = build(O_EQ, build(O_SYM, procsym), build(O_SYM, s));
    action = build(O_PRINTSRCPOS,
	build(O_QLINE, nil, build(O_LCON, (op == O_TRACE) ? 1 : 0)));
    if (cond != nil) {
	action = build(O_IF, cond, buildcmdlist(action));
    }
    /* Build O_PRINTENTRY rather than O_TRACEON, so that initial routine
     * entry is announced.  (006 - vjh)
     */
    action = build(O_TRACEON, (op == O_TRACEI), buildcmdlist(action));
    action->value.trace.event = addevent(event, buildcmdlist(action));
    if (isstdin()) {
	printevent(action->value.trace.event);
    }
}

/*
 * Set up the appropriate breakpoint for tracing an instruction.
 */

private traceinst(op, exp, cond)
Operator op;
Node exp;
Node cond;
{
    Node event, wh;
    Command action;
    Event e;

    if (exp->op == O_LCON) {
	wh = build(O_QLINE, build(O_SCON, strdup(cursource)), exp);
    } else {
	wh = exp;
    }
    if (op == O_TRACEI) {
	wh = build(O_QLINE, nil, exp);
	event = build(O_EQ, build(O_SYM, pcsym), wh);
    } else {
	event = build(O_EQ, build(O_SYM, linesym), wh);
    }
    action = build(O_PRINTSRCPOS, wh);
    if (cond) {
	action = build(O_IF, cond, buildcmdlist(action));
    }
    e = addevent(event, buildcmdlist(action));
    if (isstdin()) {
	printevent(e);
    }
}

/*
 * Set a breakpoint to print an expression at a given line or address.
 */

private traceat(op, exp, place, cond)
Operator op;
Node exp;
Node place;
Node cond;
{
    Node event;
    Command action;
    Event e;

    if (op == O_TRACEI) {
	event = build(O_EQ, build(O_SYM, pcsym), place);
    } else {
	event = build(O_EQ, build(O_SYM, linesym), place);
    }
    action = build(O_PRINTSRCPOS, exp);
    if (cond != nil) {
	action = build(O_IF, cond, buildcmdlist(action));
    }
    e = addevent(event, buildcmdlist(action));
    if (isstdin()) {
	printevent(e);
    }
}

/*
 * Construct event for tracing a procedure.
 *
 * What we want here is
 *
 * 	when $proc = p do
 *	    if <condition> then
 *	        printcall;
 *	        once $pc = $retaddr do
 *	            printrtn;
 *	        end;
 *	    end if;
 *	end;
 *
 * Note that "once" is like "when" except that the event
 * deletes itself as part of its associated action.
 */

private traceproc(op, p, place, cond)
Operator op;
Symbol p;
Node place;
Node cond;
{
    Node event;
    Command action;
    Cmdlist actionlist;
    Event e;

    /* check for error conditions */
    if (place != nil)
		error ("place cannot be specified with procedure trace");

    action = build(O_PRINTCALL, p);
    actionlist = list_alloc();
    cmdlist_append(action, actionlist);
    event = build(O_EQ, build(O_SYM, pcsym), build(O_SYM, retaddrsym));
    action = build(O_ONCE, event, buildcmdlist(build(O_PRINTRTN, p)));
    cmdlist_append(action, actionlist);
    if (cond != nil) {
	actionlist = buildcmdlist(build(O_IF, cond, actionlist));
    }
    event = build(O_EQ, build(O_SYM, procsym), build(O_SYM, p));
    e = addevent(event, actionlist);
    if (isstdin()) {
	printevent(e);
    }
}

/*
 * Set up breakpoint for tracing data.
 */

private tracedata(op, exp, place, cond)
Operator op;
Node exp;
Node place;
Node cond;
{
    Symbol p;
    Node event;
    Command action;

	p = exp->value.sym;
	if((p->level == -3) or (streq(ident(exp->nodetype->name), "$vquad"))) {
		error("can't trace a register value");
	}
    p = (place == nil) ? tcontainer(exp) : place->value.sym;
    if (p == nil) {
	p = program;
    }
    action = build(O_PRINTIFCHANGED, exp);
    if (cond != nil) {
	action = build(O_IF, cond, buildcmdlist(action));
    }
    action = build(O_TRACEON, (op == O_TRACEI), buildcmdlist(action));
    event = build(O_EQ, build(O_SYM, procsym), build(O_SYM, p));
    action->value.trace.event = addevent(event, buildcmdlist(action));
    if (isstdin()) {
	printevent(action->value.trace.event);
    }
}

/*
 * Setting and unsetting of stops.
 */

public stop(p)
Node p;
{
    Node exp, place, cond, t;
    Symbol s;
    Command action;
    Event e;

    exp = p->value.arg[0];
    place = p->value.arg[1];
    cond = p->value.arg[2];
    if (exp != nil) {
	stopvar(p->op, exp, place, cond);
    } else {
	action = build(O_STOPX);
	if (cond != nil) {
	    action = build(O_IF, cond, buildcmdlist(action));
	}
	if (place == nil or place->op == O_SYM) {
	    if (place == nil) {
		s = program;
	    } else {
		s = place->value.sym;
	    }
	    t = build(O_EQ, build(O_SYM, procsym), build(O_SYM, s));
	    if (cond != nil) {
		action = build(O_TRACEON, (p->op == O_STOPI),
		    buildcmdlist(action));
		e = addevent(t, buildcmdlist(action));
		action->value.trace.event = e;
	    } else {
		e = addevent(t, buildcmdlist(action));
	    }
	    if (isstdin()) {
		printevent(e);
	    }
	} else {
	    stopinst(p->op, place, cond, action);
	}
    }
}

private stopinst(op, place, cond, action)
Operator op;
Node place;
Node cond;
Command action;
{
    Node event;
    Event e;

    if (op == O_STOP) {
	event = build(O_EQ, build(O_SYM, linesym), place);
    } else {
	event = build(O_EQ, build(O_SYM, pcsym), place);
    }
    e = addevent(event, buildcmdlist(action));
    if (isstdin()) {
	printevent(e);
    }
}

/*
 * Implement stopping on assignment to a variable by adding it to
 * the variable list.
 */

private stopvar(op, exp, place, cond)
Operator op;
Node exp;
Node place;
Node cond;
{
    Symbol p;
    Node event;
    Command action;

    if (place == nil) {
	if (exp->op == O_LCON) {
	    p = program;
	} else {
	    p = tcontainer(exp);
	    if (p == nil) {
		p = program;
	    }
	}
    } else {
	p = place->value.sym;
    }
    action = build(O_STOPIFCHANGED, exp);
    if (cond != nil) {
	action = build(O_IF, cond, buildcmdlist(action));
    }
    action = build(O_TRACEON, (op == O_STOPI), buildcmdlist(action));
    event = build(O_EQ, build(O_SYM, procsym), build(O_SYM, p));
    action->value.trace.event = addevent(event, buildcmdlist(action));
    if (isstdin()) {
	printevent(action->value.trace.event);
    }
}

/*
 * Assign the value of an expression to a variable (or term).
 */

public assign(var, exp)
Node var;
Node exp;
{
    Address addr;
    integer varsize, expsize;
    integer fieldvalue;
    char cvalue, *vreg_decode_array, *str1, *str2, *str_tmp;
    short svalue;
    long lvalue;
    float fvalue;
    Vquad vquadvalue;
	Boolean scan_failure = false;


    eval(exp);

    if (var->op == O_SYM and regnum(var->value.sym) != -1) {
	setreg(regnum(var->value.sym), pop(Address));
    } else if (var->op == O_VREG) {
		lvalue = 0;
		expsize = size(exp->nodetype);
		popn(expsize, &lvalue);
		setnreg(var->nodetype->symvalue.raddr.reg, lvalue);
    } else {
        addr = lval(var);
		if(var->op != O_LCON && var->value.sym->type->class == PTR)
			dread(&addr, addr, sizeof(Address));
		varsize = size(var->nodetype);
		expsize = size(exp->nodetype);
	
	/* Start vector support */
	if (var->nodetype == t_vquad) {
	    vquadvalue.val[0] = 0;
	    vquadvalue.val[1] = 0;
		if((vreg_decode_array = malloc(expsize)) == nil) {
			sp -= expsize;
			error("unable to malloc memory for decode of vreg");
		}
	    popn(expsize, vreg_decode_array);
		if(str2 = (char *)strchr(vreg_decode_array, ',')) {
			str1 = vreg_decode_array;
			str2[0] =  '\0';
			str2++;
			if(strpbrk(str1, "Xx")) {
				if((str_tmp = (char *)strchr(str1, 'X')) ||
						(str_tmp = (char *)strchr(str1, 'x'))) {
					str_tmp[0] =  '\0';
					str_tmp++;
					if(sscanf(str_tmp, "%lx", &vquadvalue.val[0]) != 1) {
						scan_failure = true;
					}
				}
			}
			else {
				if(strpbrk(str1, "Ee.")) {
					if(sscanf(str1, "%f", &vquadvalue.val[0]) != 1) {
						scan_failure = true;
					}
				}
				else {
					if(sscanf(str1, "%ld", &vquadvalue.val[0]) != 1) {
						scan_failure = true;
					}
				}
			}
			if(strpbrk(str2, "Xx")) {
				if((str_tmp = (char *)strchr(str2, 'X')) ||
						(str_tmp = (char *)strchr(str2, 'x'))) {
					str_tmp[0] =  '\0';
					str_tmp++;
					if(sscanf(str_tmp, "%lx", &vquadvalue.val[1]) != 1) {
						scan_failure = true;
					}
				}
			}
			else {
				if(strpbrk(str2, "Ee.")) {
					if(sscanf(str2, "%f", &vquadvalue.val[1]) != 1) {
						scan_failure = true;
					}
				}
				else {
					if(sscanf(str2, "%ld", &vquadvalue.val[1]) != 1) {
						scan_failure = true;
					}
				}
			}
		}
		else {
			if(strpbrk(vreg_decode_array, "Xx")) {
				if((str2 = (char *)strchr(vreg_decode_array, 'X')) ||
						(str2 = (char *)strchr(vreg_decode_array, 'x'))) {
					str2[0] =  '\0';
					str2++;
				}
				if(sscanf(str2, "%lx", &vquadvalue.val[0]) != 1) {
					scan_failure = true;
				}
			}
			else {
				if(strpbrk(vreg_decode_array, "Ee.")) {
					if(sscanf(vreg_decode_array, "%F",
											&vquadvalue.val[0]) != 1) {
						scan_failure = true;
					}
				}
				else {
					if(sscanf(vreg_decode_array, "%ld", &vquadvalue.val[0]) !=
																			1) {
						scan_failure = true;
					}
				}
			}
		}
		free(vreg_decode_array);
		if(scan_failure) {
			yyerror("syntax error");
		}
		else {
	    	setvreg(addr, &vquadvalue);
		}
	    return;
	}
	else
		if(var->nodetype == t_vmr) {
		lvalue = 0;
		expsize = size(exp->nodetype);
		popn(expsize, &lvalue);
		setvmrreg(addr - (Address )vreg(VMR), lvalue);
	    return;
	}

	/* End vector support */
	if (varsize == sizeof(float) and expsize == sizeof(double)) {
	    fvalue = (float) pop(double);
	    dwrite(&fvalue, addr, sizeof(fvalue));
	} else {
	    if (var->nodetype->class == FIELD && isbitfield(var->nodetype)) {
	        rpush(addr, sizeof(int), t_int);
		fieldvalue = popsmall(t_int);
		depositField(fieldvalue, var->nodetype);
		dwrite(sp-sizeof(int), addr, varsize);
	    } else if (varsize < sizeof(long)) {
	        lvalue = 0;
		popn(expsize, &lvalue);
		if (varsize == sizeof(char)) {
		    cvalue = lvalue;
		    dwrite(&cvalue, addr, sizeof(cvalue));
		} else if (varsize == sizeof(short)) {
		    svalue = lvalue;
		    dwrite(&svalue, addr, sizeof(svalue));
		} else {
		    error("[internal error: bad size %d in assign", varsize);
		}
	  } else {
	      if (expsize <= varsize || var->value.sym->type->class == PTR) {
		  sp -= expsize;
		  dwrite(sp, addr, expsize);
	      } else {
		  sp -= expsize;
		  dwrite(sp, addr, varsize);
	      }
	    }
	}
    }
}

public char *make_string(s, len)
char *s;
int len;
{
    if (!s)
	return(nil);
    return strcpy(malloc((unsigned) strlen(s) + 1 + len), s);
}

extern String getenv();

public do_chdir (string1)
char *string1;
{
	char bad = 0;

	if(string1) {
		if(chdir(string1)) {
			bad++;
		}
	} else {
		String out = getenv("HOME");

		if(out) {
			if(chdir(out)) {
				bad++;
			}
		} else {
			bad++;
		}
	}
	if(bad) {
		error("unable to change directory");
	}
	do_pwd();
}

public do_pwd()
{

	char *cwd, *getcwd();

	if((cwd = getcwd(nil, 256)) == nil) {
		error("pwd failure");
	}
	printf("%s\n", cwd);
	free(cwd);

}

public get_env (string1)
Node string1;
{
	String out = getenv(string1->value.name->identifier);

	if(out == nil)
		error("%s is not present", string1->value.name->identifier);
	else
		error(out);
}

public set_env (string1, string2)
Node string1, string2;
{
	char *str1 = string1->value.name->identifier;
	char *str2;
	char *s;
	
	switch(string2->op)
	{
		case O_NAME:
			str2 = string2->value.name->identifier;
			break;
		
		case O_SCON:
			str2 = string2->value.scon;
			break;
		
		default:
			error("type mismatch on setenv");
			break;
	}
	s = make_string(str1, strlen(str2) +3);
	if(s == nil)
		error("malloc failure on setenv string");
	strcat(s , "=");
	strcat(s , str2);
	if(putenv(s))
		error("putenv failure on setenv");
}

/*
 * Set a debugger variable.
 */

private set (var, exp)
Node var, exp;
{
    Symbol t;

    if (var == nil) {
	defvar(nil, nil);
    } else if (exp == nil) {
	defvar(var->value.name, nil);
    } else if (var->value.name == identname("$frame", true)) {
	t = exp->nodetype;
	if (not compatible(t, t_int) and not compatible(t, t_addr)) {
	    error("$frame must be an address");
	}
	eval(exp);
	getnewregs(pop(Address));
    } else {
	defvar(var->value.name, unrval(exp));
    }
}

/*
 * Execute a list command.
 */

private list (p)
Node p;
{
    Symbol f;
    Address addr;
    Lineno line, l1, l2;

	if (p->value.arg[0]->op == O_SYM) {
    	func(p->value.arg[0]);
		f = p->value.arg[0]->value.sym;
		addr = codeloc(curfunc);
		if (addr == NOADDR) {
 	    	error("no source lines for \"%s\"", symname(f));
		}
		setsource(srcfilename(addr));
		line = srcline(addr);
		getsrcwindow(line, &l1, &l2);
    } else {
		eval(p->value.arg[0]);
		l1 = (Lineno) (pop(long));
		eval(p->value.arg[1]);
		l2 = (Lineno) (pop(long));
    }
    printlines(l1, l2);
}

/*
 * Execute a func command.
 */

private func (p)
Node p;
{
    Symbol s, f;
    Address addr;

    if (p == nil) {
		printname(stdout, curfunc);
		putchar('\n');
    } else {
		s = p->value.sym;
		if (isroutine(s)) {
	    	setcurfunc(s);
		} else {
	    	find(f, s->name) where isroutine(f) endfind(f);
	    	if (f == nil) {
				error("%s is not a procedure or function", symname(s));
	    	}
	    	setcurfunc(f);
		}
		addr = codeloc(curfunc);
		if (addr != NOADDR) {
	    	setsource(srcfilename(addr));
	    	cursrcline = srcline(addr);
		}
    }
}

/*
 * Give the user some help.
 */

public help()
{
	puts("sh man dbx             - for more explicit help");
    puts("run                    - begin execution of the program");
    puts("print <exp>            - print the value of the expression");
    puts("where                  - print currently active procedures");
    puts("stop at <line>         - suspend execution at the line");
    puts("stop in <proc>         - suspend execution when <proc> is called");
    puts("cont                   - continue execution");
    puts("step                   - single step one line");
    puts("next                   - step to next line (skip over calls)");
    puts("trace <line#>          - trace execution of the line");
    puts("trace <proc>           - trace calls to the procedure");
    puts("trace <var>            - trace changes to the variable");
    puts("trace <exp> at <line#> - print <exp> when <line> is reached");
    puts("status                 - print trace/stop's in effect");
    puts("delete <number>        - remove trace or stop of given number");
    puts("call <proc>            - call a procedure in program");
    puts("whatis <name>          - print the declaration of the name");
    puts("list <line>, <line>    - list source lines");
    puts("quit                   - exit dbx");
}

/*
 * Divert output to the given file name.
 * Cannot redirect to an existing file.
 */

private int so_fd;
private Boolean notstdout;

public setout(filename)
String filename;
{
    File f;

    f = fopen(filename, "r");
    if (f != nil) {
	fclose(f);
	error("%s: file already exists", filename);
    } else {
	so_fd = dup(1);
	close(1);
	if (creat(filename, 0666) == nil) {
	    unsetout();
	    error("can't create %s", filename);
	}
	notstdout = true;
    }
}

/*
 * Revert output to standard output.
 */

public unsetout()
{
    fflush(stdout);
    close(1);
    if (dup(so_fd) != 1) {
	panic("standard out dup failed");
    }
    close(so_fd);
    notstdout = false;
}

/*
 * Determine is standard output is currently being redirected
 * to a file (as far as we know).
 */

public Boolean isredirected()
{
    return notstdout;
}
