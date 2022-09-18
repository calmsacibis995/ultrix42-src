#ifndef lint
static char SccsId[] = "  @(#)camode.c	4.1   LPS_ULT_TRN   7/2/90";
#endif

/* file: camode.c
 *
 *	COPYRIGHT  (c)  DIGITAL  EQUIPMENT CORPORATION 1987,
 *	1988, 1989.  ALL RIGHTS RESERVED.
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
 *   2-DEC-1987 13:45 mhw
 *      Initial Entry
 *  25-JAN-1988 08:55 mhw
 *      Added duset.c and ducset.c to this file camode.c
 *  28-JAN-1988 10:29 bf
 *	Split set and reset code into action routines.  Add
 *	code to pr_set and pr_reset so they set a global to
 *	indicate what the action routine should do.
 *	Add pr_bcmm() to this file.
 *  15-FEB-1988 10:39  mhw
 *      Add include for camac.lib for non-dump oprintf
 *
 *  17-MAR-1988 15:45 mhw
 *	pr_ssu was using cp_currpar as the value of the parameter in
 *	the escape sequence.  cp_currpar is only set in a split table
 *	call which is not used for ssu.  The correct location of the 
 *	parameter is cp_pbuf[0]
 *
 *  25-MAR-1988 10:38 mhw
 *	Added dec_set and dec_reset
 *
 *   8-APR-1988 15:58 mhw
 *	add centipoints to pr_ssu
 *
 *  30-NOV-1988 14:28 araj
 *	Install cfont caching, change calls to compute font into calls to invalidate_font
 *	Make sure references to vai or g_table have a test for the validity of the data
 *	first
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
 *   Filename:  camode.c
 *
 *   Set and Reset functions for dump utility   
 *
 *   This module includes the following routines:
 *
 *   pr_set()  pr_reset()   
 *   dec_set()  dec_reset()   
 *   pr_lnm()  pr_pum()
 *   dec_awm() dec_psp()
 *   dec_psm() dec_crnlm()
 *   dec_opm() pr_crm()
 *   dec_tc1()  dec_ac1()
 *   pr_s7c1t() pr_s8c1t()
 *   pr_bcmm()
 *
 *   pr_ssu ()
 *   set_mode_private ()
 *   reset_mode_private ()
 *   set_c1rcv_mode ()
 *   reset_c1rcv_mode ()
 *
 *   end description
 *
 *-----------------------------------------------------------
 */



/* Translator edit history 
 *
 * file:	xlc_mode.c - Translator mode routines
 * created:	gh	 26-MAR-1986 09:10:31
 * edit:	gh	 2-APR-1986 12:51:24 Adding private mode routines
 *			 and deleted individual mode routines, placing the
 *			 functionality into set_mode_.. and reset_mode_..
 *		gh	 18-APR-1986 14:32:58 Adding set_c1rcv_mode() and
 *			 reset_c1rcv_mode()
 *		gh	  23-APR-1986 11:15:17 Changed names to 
 *			 set_c1rcv_mode() and reset_c1rcv_mode()
 *		nv	 10-JUL-1986 18:22:15 Added a dummy variable in
 *			functions set_mode_private() and reset_mode_private().
 *			Changed the way those two functions call
 *			compute_spacing().
 */



#define camode (1)

/*  begin include_file    */

#include    "portab.h"	    /* general portability constants */
#include    "cpsys.hc"	    /* system wide constants, typedefs, etc. */
#include    "cpast.hc"	    /* For switch tables, portab.h		*/
#include    "caglobal.hc"   /* defs for this file			*/
#include    "camac.lib_hc"  /* include non-dump oprintf                 */

#ifdef DUMP

#include    "dumpu.hc"      /* Dump Utility                             */
#include    "xlc_iface.hc"  /* defines for oprintf			*/

#endif DUMP 

/*  end   include_file    */



/****************************************************************************
    Set used for such functions as: pr_crm, pr_lnm, pr_pum, 
**************************************************************************/
VOID pr_set()
   {
#ifdef DUMP
   {
    oprintf("SET \n");    
    pprint();		    /* Print the parameter list */
   }
#endif
    cm_mode = CM_SET;
    cp_split(ast_mode_srch);
    cm_mode = CM_UNSET;
   }



/****************************************************************************
    re-Set used for such functions as: p_crm, pr_lnm, pr_pum
**************************************************************************/

VOID pr_reset()
   {
#ifdef DUMP
   {
    oprintf("RESET \n");    
    pprint();		    /* Print the parameter list */
   }
#endif
    cm_mode = CM_RESET;
    cp_split(ast_mode_srch);
    cm_mode = CM_UNSET;
   }



/****************************************************************************
    Set used for such functions as: dec_opm, dec_awm,
    dec_crnlm, dec_psm, dec_psp, dec_rlm
**************************************************************************/
VOID dec_set()
   {
#ifdef DUMP
   {
    oprintf("SET \n");    
    pprint();		    /* Print the parameter list */
   }
#endif
    cm_mode = CM_SET;
    cp_split(ast_decmode_srch);
    cm_mode = CM_UNSET;
   }



/****************************************************************************
    re-Set used for such functions as: dec_opm, dec_awm,
    dec_crnlm, dec_psm, dec_psp, dec_rlm
**************************************************************************/

VOID dec_reset()
   {
#ifdef DUMP
   {
    oprintf("RESET \n");    
    pprint();		    /* Print the parameter list */
   }
#endif
    cm_mode = CM_RESET;
    cp_split(ast_decmode_srch);
    cm_mode = CM_UNSET;
   }



/****************************************************************************
    Positioning Unit Mode
**************************************************************************/

VOID pr_pum()
   {
#ifdef DUMP
   {
    oprintf("PUM \n");    
   }
#endif 
    if (cm_mode == CM_SET)
       {
	xl_st.pum_mode = PUM_SIZEUNIT;
       }
    else if (cm_mode == CM_RESET)
       {
	xl_st.pum_mode = PUM_CHAR;
       }
   }



/*************************************************************************
    Select Size Unit 
*************************************************************************/

VOID pr_ssu()
   {
    /*  Let the split routine call the appropriate action
     *  routine for each of the parameters.
     */
    cp_split(ast_ssu_srch);
   }



/*************************************************************************
    DEC Private Select Size Unit 
*************************************************************************/

VOID dec_ssu()
   {
    /*  Let the split routine call the appropriate action
     *  routine for each of the parameters.
     */
    cp_split(ast_decssu_srch);
   }



/*************************************************************************
    Select Size Unit - Decipoints
*************************************************************************/

VOID pr_decipoints()
   {
#ifdef DUMP
   {
    oprintf("DECIPOINTSS \n");    
    pprint();		    /* Print the parameter list */
   }
#endif
    xl_st.sizeunit = DECIPT;
   }



/*************************************************************************
    Select Size Unit - Pixels
*************************************************************************/

VOID pr_pixels()
   {
#ifdef DUMP
   {
    oprintf("PIXELS \n");    
    pprint();		    /* Print the parameter list */
   }
#endif
    xl_st.sizeunit = PIXEL;
   }



/*************************************************************************
    Select Size Unit - Centipoints
*************************************************************************/

VOID dec_centipoints()
   {
#ifdef DUMP
   {
    oprintf("CENTIPOINTS \n");    
    pprint();		    /* Print the parameter list */
   }
#endif
    xl_st.sizeunit = CENTIPT;
   }



/****************************************************************************
    Line Feed/ New Line Mode    
**************************************************************************/

VOID pr_lnm()
   {
#ifdef DUMP
   {
    oprintf("LNM \n");    
    pprint();		    /* Print the parameter list */
   }
#endif
    if (cm_mode == CM_SET)
       {
    	xl_st.lfnl_mode = TRUE;
       }
    else if (cm_mode == CM_RESET)
       {
	xl_st.lfnl_mode = FALSE;
       }
   }




/****************************************************************************
    Autowrap Mode    
*****************************************************************************/
VOID dec_awm()
   {
#ifdef DUMP
   {
    oprintf("DECAWM \n");    
    pprint();		    /* Print the parameter list */
   }
#endif
    if (cm_mode == CM_SET)
       {
	xl_st.wrap_mode = TRUE;
       }
    else if (cm_mode == CM_RESET)
       {
	xl_st.wrap_mode = FALSE;
       }
   }



/****************************************************************************
    Proportional Spacing    
**************************************************************************/

VOID dec_psp()
   {
#ifdef DUMP
   {
    oprintf("DECPSP \n");    
    pprint();		    /* Print the parameter list */
   }
#endif
    if (cm_mode == CM_SET)
       {
	xl_st.psp = TRUE;
    	invalidate_font();
       }
    else if (cm_mode == CM_RESET)
       {
	xl_st.psp = FALSE;
    	invalidate_font();
       }
   }



/****************************************************************************
    Horizontal Pitch Select Mode    
**************************************************************************/

VOID dec_psm()
   {
    LONG dummy;

#ifdef DUMP
   {
    oprintf("DECPSM \n");    
    pprint();		    /* Print the parameter list */
   }
#endif

    if (cm_mode == CM_SET)
       {
	xl_st.psel_mode = TRUE;
        /* Combine the values of DECPSM and DECSHORP to define
         * xl_st .hai. The dummy values are not used here.
         */
        compute_spacing (&dummy, &dummy);
       }
    else if (cm_mode == CM_RESET)
       {
	xl_st.psel_mode = FALSE;
        /* Combine the values of DECPSM and DECSHORP to define
         * xl_st .hai. The dummy values are not used here.
         */
        compute_spacing (&dummy, &dummy);
       }
   }



/****************************************************************************
    Carriage Return/Newline Mode    
**************************************************************************/

VOID dec_crnlm()
   {
#ifdef DUMP
  {
    oprintf("DECCRNLM \n");    
    pprint();		    /* Print the parameter list */
  }
#endif
    if (cm_mode == CM_SET)
       {
	xl_st.crnl_mode = TRUE;
       }
    else if (cm_mode == CM_RESET)
       {
	xl_st.crnl_mode = FALSE;
       }
   }

/****************************************************************************
    Origin Placement Mode    
**************************************************************************/

VOID dec_opm()
   {
#ifdef DUMP
   {
    oprintf("DECOPM \n");    
    pprint();		    /* Print the parameter list */
   }
#endif
    if (cm_mode == CM_SET)
       {
	empty_jfy_buf();
    	setorigin((BOOL)TRUE);
       }
    else if (cm_mode == CM_RESET)
       {
	empty_jfy_buf();
    	setorigin((BOOL)FALSE);
       }
   }



/****************************************************************************
    Control Rendition Mode
**************************************************************************/

VOID pr_crm()
   {
#ifdef DUMP
   {
    oprintf("CRM\n");    
    pprint();		    /* Print the parameter list */
   }
#endif
    if (cm_mode == CM_SET)
       {
	empty_jfy_buf();
	cp_setctptr(&ast_crm);	/* Set Table Ptr to Control Rendition Table */
       }
    else if (cm_mode == CM_RESET)
       {
        cp_setctptr(&ast_text);	/* Set state to TEXT */
       }
   }



/****************************************************************************
    C1 Receive Disable
***************************************************************************/

VOID dec_tc1()
   {
#ifdef DUMP
   {
    oprintf("DECTC1 \n");
   }
#endif
    xl_st.c1rcv_mode = CP_7BIT_MASK;
   }



/****************************************************************************
    C1 Receive Enable
***************************************************************************/

VOID dec_ac1()
   {
#ifdef DUMP
   {
    oprintf("DECAC1 \n");
   }
#endif
    xl_st.c1rcv_mode = CP_8BIT_MASK;
   }



/****************************************************************************
    Enable C1 Transmit
***************************************************************************/

VOID pr_s8c1t()
   {
#ifdef DUMP
   {
    oprintf("S8C1T \n");
   }
#endif
   }



/****************************************************************************
    Disable C1 Transmit
***************************************************************************/

VOID pr_s7c1t()
   {
#ifdef DUMP
   {
    oprintf("S7C1T \n");
   }
#endif
   }



/*************************************************************************
    ???? Mode
*************************************************************************/

VOID pr_bcmm()
#ifdef DUMP
   {
    if (cm_mode == CM_SET)
       {
	oprintf("PRBCMM set \n");
       }
    else if (cm_mode == CM_RESET)
       {
	oprintf("PRBCMM reset \n");
       }
   }
#else
   {
   }
#endif

