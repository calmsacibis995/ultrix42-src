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
 *  008	- Added support for vectors.							*
 *		  (L Miller, 18JAN90)									*
 *									*
 *	007 - Added depositField based on MIPS version; needed to	*
 *	      make assign to a bitfield work.				*
 *	      (jlr, January 21, 1989)					*
 *									*
 *	006 - Re-merged 4.3 changes.  Negates 001,003.			*
 *	      (jlr, June 9, 1987)					*
 *									*
 *	005 - Fixed double-execute of one-line loops.			*
 *	      (Jon Reeves, April 22, 1987)				*
 *									*
 *	004 - Merged in 4.3 changes.					*
 *	      (vjh, April 29, 1986)					*
 *									*
 *	003 - Added mkuchar and mkushort macros.			*
 *	      (vjh, July 8, 1985)					*
 *									*
 *	002 - Added a call to bpact() in nextaddr(), under the 		*
 *	      conditional-branch case (eg. O_BGEQ).  After the call	*
 * 	      to pstep(), dbx needed to see if it had stopped at a	*
 *	      breakpoint.  This was not done before because dbx		*
 *	      always assumed no breakpoints between source lines 	*
 *	      when single stepping.					*
 *	      Also - moved call to bpact() under the O_RET case, 	*
 *	      because stepto() now calls bpact() as well.  Still 	*
 *	      need to call it after pstep, though.			*
 *	      (vjh, April 25, 1985)					*
 *									*
 *	001 - Temporarily renamed findnextaddr() to nextaddr; 		*
 *	      commented out nextaddr().  Fixes usignal() bug 		*
 *	      (single stepping - dostep() gets into infinite loop).	*
 *	      (Victoria Holt, April 9, 1985)				*
 *									*
 ************************************************************************/

/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char sccsid[] = "@(#)machine.c	4.2	Ultrix	11/9/90";
#endif not lint

/*
 * Target machine dependent stuff.
 */

#include "defs.h"
#include "machine.h"
#include "process.h"
#include "runtime.h"
#include "events.h"
#include "main.h"
#include "symbols.h"
#include "source.h"
#include "mappings.h"
#include "object.h"
#include "keywords.h"
#include "ops.h"
#include "eval.h"
#include <signal.h>
#include <errno.h>
#include <sys/sysinfo.h>

#ifndef public
typedef unsigned int Address;
typedef unsigned char Byte;
typedef unsigned int Word;

#define NVREG 64

typedef struct _vquad {long val[2]} Vquad;
typedef struct Vreg *Vreg;
struct Vreg {
    Vquad reg[NVREG];
};

#define NREG 16

#define ARGP 12
#define FRP 13
#define STKP 14
#define PROGCTR 15

#define VCR 16
#define VLR 17
#define VMR 18
#define VAER 19

#define A_VCR 1
#define A_VLR 2
#define A_VMRLO 3
#define A_VMRHI 4
#define A_VREGLO 5
#define A_VREGHI 6
#define A_VAER 7

#define BITSPERBYTE 8
#define BITSPERWORD (BITSPERBYTE * sizeof(Word))

#define nargspassed(frame) argn(0, frame)

#include "source.h"
#include "symbols.h"

Address pc;
Address prtaddr;

#endif

private Address printop();

/*
 * Decode and print the instructions within the given address range.
 */

public printinst(lowaddr, highaddr)
Address lowaddr;
Address highaddr;
{
    register Address addr;

    for (addr = lowaddr; addr <= highaddr; ) {
	addr = printop(addr);
    }
    prtaddr = addr;
}

/*
 * Another approach:  print n instructions starting at the given address.
 */

public printninst(count, addr)
int count;
Address addr;
{
    register Integer i;
    register Address newaddr;

    if (count <= 0) {
	error("non-positive repetition count");
    } else {
	newaddr = addr;
	for (i = 0; i < count; i++) {
	    newaddr = printop(newaddr);
	}
	prtaddr = newaddr;
    }
}

/*
 * Hacked version of adb's VAX instruction decoder.
 */

private Address printop(addr)
Address addr;
{
    Optab op;
	VaxOpcode ins, ins2;
    unsigned char mode;
    int argtype, amode, argno, argval;
	float f;
	double d;
    String reg;
    Boolean indexf;
    short offset;
    Ctrl_word ctrl_word;
    Boolean ctrlf;
    char *iname;
    int i;
    struct {
      	int     regval;
      	Boolean regdef;
    } vregs[3];

    argval = 0;
    indexf = false;
    for (i = 0; i < 3; i++) {
      	vregs[i].regdef = false;
    }
    ctrlf = false;
    printf("%08x  ", addr);
    iread(&ins, addr, sizeof(ins));
    addr += 1;
    op = optab[ins];
    argno = 0;
	if (ins == O_ESCD) {
      	iread(&ins2, addr, sizeof(ins));
      	addr += 1;
      	op = eoptab[ins2-FIRST_EOP];
      	iname = op.iname;
      	if ((ins2 == O_MTVP) || (ins2 == O_MFVP) || (ins2 == O_VSYNC)) {
			ctrlf = true;
			iread(&mode, addr, sizeof(mode));
			addr += 1;
			argno = 1;
			switch (ins2) {
				case O_MTVP: 
					iname = mtvpname[mode];
					break;
				case O_MFVP:
					iname = mfvpname[mode];
					break;
				case O_VSYNC:
					iname = vsyncname[mode];
					break;
			}
			printf("%s", iname);
	  		if(strlen(iname) < 6)
	    		printf("\t");
      	}
      	if (op.fmt != 0) {
			ctrlf = true;
			iread(&mode, addr, sizeof(mode));
			addr += 1;
			reg = regname[regnm(mode)];
			amode = addrmode(mode);
			argval = getdisp(addr, 2, reg, amode);
			addr += 2;
			ctrl_word = *(Ctrl_word *)&argval;
			argno = 1;
			switch (vinst_fmt[op.fmt].va) {
				case vreg:
	  				vregs[0].regdef = true;
	  				vregs[0].regval = ctrl_word.va;
	  				break;
				case cmp:
	  				iname = vcmpname[((VaxOpcode) op.val - O_VVCMP)]
															[ctrl_word.va];
	  				break;
				case cvt:
	  				iname = vcvtname[ctrl_word.va];
	  				break;
				default:
	  				break;
			}
			switch (vinst_fmt[op.fmt].vb) {
				case vreg:
	  				vregs[1].regdef = true;
	  				vregs[1].regval = ctrl_word.vb;
	  				break;
				case cmp:
	  				iname = vcmpname[((VaxOpcode) op.val - O_VVCMP)]
															[ctrl_word.vb];
	  				break;
				case cvt:
	  				iname = vcvtname[ctrl_word.vb];
	  				break;
				default:
	  				break;
			}
			switch (vinst_fmt[op.fmt].vc) {
				case vreg:
	  				vregs[2].regdef = true;
	  				vregs[2].regval = ctrl_word.vc;
	  				break;
				case cmp:
	  				iname = vcmpname[((VaxOpcode) op.val - O_VVCMP)]
															[ctrl_word.vc];
	  				break;
				case cvt:
	  				iname = vcvtname[ctrl_word.vc];
	  				break;
				default:
	  				break;
			}
			printf("%s", iname);
			if((strlen(iname) + printmod(ins2, ctrl_word.modifiers)) < 6)
	    		printf("\t");

      	}
	}

    if (ctrlf == false) {
      printf("%s", op.iname);
	  if(strlen(op.iname) < 6)
	    printf("\t");
	}

    for (argno; argno < op.numargs; argno++) {
	if (indexf == true) {
	    indexf = false;
	} else if ((argno == 0) || (ctrlf == true)) {
	    ctrlf = false;
	    printf("\t");
      	if ((ins2 == O_VSTL) || (ins2 == O_VSTQ) ||
	  				(ins2 == O_VSCATL) || (ins2 == O_VSCATQ)) {
			printf("v%d,", vregs[2].regval);
      		vregs[2].regdef = false;
      	}
	} else {
	    printf(",");
	}
	argtype = op.argtype[argno];
	if (is_branch_disp(argtype)) {
	    mode = 0xAF + (typelen(argtype) << 5);
	} else {
	    iread(&mode, addr, sizeof(mode));
	    addr += 1;
	}
	reg = regname[regnm(mode)];
	amode = addrmode(mode);
	switch (amode) {
	    case LITSHORT:
	    case LITUPTO31:
	    case LITUPTO47:
	    case LITUPTO63:
		if (typelen(argtype) == TYPF || typelen(argtype) == TYPD ||
		    typelen(argtype) == TYPG)
		    printf("$%s", fltimm[mode]);
		else
		    printf("$%x", mode);
		argval = mode;
		break;

	    case INDEX:
		printf("[%s]", reg);
		indexf = true;
		argno--;
		break;

	    case REG:
		printf("%s", reg);
		break;

	    case REGDEF:
		printf("(%s)", reg);
		break;

	    case AUTODEC:
		printf("-(%s)", reg);
		break;

	    case AUTOINC:
		if (reg != regname[PROGCTR]) {
		    printf("(%s)+", reg);
		} else {
		    printf("$");
		    switch (typelen(argtype)) {
			case TYPB:
			    argval = printdisp(addr, 1, reg, amode);
			    addr += 1;
			    break;

			case TYPW:
			    argval = printdisp(addr, 2, reg, amode);
			    addr += 2;
			    break;

			case TYPL:
			    argval = printdisp(addr, 4, reg, amode);
			    addr += 4;
			    break;

			case TYPF:
			    iread(&f, addr, sizeof(f));
				printf("0f%e", f);
			    addr += 4;
			    break;

			case TYPD:
			    iread(&d, addr, sizeof(d));
				printf("0d%E", d);
			    addr += 8;
			    break;

			case TYPQ:
			case TYPG:
			    iread(&argval, addr, sizeof(argval));
			    printf("%06x", argval);
			    iread(&argval, addr+4, sizeof(argval));
			    printf("%06x", argval);
			    addr += 8;
			    break;
		    }
		}
		break;

	    case AUTOINCDEF:
		if (reg == regname[PROGCTR]) {
		    printf("*$");
		    argval = printdisp(addr, 4, reg, amode);
		    addr += 4;
		} else {
		    printf("*(%s)+", reg);
		}
		break;

	    case BYTEDISP:
		argval = printdisp(addr, 1, reg, amode);
		addr += 1;
		break;

	    case BYTEDISPDEF:
		printf("*");
		argval = printdisp(addr, 1, reg, amode);
		addr += 1;
		break;

	    case WORDDISP:
		argval = printdisp(addr, 2, reg, amode);
		addr += 2;
		break;

	    case WORDDISPDEF:
		printf("*");
		argval = printdisp(addr, 2, reg, amode);
		addr += 2;
		break;

	    case LONGDISP:
		argval = printdisp(addr, 4, reg, amode);
		addr += 4;
		break;

	    case LONGDISPDEF:
		printf("*");
		argval = printdisp(addr, 4, reg, amode);
		addr += 4;
		break;
	}
    }

    for (i = 0; i <= 2; i++) {
      	if (vregs[i].regdef == true) {
			if (ctrlf == true) {
	  			printf("\t");
	  			ctrlf = false;
			} else
	  			printf(",");
			printf("v%d", vregs[i].regval);
      	}
    }
    
    if (ins == O_CASEB || ins == O_CASEW || ins == O_CASEL) {
	for (argno = 0; argno <= argval; argno++) {
	    iread(&offset, addr, sizeof(offset));
	    printf("\n\t\t%d", offset);
	    addr += 2;
	}
    }
    
    printf("\n");
    return addr;
}    

/*
 * Print the displacement of an instruction that uses displacement
 * addressing.
 */

private int printdisp(addr, nbytes, reg, mode)
Address addr;
int nbytes;
char *reg;
int mode;
{
    char byte;
    short hword;
    int argval;
    Symbol f;

    switch (nbytes) {
	case 1:
	    iread(&byte, addr, sizeof(byte));
	    argval = byte;
	    break;

	case 2:
	    iread(&hword, addr, sizeof(hword));
	    argval = hword;
	    break;

	case 4:
	    iread(&argval, addr, sizeof(argval));
	    break;
    }
    if (reg == regname[PROGCTR] && mode >= BYTEDISP) {
	argval += addr + nbytes;
    }
    if (reg == regname[PROGCTR]) {
	f = whatblock((Address) argval + 2);
	if (codeloc(f) == argval + 2) {
	    printf("%s", symname(f));
	} else {
	    printf("%x", argval);
	}
    } else {
	if (varIsSet("$hexoffsets")) {
	    if (argval < 0) {
		printf("-%x(%s)", -(argval), reg);
	    } else {
		printf("%x(%s)", argval, reg);
	    }
	} else {
	    printf("%d(%s)", argval, reg);
	}
    }
    return argval;
}

/*
 * Print the contents of the addresses within the given range
 * according to the given format.
 */

typedef struct {
    String name;
    String printfstring;
    int length;
} Format;

private Format fmt[] = {
    { "d", " %d", sizeof(short) },
    { "D", " %ld", sizeof(long) },
    { "o", " %o", sizeof(short) },
    { "O", " %lo", sizeof(long) },
    { "x", " %04x", sizeof(short) },
    { "X", " %08x", sizeof(long) },
    { "b", " \\%o", sizeof(char) },
    { "c", " '%c'", sizeof(char) },
    { "s", "%c", sizeof(char) },
    { "f", " %f", sizeof(float) },
    { "g", " %g", sizeof(double) },
    { "v", " (%ld,%ld)", 2 * sizeof(long) },
    { "vx", " (0x%X,0x%X)", 2 * sizeof(long) },
    { "vf", " (%f,%f)", sizeof(Vquad) },
    { nil, nil, 0 }
};

private Format *findformat(s)
String s;
{
    register Format *f;

    f = &fmt[0];
    while (f->name != nil and not streq(f->name, s)) {
	++f;
    }
    if (f->name == nil) {
	error("bad print format \"%s\"", s);
    }
    return f;
}

public Address printdata(lowaddr, highaddr, format)
Address lowaddr;
Address highaddr;
String format;
{
    register int n;
    register Address addr;
    register Format *f;
    int value;

    if (lowaddr > highaddr) {
	error("first address larger than second");
    }
    f = findformat(format);
    n = 0;
    value = 0;
    for (addr = lowaddr; addr <= highaddr; addr += f->length) {
	if (n == 0) {
	    printf("%08x: ", addr);
	}
	if(streq(format, "vf")) {
		Vquad value;
		
		dread(&value, addr, f->length);
		printf("(");
	    prtreal(value.val[0]);
		printf(",");
	    prtreal(value.val[1]);
		printf(")");
	}
	else {
		dread(&value, addr, f->length);
		if(streq(format, "g") || streq(format, "f")) {
	    	prtreal(value);
		} else {
			printf(f->printfstring, value);
		}
	}
	++n;
	if (n >= (16 div f->length)) {
	    putchar('\n');
	    n = 0;
	}
    }
    if (n != 0) {
	putchar('\n');
    }
    prtaddr = addr;
    return addr;
}

/*
 * The other approach is to print n items starting with a given address.
 */

public printndata(count, startaddr, format)
int count;
Address startaddr;
String format;
{
    register int i, n;
    register Address addr;
    register Format *f;
    register Boolean isstring;
    char c;
    union {
	char charv;
	short shortv;
	int intv;
	float floatv;
	double doublev;
    } value;

    if (count <= 0) {
	error("non-positive repetition count");
    }
    f = findformat(format);
    isstring = (Boolean) streq(f->name, "s");
    n = 0;
    addr = startaddr;
    value.intv = 0;
    for (i = 0; i < count; i++) {
	if (n == 0) {
	    printf("%08x: ", addr);
	}
	if (isstring) {
	    putchar('"');
	    dread(&c, addr, sizeof(char));
	    while (c != '\0') {
		printchar(c);
		++addr;
		dread(&c, addr, sizeof(char));
	    }
	    putchar('"');
	    putchar('\n');
	    n = 0;
	    addr += sizeof(String);
	} else {
		if(streq(format, "vf")) {
			Vquad value;
		
			dread(&value, addr, f->length);
			printf(" (");
	    	prtreal(value.val[0]);
			printf(",");
	    	prtreal(value.val[1]);
			printf(")");
		}
		else {
			value.doublev = 0.0;
			dread(&value, addr, f->length);
			if(streq(format, "g") || streq(format, "f")) {
				printf(" ");
	    		prtreal(value);
			}else {
				printf(f->printfstring, value);
			}
		}
	    ++n;
	    if (n >= (16 div f->length)) {
		putchar('\n');
		n = 0;
	    }
	    addr += f->length;
	}
    }
    if (n != 0) {
	putchar('\n');
    }
    prtaddr = addr;
}

/*
 * Print out a value according to the given format.
 */

public printvalue(v, format)
long v;
String format;
{
    Format *f;
    char *p, *q;

    f = findformat(format);
    if (streq(f->name, "s")) {
	putchar('"');
	p = (char *) &v;
	q = p + sizeof(v);
	while (p < q) {
	    printchar(*p);
	    ++p;
	}
	putchar('"');
    } else {
	printf(f->printfstring, v);
    }
    putchar('\n');
}

/*
 * Print out an execution time error.
 * Assumes the source position of the error has been calculated.
 *
 * Have to check if the -r option was specified; if so then
 * the object file information hasn't been read in yet.
 */

public printerror()
{
    extern Integer sys_nsig;
    extern String sys_siglist[];
    integer err;

    if (isfinished(process)) {
	err = exitcode(process);
	if (err == 0) {
	    printf("\"%s\" terminated normally\n", objname);
	} else {
	    printf("\"%s\" terminated abnormally (exit code %d)\n",
		objname, err
	    );
	}
	erecover();
    }
    if (runfirst) {
	fprintf(stderr, "Entering debugger ...\n");
	init();
    }
    err = errnum(process);
    putchar('\n');
    printsig(err);
    putchar(' ');
    printloc();
    putchar('\n');
    if (curline > 0) {
	printlines(curline, curline);
    } else {
	printinst(pc, pc);
    }
    erecover();
}

/*
 * Print out a signal.
 */

private String illinames[] = {
    "reserved addressing fault",
    "privileged instruction fault",
    "reserved operand fault"
};

private String fpenames[] = {
    nil,
    "integer overflow trap",
    "integer divide by zero trap",
    "floating overflow trap",
    "floating/decimal divide by zero trap",
    "floating underflow trap",
    "decimal overflow trap",
    "subscript out of range trap",
    "floating overflow fault",
    "floating divide by zero fault",
    "floating underflow fault"
};

public printsig (signo)
integer signo;
{
    integer code;

    if (signo < 0 or signo > sys_nsig) {
	printf("[signal %d]", signo);
    } else {
	printf("%s", sys_siglist[signo]);
    }
    code = errcode(process);
    if (signo == SIGILL) {
	if (code >= 0 and code < sizeof(illinames) / sizeof(illinames[0])) {
	    printf(" (%s)", illinames[code]);
	}
    } else if (signo == SIGFPE) {
	if (code > 0 and code < sizeof(fpenames) / sizeof(fpenames[0])) {
	    printf(" (%s)", fpenames[code]);
	}
    }
}

/*
 * Merge bit field into old value (on top of expression stack).
 */

public void depositField(value, s)
long value;
Symbol s;
{
    unsigned n;
    unsigned long offset;
    unsigned long length;

    offset = s->symvalue.field.offset mod 8;
    length = s->symvalue.field.length;
    n = pop(long);

    n = (n & ((1 << length) - 1));
    n = (n << offset) |
	(value & ~( ((1 << length) - 1) << offset ));
    push(long, n);
}

/*
 * Note the termination of the program.  We do this so as to avoid
 * having the process exit, which would make the values of variables
 * inaccessible.  We do want to flush all output buffers here,
 * otherwise it'll never get done.
 */

public endprogram()
{
    Integer exitcode;

    stepto(nextaddr(pc, true));
    printnews();
    exitcode = argn(1, nil);
    if (exitcode != 0) {
	printf("\nexecution completed (exit code %d)\n", exitcode);
    } else {
	printf("\nexecution completed\n");
    }
    getsrcpos();
    erecover();
}

/*
 * Single step the machine a source line (or instruction if "inst_tracing"
 * is true).  If "isnext" is true, skip over procedure calls.
 */

private Address getcall();

public Boolean dostep(isnext, isvector)
Boolean isnext, isvector;
{
    register Address addr;
    register Lineno line;
    String filename;
	Boolean ret = false;

    addr = nextaddr(pc, isnext);
	if(!(isvector && found_a_vec_inst)) {
    	if (not inst_tracing and nlhdr.nlines != 0) {
			line = linelookup(addr);
			while (line == 0) {
	    		addr = nextaddr(addr, isnext);
	    		line = linelookup(addr);
			}
			curline = line;
    	} else {
			curline = 0;
    	}
/* jlr005
 * No need to check for one-line loops: since nextaddr() steps if there's
 * a jump, if addr==starting addr, there must have been a jump, and we're
 * already done.
 */
    	stepto(addr);
	}
	else {
		ret = true;
	}
    filename = srcfilename(addr);
    setsource(filename);
	return(ret);
}

/*
 * Compute the next address that will be executed from the given one.
 * If "isnext" is true then consider a procedure call as straight line code.
 *
 * We must unfortunately do much of the same work that is necessary
 * to print instructions.  In addition we have to deal with branches.
 * Unconditional branches we just follow, for conditional branches
 * we continue execution to the current location and then single step
 * the machine.  We assume that the last argument in an instruction
 * that branches is the branch address (or relative offset).
 */

private Address findnextaddr();

public Address nextaddr(startaddr, isnext)
Address startaddr;
boolean isnext;
{
    Address addr;

    addr = usignal(process);
    if (addr == 0 or addr == 1) {
	addr = findnextaddr(startaddr, isnext);
    }
    return addr;
}

/*
 * Determine if it's ok to skip function f entered by instruction ins.
 * If so, we're going to compute the return address and step to it.
 * Therefore we cannot skip over a function entered by a jsb or bsb,
 * since the return address is not easily computed for them.
 */

private boolean skipfunc (ins, f)
VaxOpcode ins;
Symbol f;
{
    boolean b;

    b = (boolean) (
	ins != O_JSB and ins != O_BSBB and ins != O_BSBW and
	not inst_tracing and nlhdr.nlines != 0 and
	nosource(curfunc) and canskip(curfunc)
    );
    return b;
}

private Address findnextaddr(startaddr, isnext)
Address startaddr;
Boolean isnext;
{
    register Address addr;
    Optab op;
    VaxOpcode ins, ins2;
    unsigned char mode;
    int argtype, amode, argno, argval;
    String r;
    Boolean indexf;
    enum { KNOWN, SEQUENTIAL, BRANCH } addrstatus;

    argval = 0;
    indexf = false;
    addr = startaddr;
	found_a_vec_inst = false;
    iread(&ins, addr, sizeof(ins));
    switch (ins) {
	/*
	 * It used to be that unconditional jumps and branches were handled
	 * by taking their destination address as the next address.  While
	 * saving the cost of starting up the process, this approach
	 * doesn't work when jumping indirect (since the value in the
	 * register might not yet have been set).
	 *
	 * So unconditional jumps and branches are now handled the same way
	 * as conditional jumps and branches.
	 *
	case O_BRB:
	case O_BRW:
	    addrstatus = BRANCH;
	    break;
	 *
	 */
	    
	case O_BSBB:
	case O_BSBW:
	case O_JSB:
	case O_CALLG:
	case O_CALLS:
	    addrstatus = KNOWN;
	    stepto(addr);
	    pstep(process, DEFSIG);
	    addr = reg(PROGCTR);
	    pc = addr;
	    setcurfunc(whatblock(pc));
	    if (not isbperr()) {
		printstatus();
		/* NOTREACHED */
	    }
	    bpact();
	    if (isnext or skipfunc(ins, curfunc)) {
		addrstatus = KNOWN;
		addr = return_addr();
		stepto(addr);
		bpact();
	    } else {
		callnews(/* iscall = */ true);
	    }
	    break;

	case O_RSB:
	case O_RET:
	    addrstatus = KNOWN;
	    stepto(addr);
	    callnews(/* iscall = */ false);
	    pstep(process, DEFSIG);
	    addr = reg(PROGCTR);
	    pc = addr;
	    if (not isbperr()) {
		printstatus();
	    }
	    bpact();
	    break;

	case O_BRB: case O_BRW:
	case O_JMP: /* because it may be jmp (r1) */
	case O_BNEQ: case O_BEQL: case O_BGTR:
	case O_BLEQ: case O_BGEQ: case O_BLSS:
	case O_BGTRU: case O_BLEQU: case O_BVC:
	case O_BVS: case O_BCC: case O_BCS:
	case O_CASEB: case O_CASEW: case O_CASEL:
	case O_BBS: case O_BBC: case O_BBSS: case O_BBCS:
	case O_BBSC: case O_BBCC: case O_BBSSI:
	case O_BBCCI: case O_BLBS: case O_BLBC:
	case O_ACBL: case O_AOBLSS: case O_AOBLEQ:
	case O_SOBGEQ: case O_SOBGTR:
	    addrstatus = KNOWN;
	    stepto(addr);
	    pstep(process, DEFSIG);
	    addr = reg(PROGCTR);
	    pc = addr;
	    if (not isbperr()) {
		printstatus();
	    }
	    bpact();
	    break;

	default:
	    addrstatus = SEQUENTIAL;
	    break;
    }
    if (addrstatus != KNOWN) {
	addr += 1;
	op = optab[ins];
	argno= 0;
	if (ins == O_ESCD) {
      	iread(&ins2, addr, sizeof(ins));
      	addr += 1;
      	op = eoptab[ins2-FIRST_EOP];
      	if ((ins2 == O_MTVP) || (ins2 == O_MFVP) || (ins2 == O_VSYNC)) {
			argno = 1;
			addr += 1;
			found_a_vec_inst = true;
      	}
      	if (op.fmt != 0) {
			argno = 1;
			addr += 3;
			found_a_vec_inst = true;
      	}
	}
	for (argno; argno < op.numargs; argno++) {
	    if (indexf == true) {
		indexf = false;
	    }
	    argtype = op.argtype[argno];
	    if (is_branch_disp(argtype)) {
		mode = 0xAF + (typelen(argtype) << 5);
	    } else {
		iread(&mode, addr, sizeof(mode));
		addr += 1;
	    }
	    r = regname[regnm(mode)];
	    amode = addrmode(mode);
	    switch (amode) {
		case LITSHORT:
		case LITUPTO31:
		case LITUPTO47:
		case LITUPTO63:
		    argval = mode;
		    break;

		case INDEX:
		    indexf = true;
		    --argno;
		    break;

		case REG:
		case REGDEF:
		case AUTODEC:
		    break;

		case AUTOINC:
		    if (r == regname[PROGCTR]) {
			switch (typelen(argtype)) {
			    case TYPB:
				argval = getdisp(addr, 1, r, amode);
				addr += 1;
				break;

			    case TYPW:
				argval = getdisp(addr, 2, r, amode);
				addr += 2;
				break;

			    case TYPL:
				argval = getdisp(addr, 4, r, amode);
				addr += 4;
				break;

			    case TYPF:
				iread(&argval, addr, sizeof(argval));
				addr += 4;
				break;

			    case TYPQ:
			    case TYPD:
			    case TYPG:
				iread(&argval, addr+4, sizeof(argval));
				addr += 8;
				break;
			}
		    }
		    break;

		case AUTOINCDEF:
		    if (r == regname[PROGCTR]) {
			argval = getdisp(addr, 4, r, amode);
			addr += 4;
		    }
		    break;

		case BYTEDISP:
		case BYTEDISPDEF:
		    argval = getdisp(addr, 1, r, amode);
		    addr += 1;
		    break;

		case WORDDISP:
		case WORDDISPDEF:
		    argval = getdisp(addr, 2, r, amode);
		    addr += 2;
		    break;

		case LONGDISP:
		case LONGDISPDEF:
		    argval = getdisp(addr, 4, r, amode);
		    addr += 4;
		    break;
	    }
	}
	if (ins == O_CALLS or ins == O_CALLG) {
	    argval += 2;
	}
	if (addrstatus == BRANCH) {
	    addr = argval;
	}
    }
    return addr;
}

/*
 * Get the displacement of an instruction that uses displacement addressing.
 */

private int getdisp(addr, nbytes, reg, mode)
Address addr;
int nbytes;
String reg;
int mode;
{
    char byte;
    short hword;
    int argval;

    switch (nbytes) {
	case 1:
	    iread(&byte, addr, sizeof(byte));
	    argval = byte;
	    break;

	case 2:
	    iread(&hword, addr, sizeof(hword));
	    argval = hword;
	    break;

	case 4:
	    iread(&argval, addr, sizeof(argval));
	    break;
    }
    if (reg == regname[PROGCTR] && mode >= BYTEDISP) {
	argval += addr + nbytes;
    }
    return argval;
}

#define BP_OP       O_BPT       /* breakpoint trap */
#define BP_ERRNO    SIGTRAP     /* signal received at a breakpoint */

/*
 * Setting a breakpoint at a location consists of saving
 * the word at the location and poking a BP_OP there.
 *
 * We save the locations and words on a list for use in unsetting.
 */

typedef struct Savelist *Savelist;

struct Savelist {
    Address location;
    Byte save;
    Byte refcount;
    Savelist link;
};

private Savelist savelist;

/*
 * Set a breakpoint at the given address.  Only save the word there
 * if it's not already a breakpoint.
 */

public setbp(addr)
Address addr;
{
    Byte w;
    Byte save;
    register Savelist newsave, s;

    for (s = savelist; s != nil; s = s->link) {
	if (s->location == addr) {
	    s->refcount++;
	    return;
	}
    }
    iread(&save, addr, sizeof(save));
    newsave = new(Savelist);
    newsave->location = addr;
    newsave->save = save;
    newsave->refcount = 1;
    newsave->link = savelist;
    savelist = newsave;
    w = BP_OP;
    iwrite(&w, addr, sizeof(w));
}

/*
 * Unset a breakpoint; unfortunately we have to search the SAVELIST
 * to find the saved value.  The assumption is that the SAVELIST will
 * usually be quite small.
 */

public unsetbp(addr)
Address addr;
{
    register Savelist s, prev;

    prev = nil;
    for (s = savelist; s != nil; s = s->link) {
	if (s->location == addr) {
	    iwrite(&s->save, addr, sizeof(s->save));
	    s->refcount--;
	    if (s->refcount == 0) {
		if (prev == nil) {
		    savelist = s->link;
		} else {
		    prev->link = s->link;
		}
		dispose(s);
	    }
	    return;
	}
	prev = s;
    }
    panic("unsetbp: couldn't find address %d", addr);
}

/*
 * Enter a procedure by creating and executing a call instruction.
 */

#define CALLSIZE 7	/* size of call instruction */

public beginproc(p, argc)
Symbol p;
Integer argc;
{
    char save[CALLSIZE];
    struct {
	VaxOpcode op;
	unsigned char numargs;
	unsigned char mode;
	char addr[sizeof(long)];	/* unaligned long */
    } call;
    long dest;

    pc = 2;
    iread(save, pc, sizeof(save));
    call.op = O_CALLS;
    call.numargs = argc;
    call.mode = 0xef;
    dest = codeloc(p) - 2 - (pc + 7);
    mov(&dest, call.addr, sizeof(call.addr));
    iwrite(&call, pc, sizeof(call));
    setreg(PROGCTR, pc);
    pstep(process, DEFSIG);
    iwrite(save, pc, sizeof(save));
    pc = reg(PROGCTR);
    if (not isbperr()) {
	printstatus();
    }
}

/*
 * Create and execute sync instruction.
 */

#define SYNCSIZE 4/* size of sync instruction */

public execsync(sync_type)
int sync_type;
{
    char save[SYNCSIZE];
    struct {
	VaxOpcode op1;
	VaxOpcode op2;
	unsigned char sync_type;
	unsigned char mode;
    } sync_inst;
	Address saved_pc = pc;
	Word saved_r0 = reg(0);

    iread(save, pc, sizeof(save));
    sync_inst.op1 = O_ESCD;             /* set sync instr two */
    sync_inst.op2 = O_MFVP;             /* byte opcode */
    sync_inst.sync_type = sync_type;    /* 1st operand - literal 4 or 5 */
    sync_inst.mode = 0x50;              /* 2nd operand = r0 as operand */
    iwrite(&sync_inst, pc, sizeof(sync_inst));
    setreg(PROGCTR, pc);                /* reset program counter */
    pstep(process, DEFSIG);             /* execute sync instruction */
    pc = saved_pc;                    /* restore saved program counter */
    iwrite(save, pc, sizeof(save));     /* restore saved code */
    setreg(PROGCTR, pc);                /* reset program counter */
    setreg(0, saved_r0);                /* reset register zero */
	printstatus();
}

/* 
 * Display vector instruction modifiers
 */
private printmod (op, modifiers)
VaxOpcode op;
unsigned modifiers;
{
  	int cnt = 0;
  
  	if (modifiers != 0) {
		printf("/");
		cnt++;
    	if (modifiers & mi) {
      		if ((op == O_VLDL) || (op == O_VLDQ) ||
	  				(op == O_VGATHL) || (op == O_VGATHQ)) {
				printf("m");
				cnt++;
      		} else {
				printf("u(v)");
				cnt += 4;
      		}
    	}
    	if (modifiers & moe) {
      		if (modifiers & mtf) {
				printf("1");
				cnt++;
      		} else {
				printf("0");
				cnt++;
    		}
    	} else {
      		if ((op == O_VVMERGE) || (op == O_VSMERGE) || (op == O_IOTA)) {
				if (modifiers & mtf) {
	  				printf("1");
					cnt++;
				} else {
	  				printf("0");
					cnt++;
    			}
    		}
  		}
	}
	return(cnt);
}


typedef int INTFUNC();
#define ERR_IGNORE ((INTFUNC *) 0)
#define ERR_CATCH  ((INTFUNC *) 1)

/*
 * test if the machine is vector capable and set the global
 * vectorcapable, return vectorcapable.
 */
public  boolean is_vector_capable()
{
	long vptotal = 0;
	
#ifndef NOVECTORS
	onsyserr(EINVAL, ERR_IGNORE);

	if(getsysinfo(GSI_VPTOTAL, (char *)&vptotal, sizeof(long), 0, NULL ) <= 0)
	{
#endif /* NOVECTORS */
		vectorcapable = false;
#ifndef NOVECTORS
	} else {
		if(vptotal > 0) {
			if(!vectorcapable) {
				fprintf(stderr, "machine is now vector capable\n");
				vectorcapable = true;
			}
		} else {
			if(vectorcapable) {
				fprintf(stderr, "machine is now NOT vector capable\n");
				vectorcapable = false;
			}
		}
	}
	onsyserr(EINVAL, ERR_CATCH);
#endif /* NOVECTORS */
	return(vectorcapable);
}
