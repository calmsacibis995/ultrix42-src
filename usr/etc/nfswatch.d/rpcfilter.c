#ifndef lint
static char *sccsid = "@(#)rpcfilter.c	4.2	(ULTRIX)	1/25/91";
#endif
/* Based on:
 * RCSid = "$Header: /sparky/a/davy/system/nfswatch/RCS/rpcfilter.c,v 3.0 91/01/23 08:23:20 davy Exp $";
 */

/*
 * rpcfilter.c - filter RPC packets.
 *
 * David A. Curry				Jeffrey C. Mogul
 * SRI International				Digital Equipment Corporation
 * 333 Ravenswood Avenue			Western Research Laboratory
 * Menlo Park, CA 94025				100 Hamilton Avenue
 * davy@erg.sri.com				Palo Alto, CA 94301
 *						mogul@decwrl.dec.com
 *
 * $Log:	rpcfilter.c,v $
 * Revision 3.0  91/01/23  08:23:20  davy
 * NFSWATCH Version 3.0.
 * 
 * Revision 1.5  91/01/17  10:13:02  davy
 * Bug fix from Jeff Mogul.
 * 
 * Revision 1.7  91/01/16  15:49:12  mogul
 * Print server or client address in a.b.c.d notation if name not known
 * 
 * Revision 1.6  91/01/07  15:35:51  mogul
 * Uses hash table instead of linear search on clients
 * One-element "hint" cache to avoid client hash lookup
 * 
 * Revision 1.5  91/01/04  14:12:35  mogul
 * Support for client counters
 * Disable screen update during database upheaval
 * 
 * Revision 1.4  91/01/03  17:35:00  mogul
 * Count per-procedure info
 * 
 * Revision 1.3  90/12/04  08:22:06  davy
 * Fix from Dan Trinkle (trinkle@cs.purdue.edu) to determine byte order in
 * file handle.
 * 
 * Revision 1.2  90/08/17  15:47:44  davy
 * NFSWATCH Version 2.0.
 * 
 * Revision 1.1  88/11/29  11:20:51  davy
 * NFSWATCH Release 1.0
 * 
 */
#include <sys/param.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <rpc/types.h>
#include <rpc/xdr.h>
#include <rpc/auth.h>
#include <rpc/clnt.h>
#include <rpc/rpc_msg.h>
#include <rpc/pmap_clnt.h>
#include <rpc/svc.h>
#include <netdb.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>

#define NFSSERVER	1

#ifdef sun
#include <sys/vfs.h>
#endif /* sun */
#ifdef ultrix
#include <sys/types.h>
#include <sys/time.h>
#endif /* ultrix */
#include <nfs/nfs.h>

#include "nfswatch.h"
#include "externs.h"
#include "rpcdefs.h"

/*
 * NFS procedure types and XDR argument decoding routines.
 */
static struct nfs_proc nfs_procs[] = {
/* RFS_NULL (0)		*/
	NFS_READ,	xdr_void,	0,
/* RFS_GETATTR (1)	*/
	NFS_READ,	xdr_fhandle,	sizeof(fhandle_t),
/* RFS_SETATTR (2)	*/
	NFS_WRITE,	xdr_saargs,	sizeof(struct nfssaargs),
/* RFS_ROOT (3)		*/
	NFS_READ,	xdr_void,	0,
/* RFS_LOOKUP (4)	*/
	NFS_READ,	xdr_diropargs,	sizeof(struct nfsdiropargs),
/* RFS_READLINK (5)	*/
	NFS_READ,	xdr_fhandle,	sizeof(fhandle_t),
/* RFS_READ (6)		*/
	NFS_READ,	xdr_readargs,	sizeof(struct nfsreadargs),
/* RFS_WRITECACHE (7)	*/
	NFS_WRITE,	xdr_void,	0,
/* RFS_WRITE (8)	*/
	NFS_WRITE,	xdr_writeargs,	sizeof(struct nfswriteargs),
/* RFS_CREATE (9)	*/
	NFS_WRITE,	xdr_creatargs,	sizeof(struct nfscreatargs),
/* RFS_REMOVE (10)	*/
	NFS_WRITE,	xdr_diropargs,	sizeof(struct nfsdiropargs),
/* RFS_RENAME (11)	*/
	NFS_WRITE,	xdr_rnmargs,	sizeof(struct nfsrnmargs),
/* RFS_LINK (12)	*/
	NFS_WRITE,	xdr_linkargs,	sizeof(struct nfslinkargs),
/* RFS_SYMLINK (13)	*/
	NFS_WRITE,	xdr_slargs,	sizeof(struct nfsslargs),
/* RFS_MKDIR (14)	*/
	NFS_WRITE,	xdr_creatargs,	sizeof(struct nfscreatargs),
/* RFS_RMDIR (15)	*/
	NFS_WRITE,	xdr_diropargs,	sizeof(struct nfsdiropargs),
/* RFS_READDIR (16)	*/
	NFS_READ,	xdr_rddirargs,	sizeof(struct nfsrddirargs),
/* RFS_STATFS (17)	*/
	NFS_READ,	xdr_fhandle,	sizeof(fhandle_t)
};

/*
 * rpc_filter - pass off RPC packets to other filters.
 */
void
rpc_filter(data, length, src, dst)
register u_long src, dst;
register u_int length;
register char *data;
{
	register struct rpc_msg *msg;

	msg = (struct rpc_msg *) data;

	/*
	 * See which "direction" the packet is going.  We
	 * can classify RPC CALLs, but we cannot classify
	 * REPLYs, since they no longer have the RPC
	 * program number in them (sigh).
	 */
	switch (ntohl(msg->rm_direction)) {
	case CALL:			/* RPC call			*/
		rpc_callfilter(data, length, src, dst);
		break;
	case REPLY:			/* RPC reply			*/
		rpc_replyfilter(data, length, src, dst);
		break;
	default:			/* probably not an RPC packet	*/
		break;
	}
}

/*
 * rpc_callfilter - filter RPC call packets.
 */
void
rpc_callfilter(data, length, src, dst)
register u_long src, dst;
register u_int length;
register char *data;
{
	register struct rpc_msg *msg;

	msg = (struct rpc_msg *) data;

	/*
	 * Decide what to do based on the program.
	 */
	switch (ntohl(msg->rm_call.cb_prog)) {
	case RPC_NFSPROG:
		nfs_filter(data, length, src, dst);
		break;
	case RPC_YPPROG:
	case RPC_YPBINDPROG:
	case RPC_YPPASSWDPROG:
		pkt_counters[PKT_YELLOWPAGES].pc_interval++;
		pkt_counters[PKT_YELLOWPAGES].pc_total++;
		break;
	case RPC_MOUNTPROG:
		pkt_counters[PKT_NFSMOUNT].pc_interval++;
		pkt_counters[PKT_NFSMOUNT].pc_total++;
		break;
#ifdef notdef
	case RPC_PMAPPROG:
	case RPC_RSTATPROG:
	case RPC_RUSERSPROG:
	case RPC_DBXPROG:
	case RPC_WALLPROG:
	case RPC_ETHERSTATPROG:
	case RPC_RQUOTAPROG:
	case RPC_SPRAYPROG:
	case RPC_IBM3270PROG:
	case RPC_IBMRJEPROG:
	case RPC_SELNSVCPROG:
	case RPC_RDATABASEPROG:
	case RPC_REXECPROG:
	case RPC_ALICEPROG:
	case RPC_SCHEDPROG:
	case RPC_LOCKPROG:
	case RPC_NETLOCKPROG:
	case RPC_X25PROG:
	case RPC_STATMON1PROG:
	case RPC_STATMON2PROG:
	case RPC_SELNLIBPROG:
	case RPC_BOOTPARAMPROG:
	case RPC_MAZEPROG:
	case RPC_YPUPDATEPROG:
	case RPC_KEYSERVEPROG:
	case RPC_SECURECMDPROG:
	case RPC_NETFWDIPROG:
	case RPC_NETFWDTPROG:
	case RPC_SUNLINKMAP_PROG:
	case RPC_NETMONPROG:
	case RPC_DBASEPROG:
	case RPC_PWDAUTHPROG:
	case RPC_TFSPROG:
	case RPC_NSEPROG:
	case RPC_NSE_ACTIVATE_PROG:
	case RPC_PCNFSDPROG:
	case RPC_PYRAMIDLOCKINGPROG:
	case RPC_PYRAMIDSYS5:
	case RPC_CADDS_IMAGE:
	case RPC_ADT_RFLOCKPROG:
#endif /* notdef */
	default:
		pkt_counters[PKT_OTHERRPC].pc_interval++;
		pkt_counters[PKT_OTHERRPC].pc_total++;
		break;
	}
}

/*
 * rpc_replyfilter - count RPC reply packets.
 */
void
rpc_replyfilter(data, length, src, dst)
register u_long src, dst;
register u_int length;
register char *data;
{
	register struct rpc_msg *msg;

	msg = (struct rpc_msg *) data;

	pkt_counters[PKT_RPCAUTH].pc_interval++;
	pkt_counters[PKT_RPCAUTH].pc_total++;
}

/*
 * nfs_filter - filter NFS packets.
 */
void
nfs_filter(data, length, src, dst)
register u_long src, dst;
register u_int length;
register char *data;
{
	u_int proc;
	caddr_t args;
	SVCXPRT *xprt;
	struct rpc_msg msg;
	union nfs_rfsargs nfs_rfsargs;
	char cred_area[2*MAX_AUTH_BYTES];

	msg.rm_call.cb_cred.oa_base = cred_area;
	msg.rm_call.cb_verf.oa_base = &(cred_area[MAX_AUTH_BYTES]);

	/*
	 * Act as if we received this packet through RPC.
	 */
	if (!udprpc_recv(data, length, &msg, &xprt))
		return;

	/*
	 * Get the NFS procedure number.
	 */
	proc = msg.rm_call.cb_proc;

	if (proc >= RFS_NPROC)
		return;

	/*
	 * Now decode the arguments to the procedure from
	 * XDR format.
	 */
	args = (caddr_t) &nfs_rfsargs;
	(void) bzero(args, nfs_procs[proc].nfs_argsz);

	if (!SVC_GETARGS(xprt, nfs_procs[proc].nfs_xdrargs, args))
		return;

	prc_counters[prc_countmap[proc]].pr_total++;
	prc_counters[prc_countmap[proc]].pr_interval++;

	CountSrc(src);

	/*
	 * Now count the packet in the appropriate file system's
	 * counters.
	 */
	switch (proc) {
	case RFS_NULL:
		break;
	case RFS_GETATTR:
		nfs_count(&nfs_rfsargs.fhandle, proc);
		break;
	case RFS_SETATTR:
		nfs_count(&nfs_rfsargs.nfssaargs.saa_fh, proc);
		break;
	case RFS_ROOT:
		break;
	case RFS_LOOKUP:
		nfs_count(&nfs_rfsargs.nfsdiropargs.da_fhandle, proc);
		break;
	case RFS_READLINK:
		nfs_count(&nfs_rfsargs.fhandle, proc);
		break;
	case RFS_READ:
		nfs_count(&nfs_rfsargs.nfsreadargs.ra_fhandle, proc);
		break;
	case RFS_WRITECACHE:
		break;
	case RFS_WRITE:
		nfs_count(&nfs_rfsargs.nfswriteargs.wa_fhandle, proc);
		break;
	case RFS_CREATE:
		nfs_count(&nfs_rfsargs.nfscreatargs.ca_da.da_fhandle, proc);
		break;
	case RFS_REMOVE:
		nfs_count(&nfs_rfsargs.nfsdiropargs.da_fhandle, proc);
		break;
	case RFS_RENAME:
		nfs_count(&nfs_rfsargs.nfsrnmargs.rna_from.da_fhandle, proc);
		break;
	case RFS_LINK:
		nfs_count(&nfs_rfsargs.nfslinkargs.la_from, proc);
		break;
	case RFS_SYMLINK:
		nfs_count(&nfs_rfsargs.nfsslargs.sla_from.da_fhandle, proc);
		break;
	case RFS_MKDIR:
		nfs_count(&nfs_rfsargs.nfscreatargs.ca_da.da_fhandle, proc);
		break;
	case RFS_RMDIR:
		nfs_count(&nfs_rfsargs.nfsdiropargs.da_fhandle, proc);
		break;
	case RFS_READDIR:
		nfs_count(&nfs_rfsargs.nfsrddirargs.rda_fh, proc);
		break;
	case RFS_STATFS:
		nfs_count(&nfs_rfsargs.fhandle, proc);
		break;
	}

	/*
	 * Decide whether it's a read or write process.
	 */
	switch (nfs_procs[proc].nfs_proctype) {
	case NFS_READ:
		pkt_counters[PKT_NFSREAD].pc_interval++;
		pkt_counters[PKT_NFSREAD].pc_total++;
		break;
	case NFS_WRITE:
		pkt_counters[PKT_NFSWRITE].pc_interval++;
		pkt_counters[PKT_NFSWRITE].pc_total++;
		break;
	}
}

/*
 * nfs_count - count an NFS reference to a specific file system.
 */
void
nfs_count(fh, proc)
register fhandle_t *fh;
int proc;
{
	long fsid;
	register int i, match1, match2;

	/*
	 * Run through the NFS counters looking for the matching
	 * file system.
	 */
	match1 = 0;

	for (i = 0; i < nnfscounters; i++) {
		if (learnfs)
			fsid = nfs_counters[i].nc_fsid;
		else
			fsid = (long) nfs_counters[i].nc_dev;

		/*
		 * Compare the device numbers.  Sun uses an
		 * fsid_t for the device number, which is an
		 * array of 2 longs.  The first long contains
		 * the device number.
		 */
		match1 = !bcmp((char *) &(fh->fh_fsid), (char *) &fsid,
			sizeof(long));

		/*
		 * Check server address.
		 */
		if (allflag && match1)
			match1 = (thisdst == nfs_counters[i].nc_ipaddr);

		if (match1) {
			nfs_counters[i].nc_proc[proc]++;
			nfs_counters[i].nc_interval++;
			nfs_counters[i].nc_total++;
			break;
		}
	}

	/*
	 * We don't know about this file system, but we can
	 * learn.
	 */
	if (!match1 && learnfs && (nnfscounters < MAXEXPORT)) {
		static char fsname[64], prefix[64];
		long fsid;
		int oldm;

		oldm = sigblock(sigmask(SIGALRM));
	    				/* no redisplay while unstable */

		i = nnfscounters++;

		bcopy((char *) &(fh->fh_fsid), (char *) &fsid, sizeof(long));

		nfs_counters[i].nc_fsid = fsid;
		nfs_counters[i].nc_proc[proc]++;
		nfs_counters[i].nc_interval++;
		nfs_counters[i].nc_total++;

		/*
		 * See if server uses opposite byte order.
		 */
		if ((fsid & 0xffff0000) && ((fsid & 0xffff) == 0))
			fsid = ntohl(fsid);

		/*
		 * Some hosts use 32-bit values.
		 */
		if (fsid & 0xffff0000) {
			/*
			 * Try to intuit the byte order.
			 */
			if (fsid & 0xff00) {
			  nfs_counters[i].nc_dev = makedev((fsid >> 8) & 0xff,
							   (fsid >> 24) & 0xff);
			}
			else {
			  nfs_counters[i].nc_dev = makedev((fsid >> 16) & 0xff,
							   fsid & 0xff);
			}
		}
		else {
			nfs_counters[i].nc_dev = makedev(major(fsid),
							 minor(fsid));
		}

		*prefix = 0;

		if (allflag) {
			struct hostent *hp;

			nfs_counters[i].nc_ipaddr = thisdst;
			hp = gethostbyaddr(&thisdst, sizeof(thisdst), AF_INET);

			if (hp) {
				char *index();
				char *dotp;

				sprintf(prefix, "%s", hp->h_name);

				if ((dotp = index(prefix, '.')) != NULL)
					*dotp = 0;
			}
			else {
				struct in_addr ia;
				ia.s_addr = thisdst;
				sprintf(prefix, "%s", inet_ntoa(ia));
			}
		}

		sprintf(fsname, "%.12s(%d,%d)", prefix,
			major(nfs_counters[i].nc_dev),
			minor(nfs_counters[i].nc_dev));

		nfs_counters[i].nc_name = savestr(fsname);
		sort_nfs_counters();
		(void) sigsetmask(oldm);	/* permit redisplay */
	}

	if (filelist == NULL)
		return;

	/*
	 * Run through the file counters looking for the matching
	 * file.
	 */
	for (i = 0; i < nfilecounters; i++) {
		fsid = (long) fil_counters[i].fc_dev;

		/*
		 * Compare device numbers and file numbers.  Sun
		 * uses an fsid_t for the device, which is an
		 * array of two longs.  They use an fid for the
		 * inode.  The inode number is the first part
		 * of this.
		 */
		match1 = !bcmp((char *) &(fh->fh_fsid), (char *) &fsid,
			 sizeof(long));

		if (!match1)
			continue;

#ifdef sun
		/*
		 * NOTE: this is dependent on the contents of the fh_data
		 *       part of the file handle.  This is correct for
		 *       SunOS 4.1 on SPARCs.
		 */
		match2 = !bcmp((char *) &(fh->fh_data[2]),
			 (char *) &(fil_counters[i].fc_ino), sizeof(ino_t));
#endif /* sun */

#ifdef ultrix
		match2 = !bcmp((char *) fh->fh_fno,
			 (char *) &(fil_counters[i].fc_ino), sizeof(ino_t));
#endif /* ultrix */

		if (match2) {
			fil_counters[i].fc_proc[proc]++;
			fil_counters[i].fc_interval++;
			fil_counters[i].fc_total++;
			break;
		}
	}
}

/*
 * CountSrc uses a hash table to speed lookups.  Hash function
 *	uses high and low octect of IP address, so as to be
 *	fast and byte-order independent.  Table is organized
 *	as a array of linked lists.
 */
#define	HASHSIZE	0x100
#define	HASH(addr)	(((addr) & 0xFF) ^ (((addr) >> 24) & 0xFF))

ClientCounter *Addr_hashtable[HASHSIZE];	/* initially all NULL ptrs */

ClientCounter *cc_hint = clnt_counters;		/* one-element cache */

CountSrc(src)
register u_long src;
{
	register ClientCounter *ccp;
	int hcode = HASH(src);
	
	/* See if this is the same client as last time */
	if (cc_hint->cl_ipaddr == src) {
	    cc_hint->cl_total++;
	    cc_hint->cl_interval++;
	    return;
	}

	/* Search hash table */
	ccp = Addr_hashtable[hcode];
	while (ccp) {
	    if (ccp->cl_ipaddr == src) {
		ccp->cl_total++;
		ccp->cl_interval++;
		cc_hint = ccp;
		return;
	    }
	    ccp = ccp->cl_next;
	}
	
	/* new client */
	if (nclientcounters < MAXCLIENTS) {
	    struct hostent *hp;
	    static char clnt_name[64];
	    int oldm;
	    
	    oldm = sigblock(sigmask(SIGALRM));
	    				/* no redisplay while unstable */
	    
	    ccp = &(clnt_counters[nclientcounters]);
	    nclientcounters++;
	    
	    /* Add to hash table */
	    ccp->cl_next = Addr_hashtable[hcode];
	    Addr_hashtable[hcode] = ccp;

	    /* Fill in new ClientCounter */
	    ccp->cl_ipaddr = src;
	    hp = gethostbyaddr(&ccp->cl_ipaddr,
					sizeof(ccp->cl_ipaddr), AF_INET);
	    if (hp) {
		char *index();
		char *dotp;

		sprintf(clnt_name, "%s", hp->h_name);

		if ((dotp = index(clnt_name, '.')) != NULL)
			*dotp = 0;
	    }
	    else {
		struct in_addr ia;
		ia.s_addr = ccp->cl_ipaddr;
		sprintf(clnt_name, "%s", inet_ntoa(ia));
	    }

	    ccp->cl_name = savestr(clnt_name);
	    ccp->cl_total = 1;
	    ccp->cl_interval = 1;
	    sort_clnt_counters();
	    (void) sigsetmask(oldm);	/* permit redisplay */
	}
}

/*
 * Must be called after sorting the clnt_counters[] table
 *	Should put busiest ones at front of list, but doesn't
 */
ClientHashRebuild()
{
	register int i;
	register ClientCounter *ccp;
	int hcode;

	bzero(Addr_hashtable, sizeof(Addr_hashtable));
	
	for (i = 0, ccp = clnt_counters; i < nclientcounters; i++, ccp++) {
	    hcode = HASH(ccp->cl_ipaddr);
	    ccp->cl_next = Addr_hashtable[hcode];
	    Addr_hashtable[hcode] = ccp;
	}
}
