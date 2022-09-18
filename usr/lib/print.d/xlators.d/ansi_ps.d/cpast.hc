/*        @(#)cpast.hc	4.1      7/2/90      */

/*  Version 2.00   4/ 3/89 13:46:00.0000  */
/* makehf /hc /cp /setup=portab.h,capdl.def,pdl_st.def,cpsys.def,casys.def cpa
  ...  st.def cpast.hc*/





CONST extern SEARCH ast_esc_srch[33];
CONST extern SEARCH ast_csi_srch[55];
CONST extern SEARCH ast_dcs_srch[10];
CONST extern SEARCH ast_mode_srch[4];
CONST extern SEARCH ast_decmode_srch[7];
CONST extern SEARCH ast_tbc_srch[6];
CONST extern SEARCH ast_sgr_srch[33];
CONST extern SEARCH ast_decsgr_srch[9];
CONST extern SEARCH ast_ssu_srch[3];
CONST extern SEARCH ast_decssu_srch[2];
CONST extern SEARCH ast_jfy_srch[3];
CONST extern SEARCH ast_decjfy_srch[2];
CONST extern SEARCH ast_cmd_srch[6];
CONST extern BYTE ast_nvm_equ[2];
CONST extern WORD ast_nvm_tok[2];
CONST extern SECOND ast_gen_nvm;

/*  ** WARNING File cpast.def Line 459 -
 *  Expression error - can't evaluate [ast_nvm_tok]
 */

/*  ** WARNING File cpast.def Line 462 -
 *  Expression error - can't evaluate [ast_nvm_equ]
 */
CONST extern FIRST ast_nvm;
CONST extern BYTE ast_gen_c0_equ[13];
CONST extern WORD ast_gen_c0_tok[13];
CONST extern SECOND ast_gen_c0;

/*  ** WARNING File cpast.def Line 531 -
 *  Expression error - can't evaluate [ast_gen_c0_tok]
 */

/*  ** WARNING File cpast.def Line 534 -
 *  Expression error - can't evaluate [ast_gen_c0_equ]
 */
CONST extern BYTE ast_gen_c1_equ[16];
CONST extern WORD ast_gen_c1_tok[16];
CONST extern SECOND ast_gen_c1;

/*  ** WARNING File cpast.def Line 591 -
 *  Expression error - can't evaluate [ast_gen_c1_tok]
 */

/*  ** WARNING File cpast.def Line 594 -
 *  Expression error - can't evaluate [ast_gen_c1_equ]
 */
CONST extern BYTE ast_text_gl_equ[3];
CONST extern WORD ast_text_gl_tok[3];
CONST extern SECOND ast_text_gl;

/*  ** WARNING File cpast.def Line 626 -
 *  Expression error - can't evaluate [ast_text_gl_tok]
 */

/*  ** WARNING File cpast.def Line 629 -
 *  Expression error - can't evaluate [ast_text_gl_equ]
 */
CONST extern BYTE ast_text_gr_equ[3];
CONST extern WORD ast_text_gr_tok[3];
CONST extern SECOND ast_text_gr;

/*  ** WARNING File cpast.def Line 659 -
 *  Expression error - can't evaluate [ast_text_gr_tok]
 */

/*  ** WARNING File cpast.def Line 662 -
 *  Expression error - can't evaluate [ast_text_gr_equ]
 */
CONST extern WORD astx_text[256];
CONST extern WORD astx_fast_text_gl[256];
CONST extern WORD astx_fast_text_gr[256];
CONST extern FIRST ast_text;
CONST extern FIRST ast_fast_text_gl;
CONST extern FIRST ast_fast_text_gr;
CONST extern BYTE ast_esc_gl_equ[9];
CONST extern WORD ast_esc_gl_tok[9];
CONST extern SECOND ast_esc_gl;

/*  ** WARNING File cpast.def Line 1114 -
 *  Expression error - can't evaluate [ast_esc_gl_tok]
 */

/*  ** WARNING File cpast.def Line 1117 -
 *  Expression error - can't evaluate [ast_esc_gl_equ]
 */
CONST extern FIRST ast_esc;

/*  ** WARNING File cpast.def Line 1126 -
 *  Expression error - can't evaluate [ast_esc_srch]
 */
CONST extern BYTE ast_esc2_gl_equ[3];
CONST extern WORD ast_esc2_gl_tok[3];
CONST extern SECOND ast_esc2_gl;

/*  ** WARNING File cpast.def Line 1165 -
 *  Expression error - can't evaluate [ast_esc2_gl_tok]
 */

/*  ** WARNING File cpast.def Line 1168 -
 *  Expression error - can't evaluate [ast_esc2_gl_equ]
 */
CONST extern FIRST ast_esc2;

/*  ** WARNING File cpast.def Line 1177 -
 *  Expression error - can't evaluate [ast_esc_srch]
 */
CONST extern BYTE ast_escinv_gl_equ[3];
CONST extern WORD ast_escinv_gl_tok[3];
CONST extern SECOND ast_escinv_gl;

/*  ** WARNING File cpast.def Line 1216 -
 *  Expression error - can't evaluate [ast_escinv_gl_tok]
 */

/*  ** WARNING File cpast.def Line 1219 -
 *  Expression error - can't evaluate [ast_escinv_gl_equ]
 */
CONST extern FIRST ast_escinv;
CONST extern BYTE ast_scs_gl_equ[3];
CONST extern WORD ast_scs_gl_tok[3];
CONST extern SECOND ast_scs_gl;

/*  ** WARNING File cpast.def Line 1267 -
 *  Expression error - can't evaluate [ast_scs_gl_tok]
 */

/*  ** WARNING File cpast.def Line 1270 -
 *  Expression error - can't evaluate [ast_scs_gl_equ]
 */
CONST extern FIRST ast_scs;
CONST extern BYTE ast_csi_gl_equ[7];
CONST extern WORD ast_csi_gl_tok[7];
CONST extern SECOND ast_csi_gl;

/*  ** WARNING File cpast.def Line 1326 -
 *  Expression error - can't evaluate [ast_csi_gl_tok]
 */

/*  ** WARNING File cpast.def Line 1329 -
 *  Expression error - can't evaluate [ast_csi_gl_equ]
 */
CONST extern FIRST ast_csi;

/*  ** WARNING File cpast.def Line 1338 -
 *  Expression error - can't evaluate [ast_csi_srch]
 */
CONST extern BYTE ast_csip_gl_equ[6];
CONST extern WORD ast_csip_gl_tok[6];
CONST extern SECOND ast_csip_gl;

/*  ** WARNING File cpast.def Line 1383 -
 *  Expression error - can't evaluate [ast_csip_gl_tok]
 */

/*  ** WARNING File cpast.def Line 1386 -
 *  Expression error - can't evaluate [ast_csip_gl_equ]
 */
CONST extern FIRST ast_csip;

/*  ** WARNING File cpast.def Line 1395 -
 *  Expression error - can't evaluate [ast_csi_srch]
 */
CONST extern BYTE ast_csii_gl_equ[4];
CONST extern WORD ast_csii_gl_tok[4];
CONST extern SECOND ast_csii_gl;

/*  ** WARNING File cpast.def Line 1436 -
 *  Expression error - can't evaluate [ast_csii_gl_tok]
 */

/*  ** WARNING File cpast.def Line 1439 -
 *  Expression error - can't evaluate [ast_csii_gl_equ]
 */
CONST extern FIRST ast_csii;

/*  ** WARNING File cpast.def Line 1448 -
 *  Expression error - can't evaluate [ast_csi_srch]
 */
CONST extern BYTE ast_csiinv_gl_equ[3];
CONST extern WORD ast_csiinv_gl_tok[3];
CONST extern SECOND ast_csiinv_gl;

/*  ** WARNING File cpast.def Line 1487 -
 *  Expression error - can't evaluate [ast_csiinv_gl_tok]
 */

/*  ** WARNING File cpast.def Line 1490 -
 *  Expression error - can't evaluate [ast_csiinv_gl_equ]
 */
CONST extern FIRST ast_csiinv;
CONST extern BYTE ast_crm_c0_equ[4];
CONST extern WORD ast_crm_c0_tok[4];
CONST extern SECOND ast_crm_c0;

/*  ** WARNING File cpast.def Line 1540 -
 *  Expression error - can't evaluate [ast_crm_c0_tok]
 */

/*  ** WARNING File cpast.def Line 1543 -
 *  Expression error - can't evaluate [ast_crm_c0_equ]
 */
CONST extern BYTE ast_crm_c1_equ[2];
CONST extern WORD ast_crm_c1_tok[2];
CONST extern SECOND ast_crm_c1;

/*  ** WARNING File cpast.def Line 1572 -
 *  Expression error - can't evaluate [ast_crm_c1_tok]
 */

/*  ** WARNING File cpast.def Line 1575 -
 *  Expression error - can't evaluate [ast_crm_c1_equ]
 */
CONST extern BYTE ast_crm_gl_equ[3];
CONST extern WORD ast_crm_gl_tok[3];
CONST extern SECOND ast_crm_gl;

/*  ** WARNING File cpast.def Line 1607 -
 *  Expression error - can't evaluate [ast_crm_gl_tok]
 */

/*  ** WARNING File cpast.def Line 1610 -
 *  Expression error - can't evaluate [ast_crm_gl_equ]
 */
CONST extern FIRST ast_crm;
CONST extern BYTE ast_crm2_gl_equ[5];
CONST extern WORD ast_crm2_gl_tok[5];
CONST extern SECOND ast_crm2_gl;

/*  ** WARNING File cpast.def Line 1663 -
 *  Expression error - can't evaluate [ast_crm2_gl_tok]
 */

/*  ** WARNING File cpast.def Line 1666 -
 *  Expression error - can't evaluate [ast_crm2_gl_equ]
 */
CONST extern FIRST ast_crm2;
CONST extern BYTE ast_crm3_gl_equ[8];
CONST extern WORD ast_crm3_gl_tok[8];
CONST extern SECOND ast_crm3_gl;

/*  ** WARNING File cpast.def Line 1725 -
 *  Expression error - can't evaluate [ast_crm3_gl_tok]
 */

/*  ** WARNING File cpast.def Line 1728 -
 *  Expression error - can't evaluate [ast_crm3_gl_equ]
 */
CONST extern FIRST ast_crm3;
CONST extern BYTE ast_crm4_gl_equ[4];
CONST extern WORD ast_crm4_gl_tok[4];
CONST extern SECOND ast_crm4_gl;

/*  ** WARNING File cpast.def Line 1779 -
 *  Expression error - can't evaluate [ast_crm4_gl_tok]
 */

/*  ** WARNING File cpast.def Line 1782 -
 *  Expression error - can't evaluate [ast_crm4_gl_equ]
 */
CONST extern FIRST ast_crm4;
CONST extern BYTE ast_dcs_gl_equ[6];
CONST extern WORD ast_dcs_gl_tok[6];
CONST extern SECOND ast_dcs_gl;

/*  ** WARNING File cpast.def Line 1838 -
 *  Expression error - can't evaluate [ast_dcs_gl_tok]
 */

/*  ** WARNING File cpast.def Line 1841 -
 *  Expression error - can't evaluate [ast_dcs_gl_equ]
 */
CONST extern FIRST ast_dcs;

/*  ** WARNING File cpast.def Line 1850 -
 *  Expression error - can't evaluate [ast_dcs_srch]
 */
CONST extern BYTE ast_dcsp_gl_equ[6];
CONST extern WORD ast_dcsp_gl_tok[6];
CONST extern SECOND ast_dcsp_gl;

/*  ** WARNING File cpast.def Line 1895 -
 *  Expression error - can't evaluate [ast_dcsp_gl_tok]
 */

/*  ** WARNING File cpast.def Line 1898 -
 *  Expression error - can't evaluate [ast_dcsp_gl_equ]
 */
CONST extern FIRST ast_dcsp;

/*  ** WARNING File cpast.def Line 1907 -
 *  Expression error - can't evaluate [ast_dcs_srch]
 */
CONST extern BYTE ast_dcsi_gl_equ[4];
CONST extern WORD ast_dcsi_gl_tok[4];
CONST extern SECOND ast_dcsi_gl;

/*  ** WARNING File cpast.def Line 1948 -
 *  Expression error - can't evaluate [ast_dcsi_gl_tok]
 */

/*  ** WARNING File cpast.def Line 1951 -
 *  Expression error - can't evaluate [ast_dcsi_gl_equ]
 */
CONST extern FIRST ast_dcsi;

/*  ** WARNING File cpast.def Line 1960 -
 *  Expression error - can't evaluate [ast_dcs_srch]
 */
CONST extern BYTE ast_sxl_c0_equ[7];
CONST extern WORD ast_sxl_c0_tok[7];
CONST extern SECOND ast_sxl_c0;

/*  ** WARNING File cpast.def Line 2007 -
 *  Expression error - can't evaluate [ast_sxl_c0_tok]
 */

/*  ** WARNING File cpast.def Line 2010 -
 *  Expression error - can't evaluate [ast_sxl_c0_equ]
 */
CONST extern BYTE ast_dcs_c0_equ[7];
CONST extern WORD ast_dcs_c0_tok[7];
CONST extern SECOND ast_dcs_c0;

/*  ** WARNING File cpast.def Line 2049 -
 *  Expression error - can't evaluate [ast_dcs_c0_tok]
 */

/*  ** WARNING File cpast.def Line 2052 -
 *  Expression error - can't evaluate [ast_dcs_c0_equ]
 */
CONST extern BYTE ast_dcsignore_gl_equ[1];
CONST extern WORD ast_dcsignore_gl_tok[1];
CONST extern SECOND ast_dcsignore_gl;

/*  ** WARNING File cpast.def Line 2080 -
 *  Expression error - can't evaluate [ast_dcsignore_gl_tok]
 */

/*  ** WARNING File cpast.def Line 2083 -
 *  Expression error - can't evaluate [ast_dcsignore_gl_equ]
 */
CONST extern FIRST ast_dcsignore;
CONST extern FIRST ast_sxlignore;
CONST extern FIRST ast_dcsinv;
CONST extern BYTE ast_graph_gl_equ[11];
CONST extern WORD ast_graph_gl_tok[11];
CONST extern SECOND ast_graph_gl;

/*  ** WARNING File cpast.def Line 2188 -
 *  Expression error - can't evaluate [ast_graph_gl_tok]
 */

/*  ** WARNING File cpast.def Line 2191 -
 *  Expression error - can't evaluate [ast_graph_gl_equ]
 */
CONST extern FIRST ast_graph;
CONST extern BYTE ast_graphsxl_gl_equ[11];
CONST extern WORD ast_graphsxl_gl_tok[11];
CONST extern SECOND ast_graphsxl_gl;

/*  ** WARNING File cpast.def Line 2257 -
 *  Expression error - can't evaluate [ast_graphsxl_gl_tok]
 */

/*  ** WARNING File cpast.def Line 2260 -
 *  Expression error - can't evaluate [ast_graphsxl_gl_equ]
 */
CONST extern WORD astx_graphsxl[256];
CONST extern FIRST ast_graphsxl;
CONST extern BYTE ast_graphcmd1_gl_equ[10];
CONST extern WORD ast_graphcmd1_gl_tok[10];
CONST extern SECOND ast_graphcmd1_gl;

/*  ** WARNING File cpast.def Line 2439 -
 *  Expression error - can't evaluate [ast_graphcmd1_gl_tok]
 */

/*  ** WARNING File cpast.def Line 2442 -
 *  Expression error - can't evaluate [ast_graphcmd1_gl_equ]
 */
CONST extern FIRST ast_graphcmd1;
CONST extern BYTE ast_graphcmd2_gl_equ[10];
CONST extern WORD ast_graphcmd2_gl_tok[10];
CONST extern SECOND ast_graphcmd2_gl;

/*  ** WARNING File cpast.def Line 2507 -
 *  Expression error - can't evaluate [ast_graphcmd2_gl_tok]
 */

/*  ** WARNING File cpast.def Line 2510 -
 *  Expression error - can't evaluate [ast_graphcmd2_gl_equ]
 */
CONST extern FIRST ast_graphcmd2;
CONST extern BYTE ast_lff_gl_equ[6];
CONST extern WORD ast_lff_gl_tok[6];
CONST extern SECOND ast_lff_gl;

/*  ** WARNING File cpast.def Line 2564 -
 *  Expression error - can't evaluate [ast_lff_gl_tok]
 */

/*  ** WARNING File cpast.def Line 2567 -
 *  Expression error - can't evaluate [ast_lff_gl_equ]
 */
CONST extern WORD astx_lff[256];
CONST extern FIRST ast_lff;
CONST extern BYTE ast_lffrpt_gl_equ[9];
CONST extern WORD ast_lffrpt_gl_tok[9];
CONST extern SECOND ast_lffrpt_gl;

/*  ** WARNING File cpast.def Line 2731 -
 *  Expression error - can't evaluate [ast_lffrpt_gl_tok]
 */

/*  ** WARNING File cpast.def Line 2734 -
 *  Expression error - can't evaluate [ast_lffrpt_gl_equ]
 */
CONST extern FIRST ast_lffrpt;
CONST extern BYTE ast_ansb_gl_equ[8];
CONST extern WORD ast_ansb_gl_tok[8];
CONST extern SECOND ast_ansb_gl;

/*  ** WARNING File cpast.def Line 2792 -
 *  Expression error - can't evaluate [ast_ansb_gl_tok]
 */

/*  ** WARNING File cpast.def Line 2795 -
 *  Expression error - can't evaluate [ast_ansb_gl_equ]
 */
CONST extern FIRST ast_ansb;
CONST extern BYTE ast_atff_gl_equ[6];
CONST extern WORD ast_atff_gl_tok[6];
CONST extern SECOND ast_atff_gl;

/*  ** WARNING File cpast.def Line 2849 -
 *  Expression error - can't evaluate [ast_atff_gl_tok]
 */

/*  ** WARNING File cpast.def Line 2852 -
 *  Expression error - can't evaluate [ast_atff_gl_equ]
 */
CONST extern FIRST ast_atff;
CONST extern BYTE ast_dtff_gl_equ[6];
CONST extern WORD ast_dtff_gl_tok[6];
CONST extern SECOND ast_dtff_gl;

/*  ** WARNING File cpast.def Line 2906 -
 *  Expression error - can't evaluate [ast_dtff_gl_tok]
 */

/*  ** WARNING File cpast.def Line 2909 -
 *  Expression error - can't evaluate [ast_dtff_gl_equ]
 */
CONST extern FIRST ast_dtff;
CONST extern BYTE ast_aupss_gl_equ[2];
CONST extern WORD ast_aupss_gl_tok[2];
CONST extern SECOND ast_aupss_gl;

/*  ** WARNING File cpast.def Line 2955 -
 *  Expression error - can't evaluate [ast_aupss_gl_tok]
 */

/*  ** WARNING File cpast.def Line 2958 -
 *  Expression error - can't evaluate [ast_aupss_gl_equ]
 */
CONST extern FIRST ast_aupss;
CONST extern BYTE ast_mac_gl_equ[6];
CONST extern WORD ast_mac_gl_tok[6];
CONST extern SECOND ast_mac_gl;

/*  ** WARNING File cpast.def Line 3012 -
 *  Expression error - can't evaluate [ast_mac_gl_tok]
 */

/*  ** WARNING File cpast.def Line 3015 -
 *  Expression error - can't evaluate [ast_mac_gl_equ]
 */
CONST extern FIRST ast_mac;
CONST extern BYTE ast_macrpt_gl_equ[9];
CONST extern WORD ast_macrpt_gl_tok[9];
CONST extern SECOND ast_macrpt_gl;

/*  ** WARNING File cpast.def Line 3075 -
 *  Expression error - can't evaluate [ast_macrpt_gl_tok]
 */

/*  ** WARNING File cpast.def Line 3078 -
 *  Expression error - can't evaluate [ast_macrpt_gl_equ]
 */
CONST extern FIRST ast_macrpt;
CONST extern FIRST ast_keyboard;
CONST extern FIRST ast_macro;

