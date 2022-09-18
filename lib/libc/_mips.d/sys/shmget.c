/* 
 *		@(#)shmget.c	4.1	(ULTRIX)	7/3/90
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
#define SHM_GET 3

shmget(key, size, flag)
key_t key;
int size, flag;
{
	return(shmsys(SHM_GET, (int)key, (int)size, (int)flag));
}
