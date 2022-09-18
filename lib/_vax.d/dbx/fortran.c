/*#@(#)fortran.c	4.2  Ultrix  11/9/90*/

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
 *	006 - Fix typo (&& for &) and paren screwup in 005.		*
 *	      (jlr, May 12, 1989)					*
 *									*
 *	005 - Improve handling of VAX FORTRAN (fort) data types:	*
 *	      detect H floats, fort's versions of logicals and		*
 *	      integers; print correct value for logicals, regardless	*
 *	      of compiler.						*
 *	      (Jon Reeves, April 14, 1987)				*
 *									*
 *	004 - Fix for spr ICA-02533.  Assigning to reals gave error	*
 *	      message "incompatible types".				*
 *	      (vjh, Nov. 11, 1986)					*
 *									*
 *	003 - Merged in 4.3 changes.					*
 *	      (vjh, April 29, 1986)					*
 *									*
 *	002 - Modified fortran_init() to use LanguageName constant in 	*
 *	      call to language_define().				*
 *	      (vjh, June 22, 1985)					*
 *									*
 *	001 - Added COMMON to switch in fortran_printdecl().		*
 *	      (Victoria Holt, May 8, 1985)				*
 *									*
 ************************************************************************/

/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char sccsid[] = "@(#)fortran.c	4.2	ULTRIX	11/9/90";
#endif not lint

static char rcsid[] = "$Header: fortran.c,v 1.5 84/12/26 10:39:37 linton Exp $";

/*
 * FORTRAN dependent symbol routines.
 */

#include "defs.h"
#include "symbols.h"
#include "printsym.h"
#include "languages.h"
#include "fortran.h"
#include "tree.h"
#include "eval.h"
#include "operators.h"
#include "mappings.h"
#include "process.h"
#include "runtime.h"
#include "machine.h"

#define isfloat(range) ( \
    range->symvalue.rangev.upper == 0 and range->symvalue.rangev.lower > 0 \
)

#define isrange(t, name) (t->class == RANGE and istypename(t->type, name))

#define MAXDIM  20

private Language fort;

/*
 * Initialize FORTRAN language information.
 */

public fortran_init()
{
    fort = language_define("fortran", FORTRAN);
    language_setop(fort, L_PRINTDECL, fortran_printdecl);
    language_setop(fort, L_PRINTVAL, fortran_printval);
    language_setop(fort, L_TYPEMATCH, fortran_typematch);
    language_setop(fort, L_BUILDAREF, fortran_buildaref);
    language_setop(fort, L_EVALAREF, fortran_evalaref);
    language_setop(fort, L_MODINIT, fortran_modinit);
    language_setop(fort, L_HASMODULES, fortran_hasmodules);
    language_setop(fort, L_PASSADDR, fortran_passaddr);
    language_setop(fort, L_PRINTF, fortran_printf);
}

/*
 * Test if two types are compatible.
 *
 * Integers and reals are not compatible since they cannot always be mixed.
 */
/* jlr005
 * Types supported:
 * Integers: constants: t_int  f77: integer, integer*2  fort: integer, short
 * Real: constants: t_real  f77 and fort: real
 * Logical: constants: t_boolean  f77: logical  fort: unsigned (char,short,int)
 * Does not support mixing variable sizes (e.g., integer = integer*2).
 */

public Boolean fortran_typematch(type1, type2)
Symbol type1, type2;
{

    Boolean b;
    register Symbol t1, t2, tmp;

    t1 = rtype(type1);
    t2 = rtype(type2);
    if(t1 == nil or t1->type == nil or t2 == nil or t2->type == nil ) b = false;
    else { b = (Boolean)   (
            (t1 == t2)  or 
	    (t1->type == t_int and (istypename(t2->type, "integer") or
                                    istypename(t2->type, "integer*2") or
				    istypename(t2->type, "short"))	) or
	    (t2->type == t_int and (istypename(t1->type, "integer") or
				    istypename(t1->type, "integer*2") or
				    istypename(t1->type, "short"))	) or
	    (t1->type == t_real and (istypename(t2->type, "real"))	) or
	    (t2->type == t_real and (istypename(t1->type, "real"))	) or
	    (t1->type == t_boolean and (istypename(t2->type, "logical") or
					istypename(t2->type, "unsigned char") or
					istypename(t2->type, "unsigned short") or
					istypename(t2->type, "unsigned int")) ) or
	    (t2->type == t_boolean and (istypename(t1->type, "logical") or
					istypename(t1->type, "unsigned char") or
					istypename(t1->type, "unsigned short") or
					istypename(t1->type, "unsigned int")) )
                    );
         }
    /*OUT fprintf(stderr," %d compat %s %s \n", b,
      (t1 == nil or t1->type == nil ) ? "nil" : symname(t1->type),
      (t2 == nil or t2->type == nil ) ? "nil" : symname(t2->type)  );*/
    return b;
}

private String typename(s)
Symbol s;
{
int ub;
static char buf[20];
char *pbuf;
Symbol st,sc;

     if(s->type->class == TYPE) return(symname(s->type));

     for(st = s->type; st->type->class != TYPE; st = st->type);

     pbuf=buf;

     if(istypename(st->type,"char"))  { 
	  sprintf(pbuf,"character*");
          pbuf += strlen(pbuf);
	  sc = st->chain;
          if(sc->symvalue.rangev.uppertype == R_ARG or
             sc->symvalue.rangev.uppertype == R_TEMP) {
	      if( ! getbound(s,sc->symvalue.rangev.upper, 
                    sc->symvalue.rangev.uppertype, &ub) )
		sprintf(pbuf,"(*)");
	      else 
		sprintf(pbuf,"%d",ub);
          }
 	  else sprintf(pbuf,"%d",sc->symvalue.rangev.upper);
     }
     else {
          sprintf(pbuf,"%s ",symname(st->type));
     }
     return(buf);
}

private Symbol mksubs(pbuf,st)
Symbol st;
char  **pbuf;
{   
   int lb, ub;
   Symbol r, eltype;

   if(st->class != ARRAY or (istypename(st->type, "char")) ) return;
   else {
          mksubs(pbuf,st->type);
          assert( (r = st->chain)->class == RANGE);

          if(r->symvalue.rangev.lowertype == R_ARG or
             r->symvalue.rangev.lowertype == R_TEMP) {
	      if( ! getbound(st,r->symvalue.rangev.lower, 
                    r->symvalue.rangev.lowertype, &lb) )
		sprintf(*pbuf,"?:");
	      else 
		sprintf(*pbuf,"%d:",lb);
	  }
          else {
		lb = r->symvalue.rangev.lower;
		sprintf(*pbuf,"%d:",lb);
		}
    	  *pbuf += strlen(*pbuf);

          if(r->symvalue.rangev.uppertype == R_ARG or
             r->symvalue.rangev.uppertype == R_TEMP) {
	      if( ! getbound(st,r->symvalue.rangev.upper, 
                    r->symvalue.rangev.uppertype, &ub) )
		sprintf(*pbuf,"?,");
	      else 
		sprintf(*pbuf,"%d,",ub);
	  }
          else {
		ub = r->symvalue.rangev.upper;
		sprintf(*pbuf,"%d,",ub);
		}
    	  *pbuf += strlen(*pbuf);

       }
}

/*
 * Print out the declaration of a FORTRAN variable.
 */

public fortran_printdecl(s)
Symbol s;
{


Symbol eltype;

    switch (s->class) {

	case CONST:
	    
	    printf("parameter %s = ", symname(s));
            printval(s);
	    break;

        case REF:
            printf(" (dummy argument) ");

	case VAR:
	    if (s->type->class == ARRAY &&
		 (not istypename(s->type->type,"char")) ) {
                char bounds[130], *p1, **p;
		p1 = bounds;
                p = &p1;
                mksubs(p,s->type);
                *p -= 1; 
                **p = '\0';   /* get rid of trailing ',' */
		printf(" %s %s[%s] ",typename(s), symname(s), bounds);
	    } else {
		printf("%s %s", typename(s), symname(s));
	    }
	    break;

	case FUNC:
	    if (not istypename(s->type, "void")) {
                printf(" %s function ", typename(s) );
	    }
	    else printf(" subroutine");
	    printf(" %s ", symname(s));
	    fortran_listparams(s);
	    break;

	case MODULE:
	    printf("source file \"%s.c\"", symname(s));
	    break;

	case PROG:
	    printf("executable file \"%s\"", symname(s));
	    break;

	case COMMON:
	    printf("named common %s", symname(s));
	    break;

	default:
	    error("class %s in fortran_printdecl", classname(s));
    }
    putchar('\n');
}

/*
 * List the parameters of a procedure or function.
 * No attempt is made to combine like types.
 */

public fortran_listparams(s)
Symbol s;
{
    register Symbol t;

    putchar('(');
    for (t = s->chain; t != nil; t = t->chain) {
	printf("%s", symname(t));
	if (t->chain != nil) {
	    printf(", ");
	}
    }
    putchar(')');
    if (s->chain != nil) {
	printf("\n");
	for (t = s->chain; t != nil; t = t->chain) {
	    if (t->class != REF) {
		panic("unexpected class %d for parameter", t->class);
	    }
	    printdecl(t, 0);
	}
    } else {
	putchar('\n');
    }
}

/*
 * Print out the value on the top of the expression stack
 * in the format for the type of the given symbol.
 */

public fortran_printval(s)
Symbol s;
{
    register Symbol t;
    register Address a;
    register int i, len;
    double d1, d2;

    switch (s->class) {
	case CONST:
	case TYPE:
	case VAR:
	case REF:
	case FVAR:
	case TAG:
	    fortran_printval(s->type);
	    break;

	case FIELD:
		modula2_printval(s);
	    break;
	
	case ARRAY:
	    t = rtype(s->type);
	    if (t->class == RANGE and istypename(t->type, "char")) {
		len = size(s);
		sp -= len;
		printf("\"%.*s\"", len, sp);
	    } else {
		fortran_printarray(s);
	    }
	    break;

	case RANGE:
	     if (isfloat(s)) {
		switch (s->symvalue.rangev.lower) {
		    case sizeof(float):
			prtreal(pop(float));
			break;

		    case sizeof(double):
			if (istypename(s->type,"complex")) {
			    d2 = pop(float);
			    d1 = pop(float);
			   printf("(");
			    prtreal(d1);
			   printf(",");
			    prtreal(d2);
			   printf(")");
			} else {
			    prtreal(pop(double));
			}
			break;

/* jlr005
 * 8 byte value.  Could be a double complex (2 doubles) or an H-float.
 * Check the type name to find out for sure.
 */
		    case 2*sizeof(double):
			if (istypename(s->type,"double complex")) {
			    d2 = pop(double);
			    d1 = pop(double);
			    printf("(");
			    prtreal(d1);
			    printf(",");
			    prtreal(d2);
			    printf(")");
			} else {
			    prth(pop(struct hfloat));
			}
			break;
		
		    default:
			panic("bad size \"%d\" for real",
                                  s->symvalue.rangev.lower);
			break;
		}
	    } else {
		printint(popsmall(s), s);
	    }
	    break;

	case RECORD:
	    printrecord(s);
	    break;

	default:
	    if (ord(s->class) > ord(TYPEREF)) {
		panic("printval: bad class %d", ord(s->class));
	    }
	    error("don't know how to print a %s", fortran_classname(s));
	    /* NOTREACHED */
    }
}

/*
 * Print out an int 
 */

private printint(i, t)
Integer i;
register Symbol t;
{
/* jlr005
 * Need to check for both f77 type name (logical) and fort type names
 * [unsigned (char,short,int)].  Also, since true==1 for f77 but 
 * true==all 1's for fort, check just the low-order bit to determine
 * the value.
 */
    if (istypename(t->type, "logical") or 
	istypename(t->type, "unsigned char") or 
	istypename(t->type, "unsigned short") or
	istypename(t->type, "unsigned int") ) {
	printf((Boolean) (i & 1) == true ? "true" : "false");
    }
/* jlr005
 * f77: integer or integer*2; fort: integer or short.
 */
    else if ( (t->type == t_int) or istypename(t->type, "integer") or
		  istypename(t->type,"integer*2") or 
		  istypename(t->type,"short") ) {
	printf("%ld", i);
    } else {
      error("unknown type in fortran printint");
    }
}

/*
 * Print out a null-terminated string (pointer to char)
 * starting at the given address.
 */

private printstring(addr)
Address addr;
{
    register Address a;
    register Integer i, len;
    register Boolean endofstring;
    union {
	char ch[sizeof(Word)];
	int word;
    } u;

    putchar('"');
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
    putchar('"');
}
/*
 * Return the FORTRAN name for the particular class of a symbol.
 */

public String fortran_classname(s)
Symbol s;
{
    String str;

    switch (s->class) {
	case REF:
	    str = "dummy argument";
	    break;

	case CONST:
	    str = "parameter";
	    break;

	default:
	    str = classname(s);
    }
    return str;
}

/* reverses the indices from the expr_list; should be folded into buildaref
 * and done as one recursive routine
 */
Node private rev_index(here,n)
register Node here,n;
{
 
  register Node i;

  if( here == nil  or  here == n) i=nil;
  else if( here->value.arg[1] == n) i = here;
  else i=rev_index(here->value.arg[1],n);
  return i;
}

public Node fortran_buildaref(a, slist)
Node a, slist;
{
    register Symbol as;      /* array of array of .. cursor */
    register Node en;        /* Expr list cursor */
    Symbol etype;            /* Type of subscript expr */
    Node esub, tree;         /* Subscript expression ptr and tree to be built*/

    tree=a;

    as = rtype(tree->nodetype);     /* node->sym.type->array*/
    if ( not (
               (tree->nodetype->class == VAR or tree->nodetype->class == REF)
                and as->class == ARRAY
             ) ) {
	beginerrmsg();
	prtree(stderr, a);
	fprintf(stderr, " is not an array");
	/*fprintf(stderr, " a-> %x as %x ", tree->nodetype, as ); OUT*/
	enderrmsg();
    } else {
	for (en = rev_index(slist,nil); en != nil and as->class == ARRAY;
                     en = rev_index(slist,en), as = as->type) {
	    esub = en->value.arg[0];
	    etype = rtype(esub->nodetype);
            assert(as->chain->class == RANGE);
	    if ( not compatible( t_int, etype) ) {
		beginerrmsg();
		fprintf(stderr, "subscript ");
		prtree(stderr, esub);
		fprintf(stderr, " is type %s ",symname(etype->type) );
		enderrmsg();
	    }
	    tree = build(O_INDEX, tree, esub);
	    tree->nodetype = as->type;
	}
	if (en != nil or
             (as->class == ARRAY && (not istypename(as->type,"char"))) ) {
	    beginerrmsg();
	    if (en != nil) {
		fprintf(stderr, "too many subscripts for ");
	    } else {
		fprintf(stderr, "not enough subscripts for ");
	    }
	    prtree(stderr, tree);
	    enderrmsg();
	}
    }
    return tree;
}

/*
 * Evaluate a subscript index.
 */

public fortran_evalaref(s, base, i)
Symbol s;
Address base;
long i;
{
    Symbol r, t;
    long lb, ub;

    t = rtype(s);
    r = t->chain;
    if (
	r->symvalue.rangev.lowertype == R_ARG or
        r->symvalue.rangev.lowertype == R_TEMP
    ) {
	if (not getbound(
	    s, r->symvalue.rangev.lower, r->symvalue.rangev.lowertype, &lb
	)) {
          error("dynamic bounds not currently available");
    }
    } else {
	lb = r->symvalue.rangev.lower;
    }
    if (
	r->symvalue.rangev.uppertype == R_ARG or
        r->symvalue.rangev.uppertype == R_TEMP
    ) {
	if (not getbound(
	    s, r->symvalue.rangev.upper, r->symvalue.rangev.uppertype, &ub
	)) {
          error("dynamic bounds not currently available");
    }
    } else {
	ub = r->symvalue.rangev.upper;
    }

    if (i < lb or i > ub) {
	error("subscript out of range");
    }
    push(long, base + (i - lb) * size(t->type));
}

private fortran_printarray(a)
Symbol a;
{
struct Bounds { int lb, val, ub} dim[MAXDIM];

Symbol sc,st,eltype;
char buf[50];
char *subscr;
int i,ndim,elsize;
Stack *savesp;
Boolean done;

st = a;

savesp = sp;
sp -= size(a);
ndim=0;

for(;;){
          sc = st->chain;
          if(sc->symvalue.rangev.lowertype == R_ARG or
             sc->symvalue.rangev.lowertype == R_TEMP) {
	      if( ! getbound(a,sc->symvalue.rangev.lower, 
                    sc->symvalue.rangev.lowertype, &dim[ndim].lb) )
		error(" dynamic bounds not currently available");
	  }
	  else dim[ndim].lb = sc->symvalue.rangev.lower;

          if(sc->symvalue.rangev.uppertype == R_ARG or
             sc->symvalue.rangev.uppertype == R_TEMP) {
	      if( ! getbound(a,sc->symvalue.rangev.upper, 
                    sc->symvalue.rangev.uppertype, &dim[ndim].ub) )
		error(" dynamic bounds not currently available");
	  }
	  else dim[ndim].ub = sc->symvalue.rangev.upper;

          ndim ++;
          if (st->type->class == ARRAY) st=st->type;
	  else break;
     }

if(istypename(st->type,"char")) {
		eltype = st;
		ndim--;
	}
else eltype=st->type;
elsize=size(eltype);
sp += elsize;
 /*printf("ndim %d elsize %lx in fortran_printarray\n",ndim,elsize);OUT*/

ndim--;
for (i=0;i<=ndim;i++){
	  dim[i].val=dim[i].lb;
	  /*OUT printf(" %d %d %d \n",i,dim[i].lb,dim[i].ub);
	    fflush(stdout); OUT*/
}


for(;;) {
	buf[0]=',';
	subscr = buf+1;

	for (i=ndim-1;i>=0;i--)  {

		sprintf(subscr,"%d,",dim[i].val);
        	subscr += strlen(subscr);
	}
        *--subscr = '\0';

	for(i=dim[ndim].lb;i<=dim[ndim].ub;i++) {
	      	printf("[%d%s]\t",i,buf);
		printval(eltype);
	      	printf("\n");
		sp += 2*elsize;
	}
        dim[ndim].val=dim[ndim].ub;

        i=ndim-1;
        if (i<0) break;

        done=false;
        do {
		dim[i].val++;
		if(dim[i].val > dim[i].ub) { 
			dim[i].val = dim[i].lb;
			if(--i<0) done=true;
		}
		else done=true;
         }
	 while (not done);
         if (i<0) break;
     }
}

/*
 * Initialize typetable at beginning of a module.
 */

public fortran_modinit (typetable)
Symbol typetable[];
{
    /* nothing for now */
}

public boolean fortran_hasmodules ()
{
    return false;
}

public boolean fortran_passaddr (param, exprtype)
Symbol param, exprtype;
{
    return false;
}

public Node fortran_printf(p, argv)
Node p;
char **argv;
{
	return p;
}
