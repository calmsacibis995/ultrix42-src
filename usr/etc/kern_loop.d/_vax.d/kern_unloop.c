#ifndef lint
static char *sccsid = "@(#)kern_unloop.c	4.1	(ULTRIX)	7/2/90";
#endif lint

/*
 * kern_unloop.c
 *
 * This program disables the redirecting of kernel messages
 * to the /dev/mouse.  Messages will now appear on
 * the VAXstar monochrome screen at random.
 *
 * Ali Rafieymehr
 *
 */

#include <stdio.h>
#include <sys/file.h>
#include <vaxuba/smioctl.h>

extern int	errno;

main()
{
	int fd;

	if ((fd = open("/dev/mouse", O_RDWR)) < 0) {
	    printf("Failed to open \"/dev/mouse\", errno = %d\n", errno);
	    exit(-1);
	}

	if (ioctl(fd, QIOKERNUNLOOP) == -1) {
	    printf("ioctl QIOKERNUNLOOP failed, errno = %d\n", errno);
	    exit(-1);
	}

	close(fd);
}
