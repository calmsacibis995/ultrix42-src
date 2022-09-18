#ifndef lint
static char sccsid[] = "@(#)lint.c	1.5	(Berkeley)	3/30/83";
#endif lint

# include "mfile1"

# include "lmanifest"

# include <ctype.h>

# define VAL 0
# define EFF 1

extern int ddebug, idebug, bdebug, tdebug, edebug, xdebug;	/* CXREF */

int blocknos[20];	/* CXREF */
int blockptr = 0;	/* CXREF */
int nextblock = 1;	/* CXREF */

/* these are appropriate for the -p flag */
/* int  SZCHAR = 8;
int  SZINT = 16;
int  SZFLOAT = 32;
int  SZDOUBLE = 64;
int  SZLONG = 32;
int  SZSHORT = 16;
int SZPOINT = 16;
int ALCHAR = 8;
int ALINT = 16;
int ALFLOAT = 32;
int ALDOUBLE = 64;
int ALLONG = 32;
int ALSHORT = 16;
int ALPOINT = 16;
int ALSTRUCT = 16;
*/
int vflag = 1;  /* tell about unused argments */
int xflag = 0;  /* tell about unused externals */
int argflag = 0;  /* used to turn off complaints about arguments */
int libflag = 0;  /* used to generate library descriptions */
int vaflag = -1;  /* used to signal functions with a variable number of args */
int aflag = 0;  /* used to check precision of assignments */
int zflag = 0;  /* no 'structure never defined' error */
int Cflag = 0;  /* filter out certain output, for generating libraries */
char *libname = 0;  /* name of the library we're generating */

	/* flags for the "outdef" function */
# define USUAL (-101)
# define DECTY (-102)
# define NOFILE (-103)
# define SVLINE (-104)

# define LNAMES 250

struct lnm {
	short lid, flgs;
	}  lnames[LNAMES], *lnp;

contx( p, down, pl, pr ) register NODE *p; register *pl, *pr; {

	*pl = *pr = VAL;
	switch( p->in.op ){

	case ANDAND:
	case OROR:
	case QUEST:
		*pr = down;
		break;

	case SCONV:
	case PCONV:
	case COLON:
		*pr = *pl = down;
		break;

	case COMOP:
		*pl = EFF;
		*pr = down;

	case FORCE:
	case INIT:
	case UNARY CALL:
	case STCALL:
	case UNARY STCALL:
	case CALL:
	case UNARY FORTCALL:
	case FORTCALL:
	case CBRANCH:
		break;

	default:
		if( asgop(p->in.op) ) break;
		if( p->in.op == UNARY MUL && ( p->in.type == STRTY || p->in.type == UNIONTY || p->in.type == UNDEF) ) {
		/* struct x f( );  main( ) {  (void) f( ); }
		 * the the cast call appears as U* UNDEF
		 */
			break;  /* the compiler does this... */
			}
		if( down == EFF && hflag ) werror( "null effect" );

		}
	}

ecode( p ) NODE *p; {
	/* compile code for p */

/*	fwalk( p, contx, EFF );
	lnp = lnames;
	lprt( p, EFF, 0 );	CXREF */
	}


astype( t, i ) ATYPE *t; {
	TWORD tt;
	int j, k=0, l=0;

	if( (tt=BTYPE(t->aty))==STRTY || tt==UNIONTY ){
		if( i<0 || i>= DIMTABSZ-3 ){
			werror( "lint's little mind is blown" );
			}
		else {
			j = dimtab[i+3];
			if( j<0 || j>SYMTSZ ){
				k = dimtab[i];
				l = X_NONAME | stab[j].suse;
				}
			else {
				if( stab[j].suse <= 0 ) {
#ifndef FLEXNAMES
					werror( "no line number for %.8s",
#else
					werror( "no line number for %s",
#endif
						stab[j].sname );
					}
				else {
					k = dimtab[i];
#ifdef FLEXNAMES
					l = hashstr(stab[j].sname);
#else
					l = hashstr(stab[j].sname, LCHNM);
#endif
					}
				}
			}
		
		t->extra = k;
		t->extra1 = l;
		return( 1 );
		}
	else return( 0 );
	}


ctargs( p ) NODE *p; {
	/* count arguments; p points to at least one */
	/* the arguemnts are a tower of commas to the left */
	register c;
	c = 1; /* count the rhs */
	while( p->in.op == CM ){
		++c;
		p = p->in.left;
		}
	return( c );
	}

lpta( p ) NODE *p; {
	static ATYPE t;

	if( p->in.op == CM ){
		lpta( p->in.left );
		p = p->in.right;
		}

	t.aty = p->in.type;
	t.extra = (p->in.op==ICON);
	t.extra1 = 0;

	if( !astype( &t, p->fn.csiz ) ) {
		switch( t.aty ){

			case CHAR:
			case SHORT:
				t.aty = INT;
			case LONG:
			case ULONG:
			case INT:
			case UNSIGNED:
				break;

			case UCHAR:
			case USHORT:
				t.aty = UNSIGNED;
				break;

			case FLOAT:
				t.aty = DOUBLE;
				t.extra = 0;
				break;

			default:
				t.extra = 0;
				break;
			}
		}
	fwrite( (char *)&t, sizeof(ATYPE), 1, stdout );
	}

# define VALSET 1
# define VALUSED 2
# define VALASGOP 4
# define VALADDR 8

lprt( p, down, uses ) register NODE *p; {
	register struct symtab *q;
	register id;
	register acount;
	register down1, down2;
	register use1, use2;
	register struct lnm *np1, *np2;

	/* first, set variables which are set... */

	use1 = use2 = VALUSED;
	if( p->in.op == ASSIGN ) use1 = VALSET;
	else if( p->in.op == UNARY AND ) use1 = VALADDR;
	else if( asgop( p->in.op ) ){ /* =ops */
		use1 = VALUSED|VALSET;
		if( down == EFF ) use1 |= VALASGOP;
		}


	/* print the lines for lint */

	down2 = down1 = VAL;
	acount = 0;

	switch( p->in.op ){

	case EQ:
	case NE:
	case GT:
	case GE:
	case LT:
	case LE:
		if( p->in.left->in.type == CHAR && p->in.right->in.op==ICON && p->in.right->tn.lval < 0 ){
			werror( "nonportable character comparison" );
			}
		if( (p->in.op==EQ || p->in.op==NE ) && ISUNSIGNED(p->in.left->in.type) && p->in.right->in.op == ICON ){
			if( p->in.right->tn.lval < 0 && p->in.right->tn.rval == NONAME && !ISUNSIGNED(p->in.right->in.type) ){
				werror( "comparison of unsigned with negative constant" );
				}
			}
		break;

	case UGE:
	case ULT:
		if( p->in.right->in.op == ICON && p->in.right->tn.lval == 0 && p->in.right->tn.rval == NONAME ){
			werror( "unsigned comparison with 0?" );
			break;
			}
	case UGT:
	case ULE:
		if( p->in.right->in.op == ICON && p->in.right->tn.lval <= 0 && !ISUNSIGNED(p->in.right->in.type) && p->in.right->tn.rval == NONAME ){
			werror( "degenerate unsigned comparison" );
			}
		break;

	case COMOP:
		down1 = EFF;

	case ANDAND:
	case OROR:
	case QUEST:
		down2 = down;
		/* go recursively left, then right  */
		np1 = lnp;
		lprt( p->in.left, down1, use1 );
		np2 = lnp;
		lprt( p->in.right, down2, use2 );
		lmerge( np1, np2, 0 );
		return;

	case SCONV:
	case PCONV:
	case COLON:
		down1 = down2 = down;
		break;

	case CALL:
	case STCALL:
	case FORTCALL:
		acount = ctargs( p->in.right );
	case UNARY CALL:
	case UNARY STCALL:
	case UNARY FORTCALL:
		if( p->in.left->in.op == ICON && (id=p->in.left->tn.rval) != NONAME ){ /* used to be &name */
			struct symtab *sp = &stab[id];
			int lty;

			fsave( ftitle );
			/*
			 * if we're generating a library -C then
			 * we don't want to output references to functions
			 */
			if( Cflag ) break;
			/*  if a function used in an effects context is
			 *  cast to type  void  then consider its value
			 *  to have been disposed of properly
			 *  thus a call of type  undef  in an effects
			 *  context is construed to be used in a value
			 *  context
			 */
			if ((down == EFF) && (p->in.type != UNDEF)) {
				lty = LUE;
			} else if (down == EFF) {
				lty = LUV | LUE;
			} else {
				lty = LUV;
			}
			outdef( sp, lty, acount );
			if( acount ) {
				lpta( p->in.right );
				}
			}
		break;

	case ICON:
		/* look for &name case */
		if( (id = p->tn.rval) >= 0 && id != NONAME ){
			q = &stab[id];
			q->sflags |= (SREF|SSET);
			q->suse = -lineno;
			}
		return;

	case NAME:
		if( (id = p->tn.rval) >= 0 && id != NONAME ){
			q = &stab[id];
			if( (uses&VALUSED) && !(q->sflags&SSET) ){
				if( q->sclass == AUTO || q->sclass == REGISTER ){
					if( !ISARY(q->stype ) && !ISFTN(q->stype) && q->stype!=STRTY && q->stype!=UNIONTY ){
#ifndef FLEXNAMES
						werror( "%.8s may be used before set", q->sname );
#else
						werror( "%s may be used before set", q->sname );
#endif
						q->sflags |= SSET;
						}
					}
				}
			if( uses & VALASGOP ) break;  /* not a real use */
			if( uses & VALSET ) q->sflags |= SSET;
			if( uses & VALUSED ) q->sflags |= SREF;
			if( uses & VALADDR ) q->sflags |= (SREF|SSET);
			if( p->tn.lval == 0 ){
				lnp->lid = id;
				lnp->flgs = (uses&VALADDR)?0:((uses&VALSET)?VALSET:VALUSED);
				if( ++lnp >= &lnames[LNAMES] ) --lnp;
				}
			}
		return;

		}

	/* recurse, going down the right side first if we can */

	switch( optype(p->in.op) ){

	case BITYPE:
		np1 = lnp;
		lprt( p->in.right, down2, use2 );
	case UTYPE:
		np2 = lnp;
		lprt( p->in.left, down1, use1 );
		}

	if( optype(p->in.op) == BITYPE ){
		if( p->in.op == ASSIGN && p->in.left->in.op == NAME ){ /* special case for a =  .. a .. */
			lmerge( np1, np2, 0 );
			}
		else lmerge( np1, np2, p->in.op != COLON );
		/* look for assignments to fields, and complain */
		if( p->in.op == ASSIGN && p->in.left->in.op == FLD && p->in.right->in.op == ICON ) fldcon( p );
		}

	}

lmerge( np1, np2, flag ) struct lnm *np1, *np2; {
	/* np1 and np2 point to lists of lnm members, for the two sides
	 * of a binary operator
	 * flag is 1 if commutation is possible, 0 otherwise
	 * lmerge returns a merged list, starting at np1, resetting lnp
	 * it also complains, if appropriate, about side effects
	 */

	register struct lnm *npx, *npy;

	for( npx = np2; npx < lnp; ++npx ){

		/* is it already there? */
		for( npy = np1; npy < np2; ++npy ){
			if( npx->lid == npy->lid ){ /* yes */
				if( npx->flgs == 0 || npx->flgs == (VALSET|VALUSED) )
					;  /* do nothing */
				else if( (npx->flgs|npy->flgs)== (VALSET|VALUSED) ||
					(npx->flgs&npy->flgs&VALSET) ){
#ifndef FLEXNAMES
					if( flag ) werror( "%.8s evaluation order undefined", stab[npy->lid].sname );
#else
					if( flag ) werror( "%s evaluation order undefined", stab[npy->lid].sname );
#endif
					}
				if( npy->flgs == 0 ) npx->flgs = 0;
				else npy->flgs |= npx->flgs;
				goto foundit;
				}
			}

		/* not there: update entry */
		np2->lid = npx->lid;
		np2->flgs = npx->flgs;
		++np2;

		foundit: ;
		}

	/* all finished: merged list is at np1 */
	lnp = np2;
	}


zecode( n ){
	/* n integer words of zeros */
	OFFSZ temp;
	temp = n;
	inoff += temp*SZINT;
	;
	}

andable( p ) NODE *p; {  /* p is a NAME node; can it accept & ? */
	register r;

	if( p->in.op != NAME ) cerror( "andable error" );

	if( (r = p->tn.rval) < 0 ) return(1);  /* labels are andable */

	if( stab[r].sclass == AUTO || stab[r].sclass == PARAM ) return(0); 
#ifndef FLEXNAMES
	if( stab[r].sclass == REGISTER ) uerror( "can't take & of %.8s", stab[r].sname );
#else
	if( stab[r].sclass == REGISTER ) uerror( "can't take & of %s", stab[r].sname );
#endif
	return(1);
	}

NODE *
clocal(p) NODE *p; {

	/* this is called to do local transformations on
	   an expression tree preparitory to its being
	   written out in intermediate code.
	*/

	/* the major essential job is rewriting the
	   automatic variables and arguments in terms of
	   REG and OREG nodes */
	/* conversion ops which are not necessary are also clobbered here */
	/* in addition, any special features (such as rewriting
	   exclusive or) are easily handled here as well */

	register o;
	register unsigned t, tl;

	switch( o = p->in.op ){

	case SCONV:
	case PCONV:
		if( p->in.left->in.type==ENUMTY ){
			p->in.left = pconvert( p->in.left );
			}
		/* assume conversion takes place; type is inherited */
		t = p->in.type;
		tl = p->in.left->in.type;
		if( aflag && (tl==LONG||tl==ULONG) && (t!=LONG&&t!=ULONG) ){
			werror( "long assignment may lose accuracy" );
			}
		if( aflag>=2 && (tl!=LONG&&tl!=ULONG) && (t==LONG||t==ULONG) && p->in.left->in.op != ICON ){
			werror( "assignment to long may sign-extend incorrectly" );
			}
		if( ISPTR(tl) && ISPTR(t) ){
			tl = DECREF(tl);
			t = DECREF(t);
			switch( ISFTN(t) + ISFTN(tl) ){

			case 0:  /* neither is a function pointer */
				if( talign(t,p->fn.csiz) > talign(tl,p->in.left->fn.csiz) ){
					if( hflag||pflag ) werror( "possible pointer alignment problem" );
					}
				break;

			case 1:
				werror( "questionable conversion of function pointer" );

			case 2:
				;
				}
			}
		p->in.left->in.type = p->in.type;
		p->in.left->fn.cdim = p->fn.cdim;
		p->in.left->fn.csiz = p->fn.csiz;
		p->in.op = FREE;
		return( p->in.left );

	case PVCONV:
	case PMCONV:
		if( p->in.right->in.op != ICON ) cerror( "bad conversion");
		p->in.op = FREE;
		return( buildtree( o==PMCONV?MUL:DIV, p->in.left, p->in.right ) );

		}

	return(p);
	}

NODE *
offcon( off, t, d, s ) OFFSZ off; TWORD t;{  /* make a structure offset node */
	register NODE *p;
	p = bcon(0);
	p->tn.lval = off/SZCHAR;
	return(p);
	}

noinit(){
	/* storage class for such as "int a;" */
	return( pflag ? EXTDEF : EXTERN );
	}


cinit( p, sz ) NODE *p; { /* initialize p into size sz */
	inoff += sz;
	if( p->in.op == INIT ){
		if( p->in.left->in.op == ICON ) return;
		if( p->in.left->in.op == NAME && p->in.left->in.type == MOE ) return;
		}
	uerror( "illegal initialization" );
	}

char *
strip(s) char *s; {
#ifndef FLEXNAMES
	static char x[LFNM+1];
#else
	static char x[BUFSIZ];
#endif
	register char *p;
	static	int	stripping = 0;

	if (stripping)
		return(s);
	stripping++;
	for( p=x; *s; ++s ){
		if( *s != '"' ){
#ifndef FLEXNAMES
/* PATCHED by ROBERT HENRY on 8Jul80 to fix 14 character file name bug */
			if( p >= &x[LFNM] )
#else
			if( p >= &x[BUFSIZ] )
#endif
				cerror( "filename too long" );
			*p++ = *s;
		}
	}
	stripping = 0;
	*p = '\0';
#ifndef FLEXNAMES
	return( x );
#else
	return( hash(x) );
#endif
	}

fsave( s ) char *s; {
	static union rec fsname;
	s = strip( s );
#ifndef FLEXNAMES
	if( strncmp( s, fsname.f.fn, LFNM ) ){
#else
	if (fsname.f.fn == NULL || strcmp(s, fsname.f.fn)) {
#endif
		/* new one */
#ifndef FLEXNAMES
		strncpy( fsname.f.fn, s, LFNM );
#else
		fsname.f.fn = s;
#endif
		fsname.f.decflag = LFN;
		fwrite( (char *)&fsname, sizeof(fsname), 1, stdout );
#ifdef FLEXNAMES
		/* if generating a library, prefix with the library name */
		/* only do this for flexnames */
		if( libname ){
			fwrite( libname, strlen(libname), 1, stdout );
			putchar( ':' );
			}
		fwrite( fsname.f.fn, strlen(fsname.f.fn)+1, 1, stdout );
#endif
		}
	}

where(f){ /* print true location of error */
	if( f == 'u' && nerrors > 1 )
		--nerrors; /* don't get "too many errors" */
	fprintf( stderr, "%s(%d): ", strip(ftitle), lineno);
	}

	/* a number of dummy routines, unneeded by lint */

branch(n){;}
defalign(n){;}
deflab(n){;}
bycode(t,i){;}
cisreg(t) TWORD t; {return(1);}  /* everyting is a register variable! */

fldty(p) struct symtab *p; {
	; /* all types are OK here... */
	}

fldal(t) unsigned t; { /* field alignment... */
	if( t == ENUMTY ) return( ALCHAR );  /* this should be thought through better... */
	if( ISPTR(t) ){ /* really for the benefit of honeywell (and someday IBM) */
		if( pflag ) uerror( "nonportable field type" );
		}
	else uerror( "illegal field type" );
	return(ALINT);
	}

main( argc, argv ) char *argv[]; {
	/* definitions for CXREF */
	char *p, *cp;
	int n = 1;

	/* handle options */

	for( p=argv[1]; n < argc && *p == '-'; p = argv[++n]){

		switch( *++p ){

		/* the next bunch of cases are for CXREF */
		case 'd':
			++ddebug;
			continue;
		case 'I':
			++idebug;
			continue;
		case 'b':
			++bdebug;
			continue;
		case 't':
			++tdebug;
			continue;
		case 'e':
			++edebug;
			continue;
		case 'x':
			++xdebug;
			continue;
		case 'f':	/* filename sent to cpp */
			p = argv[++n];
			infile[0] = '"';	/* put quotes around name */
			cp = &infile[1];
			while (*cp++ = *p++) ;	/* copy filename */
			*--cp = '"';
			*++cp = '\0';
			continue;

		case 'i':	/* actual input filename */
			p = argv[++n];
			if (freopen(p,"r",stdin) == NULL) {
				fprintf(stderr, "Can't open %s\n",p);
				exit(1);
			}
			continue;

		case 'o':
			p = argv[++n];
			if ((outfp = fopen(p,"a")) == NULL) {
				fprintf(stderr, "Can't Open %s\n",p);
				exit(2);
			}
			continue;

		default:
			uerror( "illegal option: %c", *p );
			continue;

			}
		}

	if( !pflag ){  /* set sizes to sizes of target machine */
# ifdef gcos
		SZCHAR = ALCHAR = 9;
# else
/*		SZCHAR = ALCHAR = 8;	CXREF */
# endif
/*		SZINT = ALINT = sizeof(int)*SZCHAR;
		SZFLOAT = ALFLOAT = sizeof(float)*SZCHAR;
		SZDOUBLE = ALDOUBLE = sizeof(double)*SZCHAR;
		SZLONG = ALLONG = sizeof(long)*SZCHAR;
		SZSHORT = ALSHORT = sizeof(short)*SZCHAR;
		SZPOINT = ALPOINT = sizeof(int *)*SZCHAR;
		ALSTRUCT = ALINT;	CXREF		*/
		/* now, fix some things up for various machines (I wish we had "alignof") */

# ifdef pdp11
		ALLONG = ALDOUBLE = ALFLOAT = ALINT;
#endif
# ifdef ibm
		ALSTRUCT = ALCHAR;
#endif
		}

	return( mainp1( argc, argv ) );
	}

ctype( type ) unsigned type; { /* are there any funny types? */
	return( type );
	}


isitfloat ( s ) char *s; {
	/* s is a character string;
	   if floating point is implemented, set dcon to the value of s */
	/* lint version
	*/
	dcon = atof( s );
	return( FCON );
	}

fldcon( p ) register NODE *p; {
	/* p is an assignment of a constant to a field */
	/* check to see if the assignment is going to overflow, or otherwise cause trouble */
	register s;
	CONSZ v;

	if( !hflag & !pflag ) return;

	s = UPKFSZ(p->in.left->tn.rval);
	v = p->in.right->tn.lval;

	switch( p->in.left->in.type ){

	case CHAR:
	case INT:
	case SHORT:
	case LONG:
	case ENUMTY:
		if( v>=0 && (v>>(s-1))==0 ) return;
		werror( "precision lost in assignment to (possibly sign-extended) field" );
	default:
		return;

	case UNSIGNED:
	case UCHAR:
	case USHORT:
	case ULONG:
		if( v<0 || (v>>s)!=0 ) werror( "precision lost in field assignment" );
		
		return;
		}

	}

outdef( p, lty, mode ) struct symtab *p; {
	/* output a definition for the second pass */
	/* if mode is > USUAL, it is the number of args */
	char *fname;
	TWORD t;
	int line;
	static union rec rc;

	if( mode == NOFILE ){
		fname = "???";
		line = p->suse;
		}
	else if( mode == SVLINE ){
		fname = ftitle;
		line = -p->suse;
		}
	else {
		fname = ftitle;
		line = lineno;
		}
	fsave( fname );
#ifndef FLEXNAMES
	strncpy( rc.l.name, exname(p->sname), LCHNM );
#endif
	rc.l.decflag = lty;
	t = p->stype;
	if( mode == DECTY ) t = DECREF(t);
	rc.l.type.aty = t;
	rc.l.type.extra = 0;
	rc.l.type.extra1 = 0;
	astype( &rc.l.type, p->sizoff );
	rc.l.nargs = (mode>USUAL) ? mode : 0;
	rc.l.fline = line;
	fwrite( (char *)&rc, sizeof(rc), 1, stdout );
#ifdef FLEXNAMES
	rc.l.name = exname(p->sname);
	fwrite( rc.l.name, strlen(rc.l.name)+1, 1, stdout );
#endif
	}
int proflg;
int gdebug;
bbcode()	/* CXREF */
{
	/* code for beginning a new block */
	blocknos[blockptr] = nextblock++;
	fprintf(outfp, "B%d\t%05d\n", blocknos[blockptr], lineno);
	blockptr++;
}

becode()	/* CXREF */
{
	/* code for ending a block */
	if (--blockptr < 0)
		uerror("bad nesting");
	else
		fprintf( outfp, "E%d\t%05d\n", blocknos[blockptr], lineno);
}
