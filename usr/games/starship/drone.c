#ifndef lint
static char	*sccsid = "@(#)drone.c	4.1	(ULTRIX)	11/23/87";
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
 * drone.c
 *
 * Name:	starship
 * Purpose:	Drone ship control code is in this file.
 * Environment:	Ultrix-32, optionally with shared memory.
 * Compile:	see Makefile
 * Date:	April 18 1985
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
 *	Renamed from three.c to drone.c
 *
 */

#include "star.h"
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

/* Global to the program */

extern struct univ *uptr;		/* ptr to global data struct */
extern int dafter;			/* decr once for ea drone exited */

/* define new access paths to variables that are now in the "univ" struct */
#define universe uptr->quadrants
#define num_users uptr->Num_users
#define wait uptr->Wait
#define ships uptr->Ships
#define drones uptr->Drones



/********************************************************************/


call_drone (dch, sch, level)
    char dch;		/* drone char */
    char sch;		/* player's ship char */
    int level;		/* level of difficulty (speed) */

    /* Attempts to find an open slot in drone array. If a slot is found
     * it calls `act_drone' to activate the drone ship.  Otherwise drone
     * will reply that it is busy.
     */

    {
    register int i;	/* loop index */
    register struct ship *pships;	/* ship pointer */
    boolean open;	/* set true when open slot found in drones */

    /*
     * If the drone that was requested is already being used,
     * set "open" to false.	Added 4-2-85 along with shared mem support.
     */
    open = true;
    for (i = 0; i < NUMDRONES; i++)
	{
	if (drones[i].d_char == dch)
	   {
	   open = false;
	   break;
	   }
	}
    /* 
     * If we just determined that the requested drone is free,
     * find an open slot in the drone array to place it.
     * RACE CONDITION POTENTIAL: if more than 1 process calls the same
     *    drone at about the same time, they can both find it free above
     *    before one of them makes it "busy" in "act_drone".
     */
    if (open)
	{
	open = false;
	for (i = 0; i < NUMDRONES; i++)
	    {
	    if (drones[i].d_char == ' ')
	       {
	       open = true;
	       break;
	       }
	    }
	}

    if (open)
	act_drone (i, dch, sch, level);
    else
	{
	pships = ships + (sch - A);
	strcpy (pships->msg_buf, "I'm busy");
	pships->dis_msg = true;
	}
    }

act_drone (d, dch, sch, level)
    int d;		/* # of this drone entry */
    char dch;		/* drone char */
    char sch;		/* player's ship char */
    int level;		/* level of difficulty (speed) */

    /* This routine locates the drone ship one quadrant to the left of the
     * player who called the drone.  It sets up the initial characteristics
     * of the drone ship and sends him off.
     */

    {
    register struct ship *pships;	/* ship pointer */
    register struct drone *pdrones;	/* drone pointer */
    register int q_row, q_col, s_row, s_col;	/* temps to set drone down */
    boolean open;		/* set true when open spot found to set drone */

    num_users++;
    pdrones = drones + d;
    pdrones->d_after = sch;
    pdrones->d_char = dch;
    if (dch == 'K')
	strcpy (pdrones->d_name, "KLINGON");
    else
	strcpy (pdrones->d_name, "ROMULAN");
    pdrones->d_busy = true;
    pdrones->d_level = level;
    /*
     * set firing delay based on the difficulty level of the drone.
     */
    switch (level)
	{
	case 1:
	    pdrones->d_delay = 7;
	    break;
	case 2:
	    pdrones->d_delay = 5;
	    break;
	case 3:
	    pdrones->d_delay = 3;
	    break;
	case 4:
	    pdrones->d_delay = 2;
	    break;
	default:
	    pdrones->d_delay = 4;
	    break;
	}
    init_a_ship (dch, pdrones->d_name);

    pships = ships + (sch - A);
    strcpy (pships->msg_buf, "We are coming to destroy you!");
    pships->dis_msg = true;

    /* set drone in quad to player's left */

    q_row = pships->q_row;
    q_col = pships->q_col - 1;
    if (q_col < 0)
	q_col = NUMSECTORS - 1;
    s_row = SHDISPR;
    s_col = SHDISPC;
    open = false;

    /* find an open sector */

    while (!open)
	{
	if (universe[q_row][q_col].sectors[s_row][s_col].image == ' ')
	    open = true;
	else
	    {
	    s_col++;
	    if (s_col > NUMSECTORS - 1)
		{
		s_col = 0;
		s_row++;
		if (s_row > NUMSECTORS - 1)
		    s_row = 0;
		}
	    }
	}

    /* set the drone ship down here (clear where init_a_ship put it) */

    pships = ships + (dch - A);
    universe[pships->q_row][pships->q_col].sectors[pships->s_row][pships->s_col].image = ' ';
    universe[pships->q_row][pships->q_col].nships--;

    universe[q_row][q_col].sectors[s_row][s_col].image = dch;
    universe[q_row][q_col].nships++;
    pships->q_row = q_row;
    pships->q_col = q_col;
    pships->s_row = s_row;
    pships->s_col = s_col;
    pships->shields = 25000;
    pships->energy = 25000;
    }

run_drone (d)
    int d;	/* index into drone array for the drone thats after you */

    /* Makes all of the commands for the drone: Shields, Warp speed,
     * Bearing, Torpedos, Phasers.
     * It calls ship_action to carry out its moves.
     */

    {
    register struct ship *pships;	/* ptr to ship */
    register struct drone *pdrones;	/* ptr to drone */
    register struct torpedo_record *ptorps; /* ptr to torp rec */
    register int i;
    register int try;			/* index of dir's tried */
    int bearing, tbearing;		/* desired & alternate bearing */
    int dist;				/* dist to ship if found */
    boolean ontarget;			/* true if one target for torp */
    boolean found;			/* set true when clear bearing found */
    int torp_dir;			/* dir to fire torp; 0 = no torp */
    int phaser;				/* phaser amt; 0 = don't fire */
    int qrow, qcol, srow, scol;		/* drone's current position */
    int tqrow, tqcol, tsrow, tscol;	/* drone's desired new position */

    static int btrys [10][7] =
	{
	{0,0,0,0,0,0,0},	/* bearing 0 */
	{2,4,3,7,6,8,9},	/* bearing 1 */
	{1,3,4,6,7,9,9},	/* bearing 2 */
	{2,6,1,9,4,8,7},	/* bearing 3 */
	{1,7,2,8,3,9,6},	/* bearing 4 */
	{0,0,0,0,0,0,0},	/* bearing 5 */
	{3,9,2,8,1,7,4},	/* bearing 6 */
	{4,8,1,9,2,6,3},	/* bearing 7 */
	{7,9,4,6,1,3,2},	/* bearing 8 */
	{6,8,3,7,2,4,1} 	/* bearing 9 */
	};


    pdrones = drones + d;
    pships = ships + (pdrones->d_char - A);
    if (pships->energy <= 0 && pships->shields <= 0)
	{
	exit_a_ship (pdrones->d_char, true);
	pdrones->d_char = ' ';
	pdrones->d_after = ' ';
	pdrones->d_busy = false;
	dafter--;
	return;
	}

    /* restore speed to appropriate level & shields to 25k */

    switch (pdrones->d_level)
	{
	case 1:
	    pships->warp_set = 0.2;
	    break;
	case 2:
	    pships->warp_set = 0.5;
	    break;
	case 3:
	    pships->warp_set = 0.8;
	    break;
	case 4:
	    pships->warp_set = 1;
	    pships->sub_light = 0;
	    break;
	}
    if (pships->shields < 25000 && pships->shield_time == 0)
	{
	i = 25000 - pships->shields;
	if (i >= pships->energy)
	    i = pships->energy - 1;
	pships->energy -= i;
	pships->shields += i;
	}

    /*
     * Decrement time-to-repair any of the drones inoperative devices
     */
    if (pships->shield_time > 0)
	 pships->shield_time--;
    if (pships->torp_time > 0)
	 pships->torp_time--;
    if (pships->phasers > 0)
	 pships->phasers--;
    if (pships->warp_drive > 0)
	 pships->warp_drive--;
    if (pships->impulse > 0)
	 pships->impulse--;
    /*
     * If drone is glowing, time-out the glow field.
     */

    switch (pships->glow)
	{
	case 0:
	    break;
	case 1:
	case 2:
	    pships->glow++;
	    break;
	case 3:
	    pships->glow = 0;
	    break;
	default:
	    pships->glow = 0;
	    break;
	}

    /* Call shipinrange to select bearing & torp or phaser action */

    bearing = 6;
    torp_dir = 0;
    phaser = 0;
    ontarget = false;
    if (shipinrange(pdrones->d_char,pdrones->d_after,PHSRANGE,true,&dist,&bearing,&ontarget))
	{
	if (ontarget)
	    torp_dir = bearing;
	else if(dist <= 2)
	    phaser = dist;
	}

    tbearing = bearing;
    qrow = pships->q_row;
    qcol = pships->q_col;
    srow = pships->s_row;
    scol = pships->s_col;
    try = 0;

    /* Loop until a clear bearing is found */

    for (found = false; !found; )
	{
	tqrow = qrow;
	tqcol = qcol;
	tsrow = srow;
	tscol = scol;
	switch (tbearing)
	    {
	    case 1:
		tsrow++;
		tscol--;
		break;
	    case 2:
		tsrow++;
		break;
	    case 3:
		tsrow++;
		tscol++;
		break;
	    case 4:
		tscol--;
		break;
	    case 6:
		tscol++;
		break;
	    case 7:
		tsrow--;
		tscol--;
		break;
	    case 8:
		tsrow--;
		break;
	    case 9:
		tsrow--;
		tscol++;
		break;
	    }
	adjust (&tqrow, &tqcol, &tsrow, &tscol);
	if (universe[tqrow][tqcol].sectors[tsrow][tscol].image == ' ')
	    {
	    pships->bearing = tbearing;
	    found = true;
	    }
	else
	    {
	    if (try > 6)
		/* plow through something */
		{
		pships->bearing = bearing;
		found = true;
		}
	    else
		{
		tbearing = btrys[bearing][try];
		try++;
		}
	    }
	} /* end for (!found) */

    /* Torps & Phasers. 
     * Torpedos are the 1st preference.  If they are fired the phasers
     * will not be.  If torp's inoperative, ran out, or not on target
     * then fire phasers.
     */

    if ((torp_dir != 0) && (pships->torpedos > 0) && (pships->torp_time == 0)
	&& (!ships[pdrones->d_after-A].cloak))
	 {		/* fire torpedo */
	 for (found = false, i = 0; i < TORP_MAX && !found; )
	    if (pships->torp_info[i].torp_dir == -1)
		 found = true;
	    else i++;
	 if (found)
	      {
	      ptorps = pships->torp_info + i;
	      pships->torpedos--;
	      pships->torp_active = true;
	      ptorps->torp_dir = torp_dir;
	      ptorps->torp_dist = 0;
	      if (ptorps->torp_dir == pships->bearing)
		   ptorps->torp_speed = (pships->warp_speed + 0.4) + TORP_VELOCITY;
	      else ptorps->torp_speed = TORP_VELOCITY;

	      /* set starting location for torp as ship location */

	      ptorps->tquadr = pships->q_row;
	      ptorps->tquadc = pships->q_col;
	      ptorps->tsectr = pships->s_row;
	      ptorps->tsectc = pships->s_col;

	      /* keep drone from firing a torpedo on each turn */

	      pships->torp_time = pdrones->d_delay;
	      }  /* if found */
	 }  /* fire torpedos */
    else
	/*
	 * Try Phasers.
	 */
	if (phaser != 0 && pships->phasers == 0 && pships->energy > 500
	    && (!ships[pdrones->d_after-A].cloak))
	    {
	    fire_phasers(pdrones->d_char, pdrones->d_after, dist);

	    /* keep drone from firing phasers on each turn */

	    pships->phasers = pdrones->d_delay;
	    }
    ship_action (pdrones->d_char);
    }


adjust (tqrow, tqcol, tsrow, tscol)
    int *tqrow, *tqcol, *tsrow, *tscol;

    /* Adjust these values if any have wraped into another quadrant */

    {
    if (*tsrow < 0)
	{
	*tsrow = NUMSECTORS - 1;
	(*tqrow)--;
	if (*tqrow < 0)
	    *tqrow = NUMSECTORS - 1;
	}
    else
	if (*tsrow > NUMSECTORS - 1)
	    {
	    *tsrow = 0;
	    (*tqrow)++;
	    if (*tqrow > NUMSECTORS - 1)
		*tqrow = 0;
	    }
    if (*tscol < 0)
	{
	*tscol = NUMSECTORS - 1;
	(*tqcol)--;
	if (*tqcol < 0)
	    *tqcol = NUMSECTORS - 1;
	}
    else
	if (*tscol > NUMSECTORS - 1)
	    {
	    *tscol = 0;
	    (*tqcol)++;
	    if (*tqcol > NUMSECTORS - 1)
		*tqcol = 0;
	    }
    }
