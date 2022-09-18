#ifndef lint
static char *sccsid = "@(#)latdate.c	4.1 (ULTRIX)	7/17/90";
#endif lint

/*
 * l a t d a t e
 *
 * Description: This sample program illustrates the use of multiple
 *              lat services.  When a user at a terminal connected 
 *              to a terminal server issues a "CONNECT DATE" command 
 *              the date & time will be printed on his terminal.
 *
 * Setup:       It is necessary to dedicate one or more lat ttys 
 *              to the service.  For example, to dedicate ttys 14 
 *              & 15 you would need to edit /etc/ttys & change the 
 *              lines for tty14 & tty15 to look like:
 *
 *              tty14 "/etc/latdate std.9600" vt100 on 
 *              tty15 "/etc/latdate std.9600" vt100 on 
 *
 *              Then do a "kill -HUP 1" for the change to take effect.  
 *              Then issue an lcp command to advertise the latdlogin 
 *              gateway service:
 *
 *              lcp -v hostname \
 *                  -V "HOSTNAME" \
 *                  -v latdate:/dev/tty14,/dev/tty15 \
 *                  -V "lat date & time service"
 *
 * To compile:  cc -o latdate latdate.c
 *
 * Example:     CONNECT date
 *
 */

#include <errno.h>
#include <sys/file.h>
#include <sys/ioctl.h>

struct sgttyb ttyb;
char dev[256] = "/dev/";
int latfd; 

main(argc, argv)
int	argc;
char	*argv[];

{

    strcat(dev, argv[argc-1]);

    chown(dev, 0, 0);
    chmod(dev, 0622);

    if( (latfd = open(dev, O_RDWR)) < 0 ) {
	perror(dev);
	exit(1);
    }

    ttyb.sg_flags = CRMOD;
    ioctl(latfd, TIOCSETP, &ttyb);

    dup2(latfd, 0);
    dup2(latfd, 1);
    dup2(latfd, 2);

    execl("/bin/date", "lat-date", (char *)0);
}
