#ifndef lint
static char *sccsid = "@(#)cred.c	4.1	(ULTRIX)	7/17/90";
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

#include "crash.h"
#include <sys/smp_lock.h>
#include <sys/gnode_common.h>
#include <sys/gnode.h>
#define KERNEL
#include <sys/file.h>
#undef KERNEL
#include <sys/proc.h>
#include <rpc/types.h>
#include <sys/mount.h>
#undef export
#include <nfs/nfs.h>
#include <nfs/vnode.h>
#include <nfs/dnlc.h>


#define NUMCREDS 1024

struct crinfo {
	unsigned addr;
	long ref;
};

struct crinfo crtab[NUMCREDS];

int
get_cred(addr, c)
	unsigned addr;
	struct ucred *c;
{

	if (readmem((char *)c, (int)addr, sizeof(*c)) != sizeof(*c)) {
		printf("read error on credentials at 0x%x\n", addr);
		return(0);
	}
	return(1);
}

int
get_crinfo(addr, c)
	unsigned addr;
	struct crinfo **c;
{
	int i;

	for (i = 0; i < NUMCREDS; i++) {
		if (crtab[i].addr == 0)
			break;
		if (crtab[i].addr == addr) {
			*c = &crtab[i];
			return(1);
		}
	}
	if (i == NUMCREDS)
		return(0);
	crtab[i].addr = addr;
	crtab[i].ref = 0;
	*c = &crtab[i];
	return(1);
}

int
make_crtab()
{
	int i;
	struct file *fp;
	struct gnode *gp;
	struct crinfo *c;
	struct rnode *r;
	struct proc *p;
	struct ncache n;

	bzero((char *)crtab, sizeof(crtab));

	/*
	 * Count up all the cred references in the file table.
	 */
	for(i = 0; i < tab[FILE_T].ents; i++) {
		fp = &filetab[i];
		if (fp->f_count == 0)
			continue;
		if (fp->f_cred == 0) {
			printf("File entry %d has null cred\n", i);
			continue;
		}
		if (!get_crinfo((unsigned)fp->f_cred, &c))
			continue;
		c->ref++;
	}

	/*
	 * Count all the cred references in NFS gnodes.
	 */
	for(i = 0; i < tab[GNODE_T].ents; i++) {
		gp = &gnodetab[i];
		if (gp->g_count == 0)
			continue;
		if (!gnode_isremote(i, gp))
			continue;
		r = vtor((struct vnode *)gp);
		if (r->r_cred == (struct ucred *)0) {
			continue;
		}
		if (!get_crinfo((unsigned)r->r_cred, &c))
			continue;
		c->ref++;
	}

	/*
	 * Count all the cred references in process uareas.
	 */
	for(i = 0; i < tab[PROC_T].ents; i++) {
		p = &proctab[i];
		if (p->p_stat == 0)
			continue;
		get_uarea(&proctab[i]);
		if (U.u_cred == (struct ucred *)0) {
			printf("Active uarea %d has null cred\n", i);
			continue;
		}
		if (!get_crinfo((unsigned)U.u_cred, &c))
			continue;
		c->ref++;
	}

	/*
	 * Count all the cred references in the NFS dnlc cache.
	 */
	for(i = 0; i < tab[DNLC_T].ents; i++) {
		if (!get_dnlc(i, &n))
			continue;
		if (!n.cred)
			continue;
		if (!get_crinfo((unsigned)n.cred, &c))
			continue;
		c->ref++;
	}

	return(1);	
}

void
crcheck()
{
	struct ucred cred;
	int i;

	if (!make_crtab())
		return;
	for (i = 0; i < NUMCREDS; i++) {
		if (crtab[i].addr == 0)
			break;
		if (!get_cred(crtab[i].addr, &cred))
			continue;
		if (crtab[i].ref != cred.cr_ref) {
			printf("Cred 0x%x has ref %d should be %d\n",
				crtab[i].addr, cred.cr_ref, crtab[i].ref);
		}
	}
}


/*
 * This is basically a cloned/hacked copy of make_crtab() above,
 * but we just list all references to a particular cred instead
 * of creating a table of reference information for all creds.
 * We also check the reference count on the cred.
 */

void
cred_reflist(addr)
	unsigned addr;
{
	int i;
	struct ucred c;
	int cref = 0;

	struct file *fp;
	struct gnode *gp;
	struct rnode *r;
	struct proc *p;
	struct ncache n;

	if (!get_cred(addr, &c))
		return;

	printf("\n");
	printf("Reference summary for cred struct at 0x%x:\n", addr);

	printf("\nfile:  ");
	for(i = 0; i < tab[FILE_T].ents; i++) {
		fp = &filetab[i];
		if (fp->f_count == 0)
			continue;
		if (fp->f_cred != (struct ucred *)addr)
			continue;
		printf("%8d", i);
		cref++;
	}

	printf("\ngnode: ");
	for(i = 0; i < tab[GNODE_T].ents; i++) {
		gp = &gnodetab[i];
		if (gp->g_count == 0)
			continue;
		if (!gnode_isremote(i, gp))
			continue;
		r = vtor((struct vnode *)gp);
		if (r->r_cred != (struct ucred *)addr) {
			continue;
		}
		printf("%8d", i);
		cref++;
	}

	printf("\nuarea: ");
	for(i = 0; i < tab[PROC_T].ents; i++) {
		p = &proctab[i];
		if (p->p_stat == 0)
			continue;
		get_uarea(&proctab[i]);
		if (U.u_cred != (struct ucred *)addr)
			continue;
		printf("%8d", i);
		cref++;
	}

	printf("\ndnlc:  ");
	for(i = 0; i < tab[DNLC_T].ents; i++) {
		if (!get_dnlc(i, &n))
			continue;
		if (n.cred != (struct ucred *)addr)
			continue;
		printf("%8d", i);
		cref++;
	}

	if (cref == c.cr_ref) {
		printf("\n");
		printf("Credential reference count is correct.\n");
	}
	else {
		printf("\n");
		printf("Cred ref count is %d should be %d\n",c.cr_ref, cref);
	}

	return;
}

int
pr_credhdr()
{
	printf("  REF   UID   GID  RUID  RGID GROUPS\n");
}

int
prcred(c)
	struct ucred *c;
{
	int i;

	printf("ref %5d uid %5d gid %5d ruid %5d rgid %5d  groups: ", c->cr_ref, c->cr_uid,
		c->cr_gid, c->cr_ruid, c->cr_rgid);
	for(i = 0; i < NGROUPS; i++) {
		if (c->cr_groups[i] == -1)
			break;
		printf("%d ", c->cr_groups[i]);
	}
	printf("\n");
}

void
print_cred(addr)
	unsigned addr;
{
	struct ucred c;
	get_cred(addr, &c);
	prcred(&c);
}

/*
 * Read in a NFS dnlc cache entry.  This should be in a file of its
 * own...move it when more dnlc code is added.
 */
int
get_dnlc(i, n)
	int i;
	struct ncache *n;
{
	unsigned addr;

	addr = tab[DNLC_T].first + (i * tab[DNLC_T].size);
	if (readmem((char *)n, (int)addr, sizeof(*n)) != sizeof(*n)) {
		printf("read error on dnlc entry at 0x%x\n", addr);
		return(0);
	}
	return(1);
}
