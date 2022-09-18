#ifndef lint
static char *sccsid = "@(#)audit_tool.c	4.4	ULTRIX	4/11/91";
#endif lint

/************************************************************************
 *									*
 *                      Copyright (c) 1989, 1990, 1991 by               *
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
 *   09 Aug 90 - scott
 *      change prompt
 *      add -w option
 *      improve syscall integer param formatting
*/

/* Audit Reduction Tool
    link with /sys/`machine`/BINARY/syscalls.o -laud
    optional: -Ddebug1 to get block maps and proc lists
    optional: -DPRIV to include priv information
    optional: -DPRIVDB -lprivdb to use privdb library
*/

/* to handle new token in audit.h:
    1) update audit_fields struct
    2) update init_audit_fields()
    3) update appropriate output_*() routine
    4) update parse_rec()
*/

#include <sys/types.h>
#include <sys/time.h>
#include <sys/file.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/audit.h>
#include <sys/cpudata.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <strings.h>
#include <syscall.h>
#include <stdio.h>
#include <errno.h>
#ifdef mips
#include <varargs.h>
#endif mips
#include <pwd.h>
#include <grp.h>
#include <sys/mount.h>
#include <fs/nfs/nfs_clnt.h>
#include <fs/nfs/vfs.h>

#ifdef PRIV
typedef u_long priv_mask_t;
typedef struct 	priv_set_t { priv_mask_t mask[2]; } priv_set_t;
#endif PRIV

#define HOST_LEN        32
#define MAX_RULE_SIZ    4096
#define MAX_SPECIAL     263
#define MIN_SPECIAL     260
#define NO_F            0x01
#define NO_S            0x02
#define NUM_SYSCALLS    257
                    /* based on # entries in syscalls.c */
#define NRULESETS       32
#define N_SELECT        (NUM_SYSCALLS+N_TRUSTED_EVENTS)
#define N_SELECT2       8
#define RULE(i,x)       rules[i/NRULESETS]->x[i%NRULESETS]
#define RULES_IN_SET    32
#define STR_LEN         1024
#define STR_LEN2        32
#define TIME_LEN        13

#define MEM_ELMNT       32
#define MEM_NELMNT      1000
#define MEM_NBLKS       100

#define FLAG_BINARY     0x01
#define FLAG_DISPLAY    0x02
#define FLAG_FOLLOW     0x04
#define FLAG_OVERRIDE   0x08
#define FLAG_REPORT     0x10
#define FLAG_BRIEF      0x20
#define FLAG_SORT       0x40
#define FLAG_LOCALID    0x80

char *aud_mem_op();
struct a_proc *aud_mem_proc();
char *fetch_matching_rec();
char *fetch_rec();
char *gethost_l();
char *itoa();
int sig_int1();

/* special events */
char *special_event[] = {
    "shmget",
    "shmdt",
    "shmctl",
    "shmat",
    "logout"
};
#define SYS_SHMGET MIN_SPECIAL
#define SYS_SHMDT  MIN_SPECIAL+1
#define SYS_SHMCTL MIN_SPECIAL+2
#define SYS_SHMAT  MIN_SPECIAL+3
#define _LOGOUT special_event[4]

/* audit record fields */
struct audit_fields {
    long auid;
    short device;
    int error;
    int event;
    short flag;
    int hostid;
    unsigned long ipaddr;
    short n_cpu;
    short pid;
    short ppid;
    int result;
    struct timeval timeval;
    struct timezone timezone;
    short uid;
    short ruid;
#ifdef PRIV
    priv_set_t privstr;
#endif PRIV

    long auid2;
    int hostid2;
    unsigned long ipaddr2;
    short device2;
    short pid2;
    short ppid2;
    short uid2;
    short ruid2;
    short login_proc;

    char *charparam[AUD_NPARAM];        int charp_indx;      int charlen[AUD_NPARAM];
    int intparam[AUD_NPARAM];           int intp_indx;
    short shortparam[AUD_NPARAM];       int shortp_indx;
    char *int_array[AUD_NPARAM];        int int_array_indx;  int int_array_len[AUD_NPARAM];   

    int descrip[AUD_NPARAM];            int descrip_indx;
    long gnode_id[AUD_NPARAM];          int gp_id_indx;
    ushort gnode_dev[AUD_NPARAM];       int gp_dev_indx;

    char *socketaddr[AUD_NPARAM];       int socket_indx;     int socketlen[AUD_NPARAM];
    char *msgaddr[AUD_NPARAM];          int msg_indx;        int msglen[AUD_NPARAM];
    char *accessaddr[AUD_NPARAM];       int access_indx;     int accesslen[AUD_NPARAM];
    ushort ipc_uid[AUD_NPARAM];         int ipc_uid_indx;
    ushort ipc_gid[AUD_NPARAM];         int ipc_gid_indx;
    ushort ipc_mode[AUD_NPARAM];        int ipc_mode_indx;

    char *login[AUD_NPARAM];            int login_indx;      int login_len[AUD_NPARAM];
    char *login2[AUD_NPARAM];           int login2_indx;     int login2_len[AUD_NPARAM];
    char *homedir[AUD_NPARAM];          int homedir_indx;    int homedir_len[AUD_NPARAM];
    char *shell[AUD_NPARAM];            int shell_indx;      int shell_len[AUD_NPARAM];
    char *service[AUD_NPARAM];          int service_indx;    int service_len[AUD_NPARAM];
    char *devname[AUD_NPARAM];          int devname_indx;    int devname_len[AUD_NPARAM];

    int atom_id[AUD_NPARAM];            int atom_id_indx;
    struct aud_client_info x_client[AUD_NPARAM];  int x_client_indx;
    int client_id[AUD_NPARAM];          int client_id_indx;
    int property[AUD_NPARAM];           int property_indx;
    unsigned int res_class[AUD_NPARAM]; int res_class_indx;
    unsigned int res_type[AUD_NPARAM];  int res_type_indx;
    int res_id[AUD_NPARAM];             int res_id_indx;
};

/* audit record fields on which post-selection enabled */
struct selectn {
    long auid[N_SELECT2];               int auid_indx;
    char charparam[N_SELECT2][STR_LEN]; int charparam_indx;
    short dev[N_SELECT2];               int dev_indx;
    int error[N_SELECT2];               int error_indx;
    char event[N_SELECT][STR_LEN];      int event_indx;
    char event_status[N_SELECT];
    long gnode[N_SELECT2];              int gnode_indx;
    int gnode_dev[N_SELECT2];           int gnode_dev_indx;
    unsigned long ipaddr[N_SELECT2];    int ipaddr_indx;
    char logfile[MAXPATHLEN];
    short n_cpu;
    short pid[N_SELECT2];               int pid_indx;
    short ppid[N_SELECT2];              int ppid_indx;
    short ruid[N_SELECT2];              int ruid_indx;
    char rulesfil[STR_LEN];
    char time_end[TIME_LEN];
    char time_start[TIME_LEN];
    short uid[N_SELECT2];               int uid_indx;
    char username[N_SELECT2][STR_LEN];  int username_indx;
#ifdef PRIV
    int priv;
#endif PRIV
} selectn;

int flag = 0;               /* see FLAG_* def's above   */
char close_buf[MAXPATHLEN]; /* last closed file         */
int sort_flag = 0;          /* sort status of data file */

/* deselection ruleset, and ruleset ptrs */
struct ruleset {
    char *host[RULES_IN_SET];
    long auid[RULES_IN_SET];
    short ruid[RULES_IN_SET];
    char *event[RULES_IN_SET];
    char *param[RULES_IN_SET];
    int  oprtn[RULES_IN_SET];
} ruleset;
struct ruleset *rules[NRULESETS];
int ruleno = 0;             /* # rules in ruleset file  */

/* process state information */
struct a_proc {
    ushort access_gp_dev[_NFILE];
    long access_gp_id[_NFILE];
    int access_gp_indx;
    long auid;
    unsigned long ipaddr;
    short pid;
    short ruid;
    short login_proc;

    char *cwd;
    char *root;
    char *username;
    char *fd_nm[_NFILE];
    struct a_proc *a_proc_next;
    struct a_proc *a_proc_prev;
};
#define A_PROC_HDR_SIZ (sizeof(short)*(2+_NFILE) + sizeof(int) + sizeof(long)*(2+_NFILE))

#define ALIGN(to,from,type) \
    to = (struct type *)((int)from + (4-((int)from & 0x03))%4);


/* reduction tool */
main ( argc, argv )
int argc;
char *argv[];
{
    int interactive = 0;    /* interactive initialization mode        */
    struct hostent *hp;     /* hostentry pointer                      */
    int i, j;

    if ( argc < 2 ) {
        printf ( "Audit reduction tool usage: [options] logfile\n" );
        printf ( "\nselection options:\n" );
        printf ( "  -a audit_id                      -e event[:succeed:fail]\n" );
        printf ( "  -E error# or error_string        -g gnode_id\n" );
        printf ( "  -G gnode's device-major#,minor#  -h hostname or ip_address\n" );
        printf ( "  -p pid                           -P ppid\n" );
        printf ( "  -r real_uid                      -s string_parameter\n" );
        printf ( "  -t start_time                    -T end_time     format: yymmdd[hh[mm[ss]]]\n" );
        printf ( "  -u uid                           -U username\n" );
        printf ( "  -x device-major#,minor#\n" );
#ifdef PRIV
        printf ( "  -V:         select records with privilege used\n" );
#endif PRIV

        printf ( "\ncontrol options:\n" );
        printf ( "  -b:         output in binary format\n" );
        printf ( "  -B:         output in abbreviated format\n" );
        printf ( "  -d file:    use specified deselection rules file (-D to print ruleset)\n" );
        printf ( "  -f:         keep reading auditlog (like tail -f)\n" );
        printf ( "  -i:         interactive selection mode\n" );
        printf ( "  -o:         override switching logfile due to change_auditlog records\n" );
        printf ( "  -R:         generate reports by audit_id\n" );
        printf ( "  -S:         sort audit records by time (for SMP only)\n" );
        printf ( "  -w:         map ruid, group #'s to names using passwd, group tables\n" );
        exit(0);
    }

    /* sig hndlr to trigger interact() on ^C */
    signal ( SIGINT, sig_int1 );

    /* initializations for post-selection */
    init_selectn ( &selectn );

    /* process command line */
    for ( i = 1; i < argc; i++ ) {

        /* select on audit record fields */
        if ( argv[i][0] == '-' ) switch ( argv[i][1] ) {
            case 'a':   if ( selectn.auid_indx < N_SELECT2 && ++i < argc )
                            selectn.auid[selectn.auid_indx++] = atoi(argv[i]);
                                                                        break;
            case 'B':   flag |= FLAG_BRIEF;                             break;
            case 'b':   flag |= FLAG_BINARY;                            break;

            case 'D':   flag |= FLAG_DISPLAY;       /* fall through to 'd' */
            case 'd':   if ( ++i < argc ) strncpy (selectn.rulesfil, argv[i], STR_LEN);
                                                                        break;

            case 'e':   if ( selectn.event_indx < N_SELECT && ++i < argc ) {
                            for ( j = 0; argv[i][j] && argv[i][j] != ':' && j < STR_LEN2; j++ );
                            bcopy ( argv[i], selectn.event[selectn.event_indx], j );
                            selectn.event_status[selectn.event_indx] = 0x0;
                            if ( argv[i][j] == ':' && argv[i][j+1] == '0' )
                                selectn.event_status[selectn.event_indx] += NO_S;
                            if ( argv[i][j] && argv[i][j+2] == ':' && argv[i][j+3] == '0' )
                                selectn.event_status[selectn.event_indx] += NO_F;
                            selectn.event_indx++;
                        }                                               break;

            case 'E':   if ( selectn.error_indx < N_SELECT2 && ++i < argc )
                            selectn.error[selectn.error_indx++] = atoi(argv[i]);
                                                                        break;
            case 'f':   flag |= FLAG_FOLLOW;                            break;

            case 'g':   if ( selectn.gnode_indx < N_SELECT2 && ++i < argc )
                            selectn.gnode[selectn.gnode_indx++] = atoi(argv[i]);
                                                                        break;
            case 'G':   if ( selectn.gnode_dev_indx < N_SELECT2 && ++i < argc ) {
                            for ( j = 0; argv[i][j] && argv[i][j] != ','; j++ );
                            if ( argv[i][j] == ',' )
                                selectn.gnode_dev[selectn.gnode_dev_indx++] =
                                makedev ( atoi(argv[i]), atoi(argv[i]+j+1) );
                        }
                                                                        break;
            case 'h':   if ( selectn.ipaddr_indx < N_SELECT2 && ++i < argc ) {
                            if ( hp = gethostbyname(argv[i]) )
                                bcopy ( hp->h_addr, &selectn.ipaddr[selectn.ipaddr_indx++], hp->h_length );
                            else if ( (j = inet_addr(argv[i])) != -1 )
                                selectn.ipaddr[selectn.ipaddr_indx++] = j;
                            else write ( 1, "bad host/address\n", 17 );
                        }                                               break;

            case 'i':   interactive = 1;                                break;
            case 'I':   if ( selectn.ipaddr_indx < N_SELECT2 && ++i < argc )
                            selectn.ipaddr[selectn.ipaddr_indx] = inet_addr(argv[i]);
                                                                        break;
            case 'o':   flag |= FLAG_OVERRIDE;                          break;

            case 'p':   if ( selectn.pid_indx < N_SELECT2 && ++i < argc )
                            selectn.pid[selectn.pid_indx++] = atoi(argv[i]);
                                                                        break;
            case 'P':   if ( selectn.ppid_indx < N_SELECT2 && ++i < argc )
                            selectn.ppid[selectn.ppid_indx++] = atoi(argv[i]);
                                                                        break;
            case 'r':   if ( selectn.ruid_indx < N_SELECT2 && ++i < argc )
                            selectn.ruid[selectn.ruid_indx++] = atoi(argv[i]);
                                                                        break;

            case 'R':   flag |= FLAG_REPORT;                            break;
            case 's':   if ( selectn.charparam_indx < N_SELECT2 && ++i < argc )
                            strncpy (selectn.charparam[selectn.charparam_indx++], argv[i], STR_LEN);
                                                                        break;
            case 'S':   flag |= FLAG_SORT;                              break;

            case 't':   if ( ++i < argc ) strncpy (selectn.time_start, argv[i], TIME_LEN);
                                                                        break;
            case 'T':   if ( ++i < argc ) strncpy (selectn.time_end, argv[i], TIME_LEN);
                                                                        break;

            case 'u':   if ( selectn.uid_indx < N_SELECT2 && ++i < argc )
                            selectn.uid[selectn.uid_indx++] = atoi(argv[i]);
                                                                        break;
            case 'U':   if ( selectn.username_indx < N_SELECT2 && ++i < argc )
                            strncpy (selectn.username[selectn.username_indx++], argv[i], STR_LEN);
                                                                        break;
#ifdef PRIV
            case 'v':   selectn.priv = 1;                               break;
#endif PRIV
            case 'w':   flag |= FLAG_LOCALID;                           break;
            case 'x':   if ( selectn.dev_indx < N_SELECT2 && ++i < argc ) {
                            for ( j = 0; argv[i][j] && argv[i][j] != ','; j++ );
                            if ( argv[i][j] == ',' )
                                selectn.dev[selectn.dev_indx++] =
                                makedev ( atoi(argv[i]), atoi(argv[i]+j+1) );
                        }
                                                                        break;
            default:    fprintf ( stderr, "audit_tool: unknown option: %c ignored\n", argv[i][1] );
                                                                        break;
        }

        /* set initial log file */
        else if ( argv[i][0] != '-' )
            strncpy ( selectn.logfile, argv[i], MAXPATHLEN );

    }

    /* check for log file on command line */
    if ( selectn.logfile[0] == '\0' ) {
        printf ( "Usage: %s [ option ... ] auditlog_file\n", argv[0] );
        exit(1);
    }

    /* interactive mode */
    if ( interactive ) interact ( &selectn, &flag );

    /* build deselection ruleset */
    if ( *(selectn.rulesfil) ) ruleno = build_ruleset ( selectn.rulesfil, flag&FLAG_DISPLAY );
    fflush ( stdout );

    /* process audit log */
    audit_reduce ( &selectn, &flag );
    exit(0);
}


/* fetch and output audit records */
audit_reduce ( selectn, flag_p )
struct selectn *selectn;    /* selection criteria   */
int *flag_p;                /* misc options         */
{
    struct audit_fields audit_fields;
    char *rec_ptr;      /* ptr to audit data    */
    int rec_len;        /* length of audit rec  */
    static char *output_file = "report.xxxxxxxxxxxx";
    char buf_ptr[AUD_BUF_SIZ*2];
    int cnt_p = 0;
    int cnt = 0;
    int fd_i = 0;
    int fd_o = 1;
    int i = 0;

    for ( cnt = 0; (rec_ptr = fetch_matching_rec ( &audit_fields, selectn,
    &cnt_p, *flag_p, &rec_len, &fd_i )) != (char *)-1; cnt++ ) {
        fflush ( stdout );  fflush ( stderr );

        /* set file descriptor to tty or output_file */
        if ( *flag_p & FLAG_REPORT ) {
            strcpy ( &output_file[7], itoa(audit_fields.auid) );
            fd_o = open ( output_file, O_CREAT|O_RDWR|O_APPEND, 0600 );
            if ( fd_o == -1 ) fd_o = 1;
        }

        if ( (*flag_p&FLAG_BINARY) == 0 ) {
            i = output_rec_fmt ( buf_ptr, audit_fields, *flag_p );
            write ( fd_o, buf_ptr, i );
        }
        else write ( fd_o, rec_ptr, rec_len );
        close_buf[0] = '\0';

        if ( *flag_p&FLAG_REPORT ) close ( fd_o );
        fflush ( stdout );  fflush ( stderr );
        if ( cnt_p%1000 == 0 ) fprintf ( stderr, "(%d records processed...)\n\n", cnt_p );
    }

    if ( (*flag_p&(FLAG_BINARY|FLAG_BRIEF)) == 0 ) {
        printf ( "%d records output\n", cnt );
        printf ( "%d records processed\n", cnt_p );
    }
}


/* sort audit records by time - for SMP */
audit_sort ( logfile )
char *logfile;
{
    struct audit_fields af;     /* fields of parsed record      */
    struct selectn selectn;     /* selection criteria           */
    struct {
        int pos;
        struct timeval tv;
    } sort[MAXCPU];             /* per-cpu time, posn           */
    char *rec_ptr;              /* ptr to audit data            */
    int rec_len;                /* length of record             */

    char sortfile[MAXPATHLEN];  /* tmp file to hold sorted data */
    struct stat logstat;        /* stat struct for logfile      */
    struct stat sortstat;       /* stat struct for sortfile     */
    int cnt = 0;                /* # records processed          */
    int opos = 0;               /* posn in input file           */
    int fd = 0;                 /* input file descriptor        */
    int fd_o;                   /* output file descriptor       */
    int i, j;
#define ABS(x) (x > 0 ? x : -x)

    init_selectn ( &selectn );
    strncpy ( selectn.logfile, logfile, MAXPATHLEN );
    for ( i = 0; i < MAXCPU; i++ ) sort[i].pos = -1;

    /* check if logfile previously sorted */
    if ( fetch_hdr ( logfile, selectn.time_start, 1 ) ) {
        printf ( "%s already sorted.\n", logfile );
        return;
    }

    /* pass 1 - find first record per cpu */
    printf ( "sorting %s... (pass 1)\n", logfile );
    for ( ;; ) {
        if ( fetch_matching_rec ( &af, &selectn, &cnt, FLAG_OVERRIDE,
        &rec_len, &fd ) == (char *)-1 ) break;
        if ( sort[af.n_cpu].pos == -1 ) {
            opos = tell(fd);
            sort[af.n_cpu].pos = opos-rec_len;
            sort[af.n_cpu].tv = af.timeval;
        }
        if ( cnt%1000 == 0 ) printf ( "(pass 1: %d records processed...)\n", cnt );
    }
    printf ( "pass 1 complete: %d records sorted\n", cnt );

    /* open sortfile */
    for ( i = 0; (sortfile[i] = logfile[i]) && i < MAXPATHLEN-5; i++ );
    for ( j = 0; (sortfile[i] = ".sort"[j]) && i < MAXPATHLEN; i++, j++ );
    if ( (fd_o = open ( sortfile, O_RDWR|O_CREAT|O_TRUNC, 0600 )) < 0 ) {
        printf ( "failed to open %s\n", sortfile );
        return;
    }

    /* pass 2 - build sorted logfile */
    printf ( "sorting... (pass 2)\n" );
    for ( cnt = 0;; cnt++ ) {
        for ( i = 0, j = -1; i < MAXCPU; i++ ) {
            if ( sort[i].pos != -1 ) {
                if ( j == -1 ) j = i;
                else if ( ABS(sort[i].tv.tv_sec) < ABS(sort[j].tv.tv_sec) )
                    j = i;
                else if ( ( ABS(sort[i].tv.tv_sec) == ABS(sort[j].tv.tv_sec) )
                && ( ABS(sort[i].tv.tv_usec) < ABS(sort[j].tv.tv_usec) ) )
                    j = i;
            }
        }
        if ( j == -1 ) break;

        if ( lseek ( fd, sort[j].pos, L_SET ) == -1 ) perror ( "lseek" );
        rec_ptr = fetch_rec ( &fd, &rec_len, &af, 0, 0 );
        write ( fd_o, rec_ptr, rec_len );

        parse_rec ( rec_ptr, rec_len, &af );
        selectn.n_cpu = af.n_cpu;
        if ( fetch_matching_rec ( &af, &selectn, &j, FLAG_OVERRIDE,
        &rec_len, &fd ) != (char *)-1 ) {
            opos = tell(fd);
            sort[af.n_cpu].pos = opos-rec_len;
            sort[af.n_cpu].tv = af.timeval;
        }
        else sort[selectn.n_cpu].pos = -1;
        if ( cnt && (cnt%1000 == 0) ) printf ( "(pass 2: %d records processed...)\n", cnt );
    }
    printf ( "pass 2 complete: %d records sorted\n", cnt );

    /* check filesizes, rename sortfile, update hdr file */
    stat ( logfile, &logstat );
    stat ( sortfile, &sortstat );
    if ( logstat.st_size != sortstat.st_size )
        printf ( "sort failed; %s and %s not same size\n", logfile, sortfile );
    else if ( rename ( sortfile, logfile ) == -1 )
        perror ( "rename from sortfile to logfile" );
    else {
        fetch_hdr ( logfile, selectn.time_start, 2 );
        sort_flag = 1;
    }
}


/* get/free/provide memory for reduction state processing */
char *aud_mem_op ( fetch_siz, free_ptr, free_siz, debug )
int fetch_siz;      /* # bytes requested           */
char *free_ptr;     /* ptr to mem to be free'd     */
int free_siz;       /* # bytes of mem to be free'd */
int debug;          /* show memory block map       */
{
    static struct block *blk_ptr[MEM_NBLKS];
    static int blk_ptr_used = -1;
    struct block {
        char blk_map[MEM_NELMNT];
        char blk_mem[MEM_ELMNT*MEM_NELMNT];
    };
    char *cp;
    int i, j, k;

    /* fetch memory */
    if ( fetch_siz ) {
        fetch_siz = (fetch_siz-1)/MEM_ELMNT + 1;

        /* check each blk_ptr */
        for ( i = 0; i < MEM_NBLKS; i++ ) {
            if ( i > blk_ptr_used ) {
                if ( (cp = (char *)sbrk ( sizeof(int)+sizeof(struct block) )) == (char *)-1 ) 
                    return((char *)0);
                ALIGN ( blk_ptr[i], cp, block );
                blk_ptr[i] = (struct block *)((int)cp + (sizeof(int)-((int)cp & 0x03))%sizeof(int));
                blk_ptr_used++;
                for ( j = 0; j < MEM_NELMNT; j++ )
                    blk_ptr[i]->blk_map[j] = '0';
            }

            /* check blk_map for fetch_siz contiguous entries */
            for ( j = 0; j <= MEM_NELMNT-fetch_siz; j++ ) {
                for ( k = 0; k < fetch_siz; k++ )
                    if ( blk_ptr[i]->blk_map[j+k] != '0' ) break;
                if ( k < fetch_siz ) continue;
                for ( k = 0; k < fetch_siz; k++ )
                    blk_ptr[i]->blk_map[j+k] = '1';
                return ( &blk_ptr[i]->blk_mem[j*MEM_ELMNT] );
            }
        }

        return((char *)0);
    }

    /* free memory */
    if ( free_siz && free_ptr ) {
        for ( i = 0; i <= blk_ptr_used; i++ ) {
            if ( free_ptr >= blk_ptr[i]->blk_mem && 
            free_ptr+free_siz <= &blk_ptr[i]->blk_mem[MEM_ELMNT*MEM_NELMNT-1] ) {
                k = (free_ptr - blk_ptr[i]->blk_mem) / MEM_ELMNT;
                for ( j = 0; j <= (free_siz-1)/MEM_ELMNT; j++ )
                    blk_ptr[i]->blk_map[j+k] = '0';
                return((char *)0);
            }
        }
        return ((char *)0);
    }

    /* debug: draw blk_map's */
    if ( debug ) {
        for ( i = 0; i <= blk_ptr_used; i++ ) {
            fprintf ( stderr, "block %.03d:  ", i );
            for ( j = 0; j < MEM_NELMNT; j++ ) {
                fprintf ( stderr, "%c", blk_ptr[i]->blk_map[j] );
                if ( (j+1)%10 == 0 ) fprintf ( stderr, "  " );
                if ( (j+1)%40 == 0 ) fprintf ( stderr, "\n            " );
            }
            fprintf ( stderr, "\n" );
        }
        fprintf ( stderr, "\n" );
    }

}


/* get/free/provide a_proc struct for reduction state processing */
struct a_proc *aud_mem_proc ( oprtn, rel_ptr, pid, hostp, fd )
int oprtn;              /* 0: release a_proc struct; 1: get new struct  */
                        /* 2: get addr for <pid,hostp>                  */
                        /* 3: debug; 4: dump state on fd                */
struct a_proc *rel_ptr; /* free referenced a_proc                       */
short pid;
int hostp;
int fd;
{
    static struct a_proc *free_list = (struct a_proc *)0;
    static struct a_proc *used_list = (struct a_proc *)0;
    struct a_proc *ptr, *ptr2;
    char *cp;
    int i, j, k;

    /* return ptr to a_proc structure */
    if ( oprtn == 1 ) {

        /* get a_proc structure; use free_list and dbly-linked used_list */
        if ( free_list ) {
            ptr = free_list;
            free_list = free_list->a_proc_next;
        }
        else {
            if ( (cp = (char *)sbrk(sizeof(int)+sizeof (struct a_proc))) == (char *)-1 )
                return ((struct a_proc *)-1);
            ALIGN ( ptr, cp, a_proc );
        }
        if ( used_list == (struct a_proc *)0 ) used_list = ptr;
        ptr->a_proc_next = used_list;
        ptr->a_proc_prev = ptr;
        ptr->a_proc_prev = ptr->a_proc_next->a_proc_prev;
        ptr->a_proc_next->a_proc_prev = ptr;
        ptr->a_proc_prev->a_proc_next = ptr;

        /* update proc_tbl and a_proc structure */
        ptr->auid = -1;
        ptr->pid = pid;
        ptr->ruid = -1;
        ptr->login_proc = 0;
        ptr->ipaddr = hostp;
        ptr->cwd = (char *)0;
        ptr->root = (char *)0;
        ptr->username = (char *)0;
        for ( i = 0; i < _NFILE; i++ ) ptr->fd_nm[i] = (char *)0;
        ptr->access_gp_indx = 0;
        return ( ptr );
    }

    /* find a_proc struct for this <pid,hostp> */
    if ( oprtn == 2 ) {
        if ( used_list == (struct a_proc *)0 ) return ( (struct a_proc *)-1 );
        ptr = used_list;
        do {
            ptr = ptr->a_proc_prev;
            if ( ptr->pid == pid && ptr->ipaddr == hostp ) return ( ptr );
        } while ( ptr != used_list );
        return ( (struct a_proc *)-1 );
    }

    /* release a_proc struct */
    if ( oprtn == 0 && rel_ptr ) {
        if ( rel_ptr->pid == pid && rel_ptr->ipaddr == hostp ) {
            rel_ptr->a_proc_next->a_proc_prev = rel_ptr->a_proc_prev;
            rel_ptr->a_proc_prev->a_proc_next = rel_ptr->a_proc_next;
            used_list = rel_ptr->a_proc_next;
            if ( used_list == rel_ptr ) used_list = (struct a_proc *)0;
            rel_ptr->a_proc_next = free_list;
            free_list = rel_ptr;
            return ( (struct a_proc *)0 );
        }
        else return ( (struct a_proc *)-1 );
    }

    /* debug */
    if ( oprtn == 3 ) {
        fprintf ( stderr, "used_list:  " );
        if ( used_list )
            for ( i = 0, ptr = ptr2 = used_list; ptr != ptr2 || i == 0; ptr = ptr->a_proc_next, i++ ) {
                fprintf ( stderr, "0x%x  ", ptr );
                if ( (i+1)%5 == 0 ) fprintf ( stderr, "\n            " );
            }
        fprintf ( stderr, "\n" );
        fprintf ( stderr, "free_list:  " );
        for ( i = 0, ptr = free_list; ptr; ptr = ptr->a_proc_next, i++ ) {
            fprintf ( stderr, "0x%x  ", ptr );
            if ( (i+1)%5 == 0 ) fprintf ( stderr, "\n            " );
        }
        fprintf ( stderr, "\n" );
    }

#define DUMPIT(obj,indx) \
    for ( indx = 0; obj && obj[indx]; indx++ ); \
    write ( fd, &indx, sizeof(int) ); \
    write ( fd, obj, indx );

    /* dump rvalues and ptrs in a_proc structures to fd */
    if ( oprtn == 4 && used_list )
        for ( i = 0, ptr = ptr2 = used_list; ptr != ptr2 || i == 0; ptr = ptr->a_proc_next, i++ ) {
            write ( fd, ptr, A_PROC_HDR_SIZ );
            DUMPIT ( ptr->cwd, j );
            DUMPIT ( ptr->root, j );
            DUMPIT ( ptr->username, j );
            for ( j = 0; j < _NFILE; j++ ) {
                DUMPIT ( ptr->fd_nm[j], k );
            }
        }
}


/* build deselection ruleset return # rules built */
build_ruleset ( rulesfile, display )
char *rulesfile;    /* pathname of rulesfile */
int display;        /* display rulesets      */
{
    char buf[2][MAX_RULE_SIZ];      /* switch-buffering for rule input  */
    int buf_sw = 1;                 /* switch indicating current buffer */
    char conv_buf[16];              /* buffer to atoi()                 */
    char param_buf[MAX_RULE_SIZ];   /* must hold param before sbrk     */
    char event_buf[STR_LEN2];       /* must hold event before sbrk      */
    char hostname_buf[HOST_LEN];    /* must hold hostname before sbrk   */
    int ruleno = 0;                 /* count of rules built             */

    int start;
    int end;
    int fd;
    int i, j, k;
    char *cp;

    /* open rulesfile */
    if ( (fd = open ( rulesfile, 0 )) == -1 ) {
        fprintf ( stderr, "build_ruleset: could not open %s\n", rulesfile );
        return(0);
    }

    /* read in rules */
    start = end = 0;
    end = read ( fd, buf[buf_sw], sizeof buf[0] );
    do {

        /* alternate buffers; no rule may occupy >2 buffers */
        for ( j = start; j < end && buf[buf_sw][j] != '\n'; j++ );
        if ( j == end ) 
            if ( (end = read ( fd, buf[buf_sw^1], sizeof buf[0] )) == 0 ) break;

        /* allow comment lines */
        if ( buf[buf_sw][start] == '#' ) {
            for ( j = start; buf[buf_sw][j] != '\n'; j++ );
            start = j+1;
            continue;
        }

        /* allocate rules struct */
        if ( ruleno%RULES_IN_SET == 0 ) {
            if ( (cp = (char *)sbrk (sizeof(int)+sizeof(struct ruleset) )) == (char *)-1 ) {
                fprintf ( stderr, "sbrk failed on ruleset %d\n", ruleno/RULES_IN_SET );
                return ( ruleno );
            }
            ALIGN ( rules[ruleno/RULES_IN_SET], cp, ruleset );
        }

        /* read hostname */
        for ( j = start; buf[buf_sw][j] == '\t' || buf[buf_sw][j] == ' ' ||
        buf[buf_sw][j] == '\n'; j++ );
        for ( k = 0; buf[buf_sw][j] != '\t' && buf[buf_sw][j] != ' ' &&
        buf[buf_sw][j] != '\n' && k < sizeof hostname_buf-1; j++, k++ ) {
            if ( j == sizeof buf[0] ) buf_sw ^= 1, j = 0;
            hostname_buf[k] = buf[buf_sw][j];
        }
        hostname_buf[k] = '\0';
        if ( (RULE(ruleno,host) = (caddr_t)sbrk(k+1)) == (caddr_t)-1 ) {
            fprintf ( stderr, "sbrk failed on ruleset %d, rule %d\n",
            ruleno/RULES_IN_SET, ruleno%RULES_IN_SET );
            continue;
        }
        bcopy ( hostname_buf, RULE(ruleno,host), k+1 );
        for ( start = j; buf[buf_sw][start] != '\t' && buf[buf_sw][start] != ' ';
        start++ );

        /* read audit_id */
        for ( j = start; buf[buf_sw][j] == '\t' || buf[buf_sw][j] == ' '; j++ );
        if ( buf[buf_sw][j] == '\n' ) {
            fprintf ( stderr, "bad rule at line #%d in %s\n", ruleno+1, rulesfile );
            continue;
        }
        if ( buf[buf_sw][j] == '*' )
            RULE(ruleno,auid) = -1;
        else {
            for ( k = 0; buf[buf_sw][j] != '\t' && buf[buf_sw][j] != ' ' &&
            buf[buf_sw][j] != '\n' && k < sizeof conv_buf-1; j++, k++ ) {
                if ( j == sizeof buf[0] ) buf_sw ^= 1, j = 0;
                conv_buf[k] = buf[buf_sw][j];
            }
            conv_buf[k] = '\0';
            RULE(ruleno,auid) = atoi(conv_buf);
        }
        for ( start = j; buf[buf_sw][start] != '\t' && buf[buf_sw][start] != ' ';
        start++ );

        /* read real uid */
        for ( j = start; buf[buf_sw][j] == '\t' || buf[buf_sw][j] == ' '; j++ );
        if ( buf[buf_sw][j] == '\n' ) {
            fprintf ( stderr, "bad rule at line #%d in %s\n", ruleno+1, rulesfile );
            continue;
        }
        if ( buf[buf_sw][j] == '*' )
            RULE(ruleno,ruid) = -1;
        else {
            for ( k = 0; buf[buf_sw][j] != '\t' && buf[buf_sw][j] != ' ' &&
            buf[buf_sw][j] != '\n' && k < sizeof conv_buf-1; j++, k++ ) {
                if ( j == sizeof buf[0] ) buf_sw ^= 1, j = 0;
                conv_buf[k] = buf[buf_sw][j];
            }
            conv_buf[k] = '\0';
            RULE(ruleno,ruid) = atoi(conv_buf);
        }
        for ( start = j; buf[buf_sw][start] != '\t' && buf[buf_sw][start] != ' ';
        start++ );

        /* read event */
        for ( j = start; buf[buf_sw][j] == '\t' || buf[buf_sw][j] == ' '; j++ );
        if ( buf[buf_sw][j] == '\n' ) {
            fprintf ( stderr, "bad rule at line #%d in %s\n", ruleno+1, rulesfile );
            continue;
        }
        if ( buf[buf_sw][j] == '"' ) {
            for ( j++, k = 0; buf[buf_sw][j] != '"'; j++, k++ ) {
                if ( j == sizeof buf[0] ) buf_sw ^= 1, j = 0;
                event_buf[k] = buf[buf_sw][j];
            }
        }
        else for ( k = 0; buf[buf_sw][j] != '\t' && buf[buf_sw][j] != ' ' &&
        buf[buf_sw][j] != '\n' && k < sizeof event_buf-1; j++, k++ ) {
            if ( j == sizeof buf[0] ) buf_sw ^= 1, j = 0;
            event_buf[k] = buf[buf_sw][j];
        }
        event_buf[k] = '\0';
        if ( (RULE(ruleno,event) = (caddr_t)sbrk(k+1)) == (caddr_t)-1 ) {
            fprintf ( stderr, "sbrk failed on ruleset %d, rule %d\n",
            ruleno/RULES_IN_SET, ruleno%RULES_IN_SET );
            continue;
        }
        bcopy ( event_buf, RULE(ruleno,event), k+1 );
        for ( start = j; buf[buf_sw][start] != '\t' && buf[buf_sw][start] != ' ';
        start++ );

        /* read param string */
        for ( j = start; buf[buf_sw][j] == '\t' || buf[buf_sw][j] == ' '; j++ );
        if ( buf[buf_sw][j] == '\n' ) {
            fprintf ( stderr, "bad rule at line #%d in %s\n", ruleno+1, rulesfile );
            continue;
        }
        for ( k = 0; buf[buf_sw][j] != '\t' && buf[buf_sw][j] != ' ' &&
        buf[buf_sw][j] != '\n' && k < sizeof param_buf-1; j++, k++ ) {
            if ( j == sizeof buf[0] ) buf_sw ^= 1, j = 0;
            param_buf[k] = buf[buf_sw][j];
        }
        param_buf[k] = '\0';
        if ( (RULE(ruleno,param) = (caddr_t)sbrk(k+1)) == (caddr_t)-1 ) {
            fprintf ( stderr, "sbrk failed on ruleset %d, rule %d\n",
            ruleno/RULES_IN_SET, ruleno%RULES_IN_SET );
            continue;
        }
        bcopy ( param_buf, RULE(ruleno,param), k+1 );
        for ( start = j; buf[buf_sw][start] != '\t' && buf[buf_sw][start] != ' ';
        start++ );

        /* read operation */
        for ( j = start; buf[buf_sw][j] == '\t' || buf[buf_sw][j] == ' '; j++ );
        RULE(ruleno,oprtn) = -1;
        if ( buf[buf_sw][j] != '\n' ) {
            RULE(ruleno,oprtn) = 0;
            for ( k = 0; buf[buf_sw][j] != '\t' && buf[buf_sw][j] != ' ' &&
            buf[buf_sw][j] != '\n'; j++, k++ ) {
                if ( j == sizeof buf[0] ) buf_sw ^= 1, j = 0;
                if ( buf[buf_sw][j] == 'r' ) RULE(ruleno,oprtn) += 1;
                if ( buf[buf_sw][j] == 'w' ) RULE(ruleno,oprtn) += 2;
            }
            for ( ; buf[buf_sw][j] != '\n'; j++ );
            start = j+1;
            RULE(ruleno,oprtn)--;
        }

        /* max # rules hit */
        if ( ++ruleno == RULES_IN_SET * NRULESETS ) {
            fprintf ( stderr, "Maximum # rules (%d) reached.\n", ruleno );
            break;
        }

    } while ( start < end );
    close(fd);

    /* display rulesets */
    if ( display ) fprintf ( stderr, "    hostname audit_id ruid event string oprtn(r/w)\n" );
    for ( i = 0; (i < ruleno) && display; i++ ) {
        fprintf ( stderr, "r%d: ", i );
        fprintf ( stderr, "%s ", RULE(i,host) );
        if ( RULE(i,auid) == -1 )
            fprintf ( stderr, "* " );
        else fprintf ( stderr, "%-6d ", RULE(i,auid) );
        if ( RULE(i,ruid) == -1 )
            fprintf ( stderr, "* " );
        else fprintf ( stderr, "%-6d ", RULE(i,ruid) );
        fprintf ( stderr, "%s ", RULE(i,event) );
        fprintf ( stderr, "%s ", RULE(i,param) );
        if ( RULE(i,oprtn) == -1 )
            fprintf ( stderr, "*\n" );
        else fprintf ( stderr, "%d\n", RULE(i,oprtn) );
    }
    fprintf ( stderr, "\n\n" );

    return ( ruleno );
}


/* process change audit log directive */
change_log ( fd_p, logfile, time_l, af )
int *fd_p;                  /* ptr to audit log descriptor      */
char *logfile;              /* current auditlog file            */
long time_l;                /* seconds component of timestamp   */
struct audit_fields *af;    /* audit record fields              */
{
    struct stat sbuf;
    char logfilehdr[MAXPATHLEN];
    int i, j;

    /* close fd; compress previously compressed files */
    close(*fd_p);
    compress ( 1, (char *)0 );

    /* dump sort status, timestamp and next logname into current logfile hdr */
    for ( i = 0; logfile[i]; i++ );
    bcopy ( logfile, logfilehdr, i );
    if ( strncmp ( &logfilehdr[i-2], ".Z", 2 ) == 0 ) i -= 2;
    bcopy ( ".hdr\0", &logfilehdr[i], 5 );
    if ( (j = open ( logfilehdr, O_CREAT|O_WRONLY, 0600 )) != -1 ) {
        write ( j, &sort_flag, sizeof(sort_flag) );
        write ( j, &time_l, sizeof(time_l) );
        write ( j, &af->timeval.tv_sec, sizeof(time_l) );
        write ( j, af->charparam[1], af->charlen[1] );
        close(j);
    }
    sort_flag = 0;

    /* dump info in next log's logfile hdr */
    i = af->charlen[1] < MAXPATHLEN-5 ? af->charlen[1] : MAXPATHLEN-5;
    bcopy ( af->charparam[1], logfile, i ); 
    logfile[i] = '\0';
    bcopy ( af->charparam[1], logfilehdr, i );
    if ( strncmp ( &logfilehdr[i-2], ".Z", 2 ) == 0 ) i -= 2;
    bcopy ( ".hdr\0", &logfilehdr[i], 5 );
    if ( (j = open ( logfilehdr, O_CREAT|O_WRONLY|O_EXCL, 0600 )) != -1 ) {
        lseek ( j, (sizeof time_l)*2+(sizeof sort_flag)+MAXPATHLEN, L_SET );
        aud_mem_proc ( 4, (struct a_proc *)0, 0, 0, j );
        close(j);
    }

    /* audit data transferred to another host */
    if ( strncmp ( &af->charparam[0][20], "host", 4 ) == 0 ) {
        fprintf ( stderr, "** Audit log change: data sent to remote host %s **\n\n", logfile );
        *fd_p = -1;
    }

    /* open new file; uncompress files ending in .Z */
    else if ( stat ( logfile, &sbuf ) == 0 ) {
        if ( (*fd_p = open ( logfile, 0 )) == -1 )
            fprintf ( stderr, "** Audit log change: failed to open %s **\n\n", logfile );
    }

    /* else try logfile.Z */
    else {
        bcopy ( ".Z\0", &logfile[i], 3 );
        fprintf ( stderr, "** Audit log change: trying %s **\n\n", logfile );
        if ( stat ( logfile, &sbuf ) == 0 ) {
            compress ( 0, logfile );
            if ( (*fd_p = open ( logfile, 0 )) == -1 )
                fprintf ( stderr, "** Audit log change: failed to open %s **\n\n", logfile );
        }
        else *fd_p = -1;
    }
}


/* compress/uncompress filnam */
compress ( op, filnam )
int op;         /* 1: compress; 0: uncompress */
char *filnam;   /* file to uncompress         */
{
    static char oldlog[MAXPATHLEN];
    static char cmd1[MAXPATHLEN+9] = "compress ";
    static char cmd2[MAXPATHLEN+11] = "uncompress ";
    static int compress = 0;
    int i;

    switch ( op ) {

    case 0: /* uncompress filnam, if ending in .Z */
        for ( i = 0; filnam[i]; i++ );
        if ( (filnam[i-2] == '.') && (filnam[i-1] == 'Z') ) {
            bcopy ( filnam, &cmd2[11], i+1 );
            if ( system ( cmd2 ) ) fprintf ( stderr, "failed on: %s\n", cmd2 );
            else {
                fprintf ( stderr, "** Uncompressed %s **\n\n", filnam );
                filnam[i-2] = '\0';
                bcopy ( filnam, oldlog, i-1 );
                compress = 1;
            }
        }
        break;

    case 1: /* compress previously compressed filnam */
        if ( compress == 1 ) {
            for ( i = 0; oldlog[i]; i++ );
            bcopy ( oldlog, &cmd1[9], i+1 );
            if ( system ( cmd1 ) ) fprintf ( stderr, "failed on: %s\n", cmd1 );
            else fprintf ( stderr, "** Compressed %s **\n\n", oldlog );
            compress = 0;
        }
        break;

    }
}


/* check audit_fields against deselection rules; return match */
int deselect ( af, ruleno )
struct audit_fields *af;    /* audit fields struct */
int ruleno;                 /* # deselection rules */
{
    extern char *syscallnames[];
    extern char *trustedevent[];
    int match = 0;
    char *eventp;
    char *hostp;
    int i, j, k;

    /* search for a rule which matches current audit record */
    for ( i = 0; i < ruleno; i++ ) {

        /* compare hostname against rules */
        if ( RULE(i,host)[0] == '*' ) match = 1;
        else {
            hostp = gethost_l(af->ipaddr);
            for ( j = 0; RULE(i,host)[j] == hostp[j] && hostp[j]; j++ );
            if ( RULE(i,host)[j] == '\0' && hostp[j] == '\0' ) match = 1;
        }

        /* compare auid, ruid against rules */
        if ( match )
            match = ((af->auid == RULE(i,auid)) || (RULE(i,auid) == -1));
        if ( match )
            match = ((af->ruid == RULE(i,ruid)) || (RULE(i,ruid) == -1));

        /* compare event against rules */
        if ( match ) {
            match = 0;
            if ( af->event == SYS_exit && af->login_proc )
                eventp = _LOGOUT;
            else if ( af->event >= 0 && af->event <= NUM_SYSCALLS ) 
                eventp = syscallnames[af->event];
            else if ( af->event >= MIN_TRUSTED_EVENT && af->event < MIN_TRUSTED_EVENT + N_TRUSTED_EVENTS )
                eventp = trustedevent[af->event-MIN_TRUSTED_EVENT];
            else if ( af->event >= MIN_SPECIAL && af->event <= MAX_SPECIAL )
                eventp = special_event[af->event-MIN_SPECIAL];
            else {
                if ( af->flag == 0 ) af->flag = 1;
                return(0);
            }
            for ( k = 0; (eventp[k] == RULE(i,event)[k]) && eventp[k]; k++ );
            if ( (eventp[k] == '\0' && RULE(i,event)[k] == '\0') || RULE(i,event)[k] == '*' )
                match = 1;
        }

        /* compare string params against rules; allow '*' wildcard */
        if ( match ) {
            if ( RULE(i,param)[0] == '*' ) match = 1;
            else for ( match = j = 0; j < af->charp_indx; j++ ) {
                for ( k = 0; (af->charparam[j][k] == RULE(i,param)[k]) &&
                (k < af->charlen[j]); k++ );
                if ( (k == af->charlen[j] && RULE(i,param)[k] == '\0')
                || RULE(i,param)[k] == '*' ) {
                    match = 1;
                    break;
                }
            }
        }

        /* compare operation against rules for open()'s (using 1st intparam) */
        if ( match && af->event == SYS_open )
            match = (af->intparam[0] == RULE(i,oprtn)) || (RULE(i,oprtn) == -1);

        if ( match ) return ( match );
    }
    return(0);
}


/* fetch header file and update a_proc structure and state; return sort status */
int fetch_hdr ( logfile, start_str, sort )
char *logfile;
char *start_str;                    /* user specified start time */
int  sort;                          /* 1: check sort status only */
                                    /* 2: update sortflag in hdr */
{
    char logfilehdr[MAXPATHLEN];
    struct a_proc *a_ptr;
    long start_time;                /* start time from hdr file  */
    long end_time;                  /* end time from header file */
    char nextlog[MAXPATHLEN];       /* next audit log filename   */
    int fd;
    int i, j;

    /* open first logfile header */
    for ( i = 0; logfile[i] && i < MAXPATHLEN-5; i++ );
    bcopy ( logfile, logfilehdr, i );
    if ( strncmp ( &logfilehdr[i-2], ".Z", 2 ) == 0 ) i -= 2;
    bcopy ( ".hdr\0", &logfilehdr[i], 5 );
    if ( (fd = open ( logfilehdr, 2 )) == -1 ) return(0);

    /* check/update sort status */
    if ( sort == 2 ) {
        write ( fd, &sort, sizeof(int) );
        return(0);
    }
    read ( fd, &sort_flag, sizeof(sort_flag) );
    if ( sort == 1 ) return(sort_flag);

    /* open file which corresponds to user specified time */
    if ( *start_str ) {
        j = 0;
        do {
            read ( fd, &start_time, sizeof(start_time) );
            read ( fd, &end_time, sizeof(end_time) );
            i = 0;
            do {
                read ( fd, &nextlog[i], 1 );
            } while ( nextlog[i++] && i < MAXPATHLEN );

            if ( start_time == 0 && end_time == 0 ) j = 1;
            else if ( (match_time ( start_str, start_time ) >= 0) &&
                (match_time ( start_str, end_time ) <= 0) ) j = 1;

            /* get next logfile hdr */
            else {
                bcopy ( nextlog, logfile, i );
                close(fd);
                bcopy ( logfile, logfilehdr, i );
                bcopy ( ".hdr\0", &logfilehdr[i-1], 5 );
                if ( (fd = open ( logfilehdr, 0 )) == -1 ) return(0);
            }
        } while ( j == 0 );
    }

#define FETCHIT(obj,indx) \
    read ( fd, &indx, sizeof(int) ); \
    if ( indx ) { \
        obj = aud_mem_op ( indx, (char *)0, 0, 0 ); \
        if ( obj ) read ( fd, obj, indx ); \
        else lseek ( fd, indx, L_INCR ); \
    }

    /* read state information */
    lseek ( fd, (sizeof start_time)*2+(sizeof sort_flag)+MAXPATHLEN, L_SET );
    for ( ;; ) {
        a_ptr = aud_mem_proc ( 1, (struct a_proc *)0, -1, -1, 0 );
        if ( a_ptr == (struct a_proc *)-1 ) break;
        if ( (i = read ( fd, (char *)a_ptr, A_PROC_HDR_SIZ )) != A_PROC_HDR_SIZ ) break;
        FETCHIT ( a_ptr->cwd, i );
        FETCHIT ( a_ptr->root, i );
        FETCHIT ( a_ptr->username, i );
        for ( i = 0; i < _NFILE; i++ ) {
            FETCHIT ( a_ptr->fd_nm[i], j );
        }
    }
    close ( fd );
    return(0);
}


/* fetch records matching stated conditions; return record ptr; update rec_len */
char *fetch_matching_rec ( audit_fields, selectn, cnt, flag, rec_len, fd_p )
struct audit_fields *audit_fields;  /* audit record fields    */
struct selectn *selectn;            /* selection criteria     */
int *cnt;                           /* # records processed    */
int flag;                           /* misc options           */
int *rec_len;                       /* length of record       */
int *fd_p;                          /* data file descriptor   */
{
    static char logfile[MAXPATHLEN];
    struct stat sbuf;
    char *rec_ptr;
    int match;
    static int get_time = 1;
    static long time_l;
    static int cnt_l = 0;           /* # records processed in current log */
    static int auditd_event = 0;    /* auditd audit log event occurred    */
    int pos1, pos2;
    int i;

    /* open auditlog datafile and header; use timestamp if provided */
    if ( *fd_p == 0 ) {
        for ( i = 0; selectn->logfile[i]; i++ );
        bcopy ( selectn->logfile, logfile, i+1 );
        fetch_hdr ( logfile, selectn->time_start, 0 );
        if ( stat ( logfile, &sbuf ) == -1 ) {
            for ( i = 0; logfile[i]; i++ );
            if ( strncmp ( &logfile[i-2], ".Z", 2 ) )
                bcopy ( ".Z\0", &logfile[i], 3 );
        }
        compress ( 0, logfile );
        if ( (*fd_p = open ( logfile, 0 )) == -1 ) {
            fprintf ( stderr, "failed to open %s\n", logfile );
            return((char *)-1);
        }
        cnt_l = 0;
        if ( flag & FLAG_SORT ) audit_sort ( logfile );
    }
    if ( *fd_p == -1 ) return((char *)-1);

    /* fetch records until selection criteria satisfied */
    do {

        init_audit_fields ( audit_fields );
        rec_ptr = fetch_rec ( fd_p, rec_len, audit_fields, flag&FLAG_FOLLOW, cnt_l );
        if ( rec_ptr == (char *)-1 ) {
            if ( (flag&FLAG_OVERRIDE) == 0 ) compress ( 1, (char *)0 );
            return ( (char *)-1 );
        }
        (*cnt)++; cnt_l++;

        /* allow partial records after auditd event */
        if ( auditd_event && audit_fields->flag == 2 ) audit_fields->flag = 0;

        /* parse data; maintain process state */
        parse_rec ( rec_ptr, *rec_len, audit_fields );
        state_maint ( audit_fields );

        /* find records matching input params; deselect according to ruleset */
        match = match_rec ( audit_fields, selectn );
        if ( ruleno && match ) match = !deselect ( audit_fields, ruleno );

        /* save first timestamp for logfile hdr */
        if ( get_time ) {
            time_l = audit_fields->timeval.tv_sec;
            get_time = 0;
        }

        /* check for auditd change log messages */
        if ( (audit_fields->event == AUDIT_LOG_CHANGE) && ((flag&FLAG_OVERRIDE) == 0) ) {
            pos1 = tell ( *fd_p );
            lseek ( *fd_p, 0, L_XTND );
            pos2 = tell ( *fd_p );
            if ( pos1 == pos2 ) {
                change_log ( fd_p, logfile, time_l, audit_fields );
                if ( flag & FLAG_SORT ) audit_sort ( logfile );
                cnt_l = 0;
                get_time = 1;
            }
            else lseek ( *fd_p, pos1, L_SET );
        }
        if ( (audit_fields->event == AUDIT_LOG_CREAT) ||
        (audit_fields->event == AUDIT_LOG_OVERWRITE) ||
        (audit_fields->event == AUDIT_SUSPEND) ||
        (audit_fields->event == AUDIT_SHUTDOWN) ||
        (audit_fields->event == AUDIT_XMIT_FAIL) )
            auditd_event = 1;
        else auditd_event = 0;

    } while ( match == 0 );

    return ( rec_ptr );
}


/* read audit record; return ptr; modify length */
char *fetch_rec ( fd_p, rec_len, af, follow, cnt )
int *fd_p;                  /* audit file descriptor             */
int *rec_len;               /* length of audit record - returned */
struct audit_fields *af;    /* audit record fields               */
int follow;                 /* like tail -f                      */
int cnt;                    /* # records processed from *fd_p    */
{
    static char buf[AUD_BUF_SIZ];
    static int recvr = 0;
    char rec_len_buf[sizeof *rec_len];
    char token;
    int ptr = 0;
    int i, j;

    /* read record length */
    do {

        /* find first LENGTH token */
        do {
            j = read ( *fd_p, &token, sizeof token );
            if ( token != T_LENGTH && j == sizeof token ) af->flag = 2;

            /* attempt recovery of split record */
            /* assumes first record is remainder of split record */
            if ( recvr && cnt == 0 ) {
                recvr = 0;
                if ( recover ( 1, buf, *rec_len, fd_p ) == 0 )
                    af->flag = 7;
                return ( buf );
            }

        } while ( ((j == 0) && follow) || ((j > 0) && (token != T_LENGTH)) );
        if ( j <= 0 ) return ( (char *)-1 );
        bcopy ( &token, buf, sizeof token );

        /* read length */
        i = 0;
        do {
            j = read ( *fd_p, &rec_len_buf[i], 1 );
            if ( j ) i++;
        } while ( ((j == 0) && follow) || ((j > 0) && (i < sizeof *rec_len)) );
        bcopy ( rec_len_buf, rec_len, sizeof(int) );
        if ( j <= 0 ) return ( (char *)-1 );
        if ( (*rec_len < 0) | (*rec_len >= AUD_BUF_SIZ) ) af->flag = 3;

    } while ( (*rec_len < 0) || (*rec_len >= AUD_BUF_SIZ) );
    bcopy ( rec_len, &buf[sizeof token], sizeof *rec_len );

    /* read next char into start of data area */
    do {
        j = read ( *fd_p, &buf[sizeof *rec_len + sizeof token], sizeof token );
    } while ( (j == 0) && follow );
    if ( j <= 0 ) return ( (char *)-1 );

    /* if next char was another LENGTH token, read length again */
    if ( buf[sizeof *rec_len + sizeof token] == T_LENGTH ) {
        i = 0;
        do {
            j = read ( *fd_p, &rec_len_buf[i], 1 );
            if ( j ) i++;
        } while ( ((j == 0) && follow) || ((j > 0) && (i < sizeof *rec_len)) );
        bcopy ( rec_len_buf, rec_len, sizeof(int) );
        if ( j <= 0 ) return ( (char *)-1 );
        bcopy ( rec_len, &buf[sizeof token], sizeof *rec_len );
    }
    else ptr++;

    /* read data */
    if ( (*rec_len < 0) || (*rec_len >= AUD_BUF_SIZ) ) {
        af->flag = 4;
        return ( buf );
    };
    i = *rec_len - (sizeof *rec_len + sizeof token + ptr);
    j = 0;
    do {
        j += read ( *fd_p, &buf[ptr + sizeof *rec_len + sizeof token + j], i-j );
    } while ( (j < i) && follow );
    if ( j < i ) return ( (char *)-1 );

    /* consistency check */
    bcopy ( &buf[*rec_len - (sizeof *rec_len)], &i, sizeof(int) );
    if ( i != *rec_len && af->flag != 7 ) {
        i = recover ( 0, buf, *rec_len, fd_p );
        if ( i == AUDIT_LOG_CHANGE ) {
            af->flag = 8;
            recvr = 1;
        }
        else if ( i == AUDIT_SUSPEND ) af->flag = 9;
        else if ( i == AUDIT_LOG_CREAT ) af->flag = 9;
        else if ( i == AUDIT_LOG_OVERWRITE ) af->flag = 9;
        else if ( i == AUDIT_SHUTDOWN ) af->flag = 9;
        else if ( i == AUDIT_XMIT_FAIL ) af->flag = 9;
        else af->flag = 5;
        *rec_len = 0;
     }

    return ( buf );
}


/* return hostname associated with ipaddr */
char *gethost_l ( ipaddr )
long ipaddr;
{
    struct hostent *hostp;
    static long ipaddr_prev = 0;
    static char h_name_prev[128];
    int i;

    if ( ipaddr_prev && ipaddr == ipaddr_prev ) return ( h_name_prev );
    if ( (hostp = gethostbyaddr ( (char *)&ipaddr, sizeof(long), AF_INET )) == (struct hostent *)0 )
        return ( (char *)0 );

    /* save hostname, ipaddr for next iteration */
    ipaddr_prev = ipaddr;
    for ( i = 0; hostp->h_name[i]; i++ );
    bcopy ( hostp->h_name, h_name_prev, i+1 );

    return ( hostp->h_name );
}


#ifdef audit_tool_X
/* modify siz_p and offset_p to reflect referenced file */
int get_posn ( fd, siz_p, offset_p )
int fd;
off_t *siz_p;
int *offset_p;
{
    struct stat statbuf;

    if ( fstat ( fd, &statbuf ) ) return(-1);
    *siz_p = statbuf.st_size;
    *offset_p = tell(fd);
    return(0);
}
#endif audit_tool_X


/* initialize audit_fields structure */
init_audit_fields ( af )
struct audit_fields *af;
{
    af->event             = -1;
    af->error             =  0;
    af->flag              =  0;
    af->result            = -1;
    af->device            = -1;
    af->auid              = -1;
    af->uid               = -1;
    af->ruid              = -1;
    af->hostid            = -1;
    af->ipaddr            =  0;
    af->pid               = -1;
    af->ppid              = -1;
    af->n_cpu             =  0;
#ifdef PRIV
    af->privstr.mask[0]   =  0;
    af->privstr.mask[1]   =  0;
#endif PRIV

    af->device2           = -1;
    af->auid2             = -1;
    af->uid2              = -1;
    af->ruid2             = -1;
    af->hostid2           = -1;
    af->ipaddr2           =  0;
    af->pid2              = -1;
    af->ppid2             = -1;
    af->login_proc        =  0;

    af->login_indx        =  0;
    af->login2_indx       =  0;
    af->homedir_indx      =  0;
    af->shell_indx        =  0;
    af->service_indx      =  0;
    af->devname_indx      =  0;

    af->atom_id_indx      =  0;
    af->x_client_indx     =  0;
    af->client_id_indx    =  0;
    af->property_indx     =  0;
    af->res_class_indx    =  0;
    af->res_type_indx     =  0;
    af->res_id_indx       =  0;

    af->charp_indx        =  0;
    af->descrip_indx      =  0;
    af->intp_indx         =  0;
    af->shortp_indx       =  0;
    af->int_array_indx    =  0;
    af->gp_id_indx        =  0;
    af->gp_dev_indx       =  0;

    af->socket_indx       =  0;
    af->msg_indx          =  0;
    af->access_indx       =  0;
    af->ipc_uid_indx      =  0;
    af->ipc_gid_indx      =  0;
    af->ipc_mode_indx     =  0;
}


/* initialize selectn structure */
init_selectn ( selectn_p )
struct selectn *selectn_p;
{
    int i;

    selectn_p->auid_indx = 0;
    selectn_p->charparam_indx = 0;
    selectn_p->dev_indx = 0;
    selectn_p->error_indx = 0;
    selectn_p->event_indx = 0;
    selectn_p->gnode_indx = 0;
    selectn_p->gnode_dev_indx = 0;
    selectn_p->ipaddr_indx = 0;
    selectn_p->logfile[0] = '\0';
    selectn_p->n_cpu = -1;
    selectn_p->pid_indx = 0;
    selectn_p->ppid_indx = 0;
    selectn_p->ruid_indx = 0;
    selectn_p->rulesfil[0] = '\0';
    for ( i = 0; i < TIME_LEN; i++ ) {
        selectn_p->time_end[i] = '\0';
        selectn_p->time_start[i] = '\0';
    }
    selectn_p->uid_indx = 0;
    selectn_p->username_indx = 0;
#ifdef PRIV
    selectn_p->priv = -1;
#endif PRIV
}


/* interactive mode */
interact ( selectn, flag )
struct selectn *selectn;    /* selection criteria   */
int *flag;                  /* misc options         */
{
    char buf[MAXPATHLEN];
    int i, j;

#define INTER1(str,arg,len,buf,i) \
    write ( 1, str, sizeof(str) ); \
    if ( arg[0] != '\0' ) { \
        i = sprintf_l ( buf, "(%s)  ", arg ); \
        write ( 1, buf, i ); \
    } \
    i = read ( 0, buf, len ); \
    if ( i > 1 ) { \
        if ( buf[0] == '*' ) i = 1, arg[0] = '\0'; \
        else strncpy ( arg, buf, i-1 ); \
        arg[i-1] = '\0'; \
    }

#define INTER2(str,flag_mode,buf,i) \
    write ( 1, str, sizeof(str) ); \
    i = sprintf_l ( buf, "(%s)  ", *flag & flag_mode ? "yes" : "no" ); \
    write ( 1, buf, i ); \
    i = read ( 0, buf, STR_LEN ); \
    if ( i > 1 ) { \
        if ( buf[0] == '1' || buf[0] == 'y' || buf[0] == 'Y') *flag |= flag_mode; \
        else *flag &= ~flag_mode; \
    }

    write ( 1, "subject:\n", 9 );
    interact_int ( "  audit_id:  ", selectn->auid, &selectn->auid_indx );
    interact_shrt ( "  ruid:  ", selectn->ruid, &selectn->ruid_indx, 0 );
    interact_shrt ( "  uid:  ", selectn->uid, &selectn->uid_indx, 0 );
    interact_str ( "  username:  ", selectn->username, &selectn->username_indx );
    interact_shrt ( "  pid:  ", selectn->pid, &selectn->pid_indx, 0 );
    interact_shrt ( "  ppid:  ", selectn->ppid, &selectn->ppid_indx, 0 );
    interact_shrt ( "  dev:  ", selectn->dev, &selectn->dev_indx, 1 );
    interact_host ( selectn->ipaddr, &selectn->ipaddr_indx );

    write ( 1, "\nevent:\n", 7 );
    interact_event ( selectn->event, &selectn->event_indx, selectn->event_status );
    interact_int ( "  error:  ", selectn->error, &selectn->error_indx );
    INTER1 ( "  time_start:  ", selectn->time_start, STR_LEN, buf, i );
    INTER1 ( "  time_end:  ", selectn->time_end, STR_LEN, buf, i );

    write ( 1, "\nobject:\n", 9 );
    interact_str ( "  charparam:  ", selectn->charparam, &selectn->charparam_indx );
    interact_int ( "  gnode:  ", selectn->gnode, &selectn->gnode_indx );
    interact_int ( "  gnode_dev:  ", selectn->gnode_dev, &selectn->gnode_dev_indx );

    INTER1 ( "\nrules file:  ", selectn->rulesfil, STR_LEN, buf, i );
    if ( i > 1 ) ruleno = build_ruleset ( selectn->rulesfil, *flag&FLAG_DISPLAY );
    INTER2 ( "continuous operation:  ", FLAG_FOLLOW, buf, i );
    INTER2 ( "report by audit_id:  ", FLAG_REPORT, buf, i );
    INTER2 ( "quick output format:  ", FLAG_BRIEF, buf, i );
    INTER2 ( "override audit log changes:  ", FLAG_OVERRIDE, buf, i );
    INTER2 ( "use local /etc/passwd and /etc/group:  ", FLAG_LOCALID, buf, i );
    write ( 1, "\n\n", 2 );
}


/* event list = { event[:success[:fail]] } */
interact_event ( field, indx, status )
char field[][STR_LEN];
int *indx;
char *status;
{
    char buf[MAXPATHLEN];
    int found;
    int i, j, k;

    /* output current events[:succeed:fail] selected */
    i = sprintf_l ( buf, "\n  (events: %s selected)\n", *indx == 0 ? "all" : itoa(*indx) );
    write ( 1, buf, i );
    for ( j = 0; j < *indx; j++ ) {
        i = sprintf_l ( buf, "    %s", field[j] );
        write ( 1, buf, i );
        if ( (status[j] & NO_S) == 0 ) {
            i = sprintf_l ( buf, " : succeed" );
            write ( 1, buf, i );
        }
        if ( (status[j] & NO_F) == 0 ) {
            i = sprintf_l ( buf, " : fail" );
            write ( 1, buf, i );
        }
        write ( 1, "\n", 1 );
    }

    /* read new event selections */
    for ( k = *indx; k < N_SELECT; k++, (*indx)++ ) {
        write ( 1, "  event:  ", 10 );
        i = read ( 0, buf, STR_LEN2 );
        if ( i > 1 ) {
            if ( buf[0] == '*' ) {
                *indx = 0;
                break;
            }
            else strncpy ( field[k], buf, i-1 );
            field[k][i-1] = '\0';
        }
        if ( i == 1 ) break;

        /* check for previous occurrence in list */
        for ( k = found = 0; k < *indx; k++ ) {
            for ( j = 0; field[k][j]; j++ );
            if ( strncmp ( field[k], field[*indx], j ) == 0 ) {
                found = 1;
                break;
            }
        }

        /* update selectn event and status lists */
        status[k] = 0x0;
        for ( j = 0; j < i && field[*indx][j] != ':'; j++ );
        if ( j < i ) {
            field[*indx][j] = '\0';
            if ( field[*indx][j+1] == '0' )
                status[k] += NO_S;
            if ( field[*indx][j+2] == ':' && field[*indx][j+3] == '0' )
                status[k] += NO_F;
        }
        *indx -= found;
        k = *indx;
    }
}


/* interactive mode for hostnames and ip addresses */
interact_host ( ipaddr, indx )
unsigned long *ipaddr;
int *indx;
{
    char buf[MAXPATHLEN];
    struct hostent *hp;
    unsigned long addr;
    char *name;
    int i, j;

    write ( 1, "  hostname/addr:  ", 17 );
    if ( *indx == 1 ) {
        if ( name = gethost_l(ipaddr[0]) )
            i = sprintf_l ( buf, "(%s)  ", name );
        else i = sprintf_l ( buf, "(%d)  ", inet_ntoa(ipaddr[0]) );
        write ( 1, buf, i );
    }
    else if ( *indx ) {
        write ( 1, "( ", 2 );
        for ( i = 0; i < *indx; i++ ) {
            if ( name = gethost_l(ipaddr[i]) )
                j = sprintf_l ( buf, "%s  ", name );
            else j = sprintf_l ( buf, "%s  ", inet_ntoa(ipaddr[i]) );
            write ( 1, buf, j );
        }
        write ( 1, ")  ", 3 );
    }

    for ( i = 0; i < N_SELECT2; i++ ) {
        j = read ( 0, buf, HOST_LEN );
        if ( j == 1 ) break;
        if ( buf[0] == '*' ) {
            *indx = 0;
            break;
        }
        buf[j-1] = '\0';

        if ( hp = gethostbyname(buf) ) bcopy ( hp->h_addr, &ipaddr[i], hp->h_length );
        else if ( (j = inet_addr(buf)) != -1 ) ipaddr[i] = j;
        else {
            write ( 1, "   -- bad host/address\n", 21 );
            i--;
        }
        *indx = i+1;
        if ( *indx < N_SELECT2 ) write ( 1, "  hostname/addr:  ", 17 );
    }
}


/* interactive mode for integer arrays */
interact_int ( string, field, indx )
char *string;
int *field;
int *indx;
{
    char buf[MAXPATHLEN];
    int strlen;
    int i, j;

    for ( strlen = 0; string[strlen]; strlen++ );
    write ( 1, string, strlen );

    if ( *indx == 1 ) {
        i = sprintf_l ( buf, "(%d)  ", field[0] );
        write ( 1, buf, i );
    }
    else if ( *indx ) {
        write ( 1, "( ", 2 );
        for ( i = 0; i < *indx; i++ ) {
            j = sprintf_l ( buf, "%d ", field[i] );
            write ( 1, buf, j );
        }
        write ( 1, ")  ", 3 );
    }
    for ( i = 0; i < N_SELECT2; i++ ) {
        j = read ( 0, buf, STR_LEN );
        if ( j == 1 ) break;
        if ( buf[0] == '*' ) {
            *indx = 0;
            break;
        }
        field[i] = atoi(buf);
        *indx = i+1;
        if ( *indx < N_SELECT2 ) write ( 1, string, strlen );
    }
}


/* interactive mode for short arrays */
interact_shrt ( string, field, indx, dev )
char *string;
short *field;
int *indx;
int dev;
{
    char buf[MAXPATHLEN];
    int strlen;
    int i, j;

    for ( strlen = 0; string[strlen]; strlen++ );
    write ( 1, string, strlen );

    if ( *indx == 1 ) {
        if ( dev == 0 ) i = sprintf_l ( buf, "(%d)  ", field[0] );
        else i = sprintf_l ( buf, "(%d,%d)  ", major(field[0]), minor(field[0]) );
        write ( 1, buf, i );
    }
    else if ( *indx ) {
        write ( 1, "( ", 2 );
        for ( i = 0; i < *indx; i++ ) {
            if ( dev == 0 ) j = sprintf_l ( buf, "%d ", field[i] );
            else j = sprintf_l ( buf, "%d,%d ", major(field[i]), minor(field[i]) );
            write ( 1, buf, j );
        }
        write ( 1, ")  ", 3 );
    }
    for ( i = 0; i < N_SELECT2; i++ ) {
        j = read ( 0, buf, STR_LEN );
        if ( j == 1 ) break;
        if ( buf[0] == '*' ) {
            *indx = 0;
            break;
        }
        if ( dev == 0 ) {
            field[i] = atoi(buf);
            *indx = i+1;
        }
        else {
            for ( j = 0; buf[j] && buf[j] != ','; j++ );
            if ( buf[j] == ',' ) {
                field[i] = makedev ( atoi(buf), atoi(&buf[j+1]) );
                *indx = i+1;
            }
        }
        if ( *indx < N_SELECT2 ) write ( 1, string, strlen );
    }
}


/* interactive mode for string arrays */
interact_str ( string, field, indx )
char *string;
char field[][STR_LEN];
int *indx;
{
    char buf[MAXPATHLEN];
    int strlen;
    int i, j;

    for ( strlen = 0; string[strlen]; strlen++ );
    write ( 1, string, strlen );

    if ( *indx == 1 ) {
        i = sprintf_l ( buf, "(%s)  ", field[0] );
        write ( 1, buf, i );
    }
    else if ( *indx ) {
        write ( 1, "( ", 2 );
        for ( i = 0; i < *indx; i++ ) {
            j = sprintf_l ( buf, "%s ", field[i] );
            write ( 1, buf, j );
        }
        write ( 1, ")  ", 3 );
    }

    for ( i = 0; i < N_SELECT2; i++ ) {
        j = read ( 0, buf, STR_LEN );
        if ( j == 1 ) break;
        if ( buf[0] == '*' ) {
            *indx = 0;
            break;
        }
        strncpy ( field[i], buf, j-1 );
        field[i][j-1] = '\0';
        *indx = i+1;
        if ( *indx < N_SELECT2 ) write ( 1, string, strlen );
    }
}


/* convert integer/long decimal to alphanumeric string; return ptr to string */
char *itoa ( num )
int num;
{
    static char num_rep[]= "            ";
    int i;

    for ( i = sizeof num_rep; i == sizeof num_rep || (num > 0 && i >= 0); num = num/10, i-- )
        num_rep[i] = num%10 + '0';

    return ( &num_rep[++i] );
}


/* return 1 for records matching selection criteria; 0 for no match */
int match_rec ( af, selectn )
struct audit_fields *af;            /* audit record fields    */
struct selectn *selectn;            /* selection criteria     */
{
    extern char *syscallnames[];
    extern char *trustedevent[];
    struct a_proc *a_ptr;           /* a_proc ptr to get fd's */
    int retval = 1;
    int len;
    int strnglen;
    struct passwd *pw_ptr;
    char *ptr;
    int i, j, k, l;

#define MATCH(sel,af,indx,retval,i,j) \
    for ( i = j = 0; i < indx; i++ ) \
        if ( sel[i] == af ) j = 1; \
    if ( indx ) retval &= j;

    MATCH ( selectn->auid, af->auid, selectn->auid_indx, retval, i, j );
    MATCH ( selectn->uid, af->uid, selectn->uid_indx, retval, i, j );
    MATCH ( selectn->ruid, af->ruid, selectn->ruid_indx, retval, i, j );
    MATCH ( selectn->error, af->error, selectn->error_indx, retval, i, j );
    MATCH ( selectn->ipaddr, af->ipaddr, selectn->ipaddr_indx, retval, i, j );
    MATCH ( selectn->pid, af->pid, selectn->pid_indx, retval, i, j );
    MATCH ( selectn->ppid, af->ppid, selectn->ppid_indx, retval, i, j );
    MATCH ( selectn->dev, af->device, selectn->dev_indx, retval, i, j );
#undef MATCH

#ifdef PRIV
    if ( selectn->priv != -1 )   retval &= ((af->privstr.mask[0]|af->privstr.mask[1]) ? 1 : 0);
#endif PRIV
    if ( selectn->n_cpu != -1 )  retval &= (selectn->n_cpu == af->n_cpu);

    /* check for matching gnode */
    for ( i = j = 0; i < selectn->gnode_indx; i++ )
        for ( k = 0; k < af->gp_id_indx; k++ )
            if ( selectn->gnode[i] == af->gnode_id[k] ) j = 1;
    if ( selectn->gnode_indx ) retval &= j;

    for ( i = j = 0; i < selectn->gnode_dev_indx; i++ )
        for ( k = 0; k < af->gp_dev_indx; k++ )
            if ( selectn->gnode_dev[i] == af->gnode_dev[k] ) j = 1;
    if ( selectn->gnode_dev_indx ) retval &= j;

    /* check for matching event */
    for ( i = j = 0; i < selectn->event_indx && j == 0; i++ ) {
        for ( len = 0; selectn->event[i][len]; len++ );
        if ( selectn->event[i][len-1] == '+' ) len--;
        else len = STR_LEN2;
        if ( af->event == SYS_exit && af->login_proc )
            j |= !strncmp ( _LOGOUT, selectn->event[i], len );
        else if ( af->event >= 0 && af->event <= NUM_SYSCALLS )
            j |= !strncmp ( syscallnames[af->event], selectn->event[i], len );
        else if ( af->event >= MIN_TRUSTED_EVENT && af->event < MIN_TRUSTED_EVENT + N_TRUSTED_EVENTS )
            j |= !strncmp ( trustedevent[af->event-MIN_TRUSTED_EVENT], selectn->event[i], len );
        else if ( af->event >= MIN_SPECIAL && af->event <= MAX_SPECIAL )
            j |= !strncmp ( special_event[af->event-MIN_SPECIAL], selectn->event[i], len );
        if ( selectn->event_status[i] == NO_F + NO_S ) j = 0;
        if ( selectn->event_status[i] == NO_F && af->error ) j = 0;
        if ( selectn->event_status[i] == NO_S && af->error == 0 ) j = 0;
    }
    if ( selectn->event_indx ) retval &= j;

    /* check for matching username */
    for ( i = j = 0; i < selectn->username_indx; i++ ) {
        for ( len = 0; selectn->username[i][len] && (len < STR_LEN); len++ );
        if ( (a_ptr = aud_mem_proc ( 2, (struct a_proc *)0, af->pid, af->ipaddr, 0 ))!= (struct a_proc *)-1 ) {
            if ( a_ptr->username && strncmp ( selectn->username[i], a_ptr->username, len ) == 0 )
                j = 1;
        }
        if ( (j == 0) && (flag & FLAG_LOCALID) && ( (a_ptr != (struct a_proc *)-1)
        && (a_ptr->username == '\0') ) || (a_ptr == (struct a_proc *)-1) ) {
            if ( pw_ptr = getpwuid ( af->ruid ))
                if ( strncmp ( selectn->username[i], pw_ptr->pw_name, len ) == 0 )
                    j = 1;
        }
    }
    if ( selectn->username_indx ) retval &= j;

    /* check for matching strings in charparams and dereferenced fd's */
    for ( i = j = 0; j == 0 && i < selectn->charparam_indx; i++ ) {
        for ( len = 0; selectn->charparam[i][len] && (len < STR_LEN); len++ );
        for ( k = 0; j == 0 && k < af->charp_indx; k++ )
            for ( l = 0; j == 0 && l <= af->charlen[k]-len; l++ )
                if ( strncmp ( selectn->charparam[i], &af->charparam[k][l], len ) == 0 )
                    j = 1;
        if ( (j == 0) && ((a_ptr = aud_mem_proc ( 2, (struct a_proc *)0, 
        af->pid, af->ipaddr, 0 )) != (struct a_proc *)-1) )
            for ( k = 0; j == 0 && k < af->descrip_indx && af->error == 0; k++ ) {
                if ( af->event == SYS_close || af->event == SYS_dup2 )
                    ptr = close_buf;
                else ptr = a_ptr->fd_nm[af->descrip[k]];
                if ( ptr ) {
                    for ( strnglen = 0; ptr[strnglen]; strnglen++ );
                    for ( l = 0; j == 0 && l <= strnglen-len; l++ )
                        if ( strncmp ( selectn->charparam[i], &ptr[l], len ) == 0 )
                            j = 1;
                }
            }
    }
    if ( selectn->charparam_indx ) retval &= j;

    /* check for matching timestamp */
    if ( *selectn->time_start ) {
        if ( match_time ( selectn->time_start, af->timeval.tv_sec ) <= 0 ) retval &= 1;
        else retval &= 0;
    }
    if ( *selectn->time_end ) {
        if ( match_time ( selectn->time_end, af->timeval.tv_sec ) >= 0 ) retval &= 1;
        else retval &= 0;
    }

    return ( retval );
}


/* return comparison between user specified time string and numeric time_sec */
int match_time ( time_str, time_sec )
char *time_str;
unsigned long time_sec;
{
    struct tm *tma_p;               /* tm struct for time     */
    char tma_tm[TIME_LEN];          /* ascii rep for *tma_p   */
    int i, j;

    tma_p = localtime ( &time_sec );
    tma_tm[0]  = tma_p->tm_year/10+'0';
    tma_tm[1]  = tma_p->tm_year%10+'0';
    tma_tm[2]  = (tma_p->tm_mon+1)/10+'0';
    tma_tm[3]  = (tma_p->tm_mon+1)%10+'0';
    tma_tm[4]  = tma_p->tm_mday/10+'0';
    tma_tm[5]  = tma_p->tm_mday%10+'0';
    tma_tm[6]  = tma_p->tm_hour/10+'0';
    tma_tm[7]  = tma_p->tm_hour%10+'0';
    tma_tm[8]  = tma_p->tm_min/10+'0';
    tma_tm[9]  = tma_p->tm_min%10+'0';
    tma_tm[10] = tma_p->tm_sec/10+'0';
    tma_tm[11] = tma_p->tm_sec%10+'0';

    for ( i = 0; time_str[i] && i < TIME_LEN; i++ );
    j = strncmp ( time_str, tma_tm, i );
    return(j);
}


/* format record output in abbreviated format into bp; return # chars output */
int output_brief_fmt ( af, bp, a_ptr, flag )
struct audit_fields *af;
char *bp;
struct a_proc *a_ptr;
int flag;
{
    static int quick = 0;
    int ofs = 0;
    int i;

    if ( (quick == 0) && ((flag&FLAG_REPORT) == 0) ) {
        ofs += sprintf_l ( &bp[ofs], "AUID     RUID     R     E   PID\n" );
        quick = 1;
    }
    ofs += sprintf_l ( &bp[ofs], "%-8d %-8d %-5d %-3d %-5d", af->auid, af->ruid, af->result, af->error, af->pid );
#ifdef PRIV
    if ( af->privstr.mask[0]|af->privstr.mask[1] ) 
        ofs += sprintf_l ( &bp[ofs], "*: " );
    else 
#endif PRIV
        ofs += sprintf_l ( &bp[ofs], " : " );

    if ( af->event == SYS_exit && af->login_proc )
        ofs += sprintf_l ( &bp[ofs], "%s (%s)\n", syscallnames[af->event], _LOGOUT );
    else if ( af->event >= 0 && af->event <= NUM_SYSCALLS )
        ofs += sprintf_l ( &bp[ofs], "%s (", syscallnames[af->event] );
    else if ( af->event >= MIN_TRUSTED_EVENT && af->event < MIN_TRUSTED_EVENT+N_TRUSTED_EVENTS )
        ofs += sprintf_l ( &bp[ofs], "%s ", trustedevent[af->event-MIN_TRUSTED_EVENT] );
    else if ( af->event >= MIN_SPECIAL && af->event <= MAX_SPECIAL )
        ofs += sprintf_l ( &bp[ofs], "%s (", special_event[af->event-MIN_SPECIAL] );
    else ofs += sprintf_l ( &bp[ofs], "UNKNOWN EVENT %d ", af->event );

    for ( i = 0; (i < af->descrip_indx) && (af->error == 0); i++ ) {
        if ( (a_ptr != (struct a_proc *)-1) && (af->descrip[i] >= 0) && (af->descrip[i] < _NFILE) ) {
            if ( a_ptr->fd_nm[af->descrip[i]] )
                ofs += sprintf_l ( &bp[ofs], " %s", a_ptr->fd_nm[af->descrip[i]] );
            else ofs += sprintf_l ( &bp[ofs], " %d", af->descrip[i] );
        }
        else ofs += sprintf_l ( &bp[ofs], " %d", af->descrip[i] );
    }
    for ( i = 0; i < af->charp_indx && af->event != SYS_audcntl; i++ )
        ofs += sprintf_l ( &bp[ofs], " %.*s", af->charlen[i], af->charparam[i] );
    for ( i = 0; i < af->intp_indx; i++ )
        ofs += sprintf_l ( &bp[ofs], " 0x%x", af->intparam[i] );
    for ( i = 0; i < af->shortp_indx; i++ )
        ofs += sprintf_l ( &bp[ofs], " 0x%x", af->shortparam[i] );
    if ( af->event >= 0 && af->event <= NUM_SYSCALLS ) ofs += sprintf_l ( &bp[ofs], " )\n" );
    else if ( af->event <= MIN_SPECIAL && af->event <= MAX_SPECIAL ) ofs += sprintf_l ( &bp[ofs], " )\n" );
    else ofs += sprintf_l ( &bp[ofs], "\n" );

    return ( ofs );
}


/* format SYS_setgroups output; return # chars */
int output_grp_fmt ( af, bp )
struct audit_fields *af;
char *bp;
{
    struct group *grp;
    int ofs = 0;
    int i, j;

    ofs += sprintf_l ( &bp[ofs], "groups:      " );
    for ( i = 0; i < af->int_array_len[0]; i += sizeof(int) ) {
        bcopy ( af->int_array[0]+i, &j, sizeof(int) );
        if ( grp = getgrgid(j) )
            ofs += sprintf_l ( &bp[ofs], "%s ", grp->gr_name );
        else ofs += sprintf_l ( &bp[ofs], "%d ", j );
    }
    ofs += sprintf_l ( &bp[ofs], "\n" );
    return ( ofs );
}


/* format ipc output intp bp; return # chars output */
int output_ipc_fmt ( af, bp, a_ptr )
struct audit_fields *af;
char *bp;
struct a_proc *a_ptr;
{
    struct sockaddr *sockptr;
    union {
        struct sockaddr sockbuf;
        char buf[MAXPATHLEN];
    } sock_un;
    int ofs = 0;
    int i, j, k;

    /* output socket information */
    for ( i = 0; i < af->socket_indx; i++ ) {
        j = af->socketlen[i] < MAXPATHLEN-1 ? af->socketlen[i] : MAXPATHLEN-1;
        bcopy ( af->socketaddr[i], sock_un.buf, j );
        sock_un.buf[j] = '\0';
        sockptr = (struct sockaddr *)&sock_un.sockbuf;

        /* NOTE: this allows only one aud_client_info structure per record */
        /* aud_client_info structure keeps family separate */
        if ( af->x_client_indx ) {
            bcopy ( &af->x_client[af->x_client_indx-1].family, sock_un.buf, 2 );
            bcopy ( af->socketaddr[i], &sock_un.buf[2], j );
        }

        switch ( sockptr->sa_family ) {

        case AF_UNIX:
            ofs += sprintf_l ( &bp[ofs], "socket:      address (AF_UNIX) = %s\n", sockptr->sa_data );
            break;

        case AF_INET:
            ofs += sprintf_l ( &bp[ofs], "socket:      port (AF_INET) = %d    ",
                ntohs(((struct sockaddr_in *)sockptr)->sin_port) );
            ofs += sprintf_l ( &bp[ofs], "addr = %s (%s)\n",
                inet_ntoa(((struct sockaddr_in *)sockptr)->sin_addr),
                gethost_l(((struct sockaddr_in *)sockptr)->sin_addr) );
            break;

        case AF_UNSPEC:
            ofs += sprintf_l ( &bp[ofs], "socket:      port (AF_UNSPEC) = %d\n", ntohs(((struct sockaddr_in *)sockptr)->sin_port) );
            break;

        default:
            ofs += sprintf_l ( &bp[ofs], "socket:      address (unknown) = %s  address_family = %d\n",
            sockptr->sa_data, sockptr->sa_family );
            break;
        }
    }

    /* output message information */
    for ( i = 0; i < af->msg_indx; i++ ) {
        j = af->msglen[i] < MAXPATHLEN-1 ? af->msglen[i] : MAXPATHLEN-1;
        bcopy ( af->msgaddr[i], sock_un.buf, j );
        sock_un.buf[j] = '\0';
        sockptr = (struct sockaddr *)&sock_un.sockbuf;

        switch ( sockptr->sa_family ) {

        case AF_UNIX:
            ofs += sprintf_l ( &bp[ofs], "socket:      address (AF_UNIX) = %s\n", sockptr->sa_data );
            break;

        case AF_INET:
            ofs += sprintf_l ( &bp[ofs], "socket:      port (AF_INET) = %d\n", ntohs(((struct sockaddr_in *)sockptr)->sin_port) );
            break;

        default:
            ofs += sprintf_l ( &bp[ofs], "socket:      address (unknown) = %s  address_family = %d\n",
            sockptr->sa_data, sockptr->sa_family );
            break;
        }
        for ( j = 0; i < af->access_indx && j < af->accesslen[i]/sizeof(int); j++ ) {
            bcopy ( af->accessaddr[i]+j*sizeof(int), &k, sizeof(int) );
            if ( (a_ptr != (struct a_proc *)-1) && (k >= 0) && (k < _NFILE) ) {
                if ( a_ptr->fd_nm[k] )
                    ofs += sprintf_l ( &bp[ofs], "descriptor:  %s (%d)\n", a_ptr->fd_nm[k], k );
                else ofs += sprintf_l ( &bp[ofs], "descriptor:  %d\n", k );
            }
            else ofs += sprintf_l ( &bp[ofs], "descriptor:  %d\n", k );
        }
    }

    /* output ipc information */
    for ( i = 0; i < af->ipc_uid_indx; i++ ) ofs += sprintf_l ( &bp[ofs], "ipc_uid:     %d\n", af->ipc_uid[i] );
    for ( i = 0; i < af->ipc_gid_indx; i++ ) ofs += sprintf_l ( &bp[ofs], "ipc_gid:     %d\n", af->ipc_gid[i] );
    for ( i = 0; i < af->ipc_mode_indx; i++ ) ofs += sprintf_l ( &bp[ofs], "ipc_mode:    %04o\n", af->ipc_mode[i]&0x01ff );

    return ( ofs );
}


/* format event-specific parameters output into bp; return # chars output */
/* VERY DEPENDENT ON WHICH PARAMETERS PER SYSCALL ARE LOGGED */
int output_param_fmt ( af, bp )
struct audit_fields *af;
char *bp;
{
    static char *audcntl_req[] = {
        "GET_SYS_AMASK",        "SET_SYS_AMASK",    "GET_TRUSTED_AMASK",
        "SET_TRUSTED_AMASK",    "GET_PROC_AMASK",   "SET_PROC_AMASK",
        "GET_PROC_ACNTL",       "SET_PROC_ACNTL",   "SET_AUDSWITCH",
        "FLUSH_AUD_BUF",        "GETPAID",          "SETPAID",
        "GET_AUDSWITCH",        "UPDEVENTS"
    };
    static char *setsysinfo_req[] = {
        "SSI_NVPAIRS",      "SSI_ZERO_STRUCT",
        "SSI_SET_STRUCT",   "SSI_SET_PRIV"
    };
    struct passwd *pw_ptr;
    struct group *gr_ptr;
    int ofs = 0;
    int i, j;

    switch ( af->event ) {

    /* audcntl */
    case SYS_audcntl:
        if ( (af->intparam[0] >= 0) && (af->intparam[0] <= UPDEVENTS) )
            ofs += sprintf_l ( &bp[ofs], "request:     %s\n", audcntl_req[af->intparam[0]] );
        else ofs += sprintf_l ( &bp[ofs], "request:     %d (UNKNOWN)\n", af->intparam[0] );

        switch ( af->intparam[0] ) {

        case SET_SYS_AMASK:  case SET_TRUSTED_AMASK:  case SET_PROC_AMASK:
            ofs += sprintf_l ( &bp[ofs], "mask:        " );
            for ( i = 0; i < af->charlen[0]; i++ )
                ofs += sprintf_l ( &bp[ofs], "%02x", af->charparam[0][i]&0xff );
            ofs += sprintf_l ( &bp[ofs], "\n" );
            break;
        case SET_PROC_ACNTL:
            ofs += sprintf_l ( &bp[ofs], "flag:        0x%x\n", af->intparam[1] );
            break;
        case SET_AUDSWITCH:  case GET_AUDSWITCH:
            ofs += sprintf_l ( &bp[ofs], "audswitch:   %d\n", af->intparam[1] );
            break;
        case SETPAID:
            ofs += sprintf_l ( &bp[ofs], "audit_id:    %d\n", af->intparam[2] );
            break;
        case UPDEVENTS:
            if ( af->intparam[2] ) ofs += sprintf_l ( &bp[ofs], "audit_id:    %d\n", af->intparam[2] );
            if ( af->intparam[3] ) ofs += sprintf_l ( &bp[ofs], "process id:  %d\n", af->intparam[3] );
            if ( af->intparam[1] != -1 ) ofs += sprintf_l ( &bp[ofs], "cntl flag:   0x%x\n", af->intparam[1] );
            if ( af->charp_indx ) {
                ofs += sprintf_l ( &bp[ofs], "mask:        " );
                for ( i = 0; i < af->charlen[0]; i++ )
                    ofs += sprintf_l ( &bp[ofs], "%02x", af->charparam[0][i]&0xff );
                ofs += sprintf_l ( &bp[ofs], "\n" );
            }
            break;
        }
    break;

    /* chmod, fchmod */
    case SYS_chmod:     case SYS_fchmod:
        ofs += sprintf_l ( &bp[ofs], "mode:        %04o\n", af->intparam[0] );
        break;

    /* chown, fchown */
    case SYS_chown:     case SYS_fchown:
        ofs += sprintf_l ( &bp[ofs], "owner:       %d", af->intparam[0] );
        if ( (flag & FLAG_LOCALID) && (pw_ptr = getpwuid(af->intparam[0])) )
            ofs += sprintf_l ( &bp[ofs], " (%s)", pw_ptr->pw_name );
        ofs += sprintf_l ( &bp[ofs], "\ngroup:       %d", af->intparam[1] );
        if ( (flag & FLAG_LOCALID) && (gr_ptr = getgrgid(af->intparam[1])) )
            ofs += sprintf_l ( &bp[ofs], " (%s)", gr_ptr->gr_name );
        ofs += sprintf_l ( &bp[ofs], "\n" );
        break;

    /* creat, mknod, mkdir */
    case SYS_creat:     case SYS_mkdir:     case SYS_mknod:
        ofs += sprintf_l ( &bp[ofs], "mode:        %04o\n", af->intparam[0] );
        for ( i = 1; i < af->intp_indx; i++ )
            ofs += sprintf_l ( &bp[ofs], "int param:   %d\n", af->intparam[i] );
        break;

    /* exportfs - NOTE different value for SYS_exportfs on mips & vax */
    case 168: case 169:
        switch ( af->intparam[0] ) {
            case EXPORTFS_CREATE:
                ofs += sprintf_l ( &bp[ofs], "option:      CREATE   rootmap: %d   flags: 0x%x\n",
                    af->shortparam[0], af->intparam[2] );
                break;
            case EXPORTFS_REMOVE:
                ofs += sprintf_l ( &bp[ofs], "option:      REMOVE\n" );
                break;
            case EXPORTFS_READ:
                ofs += sprintf_l ( &bp[ofs], "option:      READ     cookie: 0x%x\n",
                    af->intparam[1] );
                break;
        }
        break;

    /* fcntl: only with F_DUPFD */
    case SYS_fcntl:
        ofs += sprintf_l ( &bp[ofs], "operation:   F_DUPFD\n" );
        break;

    /* ioctl: only with TIOCSTI */
    case SYS_ioctl:
        ofs += sprintf_l ( &bp[ofs], "operation:   TIOCSTI\n" );
        break;

    /* kill, killpg */
    case SYS_kill:      case SYS_killpg:
        ofs += sprintf_l ( &bp[ofs], "pid:         %d\n", af->intparam[0] );
        break;

    /* mmap, munmap */
    case SYS_mmap:
        ofs += sprintf_l ( &bp[ofs], "address:     0x%x   len: %d   prot: %04o\n",
            af->result, af->intparam[0], af->intparam[1] );
        break;
    case SYS_munmap:
        ofs += sprintf_l ( &bp[ofs], "address:     0x%x   len: %d\n",
            af->intparam[0], af->intparam[1] );
        break;

    /* msgop */
    case SYS_msgctl:
        ofs += sprintf_l ( &bp[ofs], "msqid:       %d  cmd: 0x%x\n",
            af->intparam[0], af->intparam[1] );
        break;
    case SYS_msgget:
        ofs += sprintf_l ( &bp[ofs], "msgflag:     0x%x\n", af->intparam[0] );
        if ( af->result != -1 )
            ofs += sprintf_l ( &bp[ofs], "msqid        %d\n", af->result );
        break;
    case SYS_msgrcv:    case SYS_msgsnd:
        ofs += sprintf_l ( &bp[ofs], "msqid:       %d\n", af->intparam[0] );
        break;

    /* open: only with O_CREAT option */
    case SYS_open:
        ofs += sprintf_l ( &bp[ofs], "flags:       %d :", af->intparam[0] );
        if ( af->intparam[0] == O_RDONLY ) ofs += sprintf_l ( &bp[ofs], " read" );
        if ( af->intparam[0] & O_WRONLY ) ofs += sprintf_l ( &bp[ofs], " write" );
        if ( af->intparam[0] & O_RDWR ) ofs += sprintf_l ( &bp[ofs], " rdwr" );
        if ( af->intparam[0] & O_TRUNC ) ofs += sprintf_l ( &bp[ofs], " trunc" );
        if ( af->intparam[0] & O_CREAT ) ofs += sprintf_l ( &bp[ofs], " creat" );
        ofs += sprintf_l ( &bp[ofs], "\n" );
        if ( (af->intparam[0] & O_CREAT) ) ofs += sprintf_l ( &bp[ofs], "mode:        %04o\n", af->intparam[1] );
        break;

    /* ptrace */
    case SYS_ptrace:
        ofs += sprintf_l ( &bp[ofs], "request      %d  pid: %d\n",
            af->intparam[0], af->intparam[1] );
        break;

    /*semctl */
    case SYS_semctl:
        ofs += sprintf_l ( &bp[ofs], "semid:       %d  cmd: 0x%x\n",
            af->intparam[0], af->intparam[1] );
        break;

    /* setpgrp */
    case SYS_setpgrp:
        ofs += sprintf_l ( &bp[ofs], "pid:         %d     pgrp: %d\n",
            af->intparam[0], af->intparam[1] );
        break;

    /* setregid */
    case SYS_setregid:
        ofs += sprintf_l ( &bp[ofs], "rgid:        %d", af->intparam[0] );
        if ( (flag & FLAG_LOCALID) && (gr_ptr = getgrgid(af->intparam[0])) )
            ofs += sprintf_l ( &bp[ofs], " (%s)", gr_ptr->gr_name );
        ofs += sprintf_l ( &bp[ofs], "\negid:        %d", af->intparam[1] );
        if ( (flag & FLAG_LOCALID) && (gr_ptr = getgrgid(af->intparam[1])) )
            ofs += sprintf_l ( &bp[ofs], " (%s)", gr_ptr->gr_name );
        ofs += sprintf_l ( &bp[ofs], "\n" );
        break;

    /* setreuid */
    case SYS_setreuid:
        ofs += sprintf_l ( &bp[ofs], "ruid:        %d", af->intparam[0] );
        if ( (flag & FLAG_LOCALID) && (pw_ptr = getpwuid(af->intparam[0])) )
            ofs += sprintf_l ( &bp[ofs], " (%s)", pw_ptr->pw_name );
        ofs += sprintf_l ( &bp[ofs], "\neuid:        %d", af->intparam[1] );
        if ( (flag & FLAG_LOCALID) && (pw_ptr = getpwuid(af->intparam[1])) )
            ofs += sprintf_l ( &bp[ofs], " (%s)", pw_ptr->pw_name );
        ofs += sprintf_l ( &bp[ofs], "\n" );
        break;

#ifdef PRIV
    /* setpriv */
    case SYS_setpriv:
        ofs += sprintf_l ( &bp[ofs], "priv oprtn:  " );
        if ( af->intparam[0] == 0x00 ) ofs += sprintf_l ( &bp[ofs], "P_SET_NULL  " );
        if ( af->intparam[0] & 0x001 ) ofs += sprintf_l ( &bp[ofs], "P_SET_PROC  " );
        if ( af->intparam[0] & 0x002 ) ofs += sprintf_l ( &bp[ofs], "P_SET_FILE  " );
        if ( af->intparam[0] & 0x004 ) ofs += sprintf_l ( &bp[ofs], "P_SET_FCTL  " );
        if ( af->intparam[0] & 0x008 ) ofs += sprintf_l ( &bp[ofs], "P_SET_DEBUG  " );
        if ( af->intparam[0] & 0x010 ) ofs += sprintf_l ( &bp[ofs], "P_SET_EFFECTIVE  " );
        if ( af->intparam[0] & 0x020 ) ofs += sprintf_l ( &bp[ofs], "P_SET_INHERITABLE  " );
        if ( af->intparam[0] & 0x040 ) ofs += sprintf_l ( &bp[ofs], "P_SET_PERMITTED  " );
        if ( af->intparam[0] & 0x080 ) ofs += sprintf_l ( &bp[ofs], "P_SET_INHERITED  " );
        ofs += sprintf_l ( &bp[ofs], "\n" );
        break;

    /* setsysinfo */
    case SYS_setsysinfo:
        if ( (af->intparam[0] >= 0) && (af->intparam[0] <= SSI_SET_PRIV) )
            ofs += sprintf_l ( &bp[ofs], "request:     %s\n", setsysinfo_req[af->intparam[0]-1] );
        else ofs += sprintf_l ( &bp[ofs], "request:     #%d\n", af->intparam[0] );

        switch ( af->intparam[0] ) {

        case SSI_SET_PRIV:
            ofs += sprintf_l ( &bp[ofs], "privs:       %s\n", af->intparam[1] ? "on" : "off" );
            break;

        }
        break;
#endif PRIV

    /* shmsys */
    case SYS_SHMGET:
        ofs += sprintf_l ( &bp[ofs], "shmflg:      0x%x\n", af->intparam[0] );
        if ( af->result != -1 )
            ofs += sprintf_l ( &bp[ofs], "shmid        %d\n", af->result );
        break;
    case SYS_SHMDT:
        ofs += sprintf_l ( &bp[ofs], "shmaddr:     0x%x\n", af->intparam[0] );
        break;
    case SYS_SHMCTL:
        ofs += sprintf_l ( &bp[ofs], "shmid:       %d  cmd: %d\n",
            af->intparam[0], af->intparam[1] );
        break;
    case SYS_SHMAT:
        ofs += sprintf_l ( &bp[ofs], "shmid:       %d  shmflg: 0x%x\n",
            af->intparam[0], af->intparam[2] );
        if ( af->result != -1 )
            ofs += sprintf_l ( &bp[ofs], "shmaddr      0x%x\n", af->result );
        break;

    /* umount */
    case SYS_umount:
        ofs += sprintf_l ( &bp[ofs], "device:      (%d,%d)\n",
            major(af->intparam[0]), minor(af->intparam[0]) );
        break;

    default:
        for ( i = 0; i < af->intp_indx; i++ ) ofs += sprintf_l ( &bp[ofs], "int param:   %d\n", af->intparam[i] );
        for ( i = 0; i < af->shortp_indx; i++ ) ofs += sprintf_l ( &bp[ofs], "short param: %d\n", af->shortparam[i] );

    }

    return ( ofs );
}


/* format audit record output into bp; return # chars output */
int output_rec_fmt ( bp, af, flag )
char *bp;
struct audit_fields af;
int flag;               /* FLAG_BRIEF */
{
    extern char *syscallnames[];
    extern int  sys_nerr;
    extern char *sys_errlist[];
    extern char *trustedevent[];
    char buf[MAXPATHLEN];
    struct a_proc *a_ptr;
    struct a_proc *p_ptr;
    int ofs = 0;
    struct passwd *pw_ptr;
    int i, j, k;

    /* flag represents length consistency check */
    if ( af.flag == 9 ) return ( ofs );
    else if ( af.flag == 8 ) {
        ofs += sprintf_l ( &bp[ofs], "\007\007NOTE -- partial audit record read\n" );
        return ( ofs );
    }
    else if ( af.flag == 7 ) ofs += sprintf_l ( &bp[ofs], "\007\007NOTE -- partial audit record recovered\n" );
    else if ( af.flag ) ofs += sprintf_l ( &bp[ofs], "\007\007WARNING -- audit record corrupted (%d)\n", af.flag );

    /* get a_proc struct for current process */
    a_ptr = aud_mem_proc ( 2, (struct a_proc *)0, af.pid, af.ipaddr, 0 );

    /* quick output */
    if ( flag & FLAG_BRIEF ) {
        ofs += output_brief_fmt ( &af, &bp[ofs], a_ptr, flag );
        return ( ofs );
    }

    /* output audit record header */
#ifdef debug1
    ofs += sprintf_l ( &bp[ofs], "cpu # = %d\n", af.n_cpu );
#endif debug1
    if ( af.auid > 0 ) ofs += sprintf_l ( &bp[ofs], "audit_id: %-10ld   ", af.auid );
    else if ( af.ruid != -1 ) ofs += sprintf_l ( &bp[ofs], "ruid: %-14d   ", af.ruid );
    if ( af.uid != -1 ) ofs += sprintf_l ( &bp[ofs], "uid: %-14d", af.uid );

    i = 0;
    if ( a_ptr != (struct a_proc *)-1 ) {
        if ( a_ptr->username ) {
            ofs += sprintf_l ( &bp[ofs], "username: %s", a_ptr->username );
            i = 1;
        }
    }
    else {  /* used to catch username for exit()'s */
        p_ptr = aud_mem_proc ( 2, (struct a_proc *)0, af.ppid, af.ipaddr, 0 );
        if ( p_ptr != (struct a_proc *)-1 )
            if ( p_ptr->username && af.auid == p_ptr->auid ) {
                ofs += sprintf_l ( &bp[ofs], "username: %s", p_ptr->username );
                i = 1;
            }
    }
    /* no username found; try passwd file */
    if ( (flag & FLAG_LOCALID) && (i == 0) && (pw_ptr = getpwuid ( af.ruid )) )
        ofs += sprintf_l ( &bp[ofs], "(username: %s)", pw_ptr->pw_name );

    ofs += sprintf_l ( &bp[ofs], "\n" );
    if ( af.pid != -1 ) ofs += sprintf_l ( &bp[ofs], "pid: %-10d        ", af.pid );
    if ( af.ppid != -1 ) ofs += sprintf_l ( &bp[ofs], "ppid: %-10d   ", af.ppid );
    if ( af.device != -1 ) ofs += sprintf_l ( &bp[ofs], "dev: (%d,%d)", major(af.device), minor(af.device) );
    ofs += sprintf_l ( &bp[ofs], "\n" );

    /* output event */
    if ( af.event != -1 ) {
        if ( af.event == SYS_exit && af.login_proc )
            ofs += sprintf_l ( &bp[ofs], "event:       %s (%s)\n", syscallnames[af.event], _LOGOUT );
        else if ( af.event >= 0 && af.event <= NUM_SYSCALLS )
            ofs += sprintf_l ( &bp[ofs], "event:       %s\n", syscallnames[af.event] );
        else if ( af.event >= MIN_TRUSTED_EVENT && af.event < MIN_TRUSTED_EVENT + N_TRUSTED_EVENTS )
            ofs += sprintf_l ( &bp[ofs], "event:       %s\n", trustedevent[af.event-MIN_TRUSTED_EVENT] );
        else if ( af.event >= MIN_SPECIAL && af.event <= MAX_SPECIAL )
            ofs += sprintf_l ( &bp[ofs], "event:       %s\n", special_event[af.event-MIN_SPECIAL] );
        else ofs += sprintf_l ( &bp[ofs], "event: UNKNOWN EVENT %d\n", af.event );
    }

    /* output login specific information */
    for ( i = 0; i < af.login_indx; i++ ) {
        j = af.login_len[i] < MAXPATHLEN-1 ? af.login_len[i] : MAXPATHLEN-1;
        bcopy ( af.login[i], buf, j );
        buf[j] = '\0';
        ofs += sprintf_l ( &bp[ofs], "login name:  %s\n", buf );
    }
    for ( i = 0; i < af.homedir_indx; i++ ) {
        j = af.homedir_len[i] < MAXPATHLEN-1 ? af.homedir_len[i] : MAXPATHLEN-1;
        bcopy ( af.homedir[i], buf, j );
        buf[j] = '\0';
        ofs += sprintf_l ( &bp[ofs], "home dir:    %s\n", buf );
    }
    for ( i = 0; i < af.shell_indx; i++ ) {
        j = af.shell_len[i] < MAXPATHLEN-1 ? af.shell_len[i] : MAXPATHLEN-1;
        bcopy ( af.shell[i], buf, j );
        buf[j] = '\0';
        ofs += sprintf_l ( &bp[ofs], "shell:       %s\n", buf );
    }
    for ( i = 0; i < af.service_indx; i++ ) {
        j = af.service_len[i] < MAXPATHLEN-1 ? af.service_len[i] : MAXPATHLEN-1;
        bcopy ( af.service[i], buf, j );
        buf[j] = '\0';
        ofs += sprintf_l ( &bp[ofs], "service:     %s\n", buf );
    }
    for ( i = 0; i < af.devname_indx; i++ ) {
        j = af.devname_len[i] < MAXPATHLEN-1 ? af.devname_len[i] : MAXPATHLEN-1;
        bcopy ( af.devname[i], buf, j );
        buf[j] = '\0';
        ofs += sprintf_l ( &bp[ofs], "devname:     %s\n", buf );
    }

    /* output secondary/remote identification data */
    if ( af.login2_indx > 0 || af.auid2 > 0 || af.ruid2 != -1 || af.uid2 != -1
    || af.hostid2 != -1 || af.ipaddr2 || af.pid2 != -1 || af.ppid2 != -1 || af.device2 != -1 ) {
        ofs += sprintf_l ( &bp[ofs], "........... \nremote/secondary identification data --\n" );

        for ( i = 0; i < af.login2_indx; i++ ) {
            j = af.login2_len[i] < MAXPATHLEN-1 ? af.login2_len[i] : MAXPATHLEN-1;
            bcopy ( af.login2[i], buf, j );
            buf[j] = '\0';
            ofs += sprintf_l ( &bp[ofs], "login name:  %s\n", buf );
        }
        if ( af.auid2 > 0 ) ofs += sprintf_l ( &bp[ofs], "audit_id: %-10ld   ", af.auid2 );
        else if ( af.ruid2 != -1 ) ofs += sprintf_l ( &bp[ofs], "ruid: %-14d   ", af.ruid2 );
        if ( af.uid2 != -1 ) ofs += sprintf_l ( &bp[ofs], "uid: %-14d", af.uid2 );
        if ( af.hostid2 != -1 ) ofs += sprintf_l ( &bp[ofs], "hostid(2): %-8x", af.hostid2 );
        if ( af.auid2 != -1 || af.ruid2 != -1 || af.uid2 != -1 || af.hostid2 != -1 )
            ofs += sprintf_l ( &bp[ofs], "\n" );
        if ( af.ipaddr2 ) {
            ofs += sprintf_l ( &bp[ofs], "ip_addr:     %s", inet_ntoa(af.ipaddr2) );
            ofs += sprintf_l ( &bp[ofs], " (%s)\n", gethost_l(af.ipaddr2) );
        }
        if ( af.pid2 != -1 ) ofs += sprintf_l ( &bp[ofs], "pid: %-10d        ", af.pid2 );
        if ( af.ppid2 != -1 ) ofs += sprintf_l ( &bp[ofs], "ppid: %-10d   ", af.ppid2 );
        if ( af.device2 != -1 ) ofs += sprintf_l ( &bp[ofs], "dev: (%d,%d)", major(af.device2), minor(af.device2) );
        if ( af.pid2 != -1 || af.ppid2 != -1 || af.device2 != -1 ) ofs += sprintf_l ( &bp[ofs], "\n" );

        ofs += sprintf_l ( &bp[ofs], "...........\n" );
    }

    /* output string parameters */
    /* deal with SYS_audcntl in output_param */
    for ( i = 0; i < af.charp_indx && af.event != SYS_audcntl; i++ ) {
        if ( af.charlen[i] == 0 ) continue;
        j = af.charlen[i] < MAXPATHLEN-1 ? af.charlen[i] : MAXPATHLEN-1;
        bcopy ( af.charparam[i], buf, j );
        buf[j] = '\0';
        ofs += sprintf_l ( &bp[ofs], "char param:  %s\n", buf );
    }

    /* output gnode information */
    for ( i = 0; (i < af.gp_id_indx || i < af.gp_dev_indx)
    && (af.error != ENOENT) && (af.error != ENXIO); i++ ) {
        if ( i < af.gp_id_indx )  ofs += sprintf_l ( &bp[ofs], "gnode id:    %d  \t", af.gnode_id[i] );
        if ( i < af.gp_dev_indx )
            ofs += sprintf_l ( &bp[ofs], "gnode dev:   (%d,%d)", major(af.gnode_dev[i]), minor(af.gnode_dev[i]) );
        ofs += sprintf_l ( &bp[ofs], "\n" );
    }

    /* output descriptor information */
    for ( i = 0; (i < af.descrip_indx) && (af.error == 0); i++ ) {
        if ( (a_ptr != (struct a_proc *)-1) && (af.descrip[i] >= 0) && (af.descrip[i] < _NFILE) ) {
            if ( a_ptr->fd_nm[af.descrip[i]] )
                ofs += sprintf_l ( &bp[ofs], "descriptor:  %s (%d)\n", a_ptr->fd_nm[af.descrip[i]], af.descrip[i] );
            else if ( (af.event == SYS_close || af.event == SYS_dup2) && *close_buf )
                ofs += sprintf_l ( &bp[ofs], "descriptor:  %s (%d)\n", close_buf, af.descrip[i] );
            else ofs += sprintf_l ( &bp[ofs], "descriptor:  %d\n", af.descrip[i] );
        }
        else ofs += sprintf_l ( &bp[ofs], "descriptor:  %d\n", af.descrip[i] );
    }
    close_buf[0] = '\0';

    /* output current directory and root */
    if ( a_ptr != (struct a_proc *)-1 )
        if ( af.descrip_indx || af.charp_indx ) {
            if ( a_ptr->cwd )
                ofs += sprintf_l ( &bp[ofs], "directory:   %s\n", a_ptr->cwd );
            if ( a_ptr->root && bcmp ( a_ptr->root, "/", 2 ) )
                ofs += sprintf_l ( &bp[ofs], "root dir:    %s\n", a_ptr->root );
        }

    /* output event-specific parameters */
    ofs += output_param_fmt ( &af, &bp[ofs] );

    /* output integer array parameters; special case SYS_setgroups */
    if ( (af.event == SYS_setgroups) && (flag & FLAG_LOCALID) )
        ofs += output_grp_fmt ( &af, &bp[ofs] );
    else for ( i = 0; i < af.int_array_indx; i++ ) {
        ofs += sprintf_l ( &bp[ofs], "int array:   " );
        for ( j = 0; j < af.int_array_len[i]; j += sizeof(int) ) {
            bcopy ( af.int_array[i]+j, &k, sizeof(int) );
            ofs += sprintf_l ( &bp[ofs], "%d ", k );
        }
        ofs += sprintf_l ( &bp[ofs], "\n" );
    }

    /* output ipc information */
    ofs += output_ipc_fmt ( &af, &bp[ofs], a_ptr );

    /* output X information */
    ofs += output_x_fmt ( &af, &bp[ofs] );

    /* output result, ipaddr, priv, timestamp */
    if ( af.error && (af.event <= NUM_SYSCALLS || (af.event >= SYS_SHMGET && af.event < SYS_SHMAT)) ) {
        if ( (af.error < 0) || (af.error >= sys_nerr) ) af.flag = 7;
        else ofs += sprintf_l ( &bp[ofs], "error:       %s (%d)\n", sys_errlist[af.error], af.error );
    }
    else if ( af.error ) ofs += sprintf_l ( &bp[ofs], "error:       %d\n", af.error );
    else if ( af.result != -1 ) {
        /* exit() return 8-bit quantity */
        if ( af.event == SYS_exit && af.result > 127 ) af.result |= 0xffffff00;
        else ofs += sprintf_l ( &bp[ofs], "result:      %d\n", af.result );
    }
    if ( af.hostid != -1 ) ofs += sprintf_l ( &bp[ofs], "hostid:      %-8x", af.hostid );
    if ( af.ipaddr ) {
        ofs += sprintf_l ( &bp[ofs], "ip_addr:     %s", inet_ntoa(af.ipaddr) );
        ofs += sprintf_l ( &bp[ofs], " (%s)", gethost_l(af.ipaddr) );
    }
    if ( af.hostid != -1 || af.ipaddr ) ofs += sprintf_l ( &bp[ofs], "\n" );

#ifdef PRIV
    /* output priv structure */
    if ( (af.privstr.mask[0]|af.privstr.mask[1]) || (af.event == SYS_setpriv) ) {
#ifdef PRIVDB
        if ( getprivname ( &af.privstr, buf, MAXPATHLEN ) > 0 )
            ofs += sprintf_l ( &bp[ofs], "privs:       %s\n", buf ); 
        else
#endif PRIVDB
        ofs += sprintf_l ( &bp[ofs], "privs (hex): 0x%x 0x%x\n", af.privstr.mask[0], af.privstr.mask[1] );
    }
#endif PRIV

    ofs += sprintf_l ( &bp[ofs], "timestamp:   %.19s.%02d%.5s %s\n\n\n",
    ctime(&af.timeval), (int)(af.timeval.tv_usec/10000.0), ctime(&af.timeval)+19,
    timezone(af.timezone.tz_minuteswest, af.timezone.tz_dsttime) );

    return ( ofs );
}


/* format X information output into bp; return # chars output */
int output_x_fmt ( af, bp )
struct audit_fields *af;
char *bp;
{
    int ofs = 0;
    int i;

    for ( i = 0; i < af->atom_id_indx; i++ ) ofs += sprintf_l ( &bp[ofs], "atom_id:     %d\n", af->atom_id[i] );
    for ( i = 0; i < af->client_id_indx; i++ ) ofs += sprintf_l ( &bp[ofs], "client_id:   0x%x\n", af->client_id[i] );
    for ( i = 0; i < af->property_indx; i++ ) ofs += sprintf_l ( &bp[ofs], "property:    %d\n", af->property[i] );

    for ( i = 0; i < af->res_id_indx; i++ ) ofs += sprintf_l ( &bp[ofs], "res_id:   0x%x\n", af->res_id[i] );
    for ( i = 0; i < af->res_class_indx; i++ ) ofs += sprintf_l ( &bp[ofs], "res_class:   %d\n", af->res_class[i] );
    for ( i = 0; i < af->res_type_indx; i++ ) {
        if ( af->res_class[i] == 0 && af->res_type[i] == 8 )
            ofs += sprintf_l ( &bp[ofs], "res_type:   pixmap\n" );
        else if ( af->res_class[i] == 0 && af->res_type[i] == 16 )
            ofs += sprintf_l ( &bp[ofs], "res_type:   window\n" );
        else ofs += sprintf_l ( &bp[ofs], "res_type:   %d\n", af->res_type[i] );
    }

    /* output X aud_client_info structure */
    for ( i = 0; i < af->x_client_indx; i++ ) {
        ofs += sprintf_l ( &bp[ofs], "client_host: 0x%x\n", af->x_client[i].hostid );
        ofs += sprintf_l ( &bp[ofs], "client_audit_id: %ld   client_ruid: 0x%x   client_uid: %d\n",
        af->x_client[i].auid, af->x_client[i].ruid, af->x_client[i].uid );
        ofs += sprintf_l ( &bp[ofs], "client_pid: %d   client_ppid: %d\n",
        af->x_client[i].pid, af->x_client[i].ppid );
    }

    return ( ofs );
}


/* parse audit record */
parse_rec ( rec_ptr, rec_len, af )
char *rec_ptr;              /* ptr to audit record      */
int rec_len;                /* length of audit record   */
struct audit_fields *af;    /* audit fields struct      */
{
    char token;
    int *intp;
    long *longp;
    short *shortp;
    struct aud_client_info *aci_p;
#ifdef PRIV
    struct priv_set_t *priv_p;
#endif PRIV
    char ch;
    int i, j;

#define PARSE_DEF(field,ptr) \
    bcopy ( &rec_ptr[i+sizeof token], &field, sizeof *ptr ); \
    i += (sizeof token + sizeof *ptr);

#define PARSE_DEF_P(len,field,indx,type) \
    bcopy ( &rec_ptr[i+sizeof token], &len[indx], sizeof(int) ); \
    field[indx] = (type *)&rec_ptr[i+(sizeof token)+(sizeof *intp)]; \
    i += (sizeof token + sizeof *intp + len[indx]); \
    indx++;

    /* parse audit record (5 bytes read elsewhere for length) */
    for ( i = sizeof rec_len + sizeof token; i < rec_len-5; ) {
        ch = rec_ptr[i] & 0x7f; /* use high-order bit for token value length */

        if      ( ch == T_EVENT )           { PARSE_DEF ( af->event, intp ); }
        else if ( ch == T_ERROR )           { PARSE_DEF ( af->error, intp ); }
        else if ( ch == T_RESULT )          { PARSE_DEF ( af->result, intp ); }
        else if ( ch == T_AUID )            { PARSE_DEF ( af->auid, longp ); }
        else if ( ch == (T_UID&0x7f) )      { PARSE_DEF ( af->uid, shortp ); }
        else if ( ch == (T_RUID&0x7f) )     { PARSE_DEF ( af->ruid, shortp ); }
        else if ( ch == T_HOSTADDR )        { PARSE_DEF ( af->ipaddr, intp ); }
        else if ( ch == T_HOSTID )          { PARSE_DEF ( af->hostid, intp ); }
        else if ( ch == (T_PID&0x7f) )      { PARSE_DEF ( af->pid, shortp ); }
        else if ( ch == (T_PPID&0x7f) )     { PARSE_DEF ( af->ppid, shortp ); }
        else if ( ch == (T_DEV&0x7f) )      { PARSE_DEF ( af->device, shortp ); }
        else if ( ch == (T_NCPU&0x7f) )     { PARSE_DEF ( af->n_cpu, shortp ); }
#ifdef PRIV
        else if ( ch == T_PRIVSTR )         { PARSE_DEF ( af->privstr, priv_p ); }
#endif PRIV

        else if ( ch == T_AUID2 )           { PARSE_DEF ( af->auid2, longp ); }
        else if ( ch == (T_UID2&0x7f) )     { PARSE_DEF ( af->uid2, shortp ); }
        else if ( ch == (T_RUID2&0x7f) )    { PARSE_DEF ( af->ruid2, shortp ); }
        else if ( ch == T_HOSTADDR2 )       { PARSE_DEF ( af->ipaddr2, intp ); }
        else if ( ch == T_HOSTID2 )         { PARSE_DEF ( af->hostid2, intp ); }
        else if ( ch == (T_PID2&0x7f) )     { PARSE_DEF ( af->pid2, shortp ); }
        else if ( ch == (T_PPID2&0x7f) )    { PARSE_DEF ( af->ppid2, shortp ); }
        else if ( ch == (T_DEV2&0x7f) )     { PARSE_DEF ( af->device2, shortp ); }

        else if ( ch == T_INT )             { PARSE_DEF ( af->intparam[af->intp_indx++], intp ); }
        else if ( ch == (T_SHORT&0x7f) )    { PARSE_DEF ( af->shortparam[af->shortp_indx++], shortp ); }
        else if ( ch == T_DESCRIP )         { PARSE_DEF ( af->descrip[af->descrip_indx++], intp ); }
        else if ( ch == (T_GNODE_DEV&0x7f)) { PARSE_DEF ( af->gnode_dev[af->gp_dev_indx++], shortp ); }
        else if ( ch == T_GNODE_ID )        { PARSE_DEF ( af->gnode_id[af->gp_id_indx++], intp ); }

        else if ( ch == T_TV_SEC )          { PARSE_DEF ( af->timeval.tv_sec, longp ); }
        else if ( ch == T_TV_USEC )         { PARSE_DEF ( af->timeval.tv_usec, longp ); }
        else if ( ch == T_TZ_MIN )          { PARSE_DEF ( af->timezone.tz_minuteswest, intp ); }
        else if ( ch == T_TZ_DST )          { PARSE_DEF ( af->timezone.tz_dsttime, intp ); }

        else if ( ch == (T_IPC_UID&0x7f) )  { PARSE_DEF ( af->ipc_uid[af->ipc_uid_indx++], shortp ); }
        else if ( ch == (T_IPC_GID&0x7f) )  { PARSE_DEF ( af->ipc_gid[af->ipc_gid_indx++], shortp ); }
        else if ( ch == (T_IPC_MODE&0x7f) ) { PARSE_DEF ( af->ipc_mode[af->ipc_mode_indx++], shortp ); }

        else if ( ch == T_X_ATOM )          { PARSE_DEF ( af->atom_id[af->atom_id_indx++], intp ); }
        else if ( ch == T_X_CLIENT )        { PARSE_DEF ( af->client_id[af->client_id_indx++], intp ); }
        else if ( ch == T_X_PROPERTY )      { PARSE_DEF ( af->property[af->property_indx++], intp ); }

        else if ( ch == T_X_CLIENT_INFO )   { PARSE_DEF ( af->x_client[af->x_client_indx++], aci_p ); }
        else if ( ch == T_X_RES_CLASS )     { PARSE_DEF ( af->res_class[af->res_class_indx++], intp ); }
        else if ( ch == T_X_RES_TYPE )      { PARSE_DEF ( af->res_type[af->res_type_indx++], intp ); }
        else if ( ch == T_X_RES_ID )        { PARSE_DEF ( af->res_id[af->res_id_indx++], intp ); }

        else if ( ch == T_INTP )            { PARSE_DEF_P ( af->int_array_len, af->int_array, af->int_array_indx, char ); }
        else if ( ch == T_CHARP )           { PARSE_DEF_P ( af->charlen, af->charparam, af->charp_indx, char ); }
        else if ( ch == T_SOCK )            { PARSE_DEF_P ( af->socketlen, af->socketaddr, af->socket_indx, char ); }
        else if ( ch == T_MSGHDR )          { PARSE_DEF_P ( af->msglen, af->msgaddr, af->msg_indx, char ); }
        else if ( ch == T_ACCRGHT )         { PARSE_DEF_P ( af->accesslen, af->accessaddr, af->access_indx, char ); }

        else if ( ch == T_LOGIN )           { PARSE_DEF_P ( af->login_len, af->login, af->login_indx, char ); }
        else if ( ch == T_LOGIN2 )          { PARSE_DEF_P ( af->login2_len, af->login2, af->login2_indx, char ); }
        else if ( ch == T_HOMEDIR )         { PARSE_DEF_P ( af->homedir_len, af->homedir, af->homedir_indx, char ); }
        else if ( ch == T_SHELL )           { PARSE_DEF_P ( af->shell_len, af->shell, af->shell_indx, char ); }
        else if ( ch == T_SERVICE )         { PARSE_DEF_P ( af->service_len, af->service, af->service_indx, char ); }
        else if ( ch == T_DEVNAME )         { PARSE_DEF_P ( af->devname_len, af->devname, af->devname_indx, char ); }

        /* token length field */
        else if ( ch == T_LENGTH ) i += 5;

        /* shit catcher */
        else {
            fprintf ( stderr, "\007WARNING -- unknown value 0x%x @ byte offset %d\n", ch, i );
            i += sizeof token;
        }

    }
}


/* recover from records being split across logs; return af.event/0/-1 */
int recover ( op, buf, rec_len, fd_p )
int op;         /* operation            */
char *buf;      /* audit data           */
int rec_len;    /* record length        */
int *fd_p;      /* data file descriptor */
{
    static char bufsav[AUD_BUF_SIZ];    /* used for split records */
    static int sav_p = 0;               /* offset into bufsav     */
    struct audit_fields af;
    char *ptr;
    int posn;
    int len;
    int i, j;

    /* store last partial record; update fd_p */
    if ( op == 0 ) {
        for ( i = 1; buf[i] != T_LENGTH && i < rec_len && i < AUD_BUF_SIZ; i++ );
        for ( sav_p = 0; sav_p < i; sav_p++ ) bufsav[sav_p] = buf[sav_p];
        lseek ( *fd_p, i-rec_len, L_INCR );

        /* check that next record is an AUDIT_LOG_CHANGE record */
        posn = tell ( *fd_p );
        ptr = fetch_rec ( fd_p, &len, &af, flag&FLAG_FOLLOW, 0 );
        lseek ( *fd_p, posn, L_SET );
        init_audit_fields ( &af );
        parse_rec ( ptr, len, &af );
        return ( af.event );
    }

    /* recover last partial record */
    if ( op == 1 ) {
        lseek ( *fd_p, -1, L_INCR );
        bcopy ( bufsav, buf, sav_p );

        /* get enough data to get record length */
        for ( i = sav_p; i < 1+sizeof rec_len; i++ )
            read ( *fd_p, buf[i], 1 );
        bcopy ( &buf[1], &rec_len, sizeof rec_len );

        /* read data */
        if ( (rec_len < 0) || (rec_len >= AUD_BUF_SIZ) ) {
            af.flag += 4;
            sav_p = 0;
            return(-1);
        };
        i = rec_len - (sizeof rec_len + sizeof *ptr);
        j = sav_p - (sizeof rec_len + sizeof *ptr);
        do {
            j += read ( *fd_p, &buf[sizeof rec_len + sizeof *ptr + j], i-j );
        } while ( (j < i) && flag&FLAG_FOLLOW );
        sav_p = 0;
        if ( j < i ) return(-1);

        /* consistency check */
        bcopy ( &buf[rec_len - (sizeof rec_len)], &i, sizeof(int) );
        if ( i != rec_len ) {
            af.flag += 5;
            return(-1);
        }
        return(0);
    }
}


/* catch sig_int's */
sig_int1()
{
    char buf[2];
    printf ( "\n--interrupt:  exit (y/n)?  " );
    fflush ( stdout );
    read ( 0, buf, 2 );

#ifdef debug1
    if ( buf[0] == '?' ) {
        printf ( "\n\n--debug mode: blk_maps:\n" );
        aud_mem_op ( 0, (char *)0, 0, 1 );
        printf ( "\n\n--debug mode: proc structures:\n" );
        aud_mem_proc ( 3, (struct a_proc *)0, 0, 0, 0 );
    }
#endif debug1

    if ( buf[0] == '1' || buf[0] == 'y' || buf[0] == 'Y' ) {
        compress ( 1, (char *)0 );
        exit(0);
    }
    printf ( "\n\n--interactive mode--\n" );
    interact ( &selectn, &flag );
}


/* local sprintf() routine; return # chars copied */
int sprintf_l ( str, fmt, va_alist )
char *str;
char *fmt;
#ifdef mips
va_dcl
#endif mips
{
    FILE strbuf;
    int i;
#ifdef vax
#define ARGS &va_alist
#endif vax
#ifdef mips
#define ARGS ap
    va_list ap;
    va_start(ap);
#endif mips

    strbuf._flag = _IOWRT|_IOSTRG;
    strbuf._base = strbuf._ptr = str;
    strbuf._cnt = 32767;

    (void)_doprnt ( fmt, ARGS, &strbuf );
    *strbuf._ptr = '\0';

    for ( i = 0; str[i]; i++ );
    return(i);
}


/* maintain process state */
state_maint ( af )
struct audit_fields *af;    /* audit record fields  */
{
#ifndef SYS_vfork
#define SYS_vfork 66
#endif SYS_vfork
    union {
        struct sockaddr sockbuf;
        char buf[MAXPATHLEN];
    } sock_un;
    struct sockaddr *sockptr;
    char *sockname;
    struct a_proc *a_ptr, *a_ptr2;
    int i, j, k, l;

    if ( af->error ) return;

    switch ( af->event ) {

    /* free process' resources */
    case SYS_exit:
        a_ptr = aud_mem_proc ( 2, (struct a_proc *)0, af->pid, af->ipaddr, 0 );
        if ( (a_ptr != (struct a_proc *)-1) && a_ptr->login_proc ) af->login_proc = 1;
        state_maint_close ( 0, af->pid, af->ppid, af->ipaddr, 0 );
        break;

    /* new descriptor */
    case SYS_creat:
    case SYS_open:
        state_maint_add ( af->pid, af->ppid, af->ipaddr, af->charlen[0], af->charparam[0], af->result, -1, -1 );
        break;

    /* manipulate descriptors */
    case SYS_close:
        state_maint_close ( 1, af->pid, af->ppid, af->ipaddr, af->descrip[0] );
        break;
    case SYS_dup:
    case SYS_fcntl:
        state_maint_add ( af->pid, af->ppid, af->ipaddr, 0, (char *)0, af->result, af->descrip[0], -1 );
        break;
    case SYS_dup2:
        state_maint_close ( 1, af->pid, af->ppid, af->ipaddr, af->descrip[1] );
        state_maint_add ( af->pid, af->ppid, af->ipaddr, 0, (char *)0, af->descrip[1], af->descrip[0], -1 );
        break;

    /* update path */
    case SYS_chdir:
    case SYS_chroot:
        state_maint_path_change ( af->pid, af->ppid, af->ipaddr, af->charparam[0], af->event );
        break;

    /* sockets */
    case SYS_bind:
    case SYS_connect:
        j = af->socketlen[0] < MAXPATHLEN-1 ? af->socketlen[0] : MAXPATHLEN-1;
        bcopy ( af->socketaddr[0], sock_un.buf, j );
        sock_un.buf[j] = '\0';
        sockptr = (struct sockaddr *)&sock_un.sockbuf;

        switch ( sockptr->sa_family ) {
        case AF_UNIX:
            sockname = sockptr->sa_data;
            break;
        case AF_INET:
            sockname = itoa(ntohs(((struct sockaddr_in *)sockptr)->sin_port));
            break;
        default:
            sockname = sockptr->sa_data;
            break;
        }

        for ( j = 0; sockname[j]; j++ );
        state_maint_add ( af->pid, af->ppid, af->ipaddr, j+1, sockname, af->descrip[0], -1, -1 );
        break;
    case SYS_accept:
        state_maint_add ( af->pid, af->ppid, af->ipaddr, 0, (char *)0, af->result, af->descrip[0], -1 );
        break;

    /* login event */
    case LOGIN:
        state_maint_path_change ( af->pid, af->ppid, af->ipaddr, af->homedir[0], SYS_chdir );
        state_maint_add ( af->pid, af->ppid, af->ipaddr, af->login_len[0], af->login[0], -1, -1, 1 );
        a_ptr = aud_mem_proc ( 2, (struct a_proc *)0, af->pid, af->ipaddr, 0 );
        if ( a_ptr != (struct a_proc *)-1 ) a_ptr->login_proc = 1;
        break;

    }

    /* username comes from a_ptr, or p_ptr (if auid's match) */
    a_ptr = aud_mem_proc ( 2, (struct a_proc *)0, af->pid, af->ipaddr, 0 );
    if ( a_ptr == (struct a_proc *)-1 ) {
        state_maint_open ( af->pid, af->ppid, af->ipaddr );
        a_ptr = aud_mem_proc ( 2, (struct a_proc *)0, af->pid, af->ipaddr, 0 );
    }
    if ( a_ptr != (struct a_proc *)-1 ) a_ptr->auid = af->auid;
}


/* add new file descriptor->pathname translation or username (login event) to a_proc */
state_maint_add ( pid, ppid, ipaddr, fetch_siz, name, fd, old_fd, login )
short pid;
short ppid;
long ipaddr;
int fetch_siz;
char *name;
int fd;
int old_fd;
int login;
{
    struct a_proc *a_ptr;
    char *f_ptr;
    int i;

    /* fd check */
    if ( (fd < 0 || fd >= _NFILE) && login == -1 ) return;

    /* fetch a_proc structure */
    a_ptr = aud_mem_proc ( 2, (struct a_proc *)0, pid, ipaddr, 0 );
    if ( a_ptr == (struct a_proc *)-1 ) {
#ifdef debug2
        fprintf ( stderr, "(state_maint_add: no a_proc for pid %d ipaddr 0x%x; creating one)\n", pid, ipaddr );
#endif debug2
        state_maint_open ( pid, ppid, ipaddr );
        a_ptr = aud_mem_proc ( 2, (struct a_proc *)0, pid, ipaddr, 0 );
        if ( a_ptr == (struct a_proc *)-1 ) return;
    }

    /* fetch space for name; put in a_proc */
    if ( fetch_siz > 0 ) {
        f_ptr = aud_mem_op ( fetch_siz, (char *)0, 0, 0 );
        if ( f_ptr == (char *)0 ) {
            fprintf ( stderr, "state_maint_add: no more mem; could not create field entry for pid %d\n", pid );
            return;
        }
        bcopy ( name, f_ptr, fetch_siz );
        if ( fd >= 0 && fd < _NFILE ) a_ptr->fd_nm[fd] = f_ptr;
        else if ( login > 0 ) a_ptr->username = f_ptr;
    }

    /* dup old_fd to fd */
    if ( old_fd >= 0 && old_fd <= _NFILE && a_ptr->fd_nm[old_fd] ) {
        for ( i = 0; a_ptr->fd_nm[old_fd][i] && i < MAXPATHLEN; i++ );
        f_ptr = aud_mem_op ( i+1, (char *)0, 0, 0 );
        if ( f_ptr == (char *)0 ) {
            fprintf ( stderr, "state_maint_add: no more mem; could not create field entry for pid %d\n", pid );
            return;
        }
        bcopy ( a_ptr->fd_nm[old_fd], f_ptr, i+1 );
        a_ptr->fd_nm[fd] = f_ptr;
    }
}


/* free resources associated with process' state */
state_maint_close ( op, pid, ppid, ipaddr, fd )
int op;         /* 0: release all resources; 1: free fd only */
short pid;
short ppid;
long ipaddr;
int fd;
{
    struct a_proc *ptr;
    int i, j;

    /* fetch a_proc structure */
    ptr = aud_mem_proc ( 2, (struct a_proc *)0, pid, ipaddr, 0 );
    if ( ptr == (struct a_proc *)-1 ) {
#ifdef debug2
        fprintf ( stderr, "(state_maint_close: no a_proc for pid %d ipaddr 0x%x)\n", pid, ipaddr );
#endif debug2
        state_maint_open ( pid, ppid, ipaddr );
        ptr = aud_mem_proc ( 2, (struct a_proc *)0, pid, ipaddr, 0 );
        if ( ptr == (struct a_proc *)-1 ) return;
    }

    /* release single descriptor */
    if ( op == 1 ) {
        if ( fd >= 0 && fd < _NFILE ) {
            if ( ptr->fd_nm[fd] ) {
                for ( j = 0; ptr->fd_nm[fd][j]; j++ );
                bcopy ( ptr->fd_nm[fd], close_buf, j );
                close_buf[j] = '\0';
                aud_mem_op ( 0, ptr->fd_nm[fd], j+1, 0 );
                ptr->fd_nm[fd] = '\0';
            }
        }
        return;
    }

    /* release descriptors */
    for ( i = 0; i < _NFILE; i++ )
        if ( fd >= 0 && fd < _NFILE )
            if ( ptr->fd_nm[i] ) {
                for ( j = 0; ptr->fd_nm[i][j]; j++ );
                aud_mem_op ( 0, ptr->fd_nm[i], j+1, 0 );
                ptr->fd_nm[fd] = '\0';
            }

    /* release cwd, root, and username */
    if ( ptr->cwd ) {
        for ( j = 0; ptr->cwd[j]; j++ );
        aud_mem_op ( 0, ptr->cwd, j+1, 0 );
        ptr->cwd = '\0';
    }
    if ( ptr->root ) {
        for ( j = 0; ptr->root[j]; j++ );
        aud_mem_op ( 0, ptr->root, j+1, 0 );
        ptr->root = '\0';
    }
    if ( ptr->username ) {
        for ( j = 0; ptr->username[j]; j++ );
        aud_mem_op ( 0, ptr->username, j+1, 0 );
        ptr->username = '\0';
    }

    /* release a_proc structure */
    aud_mem_proc ( 0, ptr, pid, ipaddr, 0 );
}


/* establish new a_proc structure for <pid,ipaddr> */
state_maint_open ( pid, ppid, ipaddr )
short pid;
short ppid;
long ipaddr;
{
    struct a_proc *a_ptr;
    struct a_proc *p_ptr;
    char *c_ptr;
    int i, j;

    /* get a_proc structure for <pid,ipaddr> */
    if ( (a_ptr = aud_mem_proc ( 1, (struct a_proc *)0, pid, ipaddr, 0 )) == (struct a_proc *)-1 ) {
        fprintf ( stderr, "state_maint_open: no more mem; could not create a_proc for %d\n", pid );
        return;
    }

    /* fetch a_proc structure for ppid; load fd's from ppid to pid */
    if ( (p_ptr = aud_mem_proc ( 2, (struct a_proc *)0, ppid, ipaddr, 0 )) == (struct a_proc *)-1 )
        return;
    for ( i = 0; i < _NFILE; i++ )
        if ( p_ptr->fd_nm[i] ) {
            for ( j = 0; p_ptr->fd_nm[i][j] && j < MAXPATHLEN; j++ );
            if ( (a_ptr->fd_nm[i] = aud_mem_op ( j+1, (char *)0, 0, 0 )) == (char *)0 )
                fprintf ( stderr, "state_maint_open: no more mem; could not add fd field for %d\n", pid );
            else bcopy ( p_ptr->fd_nm[i], a_ptr->fd_nm[i], j+1 );
        }

    /* load cwd from ppid to pid */
    for ( j = 0; p_ptr->cwd && p_ptr->cwd[j] && j < MAXPATHLEN; j++ );
    if ( j ) {
        if ( (c_ptr = aud_mem_op ( j+1, (char *)0, 0, 0 )) == (char *)0 )
            fprintf ( stderr, "state_maint_open: no more mem; could not add cwd field for %d\n", pid );
        else {
            a_ptr->cwd = c_ptr;
            bcopy ( p_ptr->cwd, a_ptr->cwd, j+1 );
        }
    }

    /* load root from ppid to pid */
    for ( j = 0; p_ptr->root && p_ptr->root[j] && j < MAXPATHLEN; j++ );
    if ( j ) {
        if ( (c_ptr = aud_mem_op ( j+1, (char *)0, 0, 0 )) == (char *)0 )
            fprintf ( stderr, "state_maint_open: no more mem; could not add root field for %d\n", pid );
        else {
            a_ptr->root = c_ptr;
            bcopy ( p_ptr->root, a_ptr->root, j+1 );
        }
    }

    /* load username from ppid to pid */
    for ( j = 0; p_ptr->username && p_ptr->username[j] && j < MAXPATHLEN; j++ );
    if ( j ) {
        if ( (c_ptr = aud_mem_op ( j+1, (char *)0, 0, 0 )) == (char *)0 )
            fprintf ( stderr, "state_maint_open: no more mem; could not add username field for %d\n", pid );
        else {
            a_ptr->username = c_ptr;
            bcopy ( p_ptr->username, a_ptr->username, j+1 );
        }
    }
}


/* change a_proc's cwd or root */
state_maint_path_change ( pid, ppid, ipaddr, newpath, event )
short pid;
short ppid;
long ipaddr;
char *newpath;
int event;
{
    struct a_proc *a_ptr;
    char pathbuf[MAXPATHLEN];
    char **oldpath;
    int  pathlen = 0;
    static int full = 0;
    int i, j;

    /* get field in a_proc structure to be changed */
    a_ptr = aud_mem_proc ( 2, (struct a_proc *)0, pid, ipaddr, 0 );
    if ( a_ptr == (struct a_proc *)-1 ) {
        state_maint_open ( pid, ppid, ipaddr );
        a_ptr = aud_mem_proc ( 2, (struct a_proc *)0, pid, ipaddr, 0 );
        if ( a_ptr == (struct a_proc *)-1 ) return;
    }
    if ( event == SYS_chdir ) oldpath = &a_ptr->cwd;
    else if ( event == SYS_chroot ) oldpath = &a_ptr->root;
    else return;

    /* parse newpath; can't fully parse due to links */
    if ( newpath[0] == '/' ) pathlen = 0;
    else {
        if ( full ) return;
        if ( *oldpath ) {
            for ( pathlen = 0; (*oldpath)[pathlen] && pathlen < MAXPATHLEN; pathlen++ );
            bcopy ( *oldpath, pathbuf, pathlen+1 );
        }
    }
    for ( i = 0; newpath[i] && i < MAXPATHLEN && pathlen < MAXPATHLEN; i = j ) {

        /* get component of newpath */
        for ( j = i; newpath[j] != '/' && newpath[j] != '\0'; j++ );

        /* ignore "./" */
        if ( (bcmp ( &newpath[i], "./", 2 ) == 0) && (j-i == 1) );

        /* pathbuf <- pathbuf/component */
        else {
            if ( (pathlen == 0) || (pathbuf[pathlen-1] != '/' && pathlen < MAXPATHLEN) ) pathbuf[pathlen++] = '/';
            for ( j = i; newpath[j] != '/' && newpath[j] != '\0' && pathlen < MAXPATHLEN; j++ )
                pathbuf[pathlen++] = newpath[j];
            pathbuf[pathlen] = '\0';
        }

        for ( ; newpath[j] == '/'; j++ );
    }
    if ( pathlen == MAXPATHLEN ) {
        pathlen = 27;
        bcopy ( "MAXIMUM PATHLENGTH EXCEEDED", pathbuf, pathlen+1 );
        full = 1;
    }

    /* swap pathnames */
    if ( *oldpath && **oldpath ) {
        for ( i = 0; (*oldpath)[i] && i < MAXPATHLEN; i++ );
        aud_mem_op ( 0, *oldpath, i+1, 0 );
    }
    if ( (*oldpath = aud_mem_op ( pathlen+1, (char *)0, 0, 0 )) == (char *)0 ) {
        fprintf ( stderr, "state_maint_path_change: no more mem; could not add pathname field for %d\n", pid );
        return;
    }
    if ( *oldpath ) bcopy ( pathbuf, *oldpath, pathlen+1 );
}
