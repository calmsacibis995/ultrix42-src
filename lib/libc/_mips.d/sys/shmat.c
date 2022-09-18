/* 
 *		@(#)shmat.c	4.1	(ULTRIX)	7/3/90
 */

/************************************************************************
 *									*
 *			Copyright (c) 1988 by				*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *                                                                      *
 ***********************************************************************/

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#define SHM_AT 0

caddr_t shmat(shmid, addr, flag)
int shmid;
caddr_t addr;
int flag;
{
	return((caddr_t) shmsys(SHM_AT, shmid, (int)addr, flag));
}
