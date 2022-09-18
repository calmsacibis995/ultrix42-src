/*
 * SCCSID: @(#)externs.h	4.2	ULTRIX	1/25/91
 * Based on:
 * $Header: /sparky/a/davy/system/nfswatch/RCS/externs.h,v 3.0 91/01/23 08:23:02 davy Exp $
 *
 * externs.h - external definitons for nfswatch.
 *
 * David A. Curry				Jeffrey C. Mogul
 * SRI International				Digital Equipment Corporation
 * 333 Ravenswood Avenue			Western Research Laboratory
 * Menlo Park, CA 94025				100 Hamilton Avenue
 * davy@erg.sri.com				Palo Alto, CA 94301
 *						mogul@decwrl.dec.com
 *
 * $Log:	externs.h,v $
 * Revision 3.0  91/01/23  08:23:02  davy
 * NFSWATCH Version 3.0.
 * 
 * Revision 1.3  91/01/04  15:52:07  davy
 * New features from Jeff Mogul.
 * 
 * Revision 1.2  90/08/17  15:46:43  davy
 * NFSWATCH Version 2.0.
 * 
 * Revision 1.1  88/11/29  11:20:27  davy
 * NFSWATCH Release 1.0
 * 
 */

extern char		*pname;

extern FILE		*logfp;

extern Counter		pkt_total;
extern Counter		pkt_drops;
extern Counter		int_pkt_total;
extern Counter		int_pkt_drops;
extern Counter		dst_pkt_total;
extern Counter		int_dst_pkt_total;

extern int		errno;
extern int		if_fd;
extern int		allintf;
extern int		dstflag;
extern int		srcflag;
extern int		allflag;
extern int		logging;
extern int		learnfs;
extern int		do_update;
extern int		cycletime;
extern int		showwhich;
extern int		truncation;
extern int		sortbyusage;
extern int		nnfscounters;
extern int		nfilecounters;
extern int		nclientcounters;
extern int		screen_inited;

extern u_long		thisdst;
extern u_long		srcaddrs[];
extern u_long		dstaddrs[];

extern struct timeval	starttime;

extern char		myhost[];
extern char		srchost[];
extern char		dsthost[];

extern char		*prompt;
extern char		*logfile;
extern char		*filelist;
extern char		*snapshotfile;

extern NFSCounter	nfs_counters[];
extern FileCounter	fil_counters[];
extern PacketCounter	pkt_counters[];
extern ProcCounter	prc_counters[];
extern int		prc_countmap[];
extern ClientCounter	clnt_counters[];

char			*prtime();
char			*savestr();

int			fil_comp();
int			is_exported();
int			nfs_comp();
int			udprpc_recv();
int			want_packet();
int			prc_comp();
int			clnt_comp();

void			clear_vars();
void			command();
void			error();
void			finish();
void			flush_nit();
void			get_net_addrs();
void			icmp_filter();
void			ip_filter();
void			label_screen();
void			nd_filter();
void			nfs_count();
void			nfs_filter();
void			nfswatch();
void			pkt_filter();
void			rpc_callfilter();
void			rpc_filter();
void			rpc_replyfilter();
void			setup_fil_counters();
void			setup_nfs_counters();
void			setup_pkt_counters();
void			setup_screen();
void			setup_rpcxdr();
void			snapshot();
void			sort_nfs_counters();
void			tcp_filter();
void			udp_filter();
void			update_logfile();
void			update_screen();
void			usage();
void			wakeup();
