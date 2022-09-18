#ifndef lint
static char *sccsid = "@(#)nfs_umount.c	4.2	ULTRIX	10/15/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1986 by				*
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
 *	Portions of this software have been licensed to 
 *	Digital Equipment Company, Maynard, MA.
 *	Copyright (c) 1986 Sun Microsystems, Inc.  ALL RIGHTS RESERVED.
 */

/************************************************************************
 *			Modification History
 *
 * 15 Oct 90  -- dws
 *	Fixed bug in UMNTALL handling.
 *
 * 12 Jul 88  -- chet
 *	Added -f (fast!) flag
 *
 ************************************************************************/

/*
 * umount
 */

#include <sys/param.h>
#include <sys/file.h>
#include <stdio.h>
#include <errno.h>
#include <sys/time.h>
#include <rpc/rpc.h>
#include <nfs/nfs.h>
#include <rpcsvc/mount.h>
#include <sys/socket.h>
#include <netdb.h>

struct	mntent{
	char	*mnt_fsname;		/* name of mounted file system */
	char	*mnt_dir;		/* file system path prefix */
	char	*mnt_type;		/* MNTTYPE_* */
	char	*mnt_opts;		/* MNTOPT* */
	int	mnt_freq;		/* dump frequency, in days */
	int	mnt_passno;		/* pass number on parallel fsck */
};


int	all = 0;
int	verbose = 0;
int	notify = 1;

extern	int errno;

main(argc, argv)
	int argc;
	char **argv;
{
	char *options;

	argc--, argv++;
	if (argc && *argv[0] == '-') {
		options = &argv[0][1];
		while (*options) {
			switch (*options) {
			case 'v':
				verbose++;
				break;
			case 'b':
				unmountall();
				exit(0);
			case 'f':	/* no server notification */
				notify = 0;
				break;
			default:
				fprintf(stderr, "nfs_umount: unknown option '%c'\n",
				    *options);
				usage();
			}
			options++;
		}
		argv++;
		argc--;
	}

	umountlist(argc, argv);
}

umountlist(argc, argv)
int argc;
char *argv[];
{
	struct mntent mnt;
	int i;
	
	for (i=0; i<argc; i++) {
		mnt.mnt_fsname = argv[i];
		umountmnt(&mnt);
	}
}

umountmnt(mnt)
struct mntent *mnt;
{
	if (notify)
		rpctoserver(mnt);
	if (verbose) {
		fprintf(stderr, "%s: Unmounted\n", mnt->mnt_fsname);
	}
	return(1);
}

usage()
{
	fprintf(stderr, "usage: nfs_umount [-v] <host:fsname>\n");
	exit(1);
}

rpctoserver(mnt)
	struct mntent *mnt;
{
	char *p, *index();
	struct sockaddr_in sin;
	struct hostent *hp;
	int s = -1;
	struct timeval timeout;
	CLIENT *client;
	enum clnt_stat rpc_stat;
		
	if ((p = index(mnt->mnt_fsname, ':')) == NULL) {
		fprintf(stderr,
		"%s not in nfs server hostname format\n", mnt->mnt_fsname);
		return(1);
	}
	*p++ = 0;
	if ((hp = gethostbyname(mnt->mnt_fsname)) == NULL) {
		fprintf(stderr, "%s not in hosts database\n", mnt->mnt_fsname);
		return(1);
	}
	bzero(&sin, sizeof(sin));
	bcopy(hp->h_addr, (char *) & sin.sin_addr, hp->h_length);
	sin.sin_family = AF_INET;
	timeout.tv_usec = 0;
	timeout.tv_sec = 10;
	if ((client = clntudp_create(&sin, MOUNTPROG, MOUNTVERS,
	    timeout, &s)) == NULL) {
		clnt_pcreateerror("Warning: umount");
		return(1);
	}

	/*
	 * get my hostname that matches my network address.
	 * subnets give us more than one hostname.
	 * If server's mountd has ipaddr_check on, we need this
	 */

	{
        register int len;
        register int uid;
        register int gid;
        int gids[NGROUPS];
        struct hostent *machname;
        struct sockaddr_in addr;
        struct hostent myhname;
        char hnam[MAX_MACHINE_NAME + 1];

        get_myaddr_dest(&addr,&sin);

        if ((machname = gethostbyaddr((char *) &addr.sin_addr.s_addr,4,AF_INET)) == NULL )
                if ( gethostname(hnam, MAX_MACHINE_NAME) < 0) {
                        fprintf(stderr, "rpctoserver: gethostname failed\n");
                        abort();

                } else {
                        hnam[MAX_MACHINE_NAME] = 0;
                        myhname.h_name = hnam;
                        machname = &myhname;
                }
        
        uid = geteuid();
        gid = getegid();
        if ((len = getgroups(NGROUPS, gids)) < 0){
                perror("rpctoserver: getgroups: ");
                abort();
        }
        if(len > NGRPS)  /* rpc/xdr only allows NGRPS groups */
                len = NGRPS;
        client->cl_auth = authunix_create(machname->h_name, uid, gid, len, gids);
	}

	timeout.tv_usec = 0;
	timeout.tv_sec = 25;
	rpc_stat = clnt_call(client, MOUNTPROC_UMNT, xdr_path, &p,
	    xdr_void, NULL, timeout);
	if (rpc_stat != RPC_SUCCESS) {
		clnt_perror(client, "Warning: umount");
		return(1);
	}
}

/*
 * Broadcast on the net saying that we're unmounting everything.  This is
 * so that the servers can bring their /etc/rmtab back up to date after a
 * client crash.
 */

unmountall()
{
        enum clnt_stat rpc_stat, clnt_broadcast();

        rpc_stat = clnt_broadcast(MOUNTPROG, MOUNTVERS, MOUNTPROC_UMNTALL,
		    xdr_void, NULL, xdr_void, NULL, NULL);

        if (rpc_stat != RPC_SUCCESS)
                fprintf(stderr, "Warning: umountall failed(%d).\n", rpc_stat);
}

