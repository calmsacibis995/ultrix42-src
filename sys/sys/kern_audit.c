#ifndef lint
static char *sccsid = "@(#)kern_audit.c	4.7	ULTRIX	2/28/91";
#endif lint

/************************************************************************
 *									*
 *                      Copyright (c) 1990 by                           *
 *              Digital Equipment Corporation, Maynard, MA              *
 *                      All rights reserved.                            *
 *                                                                      *
 *   This software is furnished under a license and may be used and     *
 *   copied  only  in accordance with the terms of such license and     *
 *   with the  inclusion  of  the  above  copyright  notice.   This     *
 *   software  or  any  other copies thereof may not be provided or     *
 *   otherwise made available to any other person.  No title to and     *
 *   ownership of the software is hereby transferred.                   *
 *                                                                      *
 *   The information in this software is subject to change  without     *
 *   notice  and should not be construed as a commitment by Digital     *
 *   Equipment Corporation.                                             *
 *                                                                      *
 *   Digital assumes no responsibility for the use  or  reliability     *
 *   of its software on equipment which is not supplied by Digital.     *
 *									*
 ************************************************************************/

/*
 *
 *   Modification history:
 *
 *   28 Feb 91 - prs
 *	Added support for a configurable number
 *	of open file descriptors.
 *
 *   07 Sep 90 - scott
 *      tie audit_data buffers to cpu_num
 *
 *   06 Aug 90 - scott
 *	update to reflect changes in exportfs syscall
 *	break up INSERT_AUD_VAL macro
 *	change aud_shm table
 *
 *   15 Jun 90 - sekhar
 *	added auditing for mmap and munmap(mips only).
 *
 *   07 Dec 89 - scott
 *      remove length restriction on integer arrays
 *
 *   14 Nov 89 - Ursula Sinkewicz 
 *	removed lk_ifnet.
 *
 *   16 Oct 89 - scott
 *	fix use of gnode dev major #
 *
 *   07 Jul 89 - scott
 *      created file
 *
*/

#include "../h/param.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/kernel.h"
#include "../h/kmalloc.h"
#include "../h/types.h"
#include "../h/ioctl.h"
#include "../h/socket.h"
#include "../h/time.h"
#include "../h/ipc.h"
#include "../h/msg.h"
#include "../h/sem.h"
#include "../h/shm.h"
#include "../h/file.h"
#include "../h/gnode.h"
#include "../h/sysinfo.h"
#include "../net/net/if.h"
#include "../net/netinet/in.h"
#include "../h/cpudata.h"
#include "../h/smp_lock.h"
#include "../h/mount.h"
#include "../fs/nfs/nfs_clnt.h"
#include "../fs/nfs/vfs.h"

char audbuf_mask[MAXCPU];    /* to ensure exclusive use  -- must be locked  */
char *audit_data[MAXCPU];    /* audit data buffer                           */
int a_d_len[MAXCPU];         /* # bytes data; incremented on each token     */
int a_d_ptr[MAXCPU];         /* # bytes data; incremented on each record    */
int initdone[MAXCPU];        /* per-cpu values set                          */


/* copy event type, uid, pid, ppid, pid level, and event-dependent params into audit record */
/* return ptr to offset into audit buffer, or # bytes flushed */
audit_rec_build ( code, u_ap_a, param_key, error, result, buf_indx, op )
unsigned code;                  /* syscall code             */
char **u_ap_a;                  /* ptr to parameters        */
char *param_key;                /* format of parameters     */
int error;                      /* syscall op error         */
int result;                     /* syscall res; ptr update  */
int *buf_indx;                  /* for use by audgen only   */
char op;                        /* operations:
                                    AUD_GNO : store gnode info in u area
                                    AUD_HDR : build header
                                    AUD_PRM : add parameters
                                    AUD_RES : add result, timestamp
                                    AUD_FLU : explicit flush        */

{
    static int sleeping = 0;            /* must be locked           */
    static long hostaddr = 0;

    int size = (audsize*1024) < (AUD_BUF_SIZ<<2) ? (AUD_BUF_SIZ<<2) : (audsize*1024);

    int cur_lvl = 1;                    /* current object level for event; 1 if not B1 */
    int gno_indx_l;                     /* current u_gno* index     */
    register struct ifnet *ifp;
    register struct ifaddr *ifa;
    register struct gnode *gp = (struct gnode *)0;
    register struct file *fp = (struct file *)0;
    struct sockaddr_in *sinp;
    struct timeval tp;
    struct timezone tzp;
    dev_t gno_dev;
    gno_t gno_id;
    int siz_shrt = sizeof(short);
    int retval = 0;
    int i, j, k, l;
    int s;
    int ab;         /* audit_data buffer to be used for this event */
    short cpu_no = CURRENT_CPUDATA->cpu_num;

/* macros to insert data into audit record buffer */
#define SIZ_TOKEN 1
#define INSERT_AUD0(I_siz,I_to,I_where,I_what1,I_what2,I_len2) \
    {   /* use sizeof of (I_what2) parameter */ \
        if ( (I_where) + sizeof *(I_what2) + SIZ_TOKEN >= (I_siz) )\
            cprintf ( "audit buffer overflow B\n" );\
        else {\
            (I_to)[(I_where)] = I_what1;\
            (I_where) += SIZ_TOKEN;\
            bcopy ( (I_what2), &(I_to)[(I_where)], sizeof *(I_what2) );\
            (I_where) += sizeof *(I_what2);\
        }\
    }

#define INSERT_AUD1(I_siz,I_to,I_where,I_what1,I_what2,I_len2) \
    {   /* null-term string */ \
        for ( (I_len2) = 0; *((char *)(I_what2)+(I_len2)); (I_len2)++ );\
        if ( (I_where) + sizeof (I_len2) + (I_len2) + SIZ_TOKEN >= (I_siz) )\
            cprintf ( "audit buffer overflow C\n" );\
        else {\
            (I_to)[(I_where)] = I_what1;\
            (I_where) += SIZ_TOKEN;\
            bcopy ( &(I_len2), &(I_to)[(I_where)], sizeof (I_len2) );\
            (I_where) += sizeof (I_len2);\
            bcopy ( (I_what2), &(I_to)[(I_where)], (I_len2) );\
            (I_where) += (I_len2);\
        }\
    }

#define INSERT_AUD2(I_siz,I_to,I_where,I_what1,I_what2,I_len2) \
    {   /* null-term user string */ \
        if ( copyinstr ( (I_what2), &(I_to)[(I_where)+sizeof(I_len2)+SIZ_TOKEN],\
        ( (I_siz)-((I_where)+sizeof(I_len2)+SIZ_TOKEN) ), &(I_len2) ) == 0 ) {\
            if ( (I_len2) > 0 && (I_where) + sizeof(I_len2) + SIZ_TOKEN < (I_siz) ) {\
                (I_to)[(I_where)] = I_what1;\
                (I_where) += SIZ_TOKEN;\
                bcopy ( &(I_len2), &(I_to)[(I_where)], sizeof (I_len2) );\
                (I_where) += ((I_len2) + sizeof (I_len2));\
            }\
        }\
    }

#define INSERT_AUD3(I_siz,I_to,I_where,I_what1,I_what2,I_len2) \
    {   /* (I_len2) size string */ \
        if ( (I_where) + sizeof (I_len2) >= (I_siz) + (I_len2) + SIZ_TOKEN )\
            cprintf ( "audit buffer overflow G\n" );\
        else {\
            (I_to)[(I_where)] = I_what1;\
            (I_where) += SIZ_TOKEN;\
            bcopy ( &(I_len2), &(I_to)[(I_where)], sizeof (I_len2) );\
            (I_where) += sizeof (I_len2);\
            bcopy ( (I_what2), &(I_to)[(I_where)], (I_len2) );\
            (I_where) += (I_len2);\
        }\
    }

#define INSERT_AUD4(I_siz,I_to,I_where,I_what1,I_what2,I_len2) \
    {   /* (I_len2) size user string */ \
        if ( (I_where) + sizeof (I_len2) + SIZ_TOKEN >= (I_siz) )\
            cprintf ( "audit buffer overflow I\n" );\
        else {\
            (I_to)[(I_where)] = I_what1;\
            (I_where) += SIZ_TOKEN;\
            bcopy ( &(I_len2), &(I_to)[(I_where)], sizeof (I_len2) );\
            (I_where) += sizeof (I_len2);\
        }\
        if ( (I_where) + (I_len2) < (I_siz) && (I_len2) > 0 ) {\
            if ( copyin ( (I_what2), &(I_to)[(I_where)], (I_len2) ) )\
                (I_where) -= (sizeof (I_len2) + SIZ_TOKEN);\
            else (I_where) += (I_len2);\
        }\
        else (I_where) -= (sizeof (I_len2) + SIZ_TOKEN);\
    }

#define INSERT_AUD5(I_siz,I_to,I_where,I_what1,I_what2,I_len2) \
    {   /* (I_len2) size user data */ \
        if ( (I_where) + (I_len2) + SIZ_TOKEN < (I_siz) ) {\
            (I_to)[(I_where)] = I_what1;\
            if ( copyin ( (I_what2), &(I_to)[(I_where)+SIZ_TOKEN], (I_len2) ) == 0 )\
                (I_where) += (I_len2) + SIZ_TOKEN;\
        }\
    }


    /* use audit_data buffer according to cpu_no on which event occurred.
       use audbuf_mask to indicate which audit_data buffers are in use.
       use lk_audbuf to lock audbuf_mask and sleeping flag.
    */
    ab = cpu_no & 0xff;
    if ( op & AUD_HDR ) {
        for ( ;; ) {
            smp_lock ( &lk_audbuf, LK_RETRY );
            if ( audbuf_mask[ab] == 0x0 ) {
                audbuf_mask[ab] = 0x1;
                smp_unlock ( &lk_audbuf );
                break;
            }
            else {
                sleeping = 1;
                sleep_unlock ( (caddr_t)&sleeping, PZERO, &lk_audbuf );
            }
        }
        if ( op == AUD_HDR ) *buf_indx = ab;
    }

    /* audgen() knows which audit_data buffer to use */
    /* this will be true only from audgen()          */
    if ( (op == AUD_PTR) || (op == AUD_RES) ) ab = *buf_indx;

    /* alloc memory for audit records; get hostaddr */
    if ( initdone[ab] == 0 ) {
        initdone[ab]++;
        KM_ALLOC ( audit_data[ab], caddr_t, size, KM_TEMP, KM_NOARG );
        if ( audit_data[ab] == NULL ) panic ( "kern_audit: no mem" );
        if ( hostaddr == 0 ) {
            for ( ifp = ifnet; ifp; ifp = ifp->if_next ) {
                if ( ifp->if_flags&IFF_LOOPBACK || (ifp->if_flags&IFF_UP == 0) )
                    continue;
                for ( ifa = ifp->if_addrlist; ifa; ifa = ifa->ifa_next )
                    if ( ifa->ifa_addr.sa_family == AF_INET ) {
                        sinp = (struct sockaddr_in *)&ifa->ifa_addr;
                        hostaddr = sinp->sin_addr.s_addr;
                    }
            }
        }
    }

    /* SYSCALL # DEPENDENCE */
    /* ioctl (54): consider TIOCSTI only; fcntl (92): consider F_DUPFD only */
    if ( code == 54 && (int)u_ap_a[1] != (int)TIOCSTI ) {
        smp_lock ( &lk_audbuf, LK_RETRY );
        audbuf_mask[ab] = 0x0;
        smp_unlock ( &lk_audbuf );
        return(0);
    }
    if ( code == 92 && (int)u_ap_a[1] != (int)F_DUPFD ) {
        smp_lock ( &lk_audbuf, LK_RETRY );
        audbuf_mask[ab] = 0x0;
        smp_unlock ( &lk_audbuf );
        return(0);
    }

    /* load gnode information for descriptors into u_area */
    if ( op & AUD_GNO )
        for ( i = 0; (i < u.u_narg) && (u.u_gno_indx < 2); i++ )
            if ( param_key[i] == 'C' || param_key[i] == 'c' || param_key[i] == 'M' || param_key[i] == 'm' )
                if ( (unsigned)u_ap_a[i] <= u.u_omax ) {
                    fp = U_OFILE((unsigned)u_ap_a[i]);
                    if ( fp ) gp = (struct gnode *)fp->f_data;
                    if ( gp != (struct gnode *)0 ) {
                        if ( (gp->g_mode & GFMT) == GFCHR )
                            u.u_gno_dev[u.u_gno_indx] = gp->g_rdev;
                        else u.u_gno_dev[u.u_gno_indx] = gp->g_dev;
                        u.u_gno_num[u.u_gno_indx] = gp->g_number;
                        u.u_gno_indx++;
                    }
                }

    gno_indx_l = 0;
    s = spl6();   tp = time;   splx(s);   tzp = tz;

    /* build header - length, auid, hostaddr, event, uid, pid, ppid, dev, pidlvl */
    /* update AUD_BUF_HDR when changing this code segment */
    if ( op & AUD_HDR ) {
        for ( ; a_d_len[ab] >= size-AUD_BUF_SIZ; ) {
            smp_lock ( &lk_audbuf, LK_RETRY );
            sleeping = 1;
            sleep_unlock ( (caddr_t)&sleeping, PZERO, &lk_audbuf );
        }

        a_d_len[ab] += 5;   /* leave space for T_LENGTH and length */
        INSERT_AUD0 ( size, audit_data[ab], a_d_len[ab], T_AUID, &u.u_auid, 0 );
        INSERT_AUD0 ( size, audit_data[ab], a_d_len[ab], T_RUID, &u.u_ruid, 0 );
        INSERT_AUD0 ( size, audit_data[ab], a_d_len[ab], T_HOSTADDR, &hostaddr, 0 );
        INSERT_AUD0 ( size, audit_data[ab], a_d_len[ab], T_EVENT, &code, 0 );
        INSERT_AUD0 ( size, audit_data[ab], a_d_len[ab], T_UID, &u.u_uid, 0 );
        INSERT_AUD0 ( size, audit_data[ab], a_d_len[ab], T_PID, &u.u_procp->p_pid, 0 );
        INSERT_AUD0 ( size, audit_data[ab], a_d_len[ab], T_PPID, &u.u_procp->p_ppid, 0 );
        INSERT_AUD0 ( size, audit_data[ab], a_d_len[ab], T_DEV, &u.u_ttyd, 0 );
        INSERT_AUD0 ( size, audit_data[ab], a_d_len[ab], T_NCPU, &cpu_no, 0 );
        retval = (int)&audit_data[ab][a_d_len[ab]];
    }

    /* copy event-dependent parameters into recp */
    for ( i = 0; (op & AUD_PRM) && i < u.u_narg; i++ ) {

        switch ( param_key[i] ) {

        /* integer parameter, pid */
        case 'A':
        case 'a':   INSERT_AUD0 ( size, audit_data[ab], a_d_len[ab], T_INT, &u_ap_a[i], 0 );
                    break;

        /* ptr to char array */
        case 'B':   case 'S':
        case 'b':
        case 's':   INSERT_AUD2 ( size, audit_data[ab], a_d_len[ab], T_CHARP, u_ap_a[i], k );
                    if ( u.u_gno_indx && gno_indx_l < u.u_gno_indx ) {
                        INSERT_AUD0 ( size, audit_data[ab], a_d_len[ab], T_GNODE_DEV,
                        &u.u_gno_dev[gno_indx_l], 0 );
                        INSERT_AUD0 ( size, audit_data[ab], a_d_len[ab], T_GNODE_ID,
                        &u.u_gno_num[gno_indx_l], 0 );
                        gno_indx_l++;
                    }
                    break;

        /* descriptors - int */
        case 'C':   case 'M':
        case 'c':
        case 'm':   INSERT_AUD0 ( size, audit_data[ab], a_d_len[ab], T_DESCRIP, &u_ap_a[i], 0 );
                    if ( u.u_gno_indx && gno_indx_l < u.u_gno_indx ) {
                        INSERT_AUD0 ( size, audit_data[ab], a_d_len[ab], T_GNODE_DEV,
                        &u.u_gno_dev[gno_indx_l], 0 );
                        INSERT_AUD0 ( size, audit_data[ab], a_d_len[ab], T_GNODE_ID,
                        &u.u_gno_num[gno_indx_l], 0 );
                        gno_indx_l++;
                    }
                    break;

        /* integer to be masked with umask */
        case 'D':
        case 'd':   k = (int)u_ap_a[i] &~ u.u_cmask;
                    INSERT_AUD0 ( size, audit_data[ab], a_d_len[ab], T_INT, &k, 0 );
                    break;

        /* lengths to be used elsewhere */
        case 'E':   case 'L':
        case 'e':
        case 'l':   break;

        /* ptr to integer array */
        case 'F':
        case 'f':   k = (int)u_ap_a[i-1] * sizeof k;
                    INSERT_AUD4 ( size, audit_data[ab], a_d_len[ab], T_INTP, u_ap_a[i], k );
                    break;

        /* ptr to string of size specified in next param */
        case 'G':
        case 'g':   k = (int)u_ap_a[i+1];
                    INSERT_AUD4 ( size, audit_data[ab], a_d_len[ab], T_CHARP, u_ap_a[i], k );
                    break;

        /* pointer to socket */
        case 'H':
        case 'h':   k = (int)u_ap_a[i+1];
                    INSERT_AUD4 ( size, audit_data[ab], a_d_len[ab], T_SOCK, u_ap_a[i], k );
                    break;

        /* pointer to msghdr */
        case 'I':
        case 'i':   k = ((struct msghdr *)u_ap_a[i])->msg_namelen;
                    l = ((struct msghdr *)u_ap_a[i])->msg_accrightslen;
                    INSERT_AUD4 ( size, audit_data[ab], a_d_len[ab], T_MSGHDR,
                    ((struct msghdr *)u_ap_a[i])->msg_name, k );
                    INSERT_AUD4 ( size, audit_data[ab], a_d_len[ab], T_ACCRGHT,
                    ((struct msghdr *)u_ap_a[i])->msg_accrights, l );
                    break;

        /* shmop: should not be reached; code translated in trap.c; use aud_shm table */
        case 'J':
        case 'j':   break;
                    break;

        /* pointer to msqid_ds */
        case 'K':
        case 'k':   if ( (int)u_ap_a[i-1] != IPC_SET ) break;
                    INSERT_AUD5 ( size, audit_data[ab], a_d_len[ab], T_IPC_UID,
                    &((struct msqid_ds *)u_ap_a[i])->msg_perm.uid, siz_shrt );
                    INSERT_AUD5 ( size, audit_data[ab], a_d_len[ab], T_IPC_GID,
                    &((struct msqid_ds *)u_ap_a[i])->msg_perm.gid, siz_shrt );
                    INSERT_AUD5 ( size, audit_data[ab], a_d_len[ab], T_IPC_MODE,
                    &((struct msqid_ds *)u_ap_a[i])->msg_perm.mode, siz_shrt );
                    break;

        /* pointer to semid_ds */
        case 'N':
        case 'n':   if ( (int)u_ap_a[i-1] != IPC_SET ) break;
                    INSERT_AUD5 ( size, audit_data[ab], a_d_len[ab], T_IPC_UID,
                    &((struct semid_ds *)u_ap_a[i])->sem_perm.uid, siz_shrt );
                    INSERT_AUD5 ( size, audit_data[ab], a_d_len[ab], T_IPC_GID,
                    &((struct semid_ds *)u_ap_a[i])->sem_perm.gid, siz_shrt );
                    INSERT_AUD5 ( size, audit_data[ab], a_d_len[ab], T_IPC_MODE,
                    &((struct semid_ds *)u_ap_a[i])->sem_perm.mode, siz_shrt );
                    break;

        /* pointer to shmid_ds */
        case 'O':
        case 'o':   if ( (int)u_ap_a[i-1] != IPC_SET ) break;
                    INSERT_AUD5 ( size, audit_data[ab], a_d_len[ab], T_IPC_UID,
                    &((struct shmid_ds *)u_ap_a[i])->shm_perm.uid, siz_shrt );
                    INSERT_AUD5 ( size, audit_data[ab], a_d_len[ab], T_IPC_GID,
                    &((struct shmid_ds *)u_ap_a[i])->shm_perm.gid, siz_shrt );
                    INSERT_AUD5 ( size, audit_data[ab], a_d_len[ab], T_IPC_MODE,
                    &((struct shmid_ds *)u_ap_a[i])->shm_perm.mode, siz_shrt );
                    break;

        /* device */
        case 'P':
        case 'p':   INSERT_AUD0 ( size, audit_data[ab], a_d_len[ab], T_DEV, &u_ap_a[i], 0 );
                    break;

        /* get process label; pos'n independent parameter */
        case 'Q':   break;

        /* ptr to char array (kernel space) -- used for execv[e] */
        case 'R':
        case 'r':   if ( u_ap_a[i] == '\0' ) break;
                    INSERT_AUD1 ( size, audit_data[ab], a_d_len[ab], T_CHARP, u_ap_a[i], k );
                    if ( u.u_gno_indx && gno_indx_l < u.u_gno_indx ) {
                        INSERT_AUD0 ( size, audit_data[ab], a_d_len[ab], T_GNODE_DEV,
                        &u.u_gno_dev[gno_indx_l], 0 );
                        INSERT_AUD0 ( size, audit_data[ab], a_d_len[ab], T_GNODE_ID,
                        &u.u_gno_num[gno_indx_l], 0 );
                        gno_indx_l++;
                    }
                    break;

#ifdef PRIV
        /* ptr to privilege structure */
        case 'T':
        case 't':   k = sizeof(struct priv_set_t);
                    INSERT_AUD5 ( size, audit_data[ab], a_d_len[ab], T_PRIVSTR, u_ap_a[i], k );
                    break;
#endif PRIV

        /* ptr to int */
        case 'U':
        case 'u':   k = sizeof(int);
                    INSERT_AUD5 ( size, audit_data[ab], a_d_len[ab], T_INT, u_ap_a[i], k );
                    break;

        /* exportfs operation */
        case 'V':
        case 'v':   if ( (int)u_ap_a[0] == EXPORTFS_READ ) break;
                    INSERT_AUD2 ( size, audit_data[ab], a_d_len[ab], T_CHARP,
                    ((struct exportfsdata *)u_ap_a[i])->e_path, k );
                    INSERT_AUD5 ( size, audit_data[ab], a_d_len[ab], T_INT,
                    &((struct exportfsdata *)u_ap_a[i])->e_flags, 4 );
                    INSERT_AUD5 ( size, audit_data[ab], a_d_len[ab], T_SHORT,
                    &((struct exportfsdata *)u_ap_a[i])->e_rootmap, 2 );
                    break;

        /* audit from system call's code */
        case 'X':   break;

        case '0':   break;
        }
    }

    /* copy result, error, and time-stamp into audit record */
    /* update AUD_BUF_TRAILER when updating this code segment */
    if ( op & AUD_RES ) {
        i = j = 0;
#ifdef PRIV
        if ( u.u_pused.mask[0] | u.u_pused.mask[1] )
            INSERT_AUD0 ( size, audit_data[ab], a_d_len[ab], T_PRIVSTR, &u.u_pused, 0 );
#endif PRIV
        INSERT_AUD0 ( size, audit_data[ab], a_d_len[ab], T_ERROR, &error, 0 );
        INSERT_AUD0 ( size, audit_data[ab], a_d_len[ab], T_RESULT, &result, 0 );
        INSERT_AUD0 ( size, audit_data[ab], a_d_len[ab], T_TV_SEC, &tp.tv_sec, 0 );
        INSERT_AUD0 ( size, audit_data[ab], a_d_len[ab], T_TV_USEC, &tp.tv_usec, 0 );
        INSERT_AUD0 ( size, audit_data[ab], a_d_len[ab], T_TZ_MIN, &tzp.tz_minuteswest, 0 );
        INSERT_AUD0 ( size, audit_data[ab], a_d_len[ab], T_TZ_DST, &tzp.tz_dsttime, 0 );

        /* insert lengths into audit record */
        a_d_len[ab] += 5;           /* initial T_LENGTH token and value */
        j = a_d_len[ab] - a_d_ptr[ab];
        INSERT_AUD0 ( size, audit_data[ab], a_d_ptr[ab], T_LENGTH, &j, 0 );
        a_d_ptr[ab] = a_d_len[ab]-5;    /* final T_LENGTH token and value */
        INSERT_AUD0 ( size, audit_data[ab], a_d_ptr[ab], T_LENGTH, &j, 0 );
        a_d_ptr[ab] = a_d_len[ab];
    }

    /* update buffer ptr for audgen() */
    if ( op & AUD_PTR ) a_d_len[ab] += result;

    /* implicit flush audit buffer to /dev/audit, unless in audgen() */
    /* only audgen() sets op to AUD_HDR or AUD_PTR                   */
    if ( (a_d_len[ab] >= size-AUD_BUF_SIZ) && (op != AUD_HDR) && (op != AUD_PTR) ) {
        kernaudwrite ( audit_data[ab], a_d_len[ab] );
        retval = a_d_len[ab];
        a_d_len[ab] = a_d_ptr[ab] = 0;
        if ( sleeping ) {
            smp_lock ( &lk_audbuf, LK_RETRY );
            sleeping = 0;
            wakeup ( (caddr_t)&sleeping );
            smp_unlock ( &lk_audbuf );
        }
    }

    /* explicit flush audit buffer to /dev/audit */
    for ( i = j = 0; (op & AUD_FLU) && i < MAXCPU; i++ ) {
        if ( initdone[i] ) {
            kernaudwrite ( audit_data[i], a_d_len[i] );
            j += a_d_len[i];
            a_d_len[i] = a_d_ptr[i] = 0;
            if ( sleeping ) {
                smp_lock ( &lk_audbuf, LK_RETRY );
                sleeping = 0;
                wakeup ( (caddr_t)&sleeping );
                smp_unlock ( &lk_audbuf );
            }
        }
        retval = j;
    }

    if ( op & AUD_RES ) {
        smp_lock ( &lk_audbuf, LK_RETRY );
        audbuf_mask[ab] = 0x0;
        smp_unlock ( &lk_audbuf );
    }
    return ( retval );
}


/* each event is mapped into bit [event*2] to indicate audit successful
   operation and into bit [event*2-1] to indicate audit failed operation */
char syscallauditmask[SYSCALL_MASK_LEN];
char trustedauditmask[TRUSTED_MASK_LEN];


/* cap designation indicates security level must be recorded */
/*
0:   nothing
a/A: int
b/B: *char
c/C: fd
d/D: int &~ umask
e/E: int length
f/F: int[], length in prev param
g/G: char[], length in next param
h/H: struct sockaddr *addr, length in next param
i/I: struct msghdr *msg
j/J: shmsys operations: require extra resolution
k/K: msgctl operations:
l/L: int *length
m/M: int -- descriptor (not file)
n/N: semctl operations (similar to msgctl operations)
o/O: shmctl operations (similar to msgctl operations)
p/P: dev_t
Q  : position independent param; need to get process label
r/R: used internally for execv[e] pathname
s/S: *char (NO FOLLOW LINKS)
t/T: struct priv_set_t
u/U: *int
v/V: exportfs operation
X  : perform audit from system call's code
     (exit, close, execv, dup2, reboot, execve, audcntl)
*/

/* parameters to audit per syscall */
char aud_param[][AUD_NPARAM] = {
	"00000000",	/*   0 = indir */
	"0000000X",	/*   1 = exit */
	"00000000",	/*   2 = fork */
	"00000000",	/*   3 = read */
	"00000000",	/*   4 = write */
	"Baa00000",	/*   5 = open */
	"c000000X",	/*   6 = close */
#ifdef KDEBUG
	"00000000",	/*   7 = enter kernel debuger */
#else
	"00000000",	/*   7 = old wait */
#endif
	"Bd000000",	/*   8 = creat */
	"bb000000",	/*   9 = link */
	"B0000000",	/*  10 = unlink */
	"Rr00000X",	/*  11 = execv */
	"b0000000",	/*  12 = chdir */
	"00000000",	/*  13 = old time */
	"Bd000000",	/*  14 = mknod */
	"Ba000000",	/*  15 = chmod */
	"Baa00000",	/*  16 = chown; now 3 args */
	"00000000",	/*  17 = old break */
	"b0000000",	/*  18 = old stat */
	"00000000",	/*  19 = lseek */
	"00000000",	/*  20 = getpid */
	"BBa00000",	/*  21 = mount */
	"a0000000",	/*  22 = umount */
	"a0000000",	/*  23 = old setuid */
	"00000000",	/*  24 = getuid */
	"00000000",	/*  25 = old stime */
	"aA000000",	/*  26 = ptrace */
	"00000000",	/*  27 = old alarm */
	"00000000",	/*  28 = old fstat */
	"00000000",	/*  29 = opause */
	"00000000",	/*  30 = old utime */
	"00000000",	/*  31 = was stty */
	"00000000",	/*  32 = was gtty */
	"b0000000",	/*  33 = access */
	"00000000",	/*  34 = old nice */
	"00000000",	/*  35 = old ftime */
	"00000000",	/*  36 = sync */
	"A0000000",	/*  37 = kill */
	"b0000000",	/*  38 = stat */
	"aa000000",	/*  39 = old setpgrp */
	"s0000000",	/*  40 = lstat */
	"c0000000",	/*  41 = dup */
	"00000000",	/*  42 = pipe */
	"00000000",	/*  43 = old times */
	"00000000",	/*  44 = profil */
	"00000000",	/*  45 = nosys */
	"a0000000",	/*  46 = old setgid */
	"00000000",	/*  47 = getgid */
	"00000000",	/*  48 = old sig */
	"00000000",	/*  49 = reserved for USG */
	"00000000",	/*  50 = reserved for USG */
	"b0000000",	/*  51 = turn acct off/on */
	"00000000",	/*  52 = old set phys addr */
	"00000000",	/*  53 = old lock in core */
	"c0000000",	/*  54 = ioctl */
	"0000000X",	/*  55 = reboot */
	"00000000",	/*  56 = old mpxchan */
	"bb000000",	/*  57 = symlink */
	"00000000",	/*  58 = readlink */
	"Rrr0000X",	/*  59 = execve */
	"00000000",	/*  60 = umask */
	"b0000000",	/*  61 = chroot */
	"00000000",	/*  62 = fstat */
	"00000000",	/*  63 = used internally */
	"00000000",	/*  64 = getpagesize */
	"00000000",	/*  65 = mremap */
	"00000000",	/*  66 = vfork */
	"00000000",	/*  67 = old vread */
	"00000000",	/*  68 = old vwrite */
	"00000000",	/*  69 = sbrk */
	"00000000",	/*  70 = sstk */
	"0aa0c000",	/*  71 = mmap */
	"00000000",	/*  72 = old vadvise */
	"aa000000",	/*  73 = munmap */
	"00000000",	/*  74 = mprotect */
	"00000000",	/*  75 = madvise */
	"00000000",	/*  76 = vhangup */
	"00000000",	/*  77 = old vlimit */
	"00000000",	/*  78 = mincore */
	"00000000",	/*  79 = getgroups */
	"ef000000",	/*  80 = setgroups */
	"00000000",	/*  81 = getpgrp */
	"aa000000",	/*  82 = setpgrp */
	"00000000",	/*  83 = setitimer */
#ifdef vax
	"00000000",	/*  84 = wait */
#endif vax
#ifdef mips
	"00000000",	/*  84 = wait */
#endif mips
	"00000000",	/*  85 = swapon */
	"00000000",	/*  86 = getitimer */
	"00000000",	/*  87 = gethostname */
	"ge000000",	/*  88 = sethostname */
	"00000000",	/*  89 = getdtablesize */
	"cc00000X",	/*  90 = dup2 */
	"00000000",	/*  91 = getdopt */
	"c0000000",	/*  92 = fcntl */
	"00000000",	/*  93 = select */
	"00000000",	/*  94 = setdopt */
	"00000000",	/*  95 = fsync */
	"00000000",	/*  96 = setpriority */
	"00000000",	/*  97 = socket */
	"mHe00000",	/*  98 = connect */
	"M0000000",	/*  99 = accept */
	"00000000",	/* 100 = getpriority */
	"00000000",	/* 101 = send */
	"00000000",	/* 102 = recv */
#ifdef vax
	"00000000",	/* 103 = old socketaddr */
#endif vax
#ifdef mips
	"00000000",	/* 103 = sigreturn */
#endif mips
	"mHe00000",	/* 104 = bind */
	"00000000",	/* 105 = setsockopt */
	"00000000",	/* 106 = listen */
	"00000000",	/* 107 = old vtimes */
#ifdef vax
	"00000000",	/* 108 = sigvec */
#endif vax
#ifdef mips
	"00000000",	/* 108 = sigvec */
#endif mips
	"00000000",	/* 109 = sigblock */
	"00000000",	/* 110 = sigsetmask */
	"00000000",	/* 111 = sigpause */
	"00000000",	/* 112 = sigstack */
	"mI000000",	/* 113 = recvmsg */
	"mI000000",	/* 114 = sendmsg */
#ifdef TRACE
	"00000000",	/* 115 = vtrace */
#else
	"00000000",	/* 115 = nosys */
#endif
	"00000000",	/* 116 = gettimeofday */
	"00000000",	/* 117 = getrusage */
	"00000000",	/* 118 = getsockopt */
#ifdef vax
	"00000000",	/* 119 = resuba */
#else
	"00000000",	/* 119 = nosys */
#endif
	"00000000",	/* 120 = readv */
	"00000000",	/* 121 = writev */
	"00000000",	/* 122 = settimeofday */
	"Caa00000",	/* 123 = fchown */
	"Ca000000",	/* 124 = fchmod */
	"00000000",	/* 125 = recvfrom */
	"aa000000",	/* 126 = setreuid */
	"aa000000",	/* 127 = setregid */
	"bb000000",	/* 128 = rename */
	"B0000000",	/* 129 = truncate */
	"C0000000",	/* 130 = ftruncate */
	"00000000",	/* 131 = flock */
	"00000000",	/* 132 = nosys */
	"0000He00",	/* 133 = sendto */
	"ma000000",	/* 134 = shutdown */
	"00000000",	/* 135 = socketpair */
	"Bd000000",	/* 136 = mkdir */
	"B0000000",	/* 137 = rmdir */
	"b0000000",	/* 138 = utimes */
#ifdef vax
	"00000000",	/* 139 = used internally */
#endif vax
#ifdef mips
	"00000000",	/* 139 = sigreturn (4.2 longjumps)*/
#endif mips
	"00000000",	/* 140 = adjtime */
	"00000000",	/* 141 = getpeername */
	"00000000",	/* 142 = gethostid */
	"a0000000",	/* 143 = sethostid */
	"00000000",	/* 144 = getrlimit */
	"00000000",	/* 145 = setrlimit */
	"a0000000",	/* 146 = killpg */
	"00000000",	/* 147 = nosys */
	"00000000",	/* 148 = quota */
	"00000000",	/* 149 = qquota */
	"00000000",	/* 150 = getsockname */
#ifdef vax
	"Aak00000",	/* 151 = msgctl */
	"0a000000",	/* 152 = msgget */
	"A0000000",	/* 153 = msgrcv */
	"A0000000",	/* 154 = msgsnd */
	"A0an0000",	/* 155 = semctl */
	"00000000",	/* 156 = semget */
	"00000000",	/* 157 = semop */
	"00000000",	/* 158 = uname */
	"j000000X",	/* 159 = shared memory */
	"00000000",	/* 160 = plock */
	"00000000",	/* 161 = lockf (future) */
	"00000000",	/* 162 = ustat */
	"00000000",	/* 163 = getmnt */
	"00000000",	/* 164 = getdirentries */
#ifdef NFS
	"00000000",	/* 165 = NFS block I/O daemon */
	"00000000",	/* 166 = NFS get file handle */
	"m0000000",	/* 167 = NFS server daemon */
	"auv00000",	/* 168 = exportfs */
#else
	"00000000",	/* 165 */
	"00000000",	/* 166 */
	"00000000",	/* 167 */
	"00000000",	/* 168 */
#endif
	"00000000",	/* 169 = getdomainname */
	"ge000000",	/* 170 = setdomainname */
	"00000000",	/* 171 = sigpending */
	"00000000",	/* 172 = setsid */
	"00000000",	/* 173 = waitpid */
#endif vax
#ifdef mips
	/*
	 * Syscalls 151-180 inclusive are reserved for vendor-specific
	 * system calls.  (This includes various calls added for compatibity
	 * with other Unix variants.)
	 */
	"00000000",	/* 151 = sysmips */
	"00000000",	/* 152 = cacheflush */
	"00000000",	/* 153 = cachectl */
#ifdef DEBUG
	"00000000",	/* 154 = debug */
#else
	"00000000",	/* 154 = nosys */
#endif DEBUG
	"00000000",	/* 155 = nosys */
	"00000000",	/* 156 = nosys */
#ifdef NFS
	"00000000",	/* 157 = old nfs_mount */
	"00000000",	/* 158 = nfs_svc */
#else
	"00000000",	/* 157 = nosys */
	"00000000",	/* 158 = nosys */
#endif NFS
	"00000000",	/* 159 = getdirentries */
	"00000000",	/* 160 = statfs */
	"00000000",	/* 161 = fstatfs */
	"00000000",	/* 162 = unmount */
#ifdef NFS
	"00000000",	/* 163 = async_daemon */
	"00000000",	/* 164 = get file handle */
#else
	"00000000",	/* 163 = nosys */
	"00000000",	/* 164 = nosys */
#endif NFS
	"00000000",	/* 165 = getdomainname */
	"ge000000",	/* 166 = setdomainname */
 	"00000000",	/* 167 = old pcfs_mount */
#ifdef QUOTA
 	"00000000",	/* 168 = quotactl */
#else
	"00000000",	/* 168 = not configured */
#endif QUOTA
#ifdef NFS
	"auv00000",	/* 169 = exportfs */
#else
 	"00000000",	/* 169 = not configured */
#endif NFS
	"00000000",	/* 170 = mount */
#ifdef mips
	"00000000",	/* 171 = mipshwconf */
#endif mips
#ifdef mips			
/*
 * Ultrix system calls that mips doesn't have or are incompatible 
 */
	"Aak00000",	/* 172 = msgctl */
	"0a000000",	/* 173 = msgget */
	"A0000000",	/* 174 = msgrcv */
	"A0000000",	/* 175 = msgsnd */
	"A0an0000",	/* 176 = semctl */
	"00000000",	/* 177 = semget */
	"00000000",	/* 178 = semop */
	"00000000",	/* 179 = uname */
	"j000000X",	/* 180 = shared memory */
	"00000000",	/* 181 = plock */
	"00000000",	/* 182 = lockf (future) */
	"00000000",	/* 183 = ustat */
	"00000000",	/* 184 = getmnt */
	"BBa00000",	/* 185 = mount */
	"a0000000",	/* 186 = umount */
	"00000000",	/* 187 = sigpending */
	"00000000",	/* 188 = setsid */
	"00000000",	/* 189 = waitpid */
#endif mips
#endif mips
#ifdef vax
	"00000000",	/* 174 */
	"00000000",	/* 175 */
	"00000000",	/* 176 */
	"00000000",	/* 177 */
	"00000000",	/* 178 */
	"00000000",	/* 179 */
	"00000000",	/* 180 */
	"00000000",	/* 181 */
	"00000000",	/* 182 */
	"00000000",	/* 183 */
	"00000000",	/* 184 */
	"00000000",	/* 185 */
	"00000000",	/* 186 */
	"00000000",	/* 187 */
	"00000000",	/* 188 */
	"00000000",	/* 189 */
#endif vax

	"00000000",	/* 190 */
	"00000000",	/* 191 */
	"00000000",	/* 192 */
	"00000000",	/* 193 */
	"00000000",	/* 194 */
	"00000000",	/* 195 */
	"00000000",	/* 196 */
	"00000000",	/* 197 */
	"00000000",	/* 198 */
	"00000000",	/* 199 */
	"00000000",	/* 200 */
	"00000000",	/* 201 */
	"00000000",	/* 202 */
	"00000000",	/* 203 */
	"00000000",	/* 204 */
	"00000000",	/* 205 */
	"00000000",	/* 206 */
	"00000000",	/* 207 */
	"00000000",	/* 208 */
	"00000000",	/* 209 */
	"00000000",	/* 210 */
	"00000000",	/* 211 */
	"00000000",	/* 212 */
	"00000000",	/* 213 */
	"00000000",	/* 214 */
	"00000000",	/* 215 */
	"00000000",	/* 216 */
	"00000000",	/* 217 */
	"00000000",	/* 218 */
	"00000000",	/* 219 */
	"00000000",	/* 220 */
	"00000000",	/* 221 */
	"00000000",	/* 222 */
	"00000000",	/* 223 */
	"00000000",	/* 224 */
	"00000000",	/* 225 */
	"00000000",	/* 226 */
	"00000000",	/* 227 */
	"00000000",	/* 228 */
	"00000000",	/* 229 */
	"00000000",	/* 230 */
	"00000000",	/* 231 */
	"00000000",	/* 232 */
	"00000000",	/* 233 */
	"00000000",	/* 234 */
	"00000000",	/* 235 */
	"00000000",	/* 236 */
	"00000000",	/* 237 */
	"00000000",	/* 238 */
	"00000000",	/* 239 */
	"00000000",	/* 240 */
	"00000000",	/* 241 */
	"00000000",	/* 242 */
	"00000000",	/* 243 */
	"00000000",	/* 244 */
	"00000000",	/* 245 */
	"00000000",	/* 246 */
	"00000000",	/* 247 */
	"00000000",	/* 248 */
	"00000000",	/* 249 */
	"00000000",	/* 250 */
	"00000000",	/* 251 */
	"ageaaa0X",	/* 252 = audcntl */
	"00000000",	/* 253 = audgen */
	"00000000",	/* 254 = startcpu */
	"00000000", 	/* 255 = stopcpu */
	"00000000",	/* 256 = getsysinfo */
	"a00a0000",	/* 257 = setsysinfo */
};

/* map of which parameters are security relevant for each shmop */
char aud_shm[][AUD_NPARAM] = {
	"0Aaa0000",     /* shmat    */
	"0Aao0000",     /* shmctl   */
	"0a000000",     /* shmdt    */
	"000a0000"      /* shmget   */
};

