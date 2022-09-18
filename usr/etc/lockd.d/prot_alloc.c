
# ifndef lint
static char *sccsid = "@(#)prot_alloc.c	4.1	(ULTRIX)	7/2/90";
# endif not lint

/****************************************************************
 *								*
 *  Licensed to Digital Equipment Corporation, Maynard, MA	*
 *		Copyright 1985 Sun Microsystems, Inc.		*
 *			All rights reserved.			*
 *								*
 ****************************************************************/
/**/
/*
 *	Modification history:
 *	~~~~~~~~~~~~~~~~~~~~
 *
 *	revision			comments
 *	--------	-----------------------------------------------
 *
 *	01-Jun-89	Fred Glover
 *			Update for nfssrc 4.0
 *
 *	22-Nov-88	Fred Glover
 *			Modify xmalloc() to call to calloc()
 *
 *	18-Jan-88	fries
 *			Added Header and Copyright notice.
 *
 *	
 */

/* prot_alloc.c
 * consists of routines used to allocate and free space
 */

#include <stdio.h>
#include "prot_lock.h"

extern struct fs_rlck *rel_fe, *rel_me;			/* free indicator*/
int used_le, used_fe, used_me, used_res;		/* # of entry used*/
extern struct fs_rlck  *grant_q;
extern struct fs_rlck  *monitor_q;
extern void dequeue_lock();
char *xmalloc(), *malloc();
extern reclock *get_le();
extern remote_result *get_res();
extern struct fs_rlck *get_me(), *get_fe();
extern int lock_len, res_len;
extern int debug;

void
xfree(a)
	char **a;
{
	if (*a != NULL) {
		free(*a);
		*a = NULL;
	}
}

release_res(resp)
	remote_result *resp;
{
	xfree(&resp->cookie_bytes); 
	free((char *) resp);
}

free_le(a)
	reclock *a;
{
	used_le--;
	/* 
	 * Make sure that this lock is not queued on msg_q.
	 * This addresses bugs 1012630 and 1011992, though
	 * the cause remains unknown.
	 */
	dequeue_reclock(a);
	/* free up all space allocated through malloc */
	xfree(&a->lck.svr);
	xfree(&a->lck.fh_bytes);
	xfree(&a->lck.caller_name);
	xfree(&a->lck.oh_bytes);
	xfree(&a->cookie_bytes);
	xfree(&a->lck.clnt_name);
	bzero((char *) a, sizeof (*a));
	free((char *) a);
}

release_le(a)
	reclock *a;
{
	if (a->rel == 1) {		/* release bit is on */
		if (add_mon(a, 0) == -1)
			fprintf(stderr, "release_le: add_mon failed\n");
		if (a->pre_le != NULL) {
			if (debug)
				printf("release_le: pre_le not free yet\n");
			free_le(a->pre_le);
		}
		if (a->pre_fe != NULL) {
			if (debug)
				printf("release_le: pre_fe not free yet\n");
			free_fe(a->pre_fe);
		}
		free_le(a);
	}
}

free_fe(fp)
	struct fs_rlck *fp;
{
	used_fe--;
	xfree(&fp->svr);
	xfree(&fp->fs.fh_bytes);
	free((char *) fp);
}


/*
 * allocate space and zero it;
 * in case of malloc error, print console msg and return NULL;
 */
char *
xmalloc(len)
	unsigned len;
{
	char *calloc ();

	return (calloc (len, sizeof (char)));  /* Ultrix mod - fsg */

}


/*
 * these routines are here in case we try to optimize calling to malloc 
 */
reclock *
get_le()
{
	used_le ++;
	return( (reclock *) xmalloc(lock_len) );
}

struct fs_rlck *
get_fe()
{
	used_fe ++;
	return( (struct fs_rlck *) xmalloc(sizeof(struct fs_rlck)) );
}

struct fs_rlck *
get_me()
{
	used_me ++;
	return( (struct fs_rlck *) xmalloc(sizeof(struct fs_rlck)) );
}

remote_result *
get_res()
{
	used_res ++;
	return( (remote_result *) xmalloc(res_len) );
}

