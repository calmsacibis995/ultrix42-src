/*        @(#)cpparse.hc	4.1      7/2/90      */

/*  Version 2.00   4/ 3/89 13:51:00.0000  */
/* makehf /hc /cp /setup=portab.h,capdl.def,pdl_st.def,cpsys.def cpparse.def c
  ...  pparse.hc*/





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

