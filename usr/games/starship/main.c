#ifndef lint
static	char	*sccsid = "@(#)main.c	4.2	(ULTRIX)	9/1/88";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1985 by				*
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
 ************************************************************************
/*
 * main.c
 *
 * Name:	starship
 * Purpose:	Main, curses setup, shared mem setup.
 * Usage:	starship [-bs]
 * Environment:	Ultrix-32, optionally with shared memory
 * Compile:	see Makefile
 * Date:	April 18, 1985
 * Author:	Al Delorey
 * Remarks:


    These are the voyages of the independent star ships.

Whose lifetime mission: To explore strange new galaxies,
		        To seek out and destroy other star ships,
		        To boldly go where no other star ship dares!

*/

/*
 * Modification history
 *
 * 4-18-85
 *	Derived from former file "one.c" which had main & cmd input
 *	together.  Main & cmd input are now split into main.c & cmd.c
 */


#include "star.h"
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <curses.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/file.h>
#include <errno.h>

/* Global to the program - Will be externals in other files */

struct univ *uptr;		/* ptr to global data struct */
WINDOW *scanwin;		/* for sub window: short & long range scan */
#ifdef SHMEM
int shmid;			/* shared memorey id */
char *shmat();			/* shared mem attach routine */
#endif SHMEM

/* Define new access paths to variables that are now in "univ" struct,
 * to facilitate shared memory.
 */
#define universe uptr->quadrants
#define num_users uptr->Num_users
#define wait uptr->Wait
#define ships uptr->Ships
#define drones uptr->Drones

int scanf_cc;			/* char count accumulated in scanf_buf */
char scanf_buf [READSIZE];	/* cmd input buffer */
boolean clr_cmd_flag;		/* true to clear cmd line */

char local_ship;	/* char to represent ship */
boolean redraw;		/* set true in "scanf_input" by N command */
boolean gotchar;	/* set true when input char read, false when echoed */
char esc;		/* esc char */
char bell;		/* bell char or null if no bell option in use */
boolean userabort;		/* set true if user quits */
extern int errno;

/* local variables */

struct sigvec *vec;	/* signal vector for asynch input */
int sflag;		/* slower screen update if set */
unsigned int pausetime;	/* pause longer if -s flag used */
int dafter;		/* incr once for each drone after you */

/* for average kills per game, score keeping */
char *home;		/* for environment var "HOME" */
char scorepath[50];	/* path to user's HOME/.starship */
int games;		/* number of games played, from .starship */
int kills;		/* number of total kills, from .starship */
float ave;		/* average (kills/games) */
int ruid;		/* real uid */
boolean mustcreate = false;

/* function declarations */

char *getenv();		/* to get the tty type out of the environ var */
FILE *fopen();		/* to keep average number of kills per game */
char *calloc();		/* funct type decl */
int scanf_input();	/* funct type decl for signal vector */


/*****************************************************************************/
/*									     */
/*    		START INTERNAL PROCEDURES				     */
/*									     */
/*****************************************************************************/


main (argc,argv)
    int argc;
    char *argv[];

    {
    int i;		/* loop index to move thru drones array */
    register struct ship *pships;

    /* Process command line arguments */

    bell='\07';

    for(i = 1; argc>1 && argv[i][0]=='-'; argc--,i++)
	switch(argv[i][1])
	    {
	    case 's':
		/* slower screen update; good for dialup */
		sflag++;
		continue;
	    case 'b':
		bell='\0';
		continue;
	    }
    /*
     * Before call to shmget due to 8k malloc limit after shmget
     */
    if (initscr() == ERR)
	{
	printf ("%s: initscr() failed\n", argv[0]);
	exit(-1);
	}
    if ((scanwin = subwin(stdscr,SRSCANROWS+2,MAPDISPC,0,0)) == ERR)
	{
	printf ("%s: subwin() failed\n", argv[0]);
	exit(-1);
	}
    if (sflag)
	pausetime = 2;
    else
	pausetime = 1;

    /* 
     * Get user's average score.
     */
    getave();

    /*
     * Create and attach to shared memory segment.
     * RACE CONDITION POTENTIAL: if the shared mem seg does not exist yet,
     *    and more than 1 process starts to create it, we could get more
     *    than 1 shared memory segment.
     * NOTE: We attach the shared memory segment 64k above current end
     *       of process space to leave room for other mallocs.
     */

#   ifdef SHMEM
    if ((shmid = shmget(ftok("/usr/games/starship", 'a'), sizeof(struct univ), IPC_CREAT | 0666)) < 0)
	{
	printf("%s: shmget failed, errno = %d\n", argv[0], errno);
	perror("starship");
	exit(-1);
	}
    
    if ((uptr = (struct univ *) shmat(shmid, sbrk(0) + 64*1024, 0)) < 0)
	{
	printf("%s: shmat failed, errno = %d\n", argv[0], errno);
	perror("starship");
	exit(-1);
	}
#   else
    uptr = (struct univ *) malloc(sizeof(struct univ));
#   endif SHMEM

    /*
     * Ignore signals that would exit the process ungracefully
     */
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);

    /*
     * RACE CONDITION POTENTIAL: if num_users == 0, and more than one
     *    process enters this section of code before one of them sets
     *    num_users to 1, the data base could get initialized twice.
     */

    if (num_users == 0)
	{		/* you are the first user */
	num_users = 1;
	wait = true;	/* we will make other players wait for db inits */
	init_d_s();
	}
    else 
	{		/* not 1st player */
	num_users++;
	while (wait) ;		/* wait for global inits to be done */
	};

    /* 
     * Do local initializations, print instructions (& get ship name),
     */
    local_init();
    instructs();
    pships = ships + (local_ship - A);
    pships->avekills = ave;

#   ifndef DEBUG
    noecho();
    crmode();
#   endif

    draw_screen();

#   ifndef DEBUG
#   ifdef ASYNC
    /* This section sets up:
     *   1. the handler routine (scanf_input) for asynchronous input.
     *   2. the signal to catch & handle (SIGIO).
     */
    vec = (struct sigvec *) calloc (1, sizeof (struct sigvec));
    if (vec == NULL)
	{
	perror("starship (sigvec calloc)");
	exit(-1);
	}
    vec->sv_handler = scanf_input;
    vec->sv_mask = SIGIO;
    vec->sv_onstack = 0;

    /* fcntl enables SIGIO for the file descriptor 0 (stdin) */
    if (fcntl (0, F_SETFL, FASYNC) == -1)
	{
	fprintf (stderr, "sigin: fcntl call failed\n");
	perror ("sigin");
	exit (-1);
	}

    /* Call sigvec to activate signal handling (of SIGIO on stdin) */
    sigvec (SIGIO, vec, 0);
#   endif ASYNC
#   endif DEBUG

    /* loop until ship out of energy */

    for (; pships->energy > 0 || pships->shields > 0;)
	{
#	ifndef DEBUG
#	ifndef ASYNC
	  /*
	   * Only sleep for 1/4 second at a time so that we can keep
	   * checking for cmd input.  Sleep enough 1/4 seconds to add
	   * up to "pausetime" seconds.
	   */
	  for (i = 0; i < 4 * pausetime; i++)
	      {
	      starsleep(0, 250000);		/* 250,000 usec => 1/4 sec */
	      scanf_input();
	      }
#	else ASYNC
	  starsleep(pausetime,0);
#	endif ASYNC
#	else DEBUG
	  scanf_input();
#	endif DEBUG
	if (userabort)
	    break;
	if (redraw)
	     redraw_screen();		/* re-draw entire screen */
	ship_action (local_ship);	/* carry out commands, move_to torps */
	update_screen (local_ship);	/* change screen image */

#	ifdef DRONE
	/* If you are fighting the drone, run the drone */
	if (dafter)
	    {
	    for (i = 0; i < NUMDRONES; i++)
		if (drones[i].d_after == local_ship)
		    run_drone(i);
	    }
#	endif
	if (clr_cmd_flag)
	    {
	    move_to (COMMANDR,COMMANDC);
	    clear_to_eol();
	    refresh_scr();
	    clr_cmd_flag = false;
	    };
	}

    /*
     * Call update_screen one last time to show what killed the ship,
     * save number of kills,  exit the ship from the game, then update
     * the .starship score file (done last since it changes effective uid).
     */

    update_screen (local_ship);
    kills += ships[local_ship - A].kills;
    exit_a_ship (local_ship, false);	/* clean-up & exit */
    saveave();

    /* restore tty */

#   ifndef DEBUG
    nocrmode();
    echo();
#   endif

    endwin();
    }  /* end main */

string_out(str)
    char *str;

    /* Handle output of a string. */

    {
    addstr(str);
    }

char_out(ch)
    char ch;

    /* Handle output of a char. */

    {
    addch(ch);
    }

move_to(row,col)
    int row,col;

    /* Handle moving to row,col. */

    {
    move(row,col);
    }

refresh_scr()

    /* cause screen to be updated. */

    {
    refresh();
    }

clear_screen()

    /* Handle clearing the screen. */

    {
    clear();
    }

clear_to_eol()

    /* Handle clearing to eol. */

    {
    clrtoeol();
    }

local_init()

    /* Initialize local variables (global to main star).
     * and set up for unsolicited input.
     */


    {
    clr_cmd_flag = false;
    userabort = false;
    redraw = false;
    scanf_cc = 0;
    gotchar = false;
    dafter = 0;
    };  /* proc local_init */


getave()

    /*
     * Get average number of kills per game out of user's home directory.
     * We keep a .starship there with 2 integer numbers in it:
     * 1. the number of games played, 2. the total number of kills.
     *
     * The games and kills read from the file go into the global var's
     * "games" and "kills" to be used to update the file at exit time.
     */

    {
    FILE *fp;		/* file ptr to HOME/.starship */

    if ((home = getenv("HOME")) == NULL)
	printf("Cannot access `HOME' environment variable");
    else
	{
	strcpy(scorepath, home);
	strcat(scorepath, "/.starship");
	/*
	 * Attempt to open score file.
	 */
	if ((fp = fopen(scorepath, "r")) == NULL)
	    {
	    mustcreate = true;
	    ave = 0;
	    }
	else
	    {
	    fscanf(fp,"%d %d", &games, &kills);
	    fclose(fp);
	    if (games == 0)
		ave = 0;
	    else
		ave = (float)kills/(float)games;
	    if (ave > 99)
		ave = 99.0;
	    }
	}
    }


saveave()

    /*
     * Save average number of kills per game in user's home directory.
     * We keep a .starship there with 2 integer numbers in it:
     * 1. the number of games played, 2. the total number of kills.
     *
     * Increment games here, add present kills to total and rewrite
     *   .starship file.
     */

    {
    int fd;
    FILE *fp;

    ruid = getuid();
    if (setreuid (ruid,ruid) < 0)
	{
	perror("set effective to real");
	exit(-1);
	}
    /*
     * open/create to create it if it doesn't exist.
     */
    if (mustcreate)
	{
	if ((fd = open(scorepath, O_CREAT | O_RDWR, 0644)) == NULL)
	    {
	    printf("open on %s failed.\n", scorepath);
	    perror("starship");
	    }
	close (fd);
	chmod (scorepath, 0644);
	}
    /*
     * open to write score file
     */
    if ((fp = fopen(scorepath, "w")) == NULL)
	{
	printf("open on %s failed.\n", scorepath);
	perror("starship");
	}
    else
	{
	games++;
	fprintf(fp,"%d %d", games, kills);
	fclose(fp);
	}
    }

instructs()
    {

    /* Prints out instructions for the users.  */


	char in_str[5];		/* user input */
	char ch;		/* user input */
	char name[NAMESIZE +1];/* ship name */
	char index_ch;		/* loop index */
	boolean already_used;	/* set true if letter already in use */
	char ypasswd[10];	/* pass word user enters */
	char passwd[10];	/* true pass word */
	int i;			/* loop index */

    for (i = 0; i < PASSWDLEN; i ++)
	passwd [i] = ' ';

    passwd [0] = (char)78;
    passwd [1] = (char)69;
    passwd [2] = (char)86;
    passwd [3] = (char)69;
    passwd [4] = (char)82;
    passwd [5] = '\0';

    clear_screen();
    refresh_scr();

    printf ("\n                              Welcome to STAR-SHIP\n\n");
    printf ("                                 by Al Delorey\n\n\n");
    printf (" These are the voyages of the independent star ships.\n\n");
    printf (" Whose lifetime mission: To explore strange new galaxies,\n");
    printf ("                         To seek out and destroy other star ships,\n");
    printf ("                         To boldly go where no other star ship dares!\n\n");
    printf (" Do you want instructions? ");
    gets (in_str);
    ch = in_str[0];
    upchar(&ch);
    if (ch != 'N')
         {		/* print instructions */
	clear_screen();
	refresh_scr();
	printf (" STAR-SHIP is an interactive game that you can play against ");
	printf ("other players,\n");
	printf (" Or you can play against a drone ship that the game controls.\n\n");
	printf (" The Galaxy is a matrix of 10x10 quadrants.  Each quadrant ");
	printf ("has 10x10 sectors.\n");
	printf (" A sector is one character position.\n");
	printf (" Quadrant & sector displays are given as <row>-<col>\n");
	printf ("         Where the upper left corner is      0-0\n");
	printf ("           and the lower right corner is     9-9\n\n");
	printf (" On the game screen, the numbers around the short ");
	printf ("range scan show the\n");
	printf (" directional bearings.  The reverse video section shows ");
	printf ("command usage.\n\n");
	printf (" STAR *     BASE +     TORPEDO .     SHIP <capital letter>");
	printf ("     ION CLOUD -\n\n");
	printf (" Passing through a star costs %d energy units.\n", STAR_COST);
	printf (" Being hit by Photon Torpedo costs %d energy units.\n", TORP_COST);
	printf (" Torpedos travel 10 sectors (one quadrant) maximum.\n");
	printf (" Damaged devices show an asterisk and the time remaining to complete repairs\n");
	printf ("    until they will be operational again.\n");
	printf (" Phasers reach %d sectors maximum, and their effect decreases with distance.\n", PHSRANGE);
	printf (" Using your phasers consumes your energy.\n\n");

	printf (" Do you want to continue with instructions? ");
	gets (in_str);
	ch = in_str[0];
	upchar (&ch);
	if (ch != 'N')
	    {
	    clear_screen ();
	    refresh_scr ();

	    printf (" To dock at a base you must be at one of the 4 `docking bays' (+ points)\n");
	    printf ("         and use the D command.\n");
	    printf ("     This restores all energy and devices, and also drops your shields.\n");
	    printf ("     NOTE: You cannot use your weapons when docked - but others can attack you.\n");
	    printf ("           You must move away from the base to use your weapons.\n\n");
	    printf (" You can only raise your shields to 1/2 of your present energy level.\n");
	    printf (" Accelerating or decelerating costs energy.\n");
	    printf (" At a constant speed, if your warp engines are operating, ");
	    printf ("energy regenerates.\n\n");
	    printf (" To invoke a drone opponent radio the R or the K and ");
	    printf ("specify the difficulty\n");
	    printf ("   level by sending a one digit message of 1 to 4 ");
	    printf ("(4 is hardest).  eg:  RK1.\n\n");
	    printf (" A few usefull commands to help you get started:\n");
	    printf (" Typing a single digit will change your bearing.\n");
	    printf (" Typing `i' will get you moving. `i0' will stop you.\n");
	    printf (" The `w' command will move you faster; try it after you get some experience.\n");
	    printf (" Typing `s15' will put 15k units of strength in your shields.\n");
	    printf (" The Quit, Radio & Shields commands MUST END WITH A <CR>\n\n");
	    printf (" To supress the bell, invoke Star-Ship with the -b option.\n");
	    }
	}; /* print instructions */

    do
	{
	printf ("\n Enter the name of your ship (upto 10 chars) ");
	gets (name);
	name[NAMESIZE] = '\0';
	upcase (name);

	already_used = false;
	if (!strcmp(name, "ENTERPRISE"))
	    {
	    printf (" PASSWORD ");

	    /* turn off echo and scan password */
#	    ifndef DEBUG
	    noecho();
#	    endif
	    gets (ypasswd);

	    /* turn echo back on */
#	    ifndef DEBUG
	    echo();
#	    endif
	    if (strcmp(ypasswd, passwd) || (ships['E'-A].ship_ch == 'E'))
	        {
	 	printf (" Sorry but that ship is in use already\n");
		already_used = true;
		};
	     };
	}
    while (already_used);

    if (!strcmp(name, "ENTERPRISE"))
         ch = 'E';
    else do
	    {
	    printf (" Enter a letter to represent the %s ", name);
	    gets (in_str);
	    ch = in_str[0];
	    upchar(&ch);

	    already_used = false;
	    if (ch == 'E')
	         {
		 already_used = true;
		 printf ("\n That letter is in use, choose another\n");
		 }
	    else if (ch < 'A' || ch > 'Z')
		      {
		      already_used = true;  /* to stay in repeat loop */
		      printf ("\n You must use a LETTER\n");
		      }
	    else for (index_ch = 'A'; index_ch < 'Z'; index_ch ++)
		    if (ships [index_ch-A].ship_ch == ch)
		         {
			 already_used = true;
			 printf ("\n That letter is in use, choose another\n");
			 };
	    }
	while (already_used);

    init_a_ship (ch, name);
    local_ship = ch;

    clear_screen();
    refresh_scr();
    }; /* instructs */


upcase (name)
    char *name;

    /* convert string in name to upper case.
     */

    {
    int i;

    for (i=0; name[i] != '\0'; i++)
	{
	if (name[i] >= 'a' && name[i] <= 'z')
	    name[i] = name[i] - 32;
	};
    };


upchar (ch)
    char *ch;

    /* convert char to upper case.
     */

    {
    if (*ch >= 'a' && *ch <= 'z')
	*ch = *ch - 32;
    };

draw_sr()
    {
    register int i,j;

    /* Draw srscan area */

    move_to(SRSCANR-1, SRSCANC-1);
    string_out ("7---------------9");

    for (i = SRSCANR; i <= LSRSCANR; i ++)
	{
	j = i;
	if (i == SRSCANR + SRSCANROWS / 2)
	    {
	    move_to(j,SRSCANC-2);
	    string_out ("4|               |6");
	    }
	else
	    {
	    move_to(j,SRSCANC-1);
	    string_out ("|               |");
	    }
	};

    move_to((SRSCANR + SRSCANROWS), (SRSCANC -1));
    string_out ("1---------------3");
    }

draw_lr()
    {
    /* draw LR SCAN */

    register int i,j;

    move_to (LRSCANR - 3, LRSCANC);
    string_out ("Long Range Scan");
    move_to(LRSCANR - 1, LRSCANC - 2);
    string_out ("+-----|-----|-----+");
    j = LRSCANR;

    for (i = 0; i < LRSCANSECTS; i ++)
	{
	if (i == 1)
	    {
	    move_to(j, LRSCANC - 3);
	    string_out (" | 000 | 000 | 000 | ");
	    }
	else
	    {
	    move_to(j, LRSCANC - 2);
	    string_out ("| 000 | 000 | 000 |");
	    }
	move_to(j+1, LRSCANC - 2);
	string_out ("+-----|-----|-----+");
	j = j + 2;
	};

    move_to(LRSCANR + 5, LRSCANC - 2);
    string_out ("+-----|-----|-----+");
    move_to(LRLABELR, LRLABELC);
    string_out ("SHIPS|BASES|STARS");
    }

draw_dir()

    /* directional display */

    {
    register int i;

    move_to(DIRDISPR - 2, MAPDISPC - 1);
    string_out ("   Bearings");
    move_to(DIRDISPR - 1, MAPDISPC);
    string_out ("            ");
    move_to(DIRDISPR, MAPDISPC);
    string_out ("   7 8 9    ");
    move_to(DIRDISPR + 1, MAPDISPC);
    string_out ("    \\|/     ");
    move_to(DIRDISPR + 2, MAPDISPC);
    string_out ("   4-|-6    ");
    move_to(DIRDISPR + 3, MAPDISPC);
    string_out ("    /|\\     ");
    move_to(DIRDISPR + 4, MAPDISPC);
    string_out ("   1 2 3    ");

    for (i = DIRDISPR + 5; i <= DIRDISPR + 10; i++)
	{
	move_to(i, MAPDISPC);
	string_out ("            ");
	}
    }

draw_map()

    /* directional display */

    {
    register int i,j;
    char tmp_str[10];	/* temp output string */

    move_to(MAPDISPR - 1, MAPDISPC + 2);
    string_out ("   Map   ");
    move_to(MAPDISPR, MAPDISPC);
    string_out (" 0123456789");
    j = 0;
    for (i = MAPDISPR + 1; i <= MAPDISPR + 10; i++)
	{
	move_to(i, MAPDISPC);
	sprintf (tmp_str, "%d%s", j, "       ");
	string_out (tmp_str);
	j++;
	}
    move_to(MAPDISPR + 11, MAPDISPC);
    string_out (" 0123456789");
    }

draw_screen()
    {

    /* This procedure draws the initial image on the screen.
    */
    
    char tmp_str[10];	/* temp output string */

    clear_screen();
    refresh_scr();

    draw_sr();
    draw_lr();
    draw_dir();

    /* label display fields */

    move_to(ENERGYR, ENERGYC - 10);
    string_out ("ENERGY:");
    move_to(ENERGYR, ENERGYC);
    sprintf (tmp_str, "%5d", ENERGY_MAX);
    string_out (tmp_str);
    move_to(SHIELDSR, SHIELDSC - 10);
    string_out ("SHIELDS:      0");
    move_to(TORPR, TORPC - 11);
    string_out ("TORPEDOS:");
    move_to(TORPR, TORPC);
    sprintf (tmp_str, "%4d", TORP_MAX);
    string_out (tmp_str);
    move_to(PHASERR, PHASERC - 11);
    string_out ("PHASERS:    opr");
    move_to(WARPDRIVER,WARPDRIVEC-11);
    string_out ("WARP DRV:   opr");

    move_to(QUADR, QUADC - 12);
    string_out ("QUADRANT:   0-0");
    move_to(SECTORR, SECTORC - 12);
    string_out ("SECTOR:     0-0");
    move_to(WARPSETR, WARPSETC - 12);
    string_out ("WARP SET:   0.0");
    move_to(WARPSPEEDR, WARPSPEEDC-12);
    string_out ("WARP SPEED: 0.0");
    move_to(BEARINGR, BEARINGC - 12);
    string_out ("BEARING:    6");

    move_to(IMPULSER, IMPULSEC - 15);
    string_out ("IMPULSE ENG:    opr");
    move_to(LIFESUPR, LIFESUPC - 14);
    string_out ("LIFE SUPPORT: green");
    move_to(SRCONDR, SRCONDC - 15);
    string_out ("S.R. SCAN:      opr");
    move_to(LRCONDR, LRCONDC - 15);
    string_out ("L.R. SCAN:      opr");
    move_to(RADIOR, RADIOC - 15);
    string_out ("RADIO:          opr");

    move_to(SENSORR, SENSORC - 13);
    string_out ("SENSORS:      opr");
    move_to(CLOAKINGR, CLOAKINGC - 13);
    string_out ("CLOAKING:     opr");

    /* command help: highlighted */
    move_to(CMDHELPR,CMDHELPC);
    standout();				/* curses usually does rev video */

    string_out ("[1-9] bearing   C Cloak  D Dock   G Get-msg   I[0-9] Impulse   L Locate   M Map");

    move_to(CMDHELPR + 1,CMDHELPC);
    string_out ("N Redraw-scan   P[A-Z] Phaser   Q<CR> Quit   R[A-Z][msg]<CR> Radio   X[A-Z] Xam");

    move_to(CMDHELPR + 2,CMDHELPC);
    string_out ("S[01-25]/S-[1-9] Shields+nK/-nK    T[1-9] Torpedo    W[0-6] Warp-Speed    Z Zap");

    standend();				/* end "stand-out" mode */

    /* set up message line & command input prompt */

    move_to(MESSAGER, MESSAGEC - 9);
    string_out ("MESSAGE:");
    move_to(COMMANDR, COMMANDC - 9);
    string_out ("COMMAND:");
    refresh_scr();
    };  /* draw screen */


redraw_screen()

    /* Re-draws the screen.  Useful in switch a broadcast or system message
       messed it up.
    */

    {
    register struct ship *pships;

    draw_screen();
    pships = ships + (local_ship - A);
    pships->dis_energy = true;
    pships->dis_shields = true;
    pships->dis_torpedos = true;
    pships->dis_warp_set = true;
    pships->dis_warp_speed = true;
    pships->dis_quad = true;
    pships->dis_sector = true;
    pships->dis_bearing = true;
    pships->dis_fleet = true;
    pships->dis_dir = true;
    redraw = false;
    };  /* proc redraw_screen */


shipinrange(callship, findship, range, wantbd, dist, bearing, ontarget)
    char callship;	/* calling ship */
    char findship;	/* ship to look for */
    int range;		/* range (in sectors) to scan: upto 5 */
    boolean wantbd;	/* true if bearing & distance wanted */
    int *dist;		/* distance to ship (if found) */
    int *bearing;	/* bearing toward ship (if found) */
    boolean *ontarget;	/* true if on target for torpedo */

    /*
     * Determine if another ship (findship) is within "range" & if so,
     * optionally its distance & bearing.
     *
     * RETURNS 1 if ship found, 0 if not found.
     */

    {
    register struct ship *pships;
    register int i,j;	/* loop index */
    boolean found;	/* set true when ship found */

    int sqr;		/* starting quad row */
    int sqc;		/* starting quad col */
    int ssr;		/* starting sector row */
    int ssc;		/* starting sector col */

    int cqr;		/* current quad row */
    int cqc;		/* current quad col */
    register int csr;	/* current sector row */
    register int csc;	/* current sector col */

    pships = ships + (callship - A);

      /* calculate starting quad & sector, row & col */

      sqr = pships->q_row;
      sqc = pships->q_col;
      ssr = pships->s_row - range;
      ssc = pships->s_col - range;

      /* adjust starting values if they start in previous quadrant */

      if (ssr < 0)
	  {
	  ssr = NUMSECTORS + ssr;
	  sqr = sqr - 1;
	  if (sqr < 0)
	      sqr = NUMQUADS -1;
	  };

      if (ssc < 0)
	  {
	  ssc = NUMSECTORS + ssc;
	  sqc = sqc - 1;
	  if (sqc < 0)
	      sqc = NUMQUADS - 1;
	  };

      /* init "current" values */

      cqr = sqr;
      csr = ssr;

      /* loop for (range*2)+1 rows: ship position +/- range */

      for (i=0, found=false; i < (range * 2)+1 && ! found ;)
	  {
	  csc = ssc;
	  cqc = sqc;

	  /* loop for (range * 2)+1 cols: ship position +/- range */

	  for (j=0; j < (range * 2)+1 && ! found ;)
	      {
	      if (universe [cqr][cqc].sectors [csr][csc].image == findship)
		   found = true;
	      else {
		    j++;
		    csc++;
		    if (csc > NUMSECTORS -1)
			 {		/* goto 1st sector col, in next quad col */
			 csc = 0;
			 cqc++;
			 if (cqc > NUMQUADS -1)
			      cqc = 0;
			 };
		   };
	      };  /* column loop */
	  if (! found)
	      {
	      i++;
	      csr++;
	      if (csr > NUMSECTORS -1)
		   {
		   csr = 0;
		   cqr++;
		   if (cqr > NUMQUADS -1)
			cqr = 0;
		   };
	      };
	  };  /* row loop */

       /* calculate distance bearing */
       if (found && wantbd)
	   {
	    switch (i)
	       {
	       case 0:
		   *dist = 5; 
		   switch (j)
		       {
		       case 0:
			   *ontarget = true; *bearing = 7; break;
		       case 1:
		       case 2:
		       case 3:
		       case 4:
			   *bearing = 7; break;
		       case 5:
			   *bearing = 8; *ontarget = true; break;
		       case 6:
		       case 7:
		       case 8:
		       case 9:
			   *bearing = 9; break;
		       case 10:
			   *bearing = 9; *ontarget = true; break;
		       }
		   break;
	       case 1:
		   switch (j)
		       {
		       case 0:
			   *dist = 5; *bearing = 7; break;
		       case 1:
			   *dist = 4; *ontarget = true; *bearing = 7; break;
		       case 2:
		       case 3:
		       case 4:
			   *dist = 4; *bearing = 7; break;
		       case 5:
			   *dist = 4; *bearing = 8; *ontarget = true; break;
		       case 6:
		       case 7:
		       case 8:
			   *dist = 4; *bearing = 9; break;
		       case 10:
			   *dist = 5; *bearing = 9; break;
		       case 9:
			   *dist = 4; *bearing = 9; *ontarget = true; break;
		       }
		   break;
	       case 2:
		   switch (j)
		       {
		       case 0:
			   *dist = 5; *bearing = 7; break;
		       case 1:
			   *dist = 4; *bearing = 7; break;
		       case 2:
			   *dist = 3; *ontarget = true; *bearing = 7; break;
		       case 3:
		       case 4:
			   *dist = 3; *bearing = 7; break;
		       case 5:
			   *dist = 3; *bearing = 8; *ontarget = true; break;
		       case 6:
		       case 7:
			   *dist = 3; *bearing = 9; break;
		       case 8:
			   *dist = 3; *bearing = 9; *ontarget = true; break;
		       case 9:
			   *dist = 4; *bearing = 9; break;
		       case 10:
			   *dist = 5; *bearing = 9; break;
		       }
		   break;
	       case 3:
		   switch (j)
		       {
		       case 0:
			   *dist = 5; *bearing = 7; break;
		       case 1:
			   *dist = 4; *bearing = 7; break;
		       case 2:
			   *dist = 3; *bearing = 7; break;
		       case 3:
			   *dist = 2; *bearing = 7; *ontarget = true; break;
		       case 4:
			   *dist = 2; *bearing = 4; *dist = 2; break;
		       case 5:
			   *dist = 2; *bearing = 8; *ontarget = true; break;
		       case 6:
			   *dist = 2; *bearing = 6; *dist = 2; break;
		       case 7:
			   *dist = 2; *bearing = 9; *ontarget = true; break;
		       case 8:
			   *dist = 3; *bearing = 9; break;
		       case 9:
			   *dist = 4; *bearing = 9; break;
		       case 10:
			   *dist = 5; *bearing = 9; break;
		       }
		   break;
	       case 4:
		   switch (j)
		       {
		       case 0:
			   *dist = 5; *bearing = 7; break;
		       case 1:
			   *dist = 4; *bearing = 7; break;
		       case 2:
			   *dist = 3; *bearing = 7; break;
		       case 3:
			   *dist = 2; *bearing = 8; break;
		       case 4:
			   *dist = 1; *bearing = 7; *ontarget = true; break;
		       case 5:
			   *dist = 1; *bearing = 8; *ontarget = true; break;
		       case 6:
			   *dist = 1; *bearing = 9; *ontarget = true; break;
		       case 7:
			   *dist = 2; *bearing = 2; break;
		       case 8:
			   *dist = 3; *bearing = 9; break;
		       case 9:
			   *dist = 4; *bearing = 9; break;
		       case 10:
			   *dist = 5; *bearing = 9; break;
		       }
		   break;
	       case 5:
		   *dist = abs(j-range);
		   switch (j)
		       {
		       case 0:
		       case 1:
		       case 2:
		       case 3:
		       case 4:
			   *bearing = 4; *ontarget = true; break;
		       case 5:
			   /* this case shouldn't happen */
			   *bearing = 4; break;
		       case 6:
		       case 7:
		       case 8:
		       case 9:
		       case 10:
			   *bearing = 6; *ontarget = true; break;
		       }
		   break;
	       case 6:
		   switch (j)
		       {
		       case 0:
			   *dist = 5; *bearing = 1; break;
		       case 1:
			   *dist = 4; *bearing = 1; break;
		       case 2:
			   *dist = 3; *bearing = 1; break;
		       case 3:
			   *dist = 2; *bearing = 8; *dist = 2; break;
		       case 4:
			   *dist = 1; *bearing = 1; *ontarget = true; break;
		       case 5:
			   *dist = 1; *bearing = 2; *ontarget = true; break;
		       case 6:
			   *dist = 1; *bearing = 3; *ontarget = true; break;
		       case 7:
			   *dist = 2; *bearing = 8; *dist = 2; break;
		       case 8:
			   *dist = 3; *bearing = 3; break;
		       case 9:
			   *dist = 4; *bearing = 3; break;
		       case 10:
			   *dist = 5; *bearing = 3; break;
		       }
		   break;
	       case 7:
		   switch (j)
		       {
		       case 0:
			   *dist = 5; *bearing = 1; break;
		       case 1:
			   *dist = 4; *bearing = 1; break;
		       case 2:
			   *dist = 3; *bearing = 1; break;
		       case 3:
			   *dist = 2; *bearing = 1; *ontarget = true; break;
		       case 4:
			   *dist = 2; *bearing = 4; *dist = 2; break;
		       case 5:
			   *dist = 2; *bearing = 2; *ontarget = true; break;
		       case 6:
			   *dist = 2; *bearing = 4; *dist = 2; break;
		       case 7:
			   *dist = 2; *bearing = 3; *ontarget = true; break;
		       case 8:
			   *dist = 3; *bearing = 3; break;
		       case 9:
			   *dist = 4; *bearing = 3; break;
		       case 10:
			   *dist = 5; *bearing = 3; break;
		       }
		   break;
	       case 8:
		   switch (j)
		       {
		       case 0:
			   *dist = 5; *bearing = 1; break;
		       case 1:
			   *dist = 4; *bearing = 1; break;
		       case 2:
			   *dist = 3; *bearing = 1; *ontarget = true; break;
		       case 3:
		       case 4:
			   *dist = 3; *bearing = 1; break;
		       case 5:
			   *dist = 3; *bearing = 2; *ontarget = true; break;
		       case 6:
		       case 7:
			   *dist = 3; *bearing = 3; break;
		       case 8:
			   *dist = 3; *bearing = 3; *ontarget = true; break;
		       case 9:
			   *dist = 4; *bearing = 3; break;
		       case 10:
			   *dist = 5; *bearing = 3; break;
		       }
		   break;
	       case 9:
		   switch (j)
		       {
		       case 0:
			   *dist = 5; *bearing = 1; break;
		       case 1:
			   *dist = 4; *bearing = 1; *ontarget = true; break;
		       case 2:
		       case 3:
		       case 4:
			   *dist = 4; *bearing = 1; break;
		       case 5:
			   *dist = 4; *bearing = 2; *ontarget = true; break;
		       case 6:
		       case 7:
		       case 8:
			   *dist = 4; *bearing = 3; break;
		       case 9:
			   *dist = 4; *bearing = 3; *ontarget = true; break;
		       case 10:
			   *dist = 5; *bearing = 3; break;
		       }
		   break;
	       case 10:
		   *dist = 5; 
		   switch (j)
		       {
		       case 0:
			   *bearing = 1; *ontarget = true; break;
		       case 1:
		       case 2:
		       case 3:
		       case 4:
			   *bearing = 1; break;
		       case 5:
			   *bearing = 2; *ontarget = true; break;
		       case 6:
		       case 7:
		       case 8:
		       case 9:
			   *bearing = 3; break;
		       case 10:
			   *bearing = 3; *ontarget = true; break;
		       }
		   break;
	       }   /* switch i  */
	    }  /* found & wantbd */
    if (found)
	return(1);
    else
	return(0);
    };  /* shipinrange */
