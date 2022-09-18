#ifndef lint
static char SccsId[] = "  @(#)cpbuf_mgt.c	4.1   LPS_ULT_TRN   7/2/90";
#endif

/* file: cpbuf_mgt.c
 *
 *	COPYRIGHT  (c)  DIGITAL  EQUIPMENT CORPORATION 1987,
 *      1988, 1989.  ALL RIGHTS RESERVED.
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
 *   Filename:  cpbuf_mgt.c
 *
 *   This file contains the routines for buffer management.
 *
 *   This file contains:
 *
 *   cp_add_to_cur_param()
 *   cp_start_new_param()
 *   cp_add_to_esc_ibuf()
 *   cp_add_to_csi_ibuf()
 *   cp_add_to_dcs_ibuf()
 *   cp_add_to_ibuf(state_ptr)
 *   cp_set_private()
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
 *   17-NOV-1987 17:01:07  mhs
 *      Original Version
 *
 *   8-FEB-1988 16:14  bf
 *      Changed return value for cp_add_to_ibuf()
 *
 *   3-MAR-1988 17:01 mhs
 *      Typecast all arithmetic operations.
 *
 *   1-JUL-1988 15:17  mhw
 *	Change parameter from WORD to LONG,  some parameters may be larger
 *	than 32K and so we need a LONG to handle this
 *
 *   end edit_history
 *
 *-----------------------------------------------------------
 */



/*  begin include_file    */

#include    "portab.h"	    /* general portability constants */
#include    "cpsys.hc"	    /* system wide constants, typedefs, etc. */
#include    "cpast.hc"	    /* defs for cpast */
#include    "cpmac.lib_hc"  /* Parameterised global macros */
#include    "cpglobal.hc"   /* global defs for xpccc files */

/*  end   include_file    */



/*
 *   cp_add_to_cur_param()
 *
 *   This routine adds a digit to the current parameter
 *   being formed.  The digit passed is a hex byte, 
 *   which is converted to decimal and then packed 
 *   into the current parameter after first left-shifting 
 *   the other digits using decimal multiplication.
 *   If the result is larger than what can be 
 *   represented within an unsigned word, the current 
 *   parameter is automatically set to the maximum 
 *   possible value.  The input to this routine is 
 *   indirect, and assumed to reside in global cp_c7, 
 *   since this routine is only called indirectly through 
 *   the second level state tables.
 *
 */

VOID cp_add_to_cur_param()
   {
    cp_pbuf[cp_pcnt] = 
	(cp_pbuf[cp_pcnt] * 10) + ((LONG)(cp_c7 & CP_DIGIT_MASK));

/*       CP_PACK_PARAM(cp_pbuf[cp_pcnt], (cp_c7 & CP_DIGIT_MASK)); */

    if (cp_pbuf[cp_pcnt] > CP_MAXPARM)
       {
	cp_pbuf[cp_pcnt] = CP_MAXPARM;
       }
   }



/*
 *
 *   cp_start_new_param()
 *
 *   This routine starts a new parameter and increments 
 *   the parameter count.  If the maximum parameter count 
 *   has already been exhausted, the pointer remains 
 *   pointing to the special "dummy" location (index of 
 *   16, which is the seventeenth element in the array), 
 *   otherwise it is incremented to the next location.
 *
 */

VOID cp_start_new_param()
   {
    if (cp_pcnt < CP_MAXPCNT)
       {
	cp_pcnt++;
       }
   }



/*
 *
 *   cp_add_to_ibuf()
 *
 *   This routine stores an Intermediate in the 
 *   Intermediate buffer.  If the maximum number of 
 *   Intermediates is exceeded, the parser enters the 
 *   specified Invalid state.
 *
 */

VOID cp_add_to_ibuf(state_ptr)
FIRST *state_ptr;
   {
    if (cp_icnt < CP_MAXICNT)
       {
        cp_ibuf[cp_icnt++] = cp_c7; /* store intermed and inc to next loc */
       }
    else
       {
        cp_setctptr(state_ptr);	    /* buffer overflow, go to invalid mode */
       }
   }



/*
 *
 *   cp_add_to_esc_ibuf()
 *
 *   This routine stores an ESC/SCS Intermediate in the 
 *   Intermediate buffer.  If the maximum number of 
 *   Intermediates is exceeded, the parser enters the 
 *   ESC_Invalid state.
 *
 */

VOID cp_add_to_esc_ibuf()
   {
    cp_add_to_ibuf(&ast_escinv);
   }



/*
 *
 *   cp_add_to_csi_ibuf()
 *
 *   This routine stores a CSI Intermediate in the 
 *   Intermediate buffer.  If the maximum number of 
 *   Intermediates is exceeded, the parser enters the 
 *   CSI_Invalid state.
 *
 */

VOID cp_add_to_csi_ibuf()
   {
    cp_add_to_ibuf(&ast_csiinv);
   }



/*
 *
 *   cp_add_to_dcs_ibuf()
 *
 *   This routine stores a DCS Intermediate in the 
 *   Intermediate buffer.  If the maximum number of 
 *   Intermediates is exceeded, the parser enters the 
 *   DCS_Invalid state.
 *
 */

VOID cp_add_to_dcs_ibuf()
   {
    cp_add_to_ibuf(&ast_dcsinv);
   }



/*
 *
 *   cp_set_private()
 *
 *   This routine sets the Private flag to TRUE.
 *
 */

VOID cp_set_private()
   {
    cp_pflag = SET_PRIVATE_FLAG;
   }

