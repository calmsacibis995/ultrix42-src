#ifndef lint
static char *sccsid = "@(#)misc.c	4.5      ULTRIX 	10/16/90";
#endif

/************************************************************************
 *									*
 *			Copyright (c) 1989 by				*
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


#include <stdio.h>
#include <strings.h>
#include <sys/ioctl.h>
#include "lprsetup.h"
#include "argstrings.h"

/*
 *	Adapted for ULTRIX-32
 *	from ULTRIX-11 version 
 */

/**********************************************************************
 * Modification History
 *
 * 04-Oct-90 - Adrian Thoms (thoms@wessex)
 *	Fixed so that when a parameter is set it will actually appear
 *	even when its value is the default in the template
 *
 * 13 Nov 89	Adrian Thoms (for Daren Seymour)
 *	Bug fix
 *
 * 19-Jan-89
 *
 *    DJG#4 - Added routine  GetRows which sets the global variable
 *            "rows" to contain the number of rows on the output window
 *
 *    DJG#3 - Added routines PrintHelp and sgetline for printing help
 *    messages in a manner like  "more"
 *
 * 05-Jan-89
 *
 *    DJG#2 - Modified getcmd to copy string entered as originally typed.
 *    Previously could not find printer synonyms consisting of uppercase
 *    characters.
 *
 *    DJG#1 - Added a new functions SynonymMatch, and CommentOrEmpty. They are  
 *    used in the Modify command to find a printer name in a list of synonyms.
 *
 *********************************************************************/

FILE *Popen();


/*******************************************************
 * SynonymMatch - returns a pointer to the first
 * occurrance of the synonym in the list, it returns NULL
 * if it can not find one or if synonym is longer than list.
 * DJG#1
 *******************************************************/

char *
SynonymMatch (list, synonym)

char	*list, *synonym;

{
#define SEPARATORS  "|:"       /* separators in synonym "list" */
#define BLANK      " "         /* identifies end of synonym list */

    char *token, *strtok();
    char tmplist[BUFLEN];

    strcpy (tmplist, list);    /* because strtok corrupts string */
    if (strlen (synonym) <= strlen (tmplist)) {
	/* get first token in list */
	token = strtok (tmplist, SEPARATORS);
	while (token != NULL) {
	    if (index (token, BLANK) == 0) { /* its a legal synonym */
		if (strcmp (token, synonym) == 0) 
		    return (token);
	    }
	    else {  /* its the last entry, which doesn't count */
	        return (NULL);
	    }
	    /* Get next token */
	    token = strtok (0, SEPARATORS);
	}
    }
    return (NULL);
}
	    
/*********************************************************
 * CommentOrEmpty - Returns true if the line passed is a
 * comment or an empty line, false otherwise. The line is
 * from the /etc/printcap file.   DJG#1
 *********************************************************/

int
CommentOrEmpty (line)
char	line[];
{
    int      i;

    if (line[0] == '#') { /* the line is a comment */
	return (1);
    }
    else { /* its empty or something is on it */
        for (i = 0; line[i] != '\0'; i++ ) {
	    if (line[i] != ' ' && line[i] != '\t') {
	        if (line[i] == '\n')        /* the line is empty */
		    return (1);
	        else                        /* somethings on it  */
		    return (0);
	    }
        }
    }
}

/*********************************
* YesNo - get a yes or no answer
* Returns TRUE=(yes), FALSE=(no)
**********************************/
YesNo (def_ans)
int    def_ans;
{
    int     ans, done;
	
    done = FALSE;
    ans = 0;
    while (NOT done)
    {
	ans = getcmd ();
	switch (ans)
	{
	    case NOREPLY: 
		done = TRUE;
		if (def_ans == 'y')
		    ans = TRUE;
		else
		    if (def_ans == 'n')
			ans = FALSE;
		break;
	    case YES: 
		done = TRUE;
		ans = TRUE;
		break;
	    case NO: 
	    case QUIT: 
		done = TRUE;
		ans = FALSE;
		break;
	    case HELP: 
	    default: 
		printf ("\nPlease answer yes or no  [%c] ", def_ans);
	}
    }
    return (ans);
}

/******************************************************************
* UseDefault
*	returns TRUE if NULL entered (meaning use default) or
*	FALSE after reading text to use in place of default
******************************************************************/
UseDefault (str, def, i)
char   *str, *def;
int	i;		/* index into table of current symbol name */
			/* which is used for more specific help messages */
{
    extern char symbolname[];
    extern struct table tab[];
    int    done;

    /******************
    * loop until done
    ******************/
    done = FALSE;
    while (!done)
    {
        printf (" [%s] ?  ", def);
	switch (getsymbol ())
	{
	    default: 	/* this should never match! */
	    case GOT_SYMBOL: 
		strcpy (str, symbolname);
		return (FALSE);
		break;
            case NO:
                tab[i].used = NO;
                return (TRUE);
                break;
	    case QUIT: 
		return (QUIT);
		break;
	    case HELP: 
		realhelp(i);		/* more specific help */
		PrintHelp (h_default); 	/* a general help message */
		printf ("\nEnter a value for '%s'? ", tab[i].name, def);
		break;
	    case NOREPLY: 
		if (str != def)
	    	    strcpy (str, def);
		return (TRUE);
		break;
	}
    }
}

/*********************************************
* realhelp
*	provides specific help for a given
*	symbol index.
*********************************************/

realhelp(i)
int	i;
{
    extern struct table tab[];

    switch(i) {
	case H_af:
		PrintHelp(h_af);
		break;

	case H_br:
		PrintHelp(h_br);
		break;

	case H_cf:
		PrintHelp(h_cf);
		break;

	case H_ct:
		PrintHelp(h_ct);
		break;

	case H_df:
		PrintHelp(h_df);
		break;

	case H_dn:
		PrintHelp(h_dn);
		break;

	case H_du:
		PrintHelp(h_du);
		break;

	case H_fc:
		PrintHelp(h_fc);
		break;

	case H_ff:
		PrintHelp(h_ff);
		break;

	case H_fo:
		PrintHelp(h_fo);
		break;

	case H_fs:
		PrintHelp(h_fs);
		break;

	case H_gf:
		PrintHelp(h_gf);
		break;

	case H_ic:
		PrintHelp(h_ic);
		break;

	case H_if:
		PrintHelp(h_if);
		break;

	case H_lf:
		PrintHelp(h_lf);
		break;

	case H_lo:
		PrintHelp(h_lo);
		break;

	case H_lp:
		PrintHelp(h_lp);
		break;

	case H_mc:
		PrintHelp(h_mc);
		break;

	case H_mx:
		PrintHelp(h_mx);
		break;

	case H_nf:
		PrintHelp(h_nf);
		break;

	case H_of:
		PrintHelp(h_of);
		break;

	case H_op:
		PrintHelp(h_op);
		break;

	case H_os:
		PrintHelp(h_os);
		break;

	case H_pl:
		PrintHelp(h_pl);
		break;

	case H_pp:
		PrintHelp(h_pp);
		break;

	case H_ps:
		PrintHelp(h_ps);
		break;

	case H_pw:
		PrintHelp(h_pw);
		break;

	case H_px:
		PrintHelp(h_px);
		break;

	case H_py:
		PrintHelp(h_py);
		break;

	case H_rf:
		PrintHelp(h_rf);
		break;

	case H_rm:
		PrintHelp(h_rm);
		break;

	case H_rp:
		PrintHelp(h_rp);
		break;

	case H_rs:
		PrintHelp(h_rs);
		break;

	case H_rw:
		PrintHelp(h_rw);
		break;

	case H_sb:
		PrintHelp(h_sb);
		break;

	case H_sc:
		PrintHelp(h_sc);
		break;

	case H_sd:
		PrintHelp(h_sd);
		break;

	case H_sf:
		PrintHelp(h_sf);
		break;

	case H_sh:
		PrintHelp(h_sh);
		break;

	case H_st:
		PrintHelp(h_st);
		break;

	case H_tf:
		PrintHelp(h_tf);
		break;

	case H_tr:
		PrintHelp(h_tr);
		break;

	case H_ts:
		PrintHelp(h_ts);
		break;

	case H_uv:
		PrintHelp(h_uv);
		break;

	case H_vf:
		PrintHelp(h_vf);
		break;

	case H_xc:
		PrintHelp(h_xc);
		break;

	case H_xf:
		PrintHelp(h_xf);
		break;

	case H_xs:
		PrintHelp(h_xs);
		break;

	case H_Da:
		PrintHelp(h_Da);
		break;

	case H_Dl:
		PrintHelp(h_Dl);
		break;

	case H_It:
		PrintHelp(h_It);
		break;

	case H_Lf:
		PrintHelp(h_Lf);
		break;

	case H_Lu:
		PrintHelp(h_Lu);
		break;

	case H_Ml:
		PrintHelp(h_Ml);
		break;

	case H_Nu:
		PrintHelp(h_Nu);
		break;

	case H_Or:
		PrintHelp(h_Or);
		break;

	case H_Ot:
		PrintHelp(h_Ot);
		break;

	case H_Ps:
		PrintHelp(h_Ps);
		break;

	case H_Sd:
		PrintHelp(h_Sd);
		break;

	case H_Si:
		PrintHelp(h_Si);
		break;

	case H_Ss:
		PrintHelp(h_Ss);
		break;

	case H_Ul:
		PrintHelp(h_Ul);
		break;

	case H_Xf:
		PrintHelp(h_Xf);
		break;

	default:
		printf("Sorry, no specific help is available for symbol '%s'\n",
			tab[i].name);
		break;
    }
}

/*********************************************
* MapLowerCase
*     maps the given string into lower-case.
**********************************************/
MapLowerCase (b)
char   *b;
{
    while (*b)
    {
	if (isascii (*b) && isupper (*b))
	    *b = tolower (*b);
	b++;
    }
}

HasBadChars (b)
char   *b;
{
    while (*b)
    {
	if ((NOT isalpha (*b)) && (NOT isdigit (*b)) && (*b != '_'))
	    return (TRUE);
	b++;
    }
    return (FALSE);
}

/************************************************
*  Print the symbol table
* print whole table, or just the 'used' symbols 
*************************************************/
Print (flag)
int     flag;
{
    extern struct table tab[];
    extern char pnum[];
    extern char ptype[];
    int     i, j;

    printf ("\n\tPrinter #%s ",pnum);
    printf("\n\t----------");
    if (strlen(pnum) > 1)
	printf("-");		/* add one for printers numbered 10...99 */
    printf ("\nSymbol ");
    if (flag == ALL)
	printf ("used ");

    printf (" type  value\n");
    printf ("------ ");
    if (flag == ALL)
	printf ("---- ");  /* under 'used' */

    printf (" ----  -----\n");

    /*************************************************
    * for each symbol, print name, type, used, value 
    **************************************************/
    for (i=0; tab[i].name != 0; i++)
    {
   	 /* don't print it, if not being used now */
	if ((flag == USED) && (tab[i].used == NO))
	    continue;

	printf ("  %s   ", tab[i].name);
	if (flag == ALL)
	    printf ("%s", tab[i].used == YES ? "YES  " : " NO  ");

	switch (tab[i].stype)
	{
	    case BOOL: 
		printf (" BOOL ");
		break;
	    case INT: 
		printf (" INT  ");
		break;
	    case STR: 
		printf (" STR  ");
		break;
	    default: 
		printf (" ???    ??????\n");
		continue;	/* get next symbol */
	}
 
 /***
	if ((flag == ALL) && (tab[i].used == NO)) {
		printf("\n");
		continue;
	}
  ***/

	if ((tab[i].nvalue != 0) && (tab[i].nvalue != '\0')) {
	    printf ("  %s", tab[i].nvalue);
	}
	else if ((tab[i].svalue != 0) && (tab[i].svalue != '\0')) {
	    printf ("  %s", tab[i].svalue);
	}
	printf("\n");		/* end the line */
    }
}

/********************************************************************
* Verified
*	print the current printcap data, ask if it is OK, and return
*	TRUE if it is OK, otherwise false.	
*********************************************************************/
Verified ()
{
    extern char pnum[];
	int yn;
/*
 *  Clear all waiting input and output chars.
 *  Actually we just want to clear any waiting input chars so
 *  we have a chance to see the values before confirming them.
 *  We have to sleep a second to let waiting output chars print.
 */
    sleep (1);
    ioctl (0, TIOCFLUSH, 0);

    Print (USED);		/* print values being used in current
				   configuration  */

    printf ("\nAre these the final values for printer %s ? [y] ", pnum);
    fflush (stdout);
	yn = 'y';
    if (YesNo (yn) == TRUE) {
	printf("\n");
	return (TRUE);
    }
    else {
	printf("\n");
	return (FALSE);
    }
}

/********************************************
*  DoSymbol - adds/modifies symbols.
********************************************/
DoSymbol ()
{
    extern struct table tab[];		/* default printer table	*/
    extern char symbolname[];		/* getcmd result		*/
    extern char oldfilter[];		/* print filter before modify	*/
    extern char ptype[];		/* for checking on 'af' use	*/
    extern char isnotused[];		/* "...feature is not used in LP11... */
    char     newval[LEN];		/* new value entered		*/
    char *addr, *curval;		/* malloc and current value	*/
    int     i, done = FALSE;
	int yn;

    /* 
     * find the symbol, print current value, and
     * ask for the new value, or initial value,
     * if any.
     */
    if (strlen (symbolname) > 2)
    {
	printf ("\nSymbol name '%s' is too long!\n", symbolname);
	return;
    }

    /* symbolname contains the line just read from stdin */
    for (i = 0; tab[i].name != 0; i++)
    {
	if (strcmp (tab[i].name, symbolname) == 0)
	{
	    curval = tab[i].nvalue ? tab[i].nvalue : tab[i].svalue;
	    break;
	}
    }
    if (tab[i].name == 0)
    {
	printf("\nSymbol '%s' not found.  Use the 'list' command for a\n",
		symbolname);
	printf("complete list of all of the symbols and their defaults.\n");
	return(ERROR);
    }

    /*
     * got symbol, now prompt for new value
     */
    do
    {
	printf ("\nEnter a new value for symbol '%s'?  [%s] ",
		tab[i].name, curval);
	switch (getsymbol() )
	{
	    case QUIT: 
		return (QUIT);
		break;
	    case HELP: 
		realhelp(i);		/* more specific help */
		PrintHelp (h_default);
		break;
            case NO:          /* easily turn this parameter off, ie. set used */
                tab[i].used = NO;
                done = TRUE;
                break;
	    case NOREPLY: 
		if ((strcmp (curval, "none") != 0))
    			tab[i].used = YES;
		done = TRUE;
		break;
	    case GOT_SYMBOL: 
	            default:
		strcpy (newval, symbolname);
		printf ("\nNew '%s' is '%s', is this correct?  [y] ",
			tab[i].name, newval);
			yn = 'y';
		if (YesNo (yn) == TRUE)
		{
		    if (validate(i, newval) < 0)	/* check if valid */
			continue;


		    if (strcmp (newval, curval) != 0)
		    {
			if ((addr = (char *) malloc (strlen (newval) + 1)) == NULL)
			{
			    printf ("\nmalloc: cannot get space for symbol '%s'.\n",
				    tab[i].name);
			    return(ERROR);
			}
		    	tab[i].nvalue = addr;
		    	strcpy (tab[i].nvalue, newval);
		    }
		    tab[i].used = YES;
		    done = TRUE;
		}
		break;
	}
    } while (NOT done);
}

/*
 * invalid postscript argument - print error messege + valid args
 */
static void invalid_arg(opt, opt_num)
char *opt;
int opt_num;
{

	struct arg_pair *arg_list;
	int width=0;

	get_args(opt_num,&arg_list);
	printf("\nSorry, the value of '%s' must be one of: ", opt);

	while (arg_list->arg) {
		printf("%s ", arg_list->arg);
		arg_list++;
	}
	printf("\n");
}
	
/*
 * validate: check that 
 *	EITHER 
 *		entered value = "none" to remove entry
 *	OR
 *		int's are all digits
 *		baud rates are legal 
 * 		booleans are on/off
 *		directory names must all start with '/' 
 *			(except "lp" for remote should be "null"
 *                       or start with a "@/" for tcp/ip connections )
 *
 * returns -1 if bad, else 0.
 */
validate(i,value1)
int i;
char *value1;
{
	extern struct table tab[];
	int retval = OK;	/* return value from this routine */
	int k;			/* loop counter */
	char value[LEN];	/* value of the symbol just entered */
	char dummy[LEN];

	/*
	 * save the symbol value locally
	 */
	if ((strcmp("none",value1)==0) 
            || (strcmp ("no", value1) == 0)) {	/* "none" = delete this entry */
		tab[i].nun = 1;
                tab[i].used = NO;
		strcpy(value1,"");
		retval = OK;
		return (retval);
	}

	    if (value1 != NULL)
	    	strcpy(value, value1);
	    else
	    	strcpy(value, "");		/* probably can't happen */

	switch (tab[i].stype)
	{
	    case BOOL: 
		/* Booleans can only be on or off */
		if    (!(!strcmp("on", value)
		    || (! strcmp("ON", value))
		    || (! strcmp("off", value))
		    || (! strcmp("OFF", value))))
		{
		    printf("\nSorry, boolean symbol '%s' can only be 'on' or 'off'\n", tab[i].name);
		    retval=BAD;
		}
		break;

	    case INT: 
		/* check first that we have all digits */
		for (k = 0; value[k]!='\0'; k++)
		{
		    if (! isdigit(value[k]))
		    {
			printf("\nSorry, integer symbol '%s' must contain only digits (0 - 9).\n", tab[i].name);
			retval=BAD;
			break;
		    }
		}

		/*
		 * See if BR was specified and check if it is valid
		 * only if we haven't encountered an error yet.
		 */
		if ((strcmp("br", tab[i].name) == 0) && (retval != BAD))
		{
		    switch(atoi(value))
		    {
		    case 0:
		    case 50:
		    case 75:
		    case 110:
		    case 134:
		    case 150:
		    case 200:
		    case 300:
		    case 600:
		    case 1200:
		    case 1800:
		    case 2400:
		    case 4800:
		    case 9600:
		    case 19200:
		    case 38400:	
                    case 57600:
			break;	/* baud rate OK */

		    default:
			printf("\nSorry, illegal baudrate: %s\n", value);
			printf("\nAvailable baud rates are:\n");
			printf("\t   0\t  134\t   600\t  4800\n");
			printf("\t  50\t  150\t  1200\t  9600\n");
			printf("\t  75\t  200\t  1800\t 19200\n");
			printf("\t 110\t  300\t  2400\t 38400\n");
			printf("\t 57600\n");
			retval = BAD;
			break;
		    }
		}
		break;

	    case STR: 
		/* check if name is special and must start with '/' */
		if     (!strcmp ("af", tab[i].name) 	/* accounting file */
		    || (!strcmp ("cf", tab[i].name))	/* cifplot filter */
		    || (!strcmp ("df", tab[i].name))	/* TeX DVI filter */
		    || (!strcmp ("dn", tab[i].name))	/* daemon name */
		    || (!strcmp ("gf", tab[i].name))	/* plot filter */
		    || (!strcmp ("lf", tab[i].name))	/* logfile */
		    || (!strcmp ("lp", tab[i].name))	/* device name */
		    || (!strcmp ("nf", tab[i].name))	/* ditroff filter */
		    || (!strcmp ("rf", tab[i].name))	/* FORTRAN filter */
		    || (!strcmp ("sd", tab[i].name))	/* spool directory */
		    || (!strcmp ("tf", tab[i].name))	/* troff filter */
		    || (!strcmp ("vf", tab[i].name))	/* raster filter */
		    || (!strcmp ("xf", tab[i].name)))	/* passthru filter */
		{
		    if (value[0] != '/') 
		    {
			printf("\nSorry, the value of symbol '%s' must begin with '/'.\n",tab[i].name);
			retval=BAD;
		    }
		    break;
		}

		/* check arguments to postscript options */

		if (!strcmp("It", tab[i].name))
			if (check_arg(value, as_input_trays, dummy) != 0)
			{
				invalid_arg("It", as_input_trays);
				retval=BAD;
			}
		if (!strcmp("Ml", tab[i].name))
			if (check_arg(value, as_messages, dummy) != 0)
			{
				invalid_arg("Ml", as_messages);
				retval=BAD;
			}
		if (!strcmp("Or", tab[i].name))
			if (check_arg(value, as_orientations, dummy) != 0)
			{
				invalid_arg("Or", as_orientations);
				retval=BAD;
			}
		if (!strcmp("Ot", tab[i].name))
			if (check_arg(value, as_output_trays, dummy) != 0)
			{
				invalid_arg("Ot", as_output_trays);
				retval=BAD;
			}
		if (!strcmp("Ps", tab[i].name))
			if (check_arg(value, as_page_sizes, dummy) != 0)
			{
				invalid_arg("Ps", as_page_sizes);
				retval=BAD;
			}
		if (!strcmp("Sd", tab[i].name))
			if (check_arg(value, as_page_sizes, dummy) != 0)
			{
				invalid_arg("Sd", as_page_sizes);
				retval=BAD;
			}
		if (!strcmp("Si", tab[i].name))
			if (check_arg(value, as_sides, dummy) != 0)
			{
				invalid_arg("Si", as_sides);
				retval=BAD;
			}
		if (!strcmp("Ss", tab[i].name))
			if (check_arg(value, as_page_sizes, dummy) != 0)
			{
				invalid_arg("Ss", as_page_sizes);
				retval=BAD;
			}
	    	break;

	    default: 
		printf ("lprsetup: bad type %d for symbol %s.\n",tab[i].stype,tab[i].name);
		retval=BAD;
	}
	return(retval);
}

/*****************************************************
* Read a line and return the symbol; only knows about
* quit and help, but none of the other commands.
*****************************************************/
getsymbol ()
{
    extern struct cmdtyp cmdtyp[];
    extern char symbolname[];

    int     i, length, retval;
    register char  *q;
    char    line[BUF_LINE];	/* input line */
    char    line2[BUF_LINE];	/* saved version of the input line */

    if (fgets (line, BUF_LINE, stdin) == NULL) {
	printf ("\n");	/* EOF (^D) */
	return (QUIT);
    }
    if (line[0] == '\n') {
	return (NOREPLY);
    }
    if (line[strlen (line) - 1] == '\n') {
	line[strlen (line) - 1] = NULL;
    }
    for (q = line; isspace (*q); q++) {
	; /* nop */
    }

    strcpy(line2, line);	/* save original entry, including caps */
    MapLowerCase(line);

    length = strlen(line);
    for (i = 0; cmdtyp[i].cmd_name; i++) {
	if (strncmp(cmdtyp[i].cmd_name, line, length) == 0) {
	    retval = cmdtyp[i].cmd_id;
	    break;
	}
    }

    strcpy(symbolname, line2);	/* save symbol name globaly */
    if ((strcmp(symbolname, "help") == 0)
	|| (strcmp(symbolname, "quit") == 0)
	|| (strcmp(symbolname, "?") == 0)
        || (strcmp(symbolname, "printer?") == 0)
        || (strcmp(symbolname, "no") == 0)
        || (strcmp(symbolname, "none") == 0)){
	return(retval);		/* return command id only if quit or help */
    } else {
	return (GOT_SYMBOL);	/* else return a symbol */
    }
}

/*****************************************************
* Read a line, decode the command, return command id
* This routine knows about the full command set.
* DJG#2
*****************************************************/
getcmd ()
{
    extern struct cmdtyp cmdtyp[];
    extern char symbolname[];

    int     i, length;
    register char  *q;
    char    line[BUF_LINE];	/* input line */
    char    tline[BUF_LINE];	/* tmp copy of input line */

    if (fgets (line, BUF_LINE, stdin) == NULL)
    {				/* EOF (^D) */
	printf ("\n");
	return (QUIT);
    }

    if (line[0] == '\n')
	return (NOREPLY);

    if (line[strlen (line) - 1] == '\n')
	line[strlen (line) - 1] = NULL;

    for (q = line; isspace (*q); q++)/* strip leading blanks */
	;
    strcpy (tline, line);   /* DJG#2 */
    MapLowerCase (line);

    length = strlen (line);
    for (i = 0; cmdtyp[i].cmd_name; i++)
	if (strncmp (cmdtyp[i].cmd_name, line, length) == 0)
	    return (cmdtyp[i].cmd_id);/* command id */

    strcpy (symbolname, tline);	/* save symbol name globaly DJG#2 */
    return (GOT_SYMBOL);
}
/************************************************************
* Read a line, decode the printer type, return printer id
* This routine knows about all supported printers 
*************************************************************/
getprnttype ()
{
    extern struct prnttyp prnttyp[];
    extern char printertype[];

    int     i, length;
    register char  *q;
    char    line[BUF_LINE];	/* input line */

    if (fgets (line, BUF_LINE, stdin) == NULL)
    {				/* EOF (^D) */
	printf ("\n");
	return (PTQUIT);
    }

    if (line[0] == '\n')
	return (PTNOREPLY);

    if (line[strlen (line) - 1] == '\n')
	line[strlen (line) - 1] = NULL;

    for (q = line; isspace (*q); q++)/* strip leading blanks */
	;
    MapLowerCase (line);

    length = strlen (line);
    for (i = 0; prnttyp[i].prnt_nam; i++)
	if (strncmp (prnttyp[i].prnt_nam, line, length) == 0)
	    return (prnttyp[i].prnt_id);/* printer id */

    strcpy (printertype, line);	/* save printer name globaly */
    return (NOT_SUPPORTED);
}

/************************************************************/

leave (status)
int     status;
{
    exit (status);
}

/**************************
*  free nvalue when done
**************************/
freemem ()
{
	extern struct table tab[];
	int i;

	for (i = 0; tab[i].name != 0; ++i)
	if (tab[i].nvalue > 0)
		free (tab[i].nvalue);
}

/*
 *	finish printer setup
 *	notify user of success
 */
setupdone()
{
	printf("\nSet up activity is complete for this printer.\n");
	printf("Verify that the printer works properly by using\n");
	printf("the lpr(1) command to send files to the printer.\n");

	return;
}

/****************************************
* edit the ttys file and change the line
* to an appropriate setting: 
*	"off" for adding/modifying a printer
* 	"on"  for deleting a printer
****************************************/
fixtty(line,mode)
char *line;	/* */
int mode;	/* 0 == off, 1 == on */
{
	FILE *fp;

	fp = Popen(EDTTY, "w");
	if (fp == NULL) {
		perror("popen(%s)\n", EDTTY);
		fprintf(stderr, "\n/etc/ttys not edited (could not change line %s to %s).\n", line, mode ? "on" : "off");
		return(OK);
	} 
	if (mode == 0) {
		dprintf(stderr, 
		"/^%s[ 	]/s/[ 	]on[ 	]/	off /\n", line);
		fprintf(fp, 
		"/^%s[ 	]/s/[ 	]on[ 	]/	off /\n", line);
	} else {
		dprintf(stderr, 
		"/^%s[ 	]/s/[ 	]off[ 	]/	on /\n", line);
		fprintf(fp, 
		"/^%s[ 	]/s/[ 	]off[ 	]/	on /\n", line);
	}
	fprintf(fp, "w\n");
	fprintf(fp, "q\n");
	fflush(fp);
	pclose(fp);
	kill(1, SIGHUP);	/* send hangup to init */
	return(OK);
}

FILE*	
Popen(s1, s2)
char *s1, *s2;
{
	FILE	*fp;
	close(1);
	fp = popen(s1, s2);
	dup2(3, 1);
	return(fp);
}

/**********************************
* Determines if pname is in the buf
* needed by DeleteEntry
***********************************/

findpname (pname, buf)

char  pname[], buf[];

{
    int   i,j,done;
    char  *c;

    i = strlen(pname);

    if ((strncmp (pname, buf, i) == 0) && ((buf[i] == ':') || (buf[i] == '|'))) {
	return (TRUE);
	}
    else {
        j = i;
        done = FALSE;
        while (!done) {
           while ((buf[j] != '|') && (buf[j] != ':') && (buf[j] != ' ')) j++;
              if ((buf[j] != ':') && (buf[j] != ' ')) {
                 j++;
                 c = &buf[j];
                 if ((strncmp (pname, c, i) == 0) 
                    && ((buf[i+j] == '|') || (buf[i+j] == ':'))) {
                    return (TRUE);
                 }
              }
              else {
                return (FALSE);
              }
           }
         }
}

    
/*
 * PrintHelp - Prints the requested help information,
 *             it stops every 23 lines.  DJG#3
 */

int
PrintHelp (hlpmsg)

char	*hlpmsg;

{
    int    done, pos, linecnt;
    char   line[BUFLEN];

    pos = 0;
    done = FALSE;
    linecnt = 0;

    GetRows();    /* Do it every time in case window is resized  DJG#4 */
    while (sgetline (hlpmsg, line, &pos) && !done) {
	linecnt ++;
	printf ("%s\n", line);
	if (linecnt == rows) {
	    printf ("\nPress 'RETURN' to continue or 'quit RETURN' to quit: ");
            switch (getcmd()) {
               case QUIT:
               case GOT_SYMBOL:
                    done = TRUE;
                    break;
               case NOREPLY:
               default:
                    linecnt = 0;
                    break;
	    }
	}
    }
}

/*
 * sgetline - returns in line the next line in buf, reads until a \n is found.
 * the \n is changed to a \0. Returns true if a line can be read. False if not.
 * DJG#3
 */

int
sgetline (buf, str, pos)

char	*buf, *str;
int	*pos;

{
    int	  i;

    if (buf[*pos] == '\0') {
	str[0] = '\0';
	return (0);
    }
    else {
	for (i = 0; (str[i] = buf[*pos]) != '\n'; i++, (*pos)++);
	str[i] = '\0';
	(*pos)++;
	return (1);
    }
}

/*
 * GetRows - sets the global variable "rows" to contain the number of rows in
 *           the output window.
 *           DJG#4
 */

int
GetRows ()
{
    struct winsize win;

    rows = DEFAULTROWS;
    if (ioctl(fileno(stdout), TIOCGWINSZ, &win) != -1) {
	if (win.ws_row > 0)
	    rows = win.ws_row - 2;
	else
	    rows = DEFAULTROWS;
    }
}

/******************************************************************************
* end of misc.c
******************************************************************************/
