#ifndef lint
static char SccsId[] = "  @(#)cpspoint.c	4.1   LPS_ULT_TRN   7/2/90";
#endif

/* file: cpspoint.c
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
 *   001    17-NOV-1987 16:21:42 mhs
 *      Original Version
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
 *   Filename:  cpspoint.c
 *
 *   This file contains the routines for dealing with pointers.
 *   Included are the switching routine to set the I/O pointer, 
 *   and the routine for setting the current table pointer.
 *
 *   This file contains:
 *
 *   cp_setioptr(new_ioptr)
 *   cp_setctptr(state_ptr)
 *
 *   end description
 *
 *-----------------------------------------------------------
 */





/*  begin include_file    */

#include    "portab.h"	    /* For portable definitions of types, etc.	*/
#include    "cpsys.hc"	    /* Parser global variables and structures, etc. */
#include    "cpglobal.hc"   /* defs for cpxxx files */

/*  end   include_file    */



/*
 *   cp_setioptr()
 *
 *   Sets the I/O pointer to the Keyboard, Macro, etc.
 *   based on the input.  This is a pointer to a routine 
 *   returning DEFAULT.
 *
 */

VOID cp_setioptr(new_ioptr)
PFD new_ioptr;
   {
    cp_ioptr = new_ioptr;
   }


/*
 *   cp_setctptr()
 *
 *   Sets the current state pointer to a new first level
 *   state table, based on the input.
 *
 */

VOID cp_setctptr(state_ptr)
FIRST *state_ptr;
   {
    cp_ctptr = state_ptr;
   }

