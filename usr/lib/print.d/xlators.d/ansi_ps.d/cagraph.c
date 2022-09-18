#ifndef lint
static char SccsId[] = "  @(#)cagraph.c	4.1   LPS_ULT_TRN   7/2/90";
#endif

/* file: cagraph.c
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
 *   2-FEB-1988 16:45 mhs
 *      Initial Entry of combination of cagraph and dugraph.
 *
 *  15-FEB-1988 10:56 mhw
 *      Add include of camac.lib for non-dump oprintf.
 *
 *  13-APR-1988 12:16 mhs
 *      Move state pointer set operations to new "ac_"
 *      routines in castate module.
 *
 *   1-JUN-1988 15:10 mhs/araj
 *	Add support for new get_sixel_macro routine, and fix
 *	numerous bugs with sixel initialisation.
 *
 *  29-JUN-1988 15:21 mhs
 *	Add ca_graphics_init routine so that graphics vars in
 *	xl_st can be initialised on powerup.
 *
 *   3-OCT-1988 13:15 mhw
 *	Added code to dec_gri to test for the maximum allowable repeat count.
 *
 *  21-NOV-1988 09:10 mhw
 *	Added call to empty_jfy_buf to dec_gr_enter routine because the
 *	justify buffer should be emptied when graphics mode is entered.
 *	
 *
 *  18-DEC-1988 15:19 ejs
 *		Major changes to data structutes.  GLYPH has been modified
 *		to allow struct copies more easily.  The PS output is not
 *		changed (barring bugs of course).
 *
 *   4-APR-1989 10:37 ejs
 *		Added ca_zap_repeat.   Used to reset the repeat count if a
 *		command should follow the DEC_GRI instead of a sixel.
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
 *   Filename:  cagraph.c
 *
 *   SIXELS TO HEX conversion program
 *
 *   This program will convert a sixel file into a hex file.  
 *   There will be no attempts to parse the ANSI command 
 *   sequence. In fact, only the sixel data after the q will 
 *   be dealt with in the program.  Aspect Ratio, Color and
 *   any other parameters will be ignored.
 *
 *   Only one  sixel row at a time will be dealt with and 
 *   then written to a file before going on to the next 
 *   sixel row.
 *
 *   This module includes the following routines:
 *
 *   ca_graphics_init()
 *   dec_gri()
 *   ca_zap_repeat
 *   dec_gra()
 *   dec_gci()
 *   dec_gcr()
 *   dec_gnl()
 *   dec_gr_sxl()
 *
 *   end description
 *
 *-----------------------------------------------------------
 */



/* Translator edit history for this file
 *
 * REVISION HISTORY 
 *
 * Rev 0		10-FEB-86	Michael Blumenreich
 * Original	
 *
 * Rev 0.1		25-MAR-86	Michael Blumenreich
 * Added transportability to storage buffers and variables 
 *
 * 15-JUL-1986 13:08:57 		laf
 * Added check for a repeat count of zero to "gr_repeat" (QAR 509)
 * 28-AUG-1986 21:09:18 		Araj
 * Removed "-1" in computation of Byte count in Exit_Graphic.
 * This "-1" caused us to loose the last sixel of the last line, if
 * the sixel byte count was "1" modulo 8, and there was no 
 * overprinting, or the overprinting was bigger than the 
 * overprinted area.
 * For instance ESC P q ~ ESC \ would not print
 * nor would    ESC P q !3~ $ !9~ ESC \,
 * in the first case byte count was 1, in the seconf the 
 * overprinting was 1 modulo 8, and larger than the overprinted 
 * area.
 * On the other end, ESC P q ~~ ESC \ would print
 * so would          ESC P q !17~ $ !9~ ESC \,
 * in the first case, byte count was 2 modulo 8, in the second, 
 * byte count was 1 modulo 8, so was old byte count, but old byte 
 * count was > than byte count (overprinting smaller than 
 * overprinted).
 *
 * 29-AUG-1986 13:30:26		nv
 * Changed the evaluation of v_g to be as follows:
 *	v_g = (6.0 * (hor_grid) * aspect_ratio.xval) / aspect_ratio.yval;
 * Changed the definition of 'hor_grid' and 'max_buff_size' from 'double'
 * to 'LONG'.
 *
 * 26-SEP-1986 20:14:31 	araj
 *				retyped Mikes changes for improved performance
 *				aspect ratio
 * 30-SEP-1986 10:15:07 	mgb
 *				added default setting to Hor. Grid when input
 *				parameter is greater than 9.
 * 2-OCT-1986 17:48:45 		mgb
 *				moved check_ap after defining v_g and h_g
 *
 * 8-OCT-1986 18:12:59 		araj
 *				Modified GR_CR to invalidate the 
 *				raster flag, and modified GR_AR
 *				to check the raster_flag in 
 *				conjunction with byte_cnt and
 *				old_byte_cnt, to ensure that
 *				no GR_NL, or Sixel, was received 
 *				before the AR command.
 *				Ensuring that no GR_NL had been 
 *				received did not match QA 
 *				requirements.
 *
 * 10-DEC-1986 11:41:35 	mgb
 *				Adding parameters to call to check_ap
 *
 * 29-APR-1987 10:58:21 	mgb
 *				Changed call to vpos_rel() to 
 *				vpos_rel_w_wrap() because vpos_rel() uses
 *				the wrong bottom limit.
 */



#define cagraph (1)

/*  begin include_file    */

#include    "portab.h"	    /* general portability constants */
#include    "cpsys.hc"	    /* general defs, typedefs, etc. */
#include    "cpast.hc"	    /* ANSI State Table & Token Table Declarations */
#include    "cpglobal.hc"   /* global defs for parser */
#include    "caglobal.hc"   /* global defs for action routines */
#include    "camac.lib_hc"  /* non-dump version of oprintf */

#ifdef DUMP
#include    "dumpu.hc"      /* Dump Utility                             */
#include    "xlc_iface.hc"  /* defines for oprintf			*/
#endif DUMP

/*  end   include_file    */


/*
 * Initialise the Graphics State
 */

VOID ca_graphics_init()
   {
    xl_st.cur_sxl.sixel_code = 0;
    xl_st.cur_sxl.sixel_color = 0;
    xl_st.cur_sxl.sixel_repeat = 1;
    xl_st.cur_sxl.sixel_ap.xval = 0;
    xl_st.cur_sxl.sixel_ap.yval = 0;
    xl_st.cur_sxl.sixel_size.xval = 200;
    xl_st.cur_sxl.sixel_size.yval = 100;
    xl_st.hor_grid = 200;
    xl_st.aspect_ratio.xval = 200;
    xl_st.aspect_ratio.yval = 100;
   }


/**********************************************************************
 *
 * This routine is called when 
 *
 *       <esc> P P1 ; P2 ; P3 ... q  
 *
 * has been parsed by the parser. The Aspect Ratio and Grid size will
 * be selected from P1 and P3. P1 will define the Aspect Ratio and Grid
 * size and if P3 is defined it will redefine the grid using the SSU.
 * 
 * All initialization will be done here.
 *
 *	Some Ratios cannot lead to integer # of pixels in both H and V
 *	so an approximation of the AR is now used to make sure we get 
 *	an exact Vertical grid
 *	For instance, parameter 2 is 20.x H 96 V, AR =4.5/1
 *
 *	But rounding 20.x to 21 in H and using 4.5/1 gives us
 *	a non integer V
 *
 *	So we'll replace 4.5/1 by 96/21, so V will be exact 
 *	and H won't be too bad
 *	
 **********************************************************************/

VOID dec_gr_enter()

   {
#ifdef DUMP
   {
    oprintf("DECGRAPHICS \n");   
    pprint();
   }
#endif
    /* First empty the justify buffer */

    empty_jfy_buf();


    /*
     * Save current ahp to use as left margin on carriage return and to 
     * restore the original position when exiting graphics mode
     */
    gr_left_bound = MAX (xl_st.h_fmt_bound.min, xl_st.curchar.ap.xval);

    /*
     * Initialise the sixel active position using the most recent
     * position from previous sixels or text processing - note that
     * our x value must be our graphic left bound instead
     */
    xl_st.cur_sxl.sixel_ap.xval = gr_left_bound;
    xl_st.cur_sxl.sixel_ap.yval = xl_st.curchar.ap.yval;

    /*
     * Compute the horizontal and vertical grids and the aspect ratio
     */
    if (cp_pbuf[0] > MAX_SXL_PARM)
       {
	cp_pbuf[0] = 0;	    /* if too large a value supplied, use default */
       }
    PDLI_GET_SIXEL_MACRO_GRID((WORD)cp_pbuf[0], &xl_st.aspect_ratio, 
			      &xl_st.cur_sxl.sixel_size);

    /*
     * Check to see if the user is overwriting the P1 value with a P3
     * value for the horizontal grid/etc.
     */
    if ( (cp_pcnt > 1) && (cp_pbuf[2] > 0) )
       {
	/* xl_st.cur_sxl_sixel.size.xval = P3 * SSU */
	xl_st.cur_sxl.sixel_size.xval = cp_pbuf[2] * (LONG)xl_st.sizeunit;
       }

    /*
     * Compute the final values for the horizontal and physical
     * grids (note that the horizontal grid is just passed through
     * as is, which is a bit redundant)
     */
    PDLI_GET_PHYSICAL_GRID(xl_st.cur_sxl.sixel_size.xval, &xl_st.aspect_ratio, 
			   &xl_st.cur_sxl.sixel_size);

    /*
     * Reset the repeat count for just one sixel
     */
    xl_st.cur_sxl.sixel_repeat = 1;
   }


/**********************************************************************
 *
 * This routine is called when a graphic sixel is encountered.
 *
 * The end results of this routine is to create hex values of
 * the sixel data. After every 8 sixels, 6 hex bytes of data
 * will be produced. The 6 hex bytes will be in the same column
 * (6 current rows) of the output file.
 *
 * The text-oriented check_ap routine is called to make sure
 * that we wrap at the end of a page, and that we handle the
 * first character flag properly.  It also helps us to make sure
 * that sixels, no matter what their size or shape, are aligned
 * at the top when they are side by side, by passing 700 as the
 * above baseline value and subtracting that number from the
 * current sixel size to compute the below baseline value.
 *
 **********************************************************************/
VOID dec_gr_sxl()

   {
#ifdef DUMP
   {
    oprintf("DECGRAPHICS SIXEL \n");   
   }
#endif
    /*
     * If the previous sixel pushed us past the right margin, 
     * or so close we can't even fit one sixel, then ignore
     * the current sixel and all sixels that follow until a graphic carriage
     * return or newline is encountered (done by not updating the ahp, and
     * by doing nothing more than resetting the repeat count).
     *
     * This is meant to overwrite what check_ap does with horz autowrap.
     */
    if ((xl_st.cur_sxl.sixel_ap.xval + xl_st.cur_sxl.sixel_size.xval )> xl_st.h_lim_bound.max)
       {
	xl_st.cur_sxl.sixel_repeat = 1; /* reset repeat count for one sixel */
	return;			/* ignore the current sixel */
       }


    /* if the sixel is too long for the line, truncate it */

    if (xl_st.cur_sxl.sixel_ap.xval + (xl_st.cur_sxl.sixel_size.xval * (LONG)xl_st.cur_sxl.sixel_repeat )
	> xl_st.h_lim_bound.max)
	{   xl_st.cur_sxl.sixel_repeat = ( xl_st.h_lim_bound.max - xl_st.cur_sxl.sixel_ap.xval )
					  / xl_st.cur_sxl.sixel_size.xval ;
	}

    /*
     * Update the ahp based on the ending position of the previous sixel
     */
/*    xl_st.curchar.ahp = xl_st.cur_sxl.sixel_ap.xval;
*/
    /*
     * Wrap the current sixel position to the next page, if necessary, and
     * handle the first character flag accordingly.  Update the current sixel
     * position based on the results of check_ap.  Since an ahp too large is
     * covered by the previous conditional exit, and we should never get an
     * ahp too small while in sixels, the setting of the current sixel xval
     * is probably redundant.
     */
    check_ap((LONG)(700L - 1), (LONG)(xl_st.cur_sxl.sixel_size.yval - 700L));
/*    xl_st.cur_sxl.sixel_ap.xval = xl_st.curchar.ahp;
*/
    xl_st.cur_sxl.sixel_ap.yval = xl_st.curchar.ap.yval;

    /*
     * Subtract the first sixel offset to convert the incoming code into
     * a true sixel code, and image it.  The conversion of repeat count of
     * zero to repeat count of one is probably redundant.
     */
    xl_st.cur_sxl.sixel_code = (cp_c7 - FIRST_SIXEL_CODE);

    process_sixel(&xl_st.cur_sxl);

    /*
     * Update the current sixel position based on whether or not
     * the current sixel was repeated or not.
     */
    xl_st.cur_sxl.sixel_ap.xval += (xl_st.cur_sxl.sixel_size.xval * (LONG)xl_st.cur_sxl.sixel_repeat );

    /*
     * Reset the repeat count for just one sixel
     */
    xl_st.cur_sxl.sixel_repeat = 1;
   }

 
/**********************************************************************
 *
 * Graphic new line found - output current hex data
 *
 **********************************************************************/
VOID dec_gnl()

   {
#ifdef DUMP
   {
    oprintf("DECGNL \n");    
   }
#endif
    vpos_rel_w_wrap(xl_st.cur_sxl.sixel_size.yval);
    xl_st.cur_sxl.sixel_ap.xval = gr_left_bound;
    xl_st.cur_sxl.sixel_ap.yval = xl_st.curchar.ap.yval;
   }


/**********************************************************************
 *
 * Set repeat_cnt to the incoming value
 *
 **********************************************************************/
VOID dec_gri()

   {
#ifdef DUMP
   {
    oprintf("DECGRI \n");    
    pprint();
   }
#endif
    if (cp_pbuf[0] > MAX_RPT_CNT)
	xl_st.cur_sxl.sixel_repeat = MAX_RPT_CNT;
    else 
	xl_st.cur_sxl.sixel_repeat = (cp_pbuf[0] != 0)
           ? (cp_pbuf[0])
           : (1);
   }


/**********************************************************************
 *
 * Set repeat_cnt to the incoming value
 *
 **********************************************************************/
VOID ca_zap_repeat()

   {
    	xl_st.cur_sxl.sixel_repeat = 1;
   }


/**********************************************************************
 *
 * Enter here if a graphic carrage return ($) has been encountered.
 *
 * This routine will reset the byte count back to zero so that the next
 * line will be ORed over the preceding line.
 *
 **********************************************************************/

VOID dec_gcr()

   {
#ifdef DUMP
   {
    oprintf("DECGCR \n");    
   }
#endif
    xl_st.cur_sxl.sixel_ap.xval = gr_left_bound;
   }


/**********************************************************************
 *
 * This will be called when the sixel set raster command (") is 
 * encountered. 
 *
 *        " P1 ; P2 ; P3 ; P4 ; P5 ; ... ; Pn
 * 
 *          P1 : Aspect Ratio Numerator
 *          P2 : Aspect Ratio Denominator
 *          P3 : Horizontal Extent
 *          P4 : Verticle Extent
 *          P5 - Pn : Ignored
 *
 * This will override any Aspect Ratio and Grid size that
 * has been set so long that there has not been any sixel data or line 
 * feeds encountered. 
 *
 * If P1 and/or P2 are zero, they will default to one.
 *
 * Horizontal and Verticle Grid sizes will be dependent upon the SSU.
 *
 **********************************************************************/

VOID dec_gra()

   {
#ifdef DUMP
   {
    oprintf("DECGRA \n");    
    pprint();    
   }
#endif
    xl_st.aspect_ratio.xval = (cp_pbuf[0] != 0)	    /* numerator */
       ? (cp_pbuf[0])
       : (1);
    xl_st.aspect_ratio.yval = (cp_pbuf[1] != 0)	    /* denominator */
       ? (cp_pbuf[1])
       : (1);
    PDLI_GET_PHYSICAL_GRID(xl_st.cur_sxl.sixel_size.xval, &xl_st.aspect_ratio, 
			   &xl_st.cur_sxl.sixel_size);
   }


/**************************************************************************
    Color Introducer - 
*************************************************************************/

VOID dec_gci()

   {
    COLOR cur_color;	/* current color map */

#ifdef DUMP
   {
    oprintf("DECGCI \n");    
    pprint();
   }
#endif

    if (cp_pcnt == 0)
       {
	xl_st.cur_sxl.sixel_color = (UBYTE)cp_pbuf[0];
       }
    else
       {
	 switch (cp_pbuf[1])
	    {
	     case 0:
		      return; /* invalid param, so exit */
	     case 1:
		      if ( (cp_pbuf[2] > 360) ||
			   (cp_pbuf[3] > 100) ||
			   (cp_pbuf[4] > 100)
			 )
			 {
			  return; /* invalid param, so exit */
			 }
		      xl_st.cur_sxl.sixel_color = 
			 cur_color.color_map_entry = cp_pbuf[0];
		      cur_color.color_defn.coordsys = cp_pbuf[1];
		      cur_color.color_defn.hue_angle = cp_pbuf[2];
		      cur_color.color_defn.lightness = cp_pbuf[3];
		      cur_color.color_defn.saturation = cp_pbuf[4];
		      process_color(&cur_color);
		      break;
	     case 2:
		      if ( (cp_pbuf[2] > 100) ||
			   (cp_pbuf[3] > 100) ||
			   (cp_pbuf[4] > 100)
			 )
			 {
			  return; /* invalid param, so exit */
			 }
		      xl_st.cur_sxl.sixel_color = 
			 cur_color.color_map_entry = cp_pbuf[0];
		      cur_color.color_defn.coordsys = cp_pbuf[1];
		      cur_color.color_defn.hue_angle = cp_pbuf[2];
		      cur_color.color_defn.lightness = cp_pbuf[3];
		      cur_color.color_defn.saturation = cp_pbuf[4];
		      process_color(&cur_color);
		      break;
	     default:
		      return; /* invalid param, so exit */
	    }
       }
   }


/*
 * Exit Graphics Mode (same as Graphics CR for now)
 */

VOID dec_gr_term()
   {
    xl_st.curchar.ap.xval = gr_left_bound;
   }

