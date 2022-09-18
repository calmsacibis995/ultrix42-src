#ifndef lint
static char	*sccsid = "@(#)cmd.c	4.2	(ULTRIX)	9/1/88";
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
 * cmd.c
 *
 * Name:	starship
 * Purpose:	command input.
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

/* externals from main */

extern struct univ *uptr;	/* ptr to global data struct */
extern WINDOW *scanwin;		/* for sub window: short & long range scan */

/* Define new access paths to variables that are now in "univ" struct,
 * to facilitate shared memory.
 */
#define universe uptr->quadrants
#define num_users uptr->Num_users
#define wait uptr->Wait
#define ships uptr->Ships
#define drones uptr->Drones

extern int scanf_cc;		/* char count accumulated in scanf_buf */
extern char esc;		/* esc char */
extern char bell;		/* bell char or null if no bell option in use */
extern boolean userabort;		/* set true if user quits */
extern int errno;

extern char scanf_buf [READSIZE];	/* cmd input buffer */
extern char clr_cmd [12];		/* esc seq to clear cmd line */
extern boolean clr_cmd_flag;		/* true to clear cmd line */

extern char local_ship;		/* char to represent ship */
extern boolean redraw;		/* set true in "scanf_input" by N command */
extern boolean gotchar;		/* set true when input char read, false when echoed */
extern int dafter;		/* incr once for each drone after you */


/* local variables */

long int ioctl_cc;		/* char count returned by ioctl */


/*****************************************************************************/
/*									     */
/*    		START INTERNAL PROCEDURES				     */
/*									     */
/*****************************************************************************/


cvt_int (char_str, loc)
    char *char_str;
    int loc;

    /* CVT char string to int #.  The function allows an optional +/-
       and upto 50000 for the int.
       for example:    -35000

       Params:
	char_str - char string that the number is in.
	loc      - location within char_str where # starts.
    */

    {
    int t_int;	/* temp var */
    boolean neg;	/* set true if negative (-) */
    int i;	/* string index */

    neg = false;
    i = loc;
    t_int = 0;

    if (char_str[i] == '-')
         {
	 neg = true;
	 i++;
	 }
    else if (char_str[i] == '+')
	      i++;

    for (; char_str [i] >= '0' && char_str [i] <= '9' && (t_int < 50000) ;)
	{
	t_int = t_int * 10 + ((int)char_str[i] - 48);
	i++;
	};

    if (neg)
         t_int = - t_int;
    return (t_int);
    };  /* cvt_int */

use_phasers()

    /* Called by scanf_input when user fires phasers.
     * Calls shipinrange() to determine if enemy ship is close enough
     * to hit, & if so, what its distance is.
     */
    
    {
    register struct ship *pships;
    int dist;		/* # of sectors away enemy ship is */
    char ship_ch;	/* ship hit by phaser */
    int bearing;	/* unused here, but needed as a param to shipinrange */
    boolean ontarget;	/* unused here, but needed as a param to shipinrange */

    pships = ships + (local_ship - A);
    if (pships->phasers != 0)
	{
	strcpy (pships->msg_buf, "Phasers inoperative!");
	pships->dis_msg = true;
	return;
	};
    if (pships->docked)
	{
	strcpy (pships->msg_buf, "Can't fire when docked!");
	pships->dis_msg = true;
	return;
	};
    if (pships->cloak)
	{
	strcpy (pships->msg_buf, "Can't fire when cloaked!");
	pships->dis_msg = true;
	return;
	};
     ship_ch = scanf_buf[1];
     if (ship_ch >= 'A' && ship_ch <= 'Z')
	  {

	  /* check to see if ship within phaser range */

	 if (ships[ship_ch - 'A'].cloak)
	     {
	     strcpy(pships->msg_buf, "Ship not in phaser range");
	     pships->dis_msg = true;
	     return;
	     };

	 if (shipinrange(local_ship,ship_ch,PHSRANGE,true,&dist,&bearing,&ontarget))
	     fire_phasers(local_ship, ship_ch, dist);
         else {
	      pships = ships + (local_ship - A);
	      strcpy(pships->msg_buf, "Ship not in phaser range");
	      pships->dis_msg = true;
	      }
      }  /* in A..Z */
    };  /* proc use_phasers */


fire_phasers(fireship, atship, dist)
    char fireship;	/* ship firing */
    char atship;	/* ship being fired at */
    int dist;		/* distance between ships */

    /*
     * Ship was found, by shipinrange(), to be within range, so this
     * routine is called to fire phasers.
     * Used by local ship & by drones.
     */
    {
    register struct ship *pships;
    register int t_int;		/* temp int. */

    pships = ships + (fireship - A);
    if (PHASER_COST > pships->energy)
	t_int = pships->energy / 2;
    else
	t_int = PHASER_COST;

    pships->energy -= t_int;
    pships->dis_energy = true;
   
   /* calculate amount of hit based on the "dist" */

   switch (dist)
   {
   case 0:
   case 1: t_int = (0.90 * t_int);
	    break;
   case 2: t_int = (0.80 * t_int);
	    break;
   case 3: t_int = (0.70 * t_int);
	    break;
   case 4: t_int = (0.60 * t_int);
	    break;
   case 5: t_int = (0.50 * t_int);
	    break;
   default: t_int = 0;
   };

   pships = ships + (atship - A);
   pships->dis_shields = true;
   pships->glow = 1;
   sprintf (pships->msg_buf, "Ship hit by phaser-%d",t_int);
   pships->dis_msg = true;
   if (pships->shields >= t_int)
	pships->shields = pships->shields - t_int;
   else {
	t_int = t_int - pships->shields;
	pships->energy = pships->energy-t_int;
	pships->dis_energy = true;
	pships->shields = 0;

	/* if phasored ship died, give attacker a kill! */

	if (pships->energy <= 0)
	   {
	   universe[pships->q_row][pships->q_col].sectors
		   [pships->s_row][pships->s_col].image = '-';
	   pships = ships + (fireship - A);
	   pships->kills++;
	   sprintf (pships->msg_buf, "The %s is destroyed",ships[atship-A].name);
	   pships->dis_msg = true;
	   }
	else
	    damage (atship, t_int);
	}
    };


locate (srow, scol, row, col)
    register int srow, scol;		/* quad. row of col of local ship */
    int *row, *col;			/* quad. row & col of ship to return */

    /*
     * Attempt to locate a ship near to "local ship".  If one is found
     * its quadrant coordinates are returned in "row" & "col".
     */

     {
     register struct ship *pships;
     char ch;
     register int lowest = 100;
     register int temp;

     for (ch = 'A'; ch <= 'Z'; ch++)
	 {
	 pships = ships + (ch - A);
	 if ((pships->ship_ch != ' ') && (pships->ship_ch != local_ship))
	     {
	     temp = abs(srow - pships->q_row) + abs(scol - pships->q_col);
	     if (temp < lowest)
		 {
		 lowest = temp;
		 *row = pships->q_row;
		 *col = pships->q_col;
		 }
	     }
	 }
     };

scanf_input()

    /* tty input routine for user commands.
       1st it clears the cmd line.
    */

    {
    register int i;	/* loop index */
    char in_ch;		/* input char */
    boolean docmd;	/* set true when cmd completed (by RETURN key) */

    docmd = false;

#   ifdef DEBUG
    move_to (COMMANDR,COMMANDC);
    refresh_scr();
    scanf("%s",scanf_buf);
    upcase (scanf_buf);
    docmd = true;
#   else DEBUG
    /* use ioctl call to see if any chars in input buffer,
     * if so, loop here and read 'ioctl_cc' characters.
     */
    ioctl(0,FIONREAD,&ioctl_cc);
    for (i = 0; i < ioctl_cc; i++)
	{
#       ifdef ASYNC
	  scanf("%c",&in_ch);
#       else
	  read(0,&in_ch,1);
#	endif
	upchar (&in_ch);
	switch (scanf_cc)
	    {
	    case 0:	/* 1st char */
		switch (in_ch)
		    {
		    /* single char commands */
		    case 'C': case 'D': case 'G': case 'L': case 'M': case 'N':
		    case 'Z':
		    case '1': case '2': case '3': case '4':
		    case '6': case '7': case '8': case '9':
			move_to (COMMANDR,COMMANDC + scanf_cc);
			char_out (in_ch);
			refresh_scr();
			scanf_buf[scanf_cc] = in_ch;
			scanf_cc++;
			scanf_buf[scanf_cc] = '\0';
			docmd = true;
			break;
		    /* multi-char commands */
		    case 'I': case 'P': case 'Q': case 'R': case 'S':
		    case 'T': case 'W': case 'X':
			move_to (COMMANDR,COMMANDC + scanf_cc);
			char_out (in_ch);
			refresh_scr();
			scanf_buf[scanf_cc] = in_ch;
			scanf_cc++;
			scanf_buf[scanf_cc] = '\0';
			break;
		    case 'E':
			if (local_ship == 'E')
			    {
			    move_to (COMMANDR,COMMANDC + scanf_cc);
			    char_out (in_ch);
			    refresh_scr();
			    scanf_buf[scanf_cc] = in_ch;
			    scanf_cc++;
			    scanf_buf[scanf_cc] = '\0';
			    docmd = true;
			    }
			break;
		    /* unrecognized cmds, ignore */
		    default:
			break;
		    }
		break;
	    case 1:	/* 2nd char */
		switch (in_ch)
		    {
		    case '\010':
		    case '\177':
			/* backspace and delete */
			if (scanf_cc > 0)
			    {
			    scanf_cc--;
			    scanf_buf [scanf_cc] = '\0';
			    move_to (COMMANDR, COMMANDC + scanf_cc);
			    char_out (' ');
			    refresh_scr();
			    };
			break;
		    case '\025':
			/* ^U (erase line) */
			if (scanf_cc > 0)
			    {
			    scanf_cc = 0;
			    scanf_buf[0] = '\0';
			    move_to (COMMANDR, COMMANDC);
			    clear_to_eol();
			    refresh_scr();
			    };
			break;
		    case RETURN:
			docmd = true;
			scanf_buf[scanf_cc] = '\0';
			break;
		    default:
			switch (scanf_buf[0])
			    {
			    /* 2 char cmds */
			    case 'I': case 'P': case 'T': case 'W': case 'X':
				docmd = true;
				/* fall into */
			    default:
				move_to (COMMANDR,COMMANDC + scanf_cc);
				char_out (in_ch);
				refresh_scr();
				scanf_buf[scanf_cc] = in_ch;
				scanf_cc++;
				scanf_buf[scanf_cc] = '\0';
				break;
			    }
			break;
		    }	/* end switch (in_char) */
		break;
	    case 2:	/* 3rd char */
		switch (in_ch)
		    {
		    case '\010':
		    case '\177':
			/* backspace and delete */
			if (scanf_cc > 0)
			    {
			    scanf_cc--;
			    scanf_buf [scanf_cc] = '\0';
			    move_to (COMMANDR, COMMANDC + scanf_cc);
			    char_out (' ');
			    refresh_scr();
			    };
			break;
		    case '\025':
			/* ^U (erase line) */
			if (scanf_cc > 0)
			    {
			    scanf_cc = 0;
			    scanf_buf[0] = '\0';
			    move_to (COMMANDR, COMMANDC);
			    clear_to_eol();
			    refresh_scr();
			    };
			break;
		    case RETURN:
			docmd = true;
			scanf_buf[scanf_cc] = '\0';
			break;
		    default:
			switch (scanf_buf[0])
			    {
			    /* 3 char cmds */
			    case 'S':
				docmd = true;
				/* fall into */
			    default:
				move_to (COMMANDR,COMMANDC + scanf_cc);
				char_out (in_ch);
				refresh_scr();
				scanf_buf[scanf_cc] = in_ch;
				scanf_cc++;
				scanf_buf[scanf_cc] = '\0';
				break;
			    }
			break;
		    }	/* end switch (in_char) */
		break;
	    default:	/* 4th char & above */
		switch(in_ch)
		    {
		    case '\010':
		    case '\177':
			/* backspace and delete */
			if (scanf_cc > 0)
			    {
			    scanf_cc--;
			    scanf_buf [scanf_cc] = '\0';
			    move_to (COMMANDR, COMMANDC + scanf_cc);
			    char_out (' ');
			    refresh_scr();
			    };
			break;
		    case '\025':
			/* ^U (erase line) */
			if (scanf_cc > 0)
			    {
			    scanf_cc = 0;
			    scanf_buf[0] = '\0';
			    move_to (COMMANDR, COMMANDC);
			    clear_to_eol();
			    refresh_scr();
			    };
			break;
		    case RETURN:
			docmd = true;
			scanf_buf[scanf_cc] = '\0';
			break;
		    default:
			move_to (COMMANDR,COMMANDC + scanf_cc);
			char_out (in_ch);
			refresh_scr();
			scanf_buf[scanf_cc] = in_ch;
			scanf_cc++;
			scanf_buf[scanf_cc] = '\0';
			break;
		    }
	    }	/* end switch (scanf_cc) */
	};  /* for */
    /* if cmd not finished yet then return to caller */
    if (!docmd)
	return;
    clr_cmd_flag = true;
#   endif DEBUG

#   ifdef DEBUG
    move_to(MESSAGER,MESSAGEC);
    string_out ("doing cmd >");
    string_out (scanf_buf);
    clear_to_eol();
    refresh_scr();
#   endif

    parse_cmd();
    }


parse_cmd()

    /* parse the command */

    {
    register struct ship *pships;
    register struct torpedo_record *ptorps;
    register int i, j;	/* loop index */
    register int t_int;	/* temp int. */
    register struct ship *pships2;
    boolean found;	/* set true when open torpedo record found */
    char tmp_str[15];	/* tmp string to build radio message in */
    char ch1;		/* to hold scanf_buf[1] */
    int row, col;	/* row & col for locate cmd */

    pships = ships + (local_ship - A);
    ch1 = scanf_buf[1];
    switch (scanf_buf[0])
	{
	case '1':
	case '2':
	case '3':
	case '4':
	case '6':
	case '7':
	case '8':
	case '9':
	    /* set bearing */
	    pships->bearing = scanf_buf[0] - 48;
	    pships->dis_bearing = true;
	    break;
	case 'C':	/* toggle cloaking device */ 
	    if (pships->cloaking != 0)
		{
		strcpy (pships->msg_buf, "Cloaking device inoperative");
		pships->dis_msg = true;
		break;
		};
	    if (pships->cloak)
		pships->cloak = false;
	    else
		pships->cloak = true;
	    break;
	case 'D':	/* dock at starbase */ 
	    pships->dock = true;
	    break;
	case 'E':
	    if (local_ship == 'E')
	      {
	      pships->energy = 25000;
	      pships->dis_energy = true;
	      pships->shields = 25000;
	      pships->shield_time = 0;
	      pships->dis_shields = true;
	      pships->torpedos = 12;
	      pships->torp_time = 0;
	      pships->dis_torpedos = true;
	      pships->phasers = 0;
	      pships->warp_drive = 0;
	      if (pships->sr_scan > 1)
		  pships->sr_scan = 1;
	      if (pships->lr_scan > 1)
		  pships->lr_scan = 1;
	      };
	      break;
	case 'G':	/* Get back last message */
	      pships->dis_msg = true;
	      break;
	case 'I':	/* impulse engines */
	    /* `I' alone gives warp 0.5
	     * `I'n where n = 0-9 gives ~ warp 0.n
	     */
	    if (pships->impulse != 0)
		{
		strcpy (pships->msg_buf, "Impulse engines inoperative");
		pships->dis_msg = true;
		break;
		};
	    if (pships->warp_speed < 1)
		{
		pships->dis_warp_set = true;
		pships->sub_light = 1;
		if (ch1 >= '0' && ch1 <= '9')
		    switch (ch1 - 48)
			{
			case 0:
			    pships->warp_set = 0;
			    pships->sub_light = 0;
			    break;
			case 1:
			case 2:
			case 3:
			    pships->warp_set = 0.2;
			    break;
			case 4:
			case 5:
			case 6:
			    pships->warp_set = 0.5;
			    break;
			case 7:
			case 8:
			case 9:
			    pships->warp_set = 0.8;
			    break;
			}   /* end switch */
		else
		    /* `I' only, gives warp 0.5 */
		    pships->warp_set = 0.5;
		}   /* end warp speed < 1 */
	    else
		{
		strcpy (pships->msg_buf, "Use warp engines at >= W1");
		pships->dis_msg = true;
		}
	    break;
	case 'L':
		 if (pships->energy < LOCATE_COST)
		     {
		     strcpy(pships->msg_buf, "Insufficient energy");
		     pships->dis_msg = true;
		     }
		 else
		     {
		     if (num_users < 2)
			 {
			 strcpy(pships->msg_buf, "No ships to locate");
			 pships->dis_msg = true;
			 }
		     else
			 {
			 row = -1;
			 locate(pships->q_row, pships->q_col, &row, &col);
			 if (row == -1)
			     strcpy (pships->msg_buf, "No ships found");
			 else
			     {
			     sprintf (tmp_str, "%s %1d-%1d", "Ship at", row, col);
			     strcpy (pships->msg_buf, tmp_str);
			     pships->locater = row;
			     pships->locatec = col;
			     }
			 pships->dis_msg = true;
			 pships->energy -= LOCATE_COST;
			 pships->dis_energy = true;
			 }
		     }
		 break;
	case 'M':
		 pships->dis_map = true;
		 break;
	case 'N':
		 /* Reset the "old" long range scan info.  */
		 for (i=0; i < LRSCANSECTS; i++)
		    for (j=0; j < LRSCANSECTS; j++)
			strcpy (pships->old_lr[i][j], "000");

#ifdef OLDSR
		 /* Reset "old" short range scan info */
		 for (i=0; i < SRSCANROWS; i++)
		     for (j=0; j < SRSCANCOLS; j++)
			 pships->old_sr [i] [j] = ' ';
#endif
		 pships->ioff = SRSCANR;
		 pships->joff1 = SRSCANC;
		 pships->joff2 = SRSCANC;
		 /*
		  * clear the window, & redraw the displays. On the
		  * next regular screen update (<= 1 second) the display will
		  * be filled in.
		  */
		 wclear(scanwin);
		 wrefresh(scanwin);
		 draw_sr();
		 draw_lr();
		 break;
	case 'P':
		 use_phasers();  /* phasers */
		 break;
	case 'Q':
		 userabort = true;
		 break;
	case 'R':
	    if (pships->radio != 0)     	/* radio */
		{
		strcpy (pships->msg_buf, "Radio inoperative");
		pships->dis_msg = true;
		break;
		};
	    if ((ch1 >= 'A' && ch1 <= 'Z') || ch1 == '*')
		 {	/* radio ship */
		 if (ch1 == '*' || ships[ch1-A].ship_ch != ' ')
		      {   /* ship in game */
		     i = strlen(scanf_buf);
		     if (i > READSIZE - 9)	/* patch to avoid exceeding */
			  i = READSIZE - 9;	/* msg buffer length */
		     sprintf(tmp_str,"%1d%c%1d %c %c",pships->q_row,'-',
			  pships->q_col, local_ship, bell);
		     if (ch1 != '*')
		       {
		       strcpy(ships[ch1-A].msg_buf, tmp_str);
		       /* start at 3rd char of scanf_buf, & get all but 1st 2 chars */
		       strncat(ships[ch1-A].msg_buf,&scanf_buf[2],i-2);
		       scanf_buf[i] = '\0';
		       ships[ch1-A].dis_msg = true;
		       }
		     else   /* ch1 == '*' */
			 {
			 for (ch1 = 'A'; ch1 <= 'Z'; ch1++)
			   if (ships[ch1-A].ship_ch != ' ')
			     {
			     strcpy(ships[ch1-A].msg_buf, tmp_str);
			     strncat(ships[ch1-A].msg_buf,&scanf_buf[2],i-2);
			     scanf_buf[i] = '\0';
			     ships[ch1-A].dis_msg = true;
			     }
			 }
		     }     /* ship in game */
#	     ifdef DRONE
		 else
		     if (ch1 == 'R' || ch1 == 'K')
			 {
			 dafter++;
			 /* 
			  * Get difficulty level
			  */
			 switch (scanf_buf[2])
			     {
			     case '1':
				 call_drone (ch1, local_ship, 1);
				 break;
			     case '2':
				 call_drone (ch1, local_ship, 2);
				 break;
			     case '3':
				 call_drone (ch1, local_ship, 3);
				 break;
			     case '4':
				 call_drone (ch1, local_ship, 4);
				 break;
			     default:
				 call_drone (ch1, local_ship, 1);
				 break;
			     }
			 }
#	     endif DRONE
		     else		/* no such ship */
			 {
			 strcpy(pships->msg_buf, "No such ship");
			 pships->dis_msg = true;
			 }
		 }
	    break;
	case 'S':
	    /* shields */
	    if (pships->shield_time != 0)
		{
		strcpy (pships->msg_buf, "Shields inoperative");
		pships->dis_msg = true;
		break;
		};
	    t_int = 0;
	    if (ch1 == '-')
		 {
		 switch (scanf_buf[2])
		     {
		     case '0':
			 t_int = 0; break;
		     case '1':
			 t_int = -1; break;
		     case '2':
			 t_int = -2; break;
		     case '3':
			 t_int = -3; break;
		     case '4':
			 t_int = -4; break;
		     case '5':
			 t_int = -5; break;
		     case '6':
			 t_int = -6; break;
		     case '7':
			 t_int = -7; break;
		     case '8':
			 t_int = -8; break;
		     case '9':
			 t_int = -9; break;
		     case 'A':
		     case '*':
			 t_int = -25; break;
		     }
		 }   /* end negative number */
	    else if (ch1 >= '0' && ch1 <= '9')
		      t_int = cvt_int (scanf_buf,1);
	    t_int *= 1000;
	    /*
	     * Only allow 1/2 of total present energy to go into shields.  3-27-85
	     */
	    if (t_int > 0)
		{
		if ((t_int + pships->shields) > (pships->energy - t_int))  /* more into shields than avail */
		     t_int = ((pships->energy + pships->shields) / 2) - pships->shields;
		}
	    else if (t_int < 0 && -t_int > pships->shields)  /* take too much out */
		      t_int = - pships->shields;	/* set to all thats in shields */

	    /* subtract t_int from energy and add to shields */

	    pships->energy = pships->energy - t_int;
	    pships->shields = pships->shields + t_int;

	    pships->dis_energy = true;
	    pships->dis_shields = true;
	    break;
	case 'T':
		    /* torpedos */
	    if (pships->torp_time != 0)
		{
		strcpy (pships->msg_buf, "Torpedos inoperative");
		pships->dis_msg = true;
		break;
		};
	    if (pships->docked)
		{
		strcpy (pships->msg_buf, "Can't fire when docked!");
		pships->dis_msg = true;
		break;
		};
	    if (pships->torpedos <= 0)
		{
		strcpy (pships->msg_buf, "No torpedos");
		pships->dis_msg = true;
		break;
		};
	    if (pships->cloak)
		{
		strcpy (pships->msg_buf, "Can't fire when cloaked!");
		pships->dis_msg = true;
		break;
		};
	    if ((ch1 >= '1' && ch1 <= '9') && (ch1 != '5'))
		t_int = ch1 - 48;
	    else
		t_int = pships->bearing;
	     for (found=false, i=0; i < TORP_MAX && ! found;)
		if (pships->torp_info[i].torp_dir == -1)
		     found = true;
		else i++;
	     if (found)
		  {
		  ptorps = pships->torp_info + i;
		  pships->torpedos--;
		  pships->dis_torpedos = true;
		  pships->torp_active = true;
		  ptorps->torp_dir = t_int;
		  ptorps->torp_dist = 0;
		  if (ptorps->torp_dir == pships->bearing)
		       ptorps->torp_speed = (pships->warp_speed + 0.4) + TORP_VELOCITY;
		  else ptorps->torp_speed = TORP_VELOCITY;

		  /* set starting location for torp as ship location */

		  ptorps->tquadr = pships->q_row;
		  ptorps->tquadc = pships->q_col;
		  ptorps->tsectr = pships->s_row;
		  ptorps->tsectc = pships->s_col;
		  };  /* if found */
	    break;
	case 'W':
	    /* warp speed */
	     if (pships->warp_drive != 0)
		{
		strcpy (pships->msg_buf, "Warp drive inoperative");
		pships->dis_msg = true;
		break;
		};
	     pships->dis_warp_set = true;
	     pships->sub_light = 0;
	     switch(ch1)
		{
		case '0':
		    pships->warp_set = 0;
		    break;
		case '1':
		    pships->warp_set = 1;
		    break;
		case '2':
		    pships->warp_set = 2;
		    break;
		case '3':
		    pships->warp_set = 3;
		    break;
		case '4':
		    pships->warp_set = 4;
		    break;
		case '5':
		    pships->warp_set = 5;
		    break;
		case '6':
		    pships->warp_set = 6;
		    break;
		case '7':
		    pships->warp_set = WARP_MAX;
		    break;
		case '8':
		    pships->warp_set = WARP_MAX;
		    break;
		case '9':
		    pships->warp_set = WARP_MAX;
		    break;
		case '.':
		    pships->warp_set = 0.5;
		    pships->sub_light = 1;
		    break;
		default:
		    pships->warp_set = 1;
		}
	    break;
	case 'X':
	    if (pships->sensors > 0)
		{
		strcpy(pships->msg_buf, "Sensors inoperative");
		pships->dis_msg = true;
		}
	    else
		{
		if (ch1 >= 'A' && ch1 <= 'Z')
		    {
		    if (shipinrange(local_ship,ch1,PHSRANGE,false))
			{
			pships2 = ships + (ch1 - A);
			strcpy(pships2->msg_buf, "Sensors indicate we are being scanned");
			pships2->dis_msg = true;
			i = 0;
			if (pships2->warp_drive != 0)
			    i++;
			if (pships2->torp_time != 0)
			    i++;
			if (pships2->phasers != 0)
			    i++;
			if (pships2->impulse != 0)
			    i++;
			if (pships2->radio != 0)
			    i++;
			if (pships2->shield_time != 0)
			    i++;
			if (pships2->sr_scan != 0)
			    i++;
			if (pships2->lr_scan != 0)
			    i++;
			/* Life support intentionally left out, it and sensors
			 * get damaged together.
			 */
			if (pships2->sensors != 0)
			    i++;
			if (pships2->cloaking != 0)
			    i++;

			pships->dis_msg = true;
			strcpy(pships->msg_buf, "Sensors indicate ");
			strcat(pships->msg_buf, pships2->name);

			if (pships2->shields <= 0)
			    strcat(pships->msg_buf, ": shields down; ");
			else
			    strcat(pships->msg_buf, ": shields up; ");
			switch (i)
			    {
			    case 0:
				strcat(pships->msg_buf, "undamged");
				break;
			    case 1:
				strcat(pships->msg_buf, "slightly damged");
				break;
			    case 2:
				strcat(pships->msg_buf, "moderately damged");
				break;
			    default:
				strcat(pships->msg_buf, "heavily damged");
				break;
			    }
			}   /* in range */
		    else
			{
			strcpy(pships->msg_buf, "Ship not in range");
			pships->dis_msg = true;
			}
		    }   /* ch1 in 'A' to 'Z' */
		}   /* opr */
	     break;
	case 'Z':
	     redraw = true;
	     /* Reset the "old" long range scan info.  */
	     for (i=0; i < LRSCANSECTS; i++)
		for (j=0; j < LRSCANSECTS; j++)
		    strcpy (pships->old_lr[i][j], "000");

#ifdef OLDSR
	     /* Reset "old" short range scan info */
	     for (i=0; i < SRSCANROWS; i++)
		 for (j=0; j < SRSCANCOLS; j++)
		     pships->old_sr [i] [j] = ' ';
#endif
	     pships->ioff = SRSCANR;
	     pships->joff1 = SRSCANC;
	     pships->joff2 = SRSCANC;
	};  /* switch */
       scanf_cc = 0;
	};  /* proc parse_cmd */
