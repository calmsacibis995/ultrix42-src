#ifndef lint
static char SccsId[] = "  @(#)caspac.c	4.1   LPS_ULT_TRN   7/2/90";
#endif

/* file: caspac.c
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
 *  begin edit_history
 *
 *   2-FEB-1988 16:55 mhs
 *      Initial Entry of combination of caspac and duspac
 *
 *  23-JUN-1988 14:57 mhs
 *	Change right margin from 65,536 (too large for a WORD 
 *	param without going negative) to CP_MAX_PARM in faked
 *	call to SLRM in DEC_SHORP.
 *	Add LN04 additional functionality to DEC_SHORP and DEC_VERP.
 *	Add error handling to all spacing routines.
 *  
 *   8-JUL-1988 09:53 mhw
 *	Changes for cp_pbuf from WORD to LONG
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
 *  20-MAR-1989 C. Peters
 *		Changed "gset_map" to "gset_map[0]" for Ultrix port.
 *
 *
 *  12-APR-1989 15:18	araj
 *		unfixed compute_proportional mode, by commenting "if sgr_type != type_family
 *		although if we select by font or by font collection (12/16) proportional 
 *		mode should come from there, the departure is too big to introduce without field
 *		test.
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
 *   Filename:  caspac.c
 *
 *   Spacing/increment routines
 *
 *   This module includes the following routines:
 *
 *   dec_verp()
 *   dec_shorp()
 *   pr_spi()
 *   pr_shs()
 *   pr_svs()
 *
 *   end description
 *
 *-----------------------------------------------------------
 */



/* Translator edit history for this file
 *
 * file:	xlc_spacing.c - Translator spacing/increment routines
 * created:	gh	 26-MAR-1986 09:10:31
 * edit:	gh	 2-APR-1986 15:51:01 Adding spacing functionality
 *		gh	 24-APR-1986 09:19:37 Changed SGR .flag == AF to
 *			 .font_file TRUE in compute_proportional_mode
 *		gh	 7-MAY-1986 17:05:41 Change svs(), spi(), decverp()
 *			 to call compute_vai() and set only xl_st.vsi
 *		gh	 15-MAY-1986 14:55:40 Adding compute_font() call
 *			 after every compute_proportional_mode() call
 *		gh	 27-JUN-1986 17:29:08 Changed DECSHORP to snap .ahp
 *			 to nearest higher grid value of new .hai
 *		nv	 10-JUL-1986 18:50:31 Completely redesigned decshorp(),
 *			 compute_spacing(), compute_proportional_mode(), and 
 *			 spi() to deal with 'semi-proportional' mode.
 *			 Added declaration for average_width_table.
 *		nv	 30-JUL-1986 10:03:59 Changed 'shs' to use 'CPI_xx'
 *			literals.
 *			Changed the way the ps_font_number of GL is accessed
 *			on account of the changed declaration of .gl to .gl_ptr.
 *		gh	 15-AUG-1986 14:52:40 Changed decverp's call to 
 *			scale_vtabs by passing xl_st.vai rather than .vsi
 *		nv	 15-OCT-1986 17:49:59 Excluded, by conditional
 *			compilation with respect to 
 *			'FONT_FILE_MODE_IS_MEANINGFUL', a small part of code in
 *			'compute_proportional_mode'. 
 *		nv	 17-OCT-1986 10:47:53 In 'spi' removed superfluous
 *			call to 'compute_vai'.
 *			Removed all reference to 'hsi'.
 */



#define caspac (1)

/*  begin include_file    */

#include    "portab.h"	    /* For portable definitions of types, etc.	*/
#include    "cpsys.hc"      /* Parser Global defines                    */
#include    "camac.lib_hc"  /* macros for action routines               */
#include    "caglobal.hc"   /* global defs				*/

#ifdef DUMP
#include    "xlc_iface.hc"  /* defines for oprintf			*/
#include    "dumpu.hc"      /* Dump Utility                             */
#endif DUMP

/*  end   include_file    */



/*****  compute_spacing()  **********************************************
 *									*
 *	compute_spacing() - Set .hai and proportional spacing flag	*
 *	depending on received (/default) values of DECPSM and DECSHORP.	*
 ************************************************************************/

VOID compute_spacing (old_hai1, new_hai1)
LONG *old_hai1, *new_hai1;
   {
    LONG new_hai2;



    /*	First, make sure that GL is pointing to a valid GSET, as a change in the
     *	font selection criteria may have invalidate the choice
     */

    if ( xl_st.gl_ptr->gset_valid == FALSE)
	{
	    compute_font_for_g_set ( xl_st.gl_ptr - (&xl_st.g_table[0]));
	}


    *old_hai1 = (xl_st.propl_mode == MONO_SPACED_MODE)
       ? (xl_st.hai)
       : (FNT_AVERAGE_WIDTH(xl_st.gl_ptr -> gset_map[32].char_font));

    /* If PSM is reset, then use the last DECSHORP value received; otherwise,
     * '0' (refer to PSRM Chapter 7 [on coordinate systems] sec 7.8.1 note 4 
     * and deviation 6 and sec 7.8.3 deviation 1).
     */
    switch (
	    (xl_st.psel_mode)
	       ? (0)
	       : (xl_st.shorp)
	   )
       {
        case 0:  xl_st.hai = PROPORTIONAL_PITCH; /* semi-prop spacing = 0 */
		 break;
        case 1:  xl_st.hai = CPI_10;	/* 10 cpi = 720 */
		 break;
	case 2:  xl_st.hai = CPI_12;	/* 12 cpi = 600 */
		 break;
	case 3:  xl_st.hai = CPI_13_2;	/* 13.2 cpi = 545 */
		 break;
	case 4:  xl_st.hai = CPI_16_5;	/* 16.5 cpi = 436 */
		 break;
	case 5:	 xl_st.hai = CPI_5;	/* 5 cpi = 1440 */
		 break;
	case 6:  xl_st.hai = CPI_6;	/* 6 cpi = 1200 */
		 break;
	case 7:  xl_st.hai = CPI_6_6;	/* 6.6 cpi = 1090 */
		 break;
	case 8:  xl_st.hai = CPI_8_25;	/* 8.25 cpi = 872 */
		 break;
	case 9:  xl_st.hai = CPI_15;	/* 15 cpi = 480 */
		 break;
	case 10:
		 xl_st.hai = CPI_12_77;	/* 12.77 cpi = 56 */
		 break;
	case 11:
		 xl_st.hai = CPI_17_1;	/* 17.1 cpi = 421 */
		 break;
	case 12:
		 xl_st.hai = CPI_8_55;	/* 8.55 cpi = 842 */
		 break;
	case 13:
		 xl_st.hai = CPI_18;	/* 18 cpi = 400 */
		 break;
	case 14:
		 xl_st.hai = CPI_9;	/* 9 cpi = 800 */
		 break;
	case 15:
		 xl_st.hai = CPI_10_3;	/* 10.3 cpi = 696 */
		 break;
	default:
		 return;	/* invalid value, so return */
       }

    /*
     * Invalidate the current font selection, as the pitch may have changes
     * as well as the spacing mode.
     */
    invalidate_font();

    /* 
     * Compute the new hai
     *
     */

    if	(xl_st.propl_mode == MONO_SPACED_MODE)
       { 
	    new_hai2 = *new_hai1 = xl_st.hai;
       }
       else
       {
       	    compute_font_for_g_set ( xl_st.gl_ptr - (&xl_st.g_table[0]));
	    new_hai2 = *new_hai1 = FNT_AVERAGE_WIDTH(xl_st.gl_ptr -> 
							gset_map[0].char_font);
       }


    if	(xl_st.propl_mode != PROPORTIONAL_MODE)
	{
           /*
            * 'Snap' current ahp to nearest higher character position under newer
            * hai value.
            */
           update_ahp (((xl_st.curchar.ap.xval + (new_hai2 - 1)) / new_hai2) * new_hai2,
		&xl_st.h_lim_bound);
	}
   }


/*****  compute_proportional_mode()  ************************************
 *									*
 *	compute_proportional_mode() - Xl_st.propl_mode flag is SET if:	*
 *	DECPSP 'set' received, OR					*
 *	SPI with Pn =0 received, OR					*
 *	SGR with Ps selecting a font, OR				*
 *	DECSHORP with Ps =0 receid.					*
 *									*
 *	Xl_st.propl_mode flag is RESET if:				*
 *	DECPSP 'reset' received, AND					*
 *	SPI with Pn <>0 received, AND					*
 *	SGR with Ps selecting a rendition, AND				*
 *	DECSHORP with Ps <>0 received.					*
 ************************************************************************/

VOID compute_proportional_mode()
   {
    /*
     * See if a condition exists warranting proportional mode.
     */
    if (xl_st.psp
/*        || SEE EDIT HISTORY 12-APR-1989 15:37 on WHY
        (xl_st.sgr_tbl [xl_st.cur_sgr].selection_type != SGR_TYPE_FAMILY) 
*/       )
       {
	xl_st.propl_mode = PROPORTIONAL_MODE;
       }
    else
       /*
        * No conditions matched - whether 'hai' is '0' or not determines that 
        * proportional mode is either semi-proportional or mono-spaced 
        * respectively.
        */
       {
	xl_st.propl_mode = (xl_st.hai != 0)
	   ? (MONO_SPACED_MODE)
	   : (SEMI_PROPORTIONAL_MODE);
       }
   }   


/*****  spi()  **********************************************************
 *									*
 *	spi() - Spacing Pitch Increment allows setting the vertical	*
 *	spacing increment (Pv), the horizontal spacing increment (Ph),	*
 *	or both.  If proportional spacing is in effect, spi() is to 	*
 *	have no effect.  The spi parameters are assumed to either 	*
 *	pixels or decipoints, depending upon the last mode set with a	*
 *	SSU command.  If either paramter is omitted or is =0, use the	*
 *	default value of the currently selected font.			*
 ************************************************************************/
VOID pr_spi()
   {
#ifdef DUMP
   {
    oprintf("SPI \n");    
    pprint();		    /* Print the parameter list */
   }
#endif
    /*
     * vsi = Pv * SSU
     */
    xl_st.vsi = cp_pbuf[0] * (LONG)xl_st.sizeunit;

    /*
     * If Ph is =0, set proportional spacing
     * hai = Ph * SSU
     */
    xl_st.hai = cp_pbuf[1] * (LONG)xl_st.sizeunit;

    /*
     * Set/Clear proportional-spacing flag (PSP) 
     * if necesarry
     */
    invalidate_font();
   }


/*****  shs()  **********************************************************
 *									*
 *	shs() - Set Horizontal Spacing.  SHS is used to select the	*
 *	character spacing to be used with mono-width fonts;  it has no	*
 *	effect when proportional spacing is enabled.  SHS has no effect	*
 *	on character size of horizontal tab stops.			*
 *	Allowable values for Pn:					*
 *		0 = 10 cpi - horiz pos unit = 1/10 inch			*
 *		1 = 12 cpi - horiz pos unit = 1/12 inch			*
 *		2 = 15 cpi - horiz pos unit = 1/15 inch			*
 *		3 =  6 cpi - horiz pos unit = 1/6  inch			*
 ************************************************************************/

VOID pr_shs() 
   {
#ifdef DUMP
   {
    oprintf("SHS \n");    
    pprint();		    /* Print the parameter list */
   }
#endif
    /*
     * Set hai 
     */
    switch (cp_pbuf[0])
       {
    	case 0:  xl_st.hai = CPI_10;
		 break;
    	case 1:	 xl_st.hai = CPI_12;
		 break;
    	case 2:	 xl_st.hai = CPI_15;
		 break;
    	case 3:	 xl_st.hai = CPI_6;
		 break;
	default: 
		 return;	    /* invalid parameter, so ignore SHS */
       }

    /*
     * Set/Clear proportional-spacing flag (PSP) 
     * if necesarry
     */
    invalidate_font();
   }


/*****  svs()  **********************************************************
 *									*
 *	svs() - Set Vertical Spacing.  SHS is used to select the	*
 *	line spacing to be used with all fonts.				*
 *	Allowable values for Pn:					*
 *		0 =  6 lpi - vert advance increment = 1/6  inch		*
 *		1 =  4 lpi - vert advance increment = 1/4  inch		*
 *		2 =  3 lpi - vert advance increment = 1/3  inch		*
 *		3 = 12 lpi - vert advance increment = 1/12 inch		*
 *		4 =  8 lpi - vert advance increment = 1/8  inch		*
 *		5 =  6 lines/30 mm.  vert adv incr  = 5 mm		*
 *		6 =  6 lines/30 mm.  vert adv incr  = 7.5 mm		*
 *		7 =  3 lines/30 mm.  vert adv incr  = 10 mm		*
 *		8 = 12 lines/30 mm.  vert adv incr  = 2.5 mm		*
 *		9 =  2 lpi - vert advance increment = 1/2  inch		*
 ************************************************************************/

VOID pr_svs () 
   {
#ifdef DUMP
   {
    oprintf("SVS \n");    
    pprint();		    /* Print the parameter list */
   }
#endif
    /*
     * Set vsi 
     */
    switch (cp_pbuf[0])
       {
    	case 0:  xl_st.vsi = 1200L;
		 break;
    	case 1:	 xl_st.vsi = 1800L;
    		 break;
    	case 2:	 xl_st.vsi = 2400L;
		 break;
    	case 3:	 xl_st.vsi = 600L;
		 break;
    	case 4:	 xl_st.vsi = 900L;
		 break;
	/*
    	 * NOTE: the following four values are calculated from 'rounded'
    	 * pixel values, specifically to be compatible with the LN03 
         */
    	case 5:	 xl_st.vsi = 59L * 24L;
		 break;
    	case 6:	 xl_st.vsi = 89L * 24L;
		 break;
    	case 7:	 xl_st.vsi = 119L * 24L;
		 break;
    	case 8:	 xl_st.vsi = 30L * 24L;
		 break;

    	case 9:	 xl_st.vsi = 3600L;
		 break;
	default: 
		 return;	    /* invalid parameter, so ignore SVS */
       }

    /*
     * Set vai based on vsi
     */
    invalidate_vai();
   }


/*****  decverp()  ******************************************************
 *									*
 *	decverp() - Select vertical pitch.  DECVERP determines the	*
 *	number of lines printed per inch on a page.  Character size is	*
 *	not affected - only the amount of white space between lines.	*
 *	The values are computed by even divisor of 24, so 7200/24 = 300 *
 *	is used as the multiplier for the page height (11 or 10.5)	*
 *	divided by the lines per inch desired.
 *	Allowable values for Pn:					*
 *		0 = Default pitch -- determined by current font		*
 *		1 =  6 lpi						*
 *		2 =  8 lpi						*
 *		3 = 12 lpi						*
 *		4 =  2 lpi						*
 *		5 =  3 lpi						*
 *		6 =  4 lpi						*
 *	       10 =  6.25 lpi						*
 *	       11 =  6.25 lpi						*
 *	       12 =  8.38 lpi						*
 *	       13 = 12.57 lpi						*
 *	       14 =  2.10 lpi						*
 *	       15 =  3.14 lpi						*
 *	       16 =  4.19 lpi						*
 *									*
 *	DECVERP:	does not adjust top & bottom margins		*
 *			does scale entries in vertical tab table	*
 ************************************************************************/

VOID dec_verp () 
   {
    LONG old_vai;

#ifdef DUMP
   {
    oprintf("DECVERP \n");    
    pprint();		    /* Print the parameter list */
   }
#endif
    switch (cp_pbuf[0])
       {
    	case 0:	 xl_st.vsi = 0;		/* let compute_vai determine */
    		 break;
    	case 1:	 xl_st.vsi = LPI_11_66;
		 break;
    	case 2:	 xl_st.vsi = LPI_11_88;
		 break;
    	case 3:	 xl_st.vsi = LPI_11_132;
		 break;
    	case 4:	 xl_st.vsi = LPI_11_22;
		 break;
    	case 5:	 xl_st.vsi = LPI_11_33;
		 break;
    	case 6:	 xl_st.vsi = LPI_11_44;
		 break;
	case 10:
		 xl_st.vsi = LPI_10_5_66;
		 break;
    	case 11:
    		 xl_st.vsi = LPI_10_5_66;
    		 break;
	case 12:
		 xl_st.vsi = LPI_10_5_88;
		 break;
	case 13:
		 xl_st.vsi = LPI_10_5_132;
		 break;
	case 14:
		 xl_st.vsi = LPI_10_5_22;
		 break;
	case 15:
		 xl_st.vsi = LPI_10_5_33;
		 break;
	case 16:
		 xl_st.vsi = LPI_10_5_44;
		 break;
    	default:
    		 return;	    /* invalid parameter, so ignore VERP */
       }

    /*
     * Compute the new effective vai 
     */
    old_vai = xl_st.vai;	/* Save current value for tab table scaling */
    compute_vai();

    /*
     * Scale the entries in the vertical tab table 
     */
    scale_vtabs(old_vai, xl_st.vai);    
   }


/*****  decshorp()  *****************************************************
 *									*
 *	decshorp() - Select horizontal pitch.  DECSHORP determines the	*
 *	character spacing for fixed-width fonts.  It takes effect only	*
 *	when DECSPM mode is reset.  If a DECSHORP is received when	*
 *	DECSPM is set, the DECSHORP parameter must be saved, then when	*
 *	DECSPM is reset, the DECSHORP is effected.			*
 *	Allowable values for Pn:					*
 *		0 = Default pitch -- determined by current font		*
 *		1 = 10 cpi						*
 *		2 = 12 cpi 						*
 *		3 = 13.2 cpi 						*
 *		4 = 16.5 cpi						*
 *		5 =  5 cpi						*
 *		6 =  6 cpi						*
 *		7 =  6.6 cpi						*
 *		8 =  8.25 cpi						*
 *		9 = 15 cpi						*
 *	       10 = 12.77 cpi						*
 *	       11 = 17.1 cpi						*
 *	       12 =  8.55 cpi						*
 *	       13 = 18 cpi						*
 *	       14 =  9 cpi						*
 *	       15 = 10.3 cpi						*
 *									*
 *	DECSHORP:	does not alter the size of the characters	*
 *			does 'clear' the left and right margin		*
 *			does set line home pos equal to the left margin	*
 *			does scale entries in horizontal tab table	*
 *			does affect rmf in same way as DECSLRM		*
 ************************************************************************/

VOID dec_shorp () 
   {
    LONG old_hai, new_hai;

#ifdef DUMP
   {
    oprintf("DECSHORP \n");    
    pprint();		    /* Print the parameter list */
   }
#endif
    /*
     * Store the DECSHORP value just received (if it is valid) 
     */
    if (cp_pbuf[0] > 15)
       {
	return;	    /* Return if unsupported param */
       }

    xl_st.shorp = (WORD)cp_pbuf[0];

    /*
     * Combine the values of DECPSM and DECSHORP to define xl_st.hai.
     */
    compute_spacing (&old_hai, &new_hai);

    /*
     * Set the margins by imitating a DECSRLM 
     */
    cp_pbuf[0] = 1;
    cp_pbuf[1] = 32767;
    dec_slrm();

    /* Scale the horizontal tabs table. (refer to the PSRM Chapter 9 [on tabs]
     * sec 6.2.2 "No other commands [besides DECSHORP and DECVORP] scale the 
     * tabs stop position.")
     */
    scale_htabs (old_hai, new_hai);
}

