#ifndef lint
static char SccsId[] = "  @(#)capctrl.c	4.1   LPS_ULT_TRN   7/2/90";
#endif

/* file: capctrl.c
 *
 *	COPYRIGHT  (c)  DIGITAL  EQUIPMENT CORPORATION 1985,
 *	1986, 1987, 1988, 1989.  ALL RIGHTS RESERVED.
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
 *  18-JAN-1988 11:27 mhw
 *	Combined dupctrl.c and capctrl.c using #ifdef for
 *      conditional compile of Dump Utility Routines.      
 *          
 *      Changed average_width_table to a macro called
 *      AVERAGE_WIDTH, declared in camac.lib
 *
 *      Deleted references to external functions get_width and 
 *      get_font_height. Will eventually be functions in caclib.c
 *
 *   8-FEB-1988 16:20  bf
 *	Changed return value for vpos_rel_w_wrap().
 *
 *  15-FEB-1988 15:41  mhw
 *	Added real pr_cuu code
 *
 *   7-MAR-1988 10:56 mhw
 *	Fix undocumented changes as to the order of the routines.\
 *	Order of the routines probably should be changed so that we
 *	do not need to declare them separately in many routines, but
 *	this needs to be investigated further and we currently do
 *	not have time to make working code look prettier and in the 
 *	process break it.  Easiest solution is to add routines to 
 *	def file using hce_func.
 *
 *  15-MAR-1988 08:39 mhw
 *	Add bug fix found by translator for QAR 006 NEL in pr_nel   
 *
 *   7-JUL-1988 13:35 mhw
 *	Change hdist and vdist paramaters to be LONG from WORD
 *
 *  13-JUL-1988 14:36 mhw
 *	Remove duplicate testing from PLU and PLD maximum bounds
 *
 *   4-AUG-1988 15:57 mhw
 *	added calls to process_vhar to pr_hpa, pr_hpr and pr_hpb
 *	also added new attributes to these rtns, ie. dou_under, overline, etc.
 *
 *  11-OCT-1988 15:47 mhw
 *	Changed pr_ff call to vpr from using pagelength +1 to using infinity.
 *	This is because issuing PLU could have made the avp negative.
 *
 *  20-OCT-1988 13:12 mhw
 *	Remove italics from the attribute list in hpa, hpr and hpb
 * 
 *   2-NOV-1988 09:00 mhw
 *	Add pr_cond_ff for use by dec_asfc
 *
 *  18-DEC-1988 15:19 ejs
 *		Major changes to data structutes.  GLYPH has been modified
 *		to allow struct copies more easily.  The PS output is not
 *		changed (barring bugs of course).
 *
 *  28-MAR-1989 10:15 araj
 *		Modified hdist to ensure that GL is valid before using
 *
 *
 *   9-APR-1989 16:01 ejs
 *		Modified pr_lf to include the CR code for justify.  This
 *		mimics the pr_ind, pr_ri code. All in responce to qar5.
 *
 *  12-APR-1989 21:37	araj
 *		fix hdist, semi-proportional was not multiplying by hparm
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
 *   Filename:  capctrl.c
 *
 *   Combined version of Dump Utility and real CARs.
 *
 *   This module includes
 *
 * pr_lf ()		pr_bs ()
 * pr_ff ()		pr_cr ()
 * pr_hpa ()      	pr_hpr()      	pr_hpb ()
 * pr_vpa ()      	pr_vpr ()	pr_vpb ()
 * pr_ind ()		pr_nel ()
 * pr_pld ()      	pr_plu ()	pr_ri ()
 * pr_cuu()
 *
 *
 * vert_rel_w_wrap ()	pr_cond_ff()
 * hdist ()		vdist ()
 * hpos_abs ()		hpos_rel ()
 * vpos_abs ()		vpos_rel ()	vpos_rel_w_wrap ()
 * update_ahp ()	update_avp ()
 *
 *   end description
 *
 *-----------------------------------------------------------
 */



/* Edit History of this file when it was part of the Translator
 * file:	xlc_poscon.c - Translator position control routines
 * created:	gh	 26-MAR-1986 09:10:31
 * edit:	gh	 27-MAR-1986 12:14:29 Added hpos_abs, hpos_rel,
 *			update_ahp, update_avp, hdist, vdist, vert_rel_w_wrap,
 *			hpb, vrb
 *		gh	 9-APR-1986 15:59:12 Fixing handling of justification
 *			in pr_ff() and pr_cr()
 *		gh	 24-APR-1986 10:25:49 Updating to new codegen routines
 *		gh	 2-MAY-1986 11:04:58 Adding test against lim bounds
 *			 to vpos_rel()
 *		gh	 9-JUN-1986 10:37:09 Changed hpa() and vpa() to 
 *			 decrement plist[0] only if in character mode.
 *		gh	 12-JUN-1986 15:11:22 Changing the calculation of the
 *			 delta .avp from .vai/2 to referencing the superscript
 *			 and subscript tables.
 *		gh	 13-JUN-1986 08:09:10 Changing .plf interpretation from
 *			 FALSE/PLU/PLD to 0 and nesting values
 *		gh	 20-JUN-1986 14:27:40 Adding virtual character
 *			 generation in HPA, HPR, HPB for attribute extension
 *		gh	 25-JUN-1986 09:24:39 Added right at the start of HPR()
 *			 and HPB() a test for the right margin flag being set.
 *			 The call to hpos_rel() tests for it, but now that 
 *			 virtual characters are output by hpr & hpb, the test
 *			 really belongs at the start of thos routines.
 *		nv	 10-JUL-1986 18:33:36 Modified hdist().
 *			 Added declaration for average_width_table.
 *		gh	 21-JUL-1986 09:14:11 Changed pr_cr() to do justification
 *		gh	 21-JUL-1986 12:50:13 Changed all plist parameters
 *			 from 0-based to 1-based
 *		gh	 24-JUL-1986 11:27:28 Removing duplicate clearing of
 *			 .plf in pr_lf, pr_ff, pr_cr, vpa, vpr, vpb, pr_ind,
 *			 and pr_ri();
 *		nv	 30-JUL-1986 10:34:30 Changed the way the
 *			ps_font_number of GL is accessed on account of the
 *			changed declaration of .gl to .gl_ptr.
 *		gh	 6-AUG-1986 10:27:46 Removed call to justify in vpos_rel
 *		gh	 12-AUG-1986 10:14:22 Added the clearing of .fcf in VPA
 *			if in unit mode 
 *		laf	 27-AUG-1986 15:52:04 Added updating of ul_avp to
 *			update_avp() (for UL/PLU/PLD)
 *
 *		araj	 7-OCT-1986 17:01:48 Modified pos-rel to 
 *			use margins as a limit when not in JFY 
 *			mode, but only check for overflow when 
 *			in JFY mode
 *
 *		araj	 21-OCT-1986 17:33:10 modified VPR to 
 *			clear FCF when in unit mode.
 *
 *		nv	 23-OCT-1986 18:19:10 Modify 'hdist' in character mode
 *			to use 'hai' even in proportional mode; the only 
 *			exception remains semi-proportional mode.
 *
 *		mgb	 29-APR-1987 11:29:50 Modified routine 
 *			vert_rel_w_wrap() by taking out section that does
 *			verticle possition with wrap and created a sepperate
 *			routine called vpos_rel_w_wrap().
 *
 *		mgb	 11-JUN-1987 16:08:23 Took out unused VOIDs 
 *
 *		mgb	 11-JUN-1987 16:09:02 Modified pr_nel() to update
 *			ahp with xl_st.h_fmt_bound.min instead of using
 *			xl_st.h_lim_bound.min.
 *			Also modified vpos_rel() to update
 *			avp with xl_st.v_fmt_bound.min instead of using
 *			xl_st.v_lim_bound.min.
 *
 *              kws	 19-JUN-1987 12:28:54 Change LF, IND, and RI to use
 *                      the active horizontal position after the previous
 *                      text has been justified.
 *
 *		araj	 16-JUL-1987 22:11:58 
 *			Attempt to fix QAR LPS40MR 00145,
 *			Modified PLU/PLD to use the subscript
 *			or superscript value of the font used for G0
 *			rather than that of the font used by the last 
 *			character. For 2 reasons, one, it may vary
 *			depending on whether the last character was 
 *			taken out of G0/1/2/3, two it is only updated
 *			when a character is printed, not when a font
 *			change occurs. Created a temp varibale to
 *			hold the font used for G0. In an optimization 
 *			phase, we might want to discard the temp 
 *			variable  and use in line code.
 *
 */



#define capctrl (1)

/*  begin include_file    */

#include    "portab.h"	    /* For portable definitions of types, etc.	*/
#include    "cpsys.hc"      /* Globals for entire parser                */
#include    "cpglobal.hc"   /* include all globals for cpxxx.c files    */
#include    "caglobal.hc"   /* include all globals for caxxx.c files    */
#include    "camac.lib_hc"  /* library of macros for Common ANSI AR     */

#ifdef DUMP
#include    "xlc_iface.hc"  /* defines for oprintf - Dump Utility	*/
#include    "dumpu.hc"      /* Dump Utility Printing                    */
#endif DUMP

/*  end   include_file    */



/*****  hdist()  ********************************************************
 *									*
 *	hdist(hparm) - Checks whether the translator is currently in	*
 *	CHAR or SIZEUNIT mode, and returns the given paramter converted	*
 *	to centipoints.							*
 ************************************************************************/

LONG	hdist(hparm)
LONG	hparm;
{

  if (xl_st.pum_mode == PUM_CHAR)	/* In CHAR mode ? */
     {  /* Yes */
	if (xl_st.hai != 0)		/* Monospaced ? */
	{
		return ( hparm * xl_st.hai);
	}
	else
	{
	    if	(!xl_st.gl_ptr->gset_valid)	/* if not ready, compute current GL */
		{
		    compute_font_for_g_set ( xl_st.gl_ptr - (&xl_st.g_table[0]));
		}
	    return (hparm * FNT_AVERAGE_WIDTH(xl_st.gl_ptr->gset_map[32].char_font));
	}
    }
  /* No  */
  else return ( (LONG)xl_st.sizeunit * hparm );
}



/*****  vdist()  ********************************************************
 *									*
 *	vdist(vparm) - Checks whether the translator is currently in	*
 *	CHAR or SIZEUNIT mode, and returns the given paramter converted	*
 *	to centipoints.							*
 ************************************************************************/

LONG	vdist(vparm)
LONG	vparm;
{
    if	(xl_st.pum_mode == PUM_CHAR)	/* In CHAR mode ? */
	{
	    /* is VAI valid, or did a font change invalidate it */
	    if (xl_st.vai_valid == FALSE)
	    {
		compute_vai();
	    }
	    return ( (LONG)xl_st.vai * vparm );	/* Yes */
	}	
    else return ( (LONG)xl_st.sizeunit * vparm );	/* No  */
}


/*****  hpos_abs()  *****************************************************
 *									*
 *	hpos_abs(hapos) - Horizontal position, absolute mode.		*
 *	'Hapos' is the absolute .ahp value to go to.			*
 ************************************************************************/

VOID	hpos_abs(hapos)
LONG	hapos;
{

    /* Empty the justify buffer if there is anything in it */
    empty_jfy_buf();

    /* Clear 'right margin flag' in case it was previously set and this
       hpos_abs call will bring the ap back into useful range */
    xl_st.rmf = FALSE;

    update_ahp(hapos, &xl_st.h_lim_bound);
}



/*****  hpos_rel()  *****************************************************
 *									*
 *	hpos_rel(hrpos) - Horizontal position, relative mode.		*
 *	'Hrpos' is the offset ot add to the current ahp.		*
 ************************************************************************/

VOID	hpos_rel(hrpos)
LONG	hrpos;
{

    /* If the 'right margin flag' is already set, DO NOT move the current
    	location ! */
    if (xl_st.rmf)		return;

    /* if in JFY mode, check against overflow only else check 
	against margins */
    if (xl_st.justify_mode)  
	{
	    update_ahp(xl_st.curchar.ap.xval + hrpos, &max_bound);
	}
	else
	{
	    update_ahp(xl_st.curchar.ap.xval + hrpos, &xl_st.h_lim_bound);
	}
}


/*****  vpos_abs()  *****************************************************
 *									*
 *	vpos_abs(vapos) - Vertical position, absolute mode.		*
 *	'Vapos' is the absolute .avp value to go to.			*
 ************************************************************************/

VOID	vpos_abs(vapos)
LONG	vapos;
{

    /* Empty the justify buffer */
    empty_jfy_buf();

    /* Update the current avp */
    update_avp(vapos, &xl_st.v_lim_bound);

    xl_st.fcf = TRUE;	/* Set 'first-character-flag' */
}


/*****  vpos_rel()  *****************************************************
 *									*
 *	vpos_rel(vrpos) - Vertical position, relative mode.		*
 *	'Vrpos' is the offset ot add to the current avp.		*
 ************************************************************************/

VOID	vpos_rel(vrpos)
LONG	vrpos;
{

    /* Update the current avp */
    update_avp(xl_st.curchar.ap.yval + vrpos, &xl_st.v_lim_bound);

    switch (xl_st.limit_flag)  {
    	case MIN_LIM_EXC:
    		/* If new avp exceeds top margin, set = top margin 
    		   and set the first character flag */
    		update_avp(xl_st.v_fmt_bound.min, &xl_st.v_lim_bound);
    		xl_st.fcf = TRUE;
    		break;
    	case MAX_LIM_EXC:
    		/* If new avp exceeds bottom margin, wrap to next page 
    		   and set fcf */
    		process_showpage();	/* Yes, purge current page */
    		update_avp(xl_st.v_fmt_bound.min, &xl_st.v_lim_bound);
    		xl_st.fcf = TRUE;
    }
}


/*****  update_ahp()  ***************************************************
 *									*
 *	update_ahp(hpos,phlim) - Update active horizontal position.	*
 *	Update xl_st.limit_flag with the result of 'test_against_	*
 *	limits()', and if a limit was exceeded, set xl_st.curchar.ahp 	*
 *	to that	limit.  Otherwise, set xl_st.curchar.ahp to 'hpos'.	*
 ************************************************************************/

VOID	update_ahp(hpos,phlim)
LONG	hpos;
BOUND	*phlim;
{
    switch (xl_st.limit_flag = test_against_limits(hpos, phlim))  {
    	case MIN_LIM_EXC:
    		xl_st.curchar.ap.xval = phlim->min;
		break;
    	case MAX_LIM_EXC:
    		xl_st.curchar.ap.xval = phlim->max;
    		xl_st.rmf = TRUE;	/* Set 'right margin flag' */
		break;
    	default:
    		xl_st.curchar.ap.xval = hpos;
    }
}



/*****  update_avp()  ***************************************************
 *									*
 *	update_avp(vpos,pvlim) - Update active vertical position.	*
 *	Update xl_st.limit_flag with the result of 'test_against_	*
 *	limits()', and if a limit was exceeded, set xl_st.curchar.avp	*
 *	to that limit.  Otherwise, set xl_st.curchar.avp to 'vpos'.	*
 ************************************************************************/

VOID	update_avp(vpos,pvlim)
LONG	vpos;
BOUND	*pvlim;
{
    xl_st.plf = NOPLUPLD;		/* Indicate neither PLU nor PLD */

    switch (xl_st.limit_flag = test_against_limits(vpos, pvlim))  {
    	case MIN_LIM_EXC:
    		xl_st.curchar.ap.yval = pvlim->min;		break;
    	case MAX_LIM_EXC:
    		xl_st.curchar.ap.yval = pvlim->max;		break;
    	default:
    		xl_st.curchar.ap.yval = vpos;
    }

    xl_st.curchar.attr_data.attr_baseline_offset = xl_st.curchar.ap.yval;
}


/*****  vpos_rel_w_wrap()  **********************************************
 *									*
 *	vpos_rel_w_wrap(vrwpos) - Vertical position, relative mode.	*
 *	'Vrpos' is the offset ot add to the current avp and		*
 *	wrap to a new page if end of page is hit.			*
 *									*
 ************************************************************************/

VOID vpos_rel_w_wrap(vrwpos)
LONG	vrwpos;
{

    /* Update the current avp */
    update_avp(xl_st.curchar.ap.yval + vrwpos, &xl_st.v_fmt_bound);

    switch (xl_st.limit_flag)  {
    	case MIN_LIM_EXC:
    		/* If new avp exceeds page home line, set = page home line
    		   and set the first character flag */
    		update_avp(xl_st.v_fmt_bound.min, &xl_st.v_lim_bound);
    		xl_st.fcf = TRUE;
    		break;
    	case MAX_LIM_EXC:
    		/* If new avp exceeds page end line, wrap to next page 
    		   and set fcf */
    		process_showpage();	/* Yes, purge current page */
    		update_avp(xl_st.v_fmt_bound.min, &xl_st.v_lim_bound);
    		xl_st.fcf = TRUE;
    }
}


/*****  vert_rel_w_wrap()  **********************************************
 *									*
 *	vert_rel_w_wrap(vrwpos) - Do a vertical relative move, and	*
 *	wrap to a new page if end of page is hit.			*
 ************************************************************************/

VOID	vert_rel_w_wrap(vrwpos)
LONG	vrwpos;
{
    /* If justify mode is on, justify the current contents, and empty it */
    if (xl_st.justify_mode)  
       {
	justify_buffer();
	empty_jfy_buf();
       }
    vpos_rel_w_wrap(vrwpos);
}


/*****  pr_cond_ff()  **************************************************
 *									*
 *	pr_cond_ff() - Effect a form feed if something on the page	*
 ************************************************************************/
VOID pr_cond_ff () 

{
    /* If justify mode is on, justify the current contents, and empty it */
    if (xl_st.justify_mode)  
       {
	justify_buffer();
	empty_jfy_buf();
       }

	process_condshowpg();	/* purge current page */
	update_avp(xl_st.v_fmt_bound.min, &xl_st.v_lim_bound);
	xl_st.fcf = TRUE;

}



/*****  pr_lf()  ********************************************************
 *									*
 *	pr_lf() - Effect a line feed.					*
 ************************************************************************/

VOID pr_lf ()

{
#ifdef DUMP
    {
    oprintf("LF \n");
    }
#endif

    /* If justification is on, justify the line and empty the buffer. */

    if (xl_st.justify_mode)
       {
    	justify_buffer();
    	empty_jfy_buf();

        /* ... interpret LF as CR/LF, so do the CR first */
        hpos_abs(xl_st.h_fmt_bound.min);

       }

    /* is VAI valid, or did a font change invalidate it */
    if	(xl_st.vai_valid == FALSE)
	{
	    compute_vai();
	}

    /* Update .avp to value of new line, do <FF> if necessary */    
    vert_rel_w_wrap(xl_st.vai);

    /* If "newline mode" is set ... */

    if (xl_st.lfnl_mode != 0) 
       {        
        /* ... interpret LF as CR/LF, so do the CR first */
        hpos_abs(xl_st.h_fmt_bound.min);
       }
}



/*****  pr_ff()  ********************************************************
 *									*
 *	pr_ff() - Effect a form feed.					*
 ************************************************************************/
VOID pr_ff () 


{
#ifdef DUMP
    {
    oprintf("FF \n");
    }
#endif
    /* Empty any contents of the justify buffer first */
    empty_jfy_buf();

    /* Update .avp to value of new line, do <FF> if necessary */    
    vert_rel_w_wrap(PAGE_INFINITY);
}



/*****  pr_cr()  ********************************************************
 *									*
 *	pr_cr() - Effect a carriage return.				*
 ************************************************************************/
VOID pr_cr () 


{
#ifdef DUMP
    {
    oprintf("CR \n");
    }
#endif

    /* If CR-newline mode is not enabled, this is really the equivalent to
       a HPA(0), so empty the justify buffer and position absolutely the
       active postion to the left format bound. */
    if (xl_st.justify_mode)  
       {
    	justify_buffer();
    	empty_jfy_buf();
       }

    hpos_abs(xl_st.h_fmt_bound.min);	/* Go to start of line */

    if (xl_st.crnl_mode)
       {
	    /* is VAI valid, or did a font change invalidate it */
	    if	(xl_st.vai_valid == FALSE)
		{
		    compute_vai();
		}
    	/* Then, go to a new line and reset the PLU/PLD flag */
        vert_rel_w_wrap(xl_st.vai);		/* Go to new line */
       }
}



/*****  pr_bs()  ********************************************************
 *									*
 *	pr_bs() - Effect a backspace.					*
 ************************************************************************/
VOID pr_bs () 
   {
#ifdef DUMP
   {
    oprintf("BS \n");
   }
#endif
    /* If the 'right margin flag' is already set, DO NOT move the current
     * location !
     */
    if (!(xl_st.rmf))
       {
        hpos_rel(-(get_width((WORD)SP_C, xl_st.curchar.char_data.char_font)));
       }
   }


/*****  pr_hpa()  *******************************************************
 *									*
 *	hpa() - Move active position to character column # Pn 		*
 *	(Pn is relative to origin 1 - decrement 1 to origin to 0)	*
 ************************************************************************/
VOID pr_hpa () 
{
    LONG	from_ahp;
    GLYPH	vchar;

#ifdef DUMP
    {
    oprintf("HPA \n");
    pprint();		    /* print parameter list */   
    }
#endif
    /* If a parameter value of zero is passed, convert it to a one.
     * Then decrement the effective parameter value by one to make relative
     * to an origin of 0,0 
     */
    if (cp_pbuf[0] == 0)
       {
        cp_pbuf[0] = 1;
       }
    cp_pbuf[0]--;

    from_ahp = xl_st.curchar.ap.xval;	/* Save current ahp */
    if (xl_st.requested_attributes & (UL | STRIKE | DOU_UL | OVERLINE ))
    	copy_glyph(&xl_st.curchar,&vchar);
    hpos_abs(hdist(cp_pbuf[0]));

    /* Send a virtual character that covers the distance just moved -
       this is in case strike-thru or underlining is enabled */
    if (xl_st.requested_attributes & (UL | STRIKE | DOU_UL | OVERLINE ))  
       {
    	vchar.font_data.algorithmic_attributes =
		    xl_st.requested_attributes;
    	vchar.char_data.char_code = VIR_CHAR_SPACE;
    	vchar.char_data.char_width = xl_st.curchar.ap.xval - from_ahp;
	process_vchar(&vchar);
       }
}


/*****  pr_hpr()  *******************************************************
 *									*
 *	hpr() - Move active position 'Pn' # of character columns in the	*
 *	positive direction from the current position 			*
 ************************************************************************/
VOID pr_hpr () 
{
    LONG	from_ahp;
    GLYPH	vchar;
#ifdef DUMP
    {
    oprintf("HPR \n");
    pprint();		    /* print parameter list */   
    }
#endif

    /* If the 'right margin flag' is already set, DO NOT move the current
    	location ! */
    if (xl_st.rmf)
       {
    	return;
       }

    /* If a parameter value of zero is passed, convert it to a one. */
    if (cp_pbuf[0] == 0)		cp_pbuf[0] = 1;
    from_ahp = xl_st.curchar.ap.xval;	/* Save current ahp */
    if (xl_st.requested_attributes & (UL | STRIKE | DOU_UL | OVERLINE ))
    	copy_glyph(&xl_st.curchar,&vchar);
    hpos_rel(hdist(cp_pbuf[0]));

    /* Send a virtual character that covers the distance just moved -
       this is in case strike-thru or underlining is enabled */
    if (xl_st.requested_attributes & (UL | STRIKE | DOU_UL | OVERLINE ))  
       {
    	vchar.font_data.algorithmic_attributes =
    		xl_st.requested_attributes;
    	vchar.char_data.char_code = VIR_CHAR_SPACE;
    	vchar.char_data.char_width = xl_st.curchar.ap.xval - from_ahp;
    	if (!xl_st.justify_mode)  
           {
    	    process_vchar(&vchar);
    	    return;
    	   }
    	add_to_jfy_buf(&vchar);
       }
}



/*****  pr_hpb()  *******************************************************
 *									*
 *	hpb() - Move active position 'Pn' # of character columns in the	*
 *	negative direction from the current position 			*
 ************************************************************************/
VOID pr_hpb () 
{
    LONG	from_ahp;
    GLYPH	vchar;

#ifdef DUMP
    {
    oprintf("HPB \n");
    pprint();		    /* print parameter list */   
    }
#endif
    /* If the 'right margin flag' is already set, DO NOT move the current
    	location ! */
    if (xl_st.rmf)
       {
    	return;
       }

    /* If a parameter value of zero is passed, convert it to a one. */
    if (cp_pbuf[0] == 0)
       {
    	cp_pbuf[0] = 1;
       }
    from_ahp = xl_st.curchar.ap.xval;	/* Save current ahp */
    if (xl_st.requested_attributes & (UL | STRIKE | DOU_UL | OVERLINE ))
       {
    	copy_glyph(&xl_st.curchar,&vchar);
       }
    hpos_rel(-(hdist(cp_pbuf[0])));

    /* Send a virtual character that covers the distance just moved -
       this is in case strike-thru or underlining is enabled */
    if (xl_st.requested_attributes & (UL | STRIKE | DOU_UL | OVERLINE )) 
       {
    	vchar.font_data.algorithmic_attributes =
    		xl_st.requested_attributes;
    	vchar.char_data.char_code = VIR_CHAR_SPACE;
    	vchar.char_data.char_width = xl_st.curchar.ap.xval - from_ahp;
    	if (!xl_st.justify_mode)  
           {
    	    process_vchar(&vchar);
    	    return;
    	   }
    	add_to_jfy_buf(&vchar);
       }
}


/*****  pr_vpa()  *******************************************************
 *									*
 *	vpa() - Move active position to character row # Pn 		*
 *	(Pn is relative to origin 1 - decrement 1 to origin to 0)	*
 ************************************************************************/
VOID pr_vpa () 

{
#ifdef DUMP
    {
    oprintf("VPA \n");
    pprint();		    /* print parameter list */   
    }
#endif
    /* If a parameter value of zero is passed, convert it to a one.
       Then decrement the effective parameter value by one to make relative
       to an origin of 0,0 */
    if (cp_pbuf[0] == 0)
       {
    	cp_pbuf[0] = 1;
       }
    cp_pbuf[0]--;
    vpos_abs(vdist(cp_pbuf[0]));

    /* If in unit mode, cancel the setting of the .fcf */
    if (xl_st.pum_mode == PUM_SIZEUNIT)
       {
	xl_st.fcf = FALSE;	/* Clear 'first-character-flag' */
       }
}


/*****  pr_vpr()  *******************************************************
 *									*
 *	vpr() - Move active position 'Pn' # of character rows in the	*
 *	positive direction from the current position 			*
 ************************************************************************/
VOID pr_vpr () 
{
#ifdef DUMP
    {
    oprintf("VPR \n");
    pprint();		    /* print parameter list */   
    }
#endif

    /* If a parameter value of zero is passed, convert it to a one. */
    if (cp_pbuf[0] == 0)
       {
    	cp_pbuf[0] = 1;
       }
    vpos_rel(vdist(cp_pbuf[0]));


    /* If in unit mode, cancel the setting of the .fcf */
    if (xl_st.pum_mode == PUM_SIZEUNIT)
       {
        xl_st.fcf = FALSE;	/* Clear 'first-character-flag' */
       }
}


/*****  pr_vpb()  *******************************************************
 *									*
 *	vpb() - Move active position 'Pn' # of character rows in the	*
 *	negative direction from the current position 			*
 * 	NOTE: This routine is identical in functionality to CCU, so	*
 *	it is the responsibility of the parser to call this routine 	*
 *	if either a VPB or CCU command is received.			*
 ************************************************************************/
VOID pr_vpb () 


{
#ifdef DUMP
    {
    oprintf("VPB \n");
    pprint();		    /* print parameter list */   
    }
#endif

    /* If a parameter value of zero is passed, convert it to a one. */
    if (cp_pbuf[0] == 0)
       {
	cp_pbuf[0] = 1;
       }
    vpos_rel(-(vdist(cp_pbuf[0])));
}


/*****  pr_ind()  *******************************************************
 *									*
 *	pr_ind() - The IND control character is executed the same as a	*
 *	LF control character except that the setting of the linefeed/	*
 *	newline mode has no effect.					*
 ************************************************************************/
VOID pr_ind () 

{
#ifdef DUMP
    {
    cp_exit_cur_mode();
    oprintf("IND \n");
    }
#endif

    /* If justification is on, justify the line and empty the buffer. */

    if (xl_st.justify_mode)  
       {
    	justify_buffer();
    	empty_jfy_buf();

        /* ... interpret IND as CR/LF, so do the CR first */

        hpos_abs(xl_st.h_fmt_bound.min);
       }

    /* is VAI valid, or did a font change invalidate it */
    if (xl_st.vai_valid == FALSE)
    {
	compute_vai();
    }

    /* Update .avp to value of new line, do <FF> if necessary */    
    vert_rel_w_wrap(xl_st.vai);
    cp_exit_cur_mode();
}



/*****  pr_nel()  *******************************************************
 *									*
 *	pr_nel() - The NEL (NEXT LINE) control character changes the	*
 *	active position to line home position on the next line,		*
 *	effectively performing a CR/LF.					*
 ************************************************************************/
VOID pr_nel () 
   {
#ifdef DUMP
   {
    cp_exit_cur_mode();
    oprintf("NEL \n");
   }
#endif
    /*
     * This routine is executed upon detection of the "NEL" (0x85) control char
     * and performs BOTH a carriage return and a linefeed.
     *
     */

    /* is VAI valid, or did a font change invalidate it */
    if (xl_st.vai_valid == FALSE)
    {
	compute_vai();
    }

     /*
     * Update .avp to value of new line, do <FF> if necessary 
     */

    vert_rel_w_wrap(xl_st.vai);

    /* Do CR */
    hpos_abs(xl_st.h_fmt_bound.min);
   }


/*****  pr_pld()  *******************************************************
 *									*
 *	pr_pld() - Move active position up one half line.		*
 ************************************************************************/
VOID pr_pld () 
{
     LONG	avp_save, pld_incr;
     WORD	temp;

#ifdef DUMP
    {
    cp_exit_cur_mode();
    oprintf("PLD \n");
    }
#endif
    cp_exit_cur_mode();

    /* Ignore this PLD if the maximum PLD nesting level has been reached */
    /* or if previous PLD is still in effect that exceeded bottom margin */
    if (xl_st.plf <= (PLDBM + 1))
       {
	return;
       }

    /* Save current .avp */
    avp_save = xl_st.curchar.ap.yval;

    /* If this PLD if the first active PLD or it follows another 
    	currently-effective PLD command, add the value from subscript table 
    	to the .avp.  If this PLD follows a currently-effective PLU command,
    	add the value from superscript table to the .avp. */

    /* Make sure G0 is valid */

    if	(xl_st.g_table[0].gset_valid == FALSE)
	{
	    compute_font_for_g_set (0);
	 }	    

    /* Find out what font is used for G0 */
    temp = xl_st.g_table[0].gset_map[32].char_font;

    if (xl_st.plf-- <= NOPLUPLD)  
       {
    	pld_incr = FNT_SUBSCRIPT_HEIGHT(temp);
       }
    else
       {
    	pld_incr = -FNT_SUPERSCRIPT_HEIGHT(temp);
       }

    /* Update the avp */
    switch (test_against_limits(xl_st.curchar.ap.yval + pld_incr, 
	    &xl_st.v_lim_bound))
       {
        /* If the test for exceeding the bottom margin passed, set the plf flag
         * and manually set the avp 
         */
        case MIN_LIM_EXC:
    	   xl_st.curchar.ap.yval = xl_st.v_lim_bound.min;
	   break;
        case MAX_LIM_EXC:
     	   xl_st.plf = PLDBM;
    	   xl_st.curchar.ap.yval = avp_save + pld_incr;
	   break;
        default:
    	   xl_st.curchar.ap.yval = xl_st.curchar.ap.yval + pld_incr;
       }
}


/*****  pr_plu()  *******************************************************
 *									*
 *	pr_plu() - Move active position up one half line.		*
 ************************************************************************/
VOID pr_plu () 
{
    LONG	avp_save, plu_decr;
    WORD	temp;
#ifdef DUMP
    {    
    cp_exit_cur_mode();
    oprintf("PLU \n");
    }
#endif

    cp_exit_cur_mode();

    /* Ignore this PLU if the maximum PLU nesting level has been reached */
    /* or if the previous PLU is still in effect that exceeded the top margin */
    if (xl_st.plf >= (PLUTM - 1))
       {
	return;
       }

    /* Save current .avp */
    avp_save = xl_st.curchar.ap.yval;

    /* If this PLU is the first active PLU or it follows another 
    	currently-effective PLU command, subtract the value from superscript
    	table from the .avp.  If this PLU follows a currently-effective PLD 
    	command, subtract the value from subscript table from the .avp. */

    /* Make sure G0 is valid */

    if	(xl_st.g_table[0].gset_valid == FALSE)
	{
	    compute_font_for_g_set (0);
	 }	    


    /* Find out what font is used for G0 */
    temp = xl_st.g_table[0].gset_map[32].char_font;
    if (xl_st.plf++ >= NOPLUPLD) 
       {
    	plu_decr = -FNT_SUPERSCRIPT_HEIGHT(temp);
       }
    else
       {
    	plu_decr = FNT_SUBSCRIPT_HEIGHT(temp);
       }

    /* Update the avp */
    switch (test_against_limits(xl_st.curchar.ap.yval - plu_decr, 
    	    &xl_st.v_lim_bound))  
       {
    	/* If the test for exceeding the top margin passed, set the plf flag
         * and manually set the avp 
         */
    	case MIN_LIM_EXC:
    	   xl_st.plf = PLUTM;
    	   xl_st.curchar.ap.yval = avp_save - plu_decr;
	   break;
    	case MAX_LIM_EXC:
    	   xl_st.curchar.ap.yval = xl_st.v_lim_bound.max;
	   break;
    	default:
    	   xl_st.curchar.ap.yval = xl_st.curchar.ap.yval - plu_decr;
       }
}


/*****  pr_ri()  ********************************************************
 *									*
 *	pr_ri() - (REVERSE INDEX) causes the active vertical position	*
 *	to move up one line (horizontal position remains unchanged).	*
 *	unless the new avp would be above the top margin, in which	*
 *	case the RI is ignored.						*
 ************************************************************************/
VOID pr_ri () 
{
#ifdef DUMP
    {
    cp_exit_cur_mode();
    oprintf("RI \n");
    }
#endif

    cp_exit_cur_mode();

    /* If justify mode is on, justify the current contents, and empty it */
    if (xl_st.justify_mode)  
       {
	justify_buffer();
	empty_jfy_buf();

        /* ... interpret RI as CR/RI - LF, so do the CR first */
        hpos_abs(xl_st.h_fmt_bound.min);
       }

    /* is VAI valid, or did a font change invalidate it */
    if (xl_st.vai_valid == FALSE)
    {
	compute_vai();
    }

    /* Update .avp to value of line above (do not go above top margin) */
    vpos_rel(-xl_st.vai);
}


/****************************************************************************
    Cursor UP Utility Function
***************************************************************************/

VOID pr_cuu()

   {
#ifdef DUMP
   {
    oprintf("CUU \n");
    pprint();		    /* print parameter list */   
   }
#endif
    pr_vpb();		    /* is the same as pr_vpb */
   }

