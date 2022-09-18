/*
 * SCCSID: @(#)rpcdefs.h	4.2	ULTRIX	1/25/91
 * Based on:
 * $Header: /sparky/a/davy/system/nfswatch/RCS/rpcdefs.h,v 3.0 91/01/23 08:23:19 davy Exp $
 *
 * rpcdefs.h - definitions for RPC processing code.
 *
 * David A. Curry				Jeffrey C. Mogul
 * SRI International				Digital Equipment Corporation
 * 333 Ravenswood Avenue			Western Research Laboratory
 * Menlo Park, CA 94025				100 Hamilton Avenue
 * davy@erg.sri.com				Palo Alto, CA 94301
 *						mogul@decwrl.dec.com
 *
 * $Log:	rpcdefs.h,v $
 * Revision 3.0  91/01/23  08:23:19  davy
 * NFSWATCH Version 3.0.
 * 
 * Revision 1.2  90/08/17  15:47:10  davy
 * NFSWATCH Version 2.0.
 * 
 * Revision 1.1  88/11/29  11:20:30  davy
 * NFSWATCH Release 1.0
 * 
 */

#define NFS_READ		0
#define NFS_WRITE		1

/*
 * RPC programs, from "Remote Procedure Call Programming Guide",
 * Revision A, 9 May 1988, pp. 64-65.
 */
#define RPC_PMAPPROG		((u_long) 100000) /* portmapper		     */
#define RPC_RSTATPROG		((u_long) 100001) /* remote stats	     */
#define RPC_RUSERSPROG		((u_long) 100002) /* remote users	     */
#define RPC_NFSPROG		((u_long) 100003) /* NFS		     */
#define RPC_YPPROG		((u_long) 100004) /* Yellow Pages	     */
#define RPC_MOUNTPROG		((u_long) 100005) /* mount daemon	     */
#define RPC_DBXPROG		((u_long) 100006) /* remote dbx		     */
#define RPC_YPBINDPROG		((u_long) 100007) /* yp binder		     */
#define RPC_WALLPROG		((u_long) 100008) /* shutdown msg	     */
#define RPC_YPPASSWDPROG	((u_long) 100009) /* yppasswd server	     */
#define RPC_ETHERSTATPROG	((u_long) 100010) /* ether stats	     */
#define RPC_RQUOTAPROG		((u_long) 100011) /* disk quotas	     */
#define RPC_SPRAYPROG		((u_long) 100012) /* spray packets	     */
#define RPC_IBM3270PROG		((u_long) 100013) /* 3270 mapper	     */
#define RPC_IBMRJEPROG		((u_long) 100014) /* RJE mapper		     */
#define RPC_SELNSVCPROG		((u_long) 100015) /* selection service	     */
#define RPC_RDATABASEPROG	((u_long) 100016) /* remote database access  */
#define RPC_REXECPROG		((u_long) 100017) /* remote execution	     */
#define RPC_ALICEPROG		((u_long) 100018) /* Alice Office Automation */
#define RPC_SCHEDPROG		((u_long) 100019) /* scheduling service      */
#define RPC_LOCKPROG		((u_long) 100020) /* local lock manager      */
#define RPC_NETLOCKPROG		((u_long) 100021) /* network lock manager    */
#define RPC_X25PROG		((u_long) 100022) /* X.25 inr protocol	     */
#define RPC_STATMON1PROG	((u_long) 100023) /* status monitor 1	     */
#define RPC_STATMON2PROG	((u_long) 100024) /* status monitor 2	     */
#define RPC_SELNLIBPROG		((u_long) 100025) /* selection library	     */
#define RPC_BOOTPARAMPROG	((u_long) 100026) /* boot parameters service */
#define RPC_MAZEPROG		((u_long) 100027) /* mazewars game	     */
#define RPC_YPUPDATEPROG	((u_long) 100028) /* yp update		     */
#define RPC_KEYSERVEPROG	((u_long) 100029) /* key server		     */
#define RPC_SECURECMDPROG	((u_long) 100030) /* secure login	     */
#define RPC_NETFWDIPROG		((u_long) 100031) /* NFS net forwarder init  */
#define RPC_NETFWDTPROG		((u_long) 100032) /* NFS net forwarder trans */
#define RPC_SUNLINKMAP_PROG	((u_long) 100033) /* sunlink MAP	     */
#define RPC_NETMONPROG		((u_long) 100034) /* network monitor	     */
#define RPC_DBASEPROG		((u_long) 100035) /* lightweight database    */
#define RPC_PWDAUTHPROG		((u_long) 100036) /* password authorization  */
#define RPC_TFSPROG		((u_long) 100037) /* translucent file svc    */
#define RPC_NSEPROG		((u_long) 100038) /* nse server		     */
#define RPC_NSE_ACTIVATE_PROG	((u_long) 100039) /* nse activate daemon     */

#define RPC_PCNFSDPROG		((u_long) 150001) /* pc passwd authorization */

#define RPC_PYRAMIDLOCKINGPROG	((u_long) 200000) /* Pyramid-locking	     */
#define RPC_PYRAMIDSYS5		((u_long) 200001) /* Pyramid-sys5	     */
#define RPC_CADDS_IMAGE		((u_long) 200002) /* CV cadds_image	     */

#define RPC_ADT_RFLOCKPROG	((u_long) 300001) /* ADT file locking	     */

#ifdef NFSSERVER
/*
 * Classification of NFS procedures.
 */
struct nfs_proc {
	int		nfs_proctype;
	xdrproc_t	nfs_xdrargs;
	int		nfs_argsz;
};

/*
 * NFS procedure argument structures.
 */
union nfs_rfsargs {
	fhandle_t fhandle;
	struct nfssaargs nfssaargs;
	struct nfsdiropargs nfsdiropargs;
	struct nfsreadargs nfsreadargs;
	struct nfswriteargs nfswriteargs;
	struct nfscreatargs nfscreatargs;
	struct nfsrnmargs nfsrnmargs;
	struct nfslinkargs nfslinkargs;
	struct nfsslargs nfsslargs;
	struct nfsrddirargs nfsrddirargs;
};

/*
 * Macros for use with RPC stuff.
 */
#define min(a, b)		((a) < (b) ? (a) : (b))
#define rpc_buffer(xprt)	((xprt)->xp_p1)
#define su_data(xprt)		((struct svcudp_data *)((xprt)->xp_p2))

bool_t	rpcxdr_getargs();	/* get rpc arguments			*/

/*
 * UDP service data.
 */
struct svcudp_data {
	u_int	su_iosz;	/* byte size of send/recv buffer	*/
	u_long	su_xid;		/* transaction id			*/
	XDR	su_xdrs;	/* XDR handle				*/
	char	su_verfbody[MAX_AUTH_BYTES];	/* verifier body	*/
};
#endif
