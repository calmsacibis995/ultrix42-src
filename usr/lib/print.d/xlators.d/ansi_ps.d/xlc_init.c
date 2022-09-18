#ifndef lint
static char *sccsid = "@(#)xlc_init.c	4.1      ULTRIX 7/2/90";
#endif

/* file:	xlc_init - Translator initialization routines
 * created:	gh	 26-MAR-1986 09:10:31
 * edit:	gh	 27-MAR-1986 08:41:50 Fixing default horiz. tab stops
 *		gh	 7-APR-1986 13:52:30 Changing default itemlist PFS vals
 *		gh	 10-APR-1986 13:47:44 Fixed bug in (),
 *			 added ps state initialization in state_init.
 *			 Added xl_st.orientation initialization in state_init()
 *		gh	 23-APR-1986 08:19:59 Changing state_init() because
 *			 XLATE.H structures changed.
 *		gh	 24-APR-1986 09:08:14 Changed SGR structure name
 *			 Changing state_init to not init PS variables, but to
 *			 call ps_init(). Adding last_showpage init.
 *		gh	 29-APR-1986 10:12:10 Adding xl_st.sheet_len itlzation
 *		gh	 1-MAY-1986 10:43:07 Changing default states of
 *			 lfnl and ssu modes to FALSE and DECIPT.
 *		gh	 3-JUN-1986 15:01:50 Item list selecting landscape
 *			 mode only sets orientation - scan_items should return
 *			 value of 21 - changed that.
 *		gh	 13-JUN-1986 08:01:58 Changing the initialization of
 *			 .plf
 *		nv	 27-JUN-1986 15:03:30 Renamed 'xl_st.curchar.attrs' to
 *			'xl_st .requested_attributes.'
 *		nv	 10-JUL-1986 17:47:49 Incorporated new function called
 *			reset_pitch ().
 *			Revised the way xl_st .propl_mode is initialized.
 *		nv	 12-JUL-1986 08:13:22 Removed the initialization of 
 *			the obsolute variables 'opm' and 'pcf'.
 *			Initialized 'psel_mode'.
 *			Removed redundant initialization in reset_pitch
 *			of .character_proportion and .cur_sgr.
 *			Added comments.
 *		laf	 30-JUL-1986 10:36:02 Removed reference to unused
 *			variable "pdef"
 *		laf	 27-AUG-1986 14:29:07 Added initz'n of xl_st.
 *			curchar.ul_avp to state_init().
 *
 *		araj	 7-OCT-1986 17:22:00 added 
 *			initialization for max_bound
 *		nv	 17-OCT-1986 11:23:24 Remove all reference to 'hsi'.
 *		nv	 21-OCT-1986 17:02:49 Remove initialization of
 *			'xl_st .shorp' from 'reset_pitch'; it is initialized
 *			in 'state_init'.
 *			Remove initialization of 'xl_st .vai' from
 *			'reset_pitch'; it is calculated as a result of the 
 *			call to 'compute_font'.
 *			Changed the initialization of 'xl_st .vai' and
 *			'xl_st .hsi' to be semi-proportional.
 *		nv	 22-OCT-1986 09:59:29 Redesigned 'htabs_init' and 
 *			'vtabs_init'.
 *		nv	 22-OCT-1986 13:40:06 Added current sgr initialization 
 *			with pfs initialization in 'scan_items' to make the
 *			initial sgr paper size and orientation dependent. 
 *		nv	 21-NOV-1986 17:58:25 In 'scan_items' initialized and 
 *			conditionally set 'xl_st .vm_size'.
 *		mgb	 13-JAN-1987 16:01:09 Adding option to have sixels
 *			converted into hex or binary output. Routine that was
 *			modified was scan_items().
 *		mgb	 13-JAN-1987 16:04:24 removed parameted list from
 *			state_init() since it is not used.
 *		kws      13-FEB-1987 15:16:19 Added B size and legal paper
 *			support.
 *		kws	 13-MAR-1987 15:17:08 Select SGR 10 by default with
 *                      B size paper.
 *
 *		araj	 27-MAR-1987 20:41:29 added support for 2 new 
 *			output modes. 
 *
 *		araj	 30-MAR-1987 12:40:37 
 *			changed to use official names for the 2 new 
 *			output modes
 *
 *		araj	 1-APR-1987 13:39:38 
 *			Modified to swap height and width when landscape 
 *			is selected.
 *
 *              kws	 13-MAY-1987 16:14:26 
 *			SPR ICA-04651.  Format page for A4 landscape 
 *			when selected from print command.  Page is
 *                      currently formatted for A landscape.
 *
 *		mgb	 7-JUL-1987 13:31:43 
 *			Put break in case statement that sets VM size.
 *			If you send VM size you would set 7 bit also which
 *			is not good.
 *
 *			Also added #if SYMBIONT_DEBUG prints.
 *
 *		araj	 16-JUL-1987 20:23:44 
 *			Attempt to fix  LPS40 QAR 144, by undoing Keith's
 *			13-may change to termmanag, and moving the 
 *			default initialization of SGR to here. Added 
 *			defaulting to SGR of 1 if no paper size/orientation
 *			is passed to us.
 *
 *		mgb	 19-JAN-1988 16:12:51 
 *			Common Parser update - changed parse_init() to
 *			cp_init(). Changed c to cp_c.
 *
 *		mgb	 27-JAN-1988 16:33:05 
 *			Spilt xlate.h into xlate.h & capdl.hc for the
 *			common parser. Later capdl.def of the common
 *			parser will have capdl.hc.
 *
 *		tep	26 May 1988 13:18
 *			15 June 1988 16:02 - changes to enable sixel_font
 *				for sixel translation
 *
 *		kws	 23-SEP-1988 17:15:12 
 *			Take VM size passed by the symbiont as gospel.
 *			Only account for the size of the prologue if
 *			it has not been preloaded.
 *
 *		kws	 17-OCT-1988 16:50:23 
 *			Add initialization for all paper sizes supported
 *			by a PS machine and selected via the print command
 *			line.  Fill in the PFSBOUNDS structure for format
 *			and limit bounds for all PFS and DECVPFS paper
 *			sizes.
 *
 *		kws	 23-OCT-1988 18:57:19 
 *			Change sixel output default back to binary from
 *			sixelfont.
 *
 *		kws	 24-OCT-1988 10:18:02 
 *			Add xlc_init.def
 *
 *		cp	 20-MAR-1989
 *			Changed declaration of output_sixel functions as
 *			returning 'BYTE' rather than 'int' for Ultrix port.
 *
 *		araj	28-MAR-1989 11:09
 *			fix scan items, (separate scanning from decision making)
 *			replace #if symbiont_debug with ps_trace_macro
 *			
 *		araj	29-MAR-1989 14:31
 *			Changed default SGR to be 0, not 1.
 *
 *		araj	10-APR-1989 09:09
 *			changed ps_trace_macro to item_trace_macro
 *
 *		araj	12-APR-1989 11:32
 *			added processing for SGR item
 *
 *		ejs	13-APR-1989 10:24
 *			last_showpage is a symbiont function and is moved to
 *			xlc_main
 *		bf	 3-OCT-1989 10:20
 *			Change scan_items() to fix two problems:
 *			  A) Was using 10 cpi instead of 10.3 cpi font for 
 *			     A4 paper size,
 *			  B) Was resetting an incorrect Font size on DECSTR
 *			     or RIS for certain paper sizes in Landscape.
 *                      Both problems were due to the SGR value passed down 
 *			from the symbiont not being manipulated correctly.
 */


/************************************************************************
 *                                                                      *
 *      COPYRIGHT  (c)  DIGITAL  EQUIPMENT CORPORATION 1985,            *
 *            1986, 1987, 1988, 1989                                    *   
 *		      ALL RIGHTS RESERVED.                              *
 *                                                                      *
 *      THIS SOFTWARE IS FURNISHED UNDER A LICENSE AND MAY BE           *
 *      USED AND COPIED ONLY IN ACCORDANCE  WITH THE TERMS OF           *
 *      SUCH  LICENSE  AND  WITH  THE  INCLUSION OF THE ABOVE           *
 *      COPYRIGHT  NOTICE.  THIS SOFTWARE OR ANY OTHER COPIES           *
 *      THEREOF   MAY  NOT  BE  PROVIDED  OR  OTHERWISE  MADE           *
 *      AVAILABLE  TO  ANY  OTHER  PERSON.  NO  TITLE  TO AND           *
 *      OWNERSHIP  OF  THE  SOFTWARE  IS  HEREBY TRANSFERRED.           *
 *                                                                      *
 *      THE INFORMATION IN THIS SOFTWARE IS SUBJECT TO CHANGE           *
 *      WITHOUT  NOTICE  AND SHOULD  NOT BE  CONSTRUED  AS A            *
 *      COMMITMENT  BY  DIGITAL EQUIPMENT CORPORATION.                  *
 *                                                                      *
 *      DIGITAL  ASSUMES  NO RESPONSIBILITY  FOR THE  USE  OR           *
 *      RELIABILITY  OF ITS SOFTWARE ON EQUIPMENT THAT IS NOT           *
 *      SUPPLIED BY DIGITAL.                                            *
 ************************************************************************/

#include "portab.h"
#include "capdl.hc"
#include "xlate.h"
#include "trn$.hc"
#include "dbug.h"
#include "xlc_codegen.hc"
#include "xlc_init.hc"

/**********************************************************************
 * This module contains:
 * ---------------------
 *
 *  scan_items ()
 *
 **********************************************************************/


/* Declaration of structures: */


/*
 * External functions referenced:
 */

READONLY EXTERNAL BYTE
    output_sixel_in_bin(),
    output_sixel_in_hex(),
    output_sixel_as_font();


/*
 * Internal variable declarations:
 */


/*****  scan_items()  ***************************************************
 *									*
 *	Scan items list pointed to by 'item_ptr'.  Item codes dealing 	*
 *	with default font, device type, line feed, header, and double	*
 *	spacing are ignored.  An items code value = 0 terminates the	*
 *	list scanning.  Default values of page width, height, and 	*
 *	orientation are set to correspond to a North American Portrait.	*
 *	Item codes refering to page width, height, and page orientation *
 *	are accepted, and the corresponding variables are updated.  	*
 *	When finished, a match is attempted to one of eight formats,	*
 *	and a PFS code is returned as follows:				*
 *		PFS = 20 indicates Portrait, North American		*
 *		PFS = 21 indicates Landscape, North American		*
 *		PFS = 22 indicates Portrait, A4 			*
 *		PFS = 23 indicates Landscape, A4			*
 *              PFS = 24 indicates Portrait, legal                      *
 *              PFS = 25 indicates Landscape, legal
 *              PFS = 26 indicates Portrait, B                          *
 *              PFS = 27 indicates Landscape, B                         *
 *	The last-showpage flag in xl_st is set to TRUE, unless an item	*
 *	code comes in for page-fragment, in which case it will be FALSE	*
 ************************************************************************/

VOID scan_items(item_ptr)
ITEM *item_ptr;
   {
    LONG width, height, swap_buffer;
    UBYTE orient;

    /*
     * Set up defaults for a Portrait extended North American format
     */
    width = 612;  height = 792;  orient = 0;
    cg_st.cgst_item_list_sgr = 0;	/*  this is an item to override the SGR assoicated with paper 
					 *  or define one for thos that don't have any
					 */
    cg_st.cpinit_vals.initial_sgr = 0;	/* default, in case not in itemlist 
					 * this is the one that will be used whether
					 * it came from default, paper size, or item_list_sgr
					 */
    cg_st.cpinit_vals.initial_pfs = decpfstbl[20 - FIRST_PRIVATE_PFS];
    cg_st.cgst_sheet_len = 79200;

/*    cg_st.cgst_last_showpage = TRUE; Moved to xlc_main*/

    cg_st.cgst_vm_size = ONE_MEGABYTE - PREAMBLE_SIZE;
    /* cg_st.print_prologue has already been set by XLC_MAIN, so it is now valid */
    if	(cg_st.print_prologue) 
	{
	    cg_st .cgst_vm_size = ONE_MEGABYTE - PREAMBLE_SIZE;
    	}
    	else
	{
	    cg_st .cgst_vm_size = ONE_MEGABYTE;
	}
    cg_st.cgst_output_mode = trn$k_eight_bit; 		/*By default full binary enabled */
    cg_st.cgst_output_sixel_ptr = output_sixel_in_bin; 	/* by default full binary enabled */
    cg_st.cgst_image_mode = trn$k_image_readhexstring;

    /*
     * Scan the item list updating the auto-variables width, height, and
     * orient, if trn$_page_width, trn$_page_height, and trn$_page_orientation
     * items codes are found.
     */
    while (item_ptr->code != trn$_end_of_list)
       {
	switch (item_ptr->code)
           {
	    case trn$_font_SGR:
		if (	(item_ptr->address >= 10) 
		     && (item_ptr->address <= 19)
		    )
		    {
			cg_st.cgst_item_list_sgr = item_ptr->address;
		    }
		ITEM_TRACE_MACRO("(Got an SGR selection of %d) print flush\n",item_ptr->address);
		break;
	    case trn$_page_width:
		width = item_ptr->address;
		ITEM_TRACE_MACRO("(Got a page width item value of %d) print flush\n",item_ptr->address);
		break;
	    case trn$_page_orientation:
		if (item_ptr->address == trn$k_page_portrait)
			orient = 0;
		if (item_ptr->address == trn$k_page_landscape)
			orient = 1;
		ITEM_TRACE_MACRO("(Got a page orientation item value of %d) print flush\n",item_ptr->address);
		break;
	    case trn$_page_height:
		height = item_ptr->address;

		/* convert point->centipoint, and set the sheet length */
		cg_st.cgst_sheet_len = height * 100 ;
		ITEM_TRACE_MACRO("(Got a page height item value of %d) print flush\n",item_ptr->address);
		break;

/* Moved to xlc_main
**	    case trn$_page_fragment:
**    		switch (item_ptr->address) 
**		   {
**    		    case trn$k_multi_page:
**    			cg_st.cgst_last_showpage = TRUE;	break;
**    		    case trn$k_page_fragment:
**    			cg_st.cgst_last_showpage = FALSE;	break;
**    		   }
**		ITEM_TRACE_MACRO("(Got a page fragment item value of %d) print flush\n",item_ptr->address);
**		break;
*/

	    case trn$_vmsize:
    		if  (cg_st.print_prologue) 
		    {
			cg_st .cgst_vm_size = item_ptr->address - PREAMBLE_SIZE;
		    }
		else
		    {
			cg_st .cgst_vm_size = item_ptr->address;
		    }
		ITEM_TRACE_MACRO("(Got a VM size item value of %d) print flush\n",item_ptr->address);
		break;
	    case trn$_seven_bit:
		switch (item_ptr->address)
		   {
			/* Note, do not move the instruction 
			**	cg_st.cgst_output_mode = item_ptr->address;
			** out of the case statements.
			** although it is the same for all cases
			** it should only be executed if a valid case is encountered
			*/

			case trn$k_eight_bit:
				cg_st.cgst_output_mode = item_ptr->address;
				if ( cg_st.cgst_output_sixel_ptr != output_sixel_as_font )
				  cg_st.cgst_output_sixel_ptr = output_sixel_in_bin;
			/* bin vs. hex now part of image_compression case */
				break;
			case trn$k_seven_bit:
			case trn$k_gl_gr_only:
			case trn$k_gl_only:
				cg_st.cgst_output_mode = item_ptr->address;
				if ( cg_st.cgst_output_sixel_ptr != output_sixel_as_font )
				  cg_st.cgst_output_sixel_ptr = output_sixel_in_hex; 
			/* bin vs. hex now part of image_compression case */
				break;
		    }
		    
		ITEM_TRACE_MACRO("(Got a 7/8 bit item value of %d) print flush\n",item_ptr->address);
		break;

	    case trn$_image_compression:
		switch (item_ptr->address) {
			case trn$k_image_readsixelstring:	/* not implemented -
							treat as normal image */
			case trn$k_image_readhexstring:
				/* includes both hex and binary formats */
				cg_st.cgst_image_mode = item_ptr->address;
				if ( cg_st.cgst_output_mode == trn$k_eight_bit )
				        cg_st.cgst_output_sixel_ptr = output_sixel_in_bin;
				  else  cg_st.cgst_output_sixel_ptr = output_sixel_in_hex;
				break;

			case trn$k_image_sixel_font:
				cg_st.cgst_image_mode = item_ptr->address;
				cg_st.cgst_output_sixel_ptr = output_sixel_as_font; 
				/* right now output_sixel_as_font uses
					seven bit characters only;  enhance
					this test if it ever uses characters
					from the right side */
				break;
		    }
		break;
	    }
	item_ptr++;
	}

    /*
     * If landscape mode was explicitly selected on the print comand
     * then preload PFS 21 and SGR 15, then go through the normal
     * page format selection process. 
     * If landscape mode was selected, swap the usage of height and 
     * width (Change made on  1-APR-1987 13:44:19, to close
     * a hole in the interface specification.
     * The symbiont was always passing the same numbers regardless 
     * of orientation, and we were a minority in believing that 
     * height should always be height
     */
    if (orient == 1) 
       {
	swap_buffer = width;
	width = height;
	height = swap_buffer;
        cg_st.cpinit_vals.initial_sgr = 5;
	cg_st.cpinit_vals.initial_pfs = decpfstbl[21 - FIRST_PRIVATE_PFS];
       }

    /* Now go through and try to match one of the eight page formats */

    if ((width  >=  607  &&  width  <=  617) &&
	(height >=  787  &&  height <=  797) &&
	(orient == 0))
        {
    	/* Portrait, North American matched */

        cg_st.cpinit_vals.initial_sgr = 0;
	cg_st.cpinit_vals.initial_pfs = decpfstbl[20 - FIRST_PRIVATE_PFS]; 
        }
    else  if ((width  >=  787  &&  width  <=  797) &&
	(height >=  607  &&  height <=  617) &&
	(orient == 1))
        {
    	/* Landscape, North American matched */

        cg_st.cpinit_vals.initial_sgr = 5;
	cg_st.cpinit_vals.initial_pfs = decpfstbl[21 - FIRST_PRIVATE_PFS]; 
        }
    else  if ((width  >=  590  &&  width  <=  600) &&
	(height >=  837  &&  height <=  847) &&
	(orient == 0))
        {
    	/* Portrait, A4 matched */

        cg_st.cpinit_vals.initial_sgr = 6;
	cg_st.cpinit_vals.initial_pfs = decpfstbl[22 - FIRST_PRIVATE_PFS]; 
        }

    else  if ((width  >=  837  &&  width  <=  847) &&
	(height >=  590  &&  height <=  600) &&
	(orient == 1))
        {
    	/* Landscape, A4 matched */

        cg_st.cpinit_vals.initial_sgr = 5;
	cg_st.cpinit_vals.initial_pfs = decpfstbl[23 - FIRST_PRIVATE_PFS]; 
        }
   else  if ((width  >=  607 &&  width  <=  617) &&
	(height >=  1003  &&  height <=  1013) &&
	(orient == 0))
        {
    	/* Portrait, legal matched */

        cg_st.cpinit_vals.initial_sgr = 0;
	cg_st.cpinit_vals.initial_pfs = decpfstbl[24 - FIRST_PRIVATE_PFS]; 
        }
    else  if ((width  >=  1003  &&  width <= 1013) &&
	(height >=  607 &&  height <=  617 ) &&
	(orient == 1))
        {
    	/* Landscape, legal matched */

        cg_st.cpinit_vals.initial_sgr = 5;
	cg_st.cpinit_vals.initial_pfs = decpfstbl[25 - FIRST_PRIVATE_PFS]; 
    	}
    else  if ((width  >=  787  &&  width  <=  797) &&
	(height >=  1219  &&  height <=  1229) &&
	(orient == 0))
        {
    	/* Portrait, B matched */

        cg_st.cpinit_vals.initial_sgr = 0;
	cg_st.cpinit_vals.initial_pfs = decpfstbl[26 - FIRST_PRIVATE_PFS]; 
        }
    else  if ((width  >=  1219  &&  width  <=  1229) &&
	(height >=  787  &&  height <=  797) &&
	(orient == 1))
        {
    	/* Landscape, B matched */

        cg_st.cpinit_vals.initial_sgr = 5;
	cg_st.cpinit_vals.initial_pfs = decpfstbl[27 - FIRST_PRIVATE_PFS]; 
	}
    else
    /*
     * If we are here the paper size is does not have a traditional Page
     * Format Select sequence or PFS number.  Fill in the PFSBOUNDS 
     * structure in cg_st.cpinit_vals.initial_pfs by hand using the 
     * page width and page height.  Subtract 150 pixels (HALF_INCH_CTPT centipoints)
     * to get the right margin, line end, bottom margin, and page end.
     */
        {
    	cg_st.cpinit_vals.initial_pfs.pfs_lrm.min = 0;
    	cg_st.cpinit_vals.initial_pfs.pfs_lrm.max = (width * 100) - HALF_INCH_CTPT;
    	cg_st.cpinit_vals.initial_pfs.pfs_lhe.min = 0;
    	cg_st.cpinit_vals.initial_pfs.pfs_lhe.max = (width * 100) - HALF_INCH_CTPT;
    	cg_st.cpinit_vals.initial_pfs.pfs_tbm.min = 0;
    	cg_st.cpinit_vals.initial_pfs.pfs_tbm.max = (height * 100) - HALF_INCH_CTPT;
   	cg_st.cpinit_vals.initial_pfs.pfs_phe.min = 0;
   	cg_st.cpinit_vals.initial_pfs.pfs_phe.max = (height * 100) - HALF_INCH_CTPT;
    	cg_st.cpinit_vals.initial_pfs.pfs_pwid = orient ? height * 100 : width * 100;
    	cg_st.cpinit_vals.initial_pfs.pfs_plen = orient ? width * 100 : height * 100;
    	cg_st.cpinit_vals.initial_pfs.pfs_or = orient ? ORIENT_LAND : ORIENT_PORT;
    	}

   /* Let's see if the default SGR for the paper has been overriden */

    if	(cg_st.cgst_item_list_sgr !=0)
	{
	    cg_st.cpinit_vals.initial_sgr = cg_st.cgst_item_list_sgr - 10;
	}
}
