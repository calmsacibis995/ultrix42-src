/*@(#)events.c	4.2	Ultrix	11/9/90*/

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
 *  005	- Added support for vectors.							*
 *		  (L Miller, 18JAN90)									*
 *									*
 *	004 - Merged in 4.3 changes.					*
 *	      (vjh, April 29, 1986)					*
 *									*
 *	003 - Fixed *many* bugs/problems in trace (search for 003).	*
 *	      (vjh, July 23, 1985)					*
 *									*
 *	002 - Updated all calls to findlanguage() to call with		*
 *	      LanguageName constant, rather than with a filename	*
 *	      suffix.							*
 *	      (vjh, June 22, 1985)					*
 *									*
 *	001 - Added routine delallevents() for use with the		*
 *	      "delete *" command.					*
 *	      (Victoria Holt, April 25, 1985)				*
 *									*
 ************************************************************************/

/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char sccsid[] = "@(#)events.c	5.1 (Berkeley) 5/31/85";
#endif not lint

static char rcsid[] = "$Header: events.c,v 1.5 84/12/26 10:39:26 linton Exp $";

/*
 * Event/breakpoint managment.
 */

#include "defs.h"
#include "events.h"
#include "languages.h"
#include "main.h"
#include "symbols.h"
#include "tree.h"
#include "eval.h"
#include "source.h"
#include "mappings.h"
#include "runtime.h"
#include "process.h"
#include "machine.h"
#include "lists.h"

#ifndef public
typedef struct Event *Event;
typedef struct Breakpoint *Breakpoint;

Boolean inst_tracing;
Boolean single_stepping;
Boolean isstopped;

#include "symbols.h"

Symbol linesym;
Symbol procsym;
Symbol pcsym;
Symbol retaddrsym;

#define addevent(cond, cmdlist) event_alloc(false, cond, cmdlist)
#define event_once(cond, cmdlist) event_alloc(true, cond, cmdlist)

#endif

struct Event {
    unsigned int id;
    Boolean temporary;
    Node condition;
    Cmdlist actions;
};

struct Breakpoint {
    Event event;
    Address bpaddr;	
    Lineno bpline;
    Cmdlist actions;
    Boolean temporary;
};

typedef List Eventlist;
typedef List Bplist;

#define eventlist_append(event, el) list_append(list_item(event), nil, el)
#define bplist_append(bp, bl) list_append(list_item(bp), nil, bl)

private Eventlist eventlist;		/* list of active events */
private Bplist bplist;			/* list of active breakpoints */
private Event curevent;			/* most recently created event */
private integer eventid;		/* id number of current event */
private integer trid;			/* id number of current trace */

typedef struct Trcmd {
    Integer trid;
    Event event;
    Cmdlist cmdlist;
} *Trcmd;

private List eachline;		/* commands to execute after each line */
private List eachinst;		/* commands to execute after each instruction */

private Breakpoint bp_alloc();

/*
 * Initialize breakpoint information.
 */

private Symbol builtinsym(str, class, type)
String str;
Symclass class;
Symbol type;
{
    Symbol s;

    s = insert(identname(str, true));
    s->language = findlanguage(ASSEMBLER);
    s->class = class;
    s->type = type;
    return s;
}

public bpinit(flag)
Boolean flag;
{
    linesym = builtinsym("$line", VAR, t_int);
    procsym = builtinsym("$proc", PROC, nil);
    pcsym = lookup(identname("$pc", true));
    if (pcsym == nil) {
	panic("can't find $pc");
    }
    retaddrsym = builtinsym("$retaddr", VAR, t_int);
	if(!flag) {
    	eventlist = list_alloc();
    	bplist = list_alloc();
    	eachline = list_alloc();
    	eachinst = list_alloc();
	}
}

/* Allocates a new event.  Adds the event to the eventlist.  Traps the
 * event (in translate()); when this trap is encountered, the commands
 * in cmdlist will be executed.
 *
 * "curevent" must be assigned the new event before calling translate()
 * in case translate() calls evalcmdlist() with the new event.
 * This is part of a fix for the "missing trid" panic.  (003 - vjh)
 */

public Event event_alloc(istmp, econd, cmdlist)
boolean istmp;
Node econd;
Cmdlist cmdlist;
{
    register Event e;

    e = new(Event);
    ++eventid;
    e->id = eventid;
    e->temporary = istmp;
    e->condition = econd;
    e->actions = cmdlist;
    eventlist_append(e, eventlist);
    curevent = e;
    translate(e);
    return e;
}

/* Routine to indiscriminately remove all break/tracepoints.
 * Since this routine is invoked by the user (with delete *), 
 * it only removes user defined eventpoints (ie. leaves temporaries).
 * Used to remove all of them, which would cause "program unexpecdly
 * exited" message (breakpoint on _exit() was deleted; see resume() in
 * process.c).  (003 - vjh)
 */

public boolean delallevents()
{
    Event e;
    Breakpoint bp;
    Trcmd t;

    foreach (Event, e, eventlist)
        if (not e->temporary) {
	    foreach (Breakpoint, bp, bplist)
	        if (bp->event == e) {
		    if (tracebpts) {
		        printf("deleting breakpoint at 0x%x\n", bp->bpaddr);
		        fflush(stdout);
		    }
		    list_delete(list_curitem(bplist), bplist);
	        }
	    endfor
	    list_delete(list_curitem(eventlist), eventlist);
	}
    endfor
    foreach (Trcmd, t, eachline)
	printrmtr(t);
	list_delete(list_curitem(eachline), eachline);
    endfor
    foreach (Trcmd, t, eachinst)
        printrmtr(t);
	list_delete(list_curitem(eachinst), eachinst);
    endfor
    if (list_size(eachinst) == 0) {
	inst_tracing = false;
	if (list_size(eachline) == 0) {
	    single_stepping = false;
	}
    }
}

/*
 * Delete the event with the given id.
 * Returns whether it's successful or not.
 */

public boolean delevent(id)
unsigned int id;
{
    Event e;
    Breakpoint bp;
    Trcmd t;
    boolean found;

    found = false;
    foreach (Event, e, eventlist)
	if (e->id == id) {
	    found = true;
	    foreach (Breakpoint, bp, bplist)
		if (bp->event == e) {
		    if (tracebpts) {
			printf("deleting breakpoint at 0x%x\n", bp->bpaddr);
			fflush(stdout);
		    }
		    list_delete(list_curitem(bplist), bplist);
		}
	    endfor
	    list_delete(list_curitem(eventlist), eventlist);
	    break;
	}
    endfor
    foreach (Trcmd, t, eachline)
	if (t->event->id == id) {
	    found = true;
	    printrmtr(t);
	    list_delete(list_curitem(eachline), eachline);
	}
    endfor
    foreach (Trcmd, t, eachinst)
	if (t->event->id == id) {
	    found = true;
	    printrmtr(t);
	    list_delete(list_curitem(eachinst), eachinst);
	}
    endfor
    if (list_size(eachinst) == 0) {
	inst_tracing = false;
	if (list_size(eachline) == 0) {
	    single_stepping = false;
	}
    }
    return found;
}

/* Translate an event into the appropriate breakpoints and actions.
 * While we're at it, turn on the breakpoints if the condition is true.
 * Added a check for presence of a coredump, so that condition doesn't
 * appear to be true when it really isn't (ie. reading corefile sets
 * pc to be routine in which the program died; could be Symbol s).
 * (003 - vjh)
 */

private translate(e)
Event e;
{
    Breakpoint bp;
    Symbol s;
    Node place;
    Lineno line;
    Address addr;

    checkref(e->condition);
    switch (e->condition->op) {
	case O_EQ:
	    if (e->condition->value.arg[0]->op == O_SYM) {
			s = e->condition->value.arg[0]->value.sym;
			place = e->condition->value.arg[1];
			if (s == linesym) {
		    	if (place->op == O_QLINE) {
					line = place->value.arg[1]->value.lcon;
					addr = objaddr(line, place->value.arg[0]->value.scon);
		    	} else {
					eval(place);
					line = pop(long);
					addr = objaddr(line, cursource);
		    	}
		    	if (addr == NOADDR || addr == ADDRNOEXEC) {
					if (not delevent(e->id)) {
			    		printf("!! dbx.translate: can't undo event %d?\n",
																		e->id);
					}
					beginerrmsg();
					if (addr == NOADDR)
		    			fprintf(stderr, "beyond end of file at line ");
					else
						fprintf(stderr, "no executable code found at line ");
					prtree(stderr, place);
					enderrmsg();
		    	}
		    	bp = bp_alloc(e, addr, line, e->actions);
			} else if (s == procsym) {
		    	eval(place);
		    	s = pop(Symbol);
		    	bp = bp_alloc(e, codeloc(s), 0, e->actions);
		    	if ((not coredump) and isactive(s)  /* (003 - vjh) */
		    								and pc != codeloc(program)) {
					evalcmdlist(bp->actions);
		    	}
			} else if (s == pcsym) {
		    	eval(place);
		    	bp = bp_alloc(e, pop(Address), 0, e->actions);
			} else {
		    	condbp(e);
			}
	    } else {
			condbp(e);
	    }
	    break;

	/*
	 * These should be handled specially.
	 * But for now I'm ignoring the problem.
	 */
	case O_AND:
	case O_OR:
	default:
	    condbp(e);
	    break;
    }
}

/*
 * Create a breakpoint for a condition that cannot be pinpointed
 * to happening at a particular address, but one for which we
 * must single step and check the condition after each statement.
 */

private condbp(e)
Event e;
{
    Symbol p;
    Breakpoint bp;
    Cmdlist actions;

    p = tcontainer(e->condition);
    if (p == nil) {
	p = program;
    }
    actions = buildcmdlist(build(O_IF, e->condition, e->actions));
    actions = buildcmdlist(build(O_TRACEON, false, actions));
    bp = bp_alloc(e, codeloc(p), 0, actions);
}

/*
 * Determine the deepest nested subprogram that still contains
 * all elements in the given expression.
 */

public Symbol tcontainer(exp)
Node exp;
{
    Integer i;
    Symbol s, t, u, v;

    checkref(exp);
    s = nil;
    if (exp->op == O_SYM) {
	s = container(exp->value.sym);
    } else if (not isleaf(exp->op)) {
	for (i = 0; i < nargs(exp->op); i++) {
	    t = tcontainer(exp->value.arg[i]);
	    if (t != nil) {
		if (s == nil) {
		    s = t;
		} else {
		    u = s;
		    v = t;
		    while (u != v and u != nil) {
			u = container(u);
			v = container(v);
		    }
		    if (u == nil) {
			panic("bad ancestry for \"%s\"", symname(s));
		    } else {
			s = u;
		    }
		}
	    }
	}
    }
    return s;
}

/*
 * Determine if the given function can be executed at full speed.
 * This can only be done if there are no breakpoints within the function.
 */

public Boolean canskip(f)
Symbol f;
{
    Breakpoint p;
    Boolean ok;

    ok = true;
    foreach (Breakpoint, p, bplist)
        if (whatblock(p->bpaddr) == f) {
	    ok = false;
	    break;
	}
    endfor
    return ok;
}

/*
 * Print out what's currently being traced by looking at
 * the currently active events.
 *
 * Some convolution here to translate internal representation
 * of events back into something more palatable.
 */

public status()
{
    Event e;

	if(vectorcapable) {
		fprintf(stderr, "machine is currently vector capable\n");
	}
	else {
		fprintf(stderr, "machine is currently NOT vector capable\n");
	}
	is_vector_capable();
    foreach (Event, e, eventlist)
	if (not e->temporary) {
	    printevent(e);
	}
    endfor
}

public printevent(e)
Event e;
{
    Command cmd;

    if (not isredirected()) {
	printeventid(e->id);
    }
    cmd = list_element(Command, list_head(e->actions));
    if (cmd->op == O_PRINTCALL) {
	printf("trace ");
	printname(stdout, cmd->value.sym);
    } else {
	if (list_size(e->actions) > 1) {
	    printf("{ ");
	}
	foreach (Command, cmd, e->actions)
	    printcmd(stdout, cmd);
	    if (not list_islast()) {
		printf("; ");
	    }
	endfor
	if (list_size(e->actions) > 1) {
	    printf(" }");
	}
	printcond(e->condition);
    }
    printf("\n");
}

private printeventid (id)
integer id;
{
    printf("[%d] ", id);
}

/*
 * Print out a condition.
 */

private printcond(cond)
Node cond;
{
    Symbol s;
    Node place;

    if (cond->op == O_EQ and cond->value.arg[0]->op == O_SYM) {
	s = cond->value.arg[0]->value.sym;
	place = cond->value.arg[1];
	if (s == procsym) {
	    if (place->value.sym != program) {
		printf(" in ");
		printname(stdout, place->value.sym);
	    }
	} else if (s == linesym) {
	    printf(" at ");
	    prtree(stdout, place);
	} else if (s == pcsym or s == retaddrsym) {
	    printf(" i at ");
	    prtree(stdout, place);
	} else {
	    printf(" when ");
	    prtree(stdout, cond);
	}
    } else {
	printf(" when ");
	prtree(stdout, cond);
    }
}

/*
 * Add a breakpoint to the breakpoint list, and return it.
 */

private Breakpoint bp_alloc(e, addr, line, actions)
Event e;
Address addr;
Lineno line;
Cmdlist actions;
{
    register Breakpoint p;

    p = new(Breakpoint);
    p->event = e;
    p->bpaddr = addr;
    p->bpline = line;
    p->actions = actions;
    p->temporary = false;
    if (tracebpts) {
	if (e == nil) {
	    printf("new bp at 0x%x for event ??\n", addr, e->id);
	} else {
	printf("new bp at 0x%x for event %d\n", addr, e->id);
	}
	fflush(stdout);
    }
    bplist_append(p, bplist);
    return p;
}

/*
 * Free all storage in the event and breakpoint tables.
 */

public bpfree()
{
    register Event e;

    fixbps();
    foreach (Event, e, eventlist)
	if (not delevent(e->id)) {
	    printf("!! dbx.bpfree: can't delete event %d\n", e->id);
	}
	list_delete(list_curitem(eventlist), eventlist);
    endfor
}

/*
 * Determine if the program stopped at a known breakpoint
 * and if so do the associated commands.
 */

public boolean bpact()
{
    register Breakpoint p;
    Boolean found;
    integer eventId;

    found = false;
    foreach (Breakpoint, p, bplist)
	if (p->bpaddr == pc) {
	    if (tracebpts) {
		printf("breakpoint for event %d found at location 0x%x\n",
		    p->event->id, pc);
	    }
	    found = true;
	    if (p->event->temporary) {
		if (not delevent(p->event->id)) {
		    printf("!! dbx.bpact: can't find event %d\n",
			p->event->id);
		}
	    }

	    /* See if this breakpoint is associated with a trace event
	     * that has just completed (eg. see traceoff() ).  If so,
	     * delete the associated breakpoint.  (Didn't do this before -
	     * main source of "missing trid" panic:  breakpoint would be
	     * encountered again, but the trcmd associated with it had
	     * been deleted in traceoff() ).
	     */
	    if (p->temporary) {
		list_delete(list_curitem(bplist), bplist);
		} else {
	    evalcmdlist(p->actions);
	    if (isstopped) {
		eventId = p->event->id;
	    }
		}
	}
    endfor
    if (isstopped) {
	if (found) {
	    printeventid(eventId);
	}
	printstatus();
    }
    fflush(stdout);
    return found;
}

/* Sets up a single step context (ie.  global single_stepping = true).
 * As long as this is true, dbx will step by source line
 * (instruction if first arg. is true) until traceoff() is called.
 * If tracing by source line, the commands that are to be executed
 * after each line are appended to eachline.  Otherwise, they are
 * appended to eachinst list.  After stepping each line, printnews() is
 * called, and the commands in eachline (eachinst) are executed.
 *
 * A breakpoint is set at the end of the current procedure to 
 * automatically turn off the given tracing with traceoff().
 */

public traceon(inst, event, cmdlist)
Boolean inst;
Event event;
Cmdlist cmdlist;
{
    register Trcmd trcmd;
    Breakpoint bp;
    Cmdlist actions;
    Address ret;
    Event e;

    if (event == nil) {
	e = curevent;
    } else {
	e = event;
    }
    trcmd = new(Trcmd);
    ++trid;
    trcmd->trid = trid;
    trcmd->event = e;
    trcmd->cmdlist = cmdlist;
    single_stepping = true;
    if (inst) {
	inst_tracing = true;
	list_append(list_item(trcmd), nil, eachinst);
    } else {
	list_append(list_item(trcmd), nil, eachline);
    }
    ret = return_addr();
    if (ret != 0) {  /* ret is 0 if tracing entire program. */
	actions = buildcmdlist(build(O_TRACEOFF, trcmd->trid));
	bp = bp_alloc(e, (Address) ret, 0, actions);
	bp->temporary = true;
    }
    if (tracebpts) {
	printf("adding trace %d for event %d\n", trcmd->trid, e->id);
    }
}

/* Turn off some kind of tracing.
 * Strictly an internal command, this cannot be invoked by the user.
 * First search for and remove the commands listed in eachline (or
 * eachinst) that are associated with this trid.  If trid not found,
 * panic.
 *	  >>> Next, REMOVE THE BREAKPOINT ASSOCIATED WITH THE 
 * 	  >>> EVENT:  This is now done in bpact().
 * Finally, reset inst_tracing and single_stepping flags when there is
 * no more inst/line tracing in effect.
 *
 * Note - this does not delete the event itself, only the trace
 * associated with the event.  In this way, if the user says "run"
 * again, the trace corresponding to this event will be reconstructed.
 */

public traceoff(id)
Integer id;
{
    register Trcmd t;
    register Boolean found;
    register Event e;
    register Breakpoint bp;

    found = false;
    foreach (Trcmd, t, eachline)
	if (t->trid == id) {
	    printrmtr(t);
	    list_delete(list_curitem(eachline), eachline);
	    found = true;
	    break;
	}
    endfor
    if (not found) {
	foreach (Trcmd, t, eachinst)
	    if (t->trid == id) {
		printrmtr(t);
		list_delete(list_curitem(eachinst), eachinst);
		found = true;
		break;
	    }
	endfor
	if (not found) {
	    beginerrmsg();
	    fprintf(stderr, "[internal error: trace id %d not found]\n", id);
	}
    }
    /* Delete associated breakpoints.  This is now done in bpact(),
     * by making use of the "temporary" field in the Breakpoint struct.
     *
    foreach (Event, e, eventlist)
        if (t->event->id == e->id) {
	    foreach (Breakpoint, bp, bplist)
	        if (bp->event == e and bp->bpaddr == pc) {
		    if (tracebpts) {
		        printf("deleting breakpoint at 0x%x", bp->bpaddr);
			printf(" associated with event %d\n", e->id);
			fflush(stdout);
		    }
		    list_delete(list_curitem(bplist), bplist);
		}
	    endfor
	}
    endfor
    */

    /* Reset flags if no more inst/line tracing in effect. */
    if (list_size(eachinst) == 0) {
	inst_tracing = false;
	if (list_size(eachline) == 0) {
	    single_stepping = false;
	}
    }
}

/*
 * If breakpoints are being traced, note that a Trcmd is being deleted.
 */

private printrmtr(t)
Trcmd t;
{
    if (tracebpts) {
	printf("removing trace %d", t->trid);
	if (t->event != nil) {
	    printf(" for event %d", t->event->id);
    }
	printf("\n");
    }
}

/* Print out news during single step tracing.
 * This routine has a pretty bad hack in it.  Because each trace is
 * stored and maintained independently from any other, one trace cannot
 * be aware of what another is doing.  This is expecially evident in
 * recursive routines.  Traceon() is called repeatedly each time the
 * start address of the routine is encountered (where there is a bkpt
 * associated with the event).  Traceon() will add the same command(s)
 * to the eachline list at each invocation of the routine (eg. 
 * O_PRINTSRCPOS).  The case statement in this routine prevents a
 * source line from being displayed as many times as the routine has
 * called itself.  (003 - vjh)
 */

public printnews()
{
    register Trcmd t;
    register Command cmd;
    register Boolean noprint;

    noprint = true;
    foreach (Trcmd, t, eachline)
        foreach(Command, cmd, t->cmdlist)
	    /* This case statement checks to see if we have already
	     * displayed a source line or a return.  It prevents printing
	     * the same line/return over & over in a recursively called
	     * routine.  (003 - vjh)
	     */
	    switch (cmd->op) {
	        case O_PRINTSRCPOS:
		    if (noprint) {
		        evalcmd(cmd);
			noprint = false;
		    }
		    break;

		default:
		    evalcmd(cmd);
		    break;
	    }
	endfor
    endfor
    noprint = true;
    foreach (Trcmd, t, eachinst)
        foreach(Command, cmd, t->cmdlist)
	    switch (cmd->op) {
	        case O_PRINTSRCPOS:
		    if (noprint) {
		        evalcmd(cmd);
			noprint = false;
		    }
		    break;

		default:
		    evalcmd(cmd);
		    break;
	    }
	endfor
    endfor
    bpact();
}

/*
 * A procedure call/return has occurred while single-stepping,
 * note it if we're tracing lines.
 */

private Boolean chklist();

public callnews(iscall)
Boolean iscall;
{
    if (not chklist(eachline, iscall)) {
	chklist(eachinst, iscall);
    }
}

private Boolean chklist(list, iscall)
List list;
Boolean iscall;
{
    register Trcmd t;
    register Command cmd;

    setcurfunc(whatblock(pc));
    foreach (Trcmd, t, list)
	foreach (Command, cmd, t->cmdlist)
	    if (cmd->op == O_PRINTSRCPOS and
	      (cmd->value.arg[0] == nil or cmd->value.arg[0]->op == O_QLINE)) {
		if (iscall) {
		    printentry(curfunc);
		} else {
		    printexit(curfunc);
		}
		return true;
	    }
	endfor
    endfor
    return false;
}

/*
 * When tracing variables we keep a copy of their most recent value
 * and compare it to the current one each time a breakpoint occurs.
 */

#define TRBUFFSIZE 512  /* Used to be MAXTRSIZE - now no max (vjh) */

/*
 * List of variables being watched.
 */

typedef struct Trinfo *Trinfo;

struct Trinfo {
    Node variable;
    Address traddr;
    Symbol trblock;
    char *trvalue;
};

private List trinfolist;

/*
 * Find the trace information record associated with the given record.
 * If there isn't one then create it and add it to the list.
 * Made this work in recursive routines:  checks tp->traddr == addr
 * as well.  (003 - vjh)
 */

private Trinfo findtrinfo(p)
Node p;
{
    register Trinfo tp;
    Boolean isnew;
    Address addr;

    isnew = true;
    addr = lval(p);
    if (trinfolist == nil) {
	trinfolist = list_alloc();
    } else {
	foreach (Trinfo, tp, trinfolist)
	    if (tp->variable == p and tp->traddr == addr) {
		isnew = false;
		break;
	    }
	endfor
    }
    if (isnew) {
	if (tracebpts) {
	    printf("adding trinfo for \"");
	    prtree(stdout, p);
	    printf("\"\n");
	}
	tp = new(Trinfo);
	tp->variable = p;
	tp->traddr = addr;
	tp->trvalue = nil;
	list_append(list_item(tp), nil, trinfolist);
    }
    return tp;
}

/*
 * Print out the value of a variable if it has changed since the
 * last time we checked.
 * Took away size limitations.  Now, if data is > TRBUFFSIZE, malloc()
 * a large enough buffer.  (003 - vjh)
 */

public printifchanged(p)
Node p;
{
    register Trinfo tp;
    register int n;
    char sbuff[TRBUFFSIZE];
    char *buff;
    Filename curfile;
    static Lineno prevline;
    static Filename prevfile;

    tp = findtrinfo(p);
    n = size(p->nodetype);
    if (n > TRBUFFSIZE) {
        buff = newarr(char, n);
    } else {
        buff = sbuff;
    }
    dread(buff, tp->traddr, n);
    curfile = srcfilename(pc);
    if (tp->trvalue == nil) {
	tp->trvalue = newarr(char, n);
	mov(buff, tp->trvalue, n);
	mov(buff, sp, n);
	sp += n;
	printf("initially (at line %d in \"%s\"):\t", curline, curfile);
	prtree(stdout, p);
	printf(" = ");
	printval(p->nodetype);
	putchar('\n');
    } else if (cmp(tp->trvalue, buff, n) != 0) {
	mov(buff, tp->trvalue, n);
	mov(buff, sp, n);
	sp += n;
	printf("after line %d in \"%s\":\t", prevline, prevfile);
	prtree(stdout, p);
	printf(" = ");
	printval(p->nodetype);
	putchar('\n');
    }
    prevline = curline;
    prevfile = curfile;
    if (buff != sbuff) {
        dispose(buff);
    }
}

/*
 * Stop if the value of the given expression has changed.
 * Took away size limitations.  Now, if data is > TRBUFFSIZE, malloc()
 * a large enough buffer.  (003 - vjh)
 */

public stopifchanged(p)
Node p;
{
    register Trinfo tp;
    register int n;
    char sbuff[TRBUFFSIZE];
    char *buff;
    static Lineno prevline;

    tp = findtrinfo(p);
    n = size(p->nodetype);
    if (n > TRBUFFSIZE) {
        buff = newarr(char, n);
    } else {
        buff = sbuff;
    }
    dread(buff, tp->traddr, n);
    if (tp->trvalue == nil) {
	tp->trvalue = newarr(char, n);
	mov(buff, tp->trvalue, n);
	isstopped = true;
    } else if (cmp(tp->trvalue, buff, n) != 0) {
	mov(buff, tp->trvalue, n);
	mov(buff, sp, n);
	sp += n;
	printf("after line %d:\t", prevline);
	prtree(stdout, p);
	printf(" = ");
	printval(p->nodetype);
	putchar('\n');
	isstopped = true;
    }
    prevline = curline;
    if (buff != sbuff) {
        dispose(buff);
    }
}

/*
 * Free the tracing table.
 */

public trfree()
{
    register Trinfo tp;

    foreach (Trinfo, tp, trinfolist)
	dispose(tp->trvalue);
	dispose(tp);
	list_delete(list_curitem(trinfolist), trinfolist);
    endfor
}

/*
 * Fix up breakpoint information before continuing execution.
 *
 * It's necessary to destroy events and breakpoints that were created
 * temporarily and still exist because the program terminated abnormally.
 */

public fixbps()
{
    register Event e;
    register Trcmd t;

    single_stepping = false;
    inst_tracing = false;
    trfree();
    foreach (Event, e, eventlist)
	if (e->temporary) {
	    if (not delevent(e->id)) {
		printf("!! dbx.fixbps: can't find event %d\n", e->id);
	    }
	}
    endfor
    foreach (Trcmd, t, eachline)
	printrmtr(t);
	list_delete(list_curitem(eachline), eachline);
    endfor
    foreach (Trcmd, t, eachinst)
	printrmtr(t);
	list_delete(list_curitem(eachinst), eachinst);
    endfor

	trid = 0;
}

/*
 * Set all breakpoints in object code.
 */

public setallbps()
{
    register Breakpoint p;

    foreach (Breakpoint, p, bplist)
	setbp(p->bpaddr);
    endfor
}

/*
 * Undo damage done by "setallbps".
 */

public unsetallbps()
{
    register Breakpoint p;

    foreach (Breakpoint, p, bplist)
	unsetbp(p->bpaddr);
    endfor
}
