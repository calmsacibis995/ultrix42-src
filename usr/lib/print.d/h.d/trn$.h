#ifdef ultrix
#ifdef lint
static char *sccsid = "@(#)trn$.h	4.1	(ULTRIX)	7/2/90";
#endif lint
#endif ultrix
/********************************************************************************************************************************/
/* Created 30-MAR-1987 12:28:54 by VAX-11 SDL V3.0-2      Source: 30-MAR-1987 12:17:38 USER$B:[LIBRYFW.PSAT.JORRY]TRN$.SDL;13 */
/********************************************************************************************************************************/
 
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
