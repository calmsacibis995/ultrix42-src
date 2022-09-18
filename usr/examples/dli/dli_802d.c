#ifndef lint
static  char  *sccsid = "@(#)dli_802d.c	4.1	(ULTRIX)	7/17/90";
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
 *      d l i _ e x a m p l e : d l i _ 8 0 2 d
 *
 * Description: This daemon program transmits any message it receives
 *              to the originating node, i.e., it echoes the message 
 *              back.  The device and sap are supplied by the user.  
 *              The program uses 802.3 format packets.
 *
 * Inputs:      device, sap.
 *
 * Outputs:     Exit status.
 *
 * To compile:  cc -o dli_802d dli_802d.c
 *
 * Example:     dli_802d de0 ac
 *
 * Comments:    This example demonstrates the recvfrom & sendto system
 *              calls.  Since packets may arrive from different nodes
 *              we use the recvfrom call to read the packets.  This
 *              call gives us access to the packet header information
 *              so that we can determine where the packet came from.  
 *              When we write on the socket we must use the sendto 
 *              system call to explicitly give the destination of 
 *              the packet.  The use of the "DEFAULT" I/O control flag
 *              only applies
 *              (i.e. only has an affect) when the SNAP SAP is used.
 *              When the SNAP SAP is used, any arriving packets which
 *              have the specified protocol id and which are not
 *              destined for some other program will be given to this
 *              program.
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
    u_short obsiz;
    u_char tmpsap, sap;
    struct sockaddr_dl from;
    u_char *pi = 0;
 
    if ( argc < 3 )
    {
        fprintf(stderr, "usage: %s device hex-sap\n", argv[0]);
        exit(1);
    }
 
 
    /* get device name and unit number. */
    bzero(devname, sizeof(devname));
    i = 0;
    cp = argv[1];
    while ( isalpha(*cp) )
        devname[i++] = *cp++;
    sscanf(cp, "%d", &devunit);
 
    /* get sap */
    sscanf(argv[2], "%x", &sap);
 
    /* open dli socket. note that if (and only if) the snap sap */
    /* was selected then a protocol id must also be specified. */
    if ( sap == SNAP_SAP )
        pi = protocolid;
    if ((sock = dli_802_3_conn(devname, devunit, pi, target_eaddr, 
                    DLI_DEFAULT, TYPE1, sap, sap, UI_NPCMD)) < 0) { 
        perror("dli_802d, dli_conn failed");
        exit(1);
    }
 
    /* listen and respond */
    while ( 1 ) {
        /* wait for message */
        from.dli_family = AF_DLI;
        fromlen = sizeof(struct sockaddr_dl);
        if ((rsize = recvfrom(sock, inbuf, sizeof(inbuf), NULL, 
                              &from, &fromlen)) < 0 ) {
            sprintf(inbuf, "%s: DLI reception failed", argv[0]);
            perror(inbuf);
            exit(2);
        }
 
        /* check header */
        if ( fromlen != sizeof(struct sockaddr_dl) ) {
            fprintf(stderr,"%s, incorrect header supplied\n",argv[0]);
            continue;
        }
 
        /*
         * Note that DLI swaps the source & destination saps and lan 
         * addresses in the sockaddr_dl structure returned by the 
         * recvfrom call.  That is, it places the DSAP in eh_802.ssap 
         * and the SSAP in eh_802.dsap; it also places the destination
         * lan address in eh_802.src and the source lan address in 
         * eh_802.dst.  This allows for minimal to no manipulation of
         * the address structure for subsequent sendto or dli
         * connection calls.
         */

        /* any data? */
        if ( ! rsize ) 
            fprintf(stderr, "%s: NO data received from ", argv[0]);
        else
            fprintf(stderr, "%s: data received from ", argv[0]);
        for ( i = 0; i < 6; i++ ) 
            fprintf(stderr, "%x%s", 
                    from.choose_addr.dli_802addr.eh_802.dst[i],
                    ((i<5)?"-":" "));
        fprintf(stderr, "\n     on dsap %x ", 
                from.choose_addr.dli_802addr.eh_802.ssap);
        if ( from.choose_addr.dli_802addr.eh_802.dsap == SNAP_SAP )
            fprintf(stderr, 
                    "(SNAP SAP), protocol id = %x-%x-%x-%x-%x\n    ",
                    from.choose_addr.dli_802addr.eh_802.osi_pi[0],
                    from.choose_addr.dli_802addr.eh_802.osi_pi[1],
                    from.choose_addr.dli_802addr.eh_802.osi_pi[2],
                    from.choose_addr.dli_802addr.eh_802.osi_pi[3],
                    from.choose_addr.dli_802addr.eh_802.osi_pi[4]);
        fprintf(stderr, " from ssap %x ", 
                from.choose_addr.dli_802addr.eh_802.dsap);
        fprintf(stderr, "\n\n");
    
        /* send response to originator. */
        if ( from.choose_addr.dli_802addr.eh_802.dsap == SNAP_SAP )
            bcopy(protocolid, 
                  from.choose_addr.dli_802addr.eh_802.osi_pi, 5);
        if ( sendto(sock, inbuf, rsize, NULL, &from, fromlen) < 0 ) {
            sprintf(outbuf, "%s: DLI transmission failed", argv[0]);
            perror(outbuf);
            exit(2);
        }
    }
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
         fprintf(stderr, "dli_802d: bad device name");
         return(-1);
    }
    
    if ((sock = socket(AF_DLI, SOCK_DGRAM, DLPROTO_DLI)) < 0)
    {
         perror("dli_802d, can't open DLI socket");
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
         perror("dli_802d, can't bind DLI socket");
         return(-1);
    }
    
    return(sock);
}
 
