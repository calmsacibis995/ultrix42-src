/*	@(#)ns.h	4.1	(ULTRIX)	7/2/90	*/

/************************************************************************
 *									*
 *			Copyright (c) 1984-1988 by			*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any  other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   This software is  derived  from  software  received  from  the	*
 *   University    of   California,   Berkeley,   and   from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is  subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/
/*
 * Copyright (c) 1985 Regents of the University of California
 *	All Rights Reserved
 *	@(#)ns.h	4.21 (Berkeley) 2/28/88
 */

/*
 * Modification History:
 *
 * 18-Jan-88	logcher
 *	Added BIND 4.7.3.
 *
 * 17-May-89	logcher
 *	Added BIND 4.8.
 */

/*
 * Global definitions and variables for the name server.
 */

#include <strings.h>
#include <arpa/inet.h>
#ifdef AUTHEN
#include <krb.h>
#endif AUTHEN

/*
 * Timeout time should be around 1 minute or so.  Using the
 * the current simplistic backoff strategy, the sequence
 * retrys after 4, 8, and 16 seconds.  With 3 servers, this
 * dies out in a little more than a minute.
 * (sequence RETRYBASE, 2*RETRYBASE, 4*RETRYBASE... for MAXRETRY)
 */
#define MAXZONES	128		/* max number of zones supported */
#define MINROOTS	2	 	/* min number of root hints */
#define NSMAX		10		/* max number of NS's to try */
#define RETRYBASE	4 		/* base time between retries */
#define MAXRETRY	3		/* max number of retries per addr */
#define MAXCNAMES	8		/* max # of CNAMES tried per addr */
#define MAXQUERIES	20		/* max # of queries to be made */
					/* (prevent "recursive" loops) */
#define	INIT_REFRESH	600		/* retry time for initial secondary */
					/* contact (10 minutes) */

#define ALPHA    0.7	/* How much to preserver of old response time */
#define	BETA	 1.2	/* How much to penalize response time on failure */
#define	GAMMA	 0.98	/* How much to decay unused response times */


#ifdef ULTRIXFUNC 

#define MAX_FILE_SIZE 1024

struct files_st {
	char f_name[MAX_FILE_SIZE];		/* name of the file */
	struct stat f_stats;			/* The modification time of
						   file file. */
	struct files_st *next;
};
typedef struct files_st files_st;

/*
  Each file_ref_st describes the state of the load of one file
  in the list of files which define a zone.  A list of file_ref_st defines
  the current state of a load by describing the state of the load of all
  files currently being referenced.  The list of file_ref_st encapsulates
  all of the state of a load formerly kept on the stack.  So, the load of
  a zone can be interupted in the middle and started again where it left
  off.  This allows the named to read in a zone and answer queries at
  (almost) the same time.
*/

struct file_ref_st {
	struct zoneinfo *zp;			/* the zp for this zone */
	files_st *files;			/* The list of all files that
						   are directly or recursively
						   referenced by the file
						   fp.  There is more than
						   one because of $INCLUDE
						   syntax.  When the load is
						   done the top level
						   file_ref_st will have a
						   list of all the files used
						   to define the zone.
						   files->file descripes the
						   file opened with fp */
	files_st *files_tail;			/* A pointer to the last
						   element on the files list */
	FILE *fp;				/* The descriptor of the 
						   file being read at this
						   level in the tree of files.
						   */
	char domain[MAXDNAME];			/* The current domain that
						   the records being read
						   should be place in */
	char origin[MAXDNAME];			/* The domain that the zone
						   data is placed in by
						   default. */
	int class;				/* The default class of the
						   zone data. */
	int format_errs;			/* The number of format errors
						   that occurred while reading
						   fp. */
	int dataflags;				/* The value that the flags
						   field in the database
						   struct will be set to for
						   all data in this zone. */
	int dbflags;				/* Flags sent to db_update
						   for data in this zone. */
	int lineno;				/* The lineno being read
						   of the file cur_file. */
	int zonenum;				/* The zone number of the zone
						   described by zp above. */
	struct file_ref_st *next;		/* A pointer to the
						   file_ref_st which
						   is used to read an include
						   file referenced in file fp*/
};
typedef struct file_ref_st file_ref_st;

#endif ULTRIXFUNC 

struct zoneinfo {
	int	z_type;			/* type of zone */
	int	z_auth;			/* zone is authoritative */
	char	*z_origin;		/* root domain name of zone */
	time_t	z_time;			/* time for next refresh */
					/* Primary: the time to check
					   to see if the files that define
					   the zone have changed */
					/* Secondary: The time to talk to the
					   primary to see if the zone has
					   changed */
	time_t	z_lastupdate;		/* time of last refresh */
					/* Primary: the time of the last
					   read of the data files */
					/* Secondary: mtime of cache files
					   or the last time the zone was
					   pulled. */
	u_long	z_refresh;		/* refresh interval */
					/* Primary: The amount of time
					   to read the data files again */
					/* Secondary: time to wait until the
					   primary should be contacted to see
					   if the zone has changed */
	u_long	z_retry;		/* refresh retry interval */
					/* Primary:No meaning ?File locking? */
					/* Secondary: time to wait until
					   you try to talk to a down primary */
	u_long	z_expire;		/* expiration time for cached info */
					/* expiration time of data used by
					   secondaries to determine if all the
					   data in the zone is old. */
	u_long	z_minimum;		/* minimum TTL value */
					/* Given out to the slaves as the
					   ttl of data in this zone. */
	u_long	z_serial;		/* changes if zone modified */
	char	*z_source;		/* source location of data */
					/* Secondary: name of the server
					   that provides the date */
					/* Primary: name of the file that 
					   holds the date */
	time_t	z_ftime;		/* modification time of source file */
					/* Secondary: mod time of the cache
					   file. */
					/* Primary: mod time of the oldest
					   file in the file group */
	int	z_addrcnt;		/* number of addresses stored in the
					   following array */
	struct	in_addr z_addr[NSMAX];	/* list of master servers for zone */
#ifdef AUTHEN
	char	z_dname[NSMAX][MAXDNAME]; /* list of the kerberos names for
					     servers of the zone */
#endif AUTHEN

	int	z_sysloged;		/* has fail to transfer been sysloged*/

#ifdef ALLOW_UPDATES
	int	hasChanged;		/* non-zero if zone has been updated
					 * since last checkpoint
					 */
#endif ALLOW_UPDATES

	int		z_state;	/* state indicates the current state
					   of the zone.  The state bits are
					   listed below. */

	int		z_xferpid;	/* the pid of the xfer child process */

#ifdef ULTRIXFUNC
	int 		z_class;	/* class of info stored by the zone */

	files_st	*z_files;	/* Primary: A ptr to the list of files
					   that contain this zone.
					   Secondary: A ptr to the file struct
					   which descibes the cache file. */
	file_ref_st	*z_load_info;	/* Primary: A ptr to the struct
					   that describes the state of the
					   current refresh of the zone from
					   the source files.
					   Secondary: A ptr to the struct 
					   that describes the state of the 
					   current refresh of the zone from the
					   xfer result file. */
#endif ULTRIXFUNC
};

	/* zone types (z_type) */
#define Z_PRIMARY	1
#define Z_SECONDARY	2
#define Z_CACHE		3

	/* zone state bits */
#define	Z_AUTH		0x01		/* should replace z_auth */
#define	Z_NEED_XFER	0x02		/* waiting to do xfer */
#define	Z_XFER_RUNNING	0x04		/* asynch. xfer is running */
#define	Z_NEED_RELOAD	0x08		/* waiting to do reload */

#ifdef ULTRIXFUNC
/*#define	Z_SYSLOGGED	0x10		/* have logged timeout */
/*#define	Z_CHANGED	0x20		/* should replace hasChanged */
/*#define	Z_FOUND		0x40		/* found in boot file when reloading */
/*#define	Z_INCLUDE	0x80		/* set if include used in file */
/*#define	Z_DB_BAD	0x100		/* errors when loading file */
#define	Z_SLOW_RELOAD	0x101		/* set if include used in file */

#endif ULTRIXFUNC

	/* xfer exit codes */
#define	XFER_UPTODATE	0		/* zone is up-to-date */
#define	XFER_SUCCESS	1		/* performed transfer successfully */
#define	XFER_TIMEOUT	2		/* no server reachable/xfer timeout */
#define	XFER_FAIL	3		/* other failure, has been logged */

/*
 * Structure for recording info on forwarded queries.
 */
struct qinfo {
	u_short	q_id;			/* id of query */
	u_short	q_nsid;			/* id of forwarded query */
	int	q_dfd;			/* UDP file descriptor */
	struct	sockaddr_in q_from;	/* requestor's address */
#ifdef AUTHEN
	char q_dname[MAXDNAME];
	int q_type;
	int q_class;
	AUTH_DAT q_ad;			/* auth data for requestor */
	int q_type_in;			/* query type sent to ns */
	int q_type_out;			/* query type used with other ns */
	int q_authentype_in;		/* type of authentication used to talk
					   to the sender of the query */
	int q_authenver_in;
	int q_authentype_out;		/* type of authentication used to talk
					   to other name servers */
	int q_authenver_out;
	char authen_msg;		/* the authenticated message */
	int authen_msglen;		/* len of authen message */
	char q_dname_curr[MAXDNAME];
	int q_type_curr;
	int q_class_curr;
#endif AUTHEN
#ifdef ULTRIXFUNC
	struct	fwdinfo	*q_fwdorig;	/* first forwarder used */
#endif ULTRIXFUNC
	char	*q_msg;			/* the message */
	int	q_msglen;		/* len of message */
	int	q_naddr;		/* number of addr's in q_addr */
	int	q_curaddr;		/* last addr sent to */
	struct	fwdinfo	*q_fwd;		/* last	forwarder used */
	time_t	q_time;			/* time to retry */
	struct	qinfo *q_next;		/* rexmit list (sorted by time) */
	struct	qinfo *q_link;		/* storage list (random order) */
	struct  qserv {
		struct	sockaddr_in ns_addr;	/* addresses of NS's */
		struct  databuf *ns;	/* databuf for NS record */
		struct  databuf *nsdata; /* databuf for server address */
		struct  timeval stime;	/* time first query started */
		int	nretry;		/* # of times addr retried */
#ifdef AUTHEN
		char	ns_name[MAXDNAME];
		CREDENTIALS creden;	/* credentials for the NS */
#endif AUTHEN
	} q_addr[NSMAX];		/* addresses of NS's */
	struct	databuf *q_usedns[NSMAX]; /* databuf for NS that we've tried */
	int	q_nusedns;
	int	q_cname;		/* # of cnames found */
	int	q_nqueries;		/* # of queries required */
	char	*q_cmsg;		/* the cname message */
	int	q_cmsglen;		/* len of cname message */
	struct	qstream *q_stream;	/* TCP stream, null if UDP */
	int	q_system;		/* boolean, system query */
};

#define	Q_NEXTADDR(qp,n)	\
	(((qp)->q_fwd == (struct fwdinfo *)0) ? \
	 &(qp)->q_addr[n].ns_addr : &(qp)->q_fwd->fwdaddr)

#ifdef AUTHEN

#define	Q_NEXTNAME(qp,n)	\
	(((qp)->q_fwd == (struct fwdinfo *)0) ? \
	 (qp)->q_addr[n].ns_name : (qp)->q_fwd->fwdname)

#define	Q_NEXTCRED(qp,n)	\
	(((qp)->q_fwd == (struct fwdinfo *)0) ? \
	 &(qp)->q_addr[n].creden : &(qp)->q_fwd->fwdcreden)

#endif AUTHEN

#define PRIMING_CACHE	42
#define QINFO_NULL	((struct qinfo *)0)

#ifndef XFER
extern struct qinfo *qfindid();
extern struct qinfo *qnew();
extern struct qinfo *retryqp;		/* next query to retry */
#endif XFER

/*
 * Return codes from ns_forw:
 */
#define	FW_OK		0
#define	FW_DUP		1
#define	FW_NOSERVER	2
#define	FW_SERVFAIL	3

struct qstream {
	int 	s_rfd;			/* stream file descriptor */
	int 	s_size;			/* expected amount of data to recive */
	int 	s_bufsize;		/* amount of data recived in s_buf */
	char    *s_buf;			/* buffer of recived data */
	char    *s_bufp;		/* pointer into s_buf of recived data */
	struct	qstream *s_next;	/* next stream */
	struct	sockaddr_in s_from;	/* address query came from */
	u_long	s_time;			/* time stamp of last transaction */
	int	s_refcnt;		/* number of outstanding queries */
	u_short	s_tempsize;		/* temporary for size from net */
};

#define QSTREAM_NULL	((struct qstream *)0)
extern struct qstream *streamq;		/* stream queue */

struct qdatagram {
	int 	dq_dfd;			/* datagram file descriptor */
	struct	qdatagram *dq_next;	/* next datagram */
	struct	in_addr  dq_addr;	/* address of interface */
};

#define QDATAGRAM_NULL	((struct qdatagram *)0)
extern struct qdatagram *datagramq;	/* datagram queue */

struct netinfo {
	struct netinfo *next;
	u_long net;
	u_long mask;
	struct in_addr my_addr;
};

struct fwdinfo {
	struct fwdinfo *next;
	struct sockaddr_in fwdaddr;
#ifdef AUTHEN
	char	fwdname[MAXDNAME];
	CREDENTIALS fwdcreden;
#endif AUTHEN
};

struct nets {
	char *name;
	long net;
	struct nets *next;
}; 

/*
 *  Statistics Defines
 */
struct stats {
	unsigned long	cnt;
	char	*description;
};

/* gross count of UDP packets in and out */
#define	S_INPKTS	0
#define	S_OUTPKTS	1
/* gross count of queries and inverse queries received */
#define	S_QUERIES	2
#define	S_IQUERIES	3
#define S_DUPQUERIES	4
#define	S_RESPONSES	5
#define	S_DUPRESP	6
#define	S_RESPOK	7
#define	S_RESPFAIL	8
#define	S_RESPFORMERR	9
#define	S_SYSQUERIES	10
#define	S_PRIMECACHE	11
#define	S_CHECKNS	12
#define	S_BADRESPONSES	13
#define	S_MARTIANS	14
#define S_NSTATS	15	/* Careful! */
#ifdef STATS
extern struct stats stats[S_NSTATS];
extern unsigned long typestats[T_ANY+1];
#endif

#ifdef DEBUG
extern int debug;			/* debug flag */
extern FILE *ddt;			/* debug file discriptor */
#endif
#ifndef XFER
extern int ds;				/* datagram socket */
extern struct qdatagram *dqp;
extern struct timeval tt;		/* place to store time */

extern struct itimerval ival;		/* maintenance interval */
extern struct zoneinfo zones[MAXZONES];	/* zone information */
extern int nzones;			/* number of zones in use */
#endif XFER

#ifdef vax
extern u_short htons(), ntohs();
extern u_long htonl(), ntohl();
#endif

#ifdef AUTHEN
#define DEFSIZE_KRBCRED 50
#endif AUTHEN

#define MAX_XFER_TIME         60 * 40	/* max seconds for an xfer */
#define XFER_TIME_FUDGE       10	/* max seconds for an xfer */

#ifndef XFER
extern int xfer_running_cnt;		/* number of xfers running */
extern int xfer_deferred_cnt;		/* number of deferred xfers */
#define MAX_XFERS_RUNNING     4		/* max value of xfer_running_cnt */


#define STOP_LOAD -2
#define NO_ITER_LIMIT -1
#define NUM_ITER_LOAD 10
#define ALL_DATA -1
#endif XFER

#define	_PATH_TMPXFER	"/usr/tmp/xfer.ddt.XXXXXX"
#define	_PATH_XFER	"/usr/etc/named-xfer"





