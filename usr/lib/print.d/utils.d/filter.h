/*	@(#)filter.h	4.1      ULTRIX 7/2/90 */

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
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/


/*
 * filter.h -- data structure to gather arguments and execute filters.
 *
 * Filter may be run with
 *	fc_plumb_and_run
 *
 * See filter.c for full explanation
 *
 * As a special case, if an empty filter chain is run it simply copies
 * input to output and fakes a suitable return code and state.
 *
 * Alternative use:- to mimic arbitrary numbers of args to vprintf etc.
 * this will work for all downward growth, stackclick === char * machines
 *
 * The data structure can contain up to FC_MAX_SIMPLE_COMMANDS commands,
 * each of which may have an arbitrary number of arguments.
 *
 * Currently the code breaks if more than FC_MAX_SIMPLE_COMMANDS are set up.
 */

/* SCCS history beginning
 * ***************************************************************
 *                -- Revision History --
 * ***************************************************************
 * 
 * 1.1  02/06/88 -- thoms
 * date and time created 88/06/02 17:45:24 by thoms
 * 
 * ***************************************************************
 *
 * 1.2  15/07/8 -- thoms
 * Added copyright notice and modification history, more comments
 *
 * ***************************************************************
 *
 * 1.3  28/07/88 -- thoms
 * New field fc_fds in struct filter_chain to return pipe fds
 * to caller.
 * Chain pointer fc_next for parasitic filter.
 *
 * ***************************************************************
 * 1.4  01/09/88 -- thoms
 * Declare fc_start
 *
 * ****************************************************************
 *
 * 1.5   2/08/89 -- Giles Atkinson
 * Add members to struct filter_chain to support progress monitoring.
 *
 * SCCS history end
 */


#define FC_MAX_SIMPLE_COMMANDS	10

/* Amount by which argument vector is grown each time it fills up */
#define FC_CLICK	16

/*
 * magic numbers used to record file descriptor information
 */
#define FC_MAKEPIPE	(-2)
#define FC_FD_CLOSED	(-1)
#define FC_STDIN	0
#define FC_STDOUT	1
#define FC_STDERR	2

/*
 * enum fc_state_e -- possible states of a filter
 */
enum fc_state_e	{
	fc_ready, fc_running, fc_done, fc_stopped,
};

struct filter_chain {
	struct filter_chain *fc_next; 	/* hook to chain related filter */
	void		(*fc_mon_start)(/* FCP */); /* Start monitor */
	void		(*fc_mon_stop)();	    /* Stop monitor */
	unsigned	fc_nf;
	short		fc_pid[FC_MAX_SIMPLE_COMMANDS];
	char **		fc_argv[FC_MAX_SIMPLE_COMMANDS + 1];
	short		fc_i;		/* index into current command */
	short		fc_max;		/* arg limit for current command */
	union wait	fc_status;	/* status returned by wait */
	enum fc_state_e	fc_state;	/* state of filter */
	int		fc_fds[3];	/* file descriptors to pipes */
	unsigned short	fc_is_pipe; 	/* bit map identifying pipes */
};

/* shorthand form of pointer to filter object */
typedef struct filter_chain *FCP;


/* utilities in filter.c */
extern char *strsave(/* char *str */);	/* save a string using malloc */
extern void printv(/* char **argv */);	/* print vector of strings

/* initialise filter chain structure */
extern void fc_init(/* FCP fcp */); /* used to initialise static fc */
extern FCP new_fc();		/* allocate and initialise fc */
extern void fc_delete(/* FCP fcp, int automatic */);

/* functions to add new args to current filter */
extern void fc_add_arg(/* FCP fcp, char *arg */);
extern void fc_add_args_l(/* FCP fcp, char *arg, ... */);
extern void fc_add_args_v(/* FCP fcp, char **argv */);
extern void fc_add_args_va(/* FCP fcp, va_list ap */);

/* terminate current filter description */
extern void fc_end_filter(/* FCP fcp */);

/* fire up the filters */
extern void fc_plumb_and_run(/* FCP fcp;
				int fork_action;
				int fd_in;
				int fd_out;
				int fd_err; */);

/* wait for termination of filter */
extern union wait fc_wait(/* FCP fcp */);

/* functions relating to process control of filters run with fc_pout_run */
extern int fc_stop(/* FCP fcp */);
extern int fc_start(/* FCP fcp */);
extern void fc_pout_kill(/* FCP fcp */);

/*
 * Parse a command up to a |, stuff the arguments onto a filter object
 * returns pointer to next non-white or 0
 */
extern char *parse_prog(/* FCP fcp, char *prog */);
