/*
 * Based on:
 * SCCSID: @(#)nfswatch.h	4.2	ULTRIX	1/25/91
 * $Header: /sparky/a/davy/system/nfswatch/RCS/nfswatch.h,v 3.0 91/01/23 08:23:13 davy Exp $
 *
 * nfswatch.h - definitions for nfswatch.
 *
 * David A. Curry				Jeffrey C. Mogul
 * SRI International				Digital Equipment Corporation
 * 333 Ravenswood Avenue			Western Research Laboratory
 * Menlo Park, CA 94025				100 Hamilton Avenue
 * davy@erg.sri.com				Palo Alto, CA 94301
 *						mogul@decwrl.dec.com
 *
 * $Log:	nfswatch.h,v $
 * Revision 3.0  91/01/23  08:23:13  davy
 * NFSWATCH Version 3.0.
 * 
 * Revision 1.4  91/01/17  10:12:29  davy
 * New features from Jeff Mogul.
 * 
 * Revision 1.6  91/01/07  15:34:42  mogul
 * Support for client hash table
 * 
 * Revision 1.5  91/01/07  14:10:01  mogul
 * Added SHOWHELP, SHOW_MAXCODE
 * 
 * Revision 1.4  91/01/04  14:12:11  mogul
 * Support for client counters
 * 
 * Revision 1.3  91/01/03  17:38:18  mogul
 * Support for per-procedure counters
 * 
 * Revision 1.2  90/08/17  15:47:04  davy
 * NFSWATCH Version 2.0.
 * 
 * Revision 1.1  88/11/29  11:20:28  davy
 * NFSWATCH Release 1.0
 * 
 */

/*
 * Version number.
 */
#define VERSION		"3.0 of 17 January 1991"

/*
 * General definitions.
 */
#ifndef TRUE
#define TRUE		1
#define FALSE		0
#endif /* TRUE */

#define PROMPT		"nfswatch>"	/* prompt string		*/
#define LOGFILE		"nfswatch.log"	/* log file name		*/
#define MAXEXPORT	256		/* max exported file systems	*/
#define CYCLETIME	10		/* screen update cycle time	*/
#define PACKETSIZE	4096		/* max size of a packet		*/
#define MAXNFSPROC	18		/* max number of NFS procedures	*/
#define MAXHOSTADDR	8		/* max. network addrs per host	*/
#define	MAXCLIENTS	256		/* max. # of client counters	*/
					/* MUST be even number 		*/
#define MAXINTERFACES	16		/* Max. number of interfaces	*/
#define SNAPSHOTFILE	"nfswatch.snap"	/* snapshot file name		*/

#define SHOWINDVFILES	1		/* show individual files	*/
#define SHOWFILESYSTEM	2		/* show NFS file systems	*/
#define SHOWNFSPROCS	3		/* show NFS procedure counts	*/
#define SHOWCLIENTS	4		/* show client host names	*/
#define	SHOWHELP	5		/* show help text		*/
#define	SHOW_MAXCODE	5		/* number of different displays */

/*
 * Network Interface Tap (NIT) definitions.
 */
#define NIT_DEV		"/dev/nit"	/* network interface tap device	*/
#define NIT_BUF		"nbuf"		/* nit stream buffering module	*/
#define NIT_CHUNKSIZE	8192		/* chunk size for grabbing pkts	*/

/*
 * Packet counter definitions.
 */
#define PKT_NCOUNTERS	16		/* number of packet counters	*/

#define PKT_NDREAD	0		/* ND read requests		*/
#define PKT_NDWRITE	1		/* ND write requests		*/
#define PKT_NFSREAD	2		/* NFS read requests		*/
#define PKT_NFSWRITE	3		/* NFS write requests		*/
#define PKT_NFSMOUNT	4		/* NFS mount requests		*/
#define PKT_YELLOWPAGES	5		/* Yellow Pages requests	*/
#define PKT_RPCAUTH	6		/* RPC authorization requests	*/
#define PKT_OTHERRPC	7		/* other RPC requests		*/
#define PKT_TCP		8		/* TCP packets			*/
#define PKT_UDP		9		/* UDP packets			*/
#define PKT_ICMP	10		/* ICMP packets			*/
#define PKT_ROUTING	11		/* routing control packets	*/
#define PKT_ARP		12		/* address resolution packets	*/
#define PKT_RARP	13		/* reverse addr resol packets	*/
#define PKT_BROADCAST	14		/* ethernet broadcast packets	*/
#define PKT_OTHER	15		/* none of the above packets	*/

typedef unsigned long	Counter;

/*
 * Packet counting structure.
 */
typedef struct {
	char	*pc_name;		/* name of counter		*/

	Counter	pc_interval;		/* packets this interval	*/
	Counter	pc_total;		/* packets since start		*/

	short	pc_intx, pc_inty;	/* screen coords of pc_interval	*/
	short	pc_totx, pc_toty;	/* screen coords of pc_total	*/
	short	pc_pctx, pc_pcty;	/* screen coords of percentage	*/
	short	pc_namex, pc_namey;	/* screen coords of pc_name	*/
} PacketCounter;

/*
 * NFS request counting structure.
 */
typedef struct {
	dev_t	nc_dev;			/* device numbers of file sys	*/
	long	nc_fsid;		/* for "learning" file systems	*/
	long	nc_ipaddr;		/* keep track of server address	*/
	char	*nc_name;		/* name of file system		*/

	Counter	nc_total;		/* requests since start		*/
	Counter	nc_interval;		/* requests this interval	*/
	Counter nc_proc[MAXNFSPROC];	/* each nfs proc counters	*/

	short	nc_intx, nc_inty;	/* screen coords of nc_interval	*/
	short	nc_totx, nc_toty;	/* screen coords of nc_total	*/
	short	nc_pctx, nc_pcty;	/* screen coords of percentage	*/
	short	nc_namex, nc_namey;	/* screen coords of nc_name	*/
} NFSCounter;

/*
 * Specific file request counting structure.
 */
typedef struct {
	dev_t	fc_dev;			/* device number of file sys	*/
	ino_t	fc_ino;			/* inode number of file		*/
	char	*fc_name;		/* file name			*/

	Counter	fc_total;		/* requests since start		*/
	Counter	fc_interval;		/* requests this interval	*/
	Counter	fc_proc[MAXNFSPROC];	/* each nfs proc counters	*/

	short	fc_intx, fc_inty;	/* screen coords of fc_interval	*/
	short	fc_totx, fc_toty;	/* screen coords of fc_total	*/
	short	fc_pctx, fc_pcty;	/* screen coords of percentage	*/
	short	fc_namex, fc_namey;	/* screen coords of fc_name	*/
} FileCounter;

/*
 * Per-procedure counting structure.
 */
typedef struct {
	int	pr_type;		/* procedure type		*/
	char 	*pr_name;		/* procedure name		*/
	Counter	pr_total;		/* requests since start		*/
	Counter	pr_interval;		/* requests this interval	*/

	short	pr_intx, pr_inty;	/* screen coords of pr_interval	*/
	short	pr_totx, pr_toty;	/* screen coords of pr_total	*/
	short	pr_pctx, pr_pcty;	/* screen coords of percentage	*/
	short	pr_namex, pr_namey;	/* screen coords of pr_name	*/
} ProcCounter;

/*
 * NFS client counting structure.
 */
typedef struct _cl_ {
	long	cl_ipaddr;		/* client IP address		*/
	char	*cl_name;		/* name of client system	*/

	Counter	cl_total;		/* requests since start		*/
	Counter	cl_interval;		/* requests this interval	*/

	short	cl_intx, cl_inty;	/* screen coords of cl_interval	*/
	short	cl_totx, cl_toty;	/* screen coords of cl_total	*/
	short	cl_pctx, cl_pcty;	/* screen coords of percentage	*/
	short	cl_namex, cl_namey;	/* screen coords of cl_name	*/
	
	struct	_cl_ *cl_next;		/* hash chain link		*/
} ClientCounter;

/*
 * Definitions for earlier systems which don't have these from 4.3BSD.
 */
#ifndef MAXHOSTNAMELEN
# define MAXHOSTNAMELEN	64
#endif /* MAXHOSTNAMELEN */

#ifndef NFDBITS
  typedef long		fd_mask;

# define NFDBITS	(sizeof(fd_mask) * NBBY)

# define FD_SET(n, p)	((p)->fds_bits[(n)/NFDBITS] |= (1 << ((n) % NFDBITS)))
# define FD_CLR(n, p)	((p)->fds_bits[(n)/NFDBITS] &= ~(1 << ((n) % NFDBITS)))
# define FD_ISSET(n, p)	((p)->fds_bits[(n)/NFDBITS] & (1 << ((n) % NFDBITS)))
# define FD_ZERO(p)	(void) bzero((char *)(p), sizeof(*(p)))
#endif /* NFDBITS */
