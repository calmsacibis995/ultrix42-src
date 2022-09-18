#ifndef lint
static char	*sccsid = "@(#)rmstar.c	4.1	(ULTRIX)	11/23/87";
#endif lint

/*
 * rmstar.c
 *
 * Name:	rmstar
 * Purpose:	Remove shared memory segment.
 * Environment:	Ultrix-32, with shared memory.
 * Compile:	see Makefile
 * Date:	April 19 1985
 * Author:	Alan Delorey
 * Remarks:

    These are the voyages of the independent star ships.

Whose lifetime mission: To explore strange new galaxies,
		        To seek out and destroy other star ships,
		        To boldly go where no other star ship dares!

*/

/*
 * Modification history
 *
 */

#include "star.h"
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

int shmid;			/* shared memorey id */
struct univ *uptr;
extern errno;
char *shmat();			/* shared mem attach routine */

main(argc, argv)
    int argc;
    char *argv[];
{
if ((shmid = shmget(ftok("/usr/games/starship", 'a'), sizeof(struct univ), 0666)) < 0)
    {
    printf("%s: shmget failed, errno = %d\n", argv[0], errno);
    perror("rmstar");
    exit();
    }
if ((uptr = (struct univ *) shmat(shmid, 0, 0)) < 0)
    {
    printf("%s: shmat failed, errno = %d\n", argv[0], errno);
    perror("rmstar");
    exit();
    }
if (shmctl(shmid, IPC_RMID) < 0)
    {
    printf("shmctl call to remove shared seg failed\n");
    perror("rmstar");
    exit();
    }
printf("Shared memory segment removed.\n");
}
