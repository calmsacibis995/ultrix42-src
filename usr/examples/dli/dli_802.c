#ifndef lint
static  char  *sccsid = "@(#)dli_802.c	4.1	(ULTRIX)	7/17/90";
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
 
#define PROTOCOL_ID      {0x00, 0x00, 0x00, 0x00, 0x5}
u_char protocolid[] = PROTOCOL_ID;
 
 
/*
 *      d l i _ e x a m p l e : d l i _ 8 0 2
 *
 * Description: This program sends out a message to a node where a
 *              companion program, dli_802d, echoes the message back.
 *              The 802.3 packet format is used.  The ethernet
 *              address of the node where the companion program is
 *              running, the sap, and the message are supplied by the 
 *              user.  The companion program should be started before 
 *              executing this program.
 *
 * Inputs:      device, target address, sap, short message.
 *
 * Outputs:     Exit status.
 *
 * To compile:  cc -o dli_802 dli_802.c
 *
 * Example:     dli_802 qe0 08-00-2b-02-e2-ff ac "Echo this"
 *
 * Comments:    This example demonstrates the use of 802 "TYPE1" 
 *              service.  With TYPE1 service, the processing of
 *              XID and TEST messages is handled transparently by
 *              DLI, i.e., this program doesn't have to be concerned
 *              with handling them.  If the SNAP SAP (0xAA) is
 *              selected, a 5 byte protocol id is also required.
 *              This example automatically uses a protocol id of
 *              of PROTOCOL_ID when ths SNAP SAP is used.  Also,
 *              note the use of DLI_NORMAL for the i/o control flag.
 *              DLI makes use of this only when that SNAP_SAP/Protocol
 *              ID pair is used. DLI will filter all incoming messages
 *              by comparing the Ethernet source address and Protocol 
 *              ID against the target address and Protocol ID set up 
 *              in the bind call.  Only if a match occurs will DLI 
 *              pass the message up to the application.  
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
    int i, sock, fromlen;
    struct sockaddr_dl from;
    u_short obsiz;
    u_int sap;
    u_char  *pi = 0;
 
 
    if ( argc < 5 )
    {
        fprintf(stderr, "%s %s %s\n",
                "usage:",
                argv[0],
                "device ethernet-address hex-sap short-message");
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
 
    /* get sap */
    sscanf(argv[3], "%x", &sap);
 
    /* get message */
    bzero(outbuf, sizeof(outbuf));
    if ( (obsiz = strlen(argv[4])) > 1500 ) {
        fprintf(stderr, "%s: message is too long\n", argv[0]);
        exit(2);
    }
    strcpy(outbuf, argv[4]);
 
 
    /* open dli socket. notice that if (and only if) the snap sap */
    /* was selected then a protocol id must also be provided. */
    if ( sap == SNAP_SAP )
        pi = protocolid;
    if ( (sock = dli_802_3_conn(devname, devunit, pi, target_eaddr,
                     DLI_NORMAL, TYPE1, sap, sap, UI_NPCMD)) < 0 ) {
        perror("dli_802, dli_econn failed");
        exit(3);
    }
 
 
    /* send message to target. minimum message size is 46 bytes. */
    if ( write(sock, outbuf, (obsiz < 46 ? 46 : obsiz)) < 0 ) {
        sprintf(outbuf, "%s: DLI transmission failed", argv[0]);
        perror(outbuf);
        exit(4);
    }
 
    /* wait for response from correct address */
    while (1) {
        bzero(&from, sizeof(from));
        from.dli_family = AF_DLI;
        fromlen = sizeof(struct sockaddr_dl);
        if ((rsize = recvfrom(sock, inbuf, sizeof(inbuf),
                              NULL, &from, &fromlen)) < 0 ) {
                sprintf(inbuf, "%s: DLI reception failed", argv[0]);
                perror(inbuf);
                exit(5);
        }
        if ( fromlen != sizeof(struct sockaddr_dl) ) {
                fprintf(stderr,"%s, invalid address size\n",argv[0]);
                exit(6);
        }
        if ( bcmp(from.choose_addr.dli_802addr.eh_802.dst, 
                  target_eaddr, sizeof(target_eaddr)) == 0 )
                break;
    }

    if ( ! rsize ) {
        fprintf(stderr, "%s, no data returned\n", argv[0]);
        exit(7);
    }
    /* print message */
    printf("%s\n", inbuf);
 
    close(sock);
 
}
 
/*
 *              d l i _8 0 2 _ 3 _ c o n n
 *
 *
 *
 * Description:
 *      This subroutine opens a dli 802.3 socket, then binds an 
 *      associated device name and protocol type to the socket.
 *
 * Inputs:
 *      devname         = ptr to device name
 *      devunit         = device unit number
 *      ptype           = protocol type
 *      taddr           = target address
 *      ioctl           = io control flag
 *      svc             = service class
 *      sap             = source sap
 *      dsap            = destination sap
 *      ctl             = control field
 *
 *
 * Outputs:
 *      returns         = socket handle if success, otherwise -1
 *
 *
 */

dli_802_3_conn (devname,devunit,ptype,taddr,ioctl,svc,sap,dsap,ctl)
char *devname;
u_short devunit;
u_char *ptype;
u_char *taddr;
u_char ioctl;
u_char svc;
u_char sap;
u_char dsap;
u_short ctl;

{
    int i, sock;
    struct sockaddr_dl out_bind;
    
    if ( (i = strlen(devname)) > 
         sizeof(out_bind.dli_device.dli_devname) )
    {
         fprintf(stderr, "dli_802: bad device name");
         return(-1);
    }
    
    if ((sock = socket(AF_DLI, SOCK_DGRAM, DLPROTO_DLI)) < 0)
    {
         perror("dli_802, can't open DLI socket");
         return(-1);
    }
    
    /*
     * fill out bind structure.  note that we need to determine
     * whether the ctl field is 8 bits (unnumbered format) or
     * 16 bits (informational/supervisory format).  We do this
     * by checking the low order 2 bits, which are both 1 only
     * for unnumbered control fields.
     */
    bzero(&out_bind, sizeof(out_bind));
    out_bind.dli_family = AF_DLI;
    out_bind.dli_substructype = DLI_802;
    bcopy(devname, out_bind.dli_device.dli_devname, i);
    out_bind.dli_device.dli_devnumber = devunit;
    out_bind.choose_addr.dli_802addr.ioctl = ioctl;
    out_bind.choose_addr.dli_802addr.svc = svc;
    if(ctl & 3)
        out_bind.choose_addr.dli_802addr.eh_802.ctl.U_fmt=(u_char)ctl;
    else
        out_bind.choose_addr.dli_802addr.eh_802.ctl.I_S_fmt = ctl;
    out_bind.choose_addr.dli_802addr.eh_802.ssap = sap;
    out_bind.choose_addr.dli_802addr.eh_802.dsap = dsap;
    if ( ptype )
        bcopy(ptype,out_bind.choose_addr.dli_802addr.eh_802.osi_pi,5);
    if ( taddr )
         bcopy(taddr, out_bind.choose_addr.dli_802addr.eh_802.dst, 
               DLI_EADDRSIZE);
    if ( bind(sock, &out_bind, sizeof(out_bind)) < 0 )
    {
         perror("dli_802, can't bind DLI socket");
         return(-1);
    }
    
    return(sock);
}
 
