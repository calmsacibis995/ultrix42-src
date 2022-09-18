#ifndef lint
static char *sccsid = "@(#)latdlogin.c	4.1 (ULTRIX)	7/17/90";
#endif lint

/*
 * l a t d l o g i n
 *
 * Description: This sample program acts as a LAT to DLOGIN gateway.
 *              With it, a user at a terminal connected to a terminal
 *              server can log into remote DECnet nodes without
 *              having to log into (or even have an account on) the
 *              local system.
 *
 * Setup:       This program requires that DECnet be installed on
 *              your system.  It is necessary to dedicate one or
 *              more lat ttys to the service.  For example, to 
 *              dedicate ttys 14 & 15 you would need to edit
 *              /etc/ttys & change the lines for tty14 & tty15 to look like:
 *
 *              tty14 "/etc/latdlogin std.9600" vt100 on 
 *              tty15 "/etc/latdlogin std.9600" vt100 on 
 *
 *              Then do a "kill -HUP 1" for the change to take effect.  
 *              Then issue an lcp command to advertise the latdlogin 
 *              gateway service:
 *
 *              lcp -v hostname \
 *                  -V "HOSTNAME" \
 *                  -v latdlogin:/dev/tty14,/dev/tty15 \
 *                  -V "lat/dlogin gateway"
 *
 * To compile:  cc -o latdlogin latdlogin.c
 *
 * Example:     To access DLOGIN service from LAT terminal:
 *              CONNECT dlogin NODE hostname DEST DECnet_nodename
 *
 * Comments:    More extensive tty set up could be added (such as
 *              for the parameters defined in gettytap & termcap).  
 *              See getty(8).  See 'Guide to Ethernet Communication 
 *              Servers' for LAT service set up.
 */

#include <sys/ltatty.h>
#include <sys/ioctl.h>
#include <sgtty.h>
#include <ctype.h>
#include <sys/file.h>
#include <stdio.h>

struct  sgttyb mode = { 0, 0, CERASE, CKILL, CRMOD|ECHO };
char    hostname[256];
char    tty[256] = "/dev/";
struct  ltattyi ltainfo;
long    flags = LCRTERA | LCRTBS | LPRTERA;
int     latfd; 
char    *np;


main(argc, argv)
int argc;
char *argv[];

{
    gethostname(hostname, sizeof(hostname));
     
    /* generate full path name to device special file */
    strcat(tty, argv[argc-1]);

    /* change mode & owner of tty */
    chown(tty, 0, 0);
    chmod(tty, 0622);
            
    /* open LAT line */
    latfd = open(tty, O_RDWR);        

    /* get DESTINATION field */ 
    ioctl(latfd, LIOCTTYI, &ltainfo);        

    /* make tty stdin, stdout, & stderr */
    dup2(latfd, 0);
    dup2(latfd, 1);
    dup2(latfd, 2);

    /* set tty flags & mode */
    ioctl(0, TIOCLSET, &flags);
    ioctl(0, TIOCSETP, &mode);

    if (ltainfo.lta_dest_port[0] != 0)
    {
        /* A destination was specified in the connect request. */
        /* Upper-case it, then exec dlogin. */
        printf("\nLAT to DLOGIN gateway on %s connecting to %s\n", 
               hostname,ltainfo.lta_dest_port);
        for (np = ltainfo.lta_dest_port; *np; np++)
        {
            if (isupper(*np))
                *np = tolower(*np);
        }
        execl("/usr/bin/dlogin","dlogin",ltainfo.lta_dest_port,0);
    }
    else 
    {
        /* No destination specified.  Print usage & exit. */
        printf("\nLAT to DLOGIN gateway usage: ");
        printf("CONNECT dlogin NODE %s DEST DECnet_host\n", hostname); 
        close(latfd);
        exit(0);
    }
}

