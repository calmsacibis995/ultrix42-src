#ifndef lint
static char	*sccsid = "@(#)screen.c	4.1	(ULTRIX)	11/23/87";
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
 * screen.c
 *
 * Name:	starship
 * Purpose:	Screen update routines are in this file.
 *		They update the screen image from the shared data.
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

/* external variables */

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

extern char bell;		/* bell char */


/*****************************************************************************/
/*									     */
/*    		START INTERNAL PROCEDURES				     */
/*									     */
/*****************************************************************************/

update_screen (ship_ch)
    char ship_ch;

    /* Udates the screen for (the present caller (ship_ch).
       The display flags in ship record are checked to see which
       fields need to be updated.
    */

    { 
    char tmp_str[25];  /* to convert numbers to string */
    register struct ship *pships;
    register int bi;

    printf_srscan(ship_ch);
    printf_lrscan(ship_ch);

    pships = ships + (ship_ch - A);
    if (pships->dis_msg)
	 {		/* display message on user's screen */
	 pships->dis_msg = false;
	 pships->msg_timer = 0;
	 move_to(MESSAGER, MESSAGEC);
	 clear_to_eol();
	 refresh_scr();
	 move_to(MESSAGER, MESSAGEC);
	 char_out (bell);
	 string_out (pships->msg_buf);
	 clear_to_eol();
	 refresh_scr();
	 /*
	  * radio messages from ships start with "#-#".  So if 2nd char is a
	  * '-' then we assume its a message from another ship & save its
	  * coordinates.
	  */
	 if (pships->msg_buf[1] == '-')
	     {
	     pships->radiorow = pships->msg_buf[0] - 48;
	     pships->radiocol = pships->msg_buf[2] - 48;
	     }
	 }
    else {
	 pships->msg_timer++;
	 if (pships->msg_timer > MSG_TIME)
	     {
	     /* clean up msg line */
	     move_to (MESSAGER, MESSAGEC);
	     clear_to_eol();
	     refresh_scr();
	     pships->msg_timer = 0;
	     };
	 };

    if (pships->dis_map)
	{
	pships->dis_map = false;
	pships->map_timer = 0;
	pships->dis_dir = true;
	draw_map();
	switch (pships->bearing)
	    {
	    case 1:
		move_to(MAPDISPR + pships->q_row + 2, MAPDISPC + pships->q_col);
		char_out('/');
		break;
	    case 2:
		move_to(MAPDISPR + pships->q_row + 2, MAPDISPC + pships->q_col+1);
		char_out('|');
		break;
	    case 3:
		move_to(MAPDISPR + pships->q_row + 2, MAPDISPC + pships->q_col+2);
		char_out('\\');
		break;
	    case 4:
		move_to(MAPDISPR + pships->q_row + 1, MAPDISPC + pships->q_col);
		char_out('-');
		break;
	    case 6:
		move_to(MAPDISPR + pships->q_row + 1, MAPDISPC + pships->q_col+2);
		char_out('-');
		break;
	    case 7:
		move_to(MAPDISPR + pships->q_row, MAPDISPC + pships->q_col);
		char_out('\\');
		break;
	    case 8:
		move_to(MAPDISPR + pships->q_row, MAPDISPC + pships->q_col + 1);
		char_out('|');
		break;
	    case 9:
		move_to(MAPDISPR + pships->q_row, MAPDISPC + pships->q_col + 2);
		char_out('/');
		break;
	    }	/* end switch (bearing) */
	for (bi = 0; bi < NUMBASES; bi++)
	    if (pships->bases[bi][0] != -1)
		{
		move_to(MAPDISPR + pships->bases[bi][0] + 1,
			MAPDISPC + pships->bases[bi][1] + 1);
		char_out('+');
		}
	if (pships->radiorow != -1)
	    {
	    move_to(MAPDISPR + pships->radiorow + 1, MAPDISPC + pships->radiocol + 1);
	    char_out('r');
	    }
	if (pships->locater != -1)
	    {
	    move_to(MAPDISPR + pships->locater + 1, MAPDISPC + pships->locatec + 1);
	    char_out('l');
	    }
	move_to(MAPDISPR + pships->q_row + 1, MAPDISPC + pships->q_col + 1);
	char_out(pships->ship_ch);
	}   /* end if (dis_map) */
    else
	{
	if (pships->dis_dir)
	    {
	    pships->map_timer++;
	    if (pships->map_timer > MAP_TIME)
		{
		pships->dis_dir = false;
		draw_dir();
		}
	    }
	}

    if (pships->dis_energy)
	 {
	 sprintf (tmp_str, "%5d", pships->energy);
	 move_to(ENERGYR,ENERGYC);
	 string_out (tmp_str);
	 pships->dis_energy = false;
	 };

    if (pships->shield_time > 0)
	 {
	 pships->shield_time = pships->shield_time - 1;
	 if (pships->shield_time == 0)
	      strcpy(tmp_str, "    0");
	 else sprintf (tmp_str, " *%3d", pships->shield_time);
	 move_to(SHIELDSR, SHIELDSC);
	 string_out (tmp_str);
	 }
    else if (pships->dis_shields)
	     {
	     sprintf (tmp_str,"%5d", pships->shields);
	     move_to(SHIELDSR, SHIELDSC);
	     string_out (tmp_str);
	     pships->dis_shields = false;
	     };

    if (pships->torp_time > 0)
	 {
	 pships->torp_time--;
	 if (pships->torp_time == 0)
	      pships->dis_torpedos = true;
	 else {
	      sprintf (tmp_str, "*%3d", pships->torp_time);
	      move_to(TORPR, TORPC);
	      string_out (tmp_str);
	      };
	 };

    if (pships->dis_torpedos)
	 {
	 sprintf (tmp_str,"%4d", pships->torpedos);
	 move_to(TORPR,TORPC);
	 string_out (tmp_str);
	 pships->dis_torpedos = false;
	 };

    if (pships->phasers > 0)
	 {
	 pships->phasers--;
	 if (pships->phasers == 0)
	      strcpy(tmp_str, " opr");
	 else sprintf (tmp_str, "*%3d", pships->phasers);
	 move_to(PHASERR, PHASERC);
	 string_out (tmp_str);
	 };

    if (pships->warp_drive > 0)
	 {
	 pships->warp_drive = pships->warp_drive - 1;
	 if (pships->warp_drive == 0)
	      strcpy(tmp_str, " opr");
	 else sprintf (tmp_str, "*%3d", pships->warp_drive);
	 move_to(WARPDRIVER,WARPDRIVEC);
	 string_out (tmp_str);
	 };

    if (pships->impulse > 0)
	 {
	 pships->impulse = pships->impulse - 1;
	 if (pships->impulse == 0)
	      strcpy(tmp_str, " opr");
	 else sprintf (tmp_str, "*%3d", pships->impulse);
	 move_to(IMPULSER, IMPULSEC);
	 string_out (tmp_str);
	 };

    if (pships->life_supp_time > 0)
	 {
	 pships->life_supp_time -= 1;
	 if (pships->life_supp_time == 0)
	      pships->life_support = GREEN;
	 switch ((int)pships->life_support)
	 {
	     case 0:
		strcpy (tmp_str,"   RED");
		break;
	     case 1:
		strcpy (tmp_str,"YELLOW");
		break;
	     case 2:
		strcpy (tmp_str," GREEN");
		break;
	 };
	 move_to(LIFESUPR, LIFESUPC);
	 string_out (tmp_str);
	 };

    if (pships->sr_scan > 0)
	 {
	 pships->sr_scan = pships->sr_scan - 1;
	 if (pships->sr_scan == 0)
	      strcpy(tmp_str, " opr");
	 else sprintf (tmp_str, "*%3d", pships->sr_scan);
	 move_to(SRCONDR, SRCONDC);
	 string_out (tmp_str);
	 };

    if (pships->lr_scan > 0)
	 {
	 pships->lr_scan = pships->lr_scan - 1;
	 if (pships->lr_scan == 0)
	      strcpy(tmp_str, " opr");
	 else sprintf (tmp_str, "*%3d", pships->lr_scan);
	 move_to(LRCONDR, LRCONDC);
	 string_out (tmp_str);
	 };

    if (pships->radio > 0)
	 {
	 pships->radio = pships->radio - 1;
	 if (pships->radio == 0)
	      strcpy(tmp_str, " opr");
	 else sprintf (tmp_str, "*%3d", pships->radio);
	 move_to(RADIOR, RADIOC);
	 string_out (tmp_str);
	 };

    if (pships->sensors > 0)
	 {
	 pships->sensors = pships->sensors - 1;
	 if (pships->sensors == 0)
	      strcpy(tmp_str, " opr");
	 else sprintf (tmp_str, "*%3d", pships->sensors);
	 move_to(SENSORR, SENSORC);
	 string_out (tmp_str);
	 };

    if (pships->cloaking > 0)
	 {
	 pships->cloaking = pships->cloaking - 1;
	 if (pships->cloaking == 0)
	      strcpy(tmp_str, " opr");
	 else sprintf (tmp_str, "*%3d", pships->cloaking);
	 move_to(CLOAKINGR, CLOAKINGC);
	 string_out (tmp_str);
	 };

    if (pships->dis_quad)
	 {
	 sprintf (tmp_str, "%d-%d", pships->q_row, pships->q_col);
	 move_to(QUADR,QUADC);
	 string_out (tmp_str);
	 pships->dis_quad = false;
	 };

    if ((pships->warp_speed > 0)||(pships->dis_sector))
	 {
	 sprintf (tmp_str, "%d-%d", pships->s_row, pships->s_col);
	 move_to(SECTORR,SECTORC);
	 string_out (tmp_str);
	 pships->dis_sector = false;
	 };

    if (pships->dis_warp_set)
	 {
	 sprintf (tmp_str, "%3.1f",pships->warp_set);
	 move_to(WARPSETR,WARPSETC);
	 string_out (tmp_str);
	 pships->dis_warp_set = false;
	 };

    if (pships->dis_warp_speed)
	 {
	 sprintf (tmp_str, "%3.1f", pships->warp_speed);
	 move_to(WARPSPEEDR,WARPSPEEDC);
	 string_out (tmp_str);
	 pships->dis_warp_speed = false;
	 };

    if (pships->dis_bearing)
	 {
	 sprintf (tmp_str, "%d", pships->bearing);
	 move_to(BEARINGR,BEARINGC);
	 string_out (tmp_str);
	 pships->dis_bearing = false;
	 };

    /* if anyone entered or left the game, update fleet display */

    if (pships->dis_fleet)
	 {
	 pships->dis_fleet = false;
	 update_fleet();
	 };

    /* refresh_scr to show long and short range scan, and ship status displays */
    refresh_scr();
    };  /* proc update screen */

printf_srscan(ship_ch)
    char ship_ch;

	/* Writes out the srscan area around a ship.  It is assumed that the
	 * ship will appear in the middle (the SHDISPR,SHDISPC location)
	 * of the srscan.
	*/

	
	{
	register int i,j;	/* loop indicies */
	register struct ship *pships; /* pointer to ship structure */
	int sqr;		/* starting quad row */
	int sqc;		/* starting quad col */
	int ssr;		/* starting sector row */
	int ssc;		/* starting sector col */

	int cqr;		/* current quad row */
	int cqc;		/* current quad col */
	register int csr;	/* current sector row */
	register int csc;	/* current sector col */
	char uchar;		/* universe.image character */
	int bi;			/* for bases index */

	pships = ships + (ship_ch - A);

	/* calculate starting quad & sector, row & col */
	sqr = pships->q_row;
	sqc = pships->q_col;
	ssr = pships->s_row - SHDISPR;
	ssc = pships->s_col - SHDISPC;

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
	          sqc = NUMQUADS -1;
	     };

	/* init "current" values */

	cqr = sqr;
	csr = ssr;

	/* loop for SRSCANROWS rows */

	for (i = 0; i < SRSCANROWS; i ++)
	    {
	    /* If current sector row (csr) == 1st sector of quadrant, AND
	     * cross hair goes in a diff place from last time(i != pships->ioff)
	     * then put cross-hairs on rows of srscan.
	     */
	    if (csr == 0 && i != pships->ioff)
		{
		move_to(SRSCANR + i, SRSCANC -1);
		char_out (RCROSSHAIR);
		move_to(SRSCANR + i, LSRSCANC +1);
		char_out (RCROSSHAIR);

		/* clear old crosshair */
		move_to(SRSCANR + pships->ioff, SRSCANC -1);
		char_out (VERT);
		move_to(SRSCANR + pships->ioff, LSRSCANC +1);
		char_out (VERT);
		pships->ioff = i;
		};

	    /* reset these values for the start of each row */

	    csc = ssc;
	    cqc = sqc;

	    /* loop for SRSCANCOLS cols */

	    for (j = 0; j < SRSCANCOLS; j ++)
		{
		/* If this is the first time thru the loop (i == top row),
		 * place column crosshairs at first sector of the quadrant.
		 */
		if (i == SRSCANR)
		    {
		    /* If current sector col (csc) == 1st sector of quad, AND
		     * cross hair goes in a diff place from last time
		     * (j != pships->joff) then put cross-hairs on cols
		     * of srscan.
		     */
		    if (csc == 0 && j != pships->joff1 && j != pships->joff2)
			{
			move_to (SRSCANR - 1, SRSCANC + j);
			char_out (CCROSSHAIR);
			move_to (LSRSCANR + 1, SRSCANC + j);
			char_out (CCROSSHAIR);

			/* clear old crosshair */
			if (j < SHDISPC)
			    {
			    move_to (SRSCANR - 1, SRSCANC + pships->joff1);
			    char_out (HORIZ);
			    move_to (LSRSCANR + 1, SRSCANC + pships->joff1);
			    char_out (HORIZ);
			    pships->joff1 = j;
			    if (j == 5 || j == 6)
				{
				move_to (SRSCANR - 1, SRSCANC + pships->joff2);
				char_out (HORIZ);
				move_to (LSRSCANR + 1, SRSCANC + pships->joff2);
				char_out (HORIZ);
				}
			    }
			else
			    {
			    move_to (SRSCANR - 1, SRSCANC + pships->joff2);
			    char_out (HORIZ);
			    move_to (LSRSCANR + 1, SRSCANC + pships->joff2);
			    char_out (HORIZ);
			    pships->joff2 = j;
			    if (j == 7 || j == 8 || j == 9)
				{
				move_to (SRSCANR - 1, SRSCANC + pships->joff1);
				char_out (HORIZ);
				move_to (LSRSCANR + 1, SRSCANC + pships->joff1);
				char_out (HORIZ);
				}
			    }
			};
		    };  /* i == SRSCANR */
			    
		/* Determine new image for current 'spot' on sr-scan.
		 * If the new image is different from the old then write it.
		 */

		if (pships->sr_scan > 0)	/* sr_scan inoperative */
		    uchar = ' ';
		else
		    uchar = universe[cqr][cqc].sectors [csr][csc].image;
#ifdef OLDSR
		if (uchar != pships->old_sr[i][j])
		    {
		    move_to (SRSCANR + i, SRSCANC +j);
		    char_out (uchar);
		    pships->old_sr[i][j] = uchar;
		    };
#else
		move_to (SRSCANR + i, SRSCANC +j);
		char_out (uchar);
#endif
		/*
		 * Owner of a ship sees (S) if his shields are up.
		 */
		if (i == SHDISPR)
		    {
		    if (j == SHDISPC - 1)
			{
			if (uchar == ' ' && pships->shields > 0)
			    {
			    move_to (SRSCANR + SHDISPR, SRSCANC + SHDISPC - 1);
			    char_out('(');
			    }
			}
		    else if (j == SHDISPC) 
			    {
			    if (uchar == STAR)
			      {
			      pships->starcounter++;
			      if (pships->starcounter >= 5)
				  {
				  pships->starcounter = 0;
				  strcpy(pships->msg_buf, "The star is damaging the ship!");
				  pships->dis_msg = true;
				  pships->dis_shields = true;
				  if (pships->shields >= STAR_COST)
				       /* no damage */
				       pships->shields = pships->shields - STAR_COST;
				  else {
				       bi = STAR_COST - pships->shields;
				       pships->energy = pships->energy - bi;
				       pships->shields = 0;
				       pships->dis_energy = true;
				       damage (ship_ch, bi);
				       }
				  }
				} /* uchar == STAR */
			      }   /* j == SHIPDISPC */
		    else if (j == SHDISPC + 1)
			if (uchar == ' ' && pships->shields > 0)
			    {
			    move_to (SRSCANR + SHDISPR, SRSCANC + SHDISPC + 1);
			    char_out(')');
			    }
		    }    /* end i == SHIPDISPR */

		/*
		 * If cloaking device on, show blank for others, lower
		 * case letter for owner of the ship.
		 *
		 * Make a ship "glow" (use curses standout mode) if its
		 * glow field is set from being hit by phasers or torpedo.
		 *
		 * Performance: maybe use a switch stmt here.
		 */
		if (uchar >= 'A' && uchar <= 'Z')
		    {
		    if (ships[uchar - 'A'].cloak)
			{
			if (uchar != ship_ch)
			    {
			    move_to (SRSCANR + i, SRSCANC +j);
			    char_out (' ');
#ifdef OLDSR
			    pships->old_sr[i][j] = ' ';
#endif
			    }
			else {
			    move_to (SRSCANR + i, SRSCANC +j);
			    char_out (ship_ch + 32);	/* lower case */
#ifdef OLDSR
			    pships->old_sr[i][j] = ship_ch + 32;
#endif
			    };
			}

		    switch (ships[uchar - 'A'].glow)
			{
			case 0:
			    break;
			case 1:
			case 2:
			    standout();
			    move_to (SRSCANR + i, SRSCANC +j);
			    char_out (uchar);
			    standend();
			    if (uchar == ship_ch)
				ships[uchar - 'A'].glow++;
			    break;
			case 3:
			    move_to (SRSCANR + i, SRSCANC +j);
			    char_out (uchar);
			    if (uchar == ship_ch)
				ships[uchar - 'A'].glow = 0;
			    break;
			}
		    }
		/* log base locations */
		else if (!pships->allbases && uchar == '+')
		    {
		    for (bi = 0; bi < NUMBASES; bi++)
			{
			if (pships->bases[bi][0] == cqr &&
			    pships->bases[bi][1] == cqc)
			    break;
			}
		    if (bi == NUMBASES)	/* don't have this base yet */
			{
			for (bi = 0; bi < NUMBASES; bi++)
			    if (pships->bases[bi][0] == -1)	/* empty slot */
				{
				pships->bases[bi][0] = cqr;
				pships->bases[bi][1] = cqc;
				break;
				}
			if (bi == NUMBASES - 1)
			    pships->allbases = true;
			}
		    }
		csc++;
		if (csc > NUMSECTORS -1)
		     {		/* goto 1st sector col, in next quad col */
		     csc = 0;
		     cqc++;
		     if (cqc > NUMQUADS -1)
		          cqc = 0;
		     };
		};  /* for cols */

	    /* increment current sector row & check to see if it goes into next quad */

	    csr = csr + 1;
	    if (csr > NUMSECTORS -1)
	         {
		 csr = 0;
		 cqr = cqr + 1;
		 if (cqr > NUMQUADS -1)
		      cqr = 0;
		 };
	    };  /* for rows */
	};  /* proc printf_srscan */


printf_lrscan(ship_ch)
    char ship_ch;

	/* Writes out the lrscan area around a ship, the ship's own quadrant
	   and  and the 8 quadrants around the ship.
	*/

	
	{
	register struct ship *pships; /* pointer to ship structure */
	register int i,j;	/* loop indicies */
	int squadr;		/* starting quad row */
	int squadc;		/* starting quad col */
	register int cquadr;	/* current quad row */
	register int cquadc;	/* current quad col */
	char tmp_str[25];

	/* calc starting quad row and  and col */

	pships = ships + (ship_ch - A);
	squadr = pships->q_row - 1;
	squadc = pships->q_col - 1;

	/* adjust starting values if < 0 */

	if (squadr < 0)
	     squadr = NUMQUADS -1;

	if (squadc < 0)
	     squadc = NUMQUADS -1;

	cquadr = squadr;

	/* build the output string */

	for (i = 0; i < LRSCANSECTS; i ++)
	    {	/* 3 rows of lrscan */
	    cquadc = squadc;

	    for (j = 0; j < LRSCANSECTS; j ++)
		{	/* 3 cols of lrscan */
		if (pships->lr_scan == 0)
		    sprintf (tmp_str,"%1d%1d%1d", 
			    universe [cquadr][cquadc].nships,
			    universe [cquadr][cquadc].nbases,
			    universe [cquadr][cquadc].nstars);
		else strcpy (tmp_str,"000");	    /* not operational */

		/* If current LRSCAN section has changed,
		 * then update it, and savewhat it looks like.
		 */

		if (strcmp (pships->old_lr[i][j], tmp_str))
		    {
		    /* `* 2' to skip down 2 rows, `* 6' to skip over 6 cols */
		    move_to(LRSCANR + i * 2, LRSCANC + j * (LRSCANSIZE+1));
		    string_out (tmp_str);
		    strcpy (pships->old_lr[i][j], tmp_str);
		    };

		/* increment cquadc; check for wrap-around to start of galaxy */

		cquadc++;
		if (cquadc > NUMQUADS -1)
		     cquadc = 0;
		};  /* for cols */

	    /* increment cquadr;check for wrap-around to top of galaxy */

	    cquadr++;
	    if (cquadr > NUMQUADS -1)
	         cquadr = 0;
	    };  /* for rows */
	/*
	 * Put quadrant row and column numbers around long range scan.
	 */

	cquadr = pships->q_row - 1;
	if (cquadr < 0)
	    cquadr = NUMQUADS - 1;
	for (i = 0; i < LRSCANSECTS; i++)
	    {
	    if (cquadr >= NUMQUADS)
		cquadr = 0;
	    sprintf(tmp_str, "%1d", cquadr);
	    move_to(LRSCANR + i*2, LRSCANC - 3);
	    string_out(tmp_str);
	    cquadr++;
	    }
	cquadc = pships->q_col - 1;
	if (cquadc < 0)
	    cquadc = NUMQUADS - 1;
	for (i = 0; i < LRSCANSECTS; i++)
	    {
	    if (cquadc >= NUMQUADS)
		cquadc = 0;
	    sprintf(tmp_str, "%1d", cquadc);
	    move_to(LRSCANR - 2, LRSCANC + 1 + i*(LRSCANSIZE + 1));
	    string_out(tmp_str);
	    cquadc++;
	    }

	};  /* proc printf_lrscan */


update_fleet()

	/* this procedure updates the callers screen with all fleet
	   display info.  That is, who is in the game and number of kills.
	*/

	{
	register struct ship *pships;
	register int i;		/* current output row */
	register int j;		/* loop index */
	int ch;			/* loop index */
	boolean filled;		/* set true if (fleet display full (9 ships) */
	char tmp_str[25];

	i = FLEETR;
	filled = false;
	for (ch = 'A'; ch <= 'Z' && !filled; ch ++)
	    {
	    pships = ships + (ch - A);
	    if (pships->ship_ch != ' ')
		 {
		 sprintf (tmp_str, "%c:%2d:%.1f %s",pships->ship_ch,
			pships->kills, pships->avekills, pships->name);
		 /*
		  * Blank out to NAMESIZE to cover over any possibly 
		  * longer names already on the screen.
		  */
		 for (j = strlen(pships->name); j < NAMESIZE; j++)
		     strcat(tmp_str, " ");
	         move_to(i,FLEETC);
		 string_out (tmp_str);
		 if (i < FLEETMAXR)
		      i++;
		 else filled = true;
		 };
	    };

	if (! filled)
	     {		/* blank fleet display below last ship */
	     for (; i <= FLEETMAXR; i++)
		{
		move_to (i, FLEETC);
		clear_to_eol();
		};
	     };
	};  /* proc update_fleet */
