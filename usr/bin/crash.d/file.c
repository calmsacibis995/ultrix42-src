#ifndef lint
static char *sccsid = "@(#)file.c	4.4	(ULTRIX)	3/7/91";
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
#define KERNEL
#include	<sys/file.h>
#undef KERNEL
#include	<sys/smp_lock.h>
#include	<sys/gnode_common.h>
#include	<ufs/ufs_inode.h>
#include	<sys/gnode.h>
#include	<sys/namei.h>
#include	<rpc/types.h>
#include	<sys/mount.h>
#include	<sys/text.h>
#include	<sys/proc.h>
#include	<machine/pte.h>
#include	<machine/param.h>
#include	<machine/vmparam.h>
#include	<sys/vmmac.h>

#undef export
#define NFSSERVER
#include	<nfs/nfs.h>
#undef	NFSSERVER
#include	<rpc/rpc.h>
#include	<nfs/vnode.h>
#include	<nfs/dnlc.h
#include	<time.h>

#define GT_NFS 0x05	/* can't include <sys/fs_types.h> here */


struct ucred	cred;

pr_filehdr()
{
	printf(" Slot Type  Ref  Msg   Cred     Islot    Fileops  Offset   Flags\n");
}

prfile(c, all)
	int c;
	int	all;
{
	struct	file	*fp;
	
	fp = &filetab[c];
	if(!all && fp->f_count == 0)
		return;

	printf ("%5d ", c);

	/* These types are defined in sys/file.h but are #ifdef'ed out. */
	switch (fp->f_type) {
		default:
			printf ("*%3d",fp->f_type);
			break;
		case 1:
			printf ("file");
			break;
		case 2:
			printf ("sock");
			break;
		case 3:
			printf ("npip");
			break; /* DTYPE_PORT */
		case 4:
			printf ("pipe");
			break; /* DTYPE_PIPE */
	}

	printf(" %4d %4d %8x", fp->f_count, fp->f_msgcount, fp->f_cred);
	printf(((fp->f_data != NULL) & (fp->f_type != 2) & (fp->f_type != 4))?
	    "%8d " : "     --- ",
	    getindex((char *)fp->f_data, gnodebuckets, GNODEBUCKETS));
	if (fp->f_ops == NULL)
		printf("  *Null*  ",fp->f_ops);
	else {
		struct Symbol *sp, *search();
		sp = search(fp->f_ops);
		if (sp != 0)
			if(sp->s_name[0] == '_')
				printf("%10s", &(sp->s_name[1]));
			else
				printf("%10s", sp->s_name);
		else
			printf("0x%08x",fp->f_ops);
	}
	printf(" %6d ",fp->f_offset);
	printf("  %s%s%s%s%s%s%s%s%s\n",
	    fp->f_flag & FREAD ? " read" : "",
	    fp->f_flag & FWRITE ? " write" : "",
	    fp->f_flag & FAPPEND ? " append" : "",
	    fp->f_flag & FNDELAY ? " ndelay" : "",
	    fp->f_flag & FMARK ? " mark" : "",
	    fp->f_flag & FDEFER ? " defer" : "",
	    fp->f_flag & FASYNC ? " async" : "",
	    fp->f_flag & FSHLOCK ? " shlck" : "",
	    fp->f_flag & FEXLOCK ? " exlck" : "");
}

prgnode(c, operation, match, detail)
	int	c;
	int	operation;
	int	match;
	int 	detail;
{
	int mntnum;
	struct gnode *gp;
	
	gp = &gnodetab[c];
	switch (operation) {
	case 0:
		if(gp->g_count == 0) return;
		printgnode(gp, c, detail);
		break;
	case 1:
		printgnode(gp, c, detail);
		break;
	case 2:
		if (major(gp->g_dev) == match)
			printgnode(gp, c, detail);
		break;
	case 3:
		if (minor(gp->g_dev) == match)
			printgnode(gp, c, detail);
		break;
	case 4:
		mntnum =
		    getindex((char *)gp->g_mp, mountbuckets, MOUNTBUCKETS);
		if (mntnum == match)
			printgnode(gp, c, detail);
		break;
	case 5:
		if (gp->g_number == match)
			printgnode(gp, c, detail);
		break;
	case 6:
		if (gp->g_uid == match)
			printgnode(gp, c, detail);
		break;
	case 7:
		if (gp->g_gid == match)
			printgnode(gp, c, detail);
		break;
	case 8:
		if (gp->g_mode &  match)
			printgnode(gp, c, detail);
		break;
	case 9:
		if (gp->g_mode &  match)
			printgnode(gp, c, detail);
		break;
	case 10:
		if (gp->g_mode == match)
			printgnode(gp, c, detail);
		break;
	case 11:
		if (gp->g_lk.l_lock)
			printgnode(gp, c, detail);
		break;

	}
	return;
}

prgnodelist(gp, detail)
	register struct gnode *gp;
	int detail;
{
	struct gnode *tgp;
	int index;

	if(readmem((char *)&tgp, (int)gp, sizeof(tgp))!= sizeof(tgp)) {
		printf("could not read gnode gp 0x%x\n", gp);
		return;
	}
	
	gp = tgp;
	while(gp != NULL) {
		if((index = getindex((char *)gp, gnodebuckets,
		    GNODEBUCKETS)) == -1) {
			printf("could not read gnode gp 0x%x\n", gp);
			return;
		}
		printgnode(&gnodetab[index], index, detail);
		gp = gnodetab[index].g_freef;
	}
}

printgnode(gp, c, detail)
	struct gnode *gp;
	int c;
	int detail;
{
	register char	ch;
	int mntnum;
	int index1 = 0;
	int index2 = 0;
	
	if(gp->g_count == 0) {
		index1 = getindex((char *)gp->g_freef, gnodebuckets,
		    GNODEBUCKETS);
		index2 = getindex((char *)gp->g_freeb, gnodebuckets,
		    GNODEBUCKETS);
	} 
	mntnum = getindex((char *)gp->g_mp, mountbuckets, MOUNTBUCKETS);
	
	printf("%4d %3.3x %4.4x%3d %6d %3d %4d %4d %4d %7ld",
	    c, major(gp->g_dev), minor(gp->g_dev), mntnum,
	    ((gp->g_number  < 1000000) && (gp->g_number > 0)) ?
	    gp->g_number : -1, gp->g_count, gp->g_nlink, gp->g_uid,
	    gp->g_gid, gp->g_size);
	
	if(index1)
		printf("%4d ", index1);
	else
		printf("     ");
	if(index2)
		printf("%4d ", index2);
	else
		printf("     ");

	switch(gp->g_mode & GFMT) {
		case GFDIR:
			ch = 'd';
			break;
		case GFCHR:
			ch = 'c';
			break;
		case GFBLK:
			ch = 'b';
			break;
		case GFREG:
			ch = 'f';
			break;
		case GFLNK:
			ch = 'l';
			break;
		case GFSOCK:
			ch = 's';
			break;
		default:
			ch = '?';
	}
	
	printf(" %c", ch);
	printf("%s%s%s%3o",
	    gp->g_mode & GSUID ? "u" : "-",
	    gp->g_mode & GSGID ? "g" : "-",
	    gp->g_mode & GSVTX ? "v" : "-",
	    gp->g_mode & 0777);
	printf("%s%s%s%s%s%s%s%s%s%s%s%s%s\n"," ",
	    gp->g_flag & GINCOMPLETE ? " inc" : "",
	    gp->g_lk.l_lock ? " lck" : "", 
	    gp->g_flag & GUPD ? " upd" : "",
	    gp->g_flag & GACC ? " acc" : "",
	    gp->g_flag & GMOUNT ? " mnt" : "",
	    gp->g_flag & GWANT ? " wnt" : "",
	    gp->g_flag & GTEXT ? " txt" : "",
	    gp->g_flag & GCHG ? " chg" : "",
	    gp->g_flag & GSHLOCK ? " sh" : "",
	    gp->g_flag & GEXLOCK ? " x" : "",
	    gp->g_flag & GLWAIT ? " wt" : "",
	    gp->g_flag & GMOD ? " mod" : "");
	if (detail == 2) {
		prlock_long(&gp->g_lk,0);
	}
	if (detail == 3) {
		prgnode_long(gp);
	}

}

int
gnode_isremote(i, gp)
	int i;
	struct gnode *gp;
{
	struct fs_data	fsd;
	int		mntnum;

	if((mntnum = getindex((char *)gp->g_mp, mountbuckets,
	    MOUNTBUCKETS)) == -1) {
		printf("gnode %i has bad mount point 0x%x\n", i, gp->g_mp);
		return(0);
	}
	if (!get_fsdata(mntnum, &fsd))
		return(0);
	if (fsd.fd_fstype != GT_NFS) {
		return(0);
	}
	return(1);
}
	    
prvnode(c, all)
	register int		c;
	int			all;

{
	struct rnode	*r;
	struct gnode *gp;
	struct tm *attr;
	
	gp = &gnodetab[c];
	if (!all && gp->g_count==0)
		return;
	if (!gnode_isremote(c, gp))
		return;

	r = vtor((struct vnode *)gp);
	if (r->r_cred != NULL)
	if (!get_cred((unsigned)(r->r_cred), &cred))
		return;

	printf("%3d %5u %5u %10u",c,r->r_fh.fh_fsid,
	    r->r_fh.fh_fno, r->r_fh.fh_fgen);
	printf(" %5d %5d %5d %6d %6d",cred.cr_ref,cred.cr_uid,
	    cred.cr_gid,cred.cr_ruid,cred.cr_rgid);
	printf(" %5d", (int)r->r_error);
	printf(" %s%s%s%s%s%s\n",
	    r->r_flags & RLOCKED ? "lck " : "",
	    r->r_flags & RWANT ? "wnt " : "",
	    r->r_flags & RATTRVALID ? "val " : "",
	    r->r_flags & REOF ? "eof " : "",
	    r->r_flags & RDIRTY ? "drt " : "",
	    r->r_flags & ROPEN ? "opn " : "");
	printf("cred 0x%x vfsp 0x%x \n", r->r_cred, ((struct vnode *)gp)->g_in.ve.V_vfsp);
	printf("unlname 0x%x, unldvp 0x%x", r->r_unlname, r->r_unldvp);

	attr=localtime(&r->r_nfsattrtime);
	printf(" nfsattrtime %dh %dm %d.%06ds\n",attr->tm_hour,attr->tm_min,
	       attr->tm_sec,r->r_nfsattrtime.tv_usec);
}

do_ufile(c)
	int c;
{
	int i, f, fp;
	char fflg;
	int *ofile_of;
	char *pofile_of;

	/* Should also dump the inodes. */
	getuarea(c);
	printf ("FD  UFLAG  ");
	pr_filehdr();
	if (U.u_of_count != 0) {
		ofile_of = (int *) malloc(U.u_of_count*
					  sizeof(int));
		pofile_of = (char *) 
			malloc(U.u_of_count);
		
		if (readmem((char *)ofile_of, (int)U.u_ofile_of, 
			    U.u_of_count*sizeof(int)) !=
		    U.u_of_count*sizeof(int)) {
			printf(" read error on fd overflow area\n");
			return;
		}
		if (readmem((char *)pofile_of, (int)U.u_pofile_of, 
			    U.u_of_count) !=
		    U.u_of_count) {
			printf(" read error on fd overflow area\n");
			return;
		}
	}
	for (i = 0; i <= U.u_omax; ++i) {
		if (i > NOFILE_IN_U) {
			fp = (int) ofile_of [i - NOFILE_IN_U];
			fflg = pofile_of [i - NOFILE_IN_U];
		} else {
			fp = (int)U.u_ofile[i];
			fflg = U.u_pofile[i];
		}
			
		if (fp == 0) continue;
		f = getindex((char *)fp, filebuckets, FILEBUCKETS);
		printf ("%2d    ",i);
		printf ("%s%s%s  ",
		    (fflg & UF_EXCLOSE) ? "x" : "-",
		    (fflg & UF_MAPPED) ? "m" : "-",
		    (fflg & UF_INUSE) ? "i" : "-");
		f = getindex((char *)fp, filebuckets, FILEBUCKETS);
		prfile(f, 1);
	}
}

prblocks (c)		/* print disk blocks associated with inode */
	int c;
{
	int i;
	struct gnode *gp;
	
	gp = &gnodetab[c];
	printf ("\n");
	printf ("gnode number %d (slot %d):\n", gp->g_number, c);
	printf ("\n");
	printf ("direct:     ");
	for (i = 0; i < NDADDR; i++) {
		printf("%8d", G_TO_I(gp)->di_db[i]);
		if (i % 6 == 5)
			printf("\n            ");
	}
	/* maybe should list blocks referenced by indir blocks */
	printf ("\nindirect:   ");
	for (i = 0; i < NIADDR; i++) {
		printf("%8d", G_TO_I(gp)->di_ib[i]);
		if (i % 6 == 5)
			printf("\n            ");
	}
	printf("\n");
}

int get_file_slot(s)
	register char *s;
{
	int index;
	int addr;
	
	if((*s == '@') || (*s == '*')){
		sscanf(++s, "%x", &addr);
		index =	 getindex((char *)addr, filebuckets, FILEBUCKETS);
		if(index == -1)
			printf("addr 0x%x is not a file\n", addr);
	} else
		if(isdigit(*s)) {
			index = atoi(s);
		} else
			index = -1;
	return(index);
}

int get_gnode_slot(s)
	register char *s;
{
	int index;
	int addr;
	
	switch(*s) {
		case '@' :
		case '*' :
			sscanf(++s, "%x", &addr);
			index =	 getindex((char *)addr, gnodebuckets,
			    GNODEBUCKETS);
			if(index == -1)
				printf("addr 0x%x is not a gnode\n", addr);
			break;
		case '#' :
			sscanf(++s, "%d", &addr);
			index = gnum_to_slot(addr);
			if(index == -1)
				printf("cannot find slot %d\n", addr);
			break;
		default:
			if(isdigit(*s))
				index = atoi(s);
			else {
				printf("%s is an invalid token\n", s);
				index = -1;
			}
	}
	return(index);
}

int
gnum_to_slot(gnum)
	int gnum;
{
	int slot;

	for (slot = 0; slot < tab[GNODE_T].ents; slot++) {
		if ((int)gnodetab[slot].g_number == gnum)
			return (slot);
	}
	return (-1);
}



void
printrnodehd() 
{
	printf("          FHANDLE            EFFECTIVE       REAL   ");
	printf(" AS WR      \n");
	printf("SLOT FSID    NO  GEN   REF   UID   GID    UID    GID");
	printf(" ERROR FLAGS\n");
}

void
printgnodehd() {
	printf("SLOT MAJ  MIN FS  GNUMB REF LINK  UID  ");
	printf("GID    SIZE FORW BACK    MODE  FLAGS\n");
}


prgnode_long(gp)
	struct gnode *gp;
{
	printf("g_chain[2] - 0x%x 0x%x (",gp->g_chain[0],gp->g_chain[1]);
	if (gp->g_chain[0] != NULL) {
		praddr(gp->g_chain[0]);
		printf("  ");
	}
	if (gp->g_chain[1] != NULL) {
		praddr(gp->g_chain[1]);
		printf("  ");
	}
	printf(")\n\n");

	printf("rdev %d %d (0x%x) ", (gp->g_rdev>>8),(gp->g_rdev&0377),
	       gp->g_rdev);
	printf("g_shlockc %d g_exlockc %d\n", gp->g_shlockc, gp->g_exlockc);
	printf("g_mp -");
	praddr(gp->g_mp);
	printf(" g_ops - ");
	praddr(gp->g_ops);
	printf(" g_altops -");
	praddr(gp->g_altops);
	printf("\n");

	printf("g_dquot 0x%x g_blocks %d g_gennum %d\n",
	       gp->g_dquot, gp->g_blocks, gp->g_gennum);

	printf("\n Initialization state: - ");
	switch (gp->g_init) {
		default:
			printf (" *Unknown*\n");
			break;
		case READY_GNODE:
			printf (" Ready\n");
			break;
		case RECLAIM_GNODE:
			printf (" Reclaim\n");
			break;
		case NEW_GNODE:
			printf (" New\n");
			break; 
	}
	
	prlock_long(&gp->g_lk);

	printf("\nRead-ahead %d\n",gp->g_lastr);

	if (gp->g_flag & GTEXT) {
		printf(" Text branch:\n");
		printf("g_text ");
		praddr(gp->g_textp);
		printf(" Cmap entry ");
		praddr(gp->g_hcmap_struct);
		printf("\n");
		printf(" xcount %d  hcount %d hcmap 0x%x\n",
		       gp->g_xcount, gp->g_hcount, gp->g_hcmap);
	}
	if ((gp->g_mode & GFMT) == GFSOCK) {
		printf(" Socket: \n");
		praddr(gp->g_socket);
		printf(" (0x%x)\n",gp->g_socket);
	}

	if (gp->g_count == 0) {
		printf("Free list: 0x%x 0x%x (",gp->g_forw,gp->g_back);
		if (gp->g_forw != NULL) {
			praddr(gp->g_forw);
			printf("  ");
		}
		if (gp->g_back != NULL) {
			praddr(gp->g_back);
			printf("  ");
		}
		printf(")\n");
	}
	printf("\n");
}
#define EX_RDONLY 0x01 

prexport()
{
	unsigned ep, enxt;
	struct export e;
	char path[MAXPATHLEN];

	readsym(symsrch("_exported"), &ep, sizeof(ep));

	if (ep == NULL) {
		printf("Empty List\n");
		return;
	} 
	printf(" fsid     inum       gen    map flags path\n");
/*              dddddd ddddddddd dddddddddd ddd ss    ppppppppppppppp */
	do {
		get_export(ep, &e);

		if (readmem((char *)path, (int)e.e_path, e.e_pathlen) 
		    != e.e_pathlen) {
			perror("export path read");
			printf("read error on export path at 0x%x\n", e.e_path);
			return(0);
		}

		printf("%5d %10d %10d %3d %2s    %s\n",
		       e.e_fsid, e.e_gnum, e.e_gen, e.e_rootmap,
		       e.e_flags & EX_RDONLY ? "ro" : "  ",
		       path);


		ep = (unsigned)e.e_next;

	} while (ep != NULL);
}

int
get_export(addr, ep)
	unsigned addr;
	struct export *ep;
{

	if (readmem((char *)ep, (int)addr, sizeof(*ep)) 
	    != sizeof(*ep)) {
		perror("export read");
		printf("read error on export at 0x%x\n", addr);
		return(0);
	}
	return(1);
}

pr_dnlc()
{
	struct ncache nc;
	struct ucred cred;
	unsigned addr, size;
	char name[50];
	int i;

	readsym(symsrch("_ncache"), &addr, sizeof(addr));
	readsym(symsrch("_ncsize"), &size, sizeof(size));
	printf("ncache: 0x%x, cache size %d entries\n", addr, size);
	printf("Entry       Name        uid      cred  gnode parent\n");
/*              ddddd nnnnnnnnnnnnnnn ddddd 0xhhhhhhhh xxxxx xxxxx */
	for (i=0; i<size; i++) {
		if (readmem((char *)&nc, (int) addr, sizeof(nc))
				    != sizeof(nc)) {
			printf("read error on dnlc at 0x%x\n", addr);
			perror("dnlc entry read");
			return(0);
		}
		if (nc.vp != NULL) {
			readmem((char *)&cred, (int) nc.cred, sizeof(cred));
			strncpy(name, nc.name, nc.namlen);
			name[nc.namlen]='\0';
			printf("%5d %15s %5d 0x%x %5d %5d\n",
			       i, name,cred.cr_uid, nc.cred,
			       getindex((char *)nc.vp, 
					gnodebuckets, GNODEBUCKETS),
			       getindex((char *)nc.dp,
					gnodebuckets, GNODEBUCKETS));
		}
		addr += sizeof(struct ncache);
	}

}


prnamei(index)
	int index;
{
	int	nchsize,i;
	struct nch *np, nch;
	char name[NCHNAMLEN];
	unsigned addr;
	
	readsym(symsrch("_nch"), &addr, sizeof(addr));
	readsym(symsrch("_nchsize"), &nchsize, sizeof(nchsize));

	printf("ncache: 0x%x, cache size %d entries\n", addr, nchsize);
	printf("Entry       Name        dev   inode    idev    id     gnode\n");
/*              ddddd nnnnnnnnnnnnnnn ddd ddd dddddd ddd ddd dddddd  dddddd*/

	if (index == -1) {
		for (i=0; i<nchsize; i++) {
			if (readmem((char *)&nch, (int) addr, sizeof(nch))
			    != sizeof(nch)) {
				printf("read error on namei cache at 0x%x\n", addr);
				perror("namei cache entry read");
				return(0);
			}
			if ((nch.nc_ip != NULL) && (nch.nc_ino != 0)) {
				strncpy(name, nch.nc_name, nch.nc_nlen);
				name[nch.nc_nlen]='\0';
				printf("%5d %15s %3d %3d %6d %3d %3d %6d %6d\n",
				       i, name, 
				       major(nch.nc_dev),minor(nch.nc_dev), 
				       nch.nc_ino,
				       major(nch.nc_idev),minor(nch.nc_idev), 
				       nch.nc_id,
				       getindex((char *)nch.nc_ip, 
						gnodebuckets, GNODEBUCKETS));
			}
			addr += sizeof(struct nch);
		}
	} else {
		if ((index < 0) || (index > nchsize)) {
			printf("bad index (%d) in namei cache\n", index);
			perror("bad namei cache index");
			return(0);
		}
		if (readmem((char *)&nch, (int) addr+index*sizeof(nch), 
			    sizeof(nch)) != sizeof(nch)) {
			printf("read error on namei cache at 0x%x\n", addr);
			perror("namei cache entry read");
			return(0);
		}
		if ((nch.nc_ip != NULL) && (nch.nc_ino != 0)) {
			strncpy(name, nch.nc_name, nch.nc_nlen);
			name[nch.nc_nlen]='\0';
			printf("%5d %15s %3d %3d %6d %3d %3d %6d %6d\n",
			       index, name, 
			       major(nch.nc_dev),minor(nch.nc_dev), 
			       nch.nc_ino,
			       major(nch.nc_idev),minor(nch.nc_idev), 
			       nch.nc_id,
			       getindex((char *)nch.nc_ip, 
					gnodebuckets, GNODEBUCKETS));
		}
	}
}

/* find all the ref's to a gnode */
pr_gref(index) 
	int index;
{
	struct gnode *gp;
	int i, j, gref, f;
	struct	file	*fp;
	struct	text	tbuf;
	struct	proc	*p;
	struct 	ncache 	n;
	struct	mount	*m;
	struct 	fs_data	fsd;

	gp = &gnodetab[index];
	gref=0;

	printf("\n");
	printf("Reference summary for gnode[%d]\n", index);
	printf("gnode has %d refs\n",gp->g_count);
	printf("\nfile:  ");
	for(i = 0; i < tab[FILE_T].ents; i++) {
		fp = &filetab[i];
		if (fp->f_count == 0)
			continue;
		if (getindex(fp->f_data, gnodebuckets, GNODEBUCKETS) != index)
			continue;
		printf("%8d", i);
	}
	printf("\nuarea: ");
	for(i = 0; i < tab[PROC_T].ents; i++) {
		p = &proctab[i];
		if (p->p_stat == 0)
			continue;
		get_uarea(&proctab[i]);
		if (getindex(U.u_cdir, gnodebuckets, GNODEBUCKETS) == index) {
			printf("%d - cdir ",i);
			gref++;
		}
		if (getindex(U.u_rdir, gnodebuckets, GNODEBUCKETS) == index) {
			printf("%i - rdir ",i);
			gref++;
		}
		for(j = 0; j < NOFILE; j++) {
			if (U.u_ofile[j] == NULL)
				continue;
			f = getindex((char *)U.u_ofile[j], filebuckets, FILEBUCKETS);
			if (filetab[f].f_type != DTYPE_INODE)
				continue;

			if (getindex(filetab[f].f_data, gnodebuckets, GNODEBUCKETS) == index) {
				printf("%d - ofile[%d] ",i, j);
				gref++;
			}
		}
	}
	printf("\ntext: ");
	
	for(i = 0; i < tab[TEXT_T].ents; i++) {
		if(readmem((char *)&tbuf,(int)(tab[TEXT_T].first + i*sizeof tbuf),
			   sizeof tbuf) != sizeof tbuf) {
			printf("%4d  read error on text table\n", i);
			return;
		}
		if (tbuf.x_gptr == NULL)
			continue;
		if (getindex((char *)tbuf.x_gptr,gnodebuckets, GNODEBUCKETS) == index) {
			printf(" %d",i);
			gref++;
		}

	}

	printf("\ndnlc:  ");

	for(i = 0; i < tab[DNLC_T].ents; i++) {
		if (!get_dnlc(i, &n))
			continue;

		if (getindex((char *)n.vp,gnodebuckets, GNODEBUCKETS) == index) {
			printf("%d - vp ", i);
			gref++;
		}
		if (getindex((char *)n.dp,gnodebuckets, GNODEBUCKETS) == index) {
			printf("%d - dp ", i);
			gref++;
		}
	}
	printf("\nmount:  ");
	for(i = 0; i < tab[MOUNT_T].ents; i++) {
		m = &mounttab[i];
		if ((m->m_flgs & MTE_DONE) ==0)
			continue;
		if (getindex((char *)m->m_gnodp,gnodebuckets, GNODEBUCKETS) == index) {
			printf("%d - gnodp ", i);
			gref++;
		}
		if (getindex((char *)m->m_rootgp,gnodebuckets, GNODEBUCKETS) == index) {
			printf("%d - root ", i);
			gref++;
		}
		if (getindex((char *)m->m_qinod,gnodebuckets, GNODEBUCKETS) == index) {
			printf("%d - quota ", i);
			gref++;
		}
	}

	
	printf("\n Found %d refs, %d still unaccounted\n",
	       gref, gp->g_count-gref);
}

