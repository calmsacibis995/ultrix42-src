#ifndef lint
static char SccsId[] = "  @(#)cpstartup.c	4.1   LPS_ULT_TRN   7/2/90";
#endif

/* file: cpstartup.c
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
 *   begin description
 *
 *   Filename:  cpstartup.c
 *
 *   This file contains the startup and initialisation routines.
 *
 *   This file contains:
 *
 *   cp_init()
 *   cp_reset()
 *
 *   end description
 *
 *-----------------------------------------------------------
 */


/*
 *-----------------------------------------------------------
 *
 *  begin edit_history
 *
 *  17-NOV-1987 16:51 mhs
 *      Original Version
 *
 *  11-FEB-1988 08:34  mhw
 *      Added temporary call to cp_table_init to intialize the 
 *      state tables.
 *
 *  21-MAR-1988 12:45 mhs
 *      Add include for pdl_pdli.hc now that duplicate copy of
 *      cp_host var has been removed from cpsys.def.
 *
 *  30-MAR-1988 17:17 mhs
 *      Add call to pfs_table_init.
 *
 *   4-AUG-1988 14:10 ejs
 *	Deleted cp_table_init call and prototype.
 *
 *  end edit_history
 *
 *-----------------------------------------------------------
 */



/*  begin include_file    */

#include    "portab.h"	    /* general portability constants */
#include    "cpsys.hc"	    /* system wide constants, typedefs, etc. */
#include    "cpast.hc"	    /* defs for cpast */
#include    "cpglobal.hc"   /* global defs for cpxxx files */
#include    "caglobal.hc"   /* global defs for caxxx files */

/*  end   include_file    */


/*
 *   cp_init()
 *
 *   Initialise the parser variables and data structures.
 *   Calls cp_reset to reset the buffers and counters, 
 *   initialises table pointers and other context vars.
 *
 */

VOID cp_init()
   {
    cp_setioptr(cp_host);   /* set io ptr to host input */
    cp_setctptr(&ast_text); /* set table pointer to text table */
    cp_reset();		    /* reset parameter and intermediate
			       buffers to initial values */
   }


/*
 *   cp_reset()
 *
 *   This routine gets the parser to a known state, by 
 *   clearing all buffers and indeces/counters.  It 
 *   preserves context by not affecting pointers, etc.
 *
 */

VOID cp_reset()
   {
    WORD i;

    for (i = 0; i < CP_MAXPCNT; ++i)  /* clear parameter buffer + dummy loc */
       {
        cp_pbuf[i] = 0;
       }
    for (i = 0; i <= CP_MAXICNT; ++i)  /* clear intermediate buffer */
       {
        cp_ibuf[i] = 0;
       }
    for (i = 0; i < CP_MAX_FONT_CHAR; ++i)  /* clear font name buffer */
       {
        cp_font_name_buf[i] = 0;
       }
    cp_pcnt = 0;		/* reset parameter count */
    cp_icnt = 0;		/* reset intermediate count */
    cp_font_name_cnt = 0;	/* reset font name count */
    cp_token = 0;		/* clear the token conversion table index */
    cp_sxlcmd = 0x7F;		/* choose value that will be ignored */
    cp_c = 0;			/* zeroed to prevent incorrect values */
    cp_c7 = 0;		        /* zeroed to prevent incorrect values */
    cp_pflag = RESET_PRIVATE_FLAG; /* reset DEC Private flag */
    cp_pif_word = 0;		/* clear the P/I/F word */
   }

