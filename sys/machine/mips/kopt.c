/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */

#include "../h/param.h"
#include "../machine/debug.h"

/*
 * 13-Oct-89 -- gmm
 * 	Removed led_period (along with smp changes for swtch() etc
 */
extern int xprsize;
extern unsigned xpr_flags;
extern int askme;

extern int nohash;
extern int memlimit;
extern int bufpages, nbuf, nswbuf;
extern int noctricks;
#ifdef oldmips
extern int causeecc, doecc();
extern int eccdelay;
#endif oldmips
extern int noklust, nohash, nofodklust;
#ifndef SABLE
#ifdef oldmips
extern int scatgath;
extern int disk_cache;
extern int ovlapseeks;
#endif oldmips
#endif !SABLE
#ifdef NFSDEBUG
extern int nfsdebug;
#endif NFSDEBUG
#ifdef RPCDEBUG
extern int rpcdebug;
#endif RPCDEBUG
extern int lotsfree;
extern int desfree;
extern int minfree;
extern int fastscan;
extern int slowscan;
extern int klin;
extern int klout;
extern int kltxt;
extern int klseql;
#ifdef ultrix
int handspread;
#else !ultrix
extern int handspread;
#endif ultrix
extern int saferss;
extern int maxpgio;
extern int maxslp;
extern int usermsg;
#ifdef ultrix
size_t pgthresh;
int nc_doingcache;		/* Name Cache On (1) or Off (0) */
#else !ultrix
extern size_t pgthresh;
extern int nc_doingcache;		/* Name Cache On (1) or Off (0) */
#endif ultrix
extern int ncsize;			/* size of the Name Cache */
#ifdef PROFILING
extern int profiling;
#endif PROFILING
#ifdef USE_IDLE
extern int v_zeros;
extern int v_zero_pg_hits;
extern int v_zero_pg_misses;
extern int v_zero_pt_hits;
extern int v_zero_pt_misses;
#endif
#ifdef djl
extern int class_hits;
extern int class_misses;
extern int class_ends;
extern int class_tries;
#endif

/*
 * The following is a table of symbolic names and addresses of kernel
 * variables which can be tuned to alter the performance of the system.
 * They can be modified at boot time as a boot parameter or by the mipskopt
 * system call.  Variables marked as readonly can't be modifed after system
 * boot time (i.e. through the mipskopt call).  "func" is called after the
 * variable is set in case there is processing beyond storing the new value.
 */

struct kernargs kernargs[] = {
	{ "xpr_flags",	(int *)&xpr_flags,	0,	NULL },
	{ "xprsize",	&xprsize,		1,	NULL },
	{ "askme",	&askme,			0,	NULL },
	{ "bufpages",	&bufpages,		1,	NULL },
	{ "nbuf",	&nbuf,			1,	NULL },
	{ "nswbuf",	&nswbuf,		1,	NULL },
	{ "memlimit",	&memlimit,		1,	NULL },
	{ "noctricks",	&noctricks,		0,	NULL },
#ifdef oldmips
	{ "causeecc",	&causeecc,		0,	doecc },
	{ "eccdelay",	&eccdelay,		0,	NULL },
#endif oldmips
	{ "nohash",	&nohash,		0,	NULL },
	{ "noklust",	&noklust,		0,	NULL },
	{ "nofodklust",	&nofodklust,		0,	NULL },
#ifndef SABLE
#ifdef oldmips
	{ "scatgath",	&scatgath,		0,	NULL },
	{ "disk_cache",	&disk_cache,		0,	NULL },
	{ "ovlapseeks",	&ovlapseeks,		0,	NULL },
#endif oldmips
#endif !SABLE
#ifdef NFSDEBUG
	{ "nfsdebug",	&nfsdebug,		0,	NULL },
#endif NFSDEBUG
#ifdef RPCDEBUG
	{ "rpcdebug",	&rpcdebug,		0,	NULL },
#endif RPCDEBUG
	{ "lotsfree",	&lotsfree,		0,	NULL },
	{ "desfree",	&desfree,		0,	NULL },
	{ "minfree",	&minfree,		0,	NULL },
	{ "fastscan",	&fastscan,		0,	NULL },
	{ "slowscan",	&slowscan,		0,	NULL },
	{ "klin",	&klin,			0,	NULL },
	{ "klout",	&klout,			0,	NULL },
	{ "kltxt",	&kltxt,			0,	NULL },
	{ "klseql",	&klseql,		0,	NULL },
	{ "handspread",	&handspread,		1,	NULL },
	{ "saferss",	&saferss,		0,	NULL },
	{ "maxpgio",	&maxpgio,		0,	NULL },
	{ "maxslp",	&maxslp,		0,	NULL },
	{ "usermsg",	&usermsg,		0,	NULL },
	{ "pgthresh",	&pgthresh,		0,	NULL },
#ifdef PROFILING
 	{ "profiling",	&profiling,		0,	NULL },
#endif PROFILING
	{ "nc_doingcache", &nc_doingcache,	0,	NULL },
	{ "ncsize",	&ncsize,		1,	NULL },
#ifdef USE_IDLE
	{ "v_zeros",	&v_zeros,		1,	NULL },
	{ "vpgh",	&v_zero_pg_hits,	1,	NULL },
	{ "vpgm",	&v_zero_pg_misses,	1,	NULL },
	{ "vpth",	&v_zero_pt_hits,	1,	NULL },
	{ "vptm",	&v_zero_pt_misses,		1,	NULL },
#endif USE_IDLE
#ifdef djl
	{ "class_hits",	&class_hits,		0,	NULL},
	{ "class_misses",&class_misses,		0,	NULL},
	{ "class_ends",&class_ends,		0,	NULL},
	{ "class_tries",&class_tries,		0,	NULL},
#endif djl
	{ NULL,		NULL,			1,	NULL }
};
