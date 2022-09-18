#ifndef lint
static char *sccsid = "@(#)auditd.c	4.2	ULTRIX	8/9/90";
#endif lint

/************************************************************************
 *									*
 *                      Copyright (c) 1989, 1990 by                     *
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
 *   07 Jul 89 - scott
 *      created file
 *
 *   09 Oct 89 - scott
 *      connecting daemon sends hostname to receiver; disallow sending to self
 *      add ping code to shorten connection timeout delay
 *
 *   09 Aug 90 - scott
 *      use kerberos rd_safe code
 *      add -w option (same as -?)
 *      re-create admin socket if necessary
 *      generate audit_start audit record, console msg
 *
*/

/* Audit Daemon
  -DKERBEROS to use KERBEROS authentication (-lkrb -ldes -lknet)
  -DDEBUG1 to prevent initial fork; make debugging easier
  -DDEBUG2 to allow debugging of "child" receiving from network
  -DDEBUG3 for routine trace
*/

#include <fcntl.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/mount.h>
#include <sys/time.h>
#include <sys/reboot.h>
#include <sys/resource.h>
#include <errno.h>
#include <stdio.h>
#include <sys/audit.h>
#include <sys/wait.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <syscall.h>
#include <setjmp.h>
#ifdef KERBEROS
#include <krb.h>
#include <des.h>
#endif KERBEROS
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>

#define AUDITD_CLNTS    "/etc/auditd_clients"
#define AUDITD_DIR      "/tmp/.audit"
#define BUF_SIZ         512
#define DEFAULT_ACTION  '\0'
#define DEFAULT_AUD_BUF 8
#define DEFAULT_LOG     "/usr/adm/auditlog"
#define DEFAULT_LOG_REM "/usr/adm/auditlog:remote"
#define DEFAULT_PCT     10
#define DEFAULT_TIME    30
#define DEFTIMEOUT      4
#define KRB_HDRSIZ      6
#define KRB_PKTSIZ      31
#define KRB_RECVSIZ     5
#define KRB_SENDSIZ     4
#define KRB_TIMEOUT     8
#define MAXFILESIZ      0x40000000
#define MAXPACKET       4096    /* max packet size */
#define MAX_LOG_INDX    1000
#define MIN_AUD_BUF     2
#define MSG_OOB_SIZE    1
#define N_CONN          6
#define NFLS            8
#define PRIORITY       -5
#define SUSPEND        -2
#define TKT_FILE_L      "/var/dss/kerberos/tkt/tkt.auditd"

struct sockaddr ss = {            /* server-client socket */
    AF_UNIX,                      /* address family       */
    "audS"                        /* pathname             */
};

struct audit_attr {
    char audit2log[MAXPATHLEN];   /* pathname of alternate log    */
    char console[MAXPATHLEN];     /* device name for "console"    */
#ifdef DEBUG3
    int  debug3;                  /* turn debug mode on/off       */
#endif DEBUG3
    int  dump;                    /* dump audit log buffer        */
    int  freepct;                 /* min free space w/o warning   */
    int  help;                    /* print help menu              */
    int  incr;                    /* 1 - increment log_indx       */
    int  kerb_auth;               /* 1 - use kerberos scheme      */
    int  kill;                    /* 1 - kill process             */
    int  net_server;              /* 1 - listen over inet         */
    int  nkbytes;                 /* size of audit data buffer    */
    char overflow;                /* action to take on full log   */
    char pathname[MAXPATHLEN];    /* audit data pathname          */
    char pathval;                 /* 'i'-inet_socket, 'l'-log     */
    int  pid;                     /* auditd identifier            */
    int  query;                   /* query server for log name    */
    int  show;                    /* show status                  */
    int  timeout;                 /* ping timeout value           */
} attr_g;

struct arp {
    char *aud_rec_b;              /* base of audit data           */
    char *aud_rec;                /* audit data ptr               */
};

struct hndshk {                   /* used to sync daemons         */
    char hostname[MAXHOSTNAMELEN];  /* used for self-check        */
    int  kerb_flag;                 /* 0: kerberos off; 1: on     */
};

int client_timeout();             /* client timeout handler       */
extern int errno;
int fda;                          /* auditlog file descriptor     */
int log_indx = -1;                /* audit logname suffix         */
jmp_buf env;                      /* used in kerb_ops             */
char *gethost_l();                /* convert ipaddr to hostname   */
char *inet_ntoa();                /* inet address to ascii        */
char *itoa();                     /* convert int to alphanumeric  */
int kerb_timeout();               /* kerberos timeout handler     */
int nobuf = 1;                    /* don't buffer input data      */
int pingpid;                      /* child ping process           */
int ping_noanswer();              /* ping timeout handler         */
int sigtermed = 0;                /* no compress() on sigterm     */
int sig_hndlr();                  /* SIGALRM, SIGTERM handler     */
int sig_hndlr2();                 /* SIGHUP handler               */

/*  client::inet_out --> INET --> server::sd3 (--> child::sd3)    */
int inet_out = -1;                /* inet descriptor for auditd   */
                                  /* to xfer data to net_server   */
int child[N_CONN+1];              /* child auditd pids            */

#ifdef KERBEROS
AUTH_DAT auth_data;               /* KERBEROS authentication data */
MSG_DAT msg_data;                 /* KERBEROS: for rd_safe_msg    */
CREDENTIALS cred;                 /* KERBEROS credentials         */
#endif KERBEROS


/* audit daemon */
main ( argc, argv )
int argc;
char *argv[];
{
    char path_l[MAXPATHLEN];    /* current directory */
    char buf_l[BUF_SIZ];
    int i, j, k;

    /* initialize attributes */
    init_attr ( &attr_g );

    /* process command line */
    for ( i = 1; i < argc; i++ ) {
        if ( argv[i][0] == '-' )
            for ( j = 1; (i < argc) && argv[i][j]; j++ ) switch ( argv[i][j] ) {

            case 'a':   /* use kerberos authentication */
            case 'A':   attr_g.kerb_auth = 1;
                        break;

            case 'b':   /* change alternate log pathname */
            case 'B':   if ( ++i == argc ) break;
                        for ( j = 0; (attr_g.audit2log[j] = argv[i][j]) && (j < MAXPATHLEN-4); j++ );
                        j--;
                        break;

            case 'c':   /* set idea of console */
            case 'C':   if ( ++i == argc ) break;
                        for ( j = 0; (attr_g.console[j] = argv[i][j]) && (j < MAXPATHLEN); j++ );
                        j--;
                        break;

            case 'd':   /* dump current buffer */
            case 'D':   attr_g.dump = 1;
                        if ( audcntl ( FLUSH_AUD_BUF, (char *)0, 0, 0, 0 ) == -1 )
                            perror ( "audcntl" );
                        break;

            case 'f':   /* change minimum % free space for warning */
            case 'F':   if ( ++i == argc ) break;
                        attr_g.freepct = atoi ( argv[i] );
                        for ( j = 0; argv[i][j+1]; j++ );
                        break;

            case 'h':   /* print help menu */
            case 'H':   attr_g.help = 1;
                        break;

                        /* change audit data pathname */
            case 'i':   case 'I':
            case 'l':   case 'L':
                        if ( argv[i][j] == 'i' && attr_g.kerb_auth )
                            attr_g.nkbytes = 4;
                        if ( i == argc-1 ) break;
                        attr_g.pathval = argv[i][j] < 'a' ? argv[i][j]+'a'-'A' : argv[i][j];
                        i++;
                        for ( j = 0; (attr_g.pathname[j] = argv[i][j]) && (j < MAXPATHLEN-4); j++ );
                        j--;
                        break;

            case 'k':   /* kill proc */
            case 'K':   attr_g.kill = 1;
                        if ( audcntl ( FLUSH_AUD_BUF, (char *)0, 0, 0, 0 ) == -1 )
                            perror ( "audcntl" );
                        break;

            case 'n':   /* size of audit data buffer */
            case 'N':   if ( ++i == argc ) break;
                        if ( attr_g.kerb_auth == 0 || attr_g.pathval == 'l' ) {
                            attr_g.nkbytes = atoi ( argv[i] );
                            nobuf = 0;
                        }
                        for ( j = 0; argv[i][j+1]; j++ );
                        break;

            case 'o':   /* set action to take on overflow */
            case 'O':   if ( ++i == argc ) break;
                        attr_g.overflow = argv[i][0];
                        for ( j = 0; argv[i][j+1]; j++ );
                        break;

            case 'p':   /* specify proc to handle request */
            case 'P':   if ( ++i == argc ) break;
                        attr_g.pid = atoi ( argv[i] );
                        for ( j = 0; argv[i][j+1]; j++ );
                        break;

            case 'q':   /* query server for pathname */
            case 'Q':   attr_g.query = 1;
                        break;

            case 's':   /* set network server status */
            case 'S':   attr_g.net_server = 1;
                        break;

            case 't':   /* set ping timeout value */
            case 'T':   if ( ++i == argc ) break;
                        attr_g.timeout = atoi ( argv[i] );
                        for ( j = 0; argv[i][j+1]; j++ );
                        break;

            case 'x':   /* increment log_indx */
            case 'X':   attr_g.incr = 1;
                        break;

            case 'z':   /* remove previous server-client socket */
            case 'Z':   getwd ( path_l );
                        chdir ( AUDITD_DIR );
                        unlink ( ss.sa_data );
                        bcopy ( "audX", buf_l, 5 );
                        for ( k = 1; k <= N_CONN; k++ ) {
                            buf_l[3] = k+'0';
                            unlink ( buf_l );
                        }
                        chdir ( path_l );
                        break;

            case '?':   /* show status */
            case 'w':
            case 'W':   attr_g.show = 1;
                        break;

#ifdef DEBUG3
            case '3':   attr_g.debug3 = 1;
                        break;
#endif DEBUG3

            default:    /* unknown option */
                        fprintf ( stderr, "auditd: Unknown option: %c ignored\n", argv[i][j] );
                        break;
            }
    }

    /* act as client */
    if ( client ( 0, &attr_g, &ss, 1 ) != 0 ) exit(0);

    /* start server */
    server();
    exit(0);
}


/* build an auditor audit message; use output_rec() to place in log */
audgen_l ( event, string1, string2 )
unsigned event;         /* non-kernel event value           */
char *string1;          /* first char * parameter           */
char *string2;          /* second char * parameter          */
{
    char a_d[BUF_SIZ];                /* audit data buffer            */
    int a_d_len;                      /* pos'n in audit data buffer   */
    long auditd_auid;                 /* daemon's audit_id            */
    unsigned long auditd_hostaddr;    /* daemon's IP address          */
    short auditd_euid;                /* daemon's euid                */
    short auditd_pid;                 /* daemon's pid                 */
    short auditd_ppid;                /* daemon's ppid                */
    short auditd_ruid;                /* daemon's ruid                */
    char hostname[MAXHOSTNAMELEN];    /* local hostname               */
    struct hostent *hostent;
    struct timeval tp;
    struct timezone tzp;
    char token;                       /* token placeholder            */
    int i, j;

/* insert data into audit record */
/* will not see console msgs with current set of audit messages */
#define INSERT_AUD_VAL(I_siz,I_to,I_where,I_what1,I_token,I_what2,I_len2,I_cntl) \
{\
    (I_token) = (I_what1);\
    if ( (I_where) + sizeof (I_token) >= (I_siz) ) {\
        cwrite ( "auditd: audgen overflow" );\
        return;\
    }\
    bcopy ( (char *)&(I_token), &(I_to)[(I_where)], sizeof (I_token) );\
    (I_where) += sizeof (I_token);\
    if ( (I_cntl) == 0 ) {\
        if ( (I_where) + sizeof *(I_what2) >= (I_siz) ) {\
            cwrite ( "auditd: audgen overflow" );\
            return;\
        }\
        bcopy ( (char *)(I_what2), &(I_to)[(I_where)], sizeof *(I_what2) );\
        (I_where) += sizeof *(I_what2);\
    }\
    else if ( (I_cntl) == -1 ) {\
        for ( (I_len2) = 0; *((I_what2)+(I_len2)); (I_len2)++ );\
        if ( (I_where) + sizeof (I_len2) >= (I_siz) ) {\
            cwrite ( "auditd: audgen overflow" );\
            return;\
        }\
        bcopy ( (char *)&(I_len2), &(I_to)[(I_where)], sizeof (I_len2) );\
        (I_where) += sizeof (I_len2);\
        if ( (I_where) + (I_len2) >= (I_siz) ) {\
            cwrite ( "auditd: audgen overflow" );\
            return;\
        }\
        bcopy ( (char *)(I_what2), &(I_to)[(I_where)], (I_len2) );\
        (I_where) += (I_len2);\
    }\
}

#ifdef DEBUG3
    if ( attr_g.debug3 ) cwrite ( "debug: audgen_l" );
#endif DEBUG3

    /* initialize daemon identifying values */
    gethostname ( hostname, MAXHOSTNAMELEN );
    if ( hostent = gethostbyname(hostname) )
        bcopy ((char *)hostent->h_addr, (char *)&auditd_hostaddr, hostent->h_length );
    auditd_euid = geteuid();
    auditd_ruid = getuid();
    auditd_pid = getpid();
    auditd_ppid = getppid();

    /* copy length, auid, hostaddr, event, uid, pid, ppid, pidlvl into audit record */
    a_d_len = (sizeof token + sizeof j); /* leave space for T_LENGTH and length */
    auditd_auid = audcntl ( GETPAID, (char *)0, 0, 0, 0 );
    INSERT_AUD_VAL ( BUF_SIZ, a_d, a_d_len, T_AUID, token, &auditd_auid, j, 0 );
    INSERT_AUD_VAL ( BUF_SIZ, a_d, a_d_len, T_RUID, token, &auditd_ruid, j, 0 );
    INSERT_AUD_VAL ( BUF_SIZ, a_d, a_d_len, T_UID, token, &auditd_euid, j, 0 );
    INSERT_AUD_VAL ( BUF_SIZ, a_d, a_d_len, T_HOSTADDR, token, &auditd_hostaddr, j, 0 );
    INSERT_AUD_VAL ( BUF_SIZ, a_d, a_d_len, T_EVENT, token, &event, i, 0 );
    INSERT_AUD_VAL ( BUF_SIZ, a_d, a_d_len, T_PID, token, &auditd_pid, j, 0 );
    INSERT_AUD_VAL ( BUF_SIZ, a_d, a_d_len, T_PPID, token, &auditd_ppid, j, 0 );

    /* insert parameters */
    if ( *string1 ) INSERT_AUD_VAL ( BUF_SIZ, a_d, a_d_len, T_CHARP, token, string1, i, -1 );
    if ( *string2 ) INSERT_AUD_VAL ( BUF_SIZ, a_d, a_d_len, T_CHARP, token, string2, i, -1 );

    /* insert timestamp */
    gettimeofday ( &tp, &tzp );
    INSERT_AUD_VAL ( BUF_SIZ, a_d, a_d_len, T_TV_SEC, token, &tp.tv_sec, i, 0 );
    INSERT_AUD_VAL ( BUF_SIZ, a_d, a_d_len, T_TV_USEC, token, &tp.tv_usec, i, 0 );
    INSERT_AUD_VAL ( BUF_SIZ, a_d, a_d_len, T_TZ_MIN, token, &tzp.tz_minuteswest, i, 0 );
    INSERT_AUD_VAL ( BUF_SIZ, a_d, a_d_len, T_TZ_DST, token, &tzp.tz_dsttime, i, 0 );

    /* insert total length */
    j = 0;
    a_d_len += (sizeof token + sizeof j);
    INSERT_AUD_VAL ( BUF_SIZ, a_d, j, T_LENGTH, token, &a_d_len, i, 0 );
    j = a_d_len - (sizeof token + sizeof j);
    INSERT_AUD_VAL ( BUF_SIZ, a_d, j, T_LENGTH, token, &a_d_len, i, 0 );

    /* output audit record */
    output_rec ( a_d_len, a_d, (off_t *)0 );
}


/* check hostname against list of allowed hosts; return 0 (pass) or -1 (fail) */
int chk_access ( sckt )
int sckt;                               /* connection to be checked     */
{
    struct sockaddr_in peername;        /* peername for sckt connection */
    int namelen = sizeof(peername);
    int found = 0;                      
    int len;
    int fd;                             /* fd for AUDITD_CLNTS          */
    int start, end;                     /* posn's in current buf_l      */
    char *hostp;                        /* remote hostname              */
    char buf_l[BUF_SIZ];                /* input from AUDITD_CLNTS      */
    char hostname[MAXHOSTNAMELEN];      /* local hostname               */
    struct hndshk hndshk;               /* daemons' handshake data      */
#ifdef KERBEROS
    char realm[REALM_SZ];               /* KERBEROS realm               */
#endif KERBEROS
    int i, j;

#ifdef DEBUG3
    if ( attr_g.debug3 ) cwrite ( "debug: chk_access" );
#endif DEBUG3

    /* socket is non-blocking; use alarm to protect against infinite loop */
    alarm(4);
    signal ( SIGALRM, client_timeout );
    if ( setjmp(env) ) {
        cwrite ( "auditd: client connection timeout" );
        signal ( SIGALRM, sig_hndlr );
        return(-1);
    }

    /* read hndshk data */
    for ( i = j = 0; j < MAXHOSTNAMELEN; ) {
        i = read ( sckt, &hndshk.hostname[j], MAXHOSTNAMELEN-j );
        if ( i > 0 ) j += i;
    }
    for ( i = j = 0; i < sizeof(hndshk.kerb_flag); ) {
        i = read ( sckt, &buf_l[j], sizeof(hndshk.kerb_flag)-j );
        if ( i > 0 ) j += i;
    }
    alarm(0);
    signal ( SIGALRM, sig_hndlr );
    bcopy ( buf_l, &hndshk.kerb_flag, sizeof(hndshk.kerb_flag) );

    /* cmp local hostname with accepted connection's hostname */
    /* used only to disallow talking to self across network   */
    if ( gethostname ( hostname, MAXHOSTNAMELEN ) == -1 ) quit ( "gethostname", 0 );
    if ( strcmp ( hostname, hndshk.hostname ) == 0 ) {
        send ( sckt, "BYE", MSG_OOB_SIZE, MSG_OOB );
        shutdown ( sckt, 2 );
        close ( sckt );
        cwrite ( "auditd: cannot send to self" );
        return(-1);
    }

    /* kerberos status on sender and receiver must match */
    if ( attr_g.kerb_auth != hndshk.kerb_flag ) {
        send ( sckt, "BYE", MSG_OOB_SIZE, MSG_OOB );
        shutdown ( sckt, 2 );
        close ( sckt );
        sprintf ( buf_l, "remote connection from %s refused; KERBEROS being used only on %s daemon",
        hndshk.hostname, attr_g.kerb_auth ? "local" : "remote" );
        cwrite ( buf_l );
        return(-1);
    }

    if ( getpeername ( sckt, (struct sockaddr *)&peername, &namelen ) < 0 ) {
        quit ( "getpeername", 0 );
        return(-1);
    }
    if ( (hostp = gethost_l((long)peername.sin_addr.s_addr)) == (char *)0 )
        return(-1);

#ifdef KERBEROS
    /* kerberos recvauth to establish trusted connection */
    if ( attr_g.kerb_auth ) {
        if ( kerb_ops ( 1, sckt, &peername ) == -1 ) return(-1);
        hostp = auth_data.pinst;
        if ( strcmp ( auth_data.pname, "auditd" ) ) {
            sprintf ( buf_l, "kerberos principal mismatch: %s - auditd", auth_data.pname );
            cwrite ( buf_l );
            return(-1);
        }
        krb_get_lrealm ( realm, 0 );
        if ( strcmp ( realm, auth_data.prealm ) ) {
            sprintf ( buf_l, "kerberos realm mismatch: %s - %s", realm, auth_data.prealm );
            cwrite ( buf_l );
            return(-1);
        }
    }
#endif KERBEROS

    /* check client name against file of legit hosts */
    if ( (fd = open ( AUDITD_CLNTS, 0 )) == -1 ) {
        sprintf ( buf_l, "auditd: unable to open %s for access check", AUDITD_CLNTS );
        cwrite ( buf_l );
        return(-1);
    }

    /* works by maintaining current auditd_clnt line completely in buffer */
    for ( len = 0; hostp[len] && len < MAXHOSTNAMELEN; len++ );
    i = read ( fd, buf_l, sizeof buf_l );
    for ( j = 0; j < i; j++ ) {

        /* update filename */
        if ( strncmp ( hostp, &buf_l[j], len ) == 0 )
            if ( buf_l[j+len] == ' ' || buf_l[j+len] == '\t' || buf_l[j+len] == '\n' ) {
                found = 1;
                break;
            }

        /* if don't have complete request, shift buffer and get more input */
        for ( ; buf_l[j] != '\n' && j < i; j++ );
        for ( end = j+1 < i ? j+1 : i; buf_l[end] != '\n' && end < i; end++ );
        if ( end == i ) {
            for ( start = j; start < end; start++ )
                buf_l[start-j] = buf_l[start];
            end -= j;
            j = -1;
            if ( (i = read ( fd, &buf_l[end], sizeof buf_l - end )) == 0 ) break;
            i += end;
        }
    }

    close ( fd );
    if ( found == 0 ) {
        sprintf ( buf_l, "auditd: host %s failed access to auditd master", hostp );
        cwrite ( buf_l );
        return(-1);
    }
    return(0);
}


/* check for input on sd[1,2,3] descriptors */
int chk_input ( sd1, sd2, sd3_p )
int sd1;        /* server-client socket descriptor  */
int sd2;        /* /dev/audit descriptor            */
int *sd3_p;     /* server-inet socket descriptor    */
{
    struct audit_attr attr_l;                        /* local audit attr     */
    struct sockaddr c_addr;                          /* connection address   */
    struct sockaddr_in inet_in;                      /* inet socket address  */
    int mask;                                        /* read descriptor mask */
    int ns_c = 0;                                    /* client socket        */
    int osuspend = 0;                                /* prev suspend status  */
    int suspend = 0;                                 /* suspend audit output */
    struct arp arp;                                  /* audit data ptrs      */
    char buf_l[BUF_SIZ];                             /* local buffer         */
    int conn;                                        /* connection number    */
    union wait status;                               /* child status         */
    int lastpass = 0;                                /* for final data flush */
    struct timeval timeout;                          /* select timeout       */
    int i, j, k;

#ifdef DEBUG3
    if ( attr_g.debug3 ) cwrite ( "debug: chk_input(a)" );
#endif DEBUG3

    /* get space for audit data */
    if ( (arp.aud_rec = (char *)sbrk(attr_g.nkbytes*1024)) == (char *)-1 )
        quit ( "sbrk", 1 );
    arp.aud_rec_b = arp.aud_rec;

    output_rec ( 0, "\0", (off_t *)0 ); /* device may check address space */
    timeout.tv_sec = timeout.tv_usec = 0;

    /* cycle until kill; extra passes on server auditd only */
    for ( ; attr_g.kill == 0 || (lastpass < 3 && attr_g.pid == 0); ) {

#ifdef DEBUG3
    if ( attr_g.debug3 ) cwrite ( "debug: chk_input(b)" );
#endif DEBUG3

        /* collect child signals */
        i = wait3 ( &status, WNOHANG, (struct rusage *)0 );
        for ( j = 1; i > 0 && j <= N_CONN; j++ )
            if ( child[j] == i ) child[j] = -1;

        /* check that admin socket still exists; if not, re-create it */
        setjmp(env);
        if ( (attr_g.pid == 0) && (access ( ss.sa_data, R_OK ) == -1) )
            if ( errno == ENOENT ) {
                close ( sd1 );
                if ( (sd1 = socket ( AF_UNIX, SOCK_STREAM, 0 )) < 0 ) quit ( "socket", 1 );
                if ( bind (sd1, &ss, sizeof(struct sockaddr)) < 0 ) quit ( "bind", 1 );
                if ( listen ( sd1, 5 ) < 0 ) quit ( "listen", 1 );
                cwrite ( "auditd: forced to re-create admin socket" );
            }

        /* in order to read all audit data, must follow sequence:
            0: turn off audswitch (done in handle_req)
            1: read all audit data available
            2: perform FLUSH_AUD_BUF (never otherwise; possible race condition)
            3: read all audit data available
          therefore, 3 passes to close down auditd */
        if ( lastpass == 2 )
            if ( audcntl ( FLUSH_AUD_BUF, (char *)0, 0, 0, 0 ) == -1 )
                quit ( "audcntl", 1 );

        /* select for pending connections */
        mask = 0;
        if ( sd1 >= 0 ) mask |= (1 << sd1);
        if ( sd2 >= 0 ) mask |= (1 << sd2);
        if ( *sd3_p >= 0 ) mask |= (1 << *sd3_p);
        if ( inet_out >= 0 ) mask |= (1 << inet_out);
        j = sd1 > sd2 ? sd1 : sd2;
        j = j > *sd3_p ? j : *sd3_p;
        j = j > inet_out ? j : inet_out;
        if ( lastpass == 0 )
            i = select ( j+1, &mask, (int *)0, (int *)0, (char *)0 );
        if ( lastpass )
            i = select ( j+1, &mask, (int *)0, (int *)0, &timeout );
        if ( lastpass ) lastpass++;
        if ( i <= 0 ) continue;

        /* monitor log */
        if ( (i = monitor_space()) <= attr_g.freepct && suspend == 0 ) {
            sprintf ( buf_l, "auditd: filesystem containing %s @ %d%% capacity", attr_g.pathname, 100-i );
            cwrite ( buf_l );
            suspend = overflow ( attr_g.overflow, &arp );
        }
        else suspend = 0;
        if ( suspend != osuspend ) {
            audgen_l ( AUDIT_SUSPEND, "audit data output suspend", suspend == SUSPEND ? "on" : "off" );
            sprintf ( buf_l, "auditd: audit data output suspend %s\n", suspend == SUSPEND ? "on" : "off" );
            cwrite ( buf_l );
            osuspend = suspend;
        }

        /* read from client descriptor (sd1) */
        if ( (sd1 >= 0) && (mask & (1 << sd1)) ) {

            /* accept connection */
            i = sizeof(struct sockaddr);
            if ( (ns_c = accept ( sd1, &c_addr, &i )) < 0 ) {
                if ( errno != EWOULDBLOCK ) quit ( "accept", 0 );
            }

            /* read attributes packet */
            else {

                /* this is a blocking socket; provide timeout to protect
                   against malicious/broken applications */
                alarm(4);
                signal ( SIGALRM, client_timeout );
                if ( setjmp(env) ) {
                    cwrite ( "auditd: client connection timeout" );
                    signal ( SIGALRM, sig_hndlr );
                    continue;
                }
                i = read ( ns_c, (char *)&attr_l, sizeof(struct audit_attr) );
                alarm(0);
                signal ( SIGALRM, sig_hndlr );

                /* handle client request */
                if ( i == sizeof(struct audit_attr) )
                    handle_req ( &attr_l, ns_c, &arp, sd3_p );
                else cwrite ( "invalid client request -- ignored" );

                /* shutdown socket */
                shutdown ( ns_c, 2 );
                close ( ns_c );

                if ( attr_g.kill == 1 ) {
                    lastpass = 1;
                    continue;
                }
            }
        }

        /* read new connections from inet (sd3) (server operation) */
        if ( (*sd3_p >= 0) && (attr_g.net_server == 1) && (mask & (1 << *sd3_p)) ) {

            /* accept new connections */
            j = sizeof(struct sockaddr_in);
            if ( (i = accept ( *sd3_p, (struct sockaddr *)&inet_in, &j )) < 0 ) {
                if ( errno != EWOULDBLOCK ) quit ( "accept", 0 );
            }

            /* exceeded max # connections */
            else {
                for ( conn = 1; (child[conn] >= 0) && (conn <= N_CONN); conn++ );
                if ( conn == N_CONN+1 ) {
                    send ( i, "BYE", MSG_OOB_SIZE, MSG_OOB );
                    shutdown ( i, 2 );
                    close(i);
                }

                /* perform hostname/peername access check (kerberos optional) */
                else if ( chk_access(i) == -1 ) {
                    send ( i, "BYE", MSG_OOB_SIZE, MSG_OOB );
                    shutdown ( i, 2 );
                    close ( i );
                }

                /* spawn child daemon to take care of this connection */
                else spawn_child ( &i, &sd1, &sd2, sd3_p, &arp, conn );
            }
        }

        /* read from kernel (sd2) */
        if ( (sd2 >= 0) && (mask & (1 << sd2)) ) {

            /* check for full buffer */
            if ( arp.aud_rec-arp.aud_rec_b == attr_g.nkbytes*1024 ) {
                if ( suspend != SUSPEND ) {
                    k = output_rec ( arp.aud_rec-arp.aud_rec_b, arp.aud_rec_b, &j );
                    arp.aud_rec -= k;
                }
                else arp.aud_rec = arp.aud_rec_b;
            }  

            /* read audit data from kernel via /dev/audit */
            for ( j = 0; (i = read ( sd2, arp.aud_rec, attr_g.nkbytes*1024 -
            (arp.aud_rec-arp.aud_rec_b) )) > 0; ) {
                /* output current set of audit records */
                arp.aud_rec += i;
                if ( ((arp.aud_rec-arp.aud_rec_b) >= (attr_g.nkbytes-4)*1024) || nobuf ) {
                    if ( suspend != SUSPEND ) {
                        k = output_rec ( arp.aud_rec-arp.aud_rec_b, arp.aud_rec_b, &j );
                        arp.aud_rec -= k;
                    }
                    else arp.aud_rec = arp.aud_rec_b;
                }
            }
            if ( i == -1 ) quit ( "read", 0 );

            /* check size of audit log for overflow condition */
            if ( j > MAXFILESIZ - (attr_g.nkbytes*1024) )
                overflow ( attr_g.overflow, &arp );

        }

        /* read sd3 net connection (child operation) */
        if ( (mask & (1 << *sd3_p)) && (*sd3_p >= 0) && (attr_g.pid > 0) )
            chk_input_net ( sd3_p, &arp, suspend );

        /* read from net_server auditd (inet_out) (client operation) */
        if ( (inet_out >= 0) && (attr_g.pathval == 'i') && (mask & (1 << inet_out)) )
            if ( recv ( inet_out, buf_l, MSG_OOB_SIZE, MSG_OOB ) > 0 ) {
                sprintf ( buf_l, "auditd: %s not receiving", attr_g.pathname );
                cwrite ( buf_l );
                shutdown ( inet_out, 2 );
                close ( inet_out );
                inet_out = -1;
                attr_g.pathval = '\0';
                if ( switch_log ( attr_g.audit2log, 1 ) == -1 )
                    if ( switch_log ( DEFAULT_LOG, 1 ) == -1 )
                        cwrite ( "auditd: data lost" );
                attr_g.audit2log[0] = '\0';
            }
    }

    /* flush buffers */
    i = output_rec ( arp.aud_rec - arp.aud_rec_b, arp.aud_rec_b, (off_t *)0 );
    arp.aud_rec -= i;
    audgen_l ( AUDIT_SHUTDOWN, "audit subsystem off", "" );
}


/* read sd3 net connection (child operation) */
chk_input_net ( sd3_p, arp, suspend )
int *sd3_p;         /* server-inet socket descriptor */
struct arp *arp;    /* audit data ptrs               */
int suspend;        /* status from monitor_space     */
{
    struct sockaddr_in sl, sf; /* addresses for KERBEROS    */
    char buf_l[BUF_SIZ];       /* local buffer              */
    unsigned long pktlen;
    int i, j, k;

#ifdef DEBUG3
    if ( attr_g.debug3 ) cwrite ( "debug: chk_input_net" );
#endif DEBUG3

    /* read audit data from inet */
    for ( j = k = 0; (i = read ( *sd3_p, arp->aud_rec, attr_g.nkbytes*1024 - 
    (arp->aud_rec - arp->aud_rec_b) )) > 0 || 
    attr_g.nkbytes*1024 == arp->aud_rec - arp->aud_rec_b; ) {

        /* output current set of audit records */
        k = 1;
        arp->aud_rec += i;
        if ( attr_g.kerb_auth == 0 ) {
            if ( ((arp->aud_rec-arp->aud_rec_b) >= (attr_g.nkbytes-4)*1024)
            || nobuf ) {
                if ( suspend != SUSPEND ) {
                    i = output_rec ( arp->aud_rec - arp->aud_rec_b, arp->aud_rec_b, &j );
                    arp->aud_rec -= i;
                }
                else arp->aud_rec = arp->aud_rec_b;
            }
        }

#ifdef KERBEROS
        /* use KERBEROS read safe routine */
        /* make sure at least length is read (first 6 bytes) */
        else if ( arp->aud_rec - arp->aud_rec_b > KRB_HDRSIZ ) {
            if ( suspend != SUSPEND ) {

                /* get packet length; must have complete packet (pktlen+31) bytes */
                bcopy ( &arp->aud_rec_b[2], &pktlen, sizeof pktlen );
                for ( ; arp->aud_rec - arp->aud_rec_b >= pktlen+KRB_PKTSIZ; ) {

                    /* unencapsulate data */
                    i = sizeof(struct sockaddr_in);
                    if ( getpeername ( *sd3_p, &sf, &i ) )
                        quit ( "getpeername", 0 );
                    i = sizeof(struct sockaddr_in);
                    if ( getsockname ( *sd3_p, &sl, &i ) )
                        quit ( "getsockname", 0 );
                    if ( i = krb_rd_safe ( arp->aud_rec_b, pktlen+KRB_PKTSIZ,
                    auth_data.session, &sf, &sl, &msg_data ) ) {
                        sprintf ( buf_l, "krb_rd_safe error %d", i );
                        cwrite ( buf_l );
                        send ( inet_out, "BYE", MSG_OOB_SIZE, MSG_OOB );
                        shutdown ( inet_out, 2 );
                        attr_g.kill = 1;
                        break;
                    }

                    /* output safe data locally; shift remaining data */
                    else {
                        output_rec ( msg_data.app_length, msg_data.app_data, &j );
                        for ( i = pktlen+KRB_PKTSIZ, j = 0; i < arp->aud_rec - arp->aud_rec_b; i++, j++ )
                            arp->aud_rec_b[j] = arp->aud_rec_b[i];
                        arp->aud_rec -= (pktlen+KRB_PKTSIZ);
                        bcopy ( &arp->aud_rec_b[2], &pktlen, sizeof pktlen );
                    }
                }
            }
            else arp->aud_rec = arp->aud_rec_b;
        }
#endif KERBEROS
    }
    if ( k == 0 && i == -1 && errno != EWOULDBLOCK ) quit ( "read", 0 );

    /* exit on receiving OOB data from client auditd */
    /* exit if selected and no data available */
    if ( (i = recv ( *sd3_p, buf_l, MSG_OOB_SIZE, MSG_OOB ) > 0) || (k == 0) ) {
        attr_g.kill = 1;
        bcopy ( "audX", buf_l, 5 );
        buf_l[3] = attr_g.pid+'0';
        unlink ( buf_l );
    }

    /* check size of audit log for overflow condition */
    if ( j > MAXFILESIZ - (attr_g.nkbytes*1024) )
        overflow ( attr_g.overflow, arp );
}


/* act as client; transfer attribute values to daemon server */
/* do not use quit ( *, 1 ) -- for server only               */
/* return 0 to run as server; -1 on error; 1 on success      */
int client ( child, attr_l, conn_name, outd )
int child;
struct audit_attr *attr_l;
struct sockaddr *conn_name;
int outd;
{
    int s_c;                    /* socket       */
    char buf_l[BUF_SIZ];        /* local buffer */
    char path_l[MAXPATHLEN];    /* directory    */
    int i, j;

    /* create socket */
    if ( (s_c = socket ( AF_UNIX, SOCK_STREAM, 0 )) < 0 ) {
        quit ( "socket", 0 );
        return(-1);
    }

    /* get working directory */
    if ( getwd ( path_l ) == 0 ) {
        quit ( "getwd", 0 );
        return(-1);
    }

    /* qualify relative pathnames with current directory */
    if ( attr_l->pathval == 'l' && attr_l->pathname[0] != '/' && attr_l->pathname[0] ) {
        for ( i = 0; attr_l->pathname[i]; i++ );
        for ( j = 0; path_l[j]; j++ );
        if ( i+j >= MAXPATHLEN-1 ) {
            write ( outd, "pathname too long\n", 18 );
            return(-1);
        }
        bcopy ( attr_l->pathname, &attr_l->pathname[j+1], i+1 );
        attr_l->pathname[j] = '/';
        bcopy ( path_l, attr_l->pathname, j );
    }

    if ( attr_l->audit2log[0] != '/' && attr_l->audit2log[0] ) {
        for ( i = 0; attr_l->audit2log[i]; i++ );
        for ( j = 0; path_l[j]; j++ );
        if ( i+j >= MAXPATHLEN-1 ) {
            write ( outd, "pathname too long\n", 18 );
            return(-1);
        }
        bcopy ( attr_l->audit2log, &attr_l->audit2log[j+1], i+1 );
        attr_l->audit2log[j] = '/';
        bcopy ( path_l, attr_l->audit2log, j );
    }

    if ( attr_l->console[0] != '/' && attr_l->console[0] ) {
        for ( i = 0; attr_l->console[i]; i++ );
        for ( j = 0; path_l[j]; j++ );
        if ( i+j >= MAXPATHLEN-1 ) {
            write ( outd, "pathname too long\n", 18 );
            return(-1);
        }
        bcopy ( attr_l->console, &attr_l->console[j+1], i+1 );
        attr_l->console[j] = '/';
        bcopy ( path_l, attr_l->console, j );
    }

    /* request connection */
    chdir ( AUDITD_DIR );
    if ( connect ( s_c, conn_name, sizeof(struct sockaddr) ) < 0 ) {
        if ( errno == ENOENT ) {
            close ( s_c );
            return(0);
        }
        else {
            quit ( "connect", 0 );
            return(-1);
        }
    }

    /* transmit packet */
    write ( s_c, (char *)attr_l, sizeof(struct audit_attr) );

    /* read response; this will block on i/o (reason for conditional) */
    if ( attr_l->pathval || attr_l->console[0] || attr_l->nkbytes 
    || attr_l->show || attr_l->query || attr_l->help )
        do {
            i = read ( s_c, buf_l, sizeof buf_l );
            write ( outd, buf_l, i );
        } while ( i > 0 );

    if ( child ) close ( s_c );
    return(1);
}


/* client timeout signal handler */
client_timeout()
{
    longjmp ( env, 1 );
}


/* have child process compress audit log */
compress ( filnam )
char *filnam;
{
    struct stat sbuf;
    int i;

#ifdef DEBUG3
    if ( attr_g.debug3 ) cwrite ( "debug: compress" );
#endif DEBUG3

    /* don't compress devices */
    if ( stat ( filnam, &sbuf ) == 0 )
        if ( (sbuf.st_mode & S_IFREG) == 0 ) return;

    if ( fork() == 0 ) {

        setpriority ( PRIO_PROCESS, getpid(), 0 );

        /* disassociate from controlling tty and process group */
        if ( setpgrp ( 0, getpid() ) == -1 ) quit ( "setpgrp", 0 );
        if ( (i = open ( "/dev/tty", 2 )) >= 0 ) {
            ioctl ( i, TIOCNOTTY, 0 );
            close(i);
        }
        for ( i = 0; i < _NFILE; i++ ) close(i);

        execl ( "/usr/ucb/compress", "compress", filnam, 0 );
        cwrite ( "auditd: failed to compress audit log" );
        _exit(1);
    }
}


/* output string to attr_g.console */
cwrite ( string )
char *string;
{
    long clock;
    int i, j;

    time ( &clock );
    if ( (i = open ( attr_g.console, O_RDWR|O_CREAT|O_APPEND, 0600 )) >= 0 ) {
#ifdef DEBUG3
        if ( attr_g.debug3 ) {
            write ( i, itoa(attr_g.pid,1), 1 );
            write ( i, ": ", 2 );
        }
#endif DEBUG3
        for ( j = 0; string[j]; j++ );
        write ( i, string, j );
        write ( i, " - ", 3 );
        write ( i, ctime(&clock), 25 );
        close(i);
    }

    if ( (i = open ( "/dev/tty", 2 )) >= 0 ) {
        ioctl ( i, TIOCNOTTY, 0 );
        close(i);
    }
}


/* open audit log on filesystem with most free space */
int find_filsys()
{
    int cntxt = 0;                  /* getmnt pointer           */
    int fdl;                        /* audit log file ptr       */
    long free = 0;                  /* amount of free space     */
    struct fs_data fs_info[NFLS];   /* filsys data              */
    char path[MAXPATHLEN];          /* new audit log pathname   */
    struct stat sbuf;               /* audit log dir info       */
    dev_t dev_l, dev_l2;            /* log filesystem devices   */
    int cont;
    int i, j, k;

#ifdef DEBUG3
    if ( attr_g.debug3 ) cwrite ( "debug: find_filsys" );
#endif DEBUG3

    /* get local dev_t; if not, will be possible later */
    if ( stat ( attr_g.pathname, &sbuf ) == -1 ) return(-1);
    dev_l = dev_l2 = sbuf.st_dev;

    do {

        /* get filesystem info */
        if ( (cont = getmnt ( &cntxt, fs_info, NFLS*sizeof(struct fs_data), NOSTAT_MANY, (char *)0 )) < 0 ) {
            quit ( "getmnt", 0 );
            return(-1);
        }

        /* find writeable filesystem with most freespace */
        for ( i = cont; i; i-- ) {
            if ( (long)fs_info[i-1].fd_bfreen > free ) {

                /* check flags that this is a root-owned writeable dir */
                if ( stat ( fs_info[i-1].fd_path, &sbuf ) != 0 ) quit ( "stat", 0 );
                if ( (sbuf.st_mode & 0770000) != 0040000 ) continue;
                if ( sbuf.st_uid != 0 ) continue;
                if ( access ( fs_info[i-1].fd_path, W_OK ) == -1 ) continue;

                /* build pathname; update free value */
                free = fs_info[i-1].fd_bfreen;
                for ( j = 0; (path[j] = fs_info[i-1].fd_path[j]) && (j < MAXPATHLEN); j++ );
                if ( path[j-1] != '/' ) path[j++] = '/';
                for ( k = 0; attr_g.pathname[k]; k++ );
                for ( ; k > 0 && attr_g.pathname[k-1] != '/'; k-- );
                for ( ; (path[j] = attr_g.pathname[k]) && (j < sizeof path); j++, k++ );
                dev_l2 = fs_info[i-1].fd_dev;
            }
        }

    } while ( cont );

    /* already have the filesystem with the most free space */
    if ( dev_l2 == dev_l ) return(-1);

    /* generate audit record */
    for ( i = 0; path[i] && i < MAXPATHLEN; i++ );
    if ( log_indx == -1 ) set_log_indx ( path );
    else {
        log_indx = (log_indx+1)%MAX_LOG_INDX;
        bcopy ( itoa(log_indx,3), &path[i-3], 3 );
        path[i] = '\0';
    }
    audgen_l ( AUDIT_LOG_CHANGE, "audit log change", path );

    /* change audit log */
    if ( attr_g.pathval == 'l' ) {
        close(fda);
        compress ( attr_g.pathname );
        if ( (fdl = open ( path, O_RDWR|O_APPEND|O_CREAT, 0600 )) < 0 ) {
            quit ( "open", 0 );
            return(-1);
        }
        fda = fdl;
        for ( i = 0; (attr_g.pathname[i] = path[i]) && (i < MAXPATHLEN); i++ );
    }

    return(0);
}


/* return hostname associated with ipaddr */
char *gethost_l ( ipaddr )
long ipaddr;
{
    struct hostent *hostp;
    static long ipaddr_prev = 0;
    static char h_name_prev[128];
    int i;

#ifdef DEBUG3
    if ( attr_g.debug3 ) cwrite ( "debug: gethost_l" );
#endif DEBUG3

    if ( ipaddr_prev && ipaddr == ipaddr_prev ) return ( h_name_prev );
    if ( (hostp = gethostbyaddr ( (char *)&ipaddr, sizeof(long), AF_INET )) == (struct hostent *)0 )
        return ( (char *)0 );

    /* save hostname, ipaddr for next iteration */
    ipaddr_prev = ipaddr;
    for ( i = 0; hostp->h_name[i] && i < 128; i++ );
    bcopy ( hostp->h_name, h_name_prev, i+1 );

    return ( hostp->h_name );
}


/* handle client request */
handle_req ( attr_l, ns, arp, sd3_p )
struct audit_attr *attr_l;              /* local audit attr         */
int ns;                                 /* server-client socket     */
struct arp *arp;                        /* audit data pointers      */
int *sd3_p;                             /* inet socket ptr          */
{
    static mem_size = DEFAULT_AUD_BUF;  /* size of audit buffer     */
    char pathnam_l[MAXPATHLEN];
    char pathnam2_l[MAXPATHLEN];
    char hostname[MAXHOSTNAMELEN];
    char buf_l[BUF_SIZ];                /* local buffer             */
    struct hostent *hostent;
    static char *overflow_act[] =
    { "switch to secondary pathname",
      "shutdown system",
      "use mounted filesystem with most freespace",
      "suspend audit until space becomes available",
      "overwrite current audit log" };
#define N_OVERFLOW_OPTS 5

    struct sockaddr sl;
    struct stat statbuf;
    int dvc = 0;
    int i, j;
    char ch;

#ifdef DEBUG3
    if ( attr_g.debug3 ) cwrite ( "debug: handle_req" );
#endif DEBUG3

    /* divert request to child daemon (must be first check) */
    if ( attr_l->pid > 0 ) {
        sl.sa_family = AF_UNIX;
        bcopy ( "audX", sl.sa_data, 5 );
        sl.sa_data[3] = attr_l->pid+'0';
        attr_l->net_server = 0;
        attr_l->pid = -1;
        client ( 1, attr_l, &sl, ns );
        return;
    }

    /* toggle net_server status */
    if ( attr_l->net_server && attr_g.pid == 0 ) {
        attr_g.net_server ^= 1;
        if ( net_serv ( attr_g.net_server, sd3_p ) == -1 )
            attr_g.net_server = 0;
    }

    /* update misc attributes */
    if ( attr_l->freepct >= 0 && attr_l->freepct <= 100 ) attr_g.freepct = attr_l->freepct;
    if ( attr_l->overflow ) {
        ch = attr_l->overflow < 'a' ? attr_l->overflow+'a'-'A' : attr_l->overflow;
        if ( ch-'a' < N_OVERFLOW_OPTS && ch-'a' >= 0 ) {
            attr_g.overflow = attr_l->overflow < 'a' ? attr_l->overflow+'a'-'A' : attr_l->overflow;
            audgen_l ( AUDIT_SETUP, "overflow action", overflow_act[attr_g.overflow-'a'] );
        }
    }
    if ( attr_l->kerb_auth && attr_g.pid == 0 ) attr_g.kerb_auth ^= 1;
    if ( attr_l->timeout > 0 ) attr_g.timeout = attr_l->timeout;
#ifdef DEBUG3
    if ( attr_l->debug3 ) attr_g.debug3 ^= 1;
#endif DEBUG3

    /* check for routing to self by comparing 'official' names of hosts */
    if ( attr_l->pathval == 'i' && attr_g.pid == 0 ) {
        gethostname ( hostname, MAXHOSTNAMELEN );
        if ( hostent = gethostbyname(hostname) )
            strncpy ( pathnam_l, (char *)hostent->h_name, MAXPATHLEN );
        else {
            attr_l->pathval = '\0';
            write ( ns, "-- invalid hostname --\n", 23 );
        }

        if ( hostent = gethostbyname(attr_l->pathname) )
            strncpy ( pathnam2_l, (char *)hostent->h_name, MAXPATHLEN );
        else {
            attr_l->pathval = '\0';
            write ( ns, "-- invalid hostname --\n", 23 );
        }

        if ( attr_l->pathval && strcmp ( pathnam_l, pathnam2_l ) == 0 ) {
            bcopy ( DEFAULT_LOG, attr_l->pathname, sizeof DEFAULT_LOG );
            attr_l->pathval = 'l';
        }
    }

    /* change audit log pathname */
    if ( attr_l->pathval == 'l' ) {
        i = output_rec ( arp->aud_rec - arp->aud_rec_b, arp->aud_rec_b, (off_t *)0 );
        arp->aud_rec -= i;
        switch_log ( attr_l->pathname, 0 );
    }

    /* increment log_indx (suffix on pathname) */
    /* if device, switch to alternate or default pathname */
    if ( attr_l->incr ) {
        i = output_rec ( arp->aud_rec - arp->aud_rec_b, arp->aud_rec_b, (off_t *)0 );
        arp->aud_rec -= i;
        if ( stat ( attr_g.pathname, &statbuf ) == 0 )
            if ( (statbuf.st_mode & S_IFREG) == 0 )
                dvc = 1;
        if ( dvc == 1 ) {
            if ( attr_g.audit2log[0] != '\0' ) {
                for ( i = 0; attr_g.audit2log[i] && i < MAXPATHLEN; i++ );
                bcopy ( attr_g.audit2log, pathnam_l, i );
            }
            else bcopy ( DEFAULT_LOG, pathnam_l, sizeof DEFAULT_LOG );
        }
        else {
            for ( i = 0; attr_g.pathname[i] && i < MAXPATHLEN; i++ );
            bcopy ( attr_g.pathname, pathnam_l, i );
            pathnam_l[i-4] = '\0';
        }
        switch_log ( pathnam_l, 0 );
    }

    /* change alternate log pathname */
    if ( attr_l->audit2log[0] != '\0' )
        for ( i = 0; (attr_g.audit2log[i] = attr_l->audit2log[i]) && (i < MAXPATHLEN); i++ );

    /* change idea of console */
    if ( attr_l->console[0] != '\0' ) {
        if ( (i = open ( attr_l->console, O_RDWR|O_CREAT|O_APPEND, 0600 )) < 0 )
            write ( ns, "-- invalid console name --\n", 27 );
        else {
            close(i);
            for ( j = 0; attr_g.console[j] = attr_l->console[j]; j++ );
            if ( (i = open ( "/dev/tty", 2 )) > 0 ) {
                ioctl ( i, TIOCNOTTY, 0 );
                close(i);
            }
        }
    }

    /* transmit data over inet */
    if ( attr_l->pathval == 'i' && attr_g.pid == 0 ) {
        i = output_rec ( arp->aud_rec - arp->aud_rec_b, arp->aud_rec_b, (off_t *)0 );
        arp->aud_rec -= i;
        audgen_l ( AUDIT_LOG_CHANGE, "audit log change to host", attr_l->pathname );
        if ( attr_g.pathval == 'l' ) {
            close ( fda );
            compress ( attr_g.pathname );
        }
        if ( attr_g.pathval == 'i' ) {
            send ( inet_out, "BYE", MSG_OOB_SIZE, MSG_OOB );
            shutdown ( inet_out, 2 );
            close ( inet_out );
            inet_out = -1;
        }
        for ( i = 0; (attr_g.pathname[i] = attr_l->pathname[i]) && (i < MAXPATHLEN); i++ );
        attr_g.pathval = attr_l->pathval;
        /* must be able to xfer complete KERBEROS packet, so set nbytes */
        if ( attr_g.kerb_auth ) attr_l->nkbytes = KRB_SENDSIZ;
        output_rec ( 0, "\0", (off_t *)0 );
        sprintf ( buf_l, "auditd: routing audit data to %s", attr_g.pathname );
        cwrite ( buf_l );
    }

    /* change size of audit data buffer; flush buffer */
    if ( attr_l->nkbytes == 0 ) nobuf = 1;
    if ( attr_l->nkbytes >= MIN_AUD_BUF && attr_g.pid == 0 ) {
        nobuf = 0;
        i = output_rec ( arp->aud_rec - arp->aud_rec_b, arp->aud_rec_b, (off_t *)0 );
        arp->aud_rec -= i;

        /* do not use brk() because of other memory allocation - should use malloc */
        if ( attr_l->nkbytes > mem_size ) {
            if ( (arp->aud_rec = (char *)sbrk(attr_l->nkbytes*1024)) == (char *)-1 )
                write ( ns, "-- insufficient mem --\n", 23 );
            else {
                mem_size = attr_l->nkbytes;
                arp->aud_rec_b = arp->aud_rec;
                attr_g.nkbytes = attr_l->nkbytes;
            }
        }
        else attr_g.nkbytes = attr_l->nkbytes;
    }

    /* kill process */
    if ( attr_l->kill ) attr_g.kill = attr_l->kill;
    if ( attr_g.pid == 0 && attr_g.kill ) {
        if ( audcntl ( SET_AUDSWITCH, (char *)0, 0, 0, 0 ) == -1 )
            quit ( "audcntl", 1 );
    }

    /* process show status, help requests */
    if ( attr_l->show ) show_stat ( ns, *sd3_p );
    if ( attr_l->help ) show_help ( ns );

    /* dump audit buffer */
    if ( attr_l->dump ) {
        i = output_rec ( arp->aud_rec - arp->aud_rec_b, arp->aud_rec_b, (off_t *)0 );
        arp->aud_rec -= i;
    }

    /* query name from server */
    if ( attr_l->query ) {
        for ( i = 0; attr_g.pathname[i] && i < MAXPATHLEN; i++ );
        write ( ns, attr_g.pathname, i );
        write ( ns, "\n", 1 );
    }

}


/* initialize attributes structure */
init_attr ( attr_p )
struct audit_attr *attr_p;
{
    attr_p->audit2log[0] = '\0';
    attr_p->console[0] = '\0';
#ifdef DEBUG3
    attr_p->debug3 = 0;
#endif DEBUG3
    attr_p->dump = 0;
    attr_p->freepct = -1;
    attr_p->help = 0;
    attr_p->incr = 0;
    attr_p->kerb_auth = 0;
    attr_p->kill = 0;
    attr_p->net_server = 0;
    attr_p->nkbytes = -1;
    attr_p->overflow = '\0';
    attr_p->pathname[0] = '\0';
    attr_p->pathval = '\0';
    attr_p->pid = -1;
    attr_p->query = 0;
    attr_p->show = 0;
    attr_p->timeout = -1;
}


/* convert integer/long decimal to alphanumeric string; return ptr to string */
char *itoa ( num, min_width )
int num;
int min_width;
{
    char num_rep[13];
    int i;

    bcopy ( "000000000000", num_rep, sizeof num_rep );

    for ( i = sizeof num_rep-1; (i == sizeof num_rep - 1) || (num > 0 && i >= 0); num = num/10, i-- )
        num_rep[i] = num%10 + '0';

    min_width++;
    i = i < sizeof num_rep - min_width ? i : sizeof num_rep - min_width;
    return ( &num_rep[++i] );
}


/* kerberos operations; return -1 on failure */
#ifdef KERBEROS
int kerb_ops ( op, fd, sinp )
int op;
int fd;
struct sockaddr_in *sinp;
{
    char version[9];
    KTEXT_ST ticket;                    /* kerberos ticket          */
    Key_schedule ksched;
    char realm[REALM_SZ];               /* KERBEROS realm           */

    static int initialized = 0;         /* init flag                */
    struct sockaddr_in sl;              /* local inet address       */
    char hostname[MAXHOSTNAMELEN];
    struct timeval tp;
    struct timezone tzp;
    char buf_l[BUF_SIZ];
    char *hostp;
    int i;

#ifdef DEBUG3
    if ( attr_g.debug3 ) cwrite ( "debug: kerb_ops" );
#endif DEBUG3

    gethostname ( hostname, MAXHOSTNAMELEN );
    for ( i = 0; hostname[i] != '.' && hostname[i] && i < MAXHOSTNAMELEN; i++ );
    if ( i < MAXHOSTNAMELEN ) hostname[i] = '\0';

    /* get socket attributes; make non-blocking for kerberos */
    if ( fd >= 0 ) {
        i = sizeof(struct sockaddr_in);
        if ( getsockname ( fd, &sl, &i ) ) {
            quit ( "getsockname", 0 );
            return(-1);
        }
        else {
            i = 0;   /* make socket blocking for KERBEROS */
            if ( ioctl ( fd, FIONBIO, &i ) < 0 ) {
                quit ( "ioctl: failed to modify socket for KERBEROS", 0 );
                return(-1);
            }
        }
    }

    alarm(KRB_TIMEOUT);
    signal ( SIGALRM, kerb_timeout );
    if ( setjmp(env) ) {
        cwrite ( "auditd: kerberos timeout" );
        signal ( SIGALRM, sig_hndlr );
        return(-1);
    }

    if ( initialized == 0 ) {
        initialized++;
        i = krb_svc_init ( "auditd", hostname, NULL, 0, NULL, TKT_FILE_L );
        if ( i != RET_OK ) {
            sprintf ( buf_l, "auditd: krb_svc_init bombed with %d", i );
            cwrite ( buf_l );
        }
    }

    switch ( op ) {

    case 0x1: /* KRB_RECVAUTH */
        if ( (i = krb_recvauth ( KOPT_DO_MUTUAL, fd, &ticket, "auditd",
        hostname, sinp, &sl, &auth_data, "", ksched, version )) != KSUCCESS ) {
            i = 1;
            if ( ioctl ( fd, FIONBIO, &i ) < 0 ) quit ( "ioctl", 0 );
            alarm(0);
            signal ( SIGALRM, sig_hndlr );
            return(-1);
        }

        /* restore socket to non-blocking state */
        i = 1;
        if ( ioctl ( fd, FIONBIO, &i ) < 0 ) quit ( "ioctl", 0 );

        /* compare given hostname with kerberos authenticated name */
        if ( (hostp = gethost_l((long)sinp->sin_addr.s_addr)) == (char *)0 ) {
            alarm(0);
            signal ( SIGALRM, sig_hndlr );
            return(-1);
        }
        i = strcmp ( hostp, auth_data.pinst );
        if ( i ) {
            alarm(0);
            signal ( SIGALRM, sig_hndlr );
            sprintf ( buf_l, "%s and %s do not match", hostp, auth_data.pinst );
            cwrite ( buf_l );
            return(-1);
        }
        break;


    case 0x2: /* KRB_SENDAUTH */
        for ( i = 0; attr_g.pathname[i] != '.' && attr_g.pathname[i] && i < MAXHOSTNAMELEN; i++ )
            hostname[i] = attr_g.pathname[i];
        hostname[i] = '\0';
        gettimeofday ( &tp, &tzp );

        krb_get_lrealm ( realm, 0 );
        if ( (i = krb_sendauth ( KOPT_DO_MUTUAL, fd, &ticket, "auditd", hostname,
        realm, tp.tv_usec, &msg_data, &cred, ksched, &sl, sinp, "" )) != KSUCCESS ) {
            alarm(0);
            signal ( SIGALRM, sig_hndlr );
            sprintf ( buf_l, "auditd: KERBEROS krb_sendauth error %s (%d)", krb_err_txt[i], i );
            cwrite ( buf_l );
            i = 1;
            if ( ioctl ( fd, FIONBIO, &i ) < 0 ) quit ( "ioctl", 0 );
            return(-1);
        }
        i = 1;
        if ( ioctl ( fd, FIONBIO, &i ) < 0 ) quit ( "ioctl", 0 );
        break;

    }
    alarm(0);
    signal ( SIGALRM, sig_hndlr );
    return(0);
}
#endif KERBEROS        


/* kerberos timeout signal handler */
#ifdef KERBEROS
kerb_timeout()
{
    longjmp ( env, 1 );
}
#endif KERBEROS


/* return % free disk space for filesystem containing audit log */
/* return 100 if filesystem is remote or if using device        */
int monitor_space()
{
    struct stat sbuf;                       /* audit log data */
    static struct fs_data fs_info;          /* filesys data   */
    int cntxt = 0;                          /* getmnt pointer */
    char buf_l[BUF_SIZ];                    /* local buffer   */
    char path_l[MAXPATHLEN];                /* local pathname */
    int i, j;

#ifdef DEBUG3
    if ( attr_g.debug3 ) cwrite ( "debug: monitor_space" );
#endif DEBUG3

    /* return 100 for remote filesystems and devices */
    if ( attr_g.pathval != 'l' ) return(100);
    if ( stat ( attr_g.pathname, &sbuf ) == 0 ) {
        if ( (sbuf.st_mode & S_IFREG) == 0 ) return(100);
    }

    /* re-create audit log if necessary */
    else if ( errno == ENOENT ) {                
        sprintf ( buf_l, "auditd: could not find %s", attr_g.pathname );
        cwrite ( buf_l );
        close ( fda );
        for ( j = 0; attr_g.pathname[j] && j < MAXPATHLEN-4; j++ );
        if ( log_indx == -1 ) set_log_indx ( attr_g.pathname );
        else {
            log_indx = (log_indx+1)%MAX_LOG_INDX;
            bcopy ( itoa(log_indx,3), &attr_g.pathname[j-3], 3 );
            attr_g.pathname[j] = '\0';
        }
        sprintf ( buf_l, "auditd: setting audit log to %s", attr_g.pathname );
        cwrite ( buf_l );
        if ( (fda = open ( attr_g.pathname, O_RDWR|O_APPEND|O_CREAT, 0600 )) < 0 )
            quit ( "open", 0 );
        audgen_l ( AUDIT_LOG_CREAT, "re-created audit log", attr_g.pathname );
    }

    /* get filesystem info */
    if ( (i = getmnt ( &cntxt, &fs_info, sizeof(struct fs_data), STAT_ONE, attr_g.pathname )) != 1 ) {
        if ( errno != ENOENT ) {
            quit ( "getmnt", 0 );
            return(0);
        }

        /* if not found; try parent directory */
        else {
            for ( i = 0; path_l[i] = attr_g.pathname[i] && i < MAXPATHLEN; i++ );
            if ( i == MAXPATHLEN ) i--;
            for ( ; path_l[i] != '/' && i; i-- );
            path_l[i] = '\0';
            cntxt = 0;
            if ( (i = getmnt ( &cntxt, &fs_info, sizeof(struct fs_data),
            STAT_ONE, path_l )) != 1 ) {
                quit ( "getmnt", 0 );
                return(0);
            }
        }
    }

    /* return freespace; derived from df code */
    if ( fs_info.fd_btot == 0 ) i = 100;
    else i = 100 - ( (fs_info.fd_btot-fs_info.fd_bfree) * 100 /
    (fs_info.fd_btot - (fs_info.fd_bfree - fs_info.fd_bfreen)) );

    return(i);
}


/* open/close net_server auditd's receiving inet socket; return -1 on failure */
int net_serv ( toggle, sin_p )
int toggle;     /* 1 - open; 0 - close  */
int *sin_p;     /* inet socket ptr      */
{
    struct sockaddr_in si;      /* internet socket                  */
    struct servent *serventp;   /* service entry pointer            */
    int i = 1;

#ifdef DEBUG3
    if ( attr_g.debug3 ) cwrite ( "debug: net_serv" );
#endif DEBUG3

    /* open net_server's receiving inet socket */
    if ( toggle ) {
        si.sin_family = AF_INET;
        if ( (serventp = getservbyname ( "auditd", "tcp" )) == (struct servent *)0 ) {
            quit ( "getservbyname", 0 );
            return(-1);
        }
        else si.sin_port = serventp->s_port;
        si.sin_addr.s_addr = '\0';
        if ( (*sin_p = socket ( AF_INET, SOCK_STREAM, 0 )) < 0 ) {
            quit ( "socket", 0 );
            return(-1);
        }
        if ( bind (*sin_p, (struct sockaddr *)&si, sizeof(struct sockaddr_in)) < 0 ) {
            quit ( "bind", 0 );
            close ( *sin_p );
            *sin_p = -1;
            return(-1);
        }
        if ( ioctl ( *sin_p, FIONBIO, &i ) < 0 ) {
            quit ( "ioctl", 0 );
            close ( *sin_p );
            *sin_p = -1;
            return(-1);
        }
        listen ( *sin_p, 5 );
    }

    /* close receiving inet socket; tell connected auditd */
    else {
        send ( *sin_p, "BYE", MSG_OOB_SIZE, MSG_OOB );
        shutdown ( *sin_p, 2 );
        close ( *sin_p );
        *sin_p = -1;
    }
    return(0);
}


/* output audit data to log; return # bytes output */
int output_rec ( nbytes, aud_recp, log_len )
int nbytes;
char *aud_recp;
off_t *log_len;                  /* length of audit log                   */
{
    struct sockaddr_in ss;       /* inet socket address                   */
    struct servent *serventp;    /* service entry pointer                 */
    char pathname_l[MAXPATHLEN]; /* tmp copy of audit log pathname        */
    char buf_l[BUF_SIZ];         /* local buffer                          */
    struct hostent *hp;          /* host entry                            */
    struct stat sbuf;            /* audit log data                        */
    struct arp arp_l;            /* local audit record ptrs               */
    struct hndshk hndshk;        /* handshake between daemons             */
    char *ptr;
    int i, j, k;
#ifdef KERBEROS
    struct sockaddr_in sl, sf;
    char krb_buf[KRB_RECVSIZ*1024]; /* must be > KRB_SENDSIZ; has KRB data   */
#endif KERBEROS

#define FAIL_OUTPUT(code,string,name) \
{ \
    i = 0; \
    if ( switch_log ( attr_g.audit2log, 1 ) == -1 ) { \
        if ( attr_g.pid == 0 ) { \
            if ( switch_log ( DEFAULT_LOG, 1 ) == -1 ) { \
                cwrite ( "auditd: data lost" ); \
                i == -1; \
            } \
        } \
        else { \
            if ( switch_log ( DEFAULT_LOG_REM, 1 ) == -1 ) { \
                cwrite ( "auditd: data lost" ); \
                i == -1; \
            } \
        } \
    } \
    if ( i == 0 ) { \
        if ( code ) audgen_l ( code, string, name ); \
        attr_g.audit2log[0] = '\0'; \
        nbytes = output_rec ( nbytes, aud_recp, (off_t *)0 ); \
    } \
    shutdown ( inet_out, 2 ); \
    close ( inet_out ); \
    inet_out = -1; \
    return ( nbytes ); \
}

#ifdef DEBUG3
    if ( attr_g.debug3 ) cwrite ( "debug: output_rec" );
#endif DEBUG3

    if ( log_len ) *log_len = 0;
    if ( nbytes < 0 ) {
        sprintf ( buf_l, "output_rec: nbytes = %d", nbytes );
        cwrite ( buf_l );
        return(0);
    }

    /* send audit data over inet */
    if ( attr_g.pathval == 'i' ) {

        /* open a connection to the net_server auditd */
        if ( inet_out == -1 ) {
            if ( (inet_out = socket ( AF_INET, SOCK_STREAM, 0 )) < 0 ) quit ( "socket", 0 );
            ss.sin_family = AF_INET;
            if ( (serventp = getservbyname ( "auditd", "tcp" )) == (struct servent *)0 ) quit ( "getservbyname", 0 );
            else ss.sin_port = serventp->s_port;

            /* get hostname and address */
            if ( (hp = gethostbyname(attr_g.pathname)) == (struct hostent *)0 ) {
                cwrite ( "auditd: invalid host" );
                FAIL_OUTPUT ( 0, (char *)0, (char *)0 );
            }
            bcopy ((char *)hp->h_addr, (char *)&ss.sin_addr, hp->h_length );

            /* ping remote host */
            if ( ping(attr_g.pathname) ) {
                cwrite ( "auditd: unreachable host" );
                FAIL_OUTPUT ( 0, (char *)0, (char *)0 );
            }

            if ( connect ( inet_out, (struct sockaddr *)&ss, sizeof(struct sockaddr_in) ) < 0 ) {
                quit ( "connect", 0 );
                FAIL_OUTPUT ( 0, (char *)0, (char *)0 );
            }

            /* send hostname to server for check against talking to self */
            /* send kerberos status */
            if ( gethostname ( hndshk.hostname, MAXHOSTNAMELEN ) )
                quit ( "hostname", 0 );
            hndshk.kerb_flag = attr_g.kerb_auth;
            if ( write ( inet_out, (char *)&hndshk, sizeof(struct hndshk) ) != sizeof(struct hndshk) )
                quit ( "write", 0 );

#ifdef KERBEROS
            /* krb_sendauth to establish trusted connection */
            if ( attr_g.kerb_auth )
                if ( kerb_ops ( 2, inet_out, &ss ) == -1 )
                    FAIL_OUTPUT ( 0, (char *)0, (char *)0 );
#endif KERBEROS
        }

        ptr = aud_recp;
        j = nbytes;
#ifdef KERBEROS
        /* encapsulate data in KERBEROS packet */
        if ( attr_g.kerb_auth ) {
            i = sizeof(struct sockaddr_in);
            if ( getpeername ( inet_out, &sf, &i ) ) {
                quit ( "getpeername", 0 );
                for ( i = 0; (pathname_l[i] = attr_g.pathname[i]) && (i < MAXPATHLEN); i++ );
                FAIL_OUTPUT ( AUDIT_XMIT_FAIL, "failed to send data to host", pathname_l );
            }
            i = sizeof(struct sockaddr_in);
            if ( getsockname ( inet_out, &sl, &i ) )
                quit ( "getsockname", 0 );
            if ( (j = krb_mk_safe ( aud_recp, krb_buf, nbytes, cred.session, &sl, &sf )) < nbytes ) {
                sprintf ( buf_l, "krb_mk_safe returned %d; must output %d", i, nbytes );
                cwrite ( buf_l );
                for ( i = 0; (pathname_l[i] = attr_g.pathname[i]) && (i < MAXPATHLEN); i++ );
                FAIL_OUTPUT ( AUDIT_XMIT_FAIL, "failed to send data to host", pathname_l );
            }
            else ptr = krb_buf;
        }
#endif KERBEROS

        /* output j bytes to net_server auditd; with KERBEROS, output all data */
        i = 0;
        do {
            if ( (k = write ( inet_out, &ptr[i], j-i )) < 0 ) {
                if ( errno == EWOULDBLOCK ) continue;
                sprintf ( buf_l, "auditd: could not send to %s", attr_g.pathname );
                cwrite ( buf_l );
                for ( i = 0; (pathname_l[i] = attr_g.pathname[i]) && (i < MAXPATHLEN); i++ );
                FAIL_OUTPUT ( AUDIT_XMIT_FAIL, "failed to send data to host", pathname_l );
            }
            else i += k;
        } while ( (i < j) && attr_g.kerb_auth );
        if ( attr_g.kerb_auth == 0 ) nbytes = k;
    }

    /* output data into a single local file */
    else if ( attr_g.pathval == 'l' ) {
        if ( (nbytes = write ( fda, aud_recp, nbytes )) < 0 ) {
            nbytes = 0;
            sprintf ( buf_l, "auditd: could not write to %s", attr_g.pathname );
            cwrite ( buf_l );
            if ( switch_log ( attr_g.audit2log, 1 ) == -1 ) {
                if ( switch_log ( DEFAULT_LOG, 1 ) == -1 ) {
                    cwrite ( "auditd: data lost" );
                    arp_l.aud_rec_b = aud_recp;
                    arp_l.aud_rec = nbytes + aud_recp;
                    overflow ( attr_g.overflow, &arp_l );
                }
            }
            else attr_g.audit2log[0] = '\0';
        }
        if ( fstat ( fda, &sbuf ) == -1 ) quit ( "fstat", 0 );
        else if ( log_len && (sbuf.st_mode & S_IFREG) ) *log_len = sbuf.st_size;
    }

    return ( nbytes );
}


/* take action when threshold exceeded; return SUSPEND to suspend auditing */
int overflow ( action, arp )
char action;        /* action to take on overflow   */
struct arp *arp;    /* audit data ptrs              */
{
    char buf_l[BUF_SIZ];    /* local buffer         */
    char path_l[MAXPATHLEN];/* local pathname       */
    int  fdl;               /* local logfile desc   */
    int i;

#ifdef DEBUG3
    if ( attr_g.debug3 ) cwrite ( "debug: overflow" );
#endif DEBUG3

    switch ( action ) {

    case '\0':  /* no action specified */
                return(0);
                break;

    case 'A':   /* switch to secondary pathname */
    case 'a':   if ( attr_g.audit2log[0] == '\0' ) {
                    cwrite ( "auditd: no alternate audit log set" );
                    break;
                }
                i = output_rec ( arp->aud_rec - arp->aud_rec_b, arp->aud_rec_b, (off_t *)0 );
                arp->aud_rec -= i;
                if ( switch_log ( attr_g.audit2log, 0 ) == 0 )
                    attr_g.overflow = '\0';
                attr_g.audit2log[0] = '\0';
                break;

    case 'B':   /* shutdown system */
    case 'b':   i = output_rec ( arp->aud_rec - arp->aud_rec_b, arp->aud_rec_b, (off_t *)0 );
                arp->aud_rec -= i;
                cwrite ( "auditd: overflow condition -> shutdown" );
                audgen_l ( AUDIT_REBOOT, "audit overflow -> shutdown", "" );
                sync();                
                fsync ( fda );
                if ( reboot ( RB_HALT ) == -1 ) quit ( "reboot", 0 );
                audgen_l ( AUDIT_REBOOT, "audit overflow -> shutdown", "failed" );
                break;

    case 'C':   /* use mounted filesystem with most free space */
    case 'c':   i = output_rec ( arp->aud_rec - arp->aud_rec_b, arp->aud_rec_b, (off_t *)0 );
                arp->aud_rec -= i;
                if ( find_filsys() == -1 )
                    cwrite ( "auditd: unable to find any available filesystem" );
                else {
                    sprintf ( buf_l, "auditd: audit log changed to %s\n", attr_g.pathname );
                    cwrite ( buf_l );
                }
                break;

    case 'D':   /* suspend auditing until space is available */
    case 'd':   return(SUSPEND);
                break;

    case 'E':   /* overwrite current audit log */
    case 'e':   if ( attr_g.pathval != 'l' ) break;
                if ( lseek ( fda, 0, 0 ) == -1 && attr_g.pathname[0] )
                    quit ( "lseek", 0 );
                else if ( ftruncate ( fda, 0 ) == -1 )
                    quit ( "ftruncate", 0 );
                if ( (fdl = open ( attr_g.pathname, O_RDWR|O_APPEND|O_CREAT, 0600 )) < 0 ) {
                    quit ( "overwrite: open", 0 );
                    break;
                }
                close ( fda );
                fda = fdl;
                i = output_rec ( arp->aud_rec - arp->aud_rec_b, arp->aud_rec_b, (off_t *)0 );
                arp->aud_rec -= i;
                audgen_l ( AUDIT_LOG_OVERWRITE, "overwriting audit log", attr_g.pathname );
                sprintf ( buf_l, "auditd: overwriting audit log %s", attr_g.pathname );
                cwrite ( buf_l );
                break;

    default:    /* bad overflow value */
                sprintf ( buf_l, "auditd: %c not a valid option for overflow", action );
                cwrite ( buf_l );
                break;

    }
    return(0);
}


/* CODE TAKEN FROM PING(8) */
/* return 0 only if remote host pinged */
int ping ( who )
char *who;
{
	struct sockaddr_in from;
	struct sockaddr whereto;/* Who to ping */
	struct sockaddr_in *to = (struct sockaddr_in *) &whereto;
	struct protoent *proto;
	struct hostent *hp;	/* Pointer to host info */
	int fromlen, size, timeout;
	union wait status;
	u_char	packet[MAXPACKET];
	int ident;
	int s;			/* Socket file descriptor */

	bzero( (char *)&whereto, sizeof(struct sockaddr) );

	to->sin_family = AF_INET;
	hp = gethostbyname(who);
	if (hp) {
		to->sin_family = hp->h_addrtype;
		bcopy(hp->h_addr, (caddr_t)&to->sin_addr, hp->h_length);
	} else return (-1);

	ident = getpid() & 0xFFFF;

	if ((proto = getprotobyname("icmp")) == NULL) return (-2);
	if ((s = socket(AF_INET, SOCK_RAW, proto->p_proto)) < 0) return (-3);

	timeout = attr_g.timeout;

        if ((pingpid = fork()) < 0) return (-4);
        if (pingpid != 0) {         /* parent */
                while (1) {
			if ( pinger(&whereto,ident,s) == -1) {
                                close(s);
                                return(-5);
                        }
                        sleep(1);
                        if (wait3(&status, WNOHANG, 0) == pingpid) {
                                close(s);
                                return(status.w_retcode);
                        }
                }
        }

        if (pingpid == 0) {         /* child */
                alarm(timeout);
                signal(SIGALRM, ping_noanswer);
                while (1) {
			int len = sizeof(packet);
                        fromlen = sizeof(from);
			if ((size = recvfrom(s, packet, len, 0, &from, &fromlen)) < 0)
                                continue;
			if ( ping_pr_pack(packet,size,&from,ident) == 1 )
                        	exit(0);
                }
        }

}

/*
 * 			P I N G E R
 * 
 * Compose and transmit an ICMP ECHO REQUEST packet.  The IP packet
 * will be added on by the kernel.  The ID field is our UNIX process ID,
 * and the sequence number is an ascending integer.  The first 8 bytes
 * of the data portion are used to hold a UNIX "timeval" struct in VAX
 * byte-order, to compute the round-trip time.
 */
pinger(whereto,ident,s)
struct sockaddr *whereto;
int ident;
int s;			/* Socket file descriptor */
{
	static u_char outpack[MAXPACKET];
	register struct icmp *icp = (struct icmp *) outpack;
	int i, cc;
	register struct timeval *tp = (struct timeval *) &outpack[8];
	register u_char *datap = &outpack[8+sizeof(struct timeval)];
	static int ntransmitted = 0; /* sequence # for outbound packets = #sent */
	struct timezone tz;
        int datalen = 64-8;

	icp->icmp_type = ICMP_ECHO;
	icp->icmp_code = 0;
	icp->icmp_cksum = 0;
	icp->icmp_seq = ntransmitted++;
	icp->icmp_id = ident;		/* ID */

	cc = datalen+8;			/* skips ICMP portion */

	gettimeofday( tp, &tz );

	for( i=8; i<datalen; i++)	/* skip 8 for time */
		*datap++ = i;

	/* Compute ICMP checksum here */
	icp->icmp_cksum = ping_in_cksum( icp, cc );

	/* cc = sendto(s, msg, len, flags, to, tolen) */
	i = sendto( s, outpack, cc, 0, whereto, sizeof(struct sockaddr) );

	if( i < 0 || i != cc )
		return(-1);
	else return(0);
}

/*
 *			P R _ P A C K
 *
 * Print out the packet, if it came from us.  This logic is necessary
 * because ALL readers of the ICMP socket get a copy of ALL ICMP packets
 * which arrive ('tis only fair).  This permits multiple copies of this
 * program to be run without having intermingled output (or statistics!).
 */
int ping_pr_pack( buf, cc, from, ident )
char *buf;
int cc;
struct sockaddr_in *from;
int ident;
{
	struct ip *ip;
	register struct icmp *icp;
	int hlen;

	from->sin_addr.s_addr = ntohl( from->sin_addr.s_addr );

	ip = (struct ip *) buf;
	hlen = ip->ip_hl << 2;
	if (cc < hlen + ICMP_MINLEN)
		return(0);
	cc -= hlen;
	icp = (struct icmp *)(buf + hlen);
	if( icp->icmp_id != ident )
		return(0);			/* 'Twas not our ECHO */

	return(1);
}

/*
 *			I N _ C K S U M
 *
 * Checksum routine for Internet Protocol family headers (C Version)
 *
 */
ping_in_cksum(addr, len)
u_short *addr;
int len;
{
	register int nleft = len;
	register u_short *w = addr;
	register u_short answer;
	register int sum = 0;
 	u_short odd_byte = 0;

	/*
	 *  Our algorithm is simple, using a 32 bit accumulator (sum),
	 *  we add sequential 16 bit words to it, and at the end, fold
	 *  back all the carry bits from the top 16 bits into the lower
	 *  16 bits.
	 */
	while( nleft > 1 )  {
		sum += *w++;
		nleft -= 2;
	}

	/* mop up an odd byte, if necessary */
	if( nleft == 1 ) {
               *(u_char *)(&odd_byte) = *(u_char *)w;
               sum += odd_byte;
	}

	/*
	 * add back carry outs from top 16 bits to low 16 bits
	 */
	sum = (sum >> 16) + (sum & 0xffff);	/* add hi 16 to low 16 */
	sum += (sum >> 16);			/* add carry */
	answer = ~sum;				/* truncate to 16 bits */
	return (answer);
}

int ping_noanswer()
{
        exit(-10);
}


/* print error message on console; exit          */
/* need to have audit file ptr to use audgen_l() */
quit ( string, severity )
char *string;
{
    extern int errno;
    extern int sys_nerr;
    extern char *sys_errlist[];
    char buf_l[BUF_SIZ];
    int i;

    /* output error message */
    if ( errno < sys_nerr ) sprintf ( buf_l, "auditd (%d): %s: %s", getpid(), string, sys_errlist[errno] );
    else sprintf ( buf_l, "auditd (%d): %s: error #%d", getpid(), string, errno );
    cwrite ( buf_l );

    /* if severe, turn off audit & /dev/audit overflow msgs, remove sockets, make audit log entry, exit */
    if ( severity ) {
        audcntl ( SET_AUDSWITCH, (char *)0, 0, 0, 0 );
        unlink ( ss.sa_data );
        bcopy ( "audX", buf_l, 5 );
        for ( i = 1; i <= N_CONN; i++ ) {
            buf_l[3] = i+'0';
            unlink ( buf_l );
        }

        /* kill child audit daemons */
        for ( i = 1; i <= N_CONN; i++ )
            if ( child[i] != -1 ) kill ( child[i], SIGALRM );

        audgen_l ( AUDIT_DAEMON_EXIT, "audit daemon exiting", "" );
        cwrite ( "auditd: AUDIT DAEMON EXITING ABNORMALLY" );
        exit ( severity );
    }
}


/* perform server function */
server()
{
    int sd1;                    /* server-client socket descriptor  */
    int sd2;                    /* /dev/audit descriptor            */
    int sd3 = -1;               /* server-inet socket descriptor    */
    char path_l[MAXPATHLEN];
    int i;

    /* turn off auditing for auditd */
    if ( audcntl ( SET_PROC_ACNTL, (char *)0, 0, AUDIT_OFF, 0 ) == -1 )
        quit ( "audcntl", 1 );

    /* initialize environment */
    umask ( 077 );
    for ( i = 1; i <= NSIG; i++ ) signal ( i, SIG_IGN );
    signal ( SIGIO, SIG_DFL );
    signal ( SIGTERM, sig_hndlr );
    signal ( SIGALRM, sig_hndlr );
    signal ( SIGHUP, sig_hndlr2 );
    for ( i = 0; i <= N_CONN; i++ ) child[i] = -1;
    mkdir ( AUDITD_DIR, 0700 );
    if ( chdir ( AUDITD_DIR ) ) quit ( "chdir", 1 );

    /* initialize parameters */
    if ( attr_g.freepct == -1 ) attr_g.freepct = DEFAULT_PCT;
    if ( attr_g.nkbytes < MIN_AUD_BUF ) attr_g.nkbytes = DEFAULT_AUD_BUF;
    if ( attr_g.overflow == '\0' ) attr_g.overflow = DEFAULT_ACTION;
    if ( attr_g.pathname[0] == '\0' ) for ( i = 0; attr_g.pathname[i] = DEFAULT_LOG[i]; i++ );
    if ( attr_g.pathval == '\0' ) attr_g.pathval = 'l';
    if ( attr_g.timeout == -1 ) attr_g.timeout = DEFTIMEOUT;
    attr_g.pid = 0;

    /* find first free file in sequence */
    path_l[0] = '\0';
    set_log_indx ( path_l );
    if ( attr_g.pathval == 'l' ) set_log_indx ( attr_g.pathname );
    else {
        if ( attr_g.audit2log[0] == '\0' )
            bcopy ( DEFAULT_LOG, attr_g.audit2log, sizeof DEFAULT_LOG );
        set_log_indx ( attr_g.audit2log );
    }

    if ( attr_g.console[0] == '\0' ) for ( i = 0; attr_g.console[i] = "/dev/console"[i]; i++ );
    if ( attr_g.query == 1 ) {
        for ( i = 0; attr_g.pathname[i]; i++ );
        write ( 1, attr_g.pathname, i );
        write ( 1, "\n", 1 );
    }
    if ( attr_g.show == 1 ) show_stat ( 1, -1 );
    if ( attr_g.help == 1 ) show_help ( 1 );

    /* run as daemon */
#ifndef DEBUG1
    if ( (i = fork()) == 0 ) {

        /* disassociate from controlling tty and process group */
        if ( setpgrp ( 0, getpid() ) == -1 ) quit ( "setpgrp", 0 );
        if ( (i = open ( "/dev/tty", 2 )) >= 0 ) {
            ioctl ( i, TIOCNOTTY, 0 );
            close(i);
        }
        for ( i = 0; i < _NFILE; i++ ) close(i);
#else
    if (1) {
#endif DEBUG1

        /* open audit log */
        if ( attr_g.pathval == 'l' )
            if ( (fda = open ( attr_g.pathname, O_RDWR|O_APPEND|O_CREAT, 0600 )) < 0 ) {
                quit ( "open", 0 );
                exit(1);
            }

        /* run daemon at a favorable priority */
        if ( setpriority ( PRIO_PROCESS, getpid(), PRIORITY ) == -1 )
            quit ( "set_priority", 0 );

        /* create socket for client; bind, listen */
        if ( (sd1 = socket ( AF_UNIX, SOCK_STREAM, 0 )) < 0 ) quit ( "socket", 1 );
        if ( bind (sd1, &ss, sizeof(struct sockaddr)) < 0 ) quit ( "bind", 1 );
        if ( listen ( sd1, 5 ) < 0 ) quit ( "listen", 1 );

        /* open /dev/audit for read access */
        if ( (sd2 = open ( "/dev/audit", 0 )) < 0 ) quit ( "open /dev/audit", 1 );

        /* open socket over which to receive connections from other daemons */
        if ( attr_g.net_server ) 
            if ( net_serv ( 1, &sd3 ) == -1 ) attr_g.net_server = 0;

        /* turns on system audit mechanism */
        if ( audcntl ( SET_AUDSWITCH, (char *)0, 0, 1, 0 ) == -1 )
            quit ( "audcntl", 1 );
        audgen_l ( AUDIT_START, "audit subsystem on", "" );
        cwrite ( "auditd: AUDIT DAEMON STARTING" );

        /* read audit data; monitor diskspace */
        chk_input ( sd1, sd2, &sd3 );

        /* kill child daemons */
        for ( i = 1; i <= N_CONN && attr_g.pid == 0; i++ )
            if ( child[i] != -1 ) kill ( child[i], SIGALRM );

        /* send "BYE" message to connected daemons */
        net_serv ( 0, &sd3 );
        if ( attr_g.pathval == 'i' ) {
            send ( inet_out, "BYE", MSG_OOB_SIZE, MSG_OOB );
            shutdown ( inet_out, 2 );
            close ( inet_out );
        }

        /* compress audit log */
        if ( attr_g.pathval == 'l' && sigtermed == 0 ) {
            close(fda);
            compress ( attr_g.pathname );
        }

        /* close old socket descriptors */
        close(sd1);
        close(sd2);
        close(sd3);
        bcopy ( "audX", path_l, 5 );
        path_l[3] = attr_g.pid+'0';
        unlink ( path_l );

        if ( attr_g.pid == 0 ) {
            /* remove socket from filesystem */
            unlink ( ss.sa_data );
            bcopy ( "audX", path_l, 5 );
            for ( i = 1; i <= N_CONN; i++ ) {
                path_l[3] = i+'0';
                unlink ( path_l );
            }
            rmdir ( AUDITD_DIR );

            /* turn off system audswitch */
            if ( audcntl ( SET_AUDSWITCH, (char *)0, 0, 0, 0 ) == -1 )
                quit ( "audcntl", 1 );
            cwrite ( "auditd: AUDIT DAEMON EXITING" );
        }
    }
    else if ( i == -1 ) quit ( "fork", 1 );
}


/* update log_indx and pathname to first unused member in sequence */
set_log_indx ( pathname )
char *pathname;
{
    struct stat statbuf;
    extern int errno;
    char path_l[MAXPATHLEN];
    int i;

#ifdef DEBUG3
    if ( attr_g.debug3 ) cwrite ( "debug: set_log_indx" );
#endif DEBUG3

    /* ignore non-regular files */
    if ( stat ( pathname, &statbuf ) == 0 )
        if ( (statbuf.st_mode & S_IFREG) == 0 ) return;

    for ( i = 0; pathname[i] && i < MAXPATHLEN; i++ );
    if ( i > 4 && pathname[i-4] == '.' && pathname[i-3] >= '0' && pathname[i-3] <= '9' )
        log_indx = atoi(&pathname[i-3]);
    else {
        pathname[i] = '.';
        for ( log_indx = 0; log_indx < MAX_LOG_INDX; log_indx++ ) {
            bcopy ( itoa(log_indx,3), &pathname[i+1], 3 );
            pathname[i+4] = '\0';
            if ( (stat ( pathname, &statbuf ) == -1) && (errno == ENOENT) ) {
                bcopy ( pathname, path_l, i+4 );
                bcopy ( ".Z", &path_l[i+4], 3 );
                if ( (stat ( path_l, &statbuf ) == -1) && (errno == ENOENT) )
                    break;
            }
        }
        if ( log_indx == MAX_LOG_INDX ) log_indx = 0;
    }
}


/* print help menu */
show_help ( ns )
int ns;
{
    char buf_l[BUF_SIZ];
    int i;

    write ( ns, "\n", 1 );
    sprintf ( buf_l, "-i:   host to receive audit data\n" );
    for ( i = 0; buf_l[i]; i++ );   write ( ns, buf_l, i );

    sprintf ( buf_l, "-l:   local file to receive audit data\n" );
    for ( i = 0; buf_l[i]; i++ );   write ( ns, buf_l, i );

    sprintf ( buf_l, "-c:   pathname (device or file) to receive auditd messages\n" );
    for ( i = 0; buf_l[i]; i++ );   write ( ns, buf_l, i );

    sprintf ( buf_l, "-n:   # kbytes in auditd buffer\n" );
    for ( i = 0; buf_l[i]; i++ );   write ( ns, buf_l, i );

#ifdef KERBEROS
    sprintf ( buf_l, "-a:   kerberos authentication (toggle)\n" );
    for ( i = 0; buf_l[i]; i++ );   write ( ns, buf_l, i );
#endif KERBEROS

    sprintf ( buf_l, "-s:   network audit server status (toggle)\n\n" );
    for ( i = 0; buf_l[i]; i++ );   write ( ns, buf_l, i );

    sprintf ( buf_l, "%s\n%s\n%s\n%s\n%s\n\n",
    "-d:   dump audit log buffer to file|dir|net",
    "-k:   kill audit daemon",
    "-p:   id # of daemon to receive command",
    "-q:   query server for audit log pathname",
    "-t:   timeout value for establishing remote connection",
    "-x:   increment log index; switch audit log" );
    for ( i = 0; buf_l[i]; i++ );   write ( ns, buf_l, i );

    sprintf ( buf_l, "-f:   min percent free space before an overflow condition is triggered\n" );
    for ( i = 0; buf_l[i]; i++ );   write ( ns, buf_l, i );

    sprintf ( buf_l, "-b:   local file to receive audit data on overflow condition\n" );
    for ( i = 0; buf_l[i]; i++ );   write ( ns, buf_l, i );

    sprintf ( buf_l, "-o:   action to take on overflow condition\n" );
    for ( i = 0; buf_l[i]; i++ );   write ( ns, buf_l, i );
    sprintf ( buf_l, "\
      'a'- use alternate audit log specified via -b\n\
      'b'- shutdown system\n\
      'c'- use mounted filesystem with most free space\n\
      'd'- suspend auditing until space is available\n\
      'e'- overwrite current audit log\n\n" );
    for ( i = 0; buf_l[i]; i++ );   write ( ns, buf_l, i );

    sprintf ( buf_l, "%s\n",
    "-?:   show status of audit daemon\n\
        (note: from shell, you must escape out ? character)" );
    for ( i = 0; buf_l[i]; i++ );   write ( ns, buf_l, i );
    sprintf ( buf_l, "-w:   show status of audit daemon\n" );
    for ( i = 0; buf_l[i]; i++ );   write ( ns, buf_l, i );
}


/* output daemon status onto client socket */
show_stat ( ns, sd3 )
int ns;     /* client socket descriptor  */
int sd3;    /* remote network connection */
{
    struct sockaddr_in peername;
    int namelen = sizeof(peername);
    struct audit_attr attr_l;
    struct sockaddr sl;
    char buf_l[BUF_SIZ];
    char *ptr;
    int i;

#ifdef DEBUG3
    if ( attr_g.debug3 ) cwrite ( "debug: show_stat" );
#endif DEBUG3

    for ( i = 1; attr_g.pid == 0 && i <= N_CONN; i++ )
        if ( child[i] != -1 ) {
            sprintf ( buf_l, "\nMASTER AUDIT DAEMON SERVER:\n" );
            for ( i = 0; buf_l[i]; i++ );   write ( ns, buf_l, i );
            break;
        }

    if ( attr_g.pid > 0 ) {
        write ( ns, "\n--------\n", 10 );
        if ( getpeername ( sd3, (struct sockaddr *)&peername, &namelen ) == 0 ) {
            ptr = gethost_l((long)peername.sin_addr.s_addr);
            sprintf ( buf_l, "AUDIT DAEMON #%d SERVING %s:\n", attr_g.pid, ptr );
        }
        else sprintf ( buf_l, "AUDIT DAEMON #%d:\n", attr_g.pid );
        for ( i = 0; buf_l[i]; i++ );   write ( ns, buf_l, i );
    }

#ifdef KERBEROS
    sprintf ( buf_l, "-a: kerberos authentication (toggle)                  = %s\n",
        attr_g.kerb_auth ? "on" : "off" );
    for ( i = 0; buf_l[i]; i++ );   write ( ns, buf_l, i );
#endif KERBEROS

    if ( attr_g.pid == 0 ) {
        sprintf ( buf_l, "-i: audit data stored remotely, destination host      = %s\n",
            attr_g.pathval == 'i' ? attr_g.pathname : "" );
        for ( i = 0; buf_l[i]; i++ );   write ( ns, buf_l, i );
    }

    sprintf ( buf_l, "-l: audit data stored locally, pathname               = %s\n",
        attr_g.pathval == 'l' ? attr_g.pathname : "" );
    for ( i = 0; buf_l[i]; i++ );   write ( ns, buf_l, i );

    sprintf ( buf_l, "-b: audit log alternate pathname                      = %s\n\n",
        attr_g.audit2log );
    for ( i = 0; buf_l[i]; i++ );   write ( ns, buf_l, i );

    sprintf ( buf_l, "-c: audit console                                     = %s\n",
         attr_g.console );
    for ( i = 0; buf_l[i]; i++ );   write ( ns, buf_l, i );

    sprintf ( buf_l, "-f: pct free space before overflow condition          = %d\n",
        attr_g.freepct );
    for ( i = 0; buf_l[i]; i++ );   write ( ns, buf_l, i );

    if ( nobuf == 0 && (attr_g.pid == 0 || attr_g.kerb_auth == 0) ) {
        sprintf ( buf_l, "-n: number of kbytes in buffer                        = %d\n",
            attr_g.nkbytes );
        for ( i = 0; buf_l[i]; i++ );   write ( ns, buf_l, i );
    }

    if ( attr_g.pid == 0 ) {
        sprintf ( buf_l, "-s: network audit server status (toggle)              = %s\n",
            attr_g.net_server ? "on" : "off" );
        for ( i = 0; buf_l[i]; i++ );   write ( ns, buf_l, i );
    }

    sprintf ( buf_l, "-t: connection timeout value (sec)                    = %d\n",
        attr_g.timeout );
    for ( i = 0; buf_l[i]; i++ );   write ( ns, buf_l, i );

    sprintf ( buf_l, "-o: action to take on overflow                        = " );
    for ( i = 0; buf_l[i]; i++ );   write ( ns, buf_l, i );
    switch ( attr_g.overflow ) {
    case 'a': sprintf ( buf_l, "use alternate audit log specified via -b\n" );
              break;
    case 'b': sprintf ( buf_l, "shutdown system\n" );
              break;
    case 'c': sprintf ( buf_l, "use mounted filesystem with most free space\n" );
              break;
    case 'd': sprintf ( buf_l, "suspend auditing until space is available\n" );
              break;
    case 'e': sprintf ( buf_l, "overwrite current audit log\n" );
              break;
    case 'f': sprintf ( buf_l, "switch current and alternate auditlogs\n" );
              break;
    default:  sprintf ( buf_l, "(none)\n" );
    }
    for ( i = 0; buf_l[i]; i++ );   write ( ns, buf_l, i );

    /* pass request to all child auditd's */
    for ( i = 1; attr_g.pid == 0 && i <= N_CONN; i++ )
        if ( child[i] != -1 ) {
            sl.sa_family = AF_UNIX;
            bcopy ( "audX", sl.sa_data, 5 );
            sl.sa_data[3] = i+'0';
            init_attr ( &attr_l );
            attr_l.show = 1;
            client ( 1, &attr_l, &sl, ns );
        }
}


/* kill daemon gracefully */
sig_hndlr ( sig )
int sig;
{
    attr_g.kill = 1;
    if ( attr_g.pid == 0 ) {
        if ( audcntl ( SET_AUDSWITCH, (char *)0, 0, 0, 0 ) == -1 )
            quit ( "audcntl", 1 );
    }
    if ( sig == SIGTERM ) sigtermed = 1;
}

/* caught HUP; restart chk_input loop */
sig_hndlr2()
{
    longjmp ( env, 1 );
}


/* fork auditd server to handle client auditd */
spawn_child ( conn, sd1_p, sd2_p, sd3_p, arp_p, conn_indx )
int *conn;                                  /* accepted connection  */
int *sd1_p;                                 /* local client socket  */
int *sd2_p;                                 /* kernel descriptor    */
int *sd3_p;                                 /* remote client socket */
struct arp *arp_p;                          /* audit record ptr's   */
int conn_indx;                              /* child-connect index  */
{
    struct sockaddr_in peername;            /* remote client name   */
    int namelen = sizeof(peername);
    int newpid;                             /* child daemon pid     */
    struct sockaddr sl;                     /* for parent msgs      */
    struct stat sbuf;
    char *ptr;
    int i;

#ifdef DEBUG3
    if ( attr_g.debug3 ) cwrite ( "debug: spawn_child" );
#endif DEBUG3

    newpid = fork();

    /* child daemon */
#ifndef DEBUG2
    if ( newpid == 0 ) {
#else
    if ( newpid ) {
#endif DEBUG2

        /* build attributes */
        close ( *sd1_p );
        close ( *sd2_p );
        *sd1_p = *sd2_p = -1;
        *sd3_p = *conn;
        attr_g.audit2log[0] = '\0';
        attr_g.net_server = 0;
        attr_g.pid = conn_indx;

        /* make sure able to read complete KERBEROS packet */
        if ( attr_g.kerb_auth && attr_g.nkbytes < KRB_RECVSIZ ) {
            if ( (arp_p->aud_rec = (char *)sbrk(KRB_RECVSIZ*1024)) == (char *)-1 ) {
                cwrite ( "-- insufficient mem --" );
                exit(-1);
            }
            else {
                arp_p->aud_rec_b = arp_p->aud_rec;
                attr_g.nkbytes = KRB_RECVSIZ;
            }
        }
        arp_p->aud_rec = arp_p->aud_rec_b;

        /* build pathname */
        if ( stat ( attr_g.pathname, &sbuf ) == 0 )
            if ( (sbuf.st_mode & S_IFREG) == 0 ) {
                strcpy ( attr_g.pathname, DEFAULT_LOG_REM );
                for ( i = 0; attr_g.pathname[i] && i < MAXPATHLEN; i++ );
                strcpy ( &attr_g.pathname[i], ".000" );
            }
        for ( i = 0; attr_g.pathname[i] && i < MAXPATHLEN; i++ );
        attr_g.pathname[i-4] = ':';
        strcpy ( &attr_g.pathname[i-3], "remote" );
        if ( getpeername ( *sd3_p, (struct sockaddr *)&peername, &namelen ) < 0 )
            quit ( "getpeername", 0 );
        else if ( ptr = gethost_l((long)peername.sin_addr.s_addr) )
            strcpy ( &attr_g.pathname[i-3], ptr );
        else strcpy ( &attr_g.pathname[i-3], inet_ntoa(peername.sin_addr.s_addr) );
        for ( i = 0; attr_g.pathname[i] && i < MAXPATHLEN; i++ );
        attr_g.pathname[i] = '\0';
        set_log_indx ( attr_g.pathname );
        attr_g.pathval = 'l';
        close(fda);
        if ( (fda = open ( attr_g.pathname, O_RDWR|O_APPEND|O_CREAT, 0600 )) < 0 ) {
            quit ( "open", 0 );
            return;
        }

        /* establish socket for parent */
        sl.sa_family = AF_UNIX;
        bcopy ( "audX", sl.sa_data, 5 );
        sl.sa_data[3] = conn_indx+'0';
        if ( (*sd1_p = socket ( AF_UNIX, SOCK_STREAM, 0 )) < 0 ) quit ( "socket", 0 );
        if ( bind (*sd1_p, &sl, sizeof(struct sockaddr)) < 0 ) quit ( "bind", 0 );
        if ( listen ( *sd1_p, 1 ) < 0 ) quit ( "listen", 0 );
    }

    /* parent: close up network connection */
#ifndef DEBUG2
    if ( newpid > 0 ) {
#else
    if ( newpid == 0 ) {
#endif DEBUG2
        child[conn_indx] = newpid;
        close ( *conn );
    }

    /* fork error condition */
    if ( newpid == -1 ) {
        send ( *conn, "BYE", MSG_OOB_SIZE, MSG_OOB );
        shutdown ( *conn, 2 );
        close ( *conn );
        cwrite ( "auditd: unable to create child auditd for new connection" );
    }
}


/* switch audit log to "log_new" */
int switch_log ( log_new, severity )
char *log_new;
int severity;
{
    int fdl;                        /* tmp descr      */
    char buf_l[BUF_SIZ];            /* local buffer   */
    char log_new_l[MAXPATHLEN];     /* local log name */
    struct stat sbuf;
    int dvc = 0;                    /* 1 for devices  */
    int i;

#ifdef DEBUG3
    if ( attr_g.debug3 ) cwrite ( "debug: switch_log" );
#endif DEBUG3

    /* check new log name */
    if ( *log_new == '\0' ) {
        cwrite ( "auditd: no alternate audit log set" );
        return(-1);
    }

    /* check for devices (non-regular files) */
    if ( stat ( log_new, &sbuf ) == 0 )
        if ( (sbuf.st_mode & S_IFREG) == 0 ) {
            for ( i = 0; (log_new_l[i] = log_new[i]) && (i < MAXPATHLEN); i++ );
            dvc = 1;
            log_indx = -1;
        }

    /* append log_indx to log name; allow override if .xxx specified */
    for ( i = 0; (log_new_l[i] = log_new[i]) && (i < MAXPATHLEN-4); i++ );
    if ( dvc == 0 ) {
        if ( (i >= 4) && (log_new[i-4] == '.') ) {
            if ( strncmp ( log_new, attr_g.pathname, i ) == 0 ) return(0);
            log_indx = atoi(&log_new[i-3]);
            bcopy ( itoa(log_indx,3), &log_new_l[i-3], 3 );
            log_new_l[i] = '\0';
        }
        else {
            if ( log_indx == -1 ) set_log_indx ( log_new_l );
            else {
                log_new_l[i++] = '.';
                log_indx = (log_indx+1)%MAX_LOG_INDX;
                bcopy ( itoa(log_indx,3), (char *)&log_new_l[i], 3 );
                log_new_l[i+3] = '\0';
            }
        }
    }

    /* open new log */
    if ( (fdl = open ( log_new_l, O_RDWR|O_APPEND|O_CREAT, 0600 )) < 0 ) {
        sprintf ( buf_l, "auditd: could not set audit log to %s", log_new_l );
        cwrite ( buf_l );
        return(-1);
    }

    /* put log change msg in old log, unless problem forced the switch */
    if ( severity == 0 )
        audgen_l ( AUDIT_LOG_CHANGE, "audit log change to file", log_new_l );

    /* switch logs */
    if ( attr_g.pathval == 'l' ) {
        close ( fda );
        compress ( attr_g.pathname );
    }
    for ( i = 0; (attr_g.pathname[i] = log_new_l[i]) && (i < MAXPATHLEN); i++ );
    if ( attr_g.pathval == 'i' ) {
        send ( inet_out, "BYE", MSG_OOB_SIZE, MSG_OOB );
        shutdown ( inet_out, 2 );
        close ( inet_out );
        inet_out = -1;
    }
    fda = fdl;
    sprintf ( buf_l, "auditd: setting audit log to %s", attr_g.pathname );
    cwrite ( buf_l );
    attr_g.pathval = 'l';
    return(0);
}
