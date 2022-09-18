/*        @(#)cpglobal.hc	4.1      7/2/90      */

/*  Version 2.00   4/ 3/89 13:51:00.0000  */
/* makehf /hc /cp /setup=portab.h,cpsys.def cpglobal.def cpglobal.hc  */





extern void cp_add_to_cur_param();
extern void cp_start_new_param();
extern void cp_add_to_esc_ibuf();
extern void cp_add_to_csi_ibuf();
extern void cp_add_to_dcs_ibuf();
extern void cp_set_private();
extern void cp_exit_cur_mode();
extern void cp_cond_exit_crm();
extern void cp_cond_exit_crm_ris();
extern void cp_cond_exit_crm_str();
extern void cp_switch();


NOSHARE extern UWORD cp_pif_word;
NOSHARE extern PAS cp_search_tab_ptr;


extern void cp_parse();
extern void cp_getchar();
extern void cp_split();
extern void cp_search_a_tab();
extern WORD cp_search_2tab();
extern void cp_text_search();
extern void cp_cmd_search();
extern void cp_search_cur_tab();
extern void cp_store_cmd();
extern void cp_do_cur_cmd();
extern void cp_do_store_cmd();
extern void cp_font_name();
extern void cp_setioptr();
extern void cp_setctptr();
extern void cp_init();
extern void cp_reset();

