/* @(#)process.c	4.2	Ultrix	11/9/90 */

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
 *  012	- Added support for vectors.							*
 *		  (L Miller, 18JAN90)									*
 *									*
 *	011 - Add missing signals.					*
 *	      (Jon Reeves, May 12, 1989)				*
 *									*
 *      010 - Change format of process id printf                        *
 *           (David Metsky, Feb 6, 1989)				*
 *									*
 *	009 - Changed signal handlers to void.				*
 *	      (Mark Parenti, June 9, 1988)				*
 *									*
 *      008 - Made sure the process id is only printed when running     *
 *            with xdb.  						*
 *            (David Metsky, April 14, 1988)				*
 *									*
 *	007 - Added xdb support for I/O window.				*
 *	      (James Bond (Mike Gancarz), March 15, 1988)		*
 *									*
 *	006 - Re-merged 4.3 changes to fix signal handling.		*
 *	      (Jon Reeves, July 14, 1987)				*
 *									*
 *	005 - Merged in 4.3 changes.					*
 *	      (vjh, April 29, 1986)					*
 *									*
 *	004 - Update copyright.						*
 *	      (vjh, August 23, 1985)					*
 *									*
 *	003 - Nexti at end of program did not work.  Fixed next() so  	*
 *	      that dostep() is called only once if inst_tracing is	*
 *	      true.							*
 *	      (vjh, April 25, 1985)					*
 *									*
 *	002 - Fixed bug: executing a "next" when at the last line of 	*
 *	      the program caused the message "program unexpectedly 	*
 *	      exited with 5" to appear.  This was because the code to	*
 *	      handle single stepping assumed that there were no bkpts.	*
 *	      between curpc and nextaddr.  In this case, the breakpoint *
 *	      at exit() was not reset, so the cleanup code in 		*
 *	      endprogram() was not executed.  The fix included removing *
 *	      routines contto() and xto(), and modifying stepto() to	*
 *	      always set/unset all breakpoints.				*
 *	      (vjh, April 12, 1985)					*
 *									*
 *	001 - Commented out test to reset process signo until 		*
 *	      further notice.  Fixes usignal() bug (single stepping -	*
 *	      dostep() gets into infinite loop).			*
 *	      (Victoria Holt, April 9, 1985)				*
 *									*
 ************************************************************************/

/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char sccsid[] = "@(#)process.c	2.3	ULTRIX	5/12/89";
#endif not lint

/*
 * Process management.
 *
 * This module contains the routines to manage the execution and
 * tracing of the debuggee process.
 */

#include <stdio.h>
#include "Xtty.h"
#include "defs.h"
#include "process.h"
#include "machine.h"
#include "events.h"
#include "tree.h"
#include "eval.h"
#include "operators.h"
#include "source.h"
#include "object.h"
#include "mappings.h"
#include "main.h"
#include "coredump.h"
#ifndef NOVECTORS
#include "/usr/sys/machine/vax/vectors.h"
#include <sys/acct.h>
#endif /* NOVECTORS */
#include <signal.h>
#include <errno.h>
#include <sys/param.h>
#include <sys/dir.h>
#include <sys/user.h>
#include <machine/reg.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifndef public

typedef struct Process *Process;

Process process;

#define DEFSIG -1

#include "machine.h"

#endif

#define NOTSTARTED 1
#define STOPPED 0177
#define FINISHED 0

/*
 * A cache of the instruction segment is kept to reduce the number
 * of system calls.  Might be better just to read the entire
 * code space into memory.
 */

#define CSIZE 1003       /* size of instruction cache */

typedef struct {
    Word addr;
    Word val;
} CacheWord;

/*
 * This structure holds the information we need from the user structure.
 */

struct Process {
    int pid;			/* process being traced */
    int mask;			/* process status word */
    Word reg[NREG];		/* process' registers */
    Word oreg[NREG];		/* registers when process last stopped */
    int vcr;                     /* vector count register */
    int ovcr;                    /* vector count register */
    int vlr;                     /* vector length register */
    int ovlr;                    /* vector length register */
    int vaer;                    /* vector arithmetic exception register */
    int ovaer;                   /* vector arithmetic exception register */
    Vquad vmr;                   /* vector mask register */
    Vquad ovmr;                  /* vector mask register */
    Vreg vreg[NREG];             /* process' vector registers */
    Vreg ovreg[NREG];            /* vect registers when process last stopped */
    short status;		/* either STOPPED or FINISHED */
    short signo;		/* signal that stopped process */
    short sigcode;		/* extra signal information */
    int exitval;		/* return value from exit() */
    long sigset;		/* bit array of traced signals */
    CacheWord word[CSIZE];	/* text segment cache */
    Ttyinfo ttyinfo;		/* process' terminal characteristics */
    Address sigstatus;		/* process' handler for current signal */
};

/*
 * These definitions are for the arguments to "pio".
 */

typedef enum { PREAD, PWRITE } PioOp;
typedef enum { TEXTSEG, DATASEG } PioSeg;

private struct Process pbuf;

#define MAXNCMDARGS 1000         /* maximum number of arguments to RUN */

extern int errno;
extern char *sys_siglist[];

public boolean xdb;		/* 007 - xdb specific features */
private boolean xdbwinflag = false; /* 007 - if true, then display xdb window */

private Boolean just_started;
public Boolean never_run;
private int argc;
private String argv[MAXNCMDARGS];
private String infile, outfile;

/*
 * Initialize process information.
 */

public process_init()
{
    register Integer i;
    Char buf[10];

    process = &pbuf;
	never_run = true;
    process->status = (coredump) ? STOPPED : NOTSTARTED;
    setsigtrace();
    for (i = 0; i < NREG; i++) {
	sprintf(buf, "$r%d", i);
	defregname(identname(buf, false), i);
    }
    defregname(identname("$ap", true), ARGP);
    defregname(identname("$fp", true), FRP);
    defregname(identname("$sp", true), STKP);
    defregname(identname("$pc", true), PROGCTR);

    if (vectorcapable) {
        for (i = 0; i < NREG; i++) {
	    sprintf(buf, "$v%d", i);
	    defvregname(identname(buf, false), i);
	    process->vreg[i] = new(Vreg);
	    process->ovreg[i] = new(Vreg);
		}
        defvcrname(identname("$vcr", false), VCR);
        defvcrname(identname("$vlr", false), VLR);
        defvcrname(identname("$vaer", false), VAER);
		defvmrname(identname("$vmr", false), VMR);
    }
    if (coredump) {
	coredump_readin(process->mask, process->reg, process->signo);
	pc = process->reg[PROGCTR];
	getsrcpos();
    }
    arginit();
}

/*
 * Routines to get at process information from outside this module.
 */

public Word reg(n)
Integer n;
{
    register Word w;

    if (n == NREG) {
	w = process->mask;
    } else {
	w = process->reg[n];
    }
    return w;
}

public setreg(n, w)
Integer n;
Word w;
{
    process->reg[n] = w;
}

public Address vregloc(type, i, j)
long type, i, j;
{
	Address addr;
#ifndef NOVECTORS
	switch (type) {
		case A_VCR:
			addr = (Address)&((struct vpcontext *) 0)->vpc_vcr;
			break;
		case A_VAER:
			addr = (Address)&((struct vpcontext *) 0)->vpc_vaer;
			break;
		case A_VLR:
			addr = (Address)&((struct vpcontext *) 0)->vpc_vlr;
			break;
		case A_VMRLO:
			addr = (Address)&((struct vpcontext *) 0)->vpc_vmrlo;
			break;
		case A_VMRHI:
			addr = (Address)&((struct vpcontext *) 0)->vpc_vmrhi;
			break;
		case A_VREGLO:
			addr = (Address)&((struct vpcontext *) 0)->vpc_vregs +
						(i * NVREG * sizeof(Vquad)) +
									(j * sizeof(Vquad));
			break;
		case A_VREGHI:
			addr = (Address)&((struct vpcontext *) 0)->vpc_vregs +
						(i * NVREG * sizeof(Vquad)) +
									(j * sizeof(Vquad));
			addr += sizeof(Vquad) >> 1;
			break;
		default:
			error("internal error on request to locate vector register");
			break;
	}
	return(addr);
#else  /* NOVECTORS */
	return(-1);
#endif /* NOVECTORS */
}

public Vreg vreg(n)
Integer n;
{
    if (n == VCR) {
	return((Vreg)process->vcr);
    } else if (n == VAER) {
	return((Vreg)process->vaer);
    } else if (n == VLR) {
	return((Vreg)process->vlr);
    } else if (n == VMR) {
	return((Vreg)&process->vmr);
    } else {
	return (process->vreg[n]);
    }
}

public Address vregaddr(n)
Integer n;
{
    Address addr;

    if (n == VCR) {
	addr = (Address) process->vcr;
    } else if (n == VAER) {
	addr = (Address) process->vaer;
    } else if (n == VLR) {
	addr = (Address) process->vlr;
    } else if (n == VMR) {
	addr = (Address) &process->vmr;
    } else {
	addr = (Address) process->vreg[n];
    }
    return addr;
}

public setvmrreg(n, l)
Integer n;
long l;
{
	if(n > 31)
	{
		if(l)
			process->vmr.val[0] |=  1 << (n - 32);
		else
			process->vmr.val[0] &=  ~(1 << (n - 32));
	}
	else
	{
		if(l)
			process->vmr.val[1] |=  1 << n;
		else
			process->vmr.val[1] &=  ~(1 << n);
	}
}

public setnreg(n, l)
Integer n;
long l;
{
    if (n == VCR) {
        process->vcr = l;
    } else if (n == VAER) {
        process->vaer = l;
    } else if (n == VLR) {
        process->vlr = l;
    }
}

public setvreg(addr, vquad)
Address addr;
Vquad *vquad;
{
    *(Vquad *) addr = *vquad;
}

/*
 * Begin execution.
 *
 * We set a breakpoint at the end of the code so that the
 * process data doesn't disappear after the program terminates.
 */

private Boolean remade();

public start(argv, infile, outfile)
String argv[];
String infile, outfile;
{
    String pargv[4];
    Node cond;

 

    if (coredump) {
	coredump = false;
	xdbwinflag = true;	/* 007 - causes xdb window to appear later */
	fclose(corefile);
	coredump_close();
    }
    if (remade(objname)) {
	reinit(argv, infile, outfile);
    }
    if (argv == nil) {
	argv = pargv;
	pargv[0] = objname;
	pargv[1] = nil;
    } else {
	argv[argc] = nil;
    }
    pstart(process, argv, infile, outfile);
    if (process->status == STOPPED) {
	pc = 0;
	setcurfunc(program);
	if (objsize != 0) {
	    cond = build(O_EQ, build(O_SYM, pcsym), build(O_LCON, lastaddr()));
	    event_once(cond, buildcmdlist(build(O_ENDX)));
	}
    }
}

/*
 * Check to see if the object file has changed since the symbolic
 * information last was read.
 */

private time_t modtime;

private Boolean remade(filename)
String filename;
{
    struct stat s;
    Boolean b;

    stat(filename, &s);
    b = (Boolean) (modtime != 0 and modtime < s.st_mtime);
    modtime = s.st_mtime;
    return b;
}

/*
 * Set up what signals we want to trace.
 */

private setsigtrace()
{
    register Integer i;
    register Process p;

    p = process;
    for (i = 1; i <= NSIG; i++) {
	psigtrace(p, i, true);
    }
    psigtrace(p, SIGHUP, false);
    psigtrace(p, SIGKILL, false);
    psigtrace(p, SIGALRM, false);
    psigtrace(p, SIGTSTP, false);
    psigtrace(p, SIGCONT, false);
    psigtrace(p, SIGCHLD, false);
}

/*
 * force a run with a reread of the object file
 */

public forcereread()
{
    arginit();
	modtime = 1;
}

/*
 * Initialize the argument list.
 */

public arginit()
{
    infile = nil;
    outfile = nil;
    argv[0] = objname;
    argc = 1;
}

/*
 * Add an argument to the list for the debuggee.
 */

public newarg(arg)
String arg;
{
    if (argc >= MAXNCMDARGS) {
	error("too many arguments");
    }
    argv[argc++] = arg;
}

/*
 * Set the standard input for the debuggee.
 */

public inarg(filename)
String filename;
{
    if (infile != nil) {
	error("multiple input redirects");
    }
    infile = filename;
}

/*
 * Set the standard output for the debuggee.
 * Probably should check to avoid overwriting an existing file.
 */

public outarg(filename)
String filename;
{
    if (outfile != nil) {
	error("multiple output redirect");
    }
    outfile = filename;
}

/*
 * Start debuggee executing.
 */

public run()
{
    process->status = STOPPED;
    fixbps();
    curline = 0;
    start(argv, infile, outfile);
	needruncmd = true;
    just_started = true;
	never_run = false;
    isstopped = false;
    cont(0);
}

/*
 * Continue execution wherever we left off.
 *
 * Note that this routine never returns.  Eventually bpact() will fail
 * and we'll call printstatus or step will call it.
 */

typedef void Intfunc();

private Intfunc *dbintr;
private void intr();

public cont(signo)
integer signo;
{
    integer s;

    dbintr = signal(SIGINT, intr);
    if (just_started) {
	just_started = false;
    } else {
	if (not isstopped) {
	    error("can't continue execution");
	}
	isstopped = false;
	stepover();
    }
    s = signo;
    for (;;) {
	if (single_stepping) {
	    printnews();
	} else {
	    setallbps();
	    resume(s);
	    unsetallbps();
	    s = DEFSIG;
	    if (not isbperr() or not bpact()) {
		printstatus();
	    }
	}
	stepover();
    }
    /* NOTREACHED */
}

/*
 * This routine is called if we get an interrupt while "running"
 * but actually in the debugger.  Could happen, for example, while
 * processing breakpoints.
 *
 * We basically just want to keep going; the assumption is
 * that when the process resumes it will get the interrupt,
 * which will then be handled.
 */

private void intr()
{
    signal(SIGINT, intr);
}

public void fixintr()
{
    signal(SIGINT, dbintr);
}

/*
 * Resume execution.
 */

public resume(signo)
int signo;
{
    register Process p;

    p = process;
    pcont(p, signo);
    pc = process->reg[PROGCTR];
    if (p->status != STOPPED) {
	if (p->signo != 0) {
	    error("program terminated by signal %d", p->signo);
	} else if (not runfirst) {
	    if (p->exitval == 0) {
		error("program exited");
	    } else {
		error("program exited with code %d", p->exitval);
	    }
	}
    }
}

/*
 * Continue execution up to the next source line.
 *
 * There are two ways to define the next source line depending on what
 * is desired when a procedure or function call is encountered.  Step
 * stops at the beginning of the procedure or call; next skips over it.
 */

/*
 * Stepc  or stepv is what is called when the step command is given.
 * It has to play with the "isstopped" information.
 */

public stepv()
{
    Address newfrp, oldpc;
    
	if (not isstopped) {
		error("can't continue execution");
    }
    isstopped = false;
	oldpc = pc;
    do {
	    if(dostep(false, true)) {
	    	if((newfrp = reg(FRP)) != 0) {
				if(oldpc == pc) {
					dostep(false, false);
					continue;
				}
				break;
			}
		}
		else {
    		newfrp = reg(FRP);
		}
    } while (newfrp != 0);
    isstopped = true;
}

public stepc()
{
    if (not isstopped) {
	error("can't continue execution");
    }
    isstopped = false;
    dostep(false, false);
    isstopped = true;
}

public nextv()
{
    Address oldfrp, newfrp, oldpc;

    if (not isstopped) {
		error("can't continue execution");
    }
    isstopped = false;
    oldfrp = reg(FRP);
	oldpc = pc;
    do {
	    if(dostep(true, true)) {
	    	if((newfrp = reg(FRP)) == oldfrp) {
				if(oldpc == pc) {
					dostep(false, false);
					continue;
				}
				break;
			}
		}
		else {
    		newfrp = reg(FRP);
		}
    } while (newfrp != 0);
    isstopped = true;
}

public next()
{
    Address oldfrp, newfrp;

    if (not isstopped) {
	error("can't continue execution");
    }
    isstopped = false;
    if (inst_tracing) {
        dostep(true, false);
    } else {
        oldfrp = reg(FRP);
        do {
	    dostep(true, false);
	    newfrp = reg(FRP);
        } while (newfrp < oldfrp and newfrp != 0);
    }
    isstopped = true;
}

/*
 * Continue execution until the current function returns, or,
 * if the given argument is non-nil, until execution returns to
 * somewhere within the given function.
 */

public rtnfunc (f)
Symbol f;
{
    Address addr;
    Symbol t;

    if (not isstopped) {
	error("can't continue execution");
    } else if (f != nil and not isactive(f)) {
	error("%s is not active", symname(f));
    } else {
	addr = return_addr();
	if (addr == nil) {
	    error("no place to return to");
	} else {
	    isstopped = false;
	    contto(addr);
	    if (f != nil) {
		for (;;) {
		    t = whatblock(pc);
		    addr = return_addr();
		if (t == f or addr == nil) break;
		    contto(addr);
		}
	    }
	    if (not bpact()) {
		isstopped = true;
		printstatus();
	    }
	}
    }
}

/*
 * Single-step over the current machine instruction.
 *
 * If we're single-stepping by source line we want to step to the
 * next source line.  Otherwise we're going to continue so there's
 * no reason to do all the work necessary to single-step to the next
 * source line.
 */

public stepover()
{
    Boolean b;

    if (traceexec) {
	printf("!! stepping over 0x%x\n", process->reg[PROGCTR]);
    }
    if (single_stepping) {
	dostep(false, false);
    } else {
	b = inst_tracing;
	inst_tracing = true;
	dostep(false, false);
	inst_tracing = b;
    }
    if (traceexec) {
	printf("!! stepped over to 0x%x\n", process->reg[PROGCTR]);
    }
}

/*
 * Resume execution up to the given address.  We can either ignore
 * breakpoints (stepto) or catch them (contto).
 */

public stepto(addr)
Address addr;
{
    xto(addr, false);
}

private contto (addr)
Address addr;
{
    xto(addr, true);
}

private xto (addr, catchbps)
Address addr;
boolean catchbps;
{
    Address curpc;

    if (catchbps) {
	stepover();
    }
    curpc = process->reg[PROGCTR];
    if (addr != curpc) {
	if (traceexec) {
	    printf("!! stepping from 0x%x to 0x%x\n", curpc, addr);
	}
	if (catchbps) {
	    setallbps();
	}
	setbp(addr);
	resume(DEFSIG);
	unsetbp(addr);
	if (catchbps) {
	    unsetallbps();
	}
	if (not isbperr()) {
	    printstatus();
	}
    }
}

/*
 * Print the status of the process.
 * This routine does not return.
 */

public printstatus()
{
    int status;

    if (process->status == FINISHED) {
	exit(0);
    } else {
	setcurfunc(whatblock(pc));
	getsrcpos();
	if (process->signo == SIGINT) {
	    isstopped = true;
	    printerror();
	} else if (isbperr() and isstopped) {
	    printf("stopped ");
	    printloc();
	    putchar('\n');
	    if (curline > 0) {
		printlines(curline, curline);
	    } else {
		printinst(pc, pc);
	    }
	    erecover();
	} else {
	    fixintr();
	    isstopped = true;
	    printerror();
	}
    }
}

/*
 * Print out the current location in the debuggee.
 */

public printloc()
{
    Symbol f;

    printf("in ");
    f = curfunc;
    while (isinline(f)) {
        f = container(f);
    }
    printname(stdout, f);
    putchar(' ');
    if (curline > 0 and not useInstLoc) {
	printsrcpos();
    } else {
	useInstLoc = false;
	curline = 0;
	printf("at 0x%x", pc);
    }
}

/*
 * Some functions for testing the state of the process.
 */

public Boolean notstarted(p)
Process p;
{
    return (Boolean) (p->status == NOTSTARTED);
}

public Boolean isfinished(p)
Process p;
{
    return (Boolean) (p->status == FINISHED);
}

/*
 * Predicate to test if the reason the process stopped was because
 * of a breakpoint.  If so, as a side effect clear the local copy of
 * signal handler associated with process.  We must do this so as to
 * not confuse future stepping or continuing by possibly concluding
 * the process should continue with a SIGTRAP handler.
 */

public boolean isbperr()
{
    Process p;
    boolean b;

    p = process;
    if (p->status == STOPPED and p->signo == SIGTRAP) {
	b = true;
	p->sigstatus = 0;
    } else {
	b = false;
    }
    return b;
}

/*
 * Return the signal number that stopped the process.
 */

public integer errnum (p)
Process p;
{
    return p->signo;
}

/*
 * Return the signal code associated with the signal.
 */

public integer errcode (p)
Process p;
{
    return p->sigcode;
}

/*
 * Return the termination code of the process.
 */

public integer exitcode (p)
Process p;
{
    return p->exitval;
}

/*
 * These routines are used to access the debuggee process from
 * outside this module.
 *
 * They invoke "pio" which eventually leads to a call to "ptrace".
 * The system generates an I/O error when a ptrace fails.  During reads
 * these are ignored, during writes they are reported as an error, and
 * for anything else they cause a fatal error.
 */

extern Intfunc *onsyserr();

private badaddr;
private read_err(), write_err();

/*
 * Read from the process' instruction area.
 */

public iread(buff, addr, nbytes)
char *buff;
Address addr;
int nbytes;
{
    Intfunc *f;

    f = onsyserr(EIO, read_err);
    badaddr = addr;
    if (coredump) {
	coredump_readtext(buff, addr, nbytes);
    } else {
	pio(process, PREAD, TEXTSEG, buff, addr, nbytes);
    }
    onsyserr(EIO, f);
}

/* 
 * Write to the process' instruction area, usually in order to set
 * or unset a breakpoint.
 */

public iwrite(buff, addr, nbytes)
char *buff;
Address addr;
int nbytes;
{
    Intfunc *f;

    if (coredump) {
	error("no process to write to");
    }
    f = onsyserr(EIO, write_err);
    badaddr = addr;
    pio(process, PWRITE, TEXTSEG, buff, addr, nbytes);
    onsyserr(EIO, f);
}

/*
 * Read for the process' data area.
 */

public dread(buff, addr, nbytes)
char *buff;
Address addr;
int nbytes;
{
    Intfunc *f;

    badaddr = addr;
    if (coredump) {
	f = onsyserr(EFAULT, read_err);
	coredump_readdata(buff, addr, nbytes);
	onsyserr(EFAULT, f);
    } else {
	f = onsyserr(EIO, read_err);
	pio(process, PREAD, DATASEG, buff, addr, nbytes);
	onsyserr(EIO, f);
    }
}

/*
 * Write to the process' data area.
 */

public dwrite(buff, addr, nbytes)
char *buff;
Address addr;
int nbytes;
{
    Intfunc *f;

    if (coredump) {
	error("no process to write to");
    }
    f = onsyserr(EIO, write_err);
    badaddr = addr;
    pio(process, PWRITE, DATASEG, buff, addr, nbytes);
    onsyserr(EIO, f);
}

/*
 * Trap for errors in reading or writing to a process.
 * The current approach is to "ignore" read errors and complain
 * bitterly about write errors.
 */

private read_err()
{
    /*
     * Ignore.
     */
}

private write_err()
{
    error("can't write to process (address 0x%x)", badaddr);
}

/*
 * Ptrace interface.
 */

/*
 * This magic macro enables us to look at the process' registers
 * in its user structure.
 */

#define regloc(reg)     (ctob(UPAGES) + ( sizeof(int) * (reg) ))

#define WMASK           (~(sizeof(Word) - 1))
#define cachehash(addr) ((unsigned) ((addr >> 2) % CSIZE))

#define FIRSTSIG        SIGINT
#define LASTSIG         SIGQUIT
#define ischild(pid)    ((pid) == 0)
#define traceme()       ptrace(0, 0, 0, 0)
#define setrep(n)       (1 << ((n)-1))
#define istraced(p)     (p->sigset&setrep(p->signo))

/*
 * Ptrace options (specified in first argument).
 */

#define UREAD   3       /* read from process's user structure */
#define UWRITE  6       /* write to process's user structure */
#define IREAD   1       /* read from process's instruction space */
#define IWRITE  4       /* write to process's instruction space */
#define DREAD   2       /* read from process's data space */
#define DWRITE  5       /* write to process's data space */
#define CONT    7       /* continue stopped process */
#define SSTEP   9       /* continue for approximately one instruction */
#define PKILL   8       /* terminate the process */
#define VREAD   10      /* read a vector register from process's user area */
#define VWRITE  11      /* writes a vector register to process's user area */

/* this routine is used to check if the process being debugged has a vector
 * context area.
 */

boolean vector_context()
{
#ifndef NOVECTORS
	int acflag = ~AVP;

	acflag = ptrace(UREAD, process->pid, &((struct user *) 0)->u_acflag, 0);
	if((acflag) & AVP)
		return(true);
	else
#endif /* NOVECTORS */
		return(false);
}

/*
 * Start up a new process by forking and exec-ing the
 * given argument list, returning when the process is loaded
 * and ready to execute.  The PROCESS information (pointed to
 * by the first argument) is appropriately filled.
 *
 * If the given PROCESS structure is associated with an already running
 * process, we terminate it.
 */

/* VARARGS2 */
private pstart(p, argv, infile, outfile)
Process p;
String argv[];
String infile;
String outfile;
{
    int status;
    TTYWindow *tw;
    struct sgttyb s; 
    struct ltchars l;
    int localMode;

    if (p->pid != 0) {
	pterm(p);
	cacheflush(p);
    }
    fflush(stdout);
    psigtrace(p, SIGTRAP, true);
    p->pid = vfork();
    if (p->pid == -1) {
	panic("can't fork");
    }
    if (ischild(p->pid)) {
	nocatcherrs();
	traceme();

	if (xdb) {			/* 007 - xdb support */
	    if (xdbwinflag == false) {
		xdbwinflag = true;
	    } else {
		tw = CreateTTYWindow();
		ioctl(tw->file,(int)TIOCGETP,(char *) &s);
		s.sg_flags |= ANYP | CRMOD | ECHO;
		s.sg_flags &= ~(RAW | CBREAK);
		s.sg_erase = CERASE;
		s.sg_kill = CKILL;
		ioctl(tw->file,(int)TIOCSETP,(char *) &s);
		ioctl(tw->file, (int) TIOCGLTC, (char *) &l);
		l.t_rprntc = CRPRNT;
		l.t_werasc = CWERASE;
		ioctl(tw->file, (int) TIOCSLTC, (char *) &l);
		localMode = LCRTBS | LCRTERA | LCRTKIL | LCTLECH;
		ioctl(tw->file, (int) TIOCLBIS, (char *) &localMode);
		dup2(tw->file,0);
		dup2(tw->file,1);
		dup2(tw->file,2); /* added by bjg */
	    }
	}

	if (infile != nil) {
	    infrom(infile);
	}
	if (outfile != nil) {
	    outto(outfile);
	}
	execv(argv[0], argv);
	_exit(1);
    }

                                           /* 010 - change format */
    if (xdb) printf("%d iopid\n",p->pid);  /* 008 - xdb support */

    pwait(p->pid, &status);
    getinfo(p, status);
    if (p->status != STOPPED) {
	beginerrmsg();
	fprintf(stderr, "warning: cannot execute %s\n", argv[0]);
    } else {
	ptraced(p->pid);
    }
}

/*
 * Terminate a ptrace'd process.
 */

public pterm (p)
Process p;
{
    integer status;

    if (p != nil and p->pid != 0) {
	ptrace(PKILL, p->pid, 0, 0);
	pwait(p->pid, &status);
	unptraced(p->pid);
    }
}

/*
 * Continue a stopped process.  The first argument points to a Process
 * structure.  Before the process is restarted it's user area is modified
 * according to the values in the structure.  When this routine finishes,
 * the structure has the new values from the process's user area.
 *
 * Pcont terminates when the process stops with a signal pending that
 * is being traced (via psigtrace), or when the process terminates.
 */

private pcont(p, signo)
Process p;
int signo;
{
    int s, status;

    if (p->pid == 0) {
	error("program is not active");
    }
    s = signo;
    do {
	setinfo(p, s);
	if (traceexec) {
	    printf("!! pcont from 0x%x with signal %d (%d)\n",
		p->reg[PROGCTR], s, p->signo);
	    fflush(stdout);
	}
	sigs_off();
	if (ptrace(CONT, p->pid, p->reg[PROGCTR], p->signo) < 0) {
		p->pid = 0;
    	p->status = FINISHED;
	    error("error %d trying to continue process with %s", 
											errno, sys_siglist[p->signo]);
	}
	pwait(p->pid, &status);
	sigs_on();
	getinfo(p, status);
	if (p->status == STOPPED and traceexec and not istraced(p)) {
	    printf("!! ignored signal %d at 0x%x\n",
		p->signo, p->reg[PROGCTR]);
	    fflush(stdout);
	}
	s = p->signo;
    } while (p->status == STOPPED and not istraced(p));
    if (traceexec) {
	printf("!! pcont to 0x%x on signal %d\n", p->reg[PROGCTR], p->signo);
	fflush(stdout);
    }
}

/*
 * Single step as best ptrace can.
 */

public pstep(p, signo)
Process p;
integer signo;
{
    int s, status;

    s = signo;
    do {
	setinfo(p, s);
	if (traceexec) {
	    printf("!! pstep from 0x%x with signal %d (%d)\n",
		p->reg[PROGCTR], s, p->signo);
	    fflush(stdout);
	}
	sigs_off();
	if (ptrace(SSTEP, p->pid, p->reg[PROGCTR], p->signo) < 0) {
	    panic("error %d trying to step process", errno);
	}
	pwait(p->pid, &status);
	sigs_on();
	getinfo(p, status);
	if (p->status == STOPPED and traceexec and not istraced(p)) {
	    printf("!! pstep ignored signal %d at 0x%x\n",
		p->signo, p->reg[PROGCTR]);
	    fflush(stdout);
	}
	s = p->signo;
    } while (p->status == STOPPED and not istraced(p));
    if (traceexec) {
	printf("!! pstep to 0x%x on signal %d\n",
	    p->reg[PROGCTR], p->signo);
	fflush(stdout);
    }
    if (p->status != STOPPED) {
	if (p->exitval == 0) {
	    error("program exited\n");
	} else {
	    error("program exited with code %d\n", p->exitval);
	}
    }
}

/*
 * Return from execution when the given signal is pending.
 */

public psigtrace(p, sig, sw)
Process p;
int sig;
Boolean sw;
{
    if (sw) {
	p->sigset |= setrep(sig);
    } else {
	p->sigset &= ~setrep(sig);
    }
}

/*
 * Don't catch any signals.
 * Particularly useful when letting a process finish uninhibited.
 */

public unsetsigtraces(p)
Process p;
{
    p->sigset = 0;
}

/*
 * Turn off attention to signals not being caught.
 */

private Intfunc *sigfunc[NSIG];

private sigs_off()
{
    register int i;

    for (i = FIRSTSIG; i < LASTSIG; i++) {
	if (i != SIGKILL) {
	    sigfunc[i] = signal(i, SIG_IGN);
	}
    }
}

/*
 * Turn back on attention to signals.
 */

private sigs_on()
{
    register int i;

    for (i = FIRSTSIG; i < LASTSIG; i++) {
	if (i != SIGKILL) {
	    signal(i, sigfunc[i]);
	}
    }
}

/*
 * Get process information from user area.
 */

private int rloc[] ={
    R0, R1, R2, R3, R4, R5, R6, R7, R8, R9, R10, R11, AP, FP, SP, PC
};

private getinfo(p, status)
register Process p;
register int status;
{
    register int i, j;
    Address addr;
	int value;

    p->signo = (status&0177);
    p->exitval = ((status >> 8)&0377);
    if (p->signo != STOPPED) {
	p->status = FINISHED;
	p->pid = 0;
	p->reg[PROGCTR] = 0;
    } else {
	p->status = p->signo;
	p->signo = p->exitval;
	p->sigcode = ptrace(UREAD, p->pid, &((struct user *) 0)->u_code, 0);
	p->exitval = 0;
	p->mask = ptrace(UREAD, p->pid, regloc(PS), 0);
	for (i = 0; i < NREG; i++) {
	    p->reg[i] = ptrace(UREAD, p->pid, regloc(rloc[i]), 0);
	    p->oreg[i] = p->reg[i];
	}
	if (vectorcapable && vector_context()) {
		if(((single_stepping or inst_tracing) and found_a_vec_inst) or
									(!single_stepping and !inst_tracing)) {
	    	for (i = 0; i < NREG; i++) {
	    		for (j = 0; j < NVREG; j++) {
					value = ptrace(VREAD, p->pid, vregloc(A_VREGLO, i, j), 0);
					p->ovreg[i]->reg[j].val[0] =
									p->vreg[i]->reg[j].val[0] = value;
					value = ptrace(VREAD, p->pid, vregloc(A_VREGHI, i, j), 0);
					p->ovreg[i]->reg[j].val[1] =
									p->vreg[i]->reg[j].val[1] = value;
	    		}
	    	}
	    	p->ovcr = p->vcr = ptrace(VREAD, p->pid, vregloc(A_VCR, 0, 0), 0);
	    	p->ovlr = p->vlr = ptrace(VREAD, p->pid, vregloc(A_VLR, 0, 0), 0);
	    	p->ovaer = p->vaer =
					ptrace(VREAD, p->pid, vregloc(A_VAER, 0, 0), 0);
	    	p->ovmr.val[0] = p->vmr.val[0] =
					ptrace(VREAD, p->pid, vregloc(A_VMRLO, 0, 0), 0);
	    	p->ovmr.val[1] = p->vmr.val[1] =
	    			ptrace(VREAD, p->pid, vregloc(A_VMRHI, 0, 0), 0);
		}
	}
	savetty(stdout, &(p->ttyinfo));
	addr = (Address) &(((struct user *) 0)->u_signal[p->signo]);
	p->sigstatus = (Address) ptrace(UREAD, p->pid, addr, 0);
    }
}

/*
 * Set process's user area information from given process structure.
 */

private setinfo(p, signo)
register Process p;
int signo;
{
    register int i, j;
    register int r, s;

    if (signo == DEFSIG) {
		if (istraced(p) and (p->sigstatus == 0 or p->sigstatus == 1)) {
		    p->signo = 0;
		}
    } else {
		p->signo = signo;
    }
    for (i = 0; i < NREG; i++) {
		if ((r = p->reg[i]) != p->oreg[i]) {
	    	ptrace(UWRITE, p->pid, regloc(rloc[i]), r);
			p->oreg[i] = r;
		}
    }
	if (vectorcapable && vector_context()) {
		if(((single_stepping or inst_tracing) and found_a_vec_inst) or
									(!single_stepping and !inst_tracing)) {
	    	for (i = 0; i < NREG; i++) {
	    		for (j = 0; j < NVREG; j++) {
	    			r = p->vreg[i]->reg[j].val[0];
	    			s = p->vreg[i]->reg[j].val[1];
	    			if (r != p->ovreg[i]->reg[j].val[0]) {
						ptrace(VWRITE, p->pid, vregloc(A_VREGLO, i, j), r);
						p->ovreg[i]->reg[j].val[0] = r;
					}
	    			if (s != p->ovreg[i]->reg[j].val[1]) {
						ptrace(VWRITE, p->pid, vregloc(A_VREGHI, i, j), s);
						p->ovreg[i]->reg[j].val[1] = s;
					}
	    		} 
			}
			if(p->ovcr != p->vcr) {
	    		ptrace(VWRITE, p->pid, vregloc(A_VCR, 0, 0), p->vcr);
				p->ovcr = p->vcr;
			}
			if(p->ovaer != p->vaer) {
	    		ptrace(VWRITE, p->pid, vregloc(A_VAER, 0, 0), p->vaer);
				p->ovaer = p->vaer;
			}
			if(p->ovlr != p->vlr) {
	    		ptrace(VWRITE, p->pid, vregloc(A_VLR, 0, 0), p->vlr);
				p->ovlr = p->vlr;
			}
			if(p->ovmr.val[0] != p->vmr.val[0]) {
	    		ptrace(VWRITE, p->pid, vregloc(A_VMRLO, 0, 0), p->vmr.val[0]);
				p->ovmr.val[0] = p->vmr.val[0];
			}
			if(p->ovmr.val[1] != p->vmr.val[1]) {
	    		ptrace(VWRITE, p->pid, vregloc(A_VMRHI, 0, 0), p->vmr.val[1]);
				p->ovmr.val[1] = p->vmr.val[1];
			}
    	}
	}
    restoretty(stdout, &(p->ttyinfo));
}

/*
 * Return the address associated with the current signal.
 * (Plus two since the address points to the beginning of a procedure).
 */

public Address usignal (p)
Process p;
{
    Address r;

    r = p->sigstatus;
    if (r != 0 and r != 1) {
	r += 2;
    }
    return r;
}

/*
 * Structure for reading and writing by words, but dealing with bytes.
 */

typedef union {
    Word pword;
    Byte pbyte[sizeof(Word)];
} Pword;

/*
 * Read (write) from (to) the process' address space.
 * We must deal with ptrace's inability to look anywhere other
 * than at a word boundary.
 */

private Word fetch();
private store();

private pio(p, op, seg, buff, addr, nbytes)
Process p;
PioOp op;
PioSeg seg;
char *buff;
Address addr;
int nbytes;
{
    register int i;
    register Address newaddr;
    register char *cp;
    char *bufend;
    Pword w;
    Address wordaddr;
    int byteoff;

    if (p->status != STOPPED) {
	error("program is not active");
    }
    cp = buff;
    newaddr = addr;
    wordaddr = (newaddr&WMASK);
    if (wordaddr != newaddr) {
	w.pword = fetch(p, seg, wordaddr);
	for (i = newaddr - wordaddr; i < sizeof(Word) and nbytes > 0; i++) {
	    if (op == PREAD) {
		*cp++ = w.pbyte[i];
	    } else {
		w.pbyte[i] = *cp++;
	    }
	    nbytes--;
	}
	if (op == PWRITE) {
	    store(p, seg, wordaddr, w.pword);
	}
	newaddr = wordaddr + sizeof(Word);
    }
    byteoff = (nbytes&(~WMASK));
    nbytes -= byteoff;
    bufend = cp + nbytes;
    while (cp < bufend) {
	if (op == PREAD) {
	    *((Word *) cp) = fetch(p, seg, newaddr);
	} else {
	    store(p, seg, newaddr, *((Word *) cp));
	}
	cp += sizeof(Word);
	newaddr += sizeof(Word);
    }
    if (byteoff > 0) {
	w.pword = fetch(p, seg, newaddr);
	for (i = 0; i < byteoff; i++) {
	    if (op == PREAD) {
		*cp++ = w.pbyte[i];
	    } else {
		w.pbyte[i] = *cp++;
	    }
	}
	if (op == PWRITE) {
	    store(p, seg, newaddr, w.pword);
	}
    }
}

/*
 * Get a word from a process at the given address.
 * The address is assumed to be on a word boundary.
 *
 * A simple cache scheme is used to avoid redundant ptrace calls
 * to the instruction space since it is assumed to be pure.
 *
 * It is necessary to use a write-through scheme so that
 * breakpoints right next to each other don't interfere.
 */

private Integer nfetchs, nreads, nwrites;

private Word fetch(p, seg, addr)
Process p;
PioSeg seg;
register int addr;
{
    register CacheWord *wp;
    register Word w;

    switch (seg) {
	case TEXTSEG:
	    ++nfetchs;
	    wp = &p->word[cachehash(addr)];
	    if (addr == 0 or wp->addr != addr) {
		++nreads;
		w = ptrace(IREAD, p->pid, addr, 0);
		wp->addr = addr;
		wp->val = w;
	    } else {
		w = wp->val;
	    }
	    break;

	case DATASEG:
	    w = ptrace(DREAD, p->pid, addr, 0);
	    break;

	default:
	    panic("fetch: bad seg %d", seg);
	    /* NOTREACHED */
    }
    return w;
}

/*
 * Put a word into the process' address space at the given address.
 * The address is assumed to be on a word boundary.
 */

private store(p, seg, addr, data)
Process p;
PioSeg seg;
int addr;
Word data;
{
    register CacheWord *wp;

    switch (seg) {
	case TEXTSEG:
	    ++nwrites;
	    wp = &p->word[cachehash(addr)];
	    wp->addr = addr;
	    wp->val = data;
	    ptrace(IWRITE, p->pid, addr, data);
	    break;

	case DATASEG:
	    ptrace(DWRITE, p->pid, addr, data);
	    break;

	default:
	    panic("store: bad seg %d", seg);
	    /* NOTREACHED */
    }
}

/*
 * Flush the instruction cache associated with a process.
 */

private cacheflush (p)
Process p;
{
    bzero(p->word, sizeof(p->word));
}

public printptraceinfo()
{
    printf("%d fetchs, %d reads, %d writes\n", nfetchs, nreads, nwrites);
}

/*
 * Redirect input.
 * Assuming this is called from a child, we should be careful to avoid
 * (possibly) shared standard I/O buffers.
 */

private infrom (filename)
String filename;
{
    Fileid in;

    in = open(filename, 0);
    if (in == -1) {
	write(2, "can't read ", 11);
	write(2, filename, strlen(filename));
	write(2, "\n", 1);
	_exit(1);
    }
    fswap(0, in);
}

/*
 * Redirect standard output.
 * Same assumptions as for "infrom" above.
 */

private outto (filename)
String filename;
{
    Fileid out;

    out = creat(filename, 0666);
    if (out == -1) {
	write(2, "can't write ", 12);
	write(2, filename, strlen(filename));
	write(2, "\n", 1);
	_exit(1);
    }
    fswap(1, out);
}

/*
 * Swap file numbers, useful for redirecting standard input or output.
 */

private fswap(oldfd, newfd)
Fileid oldfd;
Fileid newfd;
{
    if (oldfd != newfd) {
	close(oldfd);
	dup(newfd);
	close(newfd);
    }
}

/*
 * The signal is SIGTRAP, caught by the debugger, so reset the
 * sigstatus field to 0.
 */

public resetsig(p)
Process p;
{
    p->sigstatus = 0;
}

/*
 * Signal name manipulation.
 */

private String signames[NSIG] = {
    0,
    "HUP", "INT", "QUIT", "ILL", "TRAP",
    "IOT", "EMT", "FPE", "KILL", "BUS",
    "SEGV", "SYS", "PIPE", "ALRM", "TERM",
    "URG", "STOP", "TSTP", "CONT", "CHLD",
    "TTIN", "TTOU", "IO", "XCPU", "XFSZ",
    "VTALRM", "PROF", "WINCH", "LOST", "USR1",
    "USR2"
};

/*
 * Get the signal number associated with a given name.
 * The name is first translated to upper case if necessary.
 */

public integer siglookup (s)
String s;
{
    register char *p, *q;
    char buf[100];
    integer i;

    p = s;
    q = buf;
    while (*p != '\0') {
	if (*p >= 'a' and *p <= 'z') {
	    *q = (*p - 'a') + 'A';
	} else {
	    *q = *p;
	}
	++p;
	++q;
    }
    *q = '\0';
    p = buf;
    if (buf[0] == 'S' and buf[1] == 'I' and buf[2] == 'G') {
	p += 3;
    }
    i = 1;
    for (;;) {
	if (i >= sizeof(signames) div sizeof(signames[0])) {
	    error("signal \"%s\" unknown", s);
	    i = 0;
	    break;
	}
	if (signames[i] != nil and streq(signames[i], p)) {
	    break;
	}
	++i;
    }
    return i;
}

/*
 * Print all signals being ignored by the debugger.
 * These signals are auotmatically
 * passed on to the debugged process.
 */

public printsigsignored (p)
Process p;
{
    printsigs(~p->sigset);
}

/*
 * Print all signals being intercepted by
 * the debugger for the specified process.
 */

public printsigscaught(p)
Process p;
{
    printsigs(p->sigset);
}

private printsigs (set)
integer set;
{
    integer s;
    char separator[2];

    separator[0] = '\0';
    for (s = 1; s < sizeof(signames) div sizeof(signames[0]); s++) {
	if (set & setrep(s)) {
	    if (signames[s] != nil) {
		printf("%s%s", separator, signames[s]);
		separator[0] = ' ';
		separator[1] = '\0';
	    }
	}
    }
    if (separator[0] == ' ') {
	putchar('\n');
    }
}

/* get the vector registers from the coredump file and put them into
 * the process structure.
 */

put_vregs(f)
File f;
{
	int i, j;
	Word value;

	for (i = 0; i < NREG; i++) {
	    for (j = 0; j < NVREG; j++) {
			value = readvregfromfile(corefile, vregloc(A_VREGLO, i, j));
			process->ovreg[i]->reg[j].val[0] =
						process->vreg[i]->reg[j].val[0] = value;
			value = readvregfromfile(corefile, vregloc(A_VREGHI, i, j));
			process->ovreg[i]->reg[j].val[1] =
						process->vreg[i]->reg[j].val[1] = value;
			if(!coredump)
				return;
	    }
	}
	process->ovcr = process->vcr =
				readvregfromfile(corefile, vregloc(A_VCR, 0, 0));
	process->ovaer = process->vaer =
				readvregfromfile(corefile, vregloc(A_VAER, 0, 0));
	process->ovlr = process->vlr =
				readvregfromfile(corefile, vregloc(A_VLR, 0, 0));
	process->ovmr.val[0] = process->vmr.val[0] =
				readvregfromfile(corefile, vregloc(A_VMRLO, 0, 0));
	process->ovmr.val[1] = process->vmr.val[1] =
	    		readvregfromfile(corefile, vregloc(A_VMRHI, 0, 0));
}

