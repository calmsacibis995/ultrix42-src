#ifndef lint
static char *sccsid = "@(#)kern_loop.c	4.1	(ULTRIX)	7/2/90";
#endif lint

/*
 * kern_loop.c
 *
 * This program enables the redirecting of kernel messages
 * to the /dev/mouse.  Messages will no longer appear on
 * the VAXstar monochrome screen at random.
 *
 * Ali Rafieymehr
 *
 */

#include <stdio.h>
#include <sys/file.h>
#include <vaxuba/smioctl.h>
#include <vaxuba/qevent.h>


extern int	errno;

main()
{
	int fd;

	if ((fd = open("/dev/mouse", O_RDONLY)) < 0) {
	    printf("Failed to open \"/dev/mouse\", errno = %d\n", errno);
	    exit(-1);
	}

	if (ioctl(fd, QIOKERNLOOP) == -1) {
	    printf("ioctl QIOKERNLOOP failed, errno = %d\n", errno);
	    exit(-1);
	}

	close(fd);
}
