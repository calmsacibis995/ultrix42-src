#ifndef lint
static  char  *sccsid = "@(#)dli_ethd.c	4.1	(ULTRIX)	7/17/90";
#endif lint

#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <netdnet/dli_var.h>
#include <sys/ioctl.h>
 
extern int errno;
 
 
/*
 *      d l i _ e x a m p l e : d l i _ e t h d
 *
 * Description: This daemon program transmits any message it receives 
 *              to the originating node, i.e., it echoes the message 
 *              back.  The device and protocol type are supplied by 
 *              the user.  The program uses ethernet format packets.
 *
 * Inputs:      device, protocol type.
 *
 * Outputs:     Exit status.
 *
 * To compile:  cc -o dli_ethd dli_ethd.c
 *
 * Example:     dli_ethd de0 6006
 *
 * Comments:    This example demonstrates the use of the "DEFAULT" I/O
 *              control flag, and the recvfrom & sendto system calls.
 *              By specifying "DEFAULT" when binding the DLI socket to
 *              the device we inform the system that this program will
 *              receive any ethernet format packet with the given 
 *              protocol type which is not meant for any other program
 *              on the system.  Since packets may arrive from 
 *              different nodes we use the recvfrom call to read the 
 *              packets.  This call gives us access to the packet 
 *              header information so that we can determine where the 
 *              packet came from.  When we write on the socket we must
 *              use the sendto system call to explicitly give the 
 *              destination of the packet.
 */

/*
 * Digital Equipment Corporation supplies this software example on 
 * an "as-is" basis for general customer use.  Note that Digital 
 * does not offer any support for it, nor is it covered under any 
 * of Digital's support contracts. 
 */

main(argc, argv, envp)
int argc;
char **argv, **envp;
 
{
 
    u_char inbuf[1500], outbuf[1500];
    u_char devname[16];
    u_char target_eaddr[6];
    char *cp;
    int rsize, devunit;
    int i, sock, fromlen;
    u_short ptype, obsiz;
    struct sockaddr_dl from;
 
    if ( argc < 3 )
    {
        fprintf(stderr, 
                "usage: %s device hex-protocol-type\n", argv[0]);
        exit(1);
    }
 
    /* get device name and unit number. */
    bzero(devname, sizeof(devname));
    i = 0;
    cp = argv[1];
    while ( isalpha(*cp) )
        devname[i++] = *cp++;
    sscanf(cp, "%d", &devunit);
 
    /* get protocol type */
    sscanf(argv[2], "%x", &ptype);
 
    /* open dli socket */
    if 
    ((sock = dli_econn(devname, devunit, ptype, NULL, DLI_DEFAULT))<0)
    {
        perror("dli_ethd, dli_econn failed");
        exit(1);
    }
 
    while ( 1 ) {
        /* wait for message */
        from.dli_family = AF_DLI;
        fromlen = sizeof(struct sockaddr_dl);
        if ((rsize = recvfrom(sock, inbuf, sizeof(inbuf), 
                              NULL, &from, &fromlen)) < 0 ) {
            sprintf(inbuf, "%s: DLI reception failed", argv[0]);
            perror(inbuf);
            exit(2);
        }
 
        /* check header */
        if ( fromlen != sizeof(struct sockaddr_dl) ) {
            fprintf(stderr,"%s, incorrect header supplied\n",argv[0]);
            continue;
        }
 
        /* any data? */
        if ( ! rsize ) 
            fprintf(stderr, "%s, NO data received from ", argv[0]);
        else
            fprintf(stderr, "%s, data received from ", argv[0]);
        for ( i = 0; i < 6; i++ ) 
            fprintf(stderr, "%x%s", 
                    from.choose_addr.dli_eaddr.dli_target[i],
                    ((i<5)?"-":" "));
        fprintf(stderr, "on protocol type %x\n", 
                from.choose_addr.dli_eaddr.dli_protype);
    
        /* send response to originator. */
        if ( sendto(sock, inbuf, rsize, NULL, &from, fromlen) < 0 ) {
            sprintf(outbuf, "%s: DLI transmission failed", argv[0]);
            perror(outbuf);
            exit(2);
        }
    }
}
 
/*
 *              d l i _ e c o n n
 *
 *
 *
 * Description:
 *      This subroutine opens a dli socket, then binds an associated
 *      device name and protocol type to the socket.
 *
 * Inputs:
 *      devname         = ptr to device name
 *      devunit         = device unit number
 *      ptype           = protocol type
 *      taddr           = target address
 *      ioctl           = io control flag
 *
 *
 * Outputs:
 *      returns         = socket handle if success, otherwise -1
 *
 *
 */

dli_econn(devname, devunit, ptype, taddr, ioctl)
char *devname;
u_short devunit;
u_short ptype;
u_char *taddr;
u_char ioctl;
{
    int i, sock;
    struct sockaddr_dl out_bind;
 
    if ( (i = strlen(devname)) > 
          sizeof(out_bind.dli_device.dli_devname) )
    {
         fprintf(stderr, "dli_ethd: bad device name");
         return(-1);
    }
 
    if ((sock = socket(AF_DLI, SOCK_DGRAM, DLPROTO_DLI)) < 0)
    {
         perror("dli_ethd, can't open DLI socket");
         return(-1);
    }
    
    /*
     * fill out bind structure
     */
    bzero(&out_bind, sizeof(out_bind));
    out_bind.dli_family = AF_DLI;
    out_bind.dli_substructype = DLI_ETHERNET;
    bcopy(devname, out_bind.dli_device.dli_devname, i);
    out_bind.dli_device.dli_devnumber = devunit;
    out_bind.choose_addr.dli_eaddr.dli_ioctlflg = ioctl;
    out_bind.choose_addr.dli_eaddr.dli_protype = ptype;
    if ( taddr )
         bcopy(taddr, out_bind.choose_addr.dli_eaddr.dli_target, 
               DLI_EADDRSIZE);

    if ( bind(sock, &out_bind, sizeof(out_bind)) < 0 )
    {
         perror("dli_ethd, can't bind DLI socket");
         return(-1);
    }
    
    return(sock);
}
 
