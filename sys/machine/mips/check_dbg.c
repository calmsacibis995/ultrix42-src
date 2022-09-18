/*		4.1	check_dbg.c	*/

/*
 * Copyright 1985 by MIPS Computer Systems, Inc.
 */

/*
 * check_dbg.c -- check for $dbgmon in environ or filename ending
 * in .dbg, if so load debug monitor and transfer control to it
 */

#include "../machine/entrypt.h"

#define	MAXSTRINGS	16		/* max number of strings */
#define	STRINGBYTES	256		/* max total length of strings */

/*
 * string lists are used to maintain argv and environment string lists
 */
struct string_list {
	char *strptrs[MAXSTRINGS];	/* vector of string pointers */
	char strbuf[STRINGBYTES];	/* strings themselves */
	char *strp;			/* free ptr in strbuf */
	int strcnt;			/* number of strings in strptrs */
};

#define	streq(a,b)	(strcmp(a,b)==0)

static struct string_list dbg_argv;
static struct string_list dbg_environ;

static chkenv();
static void init_str();
static new_str1();

_check_dbg(argc, argv, environ, start_pc)
int argc;
char **argv;
char **environ;
{
	register char *cp, *bp;
	register char **wp;
	register int commas;
	register char c;
	int filelen;
	int (*init_pc)();
	struct promexec_args pa;
	char bootfile[64];
	extern char *index();
	extern (*prom_exec())();

	if (argc == 0)
		return;
	/*
	 * Filenames that end in .dbg force loading of the debugger
	 */
	filelen = strlen(argv[0]);
	if (filelen > 4 && streq(".dbg", &argv[0][filelen-4]))
		goto load_debugger;
	if (!chkenv("dbgmon", environ))
		return;	/* debugging not requested */
load_debugger:
	/*
	 * boot dbgmon from same device this command was booted from
	 * but force partition 0, since prom doesn't understand file
	 * systems
	 */
	cp = index(argv[0], ')');
	if (!cp) {
		dprintf("bad filename format %s, can't load dbgmon\n",
			argv[0]);
		return;
	}
	bp = bootfile;
	cp = argv[0];
	commas = 0;
	while ((c = *cp) && commas < 2 && c != ')') {
		if (c == ',')
			commas++;
		*bp++ = *cp++;
	}
	while(commas++ < 2)
		*bp++ = ',';
	strcpy(bp, "0)dbgmon");

	/*
	 * copy args and environment
	 */
	init_str(&dbg_argv);
	for (wp = argv; wp && *wp; wp++)
		if (new_str1(*wp, &dbg_argv))
			return;
	init_str(&dbg_environ);
	for (wp = environ; wp && *wp; wp++)
		if (new_str1(*wp, &dbg_environ))
			return;

	/*
	 * boot in the debug monitor, it had better not overlay us!
	 */
	dprintf("Loading %s\n", bootfile);
	pa.pa_bootfile = bootfile;
	pa.pa_argc = dbg_argv.strcnt;
	pa.pa_argv = dbg_argv.strptrs;
	pa.pa_environ = dbg_environ.strptrs;
	pa.pa_flags = EXEC_NOGO;
	init_pc = prom_exec(&pa);
	if ((int)init_pc == -1) {
		dprintf("couldn't load dbgmon\n");
		return;
	}
	/*
	 * transfer control to dbgmon
	 * give the debugger our argc, argv, environ and the pc of our
	 * main routine so it can properly start us back up.
	 */
	(*init_pc)(argc, argv, environ, start_pc);
	/* shouldn't return */
}

static
chkenv(str, environ)
char *str;
register char **environ;
{
	int len;

	len = strlen(str);
	while (*environ) {
		if (strncmp(str, *environ, len) == 0
		    && (*environ)[len] == '=')
			return(1);
		environ++;
	}
	return(0);
}

/*
 * init_str -- initialize string_list
 */
static void
init_str(slp)
register struct string_list *slp;
{
	slp->strp = slp->strbuf;
	slp->strcnt = 0;
	slp->strptrs[0] = 0;
}

/*
 * new_str1 -- add new string to string list
 */
static
new_str1(strp, slp)
char *strp;
register struct string_list *slp;
{
	register int len;

	if (slp->strcnt >= MAXSTRINGS - 1) {
		dprintf("too many args\n");
		return(-1);
	}

	len = strlen(strp) + 1;
	if (slp->strp + len >= &slp->strbuf[STRINGBYTES]) {
		dprintf("args too long\n");
		return(-1);
	}

	slp->strptrs[slp->strcnt++] = slp->strp;
	slp->strptrs[slp->strcnt] = 0;
	strcpy(slp->strp, strp);
	slp->strp += len;
	return(0);
}
