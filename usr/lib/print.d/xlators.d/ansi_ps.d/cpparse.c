#ifndef lint
static char SccsId[] = "  @(#)cpparse.c	4.1   LPS_ULT_TRN   7/2/90";
#endif

/* file: cpparse.c
 *
 *	COPYRIGHT  (c)  DIGITAL  EQUIPMENT CORPORATION 1987,
 *	1988, 1989 ALL RIGHTS RESERVED.
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
 *   20-NOV-1987 15:46 mhw
 *      Initial version
 *
 *   17-JAN-1988 16:00 mhw
 *      Can not find the size of the Search Tables by using sizeof
 *         in cp_search_a_tab, so changed to end when find default 
 *         CP_SRCH_DEFAULT
 *
 *   8-FEB-1988 16:10  bf
 *      Added typecasts to change 2 pointers to arrarys of objs to be 
 *      pointers to obj.  The reason thwy were defined as pointers to arrays 
 *      was to allow them to refer to different length 2nd level tables.
 *      This warning could probably have been ignored.  Changed the return
 *      value for cp_split().
 * 
 *  24-FEB-1988 09:47  mhw
 *	Store cp_c in cp_final_char for use by pr_scs
 *
 *  15-MAR-1988 14:36 mhw
 *	Change cp_split loop be less than and equal to, is currently missing
 *	the last parameter, or the only one if only one parameter.
 * 
 *   5-MAY-1988 16:32 mhs
 *	Add cp_cmd_search to search the graphics command table.
 *
 *   3-OCT-1988 09:55 mhw
 *	When EOF is found, it should clean up the parser state before returning
 *	to the device.  This includes empty the justify buffer, exit the current
 *	mode and doing a process_showpage.  None of this was being done, except
 *	in the debug version of the parser.  Now in the main parser loop, if an
 *	EOF is found, this cleanup is done.
 *
 *   4-OCT-1988 08:07 mhw
 *	For an EOF, the parser should not call process_showpage, that is left
 *	up to the device.  The parser will only clean up itself.
 *
 *  29-NOV-1988 12:50 ejs
 *	Getchar is now inline code.  First level table is now one of TWO 
 *	possible types.  Code for detecting and using expanded tables has been
 *	added.
 *
 *   1-DEC-1988 11:02 ejs
 *	Eliminated use of cp_final_char.  Using cp_c.
 *
 *   8-DEC-1988 11:57 ejs
 *	Eliminated the NVM from inline to state change.
 *
 *   4-APR-1989 09:05	araj
 *	Worked on setup/data separation.
 *	EOF/EOJ/CFF....
 *	Created cp_eof and cp_eoj
 *
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
 *   Filename:  cpparse.c
 *
 *   This file contains the routines required to do the actual
 *   parsing of the state tables.
 *
 *   This file contains:
 *
 *   cp_search_2tab(cp_second_tab_ptr)
 *   cp_getchar()
 *   cp_parse()
 *   cp_search_a_tab(search_tab_ptr, search_word)
 *   cp_text_search()
 *   cp_cmd_search()
 *   cp_search_cur_tab()
 *   cp_split(cp_split_tab_ptr)
 *   cp_eof()
 *   cp_eoj()
 *
 *  
 *   end description
 *
 *-----------------------------------------------------------
 */



/*  begin include_file    */

#include    "portab.h"	    /* general portability constants */
#include    "cpsys.hc"	    /* system wide constants, typedefs, etc. */
#include    "cpast.hc"	    /* ANSI State Table & Token Table Declarations */
#include    "cpmac.lib_hc"  /* common parser macro library */
#include    "cpglobal.hc"   /* defs for cpxxx files */
#include    "camac.lib_hc"  /* common ansi routines macro library */
#include    "caglobal.hc"   /* defs for caxxx files */

/*  end   include_file    */

NOSHARE GLOBAL UBYTE *ibuf_loc;
NOSHARE GLOBAL UBYTE *ibuf_ptr;
NOSHARE GLOBAL WORD ibuf_len;
NOSHARE	GLOBAL DEFAULT (*getr_loc)();
NOSHARE GLOBAL DEFAULT user_g;


/*****************************************************************************
 *
 *      The Search Second Table routine is called by the cp_parse routine to 
 *      find a match to the input in the second level State Table.  
 * 
 *      The routine tests the equal tests in the Range/Equal part of the table 
 *      for a match to the 7-bit version of the input (cp_c7).  If a match is
 *      found, the associated token stored in the Token half of the table, is 
 *      returned.  If no match is found, the routine will search the range 
 *      entries and get the token associated with the range it matches to.
 * 
 *      Note a match will occur since an end test is included in the table.  
 *
 ***************************************************************************/

WORD cp_search_2tab(second_tab_ptr)
SECOND *second_tab_ptr;		    /* ptr to 2nd Level state table */
   {
    BYTE *equ_ran_ptr;		    /* ptr to the array of EQU/RAN bytes */
    WORD i;			    /* local index */
    WORD *token_tab_ptr;	    /* ptr to 2nd half of 2nd level table */

    token_tab_ptr =(WORD *)(second_tab_ptr -> tab2_2); /* ptr to 2nd half of table */
    equ_ran_ptr = (BYTE *)(second_tab_ptr -> equ_ptr); /* ptr to EQU/RAN bytes */
     
    /* Do the equal testing
     */
    for (i = 0; i < second_tab_ptr->equ_cnt; ++i)
       {
	if (cp_c7 == *(equ_ran_ptr + i))    /* if equal, get the token */
	   {
	    cp_token = *(token_tab_ptr + i);
	    return(cp_token);
	    break;
	   }
       }

    /* Do range testing if no equal match is found 
     */
    while (cp_c7 > *(equ_ran_ptr + i))
       {
	++i;
       }

    cp_token = *(token_tab_ptr + i);
    return(cp_token);
   }


/******************************************************************************
 *     Called by the parser to get a character from the current input buffer, 
 *     using the current input routine, pointed to by cp_ioptr.  Also creates 
 *     the 7-bit version of the input character.  The routine pointed to by 
 *     cp_ioptr must pass a byte in its return statement.  
 ******************************************************************************/


/***************************************************************************
 *
 *    This routine will get the current input (cp_c) and find a match to one 
 *    of the four possible ranges, C0, GL, C1, and GR.  When the correct range 
 *    is found, the associated pointer into the Second Level Table will be 
 *    taken from the First Level Table.  This pointer is used by the search 
 *    routine to find the input in the state table.  The search routine will 
 *    return to cp_parse a token.  The action indicated by this token in the 
 *    Token Conversion Table (cp_tok_tbl), is called.
 * 
 *************************************************************************/

VOID cp_parse()
   {
    WORD i;		    /* i is a local index */
    SECOND *second_tab_ptr; /* pointer to 2nd level table */

    while (TRUE)
       {
	if (!nvm_recall_flag)
	{
	    /*
	     * Read in a buffer if there isn't any data left
	     */
	    if (ibuf_len == 0) 
	       {
	        (*getr_loc) (&ibuf_len, &ibuf_loc, user_g);

		if (ibuf_len == 0)  /* if after reading, still zero, then end of file */
		   {
		    return;	/* return to the device */
	           }
		ibuf_ptr = ibuf_loc;
	       }

	    /*
	     * Return the next character in the input buffer
	     */
	    ibuf_len--;
	    cp_c7 = (cp_c = *ibuf_ptr++) & CP_7BIT_MASK;

	}


	if (cp_ctptr->encoding == EXPANDED_TABLE)
	    {
	    (*cp_tok_tbl[((WORD*)cp_ctptr->ch_array[0])[cp_c]])();
	    }
	else
	    {
	    if (cp_c <= C0)
		{
		 i = 0;	    /* character is a C0 */
		}
	    else if (cp_c <= GL)
		{
		 i = 2;	    /* character is a GL */
		}
	    else if (cp_c <= C1)
		{
		 i = 1;	    /* character is a C1 */
		}
	    else
		{ 
		 i = 3;	    /* character is a GR */
		}

	    second_tab_ptr = cp_ctptr->ch_array[i];	/* get ptr to 2nd level */
	    cp_token = cp_search_2tab(second_tab_ptr); /* Search 2nd level table */
	    (*cp_tok_tbl[cp_token])(); /* call action requested */ 
	    }		    /* end of while - forever */
       }
   }


/***************************************************************************
 *     This routine takes a parameter of a pointer to the search table and the
 *     word to search for.  This word will be compared to each entry in the
 *     table until a match found.  A token is the result.  The action 
 *     associated with with this token in the Token Table, will be called.
 *************************************************************************/

VOID cp_search_a_tab(search_tab_ptr, search_word)
PAS search_tab_ptr;		/* ptr to table to search */
UWORD search_word;		/* word to serch for in table */
   {
    WORD i;		        /* local index */

    i = 0;

    /* search for match in the search table 
     */
    while ( (    (*search_tab_ptr)[i].pif != CP_SRCH_DEFAULT)
              && (search_word != (*search_tab_ptr)[i].pif
	    )
	  )
       {
        i++;		/* auto-increment i until find a match */
       }

    cp_token = (*search_tab_ptr)[i].token; /* get the token */
    (*cp_tok_tbl[cp_token])();	/* call the action requested */
   }


/******************************************************************************
 *    This routine will get the pointer to the current search table, and set 
 *    the state to text, since all routines that call this want to be in text 
 *    state when complete.  This routine will then search for the input in the 
 *    former search table (not in the text search table) by calling the 
 *    standard search routine.
 *****************************************************************************/

VOID cp_text_search()
   {
    cp_search_tab_ptr = cp_ctptr->search_tab;  /* get ptr to search table  */
    cp_setctptr(&ast_text);             /* set ctptr to ANSI 1st level table */

    /* Create the unsigned word to search for 
     */
    cp_pif_word = CP_CREATE_PIF(cp_pflag, cp_ibuf[0], cp_c7);
    cp_search_a_tab(cp_search_tab_ptr, cp_pif_word); /* search a search table */
   }


/******************************************************************************
 *     The routine will get the pointer to the graphics command search table.
 *     It will then call the routine to search the search table pointed to by 
 *     this pointer.
 ***************************************************************************/

VOID cp_cmd_search(gr_cmd)
BYTE gr_cmd;			/* command to search for in table */
   {
    cp_search_a_tab(&ast_cmd_srch[0], (UWORD)gr_cmd); /* search the cmd tbl */
   }


/******************************************************************************
 *     The routine will get the pointer to the search table for this state.  
 *     It will then call the routine to search the search table pointed to by 
 *     this pointer.
 ***************************************************************************/

VOID cp_search_cur_tab()
   {
    cp_search_tab_ptr = cp_ctptr -> search_tab;  /* get ptr to search table */

    /* create the unsigned word to search for     
     */
    cp_pif_word = CP_CREATE_PIF(cp_pflag, cp_ibuf[0], cp_c7);
    cp_search_a_tab(cp_search_tab_ptr, cp_pif_word); /* search a search table */
   }


/*****************************************************************************
 *      Based upon the final character of the input sequence, a pointer to a 
 *      split table is passed to this routine.  Each parameter of the sequence 
 *      is found within the table and the action associated with it done.
 * 
 *      Called by such common ANSI action routines as pr_tbc and pr_sgr.
 **************************************************************************/

VOID cp_split(split_tab_ptr)
PAS split_tab_ptr;	    /* ptr to split table of type search */
   {
    WORD i;		    /* local index variable */

    /* For each parameter, find the correct action to perform.
     * (NOTE: We also set cp_currpar in parameter passing)
     */
    for (i = 0; i <= cp_pcnt; ++i)
       {
	cp_search_a_tab(split_tab_ptr, (UWORD)(cp_currpar = cp_pbuf[i]));
       }
   }


/*****************************************************************************
 *
 *      cp_eof
 *
 *      Called by cp_parse upon EOF, to output any pending data (JFY_BUF
 *	SIXEL_BUF, ...) and based on the page fragment item, to 
 *	do a conditional FF, and reset the ap.
 *
 **************************************************************************/

VOID cp_eof(reset_ap)
BOOL reset_ap;
{
    empty_jfy_buf();
    if	(reset_ap)
	{
	    hpos_abs(xl_st.h_fmt_bound.min);	/* Go to start of line */
	    update_avp(xl_st.v_fmt_bound.min, &xl_st.v_lim_bound);
	    xl_st.fcf = TRUE;
	    
	};

}

/*****************************************************************************
 *
 *      cp_eoj
 *
 *      Called by cp_parse upon EOJ, to terminate the processing of a job
 *	cp_eof has already been called. 
 *
 **************************************************************************/

VOID cp_eoj()
{
    cp_exit_cur_mode();

}
