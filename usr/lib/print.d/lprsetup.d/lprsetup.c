#ifndef lint
static char *sccsid = "@(#)lprsetup.c	4.6      ULTRIX 	3/13/91";
#endif

/************************************************************************
 *   Copyright (c) Digital Equipment Corporation 1984, 1985, 1986, 1987 *
 *   1988, 1989. 1991  All Rights Reserved. 			                *
 ************************************************************************/

/************************************************************
*
*	ULTRIX-32 Printer Installation/Setup  Program
*
*	This program helps system administrators to set up
*	printers for their system.  It guides through the
*	steps in setting up /etc/printcap, makes all of the
*	necessary files and directories for each printer,
*	and insures everything necessary for successful
*	printer operation was specified.
*
************************************************************/
/*
 *	based on lprsetup program developed by John Dustin for ULTRIX-11
 * Modification history:
 *
 *  06-Mar-91	Adrian Thoms
 *	Fixed code which adds comments to /etc/printcap
 *
 *  04-Mar-91	Adrian Thoms
 *	Replaced calls to chmod, chown, unlink, rmdir with calls to safe*
 *	Renamed LinkFilter to DisableGetty,  Added more comments.
 *
 *  03-Mar-91	Adrian Thoms
 *	Fixed bug whereby SetVal was ignoring request to remove a
 *	parameter AND supplying bogus value pointed to by val which is
 *	that left by the previous SetVal call
 *
 *  03-Mar-91	Adrian Thoms
 *	Fix modify code so that spool dir. errlog and account files are
 *	deleted/created with correct logic
 *	Made UnLinkSymFile prompt system manager so that file deletions
 *	are optional
 *
 *  27-Feb-91	Adrian Thoms
 *	Only create spool directory, account and log file if they are
 *	specified in the printcap entry
 *
 *  26-Feb-91	Adrian Thoms
 *	Removed unnecessary code related to chowning and chmoding ttys
 *	Removed edit to /etc/ttys done during queue deletion
 *
 *  04-Oct-90	Adrian Thoms
 *	New feature so that baud is only prompted for with serial devices.
 *	Also "br", "fc", "fs", "xc", and "xs" are only included in the
 *	entry for serial devices
 *
 *  06-Mar-90   Adrian Thoms (thoms)
 *	Fixed null pointer problem in LAT support.
 *	Tidied up LAT setup code to agree with latest lpd LAT code.
 *
 *  07-Nov-89   Daren Seymour (EUEG)
 *    Added LAT support.
 *
 *  21-Jan-89  David Gray (gray)
 *
 *    DJG#12 - Changed ViewPrintcap to call GetRows to determine current
 *    size of screen or window, PrintHelp uses the same routine.
 *
 *    DJG#11 - Changed the way printer synonyms are searched for when
 *    replacing a modified printcap entry into the printcap file. The
 *    previous version only looked at the first synonym, this version
 *    looks at all of them.
 *
 *  19-Jan-89  David Gray (gray)
 *
 *    DJG#10 - Created a new routine called UnLinkSymFile whichs 
 *    unlinks the associated file for the specified printcap parameter.
 *    Two in particular are the "af" accounting, and "lf" error log files.
 *
 *    Help information is now printed with the PrintHelp function
 *    which is found in misc.c, PrintHelp determines the current size
 *    of the window or screen and stops when it is full, sort of like
 *    "more". This prevents lengthy descriptions from scrolling off screen
 *
 *  07-Jan-89  David Gray (gray)
 *
 *    DJG#8 - Stops prompting for synonyms when a synonym is found
 *    to contain blanks or tabs, ensures all synonyms are usable.
 *
 *    DJG#6 - Changed ModifyEntry to allow the spooling directory to
 *    be modified, and is correctly handled with fix DJG#5, also changed
 *    code to correctlt handle changes for the named accounting file and
 *    error log file.
 *
 *    DJG#5 - Changed UnlinkSpooler to remove all files in the spooling
 *    directory prior to deleting the directory itself.
 *
 *    DJG#4 - Because of an earlier change (2/17/88), modify still
 *    assumed the printer name entered was a number and thus assumed
 *    that the actual printer name to modify was lp[num]. Now modify
 *    uses the name actually typed by the user.
 *
 *    DJG#3 - Modified code to prevent ambiguous printer types from
 *    being selected, must specify a unique name, ie., lp could match
 *    lp26, lp27 or lp29. Now full printer type must be entered.
 *    Abreviations are no longer accepted.
 *
 *    DJG#2 - Changed the way the lowest printer number is found and
 *    increased the number of possible printers from 100 to as many
 *    as you want. It performs more checking on printer names to prevent
 *    duplicate entries from occurring.
 *    
 *    DJG#1 - When spooling directories, accounting files or
 *    error log files are created, any necessary intermediate
 *    directories along the path are created first.
 *
 *  05-Jan-88  David Gray (gray)
 *    General Change: voided all sprintf's
 *
 *  11-Apr-88  David Gray (gray)
 *    Modified the GetSymbol routine in misc.c to ignore default
 *    values that are equal to "none". Added line 504.
 *
 *  17-Feb-88  David Gray (gray)
 *    Made changes that allow printers to be identified by
 *    any alphanumeric name. Added additional help information:
 *    better explanations of printcap parameters, specific
 *    information on supported printers, more info on how 
 *    accounting information is maintained. Added a new command
 *    called view which allows /etc/printcap file to be viewed
 *    while in lprsetup. Added ability to add comments to 
 *    /etc/printcap while running lprsetup.
 *
 *  26-Aug-87  chetal, Pradeep
 *    Made changes so that we insert the necessary lines for
 *    "remote" printers in the /etc/printcap. (:lp=:\)
 *
 *  8-Jul-88  Dave Maxwell (EUEG)
 *    Addded lps40 support
 *
 *  20-Jul-88  Dave Maxwell (EUEG)
 *    changed lps40 to printserver
 *    changed strncmp to strcmp so printer types must be entered in full
 *    (this prevents ambiguous types e.g. 'lp' and ensures special cases
 *    e.g. 'remote' to be picked up later in program
 *
 *	
 **********************************************************/
#include <stdio.h>
#include <sys/types.h>
#include <sys/dir.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/errno.h>
#include <lat/lat.h>
#include "lprsetup.h"
#include "globals.h"

long time();
extern int errno;

/* debug */
int debug = 0;

main (argc, argv)
int argc;
char *argv;
{
    int	i;


    if (argc != 1){
    	fprintf (stderr, "\nusage: %s\n", progname);
    	exit (1);
    }

    /**********************
    * check prerequisites
    **********************/
    if (getuid () != 0 && !ROOTUSER)
    {
#ifndef LOCAL
	printf ("\n%s: must be superuser!\n\n", progname);
	leave (ERROR);
#endif
    }

    /* for Popen later (in misc.c) */
    for (i = 3; i < 20; i++)
	close(i);
    dup2(1, 3);
    if (fopen (PRINTCAP, "r") == NULL)
    {
	perror (PRINTCAP);
	leave (ERROR);
  
    }


    printf ("\nULTRIX Printer Setup Program\n");

    /******************
    * loop until done
    ******************/
    for (;;)
    {
        /********************************
        *  clear changable table values
        ********************************/
        for (i = 0; tab[i].name != 0; ++i)
        {
	    tab[i].used = NO;
	    tab[i].nvalue = 0;
        }
	/*
	 * clear modify flag after each time
	 * through, used to keep track of when to
	 * link the filter(s).
	 */
	modifying = FALSE;

	printf ("\nCommand  < add modify delete exit view quit help >: ");

	strcpy(pname, "");
	strcpy(pnum, "");

	switch (getcmd ())
	{
	    case ADD:
		DoAdd ();
		break;
	    case MODIFY:
		DoModify ();
		break;
	    case DELETE:
		DoDelete ();
		break;
	    case QUIT:
		printf("\n");
		leave (OK);
		break;
	    case HELP:
		PrintHelp (h_help);
		break;
            case VIEW:
                ViewPrintcap ();
	    case NOREPLY:
		break;
	    default:
		printf ("\nSorry, invalid choice.  Type '?' for help.\n");
		break;
	}
	freemem ();
    }
}

/*****************************************
*  add new entry to printcap and create
*  corresponding special device
*****************************************/
DoAdd ()
{
    int     done;
    int     status, printerNumber;

    /* Starting at 0 find the lowest printer number in
     * the printcap data base, this number will be used
     * for default tty's and standard file names. If
     * the number is 0, and is selected by the user then
     * the printer is assumed to be the default printer.
     * DJG#2
     */
    
    printerNumber = 0;
    printerNumber = GetLowestNumber (printerNumber);
    if (printerNumber == 0) {
        /* Check to see if "lp" currently exists */
        status = pgetent (bp, "lp");
        if (status == -1)
    	    badfile (PRINTCAP);
        else if (status == 1)  /* it does exist */
	    /* get next lowest number */
    	    printerNumber = GetLowestNumber (printerNumber + 1);
    }
    (void) sprintf (pnum, "%d", printerNumber);
    
    printf("\nAdding printer entry, type '?' for help.\n");
 
    done = 0;
    while (! done) {
        printf ("\nEnter printer name to add [%s] : ", pnum);
	switch (getcmd ())
	{
	    case NOREPLY:
		strcpy(symbolname, pnum); /* he entered the default printer # */
		/* no break! ...falls through to case GOT_SYMBOL: */
	    case GOT_SYMBOL:
		status = pgetent (bp, symbolname);
		if (status == -1) {
                    badfile(PRINTCAP);
	        }
		if (status == 1)
                    printf("\nSorry, printer '%s' already exists.\n", symbolname);
	         else
                    done = TRUE;
		break;
	    case QUIT:
		return (QUIT);
		break;
	    case HELP:
		PrintHelp (h_doadd);
		break;
	    default:
		printf ("\nInvalid choice.  Type '?' for help.\n");
		break;
	}
    }

    strcpy(pname, "");

    if (strcmp (pnum, symbolname) == 0) {       /* DJG#2 */
	if (strcmp (pnum, "0") == 0)
	    (void) sprintf (pname, "lp|lp0|0");
	else
	    (void) sprintf (pname, "lp%s|%s", pnum, pnum);
    }
    else { /* not default number                   DJG#2 */
	(void) sprintf(pname, "%s|lp%s", symbolname, pnum);
    }

    if (AddField () != QUIT)
    {
        AddComments ();
	if (AddEntry () == ERROR)
	    printf("\nError in adding printcap entry, try again.\n");
	else
	if (AddDevice () == ERROR)
	    printf("\nError in associating printer files/directories.\n");
    }
    freemem ();
    printf("\n");
    return (OK);
}

/***********************************
*  modify existing printcap entry
************************************/
DoModify ()
{
    int     done = FALSE;
    int     status;

    printf("\nModifying a printer entry, type '?' for help.\n");
    while (!done)
    {
	modifying = TRUE;

	strcpy(pnum, "");
	strcpy(pname, "");
	strcpy(longname, "");	/* in case of previous modify */

	printf ("\nEnter printer name to modify (or view to view printcap file): ");

	switch (getcmd ())
	{
	    case GOT_SYMBOL:
                 status = pgetent (bp, symbolname);
                 if (status == -1) {
	             badfile(PRINTCAP);
                 }
                 if (status == 0)
	             printf ("\nSorry, printer number '%s' is not in %s.\n", symbolname, PRINTCAP);
                 else
                 {
	             strcpy(pnum, symbolname);
	             strcpy(pname, symbolname);   /* DJG#4 */
	             ModifyEntry();
	             done = TRUE;	/* get back to main menu */
                 }
		break;
	    case VIEW:
                ViewPrintcap ();
		break;
	    case QUIT:
		done = TRUE;
		break;
	    case HELP:
		PrintHelp (h_domod);
		break;
	    case NOREPLY:
		break;
	    default:
		printf ("\nInvalid choice, try again.\n");
		break;
	}
    }
    freemem();
    return (OK);
}

/***********************************
*  delete existing printcap entry
************************************/
DoDelete ()
{
    int     done = FALSE;
    int     status;
	int yn;

    printf ("\nDeleting a printer entry, type '?' for help.\n");
    while (!done)
    {
	strcpy(pnum, "");
	strcpy(pname, "");

	printf ("\nEnter printer name to delete (or view to view printcap file): ");

	switch (getcmd ())
	{
	    case GOT_SYMBOL:
                status = pgetent (bp, symbolname);
                if (status == -1) {
	            badfile(PRINTCAP);
                }
                if (status == 0)
	            printf ("\nCannot delete printer %s, entry not found.\n", symbolname);
                else
                {
	            strcpy(pnum, symbolname);
	            (void) sprintf(pname, "%s", pnum);
	            done = TRUE;
                }
		break;
	    case VIEW:
                ViewPrintcap ();
		break;
	    case QUIT:
		return (QUIT);
		break;
	    case HELP:
		PrintHelp (h_dodel);
		break;
	    case NOREPLY:
		break;
	    default:
		printf ("\nInvalid choice, try again.\n");
	}
    }

    /*********************************
    *  read printcap into tab for
    *  final confirmation
    **********************************/
    CopyEntry ();

    Print (USED);
    printf ("\nDelete %s, are you sure? [n]  ", pname);

	yn = 'n';
    if (YesNo (yn) == TRUE) {
	DeleteEntry ();
    }
    else {
	printf ("\n%s not deleted.\n", pname);
    }

    freemem ();
    printf("\n");
    return (OK);
}

/*******************************************
* Find and return the lowest printer number
* in the printcap data base starting from
* the number "startingNumber".   DJG#2
********************************************/
int
GetLowestNumber (startingNumber)

int	startingNumber;
{
	char	pn[ALPHANUMLEN];    /* printer number string */
	char	lppn[ALPHANUMLEN];  /* lp name		 */
	int	pnum_done;      /* loop flag             */
	int	status;         /* return for printcap query */
	int     printerNumber;  /* lowest number found   */

	printerNumber = startingNumber;
	pnum_done = 0;
	while (! pnum_done) {
		(void) sprintf (pn, "%d", printerNumber);
		status = pgetent (bp, pn);
		if (status == -1)
			badfile (PRINTCAP);
		else if (status == 1)
			printerNumber ++;
		else {
			(void) sprintf (lppn, "lp%d", printerNumber);
			if (pgetent (bp, lppn) == 1)
				printerNumber ++;
			else
				pnum_done ++;
		}
	}
	return (printerNumber);
}
			
/*********************************
*  Add comments to printcap file
*  for printer added by DoAdd
**********************************/
AddComments ()

{
     int    done, leave;

     printf ("\nAdding comments to printcap file for new printer, type '?' for help.\n");
     done = FALSE;
     leave = FALSE;
     strcpy (symbolname, "n");
     printf ("Do you want to add comments to the printcap file [%s] ? : ", symbolname);
     while (!done) {
         switch (getcmd()) {
             case NOREPLY:
             case NO:
                  done = TRUE;
                  leave = TRUE;
                  break;
             case YES:
             case GOT_SYMBOL:
                  done = TRUE;
                  break;
             case QUIT:
                  done = TRUE;
                  leave = TRUE;
                  break;
             case HELP:
                  PrintHelp(h_addcmnts);
                  strcpy (symbolname, "n");
                  printf ("Do you want to add comments to the printcap file [%s] ? : ", symbolname);
                  break;
             default:
                  strcpy (symbolname, "n");
                  printf ("Do you want to add comments to the printcap file [%s] ? : ", symbolname);
                  break;
           }
     }
     if (!leave) {
        printf ("Enter comments below - Press RETURN on empty line to exit\n\n");
        done = FALSE;
        for (numcomments = 0; !done && numcomments < LINES; ) {
           printf ("# ");
           switch (getcmd()) {
	      case NOREPLY:
	      case QUIT:
                   done = TRUE;
                   break;
              case HELP:
                   PrintHelp (h_addcmnts);
                   break;
              default:
                   sprintf(printercomments[numcomments++],
			   "# %.*s", COLUMNS-3, symbolname);
                   break;
           }
        }
     }
}

                

/**************************
*  get fields for DoAdd
**************************/
AddField ()
{
    char    buf[LEN];		/* temp buffer		 */
    int     done;		/* flag			 */
    int     i,j;		/* temp index		 */
    struct stat st;
    char *valu;
    int yn;
    struct table *lp_entry=NULL;

    /********************************
    *  clear changeable tab values
    ********************************/
    for (i = 0; tab[i].name != 0; ++i)
    {
	tab[i].used = NO;
	tab[i].nvalue = 0;
    }

    if (MatchPrinter () == QUIT)
	return (QUIT);

    if (AddSyn () == QUIT)
	return (QUIT);

    if ((strcmp (ptype, "remote") !=0) && (strcmp(ptype, "printserver") != 0)) {
	do {
	    printf ("\nSet device pathname 'lp'");
	    if ((pnum[0] != '\0') && (pnum[1] == '\0'))
                (void) sprintf (buf, "%s%s", "/dev/tty0", pnum);
            else
                (void) sprintf (buf, "%s%s", "/dev/tty", pnum);
	} while (SetVal ("lp", buf) < 0);
	
	do {
	    printf ("\nSet accounting file 'af'");
            if (pnum[0] == '0' )
                (void) sprintf (buf, "%s", "/usr/adm/lpacct");
            else
                (void) sprintf (buf, "%s%s%s", "/usr/adm/lp", pnum, "acct");
	} while (SetVal ("af", buf) < 0);
    }

    /* Spooling directory is set for ALL printers, including 'remote' */

    do {
        printf ("\nSet spooler directory 'sd'");
        if (pnum[0] == '0' )
            (void) sprintf (buf, "%s", "/usr/spool/lpd");
        else
            (void) sprintf (buf, "%s%s", "/usr/spool/lpd", pnum);
    } while (SetVal ("sd", buf) < 0);

    if (strcmp (ptype, "remote") != 0)
    {
        do {
            printf ("\nSet printer error log file 'lf'");
            if (pnum[0] == '0' )
                (void) sprintf (buf, "/usr/adm/lperr");
            else
                (void) sprintf (buf, "/usr/adm/lp%serr", pnum);
        } while (SetVal ("lf", buf) < 0);
    }
    for ( i = 0; tab[i].name; ++i ) {
	if ( strcmp (tab[i].name, "lp" ) == 0 ) {
	    lp_entry = &tab[i];
	    break;
	}
    }
    if (!lp_entry) {
	printf("\nInternal Error, no \"lp\" entry");
	return(QUIT);
    }
    if (lp_entry->used == YES && lp_entry->nvalue != NULL) {
	if (strlen(lp_entry->nvalue) > 8 &&
	    strncmp(lp_entry->nvalue, "/dev/tty", 8) == 0) {
	    /*
	     * Serial printer connection
	     */
	    do {
		printf("\nSet printer connection type 'ct'");
		(void) sprintf(buf, "%s", "dev");
	    } while (SetVal("ct", buf) < 0);

	    if (strcmp (buf, "lat") == 0) {
		if (stat(lp_entry->nvalue, &st) == 0
		    && major(st.st_rdev) == LAT_MAJORDEV) {
		    int k;
		    int OP_or_OS_set=0;

		    for (k=0; k < 2; k++) {
			if (k)
			printf("\nYou must enter a value for 'ts'");

			do {
			    printf("\nSet terminal server name 'ts'");
			    (void) sprintf(buf, "");
			} while (SetVal ("ts", buf) < 0);

			if (buf[0] != '\0') break;
		    }

		    for (k=0; !OP_or_OS_set && k < 2; k++) {
			if (k)
			printf("\nYou must enter a value for either 'op' or 'os'\n");
			do {
			    printf("\nSet terminal server output port 'op'");
			    (void) sprintf(buf, "");
			} while (SetVal ("op", buf) < 0);
			if (buf[0] != '\0') OP_or_OS_set++;
			do {
			    printf("\nSet terminal server output service 'os'");
			    (void) sprintf(buf, "");
			} while (SetVal ("os", buf) < 0);
			if (buf[0] != '\0') OP_or_OS_set++;
		    }

		} else {

		    printf("\nThe tty you are currently using is not a LAT configured tty. \n\
You must configure a tty for LAT use, see MAKEDEV(8) and lcp(8) \n\
for more details.");

		    return(QUIT);	

		}
	    } else {
		/*
		 * if it's a serial printer not parallel
		 * include baud rate
		 */
		do {
		    printf ("\nSet printer baud rate 'br'");
		    (void) sprintf (buf, "%s", "9600");
		} while (SetVal ("br", buf) < 0);
	    }
	} else {
	    /*
	     * lp field does not appear to be a tty device
	     * Don't use the tty ioctl fields
	     */
	    static char *not_used_tab[] = { "br", "fc", "fs", "xc", "xs", 0 };
	    register char **p;

	    for (p = not_used_tab; *p; p++) {
		for ( i = 0; tab[i].name; ++i ) {
		    if ( strcmp (tab[i].name, *p) == 0 ) {
			tab[i].used = NO;
			break;
		    }
		}
	    }
	}
    }
    if (strcmp (ptype, "printserver") == 0) 
    {
	do {
	   printf("\nSet printserver output filter 'of'\n\
retype default exactly with NODE replaced by printserver node name\n");
	   printf("\nFor a printserver running TCP/IP, use 'iplpscomm' instead of 'lpscomm'\n");
	   sprintf(buf, "%s", "");
	} while  (SetVal ("of", buf) < 0);
    }

    if (strcmp (ptype, "remote") == 0) 
    {
	/*
	 * It is remote but we still have to add
	 *     :lp=:\
	 * in the /etc/printcap file.
	 */
	/* Find index for 'lp' */
	for ( i = 0; tab[i].name; ++i )
	  if ( strcmp(tab[i].name,"lp") == 0 )
	    break;
	tab[i].used = YES;
	*tab[i].svalue = 0;  /* make lp="" for the printcap */
	do {
            printf ("\nSet remote system name 'rm'");
		(void) sprintf(buf,"");
        } while (SetVal ("rm", buf) < 0);
	do {
            printf ("\nSet remote system printer name 'rp'");
		(void) sprintf(buf,"");
        } while (SetVal ("rp", buf) < 0);
    }

    /*********************************
    *  modify default field values
    *********************************/
    printf (h_symsel);
    for (i = 0,j=0; tab[i].name != 0; ++i){
        printf (" %s ", tab[i].name);
	if (j++ > 14){
    		printf ("\n");
		j = 0;
	}
    }
    printf ("\n");

    done = FALSE;
    for (;;)
    {
	while (!done)
	{
		printf ("\nEnter symbol name: ");
		switch (getcmd ())
		{
		    case GOT_SYMBOL:
			DoSymbol ();	/* Don't have to special case sd, the
					 * spooling directory since the entry
					 * is being added. It still can be
					 * changed since it doesn't exist yet.
					 */
			break;
		    case HELP:
			PrintHelp (h_symsel);
    			for (i = 0,j=0; tab[i].name != 0; ++i){
        			printf (" %s ", tab[i].name);
				if (j++ > 14){
    					printf ("\n");
					j = 0;
				}
    			}
			printf ("\n");
			break;
		    case NOREPLY:
			break;
		    case PRINT:
			Print (USED);
			break;
		    case LIST:
			Print (ALL);
			break;
		    case QUIT:
			done = TRUE;
			break;
		    default:
			printf ("\nInvalid choice, try again.\n");
			break;
		}
	}
	if (Verified () == TRUE)
	    break;
	else
	{
	    printf("Do you wish to continue with this entry?  [y] ");
		yn = 'y';
	    if (YesNo (yn) == FALSE) {
		return(QUIT);		/* no, they wish to abort */
	    }
	    else
	        done = FALSE;
	}
    }
    return (OK);
}

/****************************************************
*  find matching printer by name and copy over fields
*  loop through default table, find match and assign.
*****************************************************/
MatchPrinter ()
{
    int     found, done;	/* flags	*/
    int     length;		/* strlen return */
    char    *addr;		/* malloc return */
    int     i, j, k;		/* temp indices	 */
    int yn;                     /* yes - no answer */
    int     gotprinter;         /* true if a printer type is found */

    /****************************
    *  get printer type
    ****************************/
    found = FALSE;
    while (!found)
    {
	done = FALSE;
	while (!done)
	{
           printf ("\nFor more information on the specific printer types\n");
	   printf ("Enter `printer?'\n");
	   printf ("\nEnter the FULL name of one of the following printer types:\n");
		    for (i = 0,j=0; printer[i].name != 0; j++,++i){
			printf ("%s ", printer[i].name);
			if (j > 12){
		    		printf ("\n");
				j=0;
			}
		    }
	    printf ("\nor press RETURN for [unknown] : ");

	    switch (getsymbol ())
	    {
                case PRINTER_INFO:
                     SpecificPrinter();
                     break;
		case GOT_SYMBOL:
		default:
		    strcpy (ptype, symbolname);
		    done = TRUE;
		    break;
		case HELP:
		    PrintHelp (h_type);
		    break;
		case QUIT:
		    return (QUIT);
		    break;
		case NOREPLY:
		    printf ("\nUsing 'unknown' for printer type, OK? [ n ] ");
			yn = 'n';
		    if (YesNo (yn) == TRUE) {
		        done = TRUE;
			strcpy (ptype, UNKNOWN);
		    }
		    break;
	    }
	}

	/****************************************************
	*  loop through printer table, find match   DJG#3
        *  Note: The array  printer[i].name ends with a NULL.
	*****************************************************/

        for (i = 0, gotprinter = 0; printer[i].name; i++) {
		if (strcmp (printer[i].name, ptype) == 0) {
			gotprinter ++;
			break;
		}
	}

	if (!gotprinter)
	{
	    printf ("\nDon't know about printer '%s'\n", ptype);
	    printf ("\nEnter 'y' to try again, or 'n' to use 'unknown' [y]: ");
		yn = 'y';
	    if (YesNo (yn) == FALSE)
	    {
		strcpy (ptype, UNKNOWN);
		found = TRUE;
	    } else {
		found = FALSE;
	    }
	}
	else { /* found a printer type */
	        found = TRUE;
	}
    }

    /**************************************************************
    * loop thru printer values and assign to corresponding
    * default new values - note: `i' contains correct printer index
    **************************************************************/

    for (j = 0; printer[i].entry[j].name; ++j)
    {
	/******************************
	*  loop through default table
	*******************************/

	for (k = 0; tab[k].name; ++k)
	{
	    if (strcmp (printer[i].entry[j].name, tab[k].name) == 0)
	    {
		length = strlen (printer[i].entry[j].svalue) + 1;
		if ((addr = (char *) malloc (length)) == NULL)
		{
		    printf ("\nmalloc: not enough space for symbols!\n");
		    return (ERROR);
		}
		tab[k].nvalue = addr;
		strcpy (tab[k].nvalue, printer[i].entry[j].svalue);
		tab[k].used = YES;
	    }
	}
    }
    return (OK);
}

/********************************************
* get help information on a specific printer
*********************************************/
SpecificPrinter()
{
    int   done;
    int   i, j;


    done = FALSE;

    printf ("\nEntering PRINTER TYPE help information MODE\n");
    printf ("\nEnter the printer type for specific information\n");
    printf ("               Enter '?' for help, 'quit' to exit : ");
    while (!done) {
      switch (getprnttype()) {
         case LA50:
              PrintHelp (h_la50);
              break;
         case LA75:
              PrintHelp (h_la75);
              break;
         case LA100:
              PrintHelp (h_la100);
              break;
         case LA120:
              PrintHelp (h_la120);
              break;
         case LA210:
              PrintHelp (h_la210);
              break;
         case LCG01:
              PrintHelp (h_lcg01);
              break;
         case LG01:
              PrintHelp (h_lg01);
              break;
         case LG02:
              PrintHelp (h_lg02);
              break;
         case LG31:
              PrintHelp (h_lg31);
              break;
         case LJ250:
              PrintHelp (h_lj250);
              break;
         case LN01:
              PrintHelp (h_ln01);
              break;
         case LN01S:
              PrintHelp (h_ln01s);
              break;
         case LN03:
              PrintHelp (h_ln03);
              break;
         case LN03S:
              PrintHelp (h_ln03s);
              break;
         case LN03R:
              PrintHelp (h_ln03r);
              break;
         case LP25:
              PrintHelp (h_lp25);
              break;
         case LP26:
              PrintHelp (h_lp26);
              break;
         case LP27:
              PrintHelp (h_lp27);
              break;
         case LP29:
              PrintHelp (h_lp29);
              break;
         case LQP02:
              PrintHelp (h_lqp02);
              break;
         case LQP03:
              PrintHelp (h_lqp03);
              break;
         case LVP16:
              PrintHelp (h_lvp16);
              break;
	 case PRINTSERVER:
	      PrintHelp (h_printserver);
	      break;
         case REMOTE:
              PrintHelp (h_remote);
              break;
         case NOTKNOWN:
              PrintHelp (h_unknown);
              break;
         case PTHELP:
              PrintHelp (h_printype);
              break;
         case PTQUIT:
              done = TRUE;
              break;
         case NOT_SUPPORTED:
              printf ("\n\nThe printer type %s is NOT SUPPORTED by DIGITAL - No Information\n\n", printertype);
              break;
         case PTNOREPLY:
         default:
              break;
        }
	if (!done) {
       		for (i = 0,j=0; printer[i].name != 0; j++,++i){
	   		printf ("%s ", printer[i].name);
	   		if (j > 12){
		   		printf ("\n");
		   		j=0;
	   		}
       		}
	}
	else {
		printf ("\n\n");
	}
       printf ("  unknown ");
       printf ("\nEnter the printer type for specific information\n");
       printf ("               Enter '?' for help, 'quit' to exit : ");
    }
    printf ("\nLeaving PRINTER TYPE help information MODE\n");
    printf ("\n\nReturning to Selection of Printer Type for ADDING a New Printer\n\n");
}

/*****************************
*  add synonyms
*****************************/
AddSyn ()
{
    int     done, status;

    done = FALSE;
    while (!done && strlen (pname) < BUFLEN)
    {
        printf ("\nEnter printer synonym: ");
	switch (getcmd ())
	{
	    case GOT_SYMBOL:
		status = pgetent (bp, symbolname);
		if (status == -1) {
			badfile(PRINTCAP);
		}
		if (status == 1)
		    printf ("\nSynonym is already in use, try something else.\n");
		else
		    if (strlen (pname) + strlen (symbolname) > (BUFLEN - 1))
			printf ("\nSynonym too long, truncating to %d characters.\n", BUFLEN);
		    else
		    {
			strcat (pname, "|");
			strcat (pname, symbolname);
			if ((strchr (symbolname, '\t') != NULL) || 
                            (strchr (symbolname, ' ') != NULL))    /* Then last synonym */
				return (TRUE);                     /* DJG#8 */
		    }
		break;
	    case HELP:
		PrintHelp (h_synonym);
		break;
	    case QUIT:
		return (TRUE);
		break;
	    case NOREPLY:
		return (TRUE);
		break;
	    default:
		printf ("\nInvalid choice, try again.\n");
		break;
	}
    }
    return(OK);
}

/*********************************
*  set default values
*  Returns -1 on error, 0 if ok
*********************************/
SetVal (val, buf)
char   *val;			/* two-letter symbol	 */
char   *buf;			/* preset value		 */
{
    int     i;			/* temp index		 */
    char    line[LEN];		/* temp buffer		 */

    /******************
    *  find val
    ******************/
    for (i = 0; tab[i].name; ++i)
	if (strcmp (tab[i].name, val) == 0)
	    break;
    if (tab[i].name == 0) {
	printf("internal error: cannot find symbol %s in table\n", val);
	return (BAD);
    }

    /*
     * Next assignment is needed for the case when the parameter was not set
     * in the template.
     * It is set here because the parameter may be switched off again in
     * the call to UseDefault.
     */
    tab[i].used = YES;

    (void) UseDefault (line, (tab[i].nvalue ? tab[i].nvalue : buf), i);

    /*
     * By ignoring the return value of UseDefault we may do a superfluous
     * copy if the default value was held in tab[i].nvalue.
     * The win is that we now have only one flow of control.
     */

    if (validate(i, line) < 0) {
	return(BAD);
    }
    if (tab[i].nvalue != NULL) {
	free(tab[i].nvalue);
	tab[i].nvalue = NULL;
    }
    if ((tab[i].nvalue = (char *) malloc (strlen (line) + 1)) == NULL) {
	printf ("\nmalloc: no space for %s\n", tab[i].name);
	return (ERROR);
    }
    strcpy (tab[i].nvalue, line);

    /* This is so caller can examine the chosen value */
    strcpy(buf, line);

    return(OK);
}

/*********************************************
* view current contents of printcap file
**********************************************/
ViewPrintcap ()
{
    FILE *pfp;
    int  cnt, done;
    char line[BUF_LINE];


    GetRows ();      /* determine current size of window or screen - DJG#12 */

    if ((pfp = fopen (PRINTCAP, "r")) == NULL ) {
        badfile (PRINTCAP);
    }
    cnt = 0;
    done = FALSE;
    while (!done) {
      cnt ++;
      if (fgets (line, BUF_LINE, pfp) == NULL) {
         done = TRUE;
      }
      if (!done) {
         if (cnt <= rows) {
            printf ("%s", line);
         }
         else {
	    printf ("\nPress 'RETURN' to continue or 'quit RETURN' to quit: ");
            switch (getcmd()) {
               case QUIT:
               case GOT_SYMBOL:
                    done = TRUE;
                    break;
               case NOREPLY:
               default:
                    printf ("%s", line);
                    cnt = 1;
                    break;
            }
         }
      }
    }
}

/**********************************************
*  write new printcap entry to printcap file
***********************************************/
AddEntry ()
{
    FILE *ofp, *lfp;		/* output and log file pointers */
    long     timeval;		/* time for log file		*/
    char     buf[LEN];		/* temp buffer			*/

    /****************************************
    *  open output and log files
    ****************************************/
    if ((ofp = fopen (PRINTCAP, "a")) == NULL) {
	badfile(PRINTCAP);
    }
    if ((lfp = fopen (LOGCAP, "a")) == NULL) {
	badfile(LOGCAP);
    }
    
    WriteComments (ofp);
    WriteEntry (ofp);

    /****************************************
    *  write time stamp and entry to log
    ****************************************/
    timeval = time(0);
    strcpy (buf, "\nAdded ");
    strcat (buf, ctime (&timeval));
    fputs (buf, lfp);

    WriteEntry (lfp);

    fclose (ofp);
    fclose (lfp);
    return (OK);
}

/***************************************
*  create special device, if necessary
***************************************/
AddDevice ()
{
    struct passwd  *passwd;	/* password file entry	 */
    char   *device;		/* parameter value ptr	 */
    int     mode;		/* chmod mode		 */
    int     i;			/* temp index		 */

    /*******************************
    *  get daemon id
    *******************************/
    if ((passwd = getpwnam (DAEMON)) == 0)
    {
	printf ("\ngetpwnam: cannot get id for %s\n", DAEMON);
	perror ();
	leave (ERROR);
    }

    /************************************
    *  chown and chmod device to daemon
    *
    *  **** removed for ULTRIX-32 ****
    ************************************/


    /*******************************
    *  create spooling directory
    *******************************/
    MakeSpool (passwd);

    /*******************************
    *  create accounting file
    *******************************/
    MakeAcct (passwd);

    /*******************************
    *  create errorlog file
    *******************************/
    MakeErrorlog (passwd);

    /***********************************
    *  inform user of completion
    ***********************************/
    DisableGetty();
    setupdone();
}

/*
 * UnLinkSpool - optionally unlink old spool directory when modifying entry
 *
 * Description:
 *	We use existing function UnLinkSpooler by faking up the entry
 *	We prompt system manager who may want to keep old spool directory
 */
static int
UnLinkSpool(tabent, oldpath)
    struct table *tabent;
    char *oldpath;
{
    char *newpath;
    int used;
    int yn;

    printf("\nChanging spool directory\n");
    printf("Do you want to delete the old spool directory? [n] ");

    yn = 'n';
    if (YesNo (yn) == TRUE) {

	/* save current values */
	newpath = tabent->nvalue;
	used = tabent->used;

	/* Fake up for deletion */
	tabent->nvalue = oldpath;
	tabent->used = YES;

	/* Use existing function to unlink spool directory */
	UnLinkSpooler();	/* uses value stored in tabent */

	/* restore current values */
	tabent->nvalue = newpath;
	tabent->used = used;
    }
}

/*
 * UnLinkFile - optionally unlink old account or log file
 *	when modifying printcap entry
 *
 * Description:
 *	We use existing function UnLinkSymFile by faking up the entry
 *	We prompt system manager who may want to keep the old files
 *	which may be shared with other queues.
 */
static int
UnLinkFile(tabent, oldpath)
    struct table *tabent;
    char *oldpath;
{
    char *newpath;
    int used;

    /* save current values */
    newpath = tabent->nvalue;
    used = tabent->used;

    /* Fake up for deletion */
    tabent->nvalue = oldpath;
    tabent->used = YES;

    /* Use existing function to unlink file */
    UnLinkSymFile(tabent->name);

    /* restore current values */
    tabent->nvalue = newpath;
    tabent->used = used;
}

/******************************
*  Modify selected entry
*  If the entry for sd, lf, or af is 
*  changed delete the old files - DJG#6
*
*	The old logic for deciding whether to delete old files
*	was fairly broken.
*	Since the logic for dealing with modifying sd, lf and af
*	is the same for each I have removed the triplication and
*	provided an enumeration and table of appropriate functions
*	to do the deletion and creation - 03-Mar-91 Adrian Thoms
*
*******************************/
ModifyEntry ()
{
    struct passwd *passwd;	/* passwd entry ptr		 */
    int     done;		/* flag				 */
    FILE    *ifp, *ofp;		/* input/ouput file pointers	 */
    char    keyname[LEN];	/* match name			 */
    char    buf[BUFLEN];	/* read/write buffer		 */
    char    oldvalue[MAXPATH];  /* old value of af, lf, or sd    */
    int	    i,j;		/* temp index			 */
    int     old_used;		/* were these previously in use */
    int     old_and_new_different;
    int yn;
    enum which_symbol_e {
	any_other_symbol,
	sd_symbol,
	lf_symbol,
	af_symbol
    } which_symbol;
    static struct delete_and_create_fns {
	int (*delete_fn)(), (*create_fn)();
    } fn_tab[] = {
	NULL,	NULL,
	UnLinkSpool, MakeSpool,
	UnLinkFile, MakeErrorlog,
	UnLinkFile, MakeAcct
    };

    /*******************************
    *  get daemon id
    *******************************/
    if ((passwd = getpwnam (DAEMON)) == 0)
    {
	printf ("\ngetpwnam: cannot find %s uid in passwd file.\n", DAEMON);
	leave (ERROR);
    }

    /**********************
    *  modify fields
    **********************/
    CopyEntry ();	/* affects longname */

    for (;;)
    {
	done = FALSE;
	printf("\nEnter the name of the symbol you wish to change.\n");
	printf("Enter 'p' to print the current values, 'l' to list\n");
        printf("all printcap values or 'q' to quit.\n");
	while (!done)
	{
	    old_used = NO;
	    old_and_new_different = 0;
	    which_symbol = any_other_symbol;

	    printf ("\nEnter symbol name:  ");
	    switch (getcmd ())
	    {
		case GOT_SYMBOL:
		    if ((strcmp("sd", symbolname) == 0 && (which_symbol = sd_symbol)) ||
			(strcmp("lf", symbolname) == 0 && (which_symbol = lf_symbol)) ||
			(strcmp("af", symbolname) == 0 && (which_symbol = af_symbol))) {
			for (i = 0; tab[i].name != 0; i++) {
			    if (strcmp (tab[i].name, symbolname) == 0) {
				if ((old_used = tab[i].used) == YES) {
				    strcpy (oldvalue,(tab[i].nvalue ? tab[i].nvalue : tab[i].svalue));
				}
				break;
			    }
                        }
			/*
			 * No point in processing symbol if it wasn't found
			 * in the table
			 */
			if (!tab[i].name) continue;
                    }

		    DoSymbol ();

                    /**** NOTE: i still points to the correct table entry ****/

		    switch(which_symbol) {
		      case sd_symbol:
		      case lf_symbol:
		      case af_symbol:
			if (old_used == YES && tab[i].used == YES) {
			    old_and_new_different = strcmp(oldvalue,
							   (tab[i].nvalue ? tab[i].nvalue : tab[i].svalue));
			}
			if (old_used == YES && (tab[i].used == NO || old_and_new_different)) {
			    fn_tab[(int)which_symbol].delete_fn(&tab[i], oldvalue);
			}
			if (!(old_used == YES && tab[i].used == YES &&
			      !old_and_new_different)) {
			    /*
			     * Always call create function unless there was no
			     * change. 
			     * This is inefficient, but makes use of
			     * warning message in MakeSpool if spool
			     * directory switched off.
			     */
			    fn_tab[(int)which_symbol].create_fn(passwd);
			}
			break;
		      case any_other_symbol:
		      default:
			break;
                    }
		    break;
		case HELP:
		    PrintHelp (h_symsel);
    			for (i = 0,j=0; tab[i].name != 0; ++i){
        			printf (" %s ", tab[i].name);
				if (j++ > 14){
    					printf ("\n");
					j = 0;
				}
    			}
		    printf ("\n");
		    break;
		case NOREPLY:
		    break;
		case PRINT:
		    Print (USED);
		    break;
		case LIST:
		    Print (ALL);
		    break;
		case QUIT:
		    done = TRUE;
		    break;
		default:
		    printf ("\nInvalid choice, try again.\n");
		    break;
	    }
	}
	if (Verified() == TRUE)
	    break;
	else
	{
	    printf("Do you wish to continue with this entry?  [y] ");
		yn = 'y';
	    if (YesNo (yn) == FALSE) {
		return(QUIT);	/* no, they wish to abort, although at this
			point, the return value (from here) is not checked.
			We just return early without actually do anything. */
	    }
	    else
	        done = FALSE;	/* not done yet */
	}
    }

    /**************************
    * save pname for match
    * and longname to rewrite
    * PNAME changed to PNUM 
    **************************/
    strcpy (keyname, pnum);	/* can search for any name, like "lp2" or "two" */
    strcpy (pname, longname);	/* new pname contains the entire first line */

    /******************************
    *  open original and copy
    *******************************/
    if ((ifp = fopen (PRINTCAP, "r")) == NULL) {
	badfile(PRINTCAP);
    }
    if ((ofp = fopen (COPYCAP, "w")) == NULL) {
	badfile(COPYCAP);
    }

    /*******************************************
    *  copy printcap to copy until entry  DJG#11
    ********************************************/
    while (fgets (buf, BUFLEN, ifp) != 0) {
	if (CommentOrEmpty (buf))
	    fputs (buf, ofp);
	else {
	    if (strpbrk (buf, ":|\\") != 0) {
		if (SynonymMatch (buf, keyname) == 0) { /* wrong entry so just copy */
		    fputs (buf, ofp);
		    while (fgets (buf, BUFLEN, ifp) != 0) {
			fputs (buf, ofp);
			if (buf[strlen(buf) - 2] != '\\')
			    break;
		    }
		}
		else { /* right entry so replace old entry with modified one */
		    while (fgets (buf, BUFLEN, ifp) != 0) {
			if (buf[strlen(buf) - 2] != '\\') {
			    WriteEntry (ofp);
			    break;
			}
		    }
		}
	    }
	    else /* not sure what it is, but we better copy it */
		fputs (buf, ofp);
	}
    }

    fclose (ofp);
    fclose (ifp);

    /***************************
    * mv new file to old file
    ***************************/
    if (rename (COPYCAP, PRINTCAP) < 0)
    {
	printf ("\nCannot rename %s to %s (errno = %d).\n",
		COPYCAP, PRINTCAP, errno);
	/* don't know what best to do here...*/
    }

    /***************************************
    * inform user of completion
    ***************************************/
    DisableGetty();
    setupdone();
}


/**********************************
*  delete existing printcap entry
**********************************/
DeleteEntry ()
{
    FILE *ifp, *ofp, *lfp;	/* input/output file pointers	*/
    char    buf[LEN];		/* read/write buffer		*/
    long    timeval;		/* time in seconds for logfile	*/
    char    tempfile[LEN];	/* file name buffer 		*/

    /*********************************
    *  open original, copy, and log
    **********************************/
    if ((ifp = fopen (PRINTCAP, "r")) == NULL) {
	badfile(PRINTCAP);
    }

    (void) sprintf (tempfile, "%s%d", COPYCAP, getpid());
    if ((ofp = fopen (tempfile, "w")) == NULL)
    {
	printf("\nCannot open intermediate file: %s.\n", tempfile);
	perror (tempfile);
	leave (ERROR);
    }

    if ((lfp = fopen (LOGCAP, "a")) == NULL) {
	badfile(LOGCAP);
    }

    timeval = time(0);
    strcpy (buf, "\nDeleted ");
    strcat (buf, ctime(&timeval));
    fputs (buf, lfp);

    /*****************************************
    *  copy printcap to copy until next entry
    *****************************************/
    while (fgets (buf, BUFLEN, ifp) != 0)
    {
        if (!findpname (pname, buf))
	    fputs (buf, ofp);
	else
	{
	    fputs (buf, lfp);
	    while ((fgets (buf, BUFLEN, ifp) != 0) && (buf[strlen(buf) - 2] != ':'))
	    {
		    fputs (buf, lfp);
	    }
	    /* write line with colon */
	    fputs (buf, lfp);
	}
    }

    if (rename (tempfile, PRINTCAP) < 0) {
	printf("\nCannot rename %s to %s (errno=%d).\n",
		tempfile, PRINTCAP, errno);
    }
    fclose (ofp);
    fclose (ifp);
    fclose (lfp);

    UnLinkSpooler();           /* Delete Spooling directory  DJG#5 */
    UnLinkSymFile("af");       /* Delete Accounting file     DJG#10*/
    UnLinkSymFile("lf");       /* Delete Error log file      DJG#10*/

    return(OK);
}

/***********************
*  copy entry into tab
************************/
CopyEntry ()
{
    char    line[LEN];		/* read buffer		 */
    char    tmpline[LEN];	/* temporary buffer	 */
    char   *lineptr;		/* read buffer ptr	 */
    char   *ptr;		/* temp pointer		 */
    int     num;		/* pgetnum return	 */
    int     i;			/* temp index 		 */

	char s[ALPHANUMLEN];
	char *p;
	int status;

	strcpy(s, pnum);
	status = pgetent(bp, s);	
	if (status == -1) {
		badfile(PRINTCAP);
	}
    /*************************
    *  save names for rewrite
    **************************/
    if ((ptr = (char *)index(bp, ':')) > 0) {
	strncpy (longname, bp, ptr - bp);	/* was &bp */
    }
    longname[ptr - bp] = '\0';	/* was &bp */

    /****************************************************
    * loop thru table, changing values where appropriate
    ****************************************************/
    for (i = 0; tab[i].name != 0; ++i)
    {
	switch (tab[i].stype)
	{
	    case BOOL:
		
		if (pgetflag (tab[i].name) == TRUE)
		{
		    if ((tab[i].nvalue = (char *) malloc (strlen ("on") + 1)) == NULL)
		    {
			printf ("\nCannot malloc space for %s\n", tab[i].name);
		    }
		    else
		    {
		        strcpy (tab[i].nvalue, "on");
		        tab[i].used = YES;
		    }
		}
		break;
	    case INT:
		if ((num = pgetnum (tab[i].name)) >= 0)
		{
		    /* fc, fs, xc, xs are in octal, all others are decimal */
		    if ((strcmp(tab[i].name, "fc") == 0) ||
			(strcmp(tab[i].name, "fs") == 0) ||
			(strcmp(tab[i].name, "xc") == 0) ||
			(strcmp(tab[i].name, "xs") == 0)) {
			(void) sprintf (tmpline, "%o", num);
			strcpy(line, "0");	/* put the zero out in front */
			strcat(line, tmpline);
		    } else {
		    	(void) sprintf (line, "%d", num);
		    }
		    if ((tab[i].nvalue = (char *) malloc (strlen (line) + 1)) == NULL)
		    {
			printf ("\nCannot malloc space for %s\n", tab[i].name);
		    }
		    else
		    {
		    	strcpy (tab[i].nvalue, line);
		    	tab[i].used = YES;
		    }
		}
		break;
	    case STR:
		lineptr = line;
		if (pgetstr (tab[i].name, &lineptr) != NULL)
		{
		    *lineptr = 0;
		    if ((tab[i].nvalue = (char *) malloc (strlen (line) + 1)) == NULL)
		    {
			printf ("\nCannot malloc space for %s\n", tab[i].name);
		    }
		    else
		    {
		   	strcpy(tab[i].nvalue, line);
		    	tab[i].used = YES;
		    }
		}
		break;
	    default:
		printf ("\nBad type (%d) for %s\n", tab[i].stype, tab[i].name);
		break;
	}
    }

    return(OK);
}

/******************************
*  write comments for next entry
********************************/
WriteComments (fp)
FILE *fp;

{
    int  i;
    
    for (i = 0; i < numcomments; i++) {
        fprintf (fp, "%s\n", printercomments[i]);
    }
}

/******************************
*  write single entry to file
*******************************/
WriteEntry (fp)
FILE * fp;
{
    int     i;			/* temp index			 */
    char   *curval;		/* pointer to current value	 */
    char localname[LEN];	/* output filter, lp2 or whatever */

    fprintf (fp, "%s:", pname);	/* here, pname is really the longname */
    for (i = 0; tab[i].name != 0; ++i)
    {
	if (tab[i].used == YES)
	{
	    curval = (tab[i].nvalue ? tab[i].nvalue : tab[i].svalue);
	    switch (tab[i].stype)
	    {
		case BOOL:
		    if (strcmp ("on", curval) == 0)
			fprintf (fp, "\\\n\t:%s:", tab[i].name);
		    break;
		case INT:
		    if (strcmp ("none", curval) != 0)
		        fprintf (fp, "\\\n\t:%s#%s:", tab[i].name, curval);
		    break;
		case STR:
		    fprintf (fp, "\\\n\t:%s=%s:", tab[i].name, curval);
		    break;
		default:
		    printf ("\nbad type (%d) for %s\n", tab[i].stype, tab[i].name);
		    break;
	    }
	}
    }
    fprintf (fp, "\n");
    return(OK);
}

/*******************************
*  create accounting file 
*******************************/
MakeAcct (passwd)
struct passwd  *passwd;		/* password file entry	 */
{
    char   *acct;		/* parameter value ptr	 */
    int    i, j;		/* temp index	*/
    int    done;
    char   intermediate[MAXPATH];   /* intermediate directories */
    struct stat sb;
    int filedescriptor;

    for (i = 0; tab[i].name != 0; ++i)
	if (strcmp (tab[i].name, "af") == 0)
	    break;

    if (tab[i].name == 0)
    {
	printf ("\nCannot find accounting file entry in table!\n");
	printf ("No accounting file created.\n");
	return (ERROR);
    }
    if (tab[i].used != YES) {
	/* No account file specified */
	return (OK);
    }
    acct = tab[i].nvalue ? tab[i].nvalue : tab[i].svalue;

    if (stat(acct, &sb) < 0) 
    {
	if (errno == ENOENT)
	{ 
           /*
            * Accounting file does not exist, make any necessary
            * intermediate directories as needed  DJG#1
	    */
            done = 0;
            j = 1;
            while (!done) {
	        while (acct[j] != '/' && acct[j] != '\0') j++;
	        if (acct[j] == '\0') break; /* Create file */
	        strncpy (intermediate, acct, j);
	        intermediate[j++] = '\0';

                if (stat(intermediate, &sb) < 0) {
	            if (mkdir(intermediate, DIRMODE) == -1) {
	                printf ("\nCannot make intermediate accounting directory: %s\n", 
			        intermediate);
	                perror (intermediate);
	                return (ERROR);
                    }
	            if (safechmod (intermediate, 00755) == -1)  /* incase root umask is not 022 */
	            {
		        printf ("\nCannot chmod %s to mode 0755\n", intermediate);
		        perror (intermediate);
		        return (ERROR);
	            }
                } else {
	            /* this portion of the accounting file path exists... */
	            if ((sb.st_mode&S_IFMT) != S_IFDIR) {
	                printf ("\nIntermediate account directory: %s already exists\n",
			        intermediate);
		        printf ("But it is not a directory!\n");
	                return (ERROR);
	            }
                }
	    }

            /* Create actual accounting file */
	    filedescriptor=open(acct,O_CREAT,00644);
	    if (filedescriptor == -1)
	    {
		printf("\nCannot create accounting file %s\n",acct);
		perror (acct);
		return (ERROR);
	    }
/*
    	    if (safechmod (acct, 00644) == -1)
    	    {
		printf ("\nCannot chmod %s to mode 0644\n", acct);
		perror (acct);
	        close(filedescriptor);
		return (ERROR);
    	    }
*/
    	    if (safechown (acct, passwd -> pw_uid, passwd -> pw_gid) == -1)
    	    {
	        printf ("\nCannot chown %s to (%o/%o)\n", acct, passwd->pw_uid, passwd->pw_gid);
		perror (acct);
	        close(filedescriptor);
		return (ERROR);
    	    }
	    close(filedescriptor);
    	    return(OK);
	}
	else
	{
	    printf("\nCannot create accounting file %s\n",acct);
	    perror (acct);
	    return (ERROR);
	}
    }
    else
    {
	printf("\nFile with same name as accounting file %s already exists\n",acct);
	return (ERROR);
    }
}

/*******************************
*  create error log file if it doesn't already exist
*******************************/
MakeErrorlog (passwd)
struct passwd  *passwd;		/* password file entry	 */
{
    char   *errlog;		/* parameter value ptr	 */
    int    i, j;		/* temp index	*/
    int    done;
    char   intermediate[MAXPATH];   /* intermediate directories */
    struct stat sb;
    int filedescriptor;

    for (i = 0; tab[i].name != 0; ++i)
	if (strcmp (tab[i].name, "lf") == 0)
	    break;

    if (tab[i].name == 0)
    {
	printf ("\nCannot find error log file entry in table!\n");
	printf ("No error log file created.\n");
	return (ERROR);
    }
    if (tab[i].used != YES) {
	/* No error log file specified */
	return (OK);
    }
    errlog = tab[i].nvalue ? tab[i].nvalue : tab[i].svalue;

    if (stat(errlog, &sb) < 0) 
    {
	if (errno == ENOENT)
	{
           /*
            * Error Log file does not exist, make any necessary
            * intermediate directories as needed  DJG#1
	    */
            done = 0;
            j = 1;
            while (!done) {
	        while (errlog[j] != '/' && errlog[j] != '\0') j++;
	        if (errlog[j] == '\0') break; /* Create file */
	        strncpy (intermediate, errlog, j);
	        intermediate[j++] = '\0';

                if (stat(intermediate, &sb) < 0) {
	            if (mkdir(intermediate, DIRMODE) == -1) {
	                printf ("\nCannot make intermediate error log directory: %s\n", 
			        intermediate);
	                perror (intermediate);
	                return (ERROR);
                    }
	            if (safechmod (intermediate, 00755) == -1)  /* incase root umask is not 022 */
	            {
		        printf ("\nCannot chmod %s to mode 0755\n", intermediate);
		        perror (intermediate);
		        return (ERROR);
	            }
                } else {
	            /* this portion of the error log file path exists... */
	            if ((sb.st_mode&S_IFMT) != S_IFDIR) {
	                printf ("\nIntermediate error log directory: %s already exists\n",
			        intermediate);
		        printf ("But it is not a directory!\n");
	                return (ERROR);
	            }
                }
	    }

            /* Create error log file */
	    filedescriptor=open(errlog,O_CREAT,00644);
	    if (filedescriptor == -1)
	    {
		printf("\nCannot create error log file %s\n",errlog);
		perror (errlog);
		return (ERROR);
	    }
/*
    	    if (safechmod (errlog, 00644) == -1)
    	    {
		printf ("\nCannot chmod %s to mode 0644\n", errlog);
		perror (errlog);
	        close(filedescriptor);
		return (ERROR);
    	    }
*/
    	    if (safechown (errlog, passwd -> pw_uid, passwd -> pw_gid) == -1)
    	    {
	        printf ("\nCannot chown %s to (%o/%o)\n", errlog, passwd->pw_uid, passwd->pw_gid);
		perror (errlog);
	        close(filedescriptor);
		return (ERROR);
    	    }
	    close(filedescriptor);
    	    return(OK);
	}
    }
    else
    {
	if ((sb.st_mode & S_IFMT) == S_IFREG) 
        {
	    printf("\nWarning: regular file with same name as error log file %s already exists.\n",errlog);
	    return (OK);
	}
	if ((sb.st_mode & S_IFMT) == S_IFLNK)
	{
	    printf("\nWarning: symbolic link with same name as error log file %s already exists.\n",errlog);
	    printf("\nThis may result in printers sharing an error log file.\n");
	    return (OK);
	}
    }
    printf("\nInvalid name for error log file %s\n",errlog);
    printf("File with same name as error log file %s already exists,\n",errlog);
    printf("but it is not a regular file or a symbolic link.\n");
    printf("Status information word S_IFMT = %d\n",sb.st_mode&S_IFMT);
    return(ERROR);
}

/*******************************
*  create spooling directory
*******************************/
MakeSpool (passwd)
struct passwd  *passwd;		/* password file entry	 */
{
    char   *spool;		/* parameter value ptr	 */
    int    i;			/* temp index	*/
    int    j;                   /* spool index  */
    struct stat sb;
    int	   done;		/* loop flag    */
    char   intermediate[MAXPATH];   /* to create intermediate directories */

    for (i = 0; tab[i].name != 0; ++i)
	if (strcmp (tab[i].name, "sd") == 0)
	    break;

    if (tab[i].name == 0)
    {
	printf ("\nCannot find spooler directory entry in table!\n");
	printf ("No spooling directory created.\n");
	return (ERROR);
    }
    if (tab[i].used != YES) {
	/*
	 * If spool directory is not specified in the printcap entry,
	 * lpd will use the default directory
	 */
	printf ("Warning: queue will use default spool directory [/usr/spool/lpd]\n");
	return (OK);
    }
    spool = tab[i].nvalue ? tab[i].nvalue : tab[i].svalue;

    /*
     * Try creating intermediate directories if they do not exist.
     * If stat fails, and then cannot mkdir, so exit. DJG#1
     */

    done = 0;
    j = 1;
    while (!done) {
	while (spool[j] != '/' && spool[j] != '\0') j++;
	if (spool[j] == '\0') done++;
	strncpy (intermediate, spool, j);
	intermediate[j++] = '\0';

        if (stat(intermediate, &sb) < 0) {
	    if (mkdir(intermediate, DIRMODE) == -1) {
	        printf ("\nCannot make spooling directory path: %s\n", 
			intermediate);
	        perror (intermediate);
	        return (ERROR);
            }
	    if (safechmod (intermediate, 00755) == -1)  /* incase root umask is not 022 */
	    {
		printf ("\nCannot chmod %s to mode 0755\n", intermediate);
		perror (intermediate);
		return (ERROR);
	    }
        } else {
	    /* this portion of the spooling directory exists... */
	    if ((sb.st_mode&S_IFMT) != S_IFDIR) {
	        printf ("\nSpooling directory path: %s already exists\n",
			intermediate);
		printf ("But it is not a directory!\n");
	        return (ERROR);
	    }
        }
    }

    if (safechown (spool, passwd -> pw_uid, passwd -> pw_gid) == -1)
    {
	printf ("\nCannot chown %s to (%o/%o)\n", spool, passwd->pw_uid,
	    passwd->pw_gid);
	perror (spool);
	return (ERROR);
    }
    return(OK);
}

/**************************
*  Switch the line off in
*  the /etc/ttys file
**************************/
DisableGetty ()
{
    char   *curval;		/* pointer to current value	 */
    int    i;			/* temp index	*/

    /*
     * Change the /etc/ttys file entry
     *   change mode from on to off. (disable logins).
     */
    for (i = 0; tab[i].name != 0; ++i) {
	if (strcmp(tab[i].name, "lp") == 0) {
	    curval=(tab[i].nvalue ? tab[i].nvalue : tab[i].svalue);
	    if (strncmp(curval, "/dev/tty", 8) == 0) {
		curval=(char *)index(curval, 't');
		/* fixtty(ttyname,0=off - 1=on) */
		fixtty(curval,0);	/* yes, it is "/dev/ttyxx" */
		break;
	    }
        }
    }
    return(OK);
}

/************************************************************
*  unlink spooler directory - removes all files in spool 
*  directory first - DJG#5
*************************************************************/

UnLinkSpooler()
{
    int    i;
    char  *spooler;
    char  unfile[MAXPATH];
    DIR    *dptr, *opendir();
    struct direct *dp, *readdir();
    struct stat sb;

    for (i = 0; tab[i].name != 0; ++i)
	if (strcmp (tab[i].name, "sd") == 0)
	    break;

    if (tab[i].name == 0)	/* can't find 'sd' symbol */
	return;		/* spool directory is just left laying around */

    spooler = tab[i].nvalue ? tab[i].nvalue : tab[i].svalue;

    if (stat(spooler, &sb) == 0) {
        dptr = opendir (spooler);
        for (dp = readdir (dptr); dp != NULL; dp = readdir(dptr)) {
	    if ((strcmp (dp->d_name, ".") != 0) && (strcmp (dp->d_name, "..") != 0)) {
	        (void) sprintf (unfile, "%s/%s", spooler, dp->d_name);
	        if (safeunlink (unfile) != 0)
	            printf ("Couldn't unlink spooler file %s\n", unfile);
	    }
        }

        if (safermdir(spooler) < 0) {
	    printf("couldn't unlink old spooler directory (%s)\n", spooler);
        }
        else {
            printf("Removed spooling directory: %s\n", spooler);
        }
    }
    else {  /* There is a problem */
        if (errno == ENOENT) {
            printf("\nSpooling directory %s does not exist\n",spooler);
        }
        else {
            printf("\nCan not unlink spooling directory %s because of errno %d\n",
                      spooler, errno);
        }
        perror (spooler);
    }
}

/*
 * UnLinkSymFile - Removes the file associated with the specified symbol.
 * DJG#10
 */

int
UnLinkSymFile (symbol)
char	*symbol;
{
    int    i;
    char *filename;
    int yn;


    for (i = 0; tab[i].name != 0; i++) {
	if (strcmp (tab[i].name, symbol) == 0) {
	    if (tab[i].used) {
		filename = tab[i].nvalue ? tab[i].nvalue : tab[i].svalue;
		printf("Do you want to delete '%s' file '%s' [n] ", symbol, filename);
		yn = 'n';
		if (YesNo (yn) == TRUE) {

		    if (safeunlink(filename) != 0)
		    printf ("Couldn't unlink :%s: file: %s\n", symbol, filename);
		    else
		    printf ("Deleted file: %s\n", filename);
		}
	    }
	    break;
	}
    }
}

/**************************
 * badfile: print "cannot open
 * <filename>", and exit(1).
 **************************/
badfile(s)
char *s;
{
	printf("\nCannot open %s\n", s);
	perror(s);
	leave(ERROR);
}

/******************************************************************************
* end of lprsetup.c
******************************************************************************/
