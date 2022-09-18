#ifndef lint
static char *sccsid = "@(#)filter.c	4.2      ULTRIX 	10/16/90";
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
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/


/*
 * filter.c -- filter argument collection and execution
 *
 * Description:
 *	This file is the implementation of an object to
 *	describe filter programs to be run as sub-shells.
 *
 *	A filter is a pipeline of simple commands.
 *	Each simple command can be specified with
 *	an arbitrary number of arguments.
 *	There is a restriction on the number of simple
 *	commands that may be specified in a pipeline.
 *
 *	Four calls are provided to add arguments to a
 *	particular component of a filter: fc_add_arg*
 *	See below for details.
 *
 #	The filter is run with fc_plumb_and_run.
 *	`Plumb' refers to setting up pipes if necessary.
 *	The call has arguments for passing the 3 file
 *	descriptors to be dup'ed for file descriptors 0, 1 and 2
 *	in the filter.
 *	A special value FC_MAKEPIPE can be specified
 *	for any of these to indicate that a pipe should
 *	be set up to communicate with this file descriptor
 *	in the filter from the parent.
 *	The parent can find the file descriptor of such a
 *	pipe in the fc_fds field of the filter structure
 *	after fc_plumb_and_run has been called.
 */

/* SCCS history beginning
 * ***************************************************************
 *                -- Revision History --
 * ***************************************************************
 * 
 * 1.1  10/03/88 -- thoms
 * date and time created 88/03/10 14:01:09 by thoms
 * 
 * ***************************************************************
 * 
 * 1.2  22/03/88 -- thoms
 * Filter environment is inherited one except for PATH (from lp.local.h)
 * 
 * 
 * ***************************************************************
 * 
 * 1.3  30/03/88 -- thoms
 * Basic PostScript job building and execution working
 * 
 * 
 * ***************************************************************
 * 
 * 1.4  25/04/88 -- thoms
 * Additional check in filter stop code, more debug logging in fc_run
 * 
 * 
 * ***************************************************************
 * 
 * 1.5  29/04/88 -- thoms
 * More debugging in fc_wait()
 * 
 * 
 * ***************************************************************
 * 
 * 1.6  03/05/88 -- thoms
 * Fixed and stable, minimal PS functionality.
 * 
 * 
 * ***************************************************************
 * 
 * 1.7  20/05/88 -- thoms
 * fc_pout_run now has dofork argument to determine behaviour on
 * repeated fork failure
 * 
 * 
 * ***************************************************************
 *
 * 1.8 15/07/88 -- thoms
 * Added copyright notice and modification history, more comments
 * Improved some names
 *
 * ***************************************************************
 *
 * 1.9  19/07/88 -- thoms
 * Fixed a typo
 *
 * ***************************************************************
 *
 * 1.10  28/07/88 -- thoms
 * Generalised piping to filters and replaced the exported
 * calls fc_run and fc_pout_run with generalised function
 * fc_plumb_and_run which opens up pipes as required to
 * the spawned filter.
 * Added code to fc_init, fc_delete and fc_wait to cope
 * with parasitic linked on filter.  Needed for pt_lps_v3.
 *
 * ***************************************************************
 *
 * 1.11  01/09/88 -- thoms
 * Added fc_start function to restart filters, tidied up fc_stop
 * Cleaned up and improved diagnostics in fc_wait
 *
 * ***************************************************************
 *
 * 1.12  09/09/88 -- thoms
 * Added more debug logging to in fc_wait
 *
 * ***************************************************************
 *
 * 1.13  09/09/88 -- thoms
 * Fixed status zeroing bug in fc_wait
 *
 * ***************************************************************
 *
 * 1.14   2/08/89 -- Giles Atkinson
 * Added calls to monitor progress while printing.
 *
 * SCCS history end
 */


#include "lp.h"

/* Declaration of local function */
static char **chenv(/* char **env, envstr */);
static void fc_run(/* register FCP fcp; int fork_action; */);
static int dofork(/* int action_on_fail */);

/* for exported functions declarations see filter.h */


/*
 * dofork - fork with retries on failure
 */
static int
dofork(action)
int action;
{
	register int i, pid;

	for (i = 0; i < DOFORK_NRETRIES; i++) {
		if ((pid = fork()) < 0) {
			sleep((unsigned)(i*i));
			continue;
		}
		/*
		 * Child should run as daemon instead of root
		 */
		if (pid == 0)
			if (setuid(DU) < 0) {
				log("setuid to %d failed", DU);
				exit(1);
			}
		return(pid);
	}
	log("can't fork");

	switch (action) {
	case DORETURN:
		return (-1);
	default:
		log("bad action (%d) to dofork", action);
		/*FALL THRU*/
	case DOABORT:
		exit(1);
	}
	/*NOTREACHED*/
}


/*
 * strsave -- save string using malloc, spotting null pointers
 */
char *
strsave(str)
char *str;
{
	register char *saved;
	static char *null_string = "<null-string>";

	if (!str) {
		str = null_string;
	}
	saved = malloc((unsigned)strlen(str) + 1);
	strcpy(saved, str);

	return saved;
}


/*
 * printv -- print a vector of strings in a standard format to stderr
 */
void
printv(argv)
char **argv;
{
	while (*argv) fprintf(stderr, "\"%s\" ", *argv++);
}

/*
 * chenv -- replace an environment string
 *
 * This should only be called in a sub-process and
 * only after fork, not vfork
 *
 * Return values:
 *	Returns pointer to environ vector on success
 *	0 on failure to find particular env variable
 * Future improvement:
 *	Could build new environment vector to add new
 *	environment variables
 */
static char **
chenv(env, envstr)
char **env;
char *envstr;
{
	register char *vardelim;
	register char **envp;
	register int envstrlen;

	if (!(vardelim = strchr(envstr, '='))) return 0;
	envstrlen = vardelim - envstr;

	for (envp = env; *envp; envp++) {
		if (!strncmp(*envp, envstr, envstrlen)) {
			*envp = envstr;
			return env;
		}
	}
	/* could extend env vector by mallocing one day */
	return 0;
}

/*
 * parse_prog -- perform shell-like parsing of string into filter
 *
 * Return values:
 *	Returns pointer to next non-white or 0 if end of string
 */
char *
parse_prog(fcp, prog_str)
register FCP fcp;		/* resulting args put here */
char *prog_str;		/* input string */
{
	register char *retp, *s, *prog = strsave(prog_str);

	if (retp = strchr(prog, '|')) {
		*retp++ = '\0';
		while (isspace(*retp)) retp++;
		if (!*retp) {
			retp = 0;
		} else {	/* fix up to point into supplied string */
			retp -= (prog - prog_str);
		}
	}
	for (s=strtok(prog, " \t"); s; s=strtok(NULL, " \t")) {
		fc_add_arg(fcp, s);
	}
	fc_end_filter(fcp);
	free(prog);
	return retp;
}


/*
 * fc_grow -- make room for more arguments in filter object
 */
static void
fc_grow(fcp)
register FCP fcp;
{
	register char **p, **plim;
	register unsigned newsiz = (fcp->fc_i + FC_CLICK);
	newsiz /= FC_CLICK;
	newsiz *= FC_CLICK;

	fcp->fc_argv[fcp->fc_nf] =
	    ((fcp->fc_argv[fcp->fc_nf])
	     ? (char **)realloc((char *)fcp->fc_argv[fcp->fc_nf],
				newsiz*sizeof(char *))
	     : (char **)malloc(newsiz*sizeof(char *)));
	plim = &fcp->fc_argv[fcp->fc_nf][newsiz];
	for (p = &fcp->fc_argv[fcp->fc_nf][fcp->fc_max]; p < plim; p++) {
		*p = 0;
	}
	fcp->fc_max = newsiz;
}

/*
 * fc_next_simple_command_init -- init next filter component description
 */
static void
fc_next_simple_command_init(fcp)
register FCP fcp;
{
	
	fcp->fc_i = 0;
	fcp->fc_max = 0;
	fc_grow(fcp);
}

/*
 * fc_init -- initialise filter chain structure
 */
void fc_init(fcp)
register FCP fcp;
{
	register int i;

	memset(fcp, '\0', sizeof(*fcp));

	for (i = 0; i < 3; i++) {
		fcp->fc_fds[i] = FC_FD_CLOSED;
	}

	for (i = 0; i < FC_MAX_SIMPLE_COMMANDS; i++ ) {
		fcp->fc_pid[i] = -1;
	}

	fc_next_simple_command_init(fcp);
}

/*
 * new_fc -- allocate filter object and initialise
 */
FCP
new_fc()
{
	register FCP new_fcp;
	new_fcp = (FCP)malloc(sizeof(*new_fcp));
	fc_init(new_fcp);
	return new_fcp;
}

/*
 * fc_delete -- delete filter object
 */
void fc_delete(fcp, on_heap)
FCP fcp;
int on_heap;			/* was malloc'ed if non-zero */
{
	register int i;

	/*
	 * If we had any pipes set up to the filter make
	 * sure they're closed down
	 */
	for (i=0; i < 3; i++) {
		if (fcp->fc_is_pipe & (1<<i) && fcp->fc_fds[i] >= 0) {
			(void) close(fcp->fc_fds[i]);
			fcp->fc_fds[i] = FC_FD_CLOSED;
		}
	}
	for (i = 0; i <= fcp->fc_nf; i++) {
		register char **p, **pv;
		if (!(pv = fcp->fc_argv[i])) continue;
		for (p = pv; *p; p++) free(*p);
		free((char *)pv);
	}
	if (fcp->fc_next) fc_delete(fcp->fc_next, 1);
	if (on_heap) free((char *)fcp);
}

/*
 * fc_end_filter -- terminate current simple command description
 */
void
fc_end_filter(fcp)
register FCP fcp;
{
	assert(fcp->fc_nf <= FC_MAX_SIMPLE_COMMANDS);
	if (fcp->fc_i >= fcp->fc_max) {
		fc_grow(fcp);
	}
	fcp->fc_argv[fcp->fc_nf++][fcp->fc_i] = 0;
	fc_next_simple_command_init(fcp);
}

/****************************************************************
*	functions to add new args to current simple command
****************************************************************/

/*
 * fc_add_arg -- add one argument
 */
void
fc_add_arg(fcp, arg)
register FCP fcp;
char *arg;
{
	if (fcp->fc_i >= fcp->fc_max) {
		fc_grow(fcp);
	}
	fcp->fc_argv[fcp->fc_nf][fcp->fc_i++] = strsave(arg);
}

/*
 * fc_add_args_l -- add 0 terminated list of arguments to
 *	current simple command
 */
/*VARARGS1*/
void
fc_add_args_l(fcp, va_alist)
register FCP fcp;
va_dcl
{
	va_list ap;
	register char *str;
	va_start(ap);

	while (str = va_arg(ap, char *)) {
		if (fcp->fc_i >= fcp->fc_max) {
			fc_grow(fcp);
		}
		fcp->fc_argv[fcp->fc_nf][fcp->fc_i++] = strsave(str);
	}
	va_end(ap);
}

/*
 * fc_add_args_v -- add a vector of arguments to current simple command
 */
void
fc_add_args_v(fcp, argv)
register FCP fcp;
char **argv;
{
	while (*argv) {
		if (fcp->fc_i >= fcp->fc_max) {
			fc_grow(fcp);
		}
		fcp->fc_argv[fcp->fc_nf][fcp->fc_i++] = strsave(*argv++);
	}
}

/*
 * fc_add_args_va -- add va_list of arguments to current simple command
 */
void
fc_add_args_va(fcp, ap)
register FCP fcp;
va_list ap;
{
	register char *str;

	while (str = va_arg(ap, char *)) {
		if (fcp->fc_i >= fcp->fc_max) {
			fc_grow(fcp);
		}
		fcp->fc_argv[fcp->fc_nf][fcp->fc_i++] = strsave(str);
	}
}

/****************************************************************
*		functions to fire up the filters
****************************************************************/

/*
 * fc_run -- run a filter
 *
 * Description:
 *	This does the business after any necessary plumbing (pipes)
 *	to the file descriptors 0, 1 & 2 has been done by fc_plumb_and_run()
 */
static void
fc_run(fcp, fork_action)
register FCP fcp;
int fork_action;		/* argument to dofork() */
{
	register int nf;	/* counter for pipe building loop */
	extern char **environ;

	if (fcp->fc_mon_start)		/* Start progress monitor */
		(*fcp->fc_mon_start)(fcp);

	if (fcp->fc_nf == 0) { /* no filters: straight copy */
		register int n;
		char buf[BUFSIZ];

		dlog(0, "fc_run: straight copy");
		while ((n = read(fcp->fc_fds[FC_STDIN], buf, BUFSIZ)) > 0) {
			if (write(fcp->fc_fds[FC_STDOUT], buf, n) != n) {
				fcp->fc_status.w_retcode = 1;
				break;
			}
		}
		if (fcp->fc_mon_stop)		/* Stop progress monitor */
			(*fcp->fc_mon_stop)();
		fcp->fc_state = fc_done;
		return;
	}
	if (DB > 0) { /* DEBUGGING: print log message */
		int i;
		log("fc_run: ");
		for (i = 0; i < fcp->fc_nf; ) {
			printv(fcp->fc_argv[i]);
			fprintf(stderr, (++i < fcp->fc_nf) ? "| " : "\n");
		}
	}
	/*
	 * Many actions in the pipe building loop
	 * are conditional on the following macros which
	 * determine whether the particular simple command
	 * is first or last in the pipeline and therefore
	 * needs to be plumbed to the outside world.
	 */
#define		first_in_pipeline	(nf == 0)
#define		last_in_pipeline	(nf == (fcp->fc_nf-1))

	for (nf = 0; nf < fcp->fc_nf; nf++) {
		int pipe_fd[2];	/* pipe file descriptors */
		int i;		/* index for handling vector of fd's */

		if (!last_in_pipeline) pipe(pipe_fd);

		if ((fcp->fc_pid[nf] = dofork(fork_action)) == 0) {
			/* child */
			if (!last_in_pipeline) {
				fcp->fc_fds[FC_STDOUT] = pipe_fd[1];
				close(pipe_fd[0]);
			}
			/*
			 * deal with file descriptors 0, 1 & 2
			 */
			for (i = 0; i < 3; i++) {
				if (fcp->fc_fds[i] >= 0) {
					if (fcp->fc_fds[i] != i)
					    (void)dup2(fcp->fc_fds[i], i);
				} else {
					(void) close(i);
				}
			}
			/* close rest of file descriptors */
			for (i = 3; i < NOFILE; i++) (void) close(i);

			/* set up new PATH in environment */
			if (PATH_STRING &&
			    !(environ = chenv(environ, PATH_STRING))) {
				log("No PATH in inherited environment");
				exit(3);
			}

			execvp(fcp->fc_argv[nf][0], fcp->fc_argv[nf]);

			log("filter %s: not found\n",
			    fcp->fc_argv[nf][0]);
			exit(2);
		} else if (fcp->fc_pid[nf] > 0) {	/* parent */
			dlog(0, "fc_run: filter %d, pid %d",
			     nf, fcp->fc_pid[nf]);
			if (!first_in_pipeline) {
				(void) close(fcp->fc_fds[FC_STDIN]);
			}
			if (!last_in_pipeline) {
				(void) close(pipe_fd[1]);
				fcp->fc_fds[FC_STDIN] = pipe_fd[0];
			}
			fcp->fc_state = fc_running;
		} else {	/* dofork() failed */
			if (!last_in_pipeline) {
				(void) close(pipe_fd[0]);
				(void) close(pipe_fd[1]);
			}
			fcp->fc_status.w_retcode = 1; /* force a retry */
			fcp->fc_state = fc_done;
			break;
		}
	}
	return;
}

/*
 * fc_plumb_and_run -- Exported function to run a filter
 *
 * Description:
 *	We build any pipes requested and then call fc_run
 *	Note that the ends of the pipes of interest to
 *	the children are passed via the array in the filter struct.
 *	The ends of the pipe which interest the parent are copied
 *	in by this function before it returns.
 */
extern void
fc_plumb_and_run(fcp, fork_action, fd_in, fd_out, fd_err)
register FCP fcp;
int fork_action;
int fd_in;
int fd_out;
int fd_err;
{
	int pipe_fd[2];
	int parent_fds[3];
	int pipe_unwanted_fds[3];
	register int i;


	fcp->fc_fds[FC_STDIN] = fd_in;
	fcp->fc_fds[FC_STDOUT] = fd_out;
	fcp->fc_fds[FC_STDERR] = fd_err;

	for (i=0; i < 3; i++) {
		if (fcp->fc_fds[i] == FC_MAKEPIPE) {
			fcp->fc_is_pipe |= (1<<i);
			parent_fds[i] = fcp->fc_fds[i];
		}
	}

	/*
	 * If we have any pipes at all then it must not be
	 * a null filter
	 */
	if (fcp->fc_is_pipe && fcp->fc_nf == 0) {
		/* This shouldn't happen, indicates coding error */
		dlog(0,"fc_plumb_and_run: Can't pipe to empty filter");
		fcp->fc_is_pipe = 0;
		fcp->fc_state = fc_done;
	}


	if (fcp->fc_is_pipe & (1<<FC_STDIN)) {
		pipe(pipe_fd);
		pipe_unwanted_fds[FC_STDIN] = fcp->fc_fds[FC_STDIN]
		    = pipe_fd[0];
		parent_fds[FC_STDIN] = pipe_fd[1];
	}
	if (fcp->fc_is_pipe & (1<<FC_STDOUT)) {
		pipe(pipe_fd);
		pipe_unwanted_fds[FC_STDOUT] = fcp->fc_fds[FC_STDOUT]
		    = pipe_fd[1];
		parent_fds[FC_STDOUT] = pipe_fd[0];
	}
	if (fcp->fc_is_pipe & (1<<FC_STDERR)) {
		pipe(pipe_fd);
		pipe_unwanted_fds[FC_STDERR] = fcp->fc_fds[FC_STDERR]
		    = pipe_fd[1];
		parent_fds[FC_STDERR] = pipe_fd[0];
	}

	fc_run(fcp, fork_action);

	for (i=0; i < 3; i++) {
		if (fcp->fc_is_pipe & (1<<i)) {
			fcp->fc_fds[i] = parent_fds[i];
			(void) close(pipe_unwanted_fds[i]);
		} else {
			fcp->fc_fds[i] = FC_FD_CLOSED;
		}
	}
}


/*
 * fc_wait wait for termination of filter and return exit status
 */
union wait
fc_wait(fcp)
FCP fcp;
{
	register int pid = -1;
	register int last_pid;

	if (fcp->fc_state == fc_running && fcp->fc_nf > 0) {
		last_pid =  fcp->fc_pid[fcp->fc_nf - 1];

		dlog(0, "fc_wait: filter %d %s", last_pid,
		     ((kill(last_pid, 0) < 0)
		      ? "has exited" : "is still running"));

		for (pid = 0; pid >= 0 && pid != last_pid; ) {
			fcp->fc_status.w_status = 0;
			pid = wait(&fcp->fc_status);
			if (pid > 0)
			    dlog(0, "fc_wait: Found process %d", pid);
		}
		if (fcp->fc_mon_stop)		/* Stop progress monitor */
			(*fcp->fc_mon_stop)();
		fcp->fc_state = fc_done;
		if (pid != last_pid) {
			dlog(0, "fc_wait: Couldn't find process %d", last_pid);
		} else if (DB > 0){
			if (WIFEXITED(fcp->fc_status)) {
				log("Process %d exited status %d",
				    pid, fcp->fc_status.w_retcode);
			} else if (WIFSIGNALED(fcp->fc_status)) {
				log("Process %d killed by signal %d%s",
				    pid, fcp->fc_status.w_termsig,
				    ((fcp->fc_status.w_coredump) ?
				     " with core dump" : ""));
			}
		}
	}
	/* This recursive call is to clean up lpserrof */
	if (fcp->fc_next) (void)fc_wait(fcp->fc_next);
	return fcp->fc_status;
}

/****************************************************************
*	functions relating to process control of filters run
*	with a pipe to their standard input
****************************************************************/

/*
 * fc_stop -- stop output filter with ^Y^A, then wash hands
 */
int
fc_stop(fcp)
FCP fcp;
{
	int pid;

	if (!(fcp->fc_is_pipe && (1 <<0))) {
		dlog(0, "fc_stop: no input pipe");
		return -1;
	}
	switch (fcp->fc_state) {
	    default:		/* not in suitable state: error */
		break;
	    case fc_stopped:	/* already stopped, nothing to do */
		break;
	    case fc_running:	/* ok to stop */
		write(fcp->fc_fds[0], "\031\1", 2);
		while ((pid=wait3(&fcp->fc_status, WUNTRACED, 0)) > 0
		       && pid != fcp->fc_pid[0])
		    {}
		fcp->fc_state = ((WIFEXITED(fcp->fc_status))
				 ? fc_done : fc_stopped);	
		break;
	}
 	return ((fcp->fc_state == fc_stopped)
		? 0 : -1); /* ok if stopped, else error */
}

/*
 * fc_start -- restart output filter with SIGCONT
 */
int
fc_start(fcp)
FCP fcp;
{
	int pid;

	if (!(fcp->fc_is_pipe && (1 <<0))) {
		dlog(0, "fc_start: no input pipe");
		return -1;
	}
	switch (fcp->fc_state) {
	    default:		/* not in suitable state: error */
		break;
	    case fc_running:	/* already running, nothing to do */
		break;
	    case fc_stopped:	/* ok to restart */
		if (kill(fcp->fc_pid[0], SIGCONT) < 0) {
			dlog(0, "fc_start: process %d died",fcp->fc_pid[0]);
			fcp->fc_state = fc_done;
		} else {
			fcp->fc_state = fc_running;
		}
		break;
	}
 	return ((fcp->fc_state == fc_running)
		? 0 : -1); /* ok if running, else error */
}

/*
 * fc_pout_kill -- kill output filter by closing stdin
 *	restart first if necessary
 */
void fc_pout_kill(fcp)
FCP fcp;
{
	register int pid;

	if (fc_start(fcp) == 0) {
		(void) close(fcp->fc_fds[0]);
		fcp->fc_fds[0] = FC_FD_CLOSED;
	} else {
		dlog(0, "fc_pout_kill: error: filter in state %s",
		     "ready\0runng\0done\0\0stopped"[6*(int)fcp->fc_state]);
	}
}
