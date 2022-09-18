/*@(#)object.c	4.3  Ultrix  12/6/90*/

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
 *	016 - Updated all calls to findlanguage() to call with		*
 *	      LanguageName constant, rather than with a filename	*
 *	      suffix.							*
 *	      (Bob Neff, February 27, 1990)					*
 *									*
 *  015	- Added support for vectors.							*
 *		  (L Miller, 18JAN90)									*
 *									*
 *	014 - Change "use" so that a user can access source code in a *
 *		  logical fashion (i.e. specifiy the dir where the source *
 *		  code lives). Care was taken so that dbx will still work *
 *		  the same as before.									  *
 *	      (Lee R. Miller Jan 9, 1990)							  *
 *									*
 *	013 - Make "strip_" externally visible for stabstring.c so	*
 *	      C symbols within FORTRAN programs can be found.		*
 *	      (Jon Reeves, July 14, 1987)				*
 *									*
 *	012 - Fix for spr ICA-02533.  Was losing the address of	LCSYM	*
 *	      stab entries.  Change to checkvar().			*
 *	      (vjh, Nov. 11, 1986)					*
 *									*
 *	011 - 4.3 changes to check_global(), combined with the lack of	*
 *	      underscores on global VAX/FORTRAN symbol names, proved	*
 *	      to be a serious problem.  Additionally, VAX/FORTRAN	*
 *	      is indicating the wrong number of functions in the N_NSYM *
 *	      record.  Added changes to make all this stuff "work"	*
 *	      again.  Also added vmsFileConvert() for Bliss.		*
 *	      (vjh, July 16, 1986)					*
 *									*
 *	010 - Merged in 4.3 changes.					*
 *	      (vjh, April 29, 1986)					*
 *									*
 *	009 - Changed check_global() so that address calculation for	*
 *	      functions is only done for functions not already seen.	*
 *	      (vjh, Dec. 23, 1985)					*
 *									*
 *	008 - Added case for N_NOMAP in enter_nl().  Takes into account *
 *	      a DST record that will not map to a stab entry.		*
 *	      (vjh, June 26, 1985)					*
 *									*
 *	007 - enterSourceModule() checks for a LanguageName in n_other.	*
 *	      If not found, then the file extension is still used to	*
 *	      determine the language of the source file.		*
 *	      (vjh, June 22, 1985)					*
 *									*
 *	006 - Check for table overflow - function, source line, and	*
 *	      source file tables.					*
 *	      (vjh, June 22, 1985)					*
 *									*
 *	005 - Added dynamically allocated function table, and routine	*
 *	      newfunc().  Added code to handle new N_NSYMS stab entry.	*
 *	      (vjh, June 16, 1985)					*
 *									*
 *	004 - Assume that any function encountered before loader	*
 *	      namelist has source information.  This way, there are	*
 *	      fewer restrictions on the placement of N_SLINE entries	*
 *	      in the symbol table (for the FORTRAN port).  Also, speeds *
 *	      things up a bit (removed linep/prevlinep comparison in	*
 *	      exitblock macro).						*
 *	      (vjh, May 30, 1985)					*
 *									*
 *	003 - Assign curcomm->language so that fortran_printdecl()	*
 *	      can print the declared type of named common.  Used	*
 *	      to cause an internal error from a nil pointer.		*
 *	      (vjh, May 8, 1985)					*
 *									*
 *	002 - Fixed bug:  dbx would coredump if the name of a		*
 *	      FORTRAN common block was the same as a source file	*
 *	      name that was linked into the program.			*
 *	      (vjh, April 10, 1985)					*
 *									*
 *	001 - Added N_FNAME to case statement in enter_nl().		*
 *	      (Victoria Holt, April 5, 1985)				*
 *									*
 ************************************************************************/

/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char *sccsid = "@(#)object.c	2.1	ULTRIX	4/24/89";
#endif not lint

/*
 * Object code interface, mainly for extraction of symbolic information.
 */

#include "defs.h"
#include "object.h"
#include "stabstring.h"
#include "main.h"
#include "symbols.h"
#include "names.h"
#include "languages.h"
#include "mappings.h"
#include "lists.h"
#include <a.out.h>
#include <stab.h>
#include <ctype.h>
#include <sys/param.h>

#ifndef public

struct {
    unsigned int stringsize;	/* size of the dumped string table */
    unsigned int nsyms;		/* number of symbols */
    unsigned int nfiles;	/* number of files */
    unsigned int nlines;	/* number of lines */
} nlhdr;

#include "languages.h"
#include "symbols.h"

#endif

public String objname = "a.out";
public integer objsize;

public Language curlang;
public Symbol curmodule;
public Symbol curparam;
public Symbol curcomm;
public Symbol commchain;

private char *stringtab;
private struct nlist *curnp;
private Boolean warned;
public Boolean strip_ = false;

private Filetab *filep;
private Linetab *linep;

public String curfilename ()
{
    return ((filep-1)->filename);
}

/*
 * Blocks are figured out on the fly while reading the symbol table.
 */

#define MAXBLKDEPTH 100

public Symbol curblock;

private Symbol blkstack[MAXBLKDEPTH];
private integer curlevel;
private integer bnum, nesting;
private Address addrstk[MAXBLKDEPTH];

#define isfunc(s) \
    (s->class == FUNC || s->class == PROC)

public pushBlock (b)
Symbol b;
{
    if (curlevel >= MAXBLKDEPTH) {
	fatal("nesting depth too large (%d)", curlevel);
    }
    blkstack[curlevel] = curblock;
    ++curlevel;
    curblock = b;
    if (traceblocks) {
	printf("+%d entering block %s\n", curlevel, symname(b));
    }
}

/*
 * Change the current block with saving the previous one,
 * since it is assumed that the symbol for the current one is to be deleted.
 */

public changeBlock (b)
Symbol b;
{
    curblock = b;
}

public enterblock (b)
Symbol b;
{
    if (curblock == nil) {
	b->level = 1;
    } else {
	b->level = curblock->level + 1;
    }
    b->block = curblock;
    pushBlock(b);
}

public exitblock ()
{
    if (curlevel <= 0) {
	panic("nesting depth underflow (%d)", curlevel);
    }
    --curlevel;
    if (traceblocks) {
	printf("-%d exiting block %s\n", curlevel, symname(curblock));
    }
    curblock = blkstack[curlevel];
}

/*
 * Enter a source line or file name reference into the appropriate table.
 * Expanded inline to reduce procedure calls.
 *
 * private enterline (linenumber, address)
 * Lineno linenumber;
 * Address address;
 *  ...
 */

#define enterline(linenumber, address) \
{ \
    register Linetab *lp; \
 \
    assert(linep); \
    lp = linep - 1; \
    if (linenumber != lp->line) { \
	if (address != lp->addr) { \
	    ++lp; \
	} \
	lp->line = linenumber; \
	lp->addr = address; \
	linep = lp + 1; \
    } \
}

/*
 * Read in the namelist from the obj file.
 *
 * Reads and seeks are used instead of fread's and fseek's
 * for efficiency sake; there's a lot of data being read here.
 */

public readobj (file)
String file;
{
    Fileid f;
    struct exec hdr;
    struct nlist nlist;
    register Filetab *ftp;
	char *ptr, tmp[MAXPATHLEN];
	String dir;
	Boolean needed;

    f = open(file, 0);
    if (f < 0) {
	fatal("can't open %s", file);
    }
    read(f, &hdr, sizeof(hdr));
    if (N_BADMAG(hdr)) {
	objsize = 0;
	nlhdr.nsyms = 0;
	nlhdr.nfiles = 0;
	nlhdr.nlines = 0;
    } else {
	objsize = hdr.a_text;
	nlhdr.nsyms = hdr.a_syms / sizeof(nlist);
	nlhdr.nfiles = nlhdr.nsyms;
	nlhdr.nlines = nlhdr.nsyms;
    }
	defvar(identname("$symtotal", true), build(O_LCON, nlhdr.nsyms));
    if (nlhdr.nsyms > 0) {
		lseek(f, (long) N_STROFF(hdr), 0);
		read(f, &(nlhdr.stringsize), sizeof(nlhdr.stringsize));
		nlhdr.stringsize -= 4;
		stringtab = newarr(char, nlhdr.stringsize);
		read(f, stringtab, nlhdr.stringsize);
		allocmaps(nlhdr.nfiles, nlhdr.nlines);
		lseek(f, (long) N_SYMOFF(hdr), 0);
		readsyms(f);
		ordfunctab();
		setnlines();
    	for (ftp = &filetab[0]; ftp < &filetab[nlhdr.nfiles]; ftp++) {
			if(ptr = (char *)strrchr(ftp->filename, '/')) {
				if(ptr != ftp->filename){
    				if (rindex(objname, '/') != nil) {
						strcpy(tmp, objname);
						*(rindex(tmp, '/')) = '\0';
						strcat(tmp, "/");
						strncat(tmp, ftp->filename, ptr - ftp->filename);
    				}
					else {
						strcpy(tmp, ftp->filename);
						*(rindex(tmp, '/')) = '\0';
					}
					needed = true;
					foreach (String, dir, sourcepath)
						if(strcmp(tmp, dir) == 0) {
							needed = false;
							break;
						}
					endfor
					if(needed) {
						list_append(list_item(strdup(tmp)), nil, sourcepath);
					}
					ftp->filename = ++ptr;
				}
			}
		}
		setnfiles();
    } else {
		initsyms();
    }
    close(f);
}

/*
 * Found the beginning of the externals in the object file
 * (signified by the "-lg" or find an external), close the
 * block for the last procedure.
 */

private foundglobals ()
{
	while (curblock->class != PROG) 
		exitblock();
    enterline(0, (linep-1)->addr + 1);
}

/*
 * Read in symbols from object file.
 */

private readsyms (f)
Fileid f;
{
    struct nlist *namelist;
    register struct nlist *np, *ub;
    register String name;
    register Boolean afterlg;
    integer index;
    char *lastchar;

    initsyms();
    namelist = newarr(struct nlist, nlhdr.nsyms);
    read(f, namelist, nlhdr.nsyms * sizeof(struct nlist));
    afterlg = false;
    ub = &namelist[nlhdr.nsyms];
    curnp = &namelist[0];
    np = curnp;
    while (np < ub) {
	index = np->n_un.n_strx;
	if (index != 0) {
	    name = &stringtab[index - 4];
	    /*
             *  If the program contains any .f files a trailing _ is stripped
       	     *  from the name on the assumption it was added by the compiler.
	     *  This only affects names that follow the sdb N_SO entry with
             *  the .f name. 
             */
            if (strip_ and name[0] != '\0' ) {
		lastchar = &name[strlen(name) - 1];
		if (*lastchar == '_') {
		    *lastchar = '\0';
		}
            }
	} else {
	    name = nil;
	} 

	/*
	 * Assumptions:
	 *	not an N_STAB	==> name != nil
	 *	name[0] == '-'	==> name == "-lg"
	 *	name[0] != '_'	==> filename or invisible
	 *
	 * The "-lg" signals the beginning of global loader symbols.
         *
	 */
	if ((np->n_type&N_STAB) != 0) {
	    enter_nl(name, np);
	} else if (name[0] == '-') {
	    afterlg = true;
	    foundglobals();
	} else if (afterlg) {
	    check_global(name, np);
	} else if ((np->n_type&N_EXT) == N_EXT) {
	    afterlg = true;
	    foundglobals();
	    check_global(name, np);
	} else if (name[0] == '_') {
	    check_local(&name[1], np);
	} else if ((np->n_type&N_TEXT) == N_TEXT) {
	    check_filename(name);
	}
	++curnp;
	np = curnp;
    }
    dispose(namelist);
}

/*
 * Get a continuation entry from the name list.
 * Return the beginning of the name.
 */

public String getcont ()
{
    register integer index;
    register String name;

    ++curnp;
    index = curnp->n_un.n_strx;
    if (index == 0) {
	panic("continuation followed by empty stab");
    }
    name = &stringtab[index - 4];
    return name;
}

/*
 * Initialize symbol information.
 */

private initsyms ()
{
    char *progname;
	
	curblock = nil;
    curlevel = 0;
    nesting = 0;
	if(rindex(objname, '/') != nil) {
		progname = rindex(objname, '/') +1;
	} else {
		progname = objname;
	}
    program = insert(identname(progname, true));
    program->class = PROG;
    program->symvalue.funcv.beginaddr = 0;
    program->symvalue.funcv.inline = false;
    newfunc(program, codeloc(program));
    findbeginning(program);
    enterblock(program);
    curmodule = program;
}

/*
 * Free all the object file information that's being stored.
 */

public objfree ()
{
    symbol_free();
    /* keywords_free(); */
    /* names_free(); */
    /* dispose(stringtab); */
    clrfunctab();
}

/*
 * Enter a namelist entry.
 */

private enter_nl (name, np)
String name;
register struct nlist *np;
{
    register Symbol s;
    register Name n;
    char *p;

    s = nil;
    switch (np->n_type) {
	/*
	 * Build a symbol for the FORTRAN common area.  All GSYMS that follow
	 * will be chained in a list with the head kept in common.offset, and
	 * the tail in common.chain.
	 */
	case N_BCOMM:
 	    if (curcomm) {
		curcomm->symvalue.common.chain = commchain;
	    }
	    n = identname(name, true);
 	    find(curcomm, n) where curcomm->class == COMMON endfind(curcomm);
	    if (curcomm == nil) {
		curcomm = insert(n);
		curcomm->class = COMMON;
		curcomm->block = curblock;
		curcomm->language = curlang;	/* curlang is fortran */
		curcomm->level = program->level;
		curcomm->symvalue.common.chain = nil;
	    }
	    commchain = curcomm->symvalue.common.chain;
	    break;

	case N_ECOMM:
	    if (curcomm) {
		curcomm->symvalue.common.chain = commchain;
		curcomm = nil;
	    }
	    break;

	case N_LBRAC:
	    ++nesting;
	    addrstk[nesting] = (linep - 1)->addr;
	    break;

	case N_RBRAC:
	    --nesting;
	    if (addrstk[nesting] == NOADDR) {
		exitblock();
		newfunc(curblock, (linep - 1)->addr);
		addrstk[nesting] = (linep - 1)->addr;
	    }
	    break;

	case N_SLINE:
	    enterline((Lineno) np->n_desc, (Address) np->n_value);
	    break;

	/*
	 * Source files.
	 */
	case N_SO:
	    vmsFileConvert(name);
	    n = identname(name, true);
	    enterSourceModule(n, (Address) np->n_value,
	        (LanguageName) np->n_other);
	    break;

	/*
	 * Textually included files.
	 */
	case N_SOL:
	    enterfile(name, (Address) np->n_value);
	    break;

	/*
	 * These symbols are assumed to have non-nil names.
	 */
	case N_GSYM:
	case N_FUN:
	case N_ENTRY:
	case N_STSYM:
	case N_LCSYM:
	case N_RSYM:
	case N_PSYM:
	case N_LSYM:
	case N_SSYM:
	case N_LENG:
	    if (index(name, ':') == nil) {
		if (not warned) {
		    warned = true;
		    warning("old style symbol information found in \"%s\"",
			curfilename());
		}
	    } else {
		entersym(name, np);
	    }
	    break;

	case N_PC:
	case N_MOD2:
	case N_NSYMS:
	case N_FNAME:
	    break;

	case N_NOMAP:
	    /* Special handling of VAX Debug DST records that will not
	     * map to a stab entry.
	     */
	    p = (index(name, ':'));
	    if (p != nil) {
	        *p = '\0';
	    }
	    mkdstsym(name);
	    break;

	default:
	    printf("warning:  stab entry unrecognized: ");
	    if (name != nil) {
		printf("name %s,", name);
	    }
	    printf("ntype %2x, desc %x, value %x'\n",
		np->n_type, np->n_desc, np->n_value);
	    break;
    }
}

/*
 * Try to find the symbol that is referred to by the given name.  Since it's
 * an external, we need to follow a level or two of indirection.
 */

private Symbol findsym (n, var_isextref)
Name n;
boolean *var_isextref;
{
    register Symbol r, s;

    *var_isextref = false;
    find(s, n) where
	(
	    s->level == program->level and (
		s->class == EXTREF or s->class == VAR or
		s->class == PROC or s->class == FUNC
	    )
	) or (
	    s->block == program and s->class == MODULE
	)
    endfind(s);
    if (s == nil) {
	r = nil;
    } else if (s->class == EXTREF) {
	*var_isextref = true;
	r = s->symvalue.extref;
	delete(s);

	/*
	 * Now check for another level of indirection that could come from
	 * a forward reference in procedure nesting information.  In this case
	 * the symbol has already been deleted.
	 */
	if (r != nil and r->class == EXTREF) {
	    r = r->symvalue.extref;
	}
/*
    } else if (s->class == MODULE) {
	s->class = FUNC;
	s->level = program->level;
	r = s;
 */
    } else {
	r = s;
    }
    return r;
}

/*
 * Create a symbol for a text symbol with no source information.
 * We treat it as an assembly language function.
 */

private Symbol deffunc (n)
Name n;
{
    Symbol f;

    f = insert(n);
    f->language = findlanguage(ASSEMBLER);	/* RBN 2-27-90 */
    f->class = FUNC;
    f->type = t_int;
    f->block = curblock;
    f->level = program->level;
    f->symvalue.funcv.src = false;
    f->symvalue.funcv.inline = false;
    return f;
}

/*
 * Create a symbol for a data or bss symbol with no source information.
 * We treat it as an assembly language variable.
 */

private Symbol db_defvar (n)
Name n;
{
    Symbol v;

    v = insert(n);
    v->language = findlanguage(ASSEMBLER);	/* RBN 2-27-90 */
    v->class = VAR;
    v->type = t_int;
    v->level = program->level;
    v->block = curblock;
    return v;
}

/*
 * Update a symbol entry with a text address.
 */

private updateTextSym (s, isnewfunc, addr)
Symbol s;
Boolean isnewfunc;
Address addr;
{
    if (s->class == VAR) {
	s->symvalue.offset = addr;
    } else {
	s->symvalue.funcv.beginaddr = addr;
	if (isnewfunc) {
	    newfunc(s, codeloc(s));
	    findbeginning(s);
	}
    }
}

/*
 * Check to see if a global _name is already in the symbol table,
 * if not then insert it.
 */

private check_global (name, np)
String name;
register struct nlist *np;
{
    register Name n;
    register Symbol t, u;
    char buf[4096];
    Boolean isextref, isnewfunc, isdup;
    integer count;

    isdup = isnewfunc = false;
    if (not streq(name, "_end")) {
	if (name[0] == '_') {
	    n = identname(&name[1], true);
	    isnewfunc = true;
	} else {
	    n = identname(name, true);
	    if (lookup(n) != nil) {
	        isdup = true;
	    }
	}
	if ((np->n_type&N_TYPE) == N_TEXT) {
	    count = 0;
	    t = findsym(n, &isextref);
	    if (t != nil and isdup) {
	        if (not isfunc(t)) {
		    sprintf(buf, "$%s", name);    
		    n = identname(buf, false);
		    t = findsym(n, &isextref);
		    isnewfunc = false;
		} else {
		    if (np->n_value != 0) {
		        isnewfunc = true;
		    }
		}
	    }
	    while (isextref) {
		++count;
		updateTextSym(t, isnewfunc, np->n_value);
		t = findsym(n, &isextref);
	    }
	    if (count == 0) {
	        if (t == nil) {
		    t = deffunc(n);
		    updateTextSym(t, isnewfunc, np->n_value);
		    if (tracesyms) {
			printdecl(t);
		    }
		} else {
		    if (t->class == MODULE) {
			u = t;
			t = deffunc(n);
			t->block = u;
			if (tracesyms) {
			    printdecl(t);
			}
		    }
		    updateTextSym(t, isnewfunc, np->n_value);
		}
	    }
	} else if (((np->n_type&N_TYPE) == N_BSS) || 
                  (np->n_type&N_TYPE) == N_DATA) {
	    find(t, n) where
		t->class == COMMON
	    endfind(t);
	    if (t != nil) {
		u = (Symbol) t->symvalue.common.offset;
		while (u != nil) {
		    u->symvalue.offset = u->symvalue.common.offset+np->n_value;
		    u = u->symvalue.common.chain;
		}
            } else {
		check_var(np, n);
	    }
        } else {
	    check_var(np, n);
	}
    }
}

/*
 * Check to see if a namelist entry refers to a variable.
 * If not, create a variable for the entry.  In any case,
 * set the offset of the variable according to the value field
 * in the entry.
 *
 * If the external name has been referred to by several other symbols,
 * we must update each of them.
 */

private check_var (np, n)
struct nlist *np;
register Name n;
{
    register Symbol t, u, next;
    Symbol conflict;

    t = lookup(n);
    if (t == nil) {
	t = db_defvar(n);
	t->symvalue.offset = np->n_value;
	if (tracesyms) {
	    printdecl(t);
	}
    } else {
	conflict = nil;
	do {
	    next = t->next_sym;
	    if (t->name == n) {
		if (t->class == MODULE and t->block == program) {
		    conflict = t;
		} else if (t->class == EXTREF and t->level == program->level) {
		    u = t->symvalue.extref;
		    while (u != nil and u->class == EXTREF) {
			u = u->symvalue.extref;
		    }
		    u->symvalue.offset = np->n_value;
		    delete(t);
		} else if (t->level == program->level and
		    (t->class == VAR or t->class == PROC or t->class == FUNC)
		) {
		    conflict = nil;
		    if (t->symvalue.offset == nil) {
    		    	t->symvalue.offset = np->n_value;
		    }
		}
	    }
	    t = next;
	} while (t != nil);
	if (conflict != nil) {
	    u = db_defvar(n);
	    u->block = conflict;
	    u->symvalue.offset = np->n_value;
	}
    }
}

/*
 * Check to see if a local _name is known in the current scope.
 * If not then enter it.
 */

private check_local (name, np)
String name;
register struct nlist *np;
{
    register Name n;
    register Symbol t, cur;

    n = identname(name, true);
    cur = ((np->n_type&N_TYPE) == N_TEXT) ? curmodule : curblock;
    find(t, n) where t->block == cur endfind(t);
    if (t == nil) {
	t = insert(n);
	t->language = findlanguage(ASSEMBLER);
	t->type = t_int;
	t->block = cur;
	t->level = cur->level;
	if ((np->n_type&N_TYPE) == N_TEXT) {
	    t->class = FUNC;
	    t->symvalue.funcv.src = false;
	    t->symvalue.funcv.inline = false;
	    t->symvalue.funcv.beginaddr = np->n_value;
	    newfunc(t, codeloc(t));
	    findbeginning(t);
	} else {
	    t->class = VAR;
	    t->symvalue.offset = np->n_value;
	}
    }
}

/*
 * Check to see if a symbol corresponds to a object file name.
 * For some reason these are listed as in the text segment.
 */

private check_filename (name)
String name;
{
    register String mname;
    register integer i;
    Name n;
    Symbol s;

    mname = strdup(name);
    i = strlen(mname) - 2;
    if (i >= 0 and mname[i] == '.' and mname[i+1] == 'o') {
	mname[i] = '\0';
	--i;
	while (mname[i] != '/' and i >= 0) {
	    --i;
	}
	n = identname(&mname[i+1], true);
	find(s, n) where s->block == program and s->class == MODULE endfind(s);
	if (s == nil) {
	    s = insert(n);
	    s->language = findlanguage(ASSEMBLER);
	    s->class = MODULE;
	    s->symvalue.funcv.beginaddr = 0;
	    findbeginning(s);
	}
	while (curblock->class != PROG) 
		exitblock();
	enterblock(s);
	curmodule = s;
    }
}

/*
 * Check to see if a symbol is about to be defined within an unnamed block.
 * If this happens, we create a procedure for the unnamed block, make it
 * "inline" so that tracebacks don't associate an activation record with it,
 * and enter it into the function table so that it will be detected
 * by "whatblock".
 */

public chkUnnamedBlock ()
{
    register Symbol s;
    static int bnum = 0;
    char buf[100];
    Address startaddr;

    if (nesting > 0 and addrstk[nesting] != NOADDR) {
	startaddr = (linep - 1)->addr;
	++bnum;
	sprintf(buf, "$b%d", bnum);
	s = insert(identname(buf, false));
	s->language = curlang;
	s->class = PROC;
	s->symvalue.funcv.src = true;
	s->symvalue.funcv.inline = true;
	s->symvalue.funcv.beginaddr = startaddr;
	enterblock(s);
	newfunc(s, startaddr);
	addrstk[nesting] = NOADDR;
    }
}

/*
 * Compilation unit.  C associates scope with filenames
 * so we treat them as "modules".  The filename without
 * the suffix is used for the module name.
 *
 * Because there is no explicit "end-of-block" mark in
 * the object file, we must exit blocks for the current
 * procedure and module.
 */

private enterSourceModule(n, addr, lang)
Name n;
Address addr;
LanguageName lang;
{
    register Symbol s;
    Name nn;
    String mname, suffix;

    mname = strdup(ident(n));
    if (rindex(mname, '/') != nil) {
	mname = rindex(mname, '/') + 1;
    }
    suffix = rindex(mname, '.');
    if (suffix != nil) {
	*suffix++ = '\0';
    }
    if (lang != UNKNOWN) {
        curlang = findlanguage(lang);
    } else {
	curlang = findlanguage(getlangname(suffix));
    }
    if (curlang == findlanguage(FORTRAN)) {
	strip_ = true;
    } 
    if (not (*language_op(curlang, L_HASMODULES))()) {
	while (curblock->class != PROG) 
		exitblock();
	nn = identname(mname, true);
	if (curmodule == nil or curmodule->name != nn) {
	    s = insert(nn);
	    s->class = MODULE;
	    s->symvalue.funcv.beginaddr = 0;
	    findbeginning(s);
	} else {
	    s = curmodule;
	}
	s->language = curlang;
	enterblock(s);
	curmodule = s;
    }
    if (program->language == nil) {
	program->language = curlang;
    }
    warned = false;
    enterfile(ident(n), addr);
    initTypeTable();
}

/*
 * Hack for VMS transported object modules:
 *	Remove NODE::DEV:[dir] and ;ver.
 *	Also lowercase the name.
 */
private vmsFileConvert(fname)
String fname;
{
    register char *cp, *dp;

    if (fname == nil) {
	return;
    }

    /* Only convert the name if it contains :] and ; */
    cp = index(fname, ']');
    if (cp and index(fname,':') and index(fname,';')) {
	for (++cp, dp = fname; *cp != ';'; cp++, dp++) {
	    if (*cp >= 'A' and *cp <= 'Z') {
	        *dp = *cp + 'a' - 'A';		/* lower case */
	    } else {
	        *dp = *cp;
	    }
	}
	*dp = '\0';
    }
}   /* end vmsFileConvert() */

/*
 * Allocate file and line tables and initialize indices.
 */

private allocmaps (nf, nl)
integer nf, nl;
{
    if (filetab != nil) {
	dispose(filetab);
    }
    if (linetab != nil) {
	dispose(linetab);
    }
    if (nf > 0) {
        filetab = newarr(Filetab, nf);
    } else {
        filetab = nil;
    }
    if (nl > 0) {
    	linetab = newarr(Linetab, nl);
    } else {
        linetab = nil;
    }

    filep = filetab;
    linep = linetab;
}

/*
 * Add a file to the file table.
 *
 * If the new address is the same as the previous file address
 * this routine used to not enter the file, but this caused some
 * problems so it has been removed.  It's not clear that this in
 * turn may not also cause a problem.
 */

private enterfile (filename, addr)
String filename;
Address addr;
{
    assert(filep);
    filep->addr = addr;
    filep->filename = filename;
    filep->lineindex = linep - linetab;
    ++filep;
}

/*
 * Since we only estimated the number of lines (and it was a poor
 * estimation) and since we need to know the exact number of lines
 * to do a binary search, we set it when we're done.
 */

private setnlines ()
{
    nlhdr.nlines = linep - linetab;
}

/*
 * Similarly for nfiles ...
 */

private setnfiles ()
{
    nlhdr.nfiles = filep - filetab;
    setsource(filetab[0].filename);
}
