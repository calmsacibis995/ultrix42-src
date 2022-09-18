/*	@(#)evl.h	4.1	7/2/90	*/

/*
 *
 * Copyright (C) 1985 by
 * Digital Equipment Corporation, Maynard, Mass.
 *
 * This software is furnished under a license and may be used and copied
 * only  in  accordance  with  the  terms  of such  license and with the
 * inclusion of the above copyright notice. This software or  any  other
 * copies thereof may not be provided or otherwise made available to any
 * other person. No title to and ownership of  the  software  is  hereby
 * transferred.
 *
 * The information in this software is subject to change without  notice
 * and  should  not be  construed  as  a commitment by Digital Equipment
 * Corporation.
 *
 * Digital assumes no responsibility for the use or  reliability  of its
 * software on equipment which is not supplied by Digital.
 *
 *
 * Networks & Communications Software Engineering
 *
 * IDENT HISTORY:
 *
 * 1.00 10-Jul-1985
 *      DECnet-ULTRIX   V1.0
 *
 */

/* define maximums used in EVL */
#define MAX_NICE 255
#define MAX_OUT 1500
#define MAX_ENTLEN 16
#define MAX_FLDLEN 80
#define MAX_CHARS 64
#define MAX_CODED 31
#define MAX_NNAM 6
#define MAX_RDAT 128
#define MAX_EVENTS 5
#define MAX_NODES 6
#define MAX_FILTERS 128
#define MAX_SINK_OPTS 32
#define MAX_ERRORS 30
#define MAX_ERROR_SIZE 50
#define MAX_USR_LOGS 5
#define MAX_SINK_NAME 40

/* define bit masks used by EVL */
#define M_BYTE 0377
#define M_SHORT 0177777
#define M_TYP 037
#define M_CLS 0777
#define M_AREA 077
#define M_NODE 01777
#define M_CTR 0100000
#define M_PARTYP 07777
#define M_CODED 0200
#define M_FLDLEN 017
#define M_DATYP 060
#define M_NUM 017
#define M_DU 000
#define M_HEXNUM 040
#define M_CODMLT 0100
#define M_ASCIM 0100
#define M_CFLDLN 077
#define M_CTRLEN 060000
#define M_BITMAP 010000

/* define field lengths */
#define SHORTLEN 30
#define MEDLEN 50
#define LONGLEN 60

/* define constants */
#define NODE_ENT 0
#define LINE_ENT 1
#define CIRCUIT_ENT 3
#define MODULE_ENT 4
#define AREA_ENT 5
#define EVL_SEND 1024
#define EVL_RECV 1024
#define ENT_LEN 16
#define DAT_LEN 60
#define SINK_CONSOLE 1
#define SINK_FILE 2
#define SINK_MONITOR 4
#define LL_WAIT 1
#define LL_OLDVER 2
#define LL_CONSOLE 4
#define LL_FILE 8
#define LL_MONITOR 16

#define UNSIGN_PAR 0
#define SIGNED_PAR 1
#define HEXNUM_PAR 2
#define OCTAL_PAR 3

#define EMPTY -1

#define FMT_NODE 1
#define FMT_COUNTER 2
#define FMT_EADDR 3

#define TMO_INTVL 10         /* remote link signal intervals */
#define TMO_COUNT 3          /* number of intervals to wait for event */

#define no_eintr(x) while ((x) == -1 && errno == EINTR)
#define alloc(x) (struct x *)malloc(sizeof(struct x))

/* structure definition */
struct evt_class {
   short e_class;
   struct evt_type *e_typ_ptr;
   struct evt_param *e_par_ptr;
   struct evt_class *e_class_lnk;
};

struct evt_type {
   short e_type;
   char e_text[MEDLEN];
   struct evt_type *e_type_lnk;
};

struct evt_param {
   short e_param_type;
   short e_param_fmt;
   char e_kwd[SHORTLEN];
   struct evt_param *e_param_lnk;
   struct evt_param_kwd *e_param_ptr;
};

struct evt_param_kwd {
   short e_kwd_value;
   char e_kwd_range[1];
   char e_kwd_text[LONGLEN];
   struct evt_param_kwd *e_kwd_lnk;
   struct evt_err_det *e_err_ptr;
};

struct evt_err_det {
   short e_err_value;
   char e_err_text[SHORTLEN];
   struct evt_err_det *e_err_lnk;
};

struct evt_ctr {
   short e_ctr_value;
   char e_ctr_text[MEDLEN];
   struct evt_ctr *e_ctr_lnk;
   struct evt_bm *e_ctr_bm;
};

struct evt_bm {
   short e_bm_value;
   char e_bm_text[MEDLEN];
   struct evt_bm *e_bm_lnk;
};

struct event_log {
   short e_ecls;
   short e_etyp;
   short e_jul_half_day;
   short e_seconds;
   short e_millis;
   short e_area_numb;
   short e_node_numb;
   char e_node_name[MAX_NNAM+1];
   short e_ent_ty;
   short e_ent_id;
   short e_ent_aid;
   short e_ent_nid;
   char e_ent_nnam[MAX_NNAM+1];
   char e_ent_id_txt[MAX_ENTLEN];
   short e_par_typ;
   short e_par_len;
   char e_par_bin[MAX_FLDLEN];
   short e_par_fmt;
   short e_par_cmf;
   short e_ctr_bm;
};

struct log_links {
   short l_node;
   short l_tmo;
   short l_flags;
   int l_sock;
   struct log_links *l_timer_lnk;
};


struct evt_que {
   char q_nice[MAX_RDAT];
   int q_cnt;
   struct log_links *q_cur_node;
   struct evt_que *q_evt_lnk;
};

struct sinks {
    short s_flags;
    short s_node;
};

struct event {
    short e_class;
    short e_type;
    short e_ent_type;
    short e_ent_num;
    char e_ent_id[ENT_LEN];
    short e_data_len;
    char e_data[DAT_LEN];
    long e_time;
    short e_addr;
}; 

