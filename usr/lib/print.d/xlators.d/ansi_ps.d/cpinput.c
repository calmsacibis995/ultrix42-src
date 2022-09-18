#ifndef lint
static char SccsId[] = "  @(#)cpinput.c	4.1   LPS_ULT_TRN   7/2/90";
#endif

/* file: cpinput.c
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
 *   begin edit_history
 *
 *   001   9-NOV-1987 16:04:50 mhs
 *      Original Version
 *
 *   002   8-FEB-1988 16:17  bf
 *      Changed local variable in cp_switch to be of type PFD
 *      rather than PFB.
 *
 *   003  21-MAR-1988 12:42 mhs
 *      Add include for pdl_pdli.hc now that duplicate copy of
 *      cp_host has been removed from cpsys.def.
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
 *   Filename:  cpinput.c
 *
 *   This contains the input handling routines for the
 *   Common Parser.  The main routine involved is the switching 
 *   routine, cp_switch.  Input devices include the keyboard, 
 *   macro, etc.
 *
 *   This file contains:
 *
 *   cp_switch(io_flag)
 *
 *   end description
 *
 *-----------------------------------------------------------
 */





/*  begin include_file    */

#include    "portab.h"	    /* general portability constants */
#include    "cpsys.hc"	    /* system wide constants, typedefs, etc. */
#include    "cpast.hc"      /* ANSI State Table external declarations */
#include    "cpglobal.hc"   /* global defs for cpxxx files */

/*  end   include_file    */



/*
 *   cp_switch()
 *
 *   The routine sets the I/O pointer to the Keyboard, 
 *   Macro, etc., and then sets the current state pointer 
 *   to a new first level state table; all based on an 
 *   input flag.  The current mode must be exited cleanly
 *   before these actions take place.
 *
 */

VOID cp_switch(io_flag)
BYTE io_flag;
{
     PFD new_ioptr;
     FIRST *state_ptr;

     cp_exit_cur_mode();

     switch (io_flag)
     {
         case '0':    new_ioptr = cp_host;
                      state_ptr = &ast_text;
                      break;

         case '1':    new_ioptr = cp_keyboard;
                      state_ptr = &ast_keyboard;
                      break;

         case '2':    new_ioptr = cp_macro;
                      state_ptr = &ast_macro;
                      break;

         default:     new_ioptr = cp_host;
                      state_ptr = &ast_text;
                      break;
     }
     cp_setioptr(new_ioptr);
     cp_setctptr(state_ptr);
     return;
}

