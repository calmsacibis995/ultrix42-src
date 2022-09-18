#ifndef lint
static char *sccsid = "@(#)mount.c	4.2	(ULTRIX)	7/17/90";
#endif

/************************************************************************
 *									*
 *			Copyright (c) 1988 by				*
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

#include	"crash.h"
#include	<rpc/types.h>
#include	<sys/mount.h>
#undef export
#include	<nfs/nfs.h>
#include	<nfs/vnode.h>
#include	<netinet/in.h>
#include	<nfs/nfs_clnt.h>
#include	<nfs/vfs.h>
#include	<sys/buf.h>
#include	<sys/fs.h>
#include	<sys/fs_types.h>

extern struct mount *mounttab;

get_fsdata(c, fsd)
	int c;
	struct fs_data *fsd;
{
	struct mount *m;
	
	m = &mounttab[c];
	if (readmem((char *)fsd, (int)m->m_fs_data, sizeof(struct fs_data)) !=
	    sizeof(*fsd)) {
		printf("read error on fs_data for mount slot %d\n", c);
		return(0);
	}
	return(1);
}
	
prmount(c, flag)
	int	c;
	int	flag;
{
	struct	mount	*m;
	struct fs_data  fsd;

	if(c == -1)
		return;
	m = &mounttab[c];
	if (!get_fsdata(c, &fsd))
		return;

	if(flag != 1 && !m->m_fs_data)
		return;
	printf("%4u  %3x  %3.3x  ",
		c, major(m->m_dev), minor(m->m_dev));
	if (flag != 2) {
		printf("%5d  ", m->m_gnodp ? getindex((char *)m->m_gnodp,
		    gnodebuckets, GNODEBUCKETS) : -1);
		printf("%5d  ",  m->m_rootgp ? getindex((char *)m->m_rootgp,
		    gnodebuckets, GNODEBUCKETS) : -1);
		if(m->m_bufp && (m->m_bufp != (struct buf *)-1))
			printf("%4d  ", m->m_bufp ? getindex((char *)m->m_bufp,
			    bufbuckets, BUFBUCKETS) : -1);
		else
			printf("      ");
	}
	printf("%3s ", gt_names[fsd.fd_fstype]);
/*	fflush(stdout);
	printf("%+#0x", m.m_ops);
	fflush(stdout);
*/
	if (flag == 2) {
		printf("%25s", fsd.fd_devname);
		printf("%35s", fsd.fd_path);
	}
	else	printf("%25s", fsd.fd_path);
	if (flag != 2) {
		printf(" %s%s%s%s%s%s%s%s%s\n",
			 fsd.fd_flags & M_RONLY ? " ro" : "",
			 fsd.fd_flags & M_MOD ? " mod" : "",
			 fsd.fd_flags & M_QUOTA ? " q" : "",
			 fsd.fd_flags & M_LOCAL ? " loc" : "",
			 fsd.fd_flags & M_NOEXEC ? " nex" : "",
			 fsd.fd_flags & M_NOSUID ? " nsu" : "",
			 fsd.fd_flags & M_DONE ? " done" : "",
#ifndef M_NOFH
#define M_NOFH 0
#endif
#ifndef M_EXRONLY
#define M_EXRONLY 0
#endif
			 fsd.fd_flags & M_NOFH ? " nofh" : "",
			 fsd.fd_flags & M_EXRONLY ? " exro" : "");
	}
	else printf("\n");
	fflush(stdout);
}

int
prmntinfo(c)
	int c;
{
	struct mntinfo *mi;
	struct fs_data fsd;
	struct v_fs_data *vfsd;

	if (!get_fsdata(c, &fsd)) {
		printf("could not get fsdata for slot %d\n", c);
		return;
	}
	if (fsd.fd_fstype != GT_NFS) {
		printf("filesystem not remote, mount slot %d\n", c);
		return;
	}
	vfsd = (struct v_fs_data *)&fsd;
	mi = &vfsd->fd_un.gvfs.mi;
	mi->mi_hostname[8] = '\0';
	printf(
	       "%-8s%s %s %s %s 0x%x %5d %5d %5d %5d %5d %5d %2d %2d %2d %2d\n",
		mi->mi_hostname,
		mi->mi_hard ? "hard" : "soft",
		mi->mi_down ? "down" : " up ",
		mi->mi_int ? " int " : "noint",
	        mi->mi_noac ? " noac" : " ac  ",
		mi->mi_rootvp,
		mi->mi_refct,
		mi->mi_tsize,
		mi->mi_stsize,
		mi->mi_bsize,
		mi->mi_timeo,
		mi->mi_retrans,
	        mi->mi_acregmin,
	        mi->mi_acregmax,
	        mi->mi_acdirmin,
	        mi->mi_acdirmax);
}

prfsdata(c, all)
	int	c;
	int	all;
{
	struct	mount	*mp;
	struct  fs_data	fd;

	if(c == -1)
		return;
	mp = &mounttab[c];
	if(!all && !mp->m_fs_data)
		return;
	if (!get_fsdata(c, &fd))
		return;

	printf("%7u %7u %7u %7d %7u %3u %25s %s\n", fd.fd_gtot, fd.fd_gfree,
	fd.fd_btot, fd.fd_bfree, fd.fd_bfreen, fd.fd_uid, fd.fd_devname,
	fd.fd_path);
}

int get_mount_slot(s)
	register char *s;
{
	int index;
	int addr;
	
	if((*s == '@') || (*s == '*')) {
		sscanf(++s, "%x", &addr);
		index =	 getindex((char *)addr, mountbuckets, MOUNTBUCKETS);
		if(index == -1)
			printf("addr 0x%x is not a mount\n", addr);
	} else
		if(isdigit(*s))
			index = atoi(s);
		else {
			printf("%s is an invalid token\n", s);
			index = -1;
		}
	return(index);
}


void
printfsdatahd()
{
	printf("\n");
	printf(" GNODES  GNODES  BLOCKS  BLOCKS UBLOCKS\n");
	printf("  TOTAL    FREE   TOTAL    FREE CONSUME UID                   DEVNAME MOUNT POINT\n");
}


void
printmounthd(how)
	int how;
{
	if(how)
		printf("SLOT  MAJ  MIN  GNODE ROOTGP   BUF TYPE                      PATH  FLAGS\n");
	else 
		printf("SLOT  MAJ  MIN TYPE                    DEVICE                        MOUNT POINT\n");
	
}


void
printmntinfohd()
{
	printf("HOST    FLAV STATE INTR     ROOTVP    ACT");
	printf("  TSIZE   STSZ  BSIZE     TO RTRANS\n");
}
