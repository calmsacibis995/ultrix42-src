#ifndef lint
static char SccsId[] = "  @(#)caclib.c	4.1   LPS_ULT_TRN   7/2/90";
#endif

/* file: caclib.c
 *
 *	COPYRIGHT  (c)  DIGITAL  EQUIPMENT CORPORATION 1985,
 *	1986, 1987, 1988, 1989  ALL RIGHTS RESERVED.
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
 *   001   28-JAN-1988 12:45  mhw
 *         First version for common area
 *
 *   002    9-FEB-1988 16:16 mhs
 *         Change "x" parameter in test_against_limits from
 *         DEFAULT to LONG to match current description of
 *         BOUND structure in capdl.def.
 *
 *   003    3-MAR-1988 13:41 mhs
 *         Fix more arithmetic problems.  Logical expressions
 *         and comparitive equations must also be typecast.
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
 *   Filename:  caclib.c
 *
 *
 *   This module includes
 *
 * test_against_limits ()
 * copy_glyph ()
 * check_ap (above,below)  
 * get_left_bearing()	- Get left-bearing of char for justification purposes
 * get_right_bearing()	- Get right-bearing of char for justification purposes
 * get_width()		- Get width (in centipoints) of character
 * get_font_limits()	- Get min & max SPACE char widths for the specified font
 * get_font_height()
 *
 *   end description
 *
 *-----------------------------------------------------------
 */


/*  Edit History from Translator
 *
 * file:	xlc_clib.c - Translator utility routines
 * created:	gh	 26-MAR-1986 09:10:31
 * edit:	gh	 27-MAR-1986 09:55:37 Added test_against_limits,
 *			copy_glyph, copy_font, copy_cset, check_ap,
 *			get_font_height
 *		gh	 4-APR-1986 08:43:44 Added test of .pum_mode to .fcf
 *			test in check_ap()
 *		gh	 9-APR-1986 09:31:29 Corrected right-margin testing
 *			within check_ap().  Corrected copy_font & copy_cset.
 *		gh	 18-APR-1986 09:10:42 Corrected copy_glyph(), deleted
 *			copy_font & copy_cset due to changes in xlate.h
 *			Test was put into check_ap for .fcf pushing the .avp
 *			below the bottom margin.
 *		gh	 8-MAY-1986 11:42:50 Updated get_font_limits, 
 *			get_width, get_font_height.
 *		gh	 11-JUN-1986 09:57:52 Added test for proportional mode
 *			in get_width
 *		gh	 13-JUN-1986 07:54:42 Changing .plf indication of PLU
 *			and PLD conditions
 *		gh	 25-JUN-1986 09:36:24 Here's a good one - If a space
 *			char is received putting the .ahp beyond the right
 *			margin, BS, HPR, and HPB have an effect since .rmf
 *			does not get set.  Added an additional test in
 *			check_ap();
 *		nv	 10-JUL-1986 19:14:40 Redesigned get_width() to 
 *			support 'semi-prportional' mode.
 *			Added declaration for average_width_table.
 *		gh	 21-JUL-1986 11:29:48 Changed check_ap() to wrap
 *			on a space character
 *		gh	 13-AUG-1986 12:34:51 Changing check_ap to check the
 *			fcf on all occasions now.
 *		mgb	 30-SEP-1986 10:11:45 fixed test for VT when crossing
 *			bottom margin.
 *		mgb	 2-OCT-1986 17:50:41 added below baseline to test to
 *			see if past bottom margin.
 *		mgb	 21-OCT-1986 14:36:51 replaced below_baseline() 
 *			routine with a veryable called below_baseline[] to 
 *			improve performance.
 *		nv	 23-OCT-1986 18:34:22 Limit value of first character 
 *			flag to 'vai'. This solves the problem of having less 
 *			lines than the LN03 when the characters are taller
 *			than the line spacing (QAR's 204 and 206). 
 *
 *		araj	 24-OCT-1986 23:52:49 undid above, did 
 *			not help 204/206, as we would need to 
 *			limit to vai-below_baseline, and 
 *			confuses the issue, as we are neither 
 *			like the SRM nor the LN03.
 *			Makes tabs work weird, as we remove FCF, 
 *			but add MIN (FCF, vai).
 *
 *		mgb	 5-NOV-1986 10:33:53 added test for graphic state
 *			in check_ap to test for proper last line of page.
 *
 *		mgb	 10-DEC-1986 11:17:26 Change check_ap to have two
 *			parameters passed to it. This will eliminate the need
 *			to have tests for graphic mode and improve performance
 *			as well as make the code more readable. The parameters
 *			passed will be "above" for above baseline and "below"
 *			for below baseline.
 *
 */


#define caclib (1)

#include "portab.h"
#include "cpsys.hc"
#include "camac.lib_hc"
#include "caglobal.hc"


/*****  test_against_limits()  ******************************************
 *									*
 *	test_against_limits(x, plimits) - Test 'x' against the limits	*
 *	struture pointed to by 'plimits'.				*
 *	Return:		=0 if x is within limits.			*
 *			=1 if x is < limit .min value			*
 *			=2 if x is >= limit .max value			*
 ************************************************************************/

DEFAULT test_against_limits(x,plimits)
LONG x;
BOUND	*plimits;
{
    if (x < plimits->min)
    		return MIN_LIM_EXC;
    if (x >= plimits->max)
    		return MAX_LIM_EXC;
    return LIM_NOT_EXC;
}


/*****  check_ap(above,below)  ******************************************
 *									*
 *	check_ap(above,below) - Check active position			*
 *				above - above baseline			*
 *				below - below baseline			*
 ************************************************************************/

VOID	check_ap(above,below)

LONG 	above,below;
{

    /* If the current ahp is less than the left margin, reset it */
    if (xl_st.curchar.ap.xval < xl_st.h_lim_bound.min)
    	xl_st.curchar.ap.xval = xl_st.h_lim_bound.min;

    /* IF JUSTIFY MODE NOT ON, Check if printing the next character will cause 
     * ahp to exceed right margin.  If so, wrap or truncate depending on 
     * setting of wrap mode.  
     */
    if	(   (!xl_st.justify_mode) 
	&&  ((xl_st.curchar.ap.xval + xl_st.curchar.char_data.char_code) > xl_st.h_lim_bound.max)
	)  
	{
	    if  (xl_st.wrap_mode)	
		{
		    /* is VAI valid, or did a font change invalidate it */
		    if  (xl_st.vai_valid == FALSE)
			{
			    compute_vai();
			}
			
		    /* Update .avp to value of new line, do <FF> if necessary */    

		    vert_rel_w_wrap(xl_st.vai);
		    hpos_abs(xl_st.h_fmt_bound.min);
		    xl_st.plf = NOPLUPLD;	/* Clear PLU/PLD flag */

		} /* end if wrap mode */
    		else	
		{
		    xl_st.rmf = TRUE;
		} /* end else for if wrap mode */

	} /* end if "not jfy and beyond right margin */

    /* If the current avp is less than the top margin, set it to the top */

    if ((xl_st.curchar.ap.yval < xl_st.v_lim_bound.min) && 
        (xl_st.plf <= NOPLUPLD))
    	xl_st.curchar.attr_data.attr_baseline_offset = xl_st.curchar.ap.yval = xl_st.v_lim_bound.min;

    if (xl_st.fcf)
       {
	xl_st.curchar.ap.yval += above;
	xl_st.fcf = FALSE;
	xl_st.curchar.attr_data.attr_baseline_offset = xl_st.curchar.ap.yval;
       }

    /* possible wrap condition?   */
    /* no sub/superscript         */
    /* are the margins too close? */
    if ( ((xl_st.curchar.ap.yval + below) > xl_st.v_lim_bound.max) 
      && (xl_st.plf >= NOPLUPLD)
      && (xl_st.curchar.ap.yval - xl_st.v_lim_bound.min > above) )
	{
		pr_ff ();	
	    	xl_st.curchar.ap.yval += above;
		xl_st.curchar.attr_data.attr_baseline_offset = xl_st.curchar.ap.yval;
		xl_st.fcf = FALSE;
	}
}

/*****  copy_glyph()  ***************************************************
 *									*
 *	copy_glyph(fglyph,tglyph) - Copy glyph pointed to 'fglyph' to	*
 *	the glyph structure pointed to by 'tglyph'.			*
 ************************************************************************/

VOID	copy_glyph(fglyph,tglyph)
GLYPH	*fglyph,*tglyph;
{
    *tglyph = *fglyph ;
}


/*****  get_left_bearing()  *********************************************
 *									*
 *	get_left_bearing(char,font) - Set the variable 'left_bearing'	*
 *	to the center-point of the character for the purpose of		*
 *	justification.  It is assumed the character specified is going	*
 *	to be the left-most character within a justified line.		*
 ************************************************************************/
LONG	get_left_bearing(lchar,lfont)
WORD	lchar,lfont;
{
    return((LONG)FNT_LEFT_BEARING(lfont,lchar));
}


/*****  get_right_bearing()  ********************************************
 *									*
 *	get_right_bearing(char,font) - Set the variable 'right_bearing'	*
 *	to the center-point of the character for the purpose of		*
 *	justification.  It is assumed the character specified is going	*
 *	to be the right-most character within a justified line.		*
 ************************************************************************/
LONG	get_right_bearing(rchar,rfont)
WORD	rchar,rfont;
{
    return((LONG)FNT_RIGHT_BEARING(rfont,rchar));
}


/*****  get_width()  ****************************************************
 *									*
 *	get_width(char,font) - Return the width, in centipoints, of the	*
 *	specified character within the specified font.			*
 ************************************************************************/
LONG    get_width(wchar,wfont)
WORD    wchar,  /* character code of desired glyph */
        wfont;  /* PS font number (0-23) of desired font */
{
    /* If in mono-spaced mode, use the commanded horizontal advance increment.*/
    if (xl_st .propl_mode == MONO_SPACED_MODE)
      return (xl_st .hai);

    /* If in proportional mode, return the value from the width table. */
    else if (xl_st .propl_mode == PROPORTIONAL_MODE)
      return ((LONG)FNT_WIDTH(wfont,wchar));

    /* If in semi-proportional mode, return the average width table value. */
    else
      return ((LONG)FNT_AVERAGE_WIDTH(wfont));

}



/*****  get_font_limits()  **********************************************
 *									*
 *	get_font_limits(char,font,plimits) - Set the 'min' and 'max'	*
 * 	members of BOUND structure 'plimits' to the minimum and maximum	*
 * 	allowable width of a 'char' type of character within the font 	*
 *	specified.  This can be used to keep justification from 	*
 *	stretching a line that shouldn't be.				*
 ************************************************************************/
VOID	get_font_limits(fchar,ffont,fplimits)
WORD	fchar;
WORD	ffont;
BOUND	*fplimits;
{
    fplimits->min = (LONG)FNT_MIN(ffont);
    fplimits->max = (LONG)FNT_MAX(ffont);
}

/*****  get_font_height()  **********************************************
 *									*
 *	get_font_height(font) - Return with the height in centipoints	*
 *	of the specified font.						*
 ************************************************************************/
LONG	get_font_height(gffont)
WORD	gffont;
{
    return ((LONG)FNT_HEIGHT(gffont));
}

