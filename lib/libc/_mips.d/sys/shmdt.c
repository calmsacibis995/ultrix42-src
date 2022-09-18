/* 
 *		@(#)shmdt.c	4.1	(ULTRIX)	7/3/90
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
#define SHM_DT 2

shmdt(addr)
char *addr;
{
	return(shmsys(SHM_DT, (int)addr));
}
