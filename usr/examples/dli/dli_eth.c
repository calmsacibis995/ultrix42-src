#ifndef lint
static  char  *sccsid = "@(#)dli_eth.c	4.1	(ULTRIX)	7/17/90";
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
 *      d l i _ e x a m p l e : d l i _ e t h 
 *
 * Description: This program sends out a message to a node where a
 *              companion program, dli_ethd, echoes the message back.
 *              The ethernet packet format is used.  The ethernet
 *              address of the node where the companion program is
 *              running, the protocol type, and the message are
 *              supplied by the user.  The companion program should
 *              be started before executing this program.
 *
 * Inputs:      device, target address, protocol type, short message.
 *
 * Outputs:     Exit status.
 *
 * To compile:  cc -o dli_eth dli_eth.c
 *
 * Example:     dli_eth qe0 08-00-2b-02-e2-ff 6006 "Echo this"
 *
 * Comments:    This example demonstrates the use of the "NORMAL" I/O
 *              control flag.  The use of the "NORMAL" flag means that
 *              we can communicate only with a single specific node
 *              whose address is specified during the bind.  Because
 *              of this, we can use the normal write & read system 
 *              calls on the socket, because the source/destination of
 *              all data that is read/written on the socket is fixed.
 *
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
    u_char target_eaddr[6];
    u_char devname[16];
    int rsize, devunit;
    char *cp; 
    int i, sock;
    u_short ptype, obsiz;
 
    if ( argc < 5 )
    {
        fprintf(stderr, "%s %s %s\n",
           "usage:",
           argv[0],
           "device ethernet-address hex-protocol-type short-message");
        exit(1);
    }
 
    /* get device name and unit number. */
    bzero(devname, sizeof(devname));
    i = 0;
    cp = argv[1];
    while ( isalpha(*cp) )
        devname[i++] = *cp++;
    sscanf(cp, "%d", &devunit);
 
    /* get phys addr of remote node */
    bzero(target_eaddr, sizeof(target_eaddr));
    i = 0;
    cp = argv[2];
    while ( *cp ) {
        if ( *cp == '-' ) {
            cp++;
            continue;
        }
        else {
            sscanf(cp, "%2x", &target_eaddr[i++]);
            cp += 2;
        }
    }
 
    /* get protocol type */
    sscanf(argv[3], "%hx", &ptype);
 
    /* get message */
    bzero(outbuf, sizeof(outbuf));
    if ( (obsiz = strlen(argv[4])) > 1500 ) {
        fprintf(stderr, "%s: message is too long\n", argv[0]);
        exit(1);
    }
    strcpy(outbuf, argv[4]);
 
 
    /* open dli socket */
    if ( (sock = dli_econn(devname, devunit, ptype, 
                           target_eaddr, DLI_NORMAL)) < 0 ) {
        perror("dli_eth, dli_econn failed");
        exit(1);
    }
 
 
    /* send message to target.  minimum message size is 46 bytes. */
    if ( write(sock, outbuf, (obsiz < 46 ? 46 : obsiz)) < 0 ) {
        sprintf(outbuf, "%s: DLI transmission failed", argv[0]);
        perror(outbuf);
        exit(2);
    }
 
    /* wait for response */
    if ((rsize = read(sock, inbuf, sizeof(inbuf))) < 0 ) {
        sprintf(inbuf, "%s: DLI reception failed", argv[0]);
        perror(inbuf);
        exit(2);
    }
    if ( ! rsize ) {
        fprintf(stderr, "%s, no data returned\n", argv[0]);
        exit(2);
    }
 
    /* print results */
    printf("%s\n", inbuf);
 
    close(sock);
 
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
         fprintf(stderr, "dli_eth: bad device name");
         return(-1);
    }
 
    if ((sock = socket(AF_DLI, SOCK_DGRAM, DLPROTO_DLI)) < 0)
    {
         perror("dli_eth, can't open DLI socket");
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
         perror("dli_eth, can't bind DLI socket");
         return(-1);
    }
    
    return(sock);
}
 
