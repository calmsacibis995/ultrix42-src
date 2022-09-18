#ifndef lint
static  char  *sccsid = "@(#)dli_setsockopt.c	4.1	(ULTRIX)	7/17/90";
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
int debug = 0;

#define PROTOCOL_ID      {0x00, 0x00, 0x00, 0x00, 0x5}
#define CUSTOMER0        {0xab, 0x00, 0x04, 0x00, 0x00, 0x00}
#define CUSTOMER1        {0xab, 0x00, 0x04, 0x00, 0x00, 0x01}

u_char mcast0[] = CUSTOMER0;
u_char mcast1[] = CUSTOMER1;
u_char protocolid[] = PROTOCOL_ID;

/*
 *      d l i _ e x a m p l e : d l i _ s e t s o c k o p t
 *
 * Description: This program demonstrates the use of the DLI get
 *              and setsockopt calls.  It opens a socket, enables
 *              2 multicast addresses, changes the 802 control
 *              field, enables a number of group saps supplied by 
 *              the user, and reads the group saps that are enabled.
 *
 * Inputs:      device, sap, group-saps.
 *
 * Outputs:     Exit status.
 *
 * To compile:  cc -o dli_setsockopt dli_setsockopt.c
 *
 * Example:     dli_setsockopt qe0 ac  5 9 d
 *
 * Comments:    When a packet arrives with a group dsap, all dli
 *              programs which have that group sap enabled will
 *              receive copies of that packet.  Group saps are
 *              those with the low order bit set.  Group sap 1 
 *              is currently not allowed for customer use. Group 
 *              saps with the second bit set (eg 3,7,etc) are 
 *              reserved by IEEE.
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
    int i, j, k, sock, fromlen;
    u_short obsiz;
    u_char tmpsap, sap;
    struct sockaddr_dl from;
    u_char *pi = 0;
    u_char out_opt[1000], in_opt[1000];
    int optlen, ioptlen = sizeof(in_opt);

    if ( argc < 4 )
    {
        fprintf(stderr, "usage: %s device hex-sap hex-groupsaps\n",
        argv[0]);
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
    sscanf(argv[2], "%x", &sap);

    /* open dli socket */
    if ( sap == SNAP_SAP ) {
        fprintf(stderr,
                "%s: can't use SNAP_SAP in USER mode\n", argv[0]);
        exit(1);
    }
    if ( (sock = dli_802_3_conn(devname, devunit, pi, target_eaddr,
                     DLI_DEFAULT, USER, sap, sap, UI_NPCMD)) < 0 ) {
        perror("dli_setsockopt: dli_conn failed");
        exit(1);
    }

    /* enable two multicast addresses */
    bcopy(mcast0, out_opt, sizeof(mcast0));
    bcopy(mcast1, out_opt+sizeof(mcast0), sizeof(mcast1));

    if ( setsockopt(sock, DLPROTO_DLI, DLI_MULTICAST, &out_opt[0],
                    (sizeof(mcast0) + sizeof(mcast1))) < 0 ) {
        perror("dli_setsockopt: can't enable multicast");
    }

    /* set 802 control field */
    out_opt[0] = TEST_PCMD;
    optlen = 1;
    if
    (setsockopt(sock,DLPROTO_DLI,DLI_SET802CTL,&out_opt[0],optlen)<0){
        perror("dli_setsockopt: Can't set 802 control");
        exit(1);
    }

    /* enable GSAPs supplied by user */
    j = 3;
    i = 0;
    while (j < argc ) {
        sscanf(argv[j++], "%x", &k);
        out_opt[i++] = k;
    }
    optlen = i;
    if
    (setsockopt(sock,DLPROTO_DLI,DLI_ENAGSAP,&out_opt[0],optlen) < 0){
        perror("dli_setsockopt: Can't enable gsap");
        exit(1);
    }

    /* verify all gsaps are enabled */
    bzero(in_opt, (ioptlen = sizeof(in_opt)));
    if
    (getsockopt(sock,DLPROTO_DLI,DLI_GETGSAP,in_opt,&ioptlen) < 0){
        perror("dli_setsockopt: DLI getsockopt 2 failed");
        exit(1);
    }
    printf("number of enabled GSAPs = %d, GSAPS:", ioptlen);
    for(i = 0; i < ioptlen; i++) {
        if ( ! (i % 10) )
            printf("\n");
        printf("%2x ",in_opt[i]);
    }
    printf("\n");

    /* disable all but the last 4 or all GSAPs, */
    /* whichever is smallest */
    if ( optlen > 4 )
        optlen -= 4;
    if
    (setsockopt(sock,DLPROTO_DLI,DLI_DISGSAP,&out_opt[0],optlen) < 0){
        perror("dli_setsockopt: Can't disable gsap");
    }

    /* verify some gsaps still enabled */
    bzero(in_opt, (ioptlen = sizeof(in_opt)));
    if
    (getsockopt(sock,DLPROTO_DLI,DLI_GETGSAP,in_opt,&ioptlen) < 0){
        perror("dli_setsockopt: getsockopt 3 failed");
        exit(1);
    }
    printf("number of enabled GSAPs = %d, GSAPS:", ioptlen);
    for(i = 0; i < ioptlen; i++) {
        if ( ! (i % 10) )
    printf("\n");
        printf("%2x ",in_opt[i]);
    }
    printf("\n");

}

/*
 *              d l i _8 0 2 _ 3 _ c o n n
 *
 *
 *
 * Description:
 *      This subroutine opens a dli 802.3 socket, then binds an 
 *      associated device name and protocol type to it.
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
        fprintf(stderr, "dli_setsockopt: bad device name");
        return(-1);
    }
    
    if ((sock = socket(AF_DLI, SOCK_DGRAM, DLPROTO_DLI)) < 0)
    {
        perror("dli_setsockopt: can't open DLI socket");
        return(-1);
    }
    
    /*
     * fill out bind structure
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
        perror("dli_setsockopt: can't bind DLI socket");
        return(-1);
    }
    
    return(sock);
}
