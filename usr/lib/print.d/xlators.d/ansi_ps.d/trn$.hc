/*        @(#)trn$.hc	4.1      7/2/90      */

/********************************************************************************************************************************/
/* Created  7-APR-1989 14:38:21 by VAX-11 SDL V3.1-7      Source:  7-APR-1989 14:37:02 DISK$IGLOO:[DCL.PS]TRN$.SDL;18 */
/********************************************************************************************************************************/
 
/*
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



/*** MODULE trn$ ***/
/*                                                                          */
/* Symbol definitions for protocol translators                              */
/*                                                                          */
/* All additions should be appended to the end of an entry or to the end of */
/* this file (but before the END_MODULE statement), so as to avoid changing  */
/* the values of any symbols previously defined and depended upon by        */
/* existing software.                                                       */
/*                                                                          */
/* Similarly, the values of any retired symbols should be marked RESERVED   */
/* rather than re-used.                                                     */
/*                                                                          */
/* Please sign and date all such modifications.                             */
/*                                                                          */
/* 31-Jan-1986 - David Larrick (DCL)                                        */
/*	Original version, corresponding to "PostScript Translator Software  */
/*	Functional Specification for PrintServer 40", Revision 2            */
/*                                                                          */
/* 19 FEB 86 - SARAH::E_TARLIN (Eliot TARLIN) / DCL                         */
/*    HCUIS/RENDER Command codes and values added                           */
/*                                                                          */
/* 22-Apr-1986 - DCL                                                        */
/*	Add page fragment mode, in preparation for releasing Revision 3 of  */
/*	the above document.                                                 */
/*                                                                          */
/* 13-Nov-1986 - KPS                                                        */
/*	Add VMSIZE item code, for use when passing the amount of available  */
/*	memory in the LN03R to the translator in module LSYM_XLATE.BLI .    */
/*                                                                          */
/* 4-Feb-1987 - DCL                                                         */
/*	Add SEVEN_BIT item code, to disable or enable generation of code    */
/*	which may set the high (eighth) bit of its output bytes.  Used by   */
/*	ANSI-to-PostScript translator in translating sixels to generate     */
/*	full-binary output for maximum speed, or hex-encoded output that    */
/*	may be MAILed, post-processed by DECpage, etc.                      */
/*                                                                          */
/* 30-Mar-1987 - DCL                                                        */
/*	Add two more item values for the SEVEN_BIT item code.  "gl_gr_only"  */
/*	instructs the translator to restrict its output to GL and GR codes,  */
/*	i.e. no C0 or C1 characters.  "gl_only" restricts output to GL      */
/*	codes, i.e. no control characters or 8th bit.                       */
/*                                                                          */
/* 24-Dec-1987 - DCL                                                        */
/*	Add ANSI_SPECIAL, a private item code for the ANSI translator.      */
/*                                                                          */
/* 15-Apr-1988 - DCL                                                        */
/*	Add IMAGE_COMPRESSION.                                              */
/*                                                                          */
/* 19-Apr-1988 - DCL                                                        */
/*	Add RESOURCE_TABLE.  The buffer address and length point to a list  */
/*	of resource identifiers.  The format of resource identifiers is TBD. */
/*                                                                          */
/* 21-Jun-1988 - DCL                                                        */
/*	Add PROLOGUE_SELECT and its associated values:                      */
/*	  - SCRIPT_AND_PROLOGUE means the translator should emit its        */
/*	    prologue (unless the contents of RESOURCE_TABLE indicate that   */
/*	    the required prologue is already loaded), and then its script.  */
/*	  - EMIT_PROLOGUE_ONLY means the translator should emit only its    */
/*	    prologue and no script, regardless of the contents of           */
/*	    RESOURCE_TABLE.                                                 */
/*	The historical default behavior is SCRIPT_AND_PROLOGUE.             */
/*                                                                          */
/* 7-Apr-1989 - DCL                                                         */
/*	Add FONT_SGR.  We now support enough paper sizes that the ANSI      */
/*	translator is having a tough time guessing which font spacing to use */
/*	(since European paper sizes require a different cpi from US sizes). */
/*	This new item moves _all_ paper-size-recognition responsibility into */
/*	the symbiont, so we don't need to modify translators every time we  */
/*	add a new paper size.                                               */
/*                                                                          */
/* Item code and item value definitions for itemlist                        */
/* Item Codes                                                               */
#define trn$_default_font 1
#define trn$_device_type 2              /* RESERVED *                       */
#define trn$_feed 3
#define trn$_header 4
#define trn$_page_orientation 5
#define trn$_page_height 6
#define trn$_page_width 7
#define trn$_space 8
#define trn$_sheet_size 9               /* Paper Sizes                      */
#define trn$_sheet_offset_height 10
#define trn$_sheet_offset_width 11
#define trn$_appearance 12              /* Picture print quality flags/masks */
#define trn$_page_fragment 13           /* Page fragment or multi-page mode */
#define trn$_vmsize 14                  /* KPS -- available VM in LN03R printer */
#define trn$_seven_bit 15               /* Generate 7- or 8-bit output      */
#define trn$_ansi_special 16            /* Private for ANSI translator      */
#define trn$_image_compression 17       /* PostScript image compression technique to use */
#define trn$_resource_table 18          /* Buffer of resource identifiers   */
#define trn$_prologue_select 19         /* Just prologue, or prologue and script? */
#define trn$_font_SGR 20                /* Which font to use, based on paper size */
/* End of the Item List code                                                */
#define trn$_end_of_list 0
/* Device Types                                                             */
#define trn$k_dev_generic 0             /* RESERVED *                       */
#define trn$k_dev_lps40 1               /* RESERVED *                       */
/* Form Feed options                                                        */
#define trn$k_feed_nofeed 0
#define trn$k_feed_feed 1
/* Page Header options                                                      */
#define trn$k_hd_noheader 0
#define trn$k_hd_header 1
/* Page Orientation values                                                  */
#define trn$k_page_portrait 0
#define trn$k_page_landscape 1
/* Line Spacing options                                                     */
#define trn$k_sp_nospace 0
#define trn$k_sp_space 1
/* Sheet Sizes                                                              */
#define trn$k_sheet_size_a 0            /*8.5 x 11 inches                   */
#define trn$k_sheet_size_a4 1           /*210 x 297 mm                      */
#define trn$k_sheet_size_b 2            /*11 x 17 inches                    */
#define trn$k_sheet_size_a3 3           /*297 x 420 mm                      */
#define trn$k_sheet_size_c 4            /*17 x 22 inches                    */
#define trn$k_sheet_size_a2 5           /*420 x 594 mm                      */
#define trn$k_sheet_size_d 6            /*22 x 34 inches                    */
#define trn$k_sheet_size_a1 7           /*594 x 841 mm                      */
#define trn$k_sheet_size_e 8            /*34 x 44 inches                    */
#define trn$k_sheet_size_a0 9           /*841 x 1189 mm                     */
#define trn$k_sheet_size_legal 10       /*8.5 x 14                          */
#define trn$k_sheet_size_lp 11          /*13.7 x 11 inches                  */
#define trn$m_draft 1
union fill_0 {
    unsigned long int trn$l_appearance;
    struct  {
        unsigned trn$v_draft : 1;       /* Speed over image quality         */
        unsigned trn$v_fill_2 : 7;
        } trn$r_fill_1;
    } ;
/* Page fragment or multi-page mode                                         */
#define trn$k_multi_page 0
#define trn$k_page_fragment 1
/* 7-bit or 8-bit output mode                                               */
#define trn$k_eight_bit 0
#define trn$k_seven_bit 1
#define trn$k_gl_gr_only 2
#define trn$k_gl_only 3
/* PostScript image compression technique to use                            */
#define trn$k_image_readhexstring 0
#define trn$k_image_sixel_font 1
#define trn$k_image_readsixelstring 2
/* Just prologue, or prologue and script?                                   */
#define trn$k_script_and_prologue 0
#define trn$k_emit_prologue_only 1
/* Which font to use, based on paper size                                   */
#define trn$K_SGR_non_metric_portrait 11
/* Which font to use, based on paper size                                   */
#define trn$K_SGR_metric_portrait 16
/* Which font to use, based on paper size                                   */
#define trn$K_SGR_landscape 15

/* ANSI Special items                                                      */
#define trn$k_noreset_entry 0
#define trn$k_reset_entry 1
#define trn$k_noreset_exit 0
#define trn$k_reset_exit 1
typedef struct {
	BYTE reset_entry;		/* True --> reset on entry */
	BYTE reset_exit;		/* True --> reset on exit  */
} TRN$K_ANSI_SPECIAL;

