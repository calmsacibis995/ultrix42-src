#ifndef lint
static char *sccsid = "@(#)machine.c	4.2	(ULTRIX)	7/17/90";
#endif

/************************************************************************
 *									*
 *			Copyright (c) 1988 by				*
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

#include "../crash.h"
#include "../defs.h"
#include "machine.h"
#include "ops.h"

#ifndef public
typedef unsigned int Address;
typedef unsigned char Byte;
typedef unsigned int Word;

#define NREG 16

#define ARGP 12
#define FRP 13
#define STKP 14
#define PROGCTR 15

#define BITSPERBYTE 8
#define BITSPERWORD (BITSPERBYTE * sizeof(Word))

#define nargspassed(frame) argn(0, frame)
#define mkuchar(i)  ((i) & 0x000000ff)
#define mkushort(i) ((i) & 0x0000ffff)
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
 * Hacked version of adb's VAX instruction decoder.
 */

private Address printop(addr)
Address addr;
{
    Optab op;
    VaxOpcode ins;
    unsigned char mode;
    int argtype, amode, argno, argval;
    String reg;
    Boolean indexf;
    short offset;
    struct Symbol *sp, *search();

    argval = 0;
    indexf = false;
    if((sp = search(addr)) == 0)
	    printf("%08x  ", addr);
    else {
	    int off;
	    printf("%s", sp->s_name);
	    if(off = addr - sp->s_value)
		printf("+0x%-4x ", off);
	    else
		printf("         ");
    }
    iread((char *)&ins, addr, sizeof(ins));
    addr += 1;
    op = optab[ins];
    printf("%s", op.iname);
    for (argno = 0; argno < op.numargs; argno++) {
	if (indexf == true) {
	    indexf = false;
	} else if (argno == 0) {
	    printf("\t");
	} else {
	    printf(",");
	}
	argtype = op.argtype[argno];
	if (is_branch_disp(argtype)) {
	    mode = 0xAF + (typelen(argtype) << 5);
	} else {
	    iread((char *)&mode, addr, sizeof(mode));
	    addr += 1;
	}
	reg = regname[regnm(mode)];
	amode = addrmode(mode);
	switch (amode) {
	    case LITSHORT:
	    case LITUPTO31:
	    case LITUPTO47:
	    case LITUPTO63:
		if (typelen(argtype) == TYPF || typelen(argtype) ==TYPD)
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
			    iread((char *)&argval, addr, sizeof(argval));
			    printf("%06x", argval);
			    addr += 4;
			    break;

			case TYPQ:
			case TYPD:
			    iread((char *)&argval, addr, sizeof(argval));
			    printf("%06x", argval);
			    iread((char *)&argval, addr+4, sizeof(argval));
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
    if (ins == O_CASEB || ins == O_CASEW || ins == O_CASEL) {
	for (argno = 0; argno <= argval; argno++) {
	    iread((char *)&offset, addr, sizeof(offset));
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
    int argval = 0;
    struct Symbol *sp, *nmsrch();
    int diff;

    switch (nbytes) {
	case 1:
	    iread((char *)&byte, addr, sizeof(byte));
	    argval = byte;
	    break;

	case 2:
	    iread((char *)&hword, addr, sizeof(hword));
	    argval = hword;
	    break;

	case 4:
	    iread((char *)&argval, addr, sizeof(argval));
	    break;
    }
    if (reg == regname[PROGCTR] && mode >= BYTEDISP) {
	argval += addr + nbytes;
    }
    if (reg == regname[PROGCTR]) {
	/*there is something wrong with the addressing here.... */
	if((sp = search((unsigned)argval)) == 0)	
			printf("%x", argval);
		else {
			printf("%s", sp->s_name);
			if((diff = argval - sp->s_value) > 0)
				printf("+%d", diff);
		}
    } else {
	printf("%d(%s)", argval, reg);
    }
    return argval;
}

/*
 * Print the contents of the addresses within the given range
 * according to the given format.
 */


public iread(buff, addr, nbytes)
char *buff;
Address addr;
int nbytes;
{
	readmem(buff, (int)addr, nbytes);
}


dis(arg, arg1)
	register char *arg, *arg1;
{
	register unsigned int addr, count;
	unsigned scan_vaddr();
	struct Symbol *sp, *nmsrch();
	int offset = 0;
	char *offp;
	
	if(isdigit(arg[0])) {
		addr = scan_vaddr(arg);
	} else {
		if(offp = rindex(arg, '+')) {
			*offp = '\0';
			if(isdigit(*++offp)) {
				sscanf(offp, "%x", &offset);
			}
		}
		if((sp = nmsrch(arg)) == NULL) {
			printf("symbol '%s' not found\n", arg);
			return;
		} else 
			addr = sp->s_value + offset;
	}
	
	count = atoi(arg1);
	printinst(addr, count);	
}
