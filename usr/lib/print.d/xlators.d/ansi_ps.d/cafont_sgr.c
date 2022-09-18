#ifndef lint
static char *sccsid = "@(#)cafont_sgr.c	4.1      ULTRIX 7/2/90";
#endif

/* file: cafont_sgr.c
 *
 *	COPYRIGHT  (c)  DIGITAL  EQUIPMENT CORPORATION 1987,
 *	1988, 1989 ALL RIGHTS RESERVED.
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
 *  begin edit_history
 *
 *  araj   14-MAY-1988 11:29
 *	Creation by splitting CAFONT_SEL.C
 *
 *  mhw	    31-MAY-1988 13:18 
 *	Activate Double Underline function
 *
 *  mhw	    24-JUN-1988 12:03
 *	pr_sgr_off and dec_sgr_off must specifically call the
 *	turn-off functions for each attribute, such as pr_bold_off.
 *	All they were doing is changing the attribute flags, and this
 *	did not account for the need to change the font by using
 *	compute_font.
 *
 *  mhs	    30-JUN-1988 08:11
 *	Add parameterised init_sgr routine for NVM support.
 *	Move init_sgr_tbl from cafont_cfont.
 *
 *  30-NOV-1988 14:28 araj
 *	Install cfont caching, change calls to compute font into calls to invalidate_font
 *	Make sure references to vai or g_table have a test for the validity of the data
 *	first
 *
 *   7-DEC-1988 09:01 mhw
 *	Fix spacing of suband superscript.  If the opposite one is on when
 *	the other is set, a GSM of 100% must be done first to get the correct
 *	spacing.
 *
 *  18-DEC-1988 15:19 ejs
 *		Major changes to data structutes.  GLYPH has been modified
 *		to allow struct copies more easily.  The PS output is not
 *		changed (barring bugs of course).
 *
 *  12-JUL-1989 10:51 mhw
 *	Removed camac.lib from include list
 *
 *   9-OCT-1989 17:38 bf
 *	Cleaned up dec_sub(), dec_super() and dec_super_sub_off() to remove
 *	superfluous height and width local variables.  Changed 
 *	dec_super_sub_off() to fix problem where it wasn't doing a plu
 *	when exiting Sub mode.
 *
 *  end edit_history
 *
 *-----------------------------------------------------------
 */



/*
 *-----------------------------------------------------------
 *
 *   begin description
 *
 *   Filename:  cafont_sgr.c
 *
 *
 *   This module includes
 *
 *  init_sgr_specific()
 *
 *  pr_sgr()   pr_sgr_off()
 *  pr_bold()  pr_faint()
 *  pr_italics()
 *  pr_under()  pr_strike()
 *  pr_font()   pr_dou_under()
 *  pr_bold_off()
 *  pr_italics_off()  pr_under_off()
 *  pr_strike_off()
 *  pr_black()  pr_red()  pr_green()  pr_yellow()
 *  pr_blue()  pr_magenta() pr_cyan()  pr_no()
 *  pr_default() 
 *
 *  dec_sgr()  dec_sgr_off()
 *  dec_super()  dec_sub()  dec_over()
 *  dec_trans()  dec_super_sub_off()
 *  dec_over_off()  dec_trans_off()
 *
 *   end description
 *
 *-----------------------------------------------------------
 */



#define cafont_sgr	(1)

/*  begin include_file    */

#include    "portab.h"	    /* general portability constants */
#include    "cpsys.hc"	    /* general defs, typedefs, etc. */
#include    "cpast.hc"	    /* ANSI State Table & Token Table Declarations */
#include    "cpglobal.hc"   /* globals for CP modules */
#include    "caglobal.hc"   /* globals defs for the CARs */

#ifdef  DUMP
#include    "dumpu.hc"	    /* Dump Utility                             */
#include    "xlc_iface.hc"  /* defines for oprintf			*/
#endif DUMP

/*  end   include_file    */


/*************************************************************************
    Initialise a specific SGR
************************************************************************/

VOID init_sgr_specific(sgr_index)
WORD sgr_index;
   {
    xl_st.sgr_tbl [sgr_index].font_file	= cfont_def_sgr[sgr_index].font_file;
    xl_st.sgr_tbl [sgr_index].selection_type = cfont_def_sgr[sgr_index].selection_type;
    strcpy (xl_st.sgr_tbl [sgr_index].id_string, cfont_def_sgr[sgr_index].id_string);
   }


/*****  init_sgr_tbl() **************************************************
 * Init Sgr Table							*
 ************************************************************************/

VOID init_sgr_tbl()
   {
    WORD i;	    /*  Loop index */
    
    for (i = 0; i < 10; i++)
       {
	init_sgr_specific(i);
       }
   }


/*************************************************************************
    Select Graphic Rendition Bold On function
************************************************************************/

VOID pr_bold()

{
#ifdef DUMP
    {
    oprintf("SGR BOLD \n");
    }
#endif
    xl_st .requested_attributes |= BOLD;
}



/*************************************************************************
    Select Graphic Rendition Bold and Faint Off function
************************************************************************/

VOID pr_bold_off()

{
#ifdef DUMP
    {
    oprintf("SGR BOLD OFF \n");
    }
#endif
    xl_st .requested_attributes &= NO_BOLD;
}



/*************************************************************************
    Select Graphic Rendition Faint Dump Utility function
************************************************************************/

VOID pr_faint()

{
#ifdef DUMP
    {
    oprintf("SGR FAINT\n");
    }
#endif
}



/*************************************************************************
    Select Graphic Rendition Italics On function
************************************************************************/

VOID pr_italics()

{
#ifdef DUMP
    {
    oprintf("SGR ITALICS \n");
    }
#endif
    xl_st .requested_attributes |= ITALIC;
}




/*************************************************************************
    Select Graphic Rendition Italics Off function
************************************************************************/

VOID pr_italics_off()

{
#ifdef DUMP
    {
    oprintf("SGR ITALICS OFF \n");
    }
#endif
    xl_st .requested_attributes &= NO_ITALIC;
}




/*************************************************************************
    Select Graphic Rendition Underline function
************************************************************************/

VOID pr_under()

{
#ifdef DUMP
    {
    oprintf("SGR UNDERLINE \n");
    }
#endif
    xl_st .requested_attributes &= NO_DOU_UL;
    xl_st .requested_attributes |= UL;
    xl_st.curchar.attr_data.attr_baseline_offset = xl_st.curchar.ap.yval;
}



/*************************************************************************
    Select Graphic Rendition Underline Off function
    Turns off both underline and double underline
************************************************************************/

VOID pr_under_off()

{
#ifdef DUMP
    {
    oprintf("SGR UNDER OFF \n");
    }
#endif
    xl_st .requested_attributes &= NO_UL;
    xl_st .requested_attributes &= NO_DOU_UL;
}



/*************************************************************************
    Select Graphic Rendition Strike Through function
************************************************************************/

VOID pr_strike()

{
#ifdef DUMP
    {
    oprintf("SGR STRIKE \n");
    }
#endif
    xl_st .requested_attributes |= STRIKE;
}




/*************************************************************************
    Select Graphic Rendition Strike Thru off function
************************************************************************/

VOID pr_strike_off()

{
#ifdef DUMP
    {
    oprintf("SGR STRIKE OFF \n");
    }
#endif
    xl_st .requested_attributes &= NO_STRIKE;
}



/*************************************************************************
    DEC Private Select Graphic Rendition function
************************************************************************/

VOID dec_sgr()

{
#ifdef DUMP
{
    oprintf("DECSGR \n");
    pprint();		/* print parameter list */
}
#endif

    /*  Let the split routine call the appropriate action
     *  routine for each of the parameters.
     */
    cp_split(ast_decsgr_srch);

    invalidate_font ();
}



/*************************************************************************
    Select Graphic Rendition Select Font Style function
************************************************************************/

VOID pr_font()

{
#ifdef DUMP
    {
    oprintf("SGR FONT \n");
    }
#endif
    if  ( (cp_currpar >= 10) && (cp_currpar <= 19) )
	xl_st.cur_sgr = cp_currpar - 10;
}



/*************************************************************************
    Select Graphic Rendition Double Underline Dump Utility function
************************************************************************/

VOID pr_dou_under()

{
#ifdef DUMP
    {
    oprintf("SGR DOUBLE UNDER \n");
    }
#endif
    xl_st .requested_attributes &= NO_UL;
    xl_st .requested_attributes |= DOU_UL;
    xl_st.curchar.attr_data.attr_baseline_offset = xl_st.curchar.ap.yval;
}




/*************************************************************************
    Select Graphic Rendition Black Dump Utility function
************************************************************************/

VOID pr_black()

{
#ifdef DUMP
    {
    oprintf("SGR BLACK \n");
    }
#endif
}



/*************************************************************************
    Select Graphic Rendition Red Dump Utility function
************************************************************************/

VOID pr_red()

{
#ifdef DUMP
    {
    oprintf("SGR RED \n");
    }
#endif
}




/*************************************************************************
    Select Graphic Rendition Green Dump Utility function
************************************************************************/

VOID pr_green()

{
#ifdef DUMP
    {
    oprintf("SGR GREEN \n");
    }
#endif
}



/*************************************************************************
    Select Graphic Rendition Yellow Dump Utility function
************************************************************************/

VOID pr_yellow()

{
#ifdef DUMP
    {
    oprintf("SGR YELLOW \n");
    }
#endif
}



/*************************************************************************
    Select Graphic Rendition Blue Dump Utility function
************************************************************************/

VOID pr_blue()

{
#ifdef DUMP
    {
    oprintf("SGR  BLUE\n");
    }
#endif
}



/*************************************************************************
    Select Graphic Rendition Magenta Dump Utility function
************************************************************************/

VOID pr_magenta()

{
#ifdef DUMP
    {
    oprintf("SGR MAGENTA \n");
    }
#endif
}



/*************************************************************************
    Select Graphic Rendition Cyan Dump Utility function
************************************************************************/

VOID pr_cyan()

{
#ifdef DUMP
    {
    oprintf("SGR CYAN \n");
    }
#endif
}



/*************************************************************************
    Select Graphic Rendition No Printing Dump Utility function
************************************************************************/

VOID pr_no()

{
#ifdef DUMP
    {
    oprintf("SGR NO PRINT \n");
    }
#endif
}



/*************************************************************************
    Select Graphic Rendition Select Default Color Dump Utility function
************************************************************************/

VOID pr_default()

{
#ifdef DUMP
    {
    oprintf("SGR DEFAULT COLOR \n");
    }
#endif
}



VOID pr_sgr()

{
#ifdef DUMP
    {
    oprintf("SGR \n");
    pprint();		/* print parameter list */
    }
#endif

    /*  Let the split routine call the appropriate action
     *  routine for each of the parameters.
     */
    cp_split(ast_sgr_srch);

    invalidate_font ();
}



/*************************************************************************
    DEC Private Select Graphic Rendition Superscript function
************************************************************************/

VOID dec_super()

{

#ifdef DUMP
{
    oprintf("DEC SUPER \n");
}
#endif

    /* If Superscript is not on, then test if subscript is on  */
    if ((xl_st .requested_attributes & SUPERSCR) == 0)
	{
	    /* Set to 100% to get correct spacing on plu */

	    do_gsm(HUNDRED_PERCENT, HUNDRED_PERCENT);

	    if (xl_st .requested_attributes & SUBSCR) 
	    /* subscript is on, do plu */
	    {
		pr_plu();
	    }

	pr_plu();
	do_gsm(FIFTY_PERCENT, FIFTY_PERCENT);
	}
	
/* else were in superscript already, so do nothing */


/*
 *  Specifically turn off Subscript, as cannot have both together
 *  and set Superscript .
 */

    xl_st .requested_attributes &= NO_SUBSCR;
    xl_st .requested_attributes |= SUPERSCR;
}






/*************************************************************************
    DEC Private Select Graphic Rendition Subscript Dump Utility function
************************************************************************/

VOID dec_sub()

{

#ifdef DUMP
    {
    oprintf("DEC SUB \n");
    }
#endif
    /* If were in superscript mode, do a PLD */
    if (xl_st .requested_attributes & SUPERSCR) 
	{
	do_gsm(HUNDRED_PERCENT, HUNDRED_PERCENT);
	pr_pld();
	}

    /* If sub was not already set, do GSM */
    if ((xl_st .requested_attributes & SUBSCR) == 0)
	{
	do_gsm(HUNDRED_PERCENT, HUNDRED_PERCENT);
	pr_pld();
	do_gsm(FIFTY_PERCENT, FIFTY_PERCENT);
	}

/* else were in subscript already, so do nothing */


/*
 *  Specifically turn off Superscript, as cannot have both together.
 *  and set Subscript .
 */

    xl_st .requested_attributes &= NO_SUPERSCR;
    xl_st .requested_attributes |= SUBSCR;
}


/*************************************************************************
    DEC Private Select Graphic Rendition Super/Sub OFF function
************************************************************************/

VOID dec_super_sub_off()

{

#ifdef DUMP
    {
    oprintf("DEC SUPER/SUB OFF\n");
    }
#endif
    /* if either  super or subscript is set, do GSM of 100 */

    if (xl_st .requested_attributes & SUP_OR_SUB)
	    {
	    do_gsm(HUNDRED_PERCENT, HUNDRED_PERCENT);

	    /* if was superscript, do a pld */
	    if 	(xl_st .requested_attributes & SUPERSCR) 
		pr_pld();

	    /* if was subscript, do a plu */
	    if 	(xl_st .requested_attributes & SUBSCR) 
		pr_plu();
	    }

/*
 *  Turn off both attributes
 */	

    xl_st .requested_attributes &= NO_SUPERSCR;
    xl_st .requested_attributes &= NO_SUBSCR;
}





/*************************************************************************
    DEC Private Select Graphic Rendition Overline function
************************************************************************/

VOID dec_over()

{
#ifdef DUMP
{
    oprintf("DEC OVER \n");
}
#endif
    xl_st .requested_attributes |= OVERLINE;
}



/*************************************************************************
    DEC Private Select Graphic Rendition Overline Off function
************************************************************************/

VOID dec_over_off()

{
#ifdef DUMP
    {
    oprintf("DEC OVER OFF \n");
    }
#endif
    xl_st .requested_attributes &= NO_OVERLINE;
}


/*************************************************************************
    DEC Private Select Graphic Rendition Transparency Dump Utility function
************************************************************************/

VOID dec_trans()

{
#ifdef DUMP
    {
    oprintf("DEC TRANS \n");
    }
#endif
}


/*************************************************************************
    DEC Private Select Graphic Rendition Transparency Off Dump Utility function
************************************************************************/

VOID dec_trans_off()

{
#ifdef DUMP
    {
    oprintf("DEC TRANS OFF \n");
    }
#endif
}


/*************************************************************************
    Select Graphic Rendition Off function
************************************************************************/

VOID pr_sgr_off()

{
#ifdef DUMP
{
    oprintf("SGROFF \n");
    pprint();		/* print parameter list */
}
#endif
    dec_super_sub_off();
    dec_over_off();
    pr_bold_off();	
    pr_italics_off();
    pr_under_off();
    pr_strike_off();
    xl_st .requested_attributes = FALSE;
}




/*************************************************************************
    DEC Private Select Graphic Rendition Off function
************************************************************************/

VOID dec_sgr_off()

{
#ifdef DUMP
{
    oprintf("DECSGR OFF \n");
    pprint();		/* print parameter list */
}
#endif
    dec_super_sub_off();
    dec_over_off();

    xl_st .requested_attributes &= ~ALL_PR_ATTRS;
}
