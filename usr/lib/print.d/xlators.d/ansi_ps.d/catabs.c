#ifndef lint
static char SccsId[] = "  @(#)catabs.c	4.1   LPS_ULT_TRN   7/2/90";
#endif

/* file: catabs.c
 *
 *	COPYRIGHT  (c)  DIGITAL  EQUIPMENT CORPORATION 1985,
 *      1986, 1987, 1988, 1989.  ALL RIGHTS RESERVED.
 *
 *	THIS SOFTWARE IS FURNISHED UNDER A LICENSE AND MAY BE
 *	USED AND COPIED ONLY IN ACCORDANCE  WITH THE TERMS OF
 *	SUCH  LICENSE  AND  WITH  THE  INCLUSION OF THE ABOVE
 *	COPYRIGHT  NOTICE.  THIS SOFTWARE OR ANY OTHER COPIES
 *	THEREOF   MAY  NOT  BE  PROVIDED  OR  OTHERWISE  MADE
 *	AVAILABLE  TO  ANY  OTHER  PERSON.  NO  TITLE  TO AND
 *	OWNERSHIP  OF  THE  SOFTWARE  IS  HEREBY TRANSFERRED.
 *
 *	THE INFORMATION IN THIS SOFTWARE IS SUBJECT TO CHANGE
 *	WITHOUT  NOTICE  AND SHOULD  NOT BE  CONSTRUED  AS A
 *	COMMITMENT  BY  DIGITAL EQUIPMENT CORPORATION.
 *
 *	DIGITAL  ASSUMES  NO RESPONSIBILITY  FOR THE  USE  OR
 *	RELIABILITY  OF ITS SOFTWARE ON EQUIPMENT THAT IS NOT
 *	SUPPLIED BY DIGITAL.
 *
 */



/*
 *-----------------------------------------------------------
 *
 *   begin edit_history
 *
 *   27-JAN-1988 11:46 bf
 *   Started adding action routines called via a search 
 *   table from cp_split.  Change existing functions to 
 *   use new variable names.  Remove determine_which_tbl(), 
 *   scale_htabs() and scale_vtabs().
 *
 *    2-FEB-1988 16:59 mhs
 *   Combination of catabs and dutabs
 *
 *   23-MAR-1988 11:22 mhs
 *   Fix scaling of htabs/vtabs.
 *
 *    6-APR-1988 15:47 mhs
 *   Move htabs_init and vtabs_init from cainit.c.
 *
 *  18-DEC-1988 15:19 ejs
 *		Major changes to data structutes.  GLYPH has been modified
 *		to allow struct copies more easily.  The PS output is not
 *		changed (barring bugs of course).
 *
 *   end edit_history
 *
 *-----------------------------------------------------------
 */



/*
 *-----------------------------------------------------------
 *
 *   begin description
 *
 *   Filename:  catabs.c
 *
 *   This file contains all subroutines which are related to tabs 
 *   functionality for the ANSI-to-PostScript translator (both 
 *   horizontal and vertical tabs).
 *
 *   NOTE:  ALL VARIABLES ARE STORED IN UNITS OF "CENTIPOINTS", WHERE 
 *   ONE CENTIPOINT = 1/7200th OF AN INCH.
 *
 *   This module includes the following routines:
 *
 *   tab_tbl_insert ()
 *   htabs_init()
 *   vtabs_init()
 *   pr_ht()
 *   pr_vt()
 *   pr_hts()
 *   pr_vts()
 *   dec_hts()
 *   dec_shts()
 *   dec_caht()
 *   pr_tbc()
 *   dec_vts()
 *   dec_svts()
 *   dec_cavt()
 *   pr_htac()
 *   pr_htc()
 *   pr_htcl()
 *   pr_vtac()
 *   pr_vtc()
 *   update_tbl_and_ct ()
 *
 *   end description
 *
 *-----------------------------------------------------------
 */



/* Translator edit history for this file
 *
 * file:	xlc_tabs.c - Translator tabulation routines
 * created:	laf	 5-DEC-1985 11:03:22 
 * edit:	gh	 9-APR-1986 14:50:45 Adding empty_jfy_buf() to
 *			 pr_ht() & pr_vt()
 *		gh	 10-APR-1986 12:15:52 pr_vt() set .ahp, not .avp -
 *			 corrected.
 *		gh	 15-APR-1986 11:47:47 Adding code for tab scaling.
 *		gh	 30-APR-1986 17:53:07 Added setting of .rmf in HT
 *		gh	 22-MAY-1986 11:52:55 Adding test for 0 in scale_tabs
 *		gh	 23-MAY-1986 10:49:23 tab_tbl_insert() corrected to
 *			 fetch a WORD value for current tab count
 *		gh	 6-JUN-1986 09:53:13 Corrected pr_vt() to do <FF> if
 *			 there are no tabs in the table - before did it only if
 *			 there were no tabs > than the current .avp
 *		gh	 24-JUN-1986 11:07:38 There is a bug in the tab table
 *			 insert routine.  If the table is full, and the tab to
 *			 insert will be the 2nd highest, 'i', which is currently
 *			 pointing one past the last table entry, should be
 *			 decremented at the same time (*tab_count)-- is done.
 *			 Fixed it.  Also changed the plist[i] decrementing in
 *			 decsvts and decshts to be done only if in char mode.
 *		gh	 24-JUN-1986 13:55:17 2nd problem - tab count not
 *			 set to 1 by first tab inserted in blank tab table by
 *			 tab_tbl_insert().  Reorderd while loop.
 *		gh	 21-JUL-1986 12:38:41 In DECSHTS and DECSVTS changed
 *			 the routine that decremented plist[i] only if in
 *			 character mode to always decrementing plist[i].
 *		gh	 22-JUL-1986 09:24:44 There is another bug in the tab
 *			 table insert.  If the table is full, and the value to
 *			 insert is the lowest value, it currently gets inserted
 *			 into table[-1] position.  Fixing it.
 *		gh	 22-JUL-1986 16:22:20 Changing VTS to negate the effects
 *			 of .fcf (spec ver 26 page 12=20)
 *		laf	 30-JUL-1986 10:34:28 Removed reference to unused
 *			 variable "pdef"
 *
 */



#define catabs (1)

/*  begin include_file    */

#include    "portab.h"	    /* general portability constants */
#include    "cpsys.hc"	    /* system wide constants, typedefs, etc. */
#include    "cpast.hc"	    /* state tables for split command		*/
#include    "cpglobal.hc"   /* global defs for cpxxx.c files		*/
#include    "caglobal.hc"   /* global defs for caxxx.c files		*/
#include    "camac.lib_hc"  /* For Font Parameters access		*/

#ifdef DUMP
#include    "xlc_iface.hc"  /* defines for oprintf			*/
#include    "dumpu.hc"      /* Dump Utility                             */
#endif DUMP

/*  end   include_file    */


/*
 *********************************************************************
 *  tab_tbl_insert()
 * 
 * This routine is used to insert tabs in BOTH the vertical and horizontal
 * tab tables.
 *
 * If tab table is already full, addition of each new tab causes highest-
 * valued old tabstop to be bumped from table.  Exception is when new tab
 * would be highest one in table; in this case, the new tab is ignored.
 *
 *********************************************************************
 */

VOID tab_tbl_insert (tabstop, tab_count, tab_table, maxct)
LONG	tabstop;	
PW	tab_count;
AL	tab_table;
WORD	maxct;		/* necessary to keep this a general purpose rtn */

{
    WORD i, dest;

    /*
     * search through table til tabstop < guy at right (table[i])
     */
    i = 0;
    while ((i < *tab_count) && (tabstop >= tab_table [i])) {
    	/* If the tab already exists, return without doing anything */
        if (tabstop == tab_table [i]) {
	    return;
        }
    i++;
    } /* endwhile */

    /*
     * At this point, i = the table index where tabstop belongs.
     * If i == tab_count, then if the table is already full, discard the new
     * tab value and return.  If table is not full, just insert tab at position
     * 'i'.  If i < tab_count, tabs must be moved up before inserting new tab
     * at position 'i'.
     */
    if (i != *tab_count)  {		/* Is new tab the highest so far ? */
    	if (*tab_count == maxct)	/* No, make room for it */
    		dest = maxct - 1;
    	else	dest = *tab_count;
    	while (dest > i) {
        	tab_table [dest] = tab_table [dest-1];
    		dest--;
    		}
    	}    	
    else
    	if (*tab_count == maxct)	/* Yes, is tab table full ? */
    		return;			/*   Yes, return */
    tab_table[i] = tabstop;		/* Put new tab in table */
    if (*tab_count != maxct) 		/* if tab table not already full ... */
    	(*tab_count)++;			/* Increment tab count */
}


/*----------*/
VOID htabs_init ()
{
LONG init_tab_width;

xl_st.htabct = 0;
init_tab_width = hdist (DEFAULT_COLS_PER_TAB);
while (xl_st.htabct < MAX_HTABS)
  xl_st.htabs [xl_st.htabct++] = xl_st.h_lim_bound.min +
	((LONG)xl_st.htabct * init_tab_width);
}

/*----------*/
VOID vtabs_init ()
{
LONG init_tab_width;


if  (xl_st.vai_valid == FALSE)
    {
	compute_vai();
    }
xl_st.vtabct = 0;
init_tab_width = (LONG) DEFAULT_LINES_PER_TAB * xl_st.vai;
while (xl_st.vtabct < MAX_VTABS)
  xl_st.vtabs [xl_st.vtabct++] = ((LONG)xl_st.vtabct * init_tab_width);

}


/*****************************************************************************
    Clear All Vertical Tabs - Dump Utility
*******************************************************************************/

VOID dec_cavt()
    {
#ifdef DUMP
    {
    oprintf("DECCAVT \n");    
    }
#endif
    xl_st.vtabct = 0;
    return;
    }


/*
 *********************************************************************
 *   Tab Clear Process Function
 *********************************************************************
 */

VOID pr_tbc()
{
#ifdef DUMP
    {
    oprintf("TBC \n");    
    pprint();		    /* Print the parameter list */
    }
#endif
    cp_split(ast_tbc_srch);
}


/*
 *********************************************************************
 *  Clear Horizontal Tab at Active Column Function
 *********************************************************************
 */

VOID pr_htac()
{
    WORD	i;

#ifdef DUMP
    {
    oprintf("TBC HTAC \n");    
    pprint();		    /* Print the parameter list */
    }
#endif
    /* Look for a tab at the current position */
    for (i=0; i< xl_st.htabct; i++)  
    {
    	if (xl_st.htabs[i] == xl_st.curchar.ap.xval)  
	{
    		/* There is a tab at the current position - clear it */
    		/* and move all remaining tabs down in the table */
    		for (; i+1 < xl_st.htabct; i++)
    		        xl_st.htabs[i] = xl_st.htabs[i + 1];
    		xl_st.htabct--;
    		break;
    	}
    }
}



/*
 *********************************************************************
 *  Clear All Horizontal Tabs Function
 *********************************************************************
 */

VOID pr_htc()
{
#ifdef DUMP
    {
    oprintf("TBC HTC \n");    
    }
#endif
    xl_st.htabct = 0;
}



/*
 *********************************************************************
 *  Clear All Horizontal Tabs on Line Function
 *********************************************************************
 */

/*
 *   Note:  Because of the way tab settings are implemented,
 *	    clearing all tabs on the current line has the same
 *	    effect as clear all horizontal tabs.
 *
 */

VOID pr_htcl()
{
#ifdef DUMP
    {
    oprintf("TBC HTCL \n");    
    }
#endif
    xl_st.htabct = 0;
}


/*
 *********************************************************************
 *  Clear Vertical Tab at Active Column Function
 *********************************************************************
 */

VOID pr_vtac()
{
    WORD	i;

#ifdef DUMP
    {
    oprintf("TBC VTAC \n");    
    }
#endif
    /* Look for a tab at the current position */
    for (i=0; i< xl_st.vtabct; i++)  
    {
    	if (xl_st.vtabs[i] == xl_st.curchar.ap.yval)  
	{
    		/* There is a tab at the current position - clear it */
    		/* and move all remaining tabs down in the table */
    		for (; i+1 < xl_st.vtabct; i++)
    		        xl_st.vtabs[i] = xl_st.vtabs[i + 1];
    		xl_st.vtabct--;
    		break;
    	}
    }
}



/*
 *********************************************************************
 *  Clear All Vertical Tabs Function
 *********************************************************************
 */

VOID pr_vtc()
{
#ifdef DUMP
    {
    oprintf("TBC VTC \n");    
    }
#endif
    xl_st.vtabct = 0;
}


/*
 *********************************************************************
 *  Set Vertical Tab Stops Function
 *********************************************************************
 */

VOID dec_svts ()
{
    WORD i;
    LONG tab;

#ifdef DUMP
    {
    oprintf("DECSVTS \n");    
    pprint();		    /* Print the parameter list */
    }
#endif
    /* For each parameter in the list, ... */
    for (i=0; i < CP_MAXPCNT; i++) 
    {
    	cp_pbuf[i]--;
    	tab = vdist(cp_pbuf[i]);
	tab_tbl_insert (tab, &xl_st.vtabct, xl_st.vtabs, MAX_VTABS);
    } /* endfor */
}



/*
 *********************************************************************
 *  Set Horizontal Tab Stops Function
 *********************************************************************
 */

VOID dec_shts()
{
    WORD i;
    LONG tab;

#ifdef DUMP
    {
    oprintf("DECSHTS \n");    
    pprint();		    /* Print the parameter list */
    }
#endif
    /* For each parameter in the list, ... */
    for (i=0; i < CP_MAXPCNT; i++) 
    {
    	cp_pbuf[i]--;
    	tab = hdist(cp_pbuf[i]);
	tab_tbl_insert (tab, &xl_st.htabct, xl_st.htabs, MAX_HTABS);
    } /* endfor */
} 



/*
 *********************************************************************
 *  Move to next Horizontal Tab Function
 *********************************************************************
 */

VOID pr_ht()
{
    WORD i;

#ifdef DUMP
    {
    oprintf("HT \n");    
    }
#endif

    /* Empty the justify buffer, if there is anything in it */
    empty_jfy_buf();

    /* 
     * IF there are any tabs in the tab table ...
     */
    if (xl_st.htabct != 0) {

        /* Search tab table for next tabstop greater than the current ahp */
        for (i=0; (i < xl_st.htabct) && (xl_st.htabs[i] <= xl_st.curchar.ap.xval); i++);

        /* If there exists a tabstop which is greater than the current ahp,
         * AND less than the right margin, then go to it.
    	 * If the tabstop is past the right margin, then do a HPR(infinity)
         */
        if (i != xl_st.htabct) {
            if (xl_st.htabs[i] >= xl_st.h_lim_bound.max) {
		/* Do a HPR of infinity */
    		hpos_rel(xl_st.h_lim_bound.max+1);
            } else {
	        xl_st.curchar.ap.xval = xl_st.htabs[i];
	    }

	/* if there are no greater tabstops in the table, ... */
        } else if (i == xl_st.htabct) {

	/* Do a HPR of infinity */
    	hpos_rel(xl_st.h_lim_bound.max+1);
	}

    /* ... But if there are NO tabs in the tab table ... */
    } else {

	/* Do a HPR of infinity */
    	hpos_rel(xl_st.h_lim_bound.max+1);
    }
}



/*
 *********************************************************************
 *  Move to next Vertical Tab Function
 *********************************************************************
 */

VOID pr_vt()
{
    WORD i;

#ifdef DUMP
    {
    oprintf("VT \n");    
    }
#endif
    /* Empty the justify buffer, if there is anything in it */
    empty_jfy_buf();

    /* IF there are any tabs in the tab table ... */
    if (xl_st.vtabct != 0) {

        /* Search tab table for next tabstop greater than the current avp */
    	for (i=0; (i<xl_st.vtabct) && (xl_st.vtabs[i] <= xl_st.curchar.ap.yval); 
		i++)
	    ;

        /* If there exists a tabstop which is greater than the current avp,
         * if it is below or AT the bottom margin, then do a form feed;
         * Else make the tabstop the new avp.
         */
        if (i == xl_st.vtabct) {
    	    vpos_abs (xl_st.v_lim_bound.max);
        } else {
    	    vpos_abs (xl_st.vtabs[i]);
 	}   
    /* otherwise tab table was empty, so do <FF> */     
    } else  vpos_abs (xl_st.v_lim_bound.max);

}



/*
 *********************************************************************
 *  Set Horizontal Tab at current position Function
 *********************************************************************
 */

VOID pr_hts()	/* sets a horizontal tab at the current ahp */

{
#ifdef DUMP

    {
    cp_exit_cur_mode();
    oprintf("HTS \n");    
    }
#endif
    cp_exit_cur_mode();
    tab_tbl_insert (xl_st.curchar.ap.xval, &xl_st.htabct, xl_st.htabs, MAX_HTABS);
}



/*
 *********************************************************************
 *  Set Horizontal Tab at current position Function
 *********************************************************************
 */
VOID dec_hts()
{
#ifdef DUMP
    {
    oprintf("DECHTS \n");    
    }
#endif

    tab_tbl_insert (xl_st.curchar.ap.xval, &xl_st.htabct, xl_st.htabs, MAX_HTABS);
}


/*
 *********************************************************************
 *  Set Vertical Tab at current position Function
 *********************************************************************
 */

VOID pr_vts()	/* Sets a vertical tab at the current avp */
{
#ifdef DUMP
    {
    cp_exit_cur_mode();
    oprintf("VTS \n");    
    }
#endif

    cp_exit_cur_mode();

    if (!xl_st.fcf)
    	tab_tbl_insert (xl_st.curchar.ap.yval + 
    		FNT_ABOVE_BASELINE(xl_st.curchar.char_data.char_font)+1L,
    		&xl_st.vtabct, xl_st.vtabs, MAX_VTABS);
    else
    	tab_tbl_insert (xl_st.curchar.ap.yval, 
    		&xl_st.vtabct, xl_st.vtabs, MAX_VTABS);
}



/*
 *********************************************************************
 *  Set Vertical Tab at current position Function
 *********************************************************************
 */

VOID dec_vts()
{
#ifdef DUMP
    {
    oprintf("DECVTS \n");    
    }
#endif
    if (!xl_st.fcf)
    	tab_tbl_insert (xl_st.curchar.ap.yval + 
    		FNT_ABOVE_BASELINE(xl_st.curchar.char_data.char_font)+1L,
    		&xl_st.vtabct, xl_st.vtabs, MAX_VTABS);
    else
    	tab_tbl_insert (xl_st.curchar.ap.yval, 
    		&xl_st.vtabct, xl_st.vtabs, MAX_VTABS);
}


VOID scale_htabs(oldhai,newhai)
LONG	oldhai,newhai;
{
    WORD	i;

    /* If there are no entries in the tab table, exit */
    if (xl_st.htabct == 0)		return;

    /* If either old or new hai is =0, don't scale anything */
    if (oldhai == 0 || newhai == 0)		return;

    /* Go through each entry, and multiply it by the newhai, then divide it
       by the oldhai. */
    for (i=0; i<xl_st.htabct; i++)
    	xl_st.htabs[i] = ((LONG)xl_st.htabs[i] * (LONG)newhai) /  (LONG)oldhai;
}


VOID scale_vtabs(oldvai,newvai)
LONG	oldvai,newvai;
{
    WORD	i;

    /* If there are no entries in the tab table, exit */
    if (xl_st.vtabct == 0)		return;

    /* If either old or new vai is =0, don't scale anything */
    if (oldvai == 0 || newvai == 0)		return;

    /* Go through each entry, and multiply it by the newvai, then divide it
       by the oldvai. */
    for (i=0; i<xl_st.vtabct; i++)
    	xl_st.vtabs[i] = ((LONG)xl_st.vtabs[i] * (LONG)newvai) / (LONG)oldvai;
}


/************************************************************************
    Clear All Horizontal Tabs - Dump Utility
*************************************************************************/


VOID dec_caht()
   {
#ifdef DUMP
   {
    oprintf("DECCAHT \n");    
   }
#endif
    xl_st.htabct = 0;
   }

