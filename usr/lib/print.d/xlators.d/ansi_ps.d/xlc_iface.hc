/*        @(#)xlc_iface.hc	4.1      7/2/90      */

/*  Version 2.00   4/ 3/89 13:58:00.0000  */
/* makehf /hc /cp /setup=portab.h,capdl.def xlc_iface.def xlc_iface.hc  */





NOSHARE extern UBYTE * ibuf_loc;
NOSHARE extern UBYTE * ibuf_ptr;
NOSHARE extern WORD ibuf_len;
NOSHARE extern DEFAULT (* getr_loc) ();
NOSHARE extern DEFAULT user_g;
NOSHARE extern UBYTE * obuf_loc;
NOSHARE extern UBYTE * obuf_ptr;
NOSHARE extern WORD obuf_len;
NOSHARE extern DEFAULT (* putr_loc) ();
NOSHARE extern DEFAULT user_p;
NOSHARE extern UBYTE * obuf_sum;


extern DEFAULT ansi_input();
extern void ps_flush();
extern void ps_char();
extern void ps_schar();
extern void convert16();
extern void convert10();
extern void tconvert16();
extern void tconvert10();
extern void ps_str();
extern void old_ps_str();
extern void old_oprintf();

