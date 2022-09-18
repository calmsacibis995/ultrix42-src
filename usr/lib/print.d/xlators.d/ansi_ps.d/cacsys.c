#ifndef lint
static char SccsId[] = "  @(#)cacsys.c	4.1   LPS_ULT_TRN   7/2/90";
#endif
/* file: cacsys.c
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
 *   begin description
 *
 *   Filename:  cacsys.c
 *
 *   Coordinate system maintenance routines
 *
 *   This module includes
 *
 * pr_pfs ()
 * dec_pfs()
 * dec_vpfs()
 * dec_slpp()
 * dec_slrm ()
 * dec_stbm ()
 * dec_hpwa()
 * dec_asfc ()
 * flen ()
 * setorigin ()
 *
 *   end description
 *
 *-----------------------------------------------------------
 */



/*
 *-----------------------------------------------------------
 *
 *   begin edit_history
 *
 *   001   2-FEB-1988 16:13 mhs
 *      Initial version of combination of ducsys and cacsys
 *
 *   002  13-FEB-1988 14:37 bf
 *	Started real coding.  Changed all references to plist
 *	to use cp_pbuf, also all references to private to use
 *	cp_pflag.  Added include of cpbuf_mgt to get declaration.
 *
 *   003  15-FEB-1988 14:59 mhs
 *      Move DEC_ASFC from catmgt.c, to merge with original
 *      code.  Split PFS routine into separate private and
 *      non-private versions.
 *
 *   004  25-FEB-1988 14:23 mhs
 *      Typecast constants to LONG so that arithmetic computations 
 *      will be compiled correctly.
 *
 *   005  26-FEB-1988 16:31 mhw
 *	add calls to pdli_dispose_set_orient to pfs routines
 *
 *   3-MAR-1988 09:51 mhw
 *	add call to dispose_set_origin in setorigin routine
 *
 *   30-MAR-1988 12:53 mhs
 *      Add split table support for PFS/VPFS.
 *
 *   5-JUL-1988 13:14 mhw
 *	Make the changes necessary for changing cp_pbuf from WORD to LONG
 *
 *   2-NOV-1988 09:08 mhw
 *	Change dec_asfc to call pr_cond_ff() instead of conditional_showpage.
 *	This is necessary to reset the active postion in the ANSI code.
 *
 *  30-NOV-1988 14:28 araj
 *	Install cfont caching, change calls to compute font into calls to invalidate_font
 *	Make sure references to vai or g_table have a test for the validity of the data
 *	first
 *
 *  18-DEC-1988 15:19 ejs
 *		Major changes to data structutes.  GLYPH has been modified
 *		to allow struct copies more easily.  The PS output is not
 *		changed (barring bugs of course).
 *
 *	araj	30-MAR-1989 11:07
 *		Changed the fallback/default values for VPFS to be 1/2" instead of
 *		1/4" inch, as right/bottom margins is counted from the origin, not
 *		from the top of page.
 *		so if page lenght is 11", bottom margin is 11" -(1/4" origin) - (1/4" margin) = 10.5"
 *		not 10.75"
 *
 *	araj	10-APR-1989 14:51
 *		Modified vpfs to allow BM t be closer to page length than 1/4", so 
 *		we can get a VPFS of 10.56" for A size without complaining
 *
 *   end edit_history
 *
 *-----------------------------------------------------------
 */



/* Translator edit history of this file
 *
 * file:	xlc_coordsys.c - Translator coordinate maintainence routines
 * created:	gh	 26-MAR-1986 09:10:31
 * edit:	gh	 31-MAR-1986 10:20:50 Adding pfs(),putting code into
 *			 flen(), slrm(), stbm(), setorigin()
 *		gh	 7-APR-1986 13:59:19 Changed PFS values
 *		gh	 10-APR-1986 11:23:14 Added orientation initialization
 *			 to PFS
 *		gh	 7-MAY-1986 14:14:53 Changing slrm to return if 
 *			 effective rm <= lm from effective rm < lm
 *		gh	 2-JUN-1986 17:15:17 pfs() is being changed from
 *			 SETTING the origin to 0,0 to RESETTING it it 1800,1800
 *		gh	 4-JUN-1986 10:55:57 Adding call to sgr() to select a
 *			 proper font for landscape/portrait mode changes.
 *		gh	 9-JUN-1986 10:44:26 Changing right and bottom margin
 *			 setting (i.e., removing -- from pbuf[0]) when not in
 *			 CHAR mode.
 *		gh	 10-JUN-1986 16:09:37 Adding to pfs() the setting of
 *			 xl_st.shorp=1 if p[ortrait mode, =2 if ladnscape
 *		gh	 11-JUN-1986 15:45:55 Changing sgr(11) call to sgr(10)
 *			 in pfs()
 *		gh	 12-JUN-1986 10:22:13 within pfs(): moving call to sgr
 *			 down after .hai, vsize, etc are calculated.
 *		gh	 13-JUN-1986 08:00:44 Changing the clearing of .plf 
 *		nv	 10-JUL-1986 18:15:16 Cut out most of the final 
 *			 portion of the 'pfs' function.
 *		gh	 21-JUL-1986 12:42:48 Changed 0-based references to 
 *			 1-based references in calls to poscon routines.
 *		gh	 27-AUG-1986 09:19:58 Changed pfs(?21) left and right
 *			 margins to match 528 centipoints/char which greatly
 *			 speeded up printing in landscape.
 *		gh	 29-AUG-1986 08:06:01 Changed pfs(1),(?22),(?23) values
 *			 to m,atch ln03
 *
 *		araj	 9-OCT-1986 11:46:19 changed values of 
 *			 sheet_flen and sheet_width, to always be 
 *			 8.5", 11", 210mm or 297mm regardless of 
 *			 the PFS. These values are used to limit 
 *			 the form length required by DEC_SLPP.
 *			 
 *			 We used to limit them to the printable 
 *			 area defined by the PFS, but QA did not 
 *			 like it (QAR204, and the long battle 
 *			 over it). So now we will limit it to the 
 *			 paper size implied by the PFS.
 *			 
 *			Also, disabled processing of param 0-7
 *			in private sequences.		
 *
 *		araj	 31-OCT-1986 09:29:40 
 *			QA liked the above change in the 
 *			vertical direction, but not in the
 *			horizontal direction.
 *			So we will limit horizontal to 8"
 *			independant of PFS
 *
 *		araj	 3-NOV-1986 14:32:52 
 *			made it 8.25, to include teh origin 
 *			offset
 *              kws      13-MAY-1987 16:35:35 
 *                      Change B size bottom margin from 10.5
 *                      inches to 10.56 inches.
 */



/*  begin include_file    */
#define cacsys (1)

#include    "portab.h"	    /* For portable definitions of types, etc.	*/
#include    "cpsys.hc"      /* Parser Global defines                    */
#include    "caglobal.hc"   /* global defs				*/
#include    "camac.lib_hc"  /* for oprintf				*/

#ifdef DUMP

#include    "xlc_iface.hc"  /* defines for oprintf			*/
#include    "dumpu.hc"      /* Dump Utility                             */

#endif

/*  end   include_file    */




/*****  setorigin()  ****************************************************
 *									*
 *	setorigin(oflag) - Set the origin to 0,0 if oflag == TRUE, or	*
 *	reset the origin to 1800,1800 if oflag = FALSE.			*
 ************************************************************************/

VOID setorigin(oflag)
BOOL oflag;
{
    xl_st.origin.xval = xl_st.origin.yval = (oflag)
       ? (0)
       : (QUARTER_INCH_CTPT);

    process_set_origin(xl_st.origin); /* call composer to set origin */
}



/*****  ca_init_pfs()  **************************************************
 *									*
 *	ca_init_pfs() - PFS initialisation routine			*
 ************************************************************************/

VOID ca_init_pfs(pfs_param)
PPFSBOUNDS pfs_param;	/* PFS param list made ready for ca_init_pfs */

{
    /* Left margin */
	xl_st.h_lim_bound.min = pfs_param->pfs_lrm.min;
    /* Right margin */
	xl_st.h_lim_bound.max = pfs_param->pfs_lrm.max;
    /* Line home pos */
	xl_st.h_fmt_bound.min = pfs_param->pfs_lhe.min;
    /* Line end pos */
	xl_st.h_fmt_bound.max = pfs_param->pfs_lhe.max;
    /* Top margin */
	xl_st.v_lim_bound.min = pfs_param->pfs_tbm.min;
    /* Bottom margin */
	xl_st.v_lim_bound.max = pfs_param->pfs_tbm.max;
    /* Page home line */
	xl_st.v_fmt_bound.min = pfs_param->pfs_phe.min;
    /* Page end line */
	xl_st.v_fmt_bound.max = pfs_param->pfs_phe.max;
    /* Width of form */
	xl_st.sheet_fwid = pfs_param->pfs_pwid;
	xl_st.flen = 
    /* Length of form */
        xl_st.sheet_flen = pfs_param->pfs_plen;
    /* Set orientation */
	xl_st.orientation = pfs_param->pfs_or;

    /*
    ** Since at least one device cannot determine the sheet size, we
    ** pass PFS information to the device so it can guess.
    */
    process_logical_page(pfs_param->pfs_plen,pfs_param->pfs_pwid) ;
}


/*****  do_pfs()  *******************************************************
 *									*
 *	do_pfs() - PFS utility routine					*
 ************************************************************************/

VOID do_pfs(pfs_param)
PPFSBOUNDS pfs_param;	/* PFS param list made ready for do_pfs */

{
    /* Empty justify buffer if there is anything in it */
        empty_jfy_buf();

    /* Set up the PFS tables */
        ca_init_pfs(pfs_param);

    /* Call dispose of routine to set the orientation */
    process_orientation(xl_st.orientation);

    invalidate_font();

    /* Reset the origin to get margins (select the printable page vs. the
     * physical page), and set .ahp and .avp 
     */
    setorigin(FALSE);				/* Reset origin */
    hpos_abs(xl_st.h_fmt_bound.min);		/* ahp = line home pos */
    vpos_abs(xl_st.v_fmt_bound.min);		/* avp = page home pos */
    xl_st.plf = NOPLUPLD;			/* Clear PLU/PLD flag */
}


/*****  pr_pfs()  *******************************************************
 *									*
 *	pfs() - Select a public page format.			        *
 *									*
 *	cp_pflag/cp_pbuf[0]=	Description				*
 *	-----------------	-----------				*
 *	 0			Portrait, normal text format 		*
 *	 2			Portrait, normat A4 format		*
 *	 4			Portrait, North American letter format	*
 *	 6			Portrait, extended A4 format		*
 *       8                      Portrait, extended legal format         *
 *	 1			Landscape, normal text format		*
 *	 3			Landscape, normal A4 format 		*
 *	 5			Landscape, normal North Amer letter	*
 *	 7			Landscape, extended A4 format		*
 *       9                      Landscape, extended legal format         *
 *	(all other paramter values are ignored)				*
 ************************************************************************/

VOID pr_pfs()

{
#ifdef DUMP 
   {
    oprintf("PFS \n");
    pprint();		    /* print the parameter list */    
   }
#endif

    /* Ignore PFS if beyond bounds, otherwise call the do_pfs utility routine */
    if ( (cp_pbuf[0] >= FIRST_PFS) && 
	 (cp_pbuf[0] <= LAST_PFS)
       )
       {
	do_pfs(& (pfstbl[cp_pbuf[0] - FIRST_PFS]));
       }
}



/*****  dec_pfs()  ******************************************************
 *									*
 *	dec_pfs() - Select a private page format.		        *
 *									*
 *	cp_pflag/cp_pbuf[0]=	Description				*
 *	-----------------	-----------				*
 *	?20			Portrait, extended North Amer private	*
 *	?22			Portrait, extended A4 private format	*
 *      ?24                     Portrait, extended legal private        *
 *	?26			Portrait, extended B private format	*
 *	?21			Landscape, extended North Amer private	*
 *	?23			Landscape, extended A4 private		*
 *      ?25                     Landscape, extended legal private       *
 *	?27			Landscape, extended B private		*
 *	(all other parameter values are ignored)			*
 ************************************************************************/

VOID dec_pfs()

{
#ifdef DUMP 
   {
    oprintf("DEC_PFS \n");
    pprint();		    /* print the parameter list */    
   }
#endif
    /* Ignore PFS if beyond bounds, otherwise call the do_pfs utility routine */
    if ( (cp_pbuf[0] >= FIRST_PRIVATE_PFS) && 
	 (cp_pbuf[0] <= LAST_PRIVATE_PFS)
       )
       {
	do_pfs(& (decpfstbl[cp_pbuf[0] - FIRST_PRIVATE_PFS]));
       }
}


/***********************************************************************
    DEC Variable Page Format Select
**********************************************************************/

VOID dec_vpfs()

   {
    PFSBOUNDS vpfs_param;	/* VPFS param list made ready for do_pfs */

#ifdef DUMP
   {
    oprintf("DECVPFS \n");
    pprint();			/* print the parameter list */    
   }
#endif
    /* Note that the order of computation is important, as some parameters are
     * used to clarify other parameters.  The form width and length are highest
     * priority, followed by the top/bottom/left/right margins.  The input 
     * units are determined by SSU and converted to centipoints 
     */

    /* Set orientation */
    vpfs_param.pfs_or = (cp_pbuf[0] != 2)
       ? (ORIENT_PORT)
       : (ORIENT_LAND);

    /* Length of form = P1 * SSU, left unchanged if param is zero or omitted */
    vpfs_param.pfs_plen = (cp_pbuf[1] != 0) 
       ? (cp_pbuf[1] * (LONG)xl_st.sizeunit)
       : (xl_st.sheet_flen);

    /* Width of form = P2 * SSU, left unchanged if param is zero or omitted */
    vpfs_param.pfs_pwid = (cp_pbuf[2] != 0) 
       ? (cp_pbuf[2] * (LONG)xl_st.sizeunit)
       : (xl_st.sheet_fwid);

    /* Left margin = P5 * SSU.
     * Note that missing P5 is interpreted as zero.
     */
    vpfs_param.pfs_lrm.min = (cp_pbuf[5] != 0)
       ? ((cp_pbuf[5] - 1) * (LONG)(xl_st.sizeunit))
       : (0);

    /* Right margin, no greater than 1/4 inch from right side of sheet */
    vpfs_param.pfs_lrm.max = cp_pbuf[6] * (LONG)xl_st.sizeunit;
    if ( (vpfs_param.pfs_lrm.max == 0) || 
         (vpfs_param.pfs_lrm.max > vpfs_param.pfs_pwid)
       )
       {
	vpfs_param.pfs_lrm.max = vpfs_param.pfs_pwid - HALF_INCH_CTPT;
       }

    /* Top margin = P3 * SSU.
     * Note that missing P3 is interpreted as zero.
     */
    vpfs_param.pfs_tbm.min = (cp_pbuf[3] != 0)
       ? ((cp_pbuf[3] - 1) * (LONG)(xl_st.sizeunit))
       : (0);

    /* Bottom margin, no greater than 1/4 inch from bottom of sheet */
    vpfs_param.pfs_tbm.max = cp_pbuf[4] * (LONG)xl_st.sizeunit;
    if ( (vpfs_param.pfs_tbm.max == 0) || 
	 (vpfs_param.pfs_tbm.max > vpfs_param.pfs_plen)
       )
       {
	vpfs_param.pfs_tbm.max = vpfs_param.pfs_plen - HALF_INCH_CTPT;
       }

    /* Line home pos = P9 * SSU, left unchanged if param is zero or omitted */
    vpfs_param.pfs_lhe.min = (cp_pbuf[9] != 0)
       ? ((cp_pbuf[9] - 1) * (LONG)(xl_st.sizeunit))
       : (vpfs_param.pfs_lrm.min);

    /* Line end pos, no further right than the right margin */
    vpfs_param.pfs_lhe.max = cp_pbuf[10] * (LONG)xl_st.sizeunit;
    if ( (vpfs_param.pfs_lhe.max == 0) || 
	 (vpfs_param.pfs_lhe.max > vpfs_param.pfs_lrm.max)
       )
       {
	vpfs_param.pfs_lhe.max = vpfs_param.pfs_lrm.max;
       }

    /* Page home pos = P7 * SSU, left unchanged if param is zero or omitted */
    vpfs_param.pfs_phe.min = (cp_pbuf[7] != 0)
       ? ((cp_pbuf[7] - 1) * (LONG)(xl_st.sizeunit))
       : (vpfs_param.pfs_tbm.min);

    /* Page end line, no further down than the bottom margin */
    vpfs_param.pfs_phe.max = cp_pbuf[8] * (LONG)xl_st.sizeunit;
    if ( (vpfs_param.pfs_phe.max == 0) || 
         (vpfs_param.pfs_phe.max > vpfs_param.pfs_tbm.max)
       )
       {
        vpfs_param.pfs_phe.max = vpfs_param.pfs_tbm.max;
       }

    /* If margins or bounds are crossed, then ignore sequence and exit,
     * otherwise call do_pfs to move the new margins and bounds into
     * the global pfs bounds table
     */
    if ( (vpfs_param.pfs_lrm.max > vpfs_param.pfs_lrm.min) &&
         (vpfs_param.pfs_tbm.max > vpfs_param.pfs_tbm.min) &&
         (vpfs_param.pfs_lhe.max > vpfs_param.pfs_lhe.min) &&
         (vpfs_param.pfs_phe.max > vpfs_param.pfs_phe.min)
       )
       {
	do_pfs(& vpfs_param);
       }
   }


/*****  dec_slpp ()  ****************************************************
 *									*
 *	dec_slpp () - set form length (DECSLPP)				*
 * 	The max value flen can be set to is 'sheet_flen - origin'	*
 ************************************************************************/

VOID dec_slpp()

   {

#ifdef DUMP
    {
    oprintf("DECSLPP \n");
    pprint();		    /* print the parameter list */    
    }
#endif

    /* If the parameter is = 0, set flen to max */
    xl_st.flen = (cp_pbuf[0] != 0)
       ? (vdist(cp_pbuf[0]))
       : (xl_st.sheet_flen);

    /* flen can be no larger than sheet_flen - current origin */
    if (xl_st.flen > (xl_st.sheet_flen - xl_st.origin.yval))
       {
        xl_st.flen = xl_st.sheet_flen - xl_st.origin.yval;
       }
    		
    /* TM = PH = 0.    BM = PE = flen */
    xl_st.v_lim_bound.min = xl_st.v_fmt_bound.min = 0;
    xl_st.v_lim_bound.max = xl_st.v_fmt_bound.max = xl_st.flen;
   }



/*****  dec_slrm()  *****************************************************
 *									*
 *	dec_slrm() - Set the left and right margins 			*
 ************************************************************************/

VOID dec_slrm()

   {
    LONG temp_line_home, temp_line_end;

#ifdef DUMP
   {
    oprintf("DECSLRM \n");
    pprint();		    /* print the parameter list */    
   }
#endif

    /* If the left margin is <> 0, set temp line home pos = new value */
    temp_line_home = (cp_pbuf[0] != 0) 
       ? (hdist((cp_pbuf[0] - 1)))
       : (xl_st.h_fmt_bound.min);

    /* If the right margin is <> 0, set temp line end pos = new value
     * (if new line end position would be beyond the right printable limit,
     *  set it to the right printable limit 
     */
    temp_line_end = (cp_pbuf[1] != 0)  
       ? (hdist(cp_pbuf[1]))
       : (xl_st.h_fmt_bound.max);
    if (temp_line_end > (xl_st.sheet_fwid - xl_st.origin.xval))
       {
	temp_line_end = xl_st.sheet_fwid - xl_st.origin.xval;
       }

    /* Return without changing anything if the new right margin will 
     * be <= the new left margin 
     */
    if (temp_line_end <= temp_line_home)
       {
        return;
       }

    /* All is ok, empty the justify buf, & set the new left and right margins */

    empty_jfy_buf();
    xl_st.h_fmt_bound.min = xl_st.h_lim_bound.min = temp_line_home;
    xl_st.h_fmt_bound.max = xl_st.h_lim_bound.max = temp_line_end;

    /* If the ahp is < the new left margin, move the ahp */

    if (xl_st.curchar.ap.xval < xl_st.h_fmt_bound.min)
       {
    	hpos_abs(xl_st.h_fmt_bound.min);
       }

    /* If the new right margin is > the ahp, clear the right margin flag */

    if (xl_st.h_fmt_bound.max > xl_st.curchar.ap.xval)
       {
        xl_st.rmf = FALSE;
       }
    else
       {
	xl_st.rmf = TRUE;
    	update_ahp(xl_st.h_fmt_bound.max,&xl_st.h_lim_bound);
       }
   }



/*****  dec_stbm()  *****************************************************
 *									*
 *	dec_stbm() - Set the top and bottom margins.			*
 ************************************************************************/

VOID dec_stbm()

   {
    LONG temp_page_home, temp_page_end;

#ifdef DUMP
   {
    oprintf("DECSTBM \n");
    pprint();		    /* print the parameter list */    
   }
#endif

    /* If the top margin is <>0, set temp page home pos = new value */
    temp_page_home = (cp_pbuf[0] != 0) 
       ? (vdist((cp_pbuf[0] - 1)))
       : (xl_st.v_fmt_bound.min);

    /* If the bottom margin is <>0, set temp page end pos = new value
     * (if new page end position would be beyond the bottom printable limit,
     *  set it to the bottom printable limit 
     */
    temp_page_end = (cp_pbuf[1] != 0)  
       ? (vdist(cp_pbuf[1]))
       : (xl_st.v_fmt_bound.max);
    if (temp_page_end > (xl_st.sheet_flen - xl_st.origin.yval))
       {
    	temp_page_end = xl_st.sheet_flen - xl_st.origin.yval;
       }

    /* Return without changing anything if the new bottom margin will 
     * be <= the new top margin 
     */
    if (temp_page_end <= temp_page_home)
       {
    	return;
       }

    /* All is ok, set the new top and bottom margins */

    xl_st.v_fmt_bound.min = xl_st.v_lim_bound.min = temp_page_home;
    xl_st.v_fmt_bound.max = xl_st.v_lim_bound.max = temp_page_end;
    xl_st.plf = NOPLUPLD;

    /* If the avp is < the new top margin, move the avp */

    if (xl_st.curchar.ap.yval < xl_st.v_fmt_bound.min)  
       {
	vpos_abs(xl_st.v_fmt_bound.min);
	xl_st.fcf = TRUE;
       }
   }



/*****  dec_asfc()  *****************************************************
 *									*
 *	dec_asfc() - Automatic sheet feeder control.  Handles the       *
 *      DEC_ASFC control sequence to select specified paper trays.      *
 ************************************************************************/

VOID dec_asfc()

{
#ifdef DUMP
    {
    oprintf("DECASFC \n");    
    pprint();		    /* print the parameter list */    
    }
#endif

    /* Force a conditional form feed to eject the current page.	*/

    pr_cond_ff();

    /* If the parameter is zero, or the same as the current tray
       setting, return without setting the paper tray. */

    if (cp_pbuf [0] == 0 || cp_pbuf [0] == xl_st.paper_tray)
    	return;

    /* Get the tray number from the parameter list	*/

    xl_st.paper_tray = (WORD)cp_pbuf [0];

    /* Output the setpapertray command. */

    process_tray_select (xl_st.paper_tray);

}


/***********************************************************************
    Horizontal Page Width Alignment
**********************************************************************/

VOID dec_hpwa()

    {

#ifdef DUMP
    {
    oprintf("DECHPWA \n");
    pprint();		    /* print the parameter list */    
    }
#endif

    return;
    }
