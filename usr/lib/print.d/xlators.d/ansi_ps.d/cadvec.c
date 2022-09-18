#ifndef lint
static char SccsId[] = "  @(#)cadvec.c	4.1   LPS_ULT_TRN   7/2/90";
#endif

/* file: cadvec.c
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
 *   2-FEB-1988 16:40 mhs
 *      Initial Entry of combination of cadvec and dudvec
 *
 *  21-MAR-1988 12:21 araj
 *	changed ahp/avp of DECVEC to be a point, to solve warnuings in CAPDL.def
 *
 *  21-MAR-1988 20:31 araj
 *	Removed counter centering of DECVEC.
 *	Postscript centers the vector around the starting point.
 *	The LN03 does not.
 *	The tanslator counter-center the DECVEC, to compensate for
 *	the PS effect. This does not work for the LN04.
 *	The CAR cod is being modified to not do the counter-centering, 
 *	the transltor codegen will have to be modified to do the counter-centering
 *
 *   2-MAY-1988 15:49 mhs
 *	Implement DECRVEC.
 *
 *   7-JUL-1988 15:43 mhw
 *	Change cp_pbuf implementation from WORD to LONG
 *
 *  21-OCT-1988 16:18 mhw
 *	Change dec_vec such that length and thickness are inverted if
 *	drawing in the y vs. the x direction.  This is done such that the
 *	centering technique used by Postscript in the Translator will 
 *	work correctly.  This causes the CPAR to always send information
 *	to the device as if the request were to draw in the x direction
 *
 *  25-OCT-1988 09:40 mhw
 *	Made the above changes to decrvec also.  All decrvec cases are now
 *	converted such that the device receives information to draw a
 *	horizontal line to the right.
 *	
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
 *   Filename:  cadvec.c
 *
 *   Routines that handle DECVEC.
 *
 *   This module includes the following routines:
 *
 *   dec_vec()
 *   dec_rvec()
 *   
 *
 *   end description
 *
 *-----------------------------------------------------------
 */


/* Translator edit history for this file
 *
 * file:	xlc_decvec.c - Translator decvec routines
 * created:	gh	 26-MAR-1986 09:10:31
 * edit:	gh	 24-APR-1986 13:10:54 Adding functionality to decvec()
 *		gh	 30-APR-1986 08:57:54 decvec() using wrong plist[] value
 *			 in caluclating line thickness. -fixed.
 *		gh	 22-JUL-1986 11:10:23 Changed DECVEC X and Y starting
 *			 position parameters relative to 1
 *		nv	 27-AUG-1986 18:07:16 Changed decvec to compensate for 
 *			the fact that Postscript draws lines about their center
 *			and reformatted the procedure.
 */



/*  begin include_file    */

/*  This is added so we can see 'static's  */
#define cadvec	(1)


#include    "portab.h"	    /* For portable definitions of types, etc.	*/
#include    "cpsys.hc"      /* Parser Global Definitions                */
#include    "caglobal.hc"   /* global defs				*/
#include    "camac.lib_hc"  /* Oprintf					*/

#ifdef DUMP
#include    "xlc_iface.hc"  /* defines for oprintf			*/
#include    "dumpu.hc"      /* Dump Utility                             */
#endif

/*  end   include_file    */


/*****  dec_vec()  ******************************************************
 *									*
 *	dec_vec() - Draw a horizontal or vertical line as specified in	*
 *	the DECVEC parameters.  Format of command:			*
 *	<CSI> P1 ; P2 ; P3 ; P4 ; P5 ! |				*
 *	P1 =	0 to draw a horizontal line				*
 *		1 to draw a vertical line				*
 *		>1 - ignore DECVEC command				*
 *	P2 =	the 'x' starting position. 				*
 *	P3 =	the 'y' starting position. 				*
 *	P4 =	the line length. If < 1 pixel, draw 1 pixel length line	*
 *	P5 =	the line width. If < 1 pixel, draw 1 pixel width line	*
 *      The upper left corner is the starting point of the DECVEC.      *
 ************************************************************************/
VOID dec_vec()

   {
    LONG    length;
    LONG    width;
    LONG    temp;
    DECVEC  dvec;		/* Used to form DECVEC command */

#ifdef DUMP
   {
    oprintf("DECVEC \n");    
    pprint();    
   }
#endif

    /* If P1 <> 0 or 1, ignore the command */
    if (cp_pbuf[0] > 1)
       {
        return;
       }

    /* Starting X position = P2 * SSU.
     * Note that missing P2 is interpreted as zero.
     */
    dvec.dv_ap.xval = (cp_pbuf[1] != 0) 
       ? ((cp_pbuf[1] - 1) * (LONG)(xl_st.sizeunit))
       : (0);

    /* Starting Y position = P3 * SSU.
     * Note that missing P3 is interpreted as zero.
     */
    dvec.dv_ap.yval = (cp_pbuf[2] != 0) 
       ? ((cp_pbuf[2] - 1) * (LONG)(xl_st.sizeunit))
       : (0);

    /* If the effective line width (P5 * SSU) is less than 1 pixel,
     * make it one pixel.
     */
     if ( (width = dvec.thickness = (cp_pbuf[4] * (LONG)xl_st.sizeunit))
         < CENTIPTS_PER_PIXEL )
       {
        width = dvec.thickness = CENTIPTS_PER_PIXEL;
       }

    /* If the effective line length (P4 * SSU) is less than 1 pixel,
     * make it one pixel and calculate the effective endpoint.
     */
    if ( (length = (cp_pbuf[3] * (LONG)xl_st.sizeunit)) 
         < CENTIPTS_PER_PIXEL )
       {
        length = CENTIPTS_PER_PIXEL;
       }

    switch (cp_pbuf[0])
       {
        case 0:	    /* If horizontal line, ... */
	        break;
        case 1:	    /* If vertical line, ... */
		temp = length;
		length = width;
		width = dvec.thickness = temp;
		break;
       }


    dvec.dvend.xval = dvec.dv_ap.xval + length;
    dvec.dvend.yval = dvec.dv_ap.yval;

    /* Go draw the vector
     */
    process_decvec(&dvec);
   }


/*****  dec_rvec()  *****************************************************
 *									*
 *	dec_rvec() - Draw a relative horizontal or vertical line as     *
 *	specified in the DECRVEC parameters.  Format of command:	*
 *	<CSI> P1 ; P2 ; P3 SP |						*
 *	P1 =	0 to draw a horizontal line				*
 *		1 to draw a vertical line				*
 *		2 to draw a negative horizontal line			*
 *		3 to draw a negative vertical line			*
 *		>3 - ignore DECRVEC command				*
 *	P2 =	the line length. If < 1 pixel, draw 1 pixel length line	*
 *	P3 =	the line width. If < 1 pixel, draw 1 pixel width line	*
 *      The upper left corner is the starting point of the DECVEC.      *
 ************************************************************************/

VOID dec_rvec()

   {
    LONG    length;
    LONG    width;
    LONG    temp;
    DECVEC  dvec;		/* Used to form DECVEC command */

#ifdef DUMP
   {
    oprintf("DECRVEC \n");    
    pprint();    
   }
#endif
    /* If P1 <> 0, 1, 2 or 3 then ignore the command */
    if (cp_pbuf[0] > 3)
       {
        return;
       }

    /* If the effective line width (P3 * SSU) is less than 1 pixel,
     * make it one pixel.
     */
    if ( (width = dvec.thickness = (cp_pbuf[2] * (LONG)xl_st.sizeunit))
         < CENTIPTS_PER_PIXEL )
       {
        width = dvec.thickness = CENTIPTS_PER_PIXEL;
       }

    /* If the effective line length (P2 * SSU) is less than 1 pixel,
     * make it one pixel and calculate the effective endpoint.
     */
    if ( (length = (cp_pbuf[1] * (LONG)xl_st.sizeunit)) 
         < CENTIPTS_PER_PIXEL )
       {
        length = CENTIPTS_PER_PIXEL;
       }

    switch (cp_pbuf[0])
       {
        case 0:	    /* If horizontal line to the right, ... */
		dvec.dv_ap.xval = xl_st.curchar.ap.xval;
		dvec.dvend.xval = dvec.dv_ap.xval + length;
	        dvec.dvend.yval = dvec.dv_ap.yval = xl_st.curchar.ap.yval;
	        break;
        case 1:	    /* If vertical line down, ... */
		temp = length;
		length = width;
		width = dvec.thickness = temp;
		dvec.dv_ap.xval = xl_st.curchar.ap.xval;
		dvec.dvend.xval = dvec.dv_ap.xval + length;
	        dvec.dvend.yval = dvec.dv_ap.yval = xl_st.curchar.ap.yval;
		break;
        case 2:	    /* If horizontal line to the left, ... */
	        dvec.dvend.xval = xl_st.curchar.ap.xval;
		dvec.dv_ap.xval = dvec.dvend.xval - length;
	        dvec.dv_ap.yval = dvec.dvend.yval = xl_st.curchar.ap.yval;
		break;
        case 3:	    /* If vertical line up, ... */
		temp = length;
		length = width;
		width = dvec.thickness = temp;
	        dvec.dvend.xval = xl_st.curchar.ap.xval;
		dvec.dv_ap.xval = dvec.dvend.xval - length;
	        dvec.dv_ap.yval = dvec.dvend.yval = xl_st.curchar.ap.yval - width;
		break;
       }

    /* Go draw the vector
     */
    process_decvec(&dvec);
   }

