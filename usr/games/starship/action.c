#ifndef lint
static char	*sccsid = "@(#)action.c	4.2	(ULTRIX)	9/1/88";
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
 * action.c
 *
 * Name:	starship
 * Purpose:	General data structure manipulation routines are in
 *		this file.  They initialize the shared data structures &
 *		update them.
 * Environment:	Ultrix-32, optionally with shared memory
 * Compile:	see Makefile
 * Date:	April 18, 1985
 * Author:	Alan Delorey
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
 *	Derived from former file "two.c" which had ship action & screen
 *	update routines together.  Ship action & screen update are now
 *	split into action.c & screen.c
 */


#include "star.h"
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <curses.h>	/* also includes <sgtty.h> and <stdio.h> */
#include <sys/time.h>

/* external variables from main.c */

extern struct univ *uptr;		/* ptr to shared memory segment */
#ifdef SHMEM
extern int shmid;			/* used to remove shared seg */
#endif SHMEM

/* Define new access paths to variables that are now in one struct,
 * to facilitate shared memory
 */
#define universe uptr->quadrants
#define num_users uptr->Num_users
#define wait uptr->Wait
#define ships uptr->Ships
#define drones uptr->Drones

extern char esc;
extern char bell;		/* bell char */
extern boolean userabort;		/* true if abort-type exit */
extern int errno;

/* local variables */

struct timeval times;	/* time structures from sys/time.h */
struct timezone timez;
int seed;			/* random number seed */

/*****************************************************************************/
/*									     */
/*    		START INTERNAL PROCEDURES				     */
/*									     */
/*****************************************************************************/


get_rand()

    /* Get a random number according to the host system's random number
     * facility.  Normalize it to be between 0 and 9 inclusive.
     */

    {
    return (random() % 10);
    };

star_random()

    /* Randomly set up the stars in the galaxy.  There will be NUMSTARS stars
       in the galaxy.
    */

    
    {
    register int i,j;	/* loop indicies */
    long int holdran [4];	/* to hold random integers */

    for (i = 0; i < NUMSTARS; i ++)
	{
	for (j = 0; j < 4; j ++)	/* 4 random #'s per star */
	    holdran[j] = get_rand();

	/* assign stars to global universe data structure */

	universe [holdran[0]][holdran[1]].sectors [holdran[2]][holdran[3]].image = STAR;
	universe [holdran[0]][holdran[1]].nstars++;
	};
    };  /* proc star random */


base_random()

    /* Randomly set up the star bases in the galaxy.
       There will be NUMBASES of them.
    */

    
    {
    register int i,j;	/* loop indices */
    long int holdran[4];	/* to hold 4 random integers per base */

    for (i = 0; i < NUMBASES; i ++)
	{
	for (j = 0; j < 4; j ++)	/* 4 ran #s per base */
	    {
	    holdran[j] = get_rand();
	    };

	/* Assign bases to global universe */

	universe [holdran[0]][holdran[1]].sectors [holdran[2]][holdran[3]].image = BASE;
	universe [holdran[0]][holdran[1]].nbases++;
	};
    };  /* proc base random */


ion_random()

    /* Randomly disperse a few ion clouds in the galaxy.
     * There will be NUMCLOUDS of them.
     */

    {
    register i,j;		/* loop indicies */
    register rt, ct;	/* row temp & col temp */
    long int hr[4];		/* to hold 4 random ints per ion */

    /* do NUMCLOUDS; 4 random #'s to start each ion cloud */

    for (i = 0; i < NUMCLOUDS; i++)
	{
	for (j = 0; j < 4; j++)
	    hr[j] = get_rand();
	universe [hr[0]][hr[1]].sectors[hr[2]][hr[3]].image = ION;

	/* 2. same row, col + 1 */
	ct = hr[3] + 1;
	if (ct > NUMSECTORS - 1)
	    ct = hr[3] - 1;
	universe [hr[0]][hr[1]].sectors[hr[2]][ct].image = ION;

	/* 3. row + 1, col - 1 */
	rt = hr[2] + 1;
	if (rt > NUMSECTORS - 1)
	    rt = hr[2] - 1;
	ct = hr[3] - 1;
	if (ct < 0)
	    ct = NUMSECTORS - 1;
	universe [hr[0]][hr[1]].sectors[rt][ct].image = ION;

	/* 4. row + 1, same col */
	universe [hr[0]][hr[1]].sectors[rt][hr[3]].image = ION;

	/* 5. row + 1, col + 1 */
	ct = hr[3] + 1;
	if (ct > NUMSECTORS - 1)
	    ct = hr[3] - 1;
	universe [hr[0]][hr[1]].sectors[rt][ct].image = ION;

	/* 6. row + 2, same col */
	rt = hr[2] + 2;
	if (rt > NUMSECTORS - 1)
	    rt = hr[2] - 1;
	universe [hr[0]][hr[1]].sectors[rt][hr[3]].image = ION;
	}
    };


init_d_s()

    /* This procedure initializes the global, shared data structures.
       It calls star_random & base_random.
    */

    
    {
	register int i,j,k,l;	/* loop indicies */
	char ch;		/* loop index */

    esc = (char)ASCII_ESC;

    /* initialize random number generator */

    gettimeofday (&times,&timez);
    seed = times.tv_sec;
    srandom(seed);

    /* initialize "universe" - by looping thru all quadrants & sectors */

    for (i = 0; i < NUMQUADS; i ++)	/* loop thru all quadrants */
	for (j = 0; j < NUMQUADS; j ++)
	    {
	    universe [i][j].nstars = 0;
	    universe [i][j].nships = 0;
	    universe [i][j].nbases = 0;
	    for (k = 0; k < NUMSECTORS; k ++)	/* loop thru all sectors */
		for (l = 0; l < NUMSECTORS; l ++)
			universe [i][j].sectors [k][l].image = ' ';
	    }

    /* blank out all ship letters */

    for (ch = 'A'; ch <= 'Z'; ch ++)
	ships[ch-A].ship_ch = ' ';

    /* call star_random (to set up stars), 
     * base random (to set up star bases),
     * ion_random (to set up ion clouds).
     */

    star_random();
    base_random();
    ion_random();

#   ifdef DRONE
    for (i = 0; i < NUMDRONES; i++)
	{
	drones[i].d_char = ' ';
	drones[i].d_after = ' ';
	};
#   endif DRONE

    /* THIS LINE MUST BE LAST STMT IN THIS PROCEDURE !! */

    wait = false;
    };  /* proc init_d_s */


init_a_ship (sch,s_name)
    char sch;
    char *s_name;

    /* Initializes a new ship that enters the game.  It gives a random quadrant
       and sector position to the ship.
    */

    
    {
    register struct ship *pships;	/* ptr to ships */
    register struct torpedo_record *ptorps;	/* ptr to ships */
    register int i,j;	/* loop index */
    boolean open;		/* set false if sector occupied */
    long int holdran[4];	/* holds ran #s */
    char ch;		/* loop index */

    /* initialize random number generator */

    gettimeofday (&times,&timez);
    seed = times.tv_sec;
    srandom(seed);

    do
	{
	open = true;
	for (j = 0; j < 4; j ++)
	    holdran[j] = (random() % 10);
	if (universe [holdran[0]][holdran[1]].sectors [holdran[2]][holdran[3]].image != ' ')
	     open = false;
        } 
    while (!open);

    /* set up the ship info */

    pships = ships + (sch - A);
    pships->ship_ch = sch;
    strcpy(pships->name, s_name);
    pships->energy = ENERGY_MAX;
    pships->shields = 0;
    pships->shield_time = 0;
    pships->torpedos = TORP_MAX;
    pships->torp_time = 0;
    pships->phasers = 0;
    pships->warp_drive = 0;
    pships->warp_set = 0.0;
    pships->warp_speed = 0.0;

    pships->q_row = holdran[0];
    pships->q_col = holdran[1];
    pships->s_row = holdran[2];
    pships->s_col = holdran[3];

    pships->bearing = 6;
    pships->impulse = 0;
    pships->sub_light = 0;
    pships->life_support = GREEN;
    pships->life_supp_time = 0;
    pships->sr_scan = 0;
    pships->lr_scan = 0;
    pships->radio = 0;
    pships->sensors = 0;
    pships->cloaking = 0;
    pships->dock = false;
    pships->docked = false;
    pships->ship_invis = false;
    pships->cloak = false;
    pships->msg_buf[0] = '\0';
    pships->msg_timer = 0;
    pships->map_timer = 0;
    pships->kills = 0;
    pships->glow = 0;

    /* init torp info */

    for (i = 0; i < TORP_MAX; i ++)
	{
	ptorps = pships->torp_info + i;
	ptorps->torp_dir = -1;
	ptorps->torp_dist = 0;
	};
    pships->torp_active = false;

    /* set bases to "none seen" */

    for (i = 0; i < NUMBASES; i++)
	{
	pships->bases[i][0] = -1;
	pships->bases[i][1] = -1;
	}
    pships->allbases = false;
    pships->locater = -1;
    pships->locatec = -1;
    pships->radiorow = -1;
    pships->radiocol = -1;
    pships->starcounter = 0;

    /* set approp fields to true to trigger screen fields to be updated */

    pships->dis_energy = false;
    pships->dis_shields = false;
    pships->dis_torpedos = false;
    pships->dis_warp_set = false;
    pships->dis_warp_speed = false;
    pships->dis_quad = true;
    pships->dis_sector = true;
    pships->dis_bearing = false;
    pships->dis_msg = false;
    pships->dis_map = false;
    pships->dis_dir = false;

    /* Initialize pointers to strings for faster access (See K & R p 110.)
     * These strings are used to save long range scan info.
     * And initialize the "old" long range scan info.
     */
    
    for (i=0; i < LRSCANSECTS; i++)
       for (j=0; j < LRSCANSECTS; j++)
	   {
	   pships->old_lr[i][j] = pships->old_lr1 [i][j];
	   strcpy (pships->old_lr[i][j], "000");
	   }

#ifdef OLDSR
    /* Initialize "old" short range scan info */
    for (i=0; i < SRSCANROWS; i++)
	for (j=0; j < SRSCANCOLS; j++)
	    pships->old_sr [i][j] = ' ';
#endif

    pships->ioff = SRSCANR;
    pships->joff1 = SRSCANR;
    pships->joff2 = SRSCANR;

    /* make all ships re-display fleet display */

    for (ch = 'A'; ch <= 'Z'; ch ++)
	ships[ch-A].dis_fleet = true;

    universe[holdran[0]][holdran[1]].sectors [holdran[2]][holdran[3]].image = sch;
    universe[holdran[0]][holdran[1]].nships++;
    };  /* proc init_a_ship */


exit_a_ship (sch, drone)
    char sch;			/* ship to exit */
    boolean drone;		/* true if its a drone exiting */

    /* Exits a ship from the game.  Clear it from ships & universe.
     * If its a drone, don't clear screen.
     */

    
    {
    register int i;				/* loop index */
    register struct ship *pships;		/* ship pointer */
    register struct torpedo_record *ptorps;	/* ship pointer */
    register struct drone *pdrones;		/* ship pointer */
    char ch;	/* loop index */

    pships = ships + (sch - A);
    if (!drone)
	{
	if (userabort)
	    {
	    clear_screen ();
	    /* everyone gets a score for a QUITER! */
	    for (ch = 'A'; ch <= 'Z'; ch++)
		ships[ch - A].kills++;
	    }
	else
	    if (pships->life_support == RED)
		  {
		  move_to(COMMANDR+1,COMMANDC);
		  string_out("Life support exhausted - crew died");
		  move_to(COMMANDR+2,COMMANDC);
		  clear_to_eol();
		  }
	    else if (pships->energy <= 0)
		      {
		      move_to(COMMANDR+1,COMMANDC);
		      string_out("Out of energy - you lost    ");
		      move_to(COMMANDR+2,COMMANDC);
		      clear_to_eol();
		      }
	}

    refresh_scr();
    universe [pships->q_row][pships->q_col].
	      sectors [pships->s_row][pships->s_col].image = '-';
    universe [pships->q_row][pships->q_col].nships--;

    /* clean up torpedos */
    /* as a side effect of a ship dying-all of its torps magically disappear! */

    for (i = 0; i < TORP_MAX; i ++)
	{
	ptorps = pships->torp_info + i;
	if (ptorps->torp_dir != -1)	/* if torp active */
	    universe[ptorps->tquadr][ptorps->tquadc].
		sectors[ptorps->tsectr][ptorps->tsectc].image = ' ';
	};

    pships->ship_ch = ' ';  /* last, to avoid timing problems */

    /* force fleet display to be re-displayed */

    for (ch = 'A'; ch <= 'Z'; ch ++)
	ships[ch-A].dis_fleet = true;

#   ifdef DRONE
    for (i = 0; i < NUMDRONES; i++)
	if (drones[i].d_after == sch)
	    {
	    pdrones = drones + i;
	    exit_a_ship (pdrones->d_char, true);
	    pdrones->d_char = ' ';
	    pdrones->d_after = ' ';
	    pdrones->d_busy = false;
	    }
#   endif DRONE

    /*
     * If using shared memory, and this is the last user exiting,
     * remove the shared memory segment and return immediately.
     */

    num_users--;

#   ifdef SHMEM
    if (num_users <= 0 && !drone)
	if (shmctl(shmid, IPC_RMID) < 0)
	    printf("shmctl call to remove shared seg failed\n");
#   endif SHMEM
    };


damage (ship_ch, damage_amt)
    char ship_ch;
    int damage_amt;

    /* Randomly picks a device to be damaged.  The length of time the device
       is damaged is: DAMAGE_CONST seconds per 100 units of energy that
       penetrated the shields (damage_amt).

       Params:
	ship_ch - The ship that is damaged.
	damage_amt - amount of energy that penetrated the shields.
    */

    
    {
    register int damage_time;    /* # of seconds that device is out for */
    register struct ship *pships;
    long int holdran;	/* random # for device damaged */

    holdran = get_rand();
    damage_time = DAMAGE_CONST * (damage_amt / 100);
    pships = ships + (ship_ch - A);
    switch (holdran)
	{
	case 0:
	    pships->shield_time += damage_time;
	    pships->dis_shields = false;
	    break;
	case 1:
	    pships->torp_time += damage_time;
	    pships->dis_torpedos = false;
	    break;
	case 2:
	    pships->phasers += damage_time;
	    break;
	case 3:
	    pships->warp_drive += damage_time;
	    break;
	case 4:
	    pships->impulse += damage_time;
	    break;
	case 5:
	    pships->life_supp_time += damage_time;

	    switch (pships->life_support)
		{
		case GREEN:
		    pships->life_support = YELLOW;
		    break;
		case YELLOW:
		    pships->life_support = RED;
		    break;
		case RED:
		    /* force death of ship */
		    pships->energy = -1000;
		    pships->shields = 0;
		    break;
		};
	    /* sensors doubled up with life support */
	    pships->sensors += damage_time;
	    break;
	case 6:
	    pships->sr_scan += damage_time;
	    break;
	case 7:
	    pships->lr_scan += damage_time;
	    break;
	case 8:
	    pships->radio   += damage_time;
	    break;
	case 9:
	    pships->cloaking  += damage_time;
	    pships->cloak = false;
	    break;
	};  /* switch */
    }; /* PROC damage */


ship_action (ship_ch)
    char ship_ch;

    /* Perform all ship actions:
	    change_speed
	    move_torps
	    move_ship
	    dock_ship

       Param: ship_ch - The calling ship, the one to move.
    */

    {
    register struct ship *pships;

    pships = ships + (ship_ch - A);
    if (pships->cloak)
	{
	if (pships->energy > CLOAK_COST)
	    pships->energy -= CLOAK_COST;
	else pships->cloak = false;
	}
    if (pships->warp_speed != pships->warp_set)
	 change_speed(ship_ch);
    else if (((pships->energy + pships->shields) < ENERGY_MAX) && (pships->warp_drive == 0))
	     {
	     pships->energy = pships->energy + CONSTSP_SAVE;
	     pships->dis_energy = true;
	     if ((pships->energy + pships->shields) > ENERGY_MAX)
		  /* don't allow > 50,000 */
		  pships->energy = pships->energy - 
			((pships->energy + pships->shields) - ENERGY_MAX);
	     };

    if (pships->torp_active)
	 move_torps(ship_ch);

    if (pships->warp_speed > 0)
	if (pships->warp_speed < 1)
	    if (pships->warp_speed <= 0.3)
	        if (pships->sub_light >= 4)
		    {
		    move_ship(ship_ch);
		    pships->sub_light = 1;
		    }
		else pships->sub_light++;
	    else 
		if (pships->warp_speed <= 0.6)
		    if (pships->sub_light >= 3)
			{
			move_ship(ship_ch);
			pships->sub_light = 1;
			}
		    else pships->sub_light++;
		else
		    /* speed is 0.7 to 0.9 */
		    if (pships->sub_light >= 2)
		         {
		         move_ship(ship_ch);
		         pships->sub_light = 1;
		         }
		    else 
		        /* incr counter/timer */
		        pships->sub_light++;
	 /* warp speed >= 1, so just move ship */
	 else move_ship(ship_ch);

    if (pships->dock && (pships->warp_speed == 0))
	 dock_ship(ship_ch);
    };  /* proc ship_action */


change_speed(ship_ch)
    char ship_ch;

	/* Increase or decrease the ship's speed by 0.5 warp.  This gives an
	   acceleration (or deceleration) rate of 0.5 warp per second.
	*/

	{
	register struct ship *pships;

	pships = ships + (ship_ch - A);
	if (pships->energy >= CHSPEED_COST)
	    {
	    pships->energy = pships->energy - CHSPEED_COST;
	    pships->dis_energy = true;
	    if (pships->warp_speed > pships->warp_set)
		 /* decrease speed */
		 {
		 pships->dis_warp_speed = true;
		 if (pships->warp_speed >= (pships->warp_set + SPEED_CHAMT))
		      pships->warp_speed = pships->warp_speed - SPEED_CHAMT;
		 else pships->warp_speed = pships->warp_set;
		 }
	    else if (pships->warp_speed < pships->warp_set)
		     /* increase speed */
		     {
		     pships->dis_warp_speed = true;
		     if (pships->warp_speed <= (pships->warp_set - SPEED_CHAMT))
			  pships->warp_speed = pships->warp_speed + SPEED_CHAMT;
		     else pships->warp_speed = pships->warp_set;
		     };
	    }
	else {
	     strcpy (pships->msg_buf,"Not enough energy");
	     pships->dis_msg = true;
	     };
        };  /* proc change_speed */


move_torps(ship_ch)
    char ship_ch;

	/* move the ship's torpedos within the global data base (in universe &
	   in ships).
	*/

	{
	register struct ship *pships;
	register struct torpedo_record *ptorps;
	register int i,j;	/* loop index */
	register int csr;	/* current sector row */
	register int csc;	/* current sector col */
	int cqr;		/* current quad row */
	int cqc;		/* current quad col */
	int hurt;	/* energy units that penetrated a ship's shields */
	int nsectors; 	/* # of sectors to move the ship */
	boolean expire;	/* set true if (torp hits a ship, BASE or STAR.)
	    			Based on this flag, torp_dir is set to -1.
	    			This flag is used to avoid a race condition
	    			  with scanf_input firing torpedos */
	char uchar;	/* current universe.image char */


	 pships = ships + (ship_ch - A);
	 pships->torp_active = false;	/* assume none active */

	 /* loop thru all posible torpedos for this ship */

	 for (j = 0; j < TORP_MAX; j ++)
	    {
	    ptorps = pships->torp_info + j;
	    if (ptorps->torp_dir != - 1)
		 {	/* torp active here */
		 pships->torp_active = true;
		 expire = false;

		 /* must blank torpedo from old position if it was not just fired */
	
		 if (ptorps->torp_dist != 0)
		    universe[ptorps->tquadr][ptorps->tquadc].
			sectors [ptorps->tsectr][ptorps->tsectc].image =' ';

		 cqr = ptorps->tquadr;
		 cqc = ptorps->tquadc;
		 csr = ptorps->tsectr;
		 csc = ptorps->tsectc;
		 nsectors = ptorps->torp_speed;

		 /* loop to move torp until all sectors traversed or 
		    torp hits something solid */

		 i = 1;
		 for (; (i <= nsectors) && (ptorps->torp_dist < TORP_TRAVEL) ;)
		    {
		    switch (ptorps->torp_dir)
		    {
		    case 1:
		       csr = csr + 1;
		       csc = csc - 1;
		       break;
		    case 2:
			csr = csr + 1;
			break;
		    case 3:
		       csr = csr + 1;
		       csc = csc + 1;
			break;
		    case 4:
			csc = csc - 1;
			break;
		    case 6: csc = csc + 1;
			break;
		    case 7:
		        csr = csr - 1;
		        csc = csc - 1;
			break;
		    case 8:
			csr = csr - 1;
			break;
		    case 9:
		        csr = csr - 1;
		        csc = csc + 1;
			break;
		    default:  ;	/* do nothing */
		    };  /* switch */

		    /* check for entering adjacent quadrant */

		    if (csr < 0)
			{	/* goto bottom edge of above quad */
			csr = NUMSECTORS -1;
			cqr--;
			pships->dis_quad = true;
			if (cqr < 0)
			     cqr = NUMQUADS -1;
			};

		    if (csc < 0)
			{	/* goto right edge of prev quad */
			csc = NUMSECTORS -1;
			cqc = cqc - 1;
			pships->dis_quad = true;
			if (cqc < 0)
			     cqc = NUMQUADS -1;
			};

		    if (csr > NUMSECTORS -1)
			{	/* goto top edge of quad below */
			csr = 0;
			cqr = cqr + 1;
			pships->dis_quad = true;
			if (cqr > NUMQUADS -1)
			     cqr = 0;
			};

		    if (csc > NUMSECTORS -1)
			{	/* goto left edge of next quad */
			csc = 0;
			cqc = cqc + 1;
			pships->dis_quad = true;
			if (cqc > NUMQUADS -1)
			     cqc = 0;
			};

		    /* check for anything in the path of movement */

		    uchar = universe [cqr][cqc].sectors [csr][csc].image;
		    switch (uchar)
		    {
		    case ' ':
			  i = i + 1;
			  ptorps->torp_dist++;
			  break;
		    case TORP: 		/* pass thru */
			  i++;
			  ptorps->torp_dist++;
			  break;
		    case BASE:
		    case STAR: 	/* torp absorbed */
			  i = nsectors + 1;  /* force end of while loop */
			  expire = true;
			  break;
		    case ION:  		/* clear_screen path */
			  i++;
			  ptorps->torp_dist++;
			  universe [cqr][cqc].sectors [csr][csc].image=' ';
			  break;
		    default:
			  i = nsectors + 1;
			  expire = true;

			  if (uchar >= 'A' && uchar <= 'Z')
			       {	/* hit a ship! */
			       pships = ships + uchar - A;
			       pships->dis_shields = true;
			       pships->glow = 1;
			       pships->cloak = false;
			       strcpy(pships->msg_buf,
				   "Ship hit by Photon Torpedo");
			       pships->dis_msg = true;
			       if (pships->shields >= TORP_COST)
				    pships->shields = pships->shields-TORP_COST;
			       else {
				    hurt = TORP_COST - pships->shields;
				    pships->energy = pships->energy - hurt;
				    pships->shields = 0;
				    pships->dis_energy = true;
				    damage (uchar, hurt);

				    /* give the guy a kill! */
				    if (pships->energy <= 0)
				       {
				       universe[pships->q_row][pships->q_col].sectors
					    [pships->s_row][pships->s_col].image = '-';
				       pships = ships + (ship_ch - A);
				       pships->kills++;
				       sprintf (pships->msg_buf, "The %s is destroyed",ships[uchar-A].name);
				       pships->dis_msg = true;
				       }
				    }
			       }
			    break;
		}  /* switch */

		}  /* while i <= nsectors */

	    /* update torpedos posit in universe and in ships record */

	    pships = ships + (ship_ch - A);
	    if (! expire && (ptorps->torp_dist < TORP_TRAVEL))
		 {
		 universe[cqr][cqc].sectors [csr][csc].image = TORP;
		 ptorps->tquadr = cqr;
		 ptorps->tquadc = cqc;
		 ptorps->tsectr = csr;
		 ptorps->tsectc = csc;
		 }
	    else ptorps->torp_dir = -1;	/* set torp record free */

	    };  /* torp active */
	};  /* for j = 0; j < TORP_MAX */
	};  /* proc move_torps */


move_ship(ship_ch)
    char ship_ch;

	/* move the ship's position within the global data base (in universe &
	   in ships).

	   Param: ship_ch - The calling ship, the one to move.
	*/

	
	{
	register struct ship *pships;
	register int cqr;		/* current quad row */
	register int cqc;		/* current quad col */
	register int csr;		/* current sector row */
	register int csc;		/* current sector col */
	register int i;	/* loop index */
	int holdqr;	/* hold prev posit in case ship hits base or ship */
	int holdqc;
	int holdsr;
	int holdsc;
	int hurt;	/* energy units that penetrated a ship's shields */
	int nsectors; 	/* # of sectors to move the ship */
	char uchar;	/* universe char at current coordinates */

	 pships = ships + (ship_ch - A);
	 /* must blank ship first, in case ship hit something & didn't move */
	
	 if (! pships->ship_invis)
	      universe[pships->q_row][pships->q_col].sectors
		    [pships->s_row][pships->s_col].image = ' ';
	 else pships->ship_invis = false;

	 cqr = pships->q_row;
	 cqc = pships->q_col;
	 csr = pships->s_row;
	 csc = pships->s_col;
	 if (pships->warp_speed < 1)	/* sublight */
	      nsectors = 1;
	 else nsectors = (pships->warp_speed + 0.4);  /* add 0.4 to effectively round */

	 /* move until all sectors traversed or ship hits something solid */

	 for (i=1; i <= nsectors ;)
	    {

	    /* hold position in case ship hits a base or another ship */

	    holdqr = cqr;
	    holdqc = cqc;
	    holdsr = csr;
	    holdsc = csc;

	    switch (pships->bearing)
	    {
	    case 1:
	       csr++;
	       csc--;
		break;
	    case 2:
		csr++;
		break;
	    case 3:
	       csr++;
	       csc++;
		break;
	    case 4: csc--;
		break;
	    case 6:
		csc++;
		break;
	    case 7:
	       csr--;
	       csc--;
		break;
	    case 8:
		csr--;
		break;
	    case 9:
	        csr--;
	        csc++;
		break;
	    default:  ;	/* do nothing */
	    };  /* switch */

	    /* check for entering adjacent quadrant */

	    if (csr < 0)
		 {	/* goto bottom edge of above quad */
		csr = NUMSECTORS -1;
		cqr = cqr - 1;
		pships->dis_quad = true;
		if (cqr < 0)
		     cqr = NUMQUADS -1;
		};

	    if (csc < 0)
		 {	/* goto right edge of prev quad */
		csc = NUMSECTORS -1;
		cqc = cqc - 1;
		pships->dis_quad = true;
		if (cqc < 0)
		     cqc = NUMQUADS -1;
		};

	    if (csr > NUMSECTORS -1)
		 {	/* goto top edge of quad below */
		csr = 0;
		cqr = cqr + 1;
		pships->dis_quad = true;
		if (cqr > NUMQUADS -1)
		     cqr = 0;
		};

	    if (csc > NUMSECTORS -1)
		 {	/* goto left edge of next quad */
		csc = 0;
		cqc = cqc + 1;
		pships->dis_quad = true;
		if (cqc > NUMQUADS -1)
		     cqc = 0;
		};

	    /* check for anything in the path of movement */

	    uchar = universe [cqr][cqc].sectors [csr][csc].image;
	    switch (uchar)
	    {
	    case ' ':
		 i++;
		 pships->docked = false;
		 break;
	    case TORP:
		   i++;
		   pships->docked = false;
		   pships->dis_shields = true;
		   if (pships->shields >= TORP_COST)
			pships->shields = pships->shields - TORP_COST;
		   else {
			hurt = TORP_COST - pships->shields;
			pships->energy = pships->energy - hurt;
			pships->shields = 0;
			pships->dis_energy = true;
			damage (ship_ch, hurt);
			strcpy(pships->msg_buf, "Ship hit by Photon Torpedo");
			pships->dis_msg = true;
			};
		    /* NOTE: there is currently no way to terminate the
		       torpedo, since the ship hit it (ship moving > w1)
		       the torp owner is unknown! */
		  break;
	    case STAR:
		  i++;
		  pships->docked = false;
		  strcpy(pships->msg_buf, "Passing thru a star!");
		  pships->dis_msg = true;
		  pships->ship_invis = true;
		  pships->dis_shields = true;
		  pships->starcounter = 0;
		  if (pships->shields >= STAR_COST)
		       /* no damage */
		       pships->shields = pships->shields - STAR_COST;
		  else {
		       hurt = STAR_COST - pships->shields;
		       pships->energy = pships->energy - hurt;
		       pships->shields = 0;
		       pships->dis_energy = true;
		       damage (ship_ch, hurt);
		       };
		  break;
	    case ION:
		  /* ship in ion cloud */
		  i++;
		  pships->docked = false;
		  strcpy(pships->msg_buf, "Passing thru an ion cloud!");
		  pships->dis_msg = true;
		  pships->ship_invis = true;
		  pships->dis_shields = true;
		  if (pships->shields >= ION_COST)
		       /* no damage */
		       pships->shields = pships->shields - ION_COST;
		  else {
		       hurt = ION_COST - pships->shields;
		       pships->energy = pships->energy - hurt;
		       pships->shields = 0;
		       pships->dis_energy = true;
		       damage (ship_ch, hurt);
		       };
		  break;
	    case BASE:
		  i = nsectors + 1;	/* to trigger end of for loop */
		  if (pships->warp_speed >= 1.0)
		      {
		      pships->dis_bearing = true;
		      switch (pships->bearing)
			  {
			  case 1:
			      pships->bearing = 9;
			      break;
			  case 2:
			      pships->bearing = 8;
			      break;
			  case 3:
			      pships->bearing = 7;
			      break;
			  case 4:
			      pships->bearing = 6;
			      break;
			  case 6:
			      pships->bearing = 4;
			      break;
			  case 7:
			      pships->bearing = 3;
			      break;
			  case 8:
			      pships->bearing = 2;
			      break;
			  case 9:
			      pships->bearing = 1;
			      break;
			  }
			}
		  else	/* warp speed < 1, so stop */
		      {
		      pships->warp_speed = 0.0;
		      pships->warp_set = 0.0;
		      pships->dis_warp_speed = true;
		      pships->dis_warp_set = true;
		      }
		  pships->dis_shields = true;
		  if (pships->shields >= BASE_COST)
		       /* no damage */
		       pships->shields = pships->shields - BASE_COST;
		  else {
		       hurt = BASE_COST - pships->shields;
		       pships->energy = pships->energy - hurt;
		       pships->shields = 0;
		       pships->dis_energy = true;
		       damage (ship_ch, hurt);
		       };

		  /* ship hit a star base, set position back */

		  cqr = holdqr;
		  cqc = holdqc;
		  csr = holdsr;
		  csc = holdsc;
		  strcpy(pships->msg_buf, "Collision with a star base!");
		  pships->dis_msg = true;
		  break;
	    default:
		  i = nsectors + 1;
		  if (uchar >= 'A' && uchar <= 'Z')
		      {    /* collision with another ship set position back */
		      pships->dis_bearing = true;
		      switch (pships->bearing)
			  {
			  case 1:
			      pships->bearing = 9;
			      break;
			  case 2:
			      pships->bearing = 8;
			      break;
			  case 3:
			      pships->bearing = 7;
			      break;
			  case 4:
			      pships->bearing = 6;
			      break;
			  case 6:
			      pships->bearing = 4;
			      break;
			  case 7:
			      pships->bearing = 3;
			      break;
			  case 8:
			      pships->bearing = 2;
			      break;
			  case 9:
			      pships->bearing = 1;
			      break;
			  }
		      pships->dis_shields = true;
		      if (pships->shields >= SHIP_COST)
			   /* no damage */
			   pships->shields = pships->shields - SHIP_COST;
		      else {
			   hurt = SHIP_COST - pships->shields;
			   pships->energy = pships->energy - hurt;
			   pships->shields = 0;
			   pships->dis_energy = true;
			   damage (ship_ch, hurt);
			   };

			cqr = holdqr;
			cqc = holdqc;
			csr = holdsr;
			csc = holdsc;
			sprintf(pships->msg_buf, "Collision with the %s!",
				ships[uchar - A].name);
			pships->dis_msg = true;

			pships = ships + (uchar - A);
			/* 
			 * If the 2 ships were traveling in the same
			 * direction, then with the 1st one's bearing reversed
			 * they will be going in opposite directions,
			 * indicated by sum of bearings == 10.
			 */
			if (pships->bearing + ships[ship_ch - A].bearing != 10)
			    {
			    pships->dis_bearing = true;
			    switch (pships->bearing)
			      {
			      case 1:
				  pships->bearing = 9;
				  break;
			      case 2:
				  pships->bearing = 8;
				  break;
			      case 3:
				  pships->bearing = 7;
				  break;
			      case 4:
				  pships->bearing = 6;
				  break;
			      case 6:
				  pships->bearing = 4;
				  break;
			      case 7:
				  pships->bearing = 3;
				  break;
			      case 8:
				  pships->bearing = 2;
				  break;
			      case 9:
				  pships->bearing = 1;
				  break;
			      }
			    }
			sprintf(pships->msg_buf, "Collision with the %s!",
				ships[ship_ch - A].name);
		        pships->dis_msg = true;
		        };
			break;
	    };  /* switch */

	    };  /* for i <= nsectors */

	/* update ships posit in universe and in ships record */

	pships = ships + (ship_ch - A);
	if (! pships->ship_invis)
	     universe[cqr][cqc].sectors [csr][csc].image = ship_ch;

	/* Update lrscan: for simplicity assume the ship changed quad's.
	   Decrement ships count in old quad, increm ship count in new quad.*/

	universe [pships->q_row][pships->q_col].nships--;

	universe [cqr][cqc].nships++;

	pships->q_row = cqr;
	pships->q_col = cqc;

	pships->s_row = csr;
	pships->s_col = csc;

	};  /* proc move_ship */


dock_ship(ship_ch)
    char ship_ch;

	/* Docks the ship & restores all of the ships systems.
	*/

	
	{
	register struct ship *pships;
	register int i;		/* loop index */
	register int quadr;	/* posit to check for base */
	register int quadc;
	register int sectr;
	register int sectc;
	boolean found;	/* set true if base found */

	pships = ships + (ship_ch - A);
	pships->dock = false;
	i = 1;
	found = false;

	for (; i <= 4 && !found ;)
	    {	/* check all 4 directions */
	    quadr = pships->q_row;
	    quadc = pships->q_col;
	    sectr = pships->s_row;
	    sectc = pships->s_col;

	    /* check next direction of the 4 possible directions */

	    switch (i)
	    {
	    case 1:			/* S+ */
		sectc++;
		if (sectc > NUMSECTORS -1)
		     {
		     sectc = 0;
		     quadc = quadc + 1;
		     if (quadc > NUMQUADS -1)
			  quadc = 0;
		     };
		break;
	    case 2:  			/* +S */
		sectc--;
		if (sectc < 0)
		     {
		     sectc = NUMSECTORS -1;
		     quadc = quadc - 1;
		     if (quadc < 0)
			  quadc = NUMQUADS -1;
		     };
		break;
	    case 3:  			/* + */
		sectr--;		/* S */
		if (sectr < 0)
		     {
		     sectr = NUMSECTORS -1;
		     quadr = quadr - 1;
		     if (quadr < 0)
			  quadr = NUMQUADS -1;
		     };
		break;
	    case 4:  			/* S */
		sectr++;		/* + */
		if (sectr > NUMSECTORS -1)
		     {
		     sectr = 0;
		     quadr = quadr + 1;
		     if (quadr > NUMQUADS -1)
			  quadr = 0;
		     };
	    }; /* switch (*/

	    if (universe[quadr][quadc].sectors[sectr][sectc].image == BASE)
		 found = true;
	    else i++;
	    };  /* while */

	if (found)
	     {	/* dock */
	    pships->docked = true;
	    strcpy(pships->msg_buf, "Docking at star base");
	    pships->dis_msg = true;
	    pships->energy = ENERGY_MAX;

	    /* if devices not operational, set time to 1 & they'll go opr */

	    pships->shields = 0;
	    if (pships->shield_time > 1)
		 pships->shield_time = 1;
	    pships->torpedos = TORP_MAX;
	    if (pships->torp_time > 1)
		 pships->torp_time = 1;
	    if (pships->phasers > 1)
		 pships->phasers = 1;
	    if (pships->warp_drive > 1)
		 pships->warp_drive = 1;
	    pships->warp_speed = 0.0;
	    if (pships->impulse > 1)
		 pships->impulse = 1;
	    pships->life_support = GREEN;
	    if (pships->sr_scan > 1)
		 pships->sr_scan = 1;
	    if (pships->lr_scan > 1)
		 pships->lr_scan = 1;
	    if (pships->radio > 1)
		 pships->radio = 1;
	    pships->cloak = false;
	    if (pships->cloaking > 1)
		 pships->cloaking = 1;
	    if (pships->sensors > 1)
		 pships->sensors = 1;

	    /* set display fields */

	    pships->dis_energy = true;
	    pships->dis_shields = true;
	    pships->dis_torpedos = true;
	    }  /* dock */
	else {
	    strcpy(pships->msg_buf, "No star base to dock at!");
	    pships->dis_msg = true;
	    };
    };  /* proc dock_ship */
