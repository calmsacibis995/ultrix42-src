/*        @(#)xlm_io.hc	4.1      7/2/90      */
#define M$CHECK_BUFF_OF() \
	/* \
	 * Check for a buffer overflow or buffer full \
	 */ \
	if (obuf_ptr >= obuf_sum) { \
	    obuf_len = obuf_ptr - obuf_loc; \
	    (*putr_loc) (&obuf_len, &obuf_loc, user_p); \
	    obuf_ptr = obuf_loc; \
	    obuf_sum = obuf_loc + obuf_len; \
	} 

#define M$PS_CHAR(c) \
    /*\
     * Stuff the character into the output buffer\
     */\
    *obuf_ptr++ = c;\
\
    /*\
     * Check for a buffer overflow or buffer full\
     */\
    M$CHECK_BUFF_OF();

/* this macro is only valid for the ANSI xlator.  It is here since this
** file is 'shared'.
*/
#define M$PS_SCHAR(c) \
	if 	(cg_st.cgst_output_mode == trn$k_eight_bit)\
	    {\
	    if ( (c == '\\') || (c == '(') || (c == ')') )\
		{\
		M$PS_CHAR ('\\');\
		};\
\
	    M$PS_CHAR (c);\
\
	    }\
	else\
	    {\
	        ps_schar(c);\
	    } ;

