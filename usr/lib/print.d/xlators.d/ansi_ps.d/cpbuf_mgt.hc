/*        @(#)cpbuf_mgt.hc	4.1      7/2/90      */

/*  Version 2.00   4/ 3/89 13:50:00.0000  */
/* makehf /hc /cp /setup=portab.h,capdl.def,pdl_st.def,cpsys.def cpbuf_mgt.def
  ...   cpbuf_mgt.hc*/





extern void cp_add_to_cur_param();
extern void cp_start_new_param();
extern void cp_add_to_esc_ibuf();
extern void cp_add_to_csi_ibuf();
extern void cp_add_to_dcs_ibuf();
extern void cp_set_private();

