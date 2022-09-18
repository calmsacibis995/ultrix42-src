/* 
 *		@(#)shmctl.c	4.1	(ULTRIX)	7/3/90
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
#define SHM_CTL 1

shmctl(shmid, cmd, buf)
int shmid;
int cmd;
struct shmid_ds *buf;
{
	return(shmsys(SHM_CTL, shmid, cmd, (int) buf));
}
