#ifndef lint
static char SccsId[] = "  @(#)cpast.c	4.1   LPS_ULT_TRN   7/2/90";
#endif

/* file: cpast.c
 *
 *      COPYRIGHT  (c)  DIGITAL  EQUIPMENT CORPORATION 1987,
 *      1988, 1989.  ALL RIGHTS RESERVED.
 *
 *      THIS SOFTWARE IS FURNISHED UNDER A LICENSE AND MAY BE
 *      USED AND COPIED ONLY IN ACCORDANCE  WITH THE TERMS OF
 *      SUCH  LICENSE  AND  WITH  THE  INCLUSION OF THE ABOVE
 *      COPYRIGHT  NOTICE.  THIS SOFTWARE OR ANY OTHER COPIES
 *      THEREOF   MAY  NOT  BE  PROVIDED  OR  OTHERWISE  MADE
 *      AVAILABLE  TO  ANY  OTHER  PERSON.  NO  TITLE  TO AND
 *      OWNERSHIP  OF  THE  SOFTWARE  IS  HEREBY TRANSFERRED.
 *
 *      THE INFORMATION IN THIS SOFTWARE IS SUBJECT TO CHANGE
 *      WITHOUT  NOTICE  AND SHOULD  NOT BE  CONSTRUED  AS A
 *      COMMITMENT  BY  DIGITAL EQUIPMENT CORPORATION.
 *
 *      DIGITAL  ASSUMES  NO RESPONSIBILITY  FOR THE  USE  OR
 *      RELIABILITY  OF ITS SOFTWARE ON EQUIPMENT THAT IS NOT
 *      SUPPLIED BY DIGITAL.
 *
 */



/*
 *-----------------------------------------------------------
 *
 *   begin description
 *
 *   Filename:  cpast.c
 *
 *   This file contains the ANSI state tables used by the Common 
 *   Parser.  The templates are defined in CPSYS.DEF.
 *
 *   The first level state tables are FIRST structures of three 
 *   elements: exit routine pointer; search table pointer; array 
 *   of four sixteen-bit pointers to table addresses for C0, C1, 
 *   GL and GR (to cover the 00h through FFh index range as four 
 *   subranges).
 *
 *   The  level state tables are SECOND structures of 
 *   three elements: token entry table pointer; equate count; 
 *   equ/range table pointer.  The token entry tables are array 
 *   of WORD and the equ/range tables are array of BYTE.
 *
 *   The search tables are arrays of SEARCH structure; which 
 *   means they are arrays of index/action routine pairs.  The 
 *   action routines are referenced through tokens, and the 
 *   indeces are a WORD created by merging the Intermediate, 
 *   Final and Private Flag.  The search tables are used to 
 *   locate the action routine for a given escape (ESC, CSI 
 *   or DCS) sequence.
 *
 *   Additionally, there is a search table for graphics 
 *   commands, and split tables for parameterised sequences 
 *   such as PFS, SGR, etc.
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
 *    4-NOV-1987 12:17:57 mhs
 *      Original Version
 *   14-JAN-1988 10:04 mhw
 *      Changed ast_esc_gl_tok and ast_csi_gl_tok to call cp_text_search as 
 *         defined in the Design Spec, to process finals, and not ac_text.
 *      Changed ESC & CSI Search Table's default to be ac_nop not ac_text. 
 *         cp_text_search will have set the state to text already, so not 
 *         needed.      
 *   17-JAN-1988 14:37 mhw
 *      Added equates to ast_esc_gl table to dcs,csi,ocs,pm and apc.  These are
 *         final states that should be called from the search table and set to
 *         text, but simply called.
 *
 *   10-FEB-1988 15:56 bf
 *      Changed C initialised declarations into executable code to
 *         initialise various tables.
 *
 *   11-FEB-1988 12:50 bf
 *	Removed all variable definitions in separate .vc file.
 *	    Changed all declarations to extern names.
 *	    This is to ensure proper variable placement.
 *
 *   22-MAR-1988 17:18 mhw
 *      Change ignore mode first level table such that C1 goes to
 *      general C1 table, not ignore C0 table
 *      Fixed ast_mode_srch table, used decimal number as hex to
 *      create search word
 *
 *   25-MAR-1988 11:14 mhw
 *      Added ast_decmode_srch table and added dec_set and dec_reset to
 *      CSI search table
 *
 *   29-MAR-1988 16:28 mhs
 *      Replace usage of CP_PACK_PARAM with full hex words.
 *      Add JFY split tables.
 *
 *   13-APR-1988 12:45 mhs
 *      Fix entries in graphics token tables that didn't point
 *      to "ac_" routines.
 *
 *   14-APR-1988 15:25 mhw
 *	Correct type for final character of dec_snc
 *	added dec_sss (Select Sheet Size) to csi search table
 *	added intermediate and final to search table for DECRVEC & DECVPFS
 *	added reserve memory routines dec_rfnm and dec_rbmm
 *
 *    4-MAY-1988 10:46 mhs
 *	Move DCS tokens to search table from DCS state table, as they 
 *	currently are not accessible when the DCS sequence contains params.
 *	Fix entries to LFF and other DCS tables that point to wrong routines.
 *
 *  11-MAY-1988 16:16 mhs
 *	Remove pr_nul from General C0 table since NULL is not supported on
 *	the VAX and should be handles as a NOP, just as the other unsupported
 *	C0 and C1 codes are.
 *      Correct IGNORE Mode to use the DCS C0 table, and rename it DCS IGNORE
 *	Mode.  This mode is entered when there's no font memory to reserve,
 *	there's bad syntax within a DCS after the Final is received, or an
 *	APC, PM or OPM is received.  It ignores all GL/GR, accepts only ESC,
 *	SUB and CAN among the C0 characters, and accepts all C1 (since they
 *	can be accessed via ESC anyway).  SUB is reprocessed as "?", which is 
 *	a NOP in IGNORE Mode.
 *	Revise DCS Invalid handling to match ESC & CSI Invalid handling, with
 *	the exception that all GL/GR are ignored since only an ST is a true
 *	Final for a DCS.
 *      Replace ast_nosrch with typecasting of NULLPTR to PAS.
 *	Fix CRM GL tables to handle the four corner characters (SP, DEL, XA0
 *	and XFF).  Note that the entire table could just be a call to 
 *	ac_crnr_crm (after an appropriate name change), with the "default"
 *	case in the switch statement in that routine covering normal text.
 *
 *   3-JUN-1988 07:40 mhs
 *	Restore dcsignore to previous version, and create sxlignore for
 *	special processing of C0/SUB.  Use only for LFF/MAC/GRAPHICS/DLD.
 *
 *   4-AUG-1988 14:10 ejs
 *  Deleted cp_tok_tbl declaration.  Now created as part of maketok process.
 *
 *   end edit_history
 *
 *-----------------------------------------------------------
 */


/*
 *   Filename :  cpast.c
 */

/*
 * Include Files
 */

#include    "portab.h"
#include    "cpsys.hc"
#include    "cpast.hc"      /* External declarations for cpast.c */
#include    "cptok.tc"	    /* Token declarations and externals */
#include    "cpglobal.hc"   /* defs for parser routines */
#include    "caglobal.hc"   /* ANSI global variables and structures, etc. */


/*-----------------------------------------------------------
 * Token Conversion Table  (Produced by "maketok" tool)
 *-----------------------------------------------------------
 */

#include    "cptok.tkc"     /* Token Conversion Table code */



int cp_table_init()
   {
    cp_init_tok_tbl();
    return (1);
   }

