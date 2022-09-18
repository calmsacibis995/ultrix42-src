#ifndef lint
static char SccsId[] = "  @(#)cpsixel.c	4.1   LPS_ULT_TRN   7/2/90";
#endif

/* file: cpsixel.c
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
 *   Filename:  cpsixel.c
 *
 *   This file contains the routines for handling sixel commands.
 *
 *   This file contains:
 *
 *   cp_store_cmd()
 *   cp_do_cur_cmd()
 *   cp_do_store_cmd()
 *   cp_font_name()
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
 *   17-NOV-1987 16:30 mhs
 *      Original Version
 *
 *   5-MAY-1988 15:23 mhs
 *	Fix problem with graphics commands overwriting sixel data.
 *
 *  29-SEP-1988 14:14 mhw
 *	Changed cp_do_store_cmd such that it no longer calls cp_store_cmd.
 *	cp_store_cmd expected a parameter that cp_do_store_cmd was not
 *	passing.  Also cp_store_cmd wiped out the parameter buffer by doing
 *	a cp_reset and the parameters were still needed by the previous
 *	sixel command that had not yet been executed.  All of the work that
 *	was needed is now done within cp_do_store_cmd itself.  This fixed the
 *	the sixel repeat count bug and goodness knows what other bugs.
 *
 *   end edit_history
 *
 *-----------------------------------------------------------
 */



/*  begin include_file    */

#include    "portab.h"	    /* general portability constants */
#include    "cpsys.hc"	    /* system wide constants, typedefs, etc. */
#include    "cpast.hc"      /* defs for cpast */
#include    "cpglobal.hc"   /* global parser defs */

/*  end   include_file    */



/*
 *
 *   cp_store_cmd()
 *
 *   Store the graphics command before the parameters are 
 *   received, using the 7-bit version.  We must first reset
 *   the parameter buffer (by calling cp_reset) so that, for
 *   example, the next repeat count won't multiply the old 
 *   repeat count and add it to the new one.  Note that if
 *   we call cp_reset AFTER setting cp_sxlcmd, it will
 *   overwrite our sixel command value with a "-1".  The
 *   sixel command is passed as a parameter since cp_reset
 *   erases both cp_c7 and cp_sxlcmd and presents a chicken
 *   or egg problem for us otherwise.
 *
 */

VOID cp_store_cmd(sxlcmd)
UBYTE sxlcmd;
   {
    cp_reset();		/* reset the param buff for the next graphics cmd */
    cp_sxlcmd = sxlcmd;
   }


/*
 *
 *   cp_do_cur_cmd()
 *
 *   Get the previously stored command and execute it.
 *
 */

VOID cp_do_cur_cmd()
   {
    cp_cmd_search(cp_sxlcmd);	/* execute the current graphics command */
   }


/*
 *
 *   cp_do_store_cmd()
 *
 *   Get the previously stored command and execute it.
 *   Save the current command.
 *
 */

VOID cp_do_store_cmd()
   {
    UBYTE newcmd;		/* define temporary storage for new command */

    newcmd = cp_c7;		/* save new command, cp_reset will wipe out
				    cp_c7
				 */
    cp_cmd_search(cp_sxlcmd);	/* execute the current graphics command */
    cp_reset();		/* reset the param buff for the next graphics cmd */
    cp_sxlcmd = newcmd; 
   }


/*
 *
 *   cp_font_name()
 *
 *   Adds the new character to the Font Name Buffer.  
 *   If too many characters are received, the DCS IGNORE
 *   state is entered.  This routine is very similar to 
 *   the cp_add_to_ibuf routine.
 *
 */

VOID cp_font_name()
   {
    if (cp_font_name_cnt <= amt_allocated)
       {
        cp_font_name_buf[cp_font_name_cnt++] = cp_c7;
       }
    else
       {
	cp_setctptr(&ast_dcsignore);	/* set state to DCS IGNORE */
       }
   }

