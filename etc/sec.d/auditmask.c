#ifndef lint
static char *sccsid = "@(#)auditmask.c	4.3	ULTRIX	11/13/90";
#endif lint

/************************************************************************
 *									*
 *                      Copyright (c) 1989 by                           *
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
 *   22 Sep 89 - scott
 *      created file
 *
 *   06 Aug 90 - scott
 *      changed comment symbol from '#' to '!'
 *
 *   12 Nov 90 - scott
 *      added '-s' option for audstyle
 *
*/

/* get/set audit mask
    link with /sys/`machine`/BINARY/syscalls.o -laud
*/
#include <sys/audit.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <nlist.h>

#define REQ_LEN 32

struct nlist nlst[] = {
    { "_nsysent" },
#define X_NSYSENT      0
    { 0 },
};

char *special_event[] = {
    "shmget",
    "shmdt",
    "shmctl",
    "shmat"
};

extern char *syscallnames[];
extern char *trustedevent[];

main ( argc, argv )
int argc;
char *argv[];
{
    char buf1[SYSCALL_MASK_LEN];    /* syscall mask         */
    char buf2[TRUSTED_MASK_LEN];    /* trusted event mask   */
    struct stat statbuf;
    int flag_new;                   /* used for audstyle    */
    int flag_old;                   /* used for audstyle    */
    int i;

    /* get auditmasks */
    if ( (audcntl ( GET_SYS_AMASK, buf1, SYSCALL_MASK_LEN, 0, 0 ) == -1) ||
    (audcntl ( GET_TRUSTED_AMASK, buf2, TRUSTED_MASK_LEN, 0, 0 ) == -1) ) {
        perror ( "audcntl" );
        exit(1);
    }

    /* get audit style */
    if ( (flag_old = audcntl ( GET_AUDSTYLE, (char *)0, 0, 0, 0 )) == -1 ) {
        perror ( "audcntl" );
        exit(1);
    }
    flag_new = flag_old;

    /* if input redirected, set mask according to stdin */
    fstat ( 0, &statbuf );
    if ( statbuf.st_mode & S_IFREG ) input_file_audit ( buf1, buf2 );

    /* display auditmask */
    else if ( argc == 1 ) {
        show_audit ( buf1, buf2 );
        show_audstyl ( flag_old );
        exit(0);
    }

    /* modify auditmask */
    else for ( i = 1; i < argc; i++ ) {

        /* full audit */
        if ( argv[i][0] == '-' && argv[i][1] == 'f' )
            full_audit ( buf1, buf2 );

        /* no audit events */
        else if ( argv[i][0] == '-' && argv[i][1] == 'n' )
            no_audit ( buf1, buf2 );

        /* set audstyle flags; allow for "exec_argp[:0]", "exec_envp[:0]" */
        else if ( argv[i][0] == '-' && argv[i][1] == 's' && i < argc-1 ) {
            if ( strcmp ( argv[++i], "exec_argp" ) == 0 )      flag_new |= AUD_EXEC_ARGP;
            else if ( strcmp ( argv[i], "exec_argp:0" ) == 0 ) flag_new &= ~AUD_EXEC_ARGP;
            else if ( strcmp ( argv[i], "exec_envp" ) == 0 )   flag_new |= AUD_EXEC_ENVP;
            else if ( strcmp ( argv[i], "exec_envp:0" ) == 0 ) flag_new &= ~AUD_EXEC_ENVP;
            else if ( argv[i][0] >= '0' && argv[i][0] <= '9' ) flag_new |= atoi(argv[i]);
            else printf ( "bad flag value: %s\n", argv[i] );
        }

        /* selective auditing */
        else change_audit_mask ( argv[i], buf1, buf2 );

    }

    /* audstyle */
    if ( flag_new != flag_old )
        if ( audcntl ( SET_AUDSTYLE, (char *)0, 0, flag_new, 0 ) == -1 )
            perror ( "audcntl" );

    /* set auditmasks */
    if ( (audcntl ( SET_SYS_AMASK, buf1, SYSCALL_MASK_LEN, 0, 0 ) == -1) ||
    (audcntl ( SET_TRUSTED_AMASK, buf2, TRUSTED_MASK_LEN, 0, 0 ) == -1) )
        perror ( "audcntl" );
}


/* add/remove audit event to/from appropriate mask */
change_audit_mask ( event, buf1, buf2 )
char *event;
char *buf1;
char *buf2;
{
    char event_l[REQ_LEN];
    char flags;
    static int nsyscalls = 0;
    int i;

    /* get # syscalls by reading kernel */
    if ( nsyscalls == 0 ) nsyscalls = getkval ( X_NSYSENT );
    if ( nsyscalls == -1 ) {
        perror ( "nsyscalls failed" );
        exit(1);
    }

    /* read event:succeed:fail; kludge for "old" syscalls */
    for ( i = 0; event[i] != ':' && event[i] != ' ' && event[i] && i < REQ_LEN-1; i++ )
        event_l[i] = event[i];
    if ( strncmp ( event_l, "old", 3 ) == 0 ) {
        event_l[i++] = ' ';
        for ( ; event[i] != ':' && event[i] != ' ' && i < REQ_LEN-1; i++ )
            event_l[i] = event[i];
    }
    event_l[i] = '\0';
    if ( event[i] && i < REQ_LEN-1)
        flags = ((event[i+1] == '0' ? 0 : 1) << 1) | (event[i+3] == '0' ? 0 : 1 );
    else flags = 0x3;

    /* set syscall event */
    for ( i = 0; i < nsyscalls; i++ )
        if ( strcmp ( event_l, syscallnames[i] ) == 0 ) {
            A_SYSMASK_SET ( buf1, i, (flags&0x2)>>1, flags&0x1 );
            return;
        }

    /* kludge for shmsys operations */
    for ( i = 0; i < 4; i++ )
        if ( strcmp ( event_l, special_event[i] ) == 0 ) {
            A_SYSMASK_SET ( buf1, (N_SYSCALLS-4+i), (flags&0x2)>>1, flags&0x1 );
            return;
        }

    /* set trusted event */
    for ( i = 0; i < N_TRUSTED_EVENTS; i++ )
        if ( strcmp ( event_l, trustedevent[i] ) == 0 ) {
            A_SYSMASK_SET ( buf2, i+MIN_TRUSTED_EVENT, (flags&0x2)>>1, flags&0x1 );
            return;
        }

    /* event not found */
    if ( i == N_TRUSTED_EVENTS ) {
        printf ( "Can't find event %s\n", event_l );
        return;
    }
}


/* set syscall and trusted audit masks to full audit */
full_audit ( buf1, buf2 )
char *buf1;     /* syscall mask       */
char *buf2;     /* trusted event mask */
{
    int i;

    for ( i = 0; i < SYSCALL_MASK_LEN; i++ ) buf1[i] = FULL_AUDIT[i];
    for ( i = 0; i < TRUSTED_MASK_LEN; i++ ) buf2[i] = '\377';
}


/* get integer value from kernel */
int getkval ( var )
int var;
{
    static int vm_fd = -1;
    static int km_fd;
    int i = 0;

    if ( vm_fd == -1 ) {
        if ( (vm_fd = open ( "/vmunix", 0 )) == -1 ) return(-1);
        if ( (km_fd = open ( "/dev/kmem", 0 )) == -1 ) return(-1);
        nlist ( "/vmunix", nlst );
        if ( nlst[0].n_type == 0 ) return (-1);
    }

    if ( lseek ( km_fd, nlst[var].n_value, 0 ) == -1 ) return(-1);
    read ( km_fd, &i, sizeof(int) );

    return ( i );
}


/* parse input for change_audit_mask */
input_file_audit ( buf1, buf2 )
char *buf1;     /* syscall mask       */
char *buf2;     /* trusted event mask */
{
    char inbuf[1024];
    char request[REQ_LEN];
    int start;
    int end;
    int i, j, k;

    for ( ; i = read ( 0, inbuf, sizeof inbuf ); ) {
        for ( j = 0; j < i; j++ ) {

            /* if don't have complete request, shift buffer and get more input */
            for ( ; inbuf[j] == '\n'; j++ );
            for ( end = j; inbuf[end] != '\n' && end < i; end++ );
            if ( end == i ) {
                for ( start = j; start < end; start++ )
                    inbuf[start-j] = inbuf[start];
                end -= j;
                j = -1;
                i = read ( 0, &inbuf[end], sizeof inbuf - end ) + end;
                continue;
            }

            /* ignore comments */
            if ( inbuf[j] == '!' ) {
                for ( ; inbuf[j] != '\n'; j++ );
                for ( ; inbuf[j] == '\n'; j++ );
                j--;
                continue;
            }

            /* format change_audit_mask request */
            for ( k = 0; inbuf[j] != ' ' && inbuf[j] != '\t' && inbuf[j] != '\n'; j++ )
                request[k++] = inbuf[j];
            if ( strncmp ( request, "old", 3 ) == 0 ) {
                request[k++] = inbuf[j++];
                for ( ; inbuf[j] != ' ' && inbuf[j] != '\t' && inbuf[j] != '\n'; j++ )
                    request[k++] = inbuf[j];
            }
            for ( ; inbuf[j] == ' ' || inbuf[j] == '\t'; j++ );
            request[k++] = ':';
            if ( inbuf[j] != '\n' ) {
                request[k++] = !strncmp ( &inbuf[j], "succeed", 7 ) + '0';
                if ( request[k-1] == '1' )
                    for ( ; inbuf[j] != ' ' && inbuf[j] != '\t' && inbuf[j] != '\n'; j++ );
            }
            else request[k++] = '0';
            for ( ; inbuf[j] == ' ' || inbuf[j] == '\t'; j++ );
            request[k++] = ':';
            if ( inbuf[j] != '\n' ) {
                request[k++] = !strncmp ( &inbuf[j], "fail", 4 ) + '0';
                for ( ; inbuf[j] != '\n'; j++ );
            }
            else request[k++] = '0';
            request[k] = '\0';
            change_audit_mask ( request, buf1, buf2 );
        }
    }
}


/* zero audit masks */
no_audit ( buf1, buf2 )
char *buf1;     /* syscall mask       */
char *buf2;     /* trusted event mask */
{
    int i;

    for ( i = 0; i < SYSCALL_MASK_LEN; i++ ) buf1[i] = '\0';
    for ( i = 0; i < TRUSTED_MASK_LEN; i++ ) buf2[i] = '\0';
}


/* display auditmask */
show_audit ( buf1, buf2 )
char *buf1;     /* syscall mask       */
char *buf2;     /* trusted event mask */
{
    int nsyscalls;
    int j, k;

    /* get # syscalls */
    nsyscalls = getkval ( X_NSYSENT );
    if ( nsyscalls == -1 ) {
        perror ( "failed" );
        exit(1);
    }

    /* display syscall mask */
    printf ( "! Audited system calls:\n" );
    for ( j = 0; j < SYSCALL_MASK_LEN; j++ )
        for ( k = 0; k < 8; k+=2 ) {
            if ( (j<<2)+(k>>1) >= nsyscalls ) break;
            if ( buf1[j] & (0x3 << (6-k%8)) ) {
                printf ( "%-20s", syscallnames[(j<<2)+(k>>1)] );
                if ( buf1[j] & (0x2 << (6-k%8)) )
                    printf ( "  succeed" );
                else printf ( "         " );
                if ( buf1[j] & (0x1 << (6-k%8)) )
                    printf ( "  fail\n" );
                else printf ( "      \n" );
            }
        }

    /* kludge for shmsys operations */
    j = (N_SYSCALLS*2)/8 - 1;
    for ( k = 0; k < 8; k+=2 )
        if ( buf1[j] & (0x3 << (6-k%8)) ) {
            printf ( "%-20s", special_event[k>>1] );
            if ( buf1[j] & (0x2 << (6-k%8)) )
                printf ( "  succeed" );
            else printf ( "         " );
            if ( buf1[j] & (0x1 << (6-k%8)) )
                printf ( "  fail\n" );
            else printf ( "      \n" );
        }

    /* display trusted event mask */
    printf ( "\n! Audited trusted events:\n" );
    for ( j = 0; j < TRUSTED_MASK_LEN; j++ )
        for ( k = 0; k < 8; k+=2 ) {
            if ( buf2[j] & (0x3 << (6-k%8)) ) {
                printf ( "%-20s", trustedevent[(j<<2)+(k>>1)] );
                if ( buf2[j] & (0x2 << (6-k%8)) )
                    printf ( "  succeed" );
                else printf ( "         " );
                if ( buf2[j] & (0x1 << (6-k%8)) )
                    printf ( "  fail\n" );
                else printf ( "      \n" );
            }
        }
}


/* display audstyle flags */
show_audstyl ( flag )
int flag;
{
    if ( flag ) {
        printf ( "\n!  Audstyle flags: " );
        if ( flag & AUD_EXEC_ARGP ) printf ( "exec_argp " );
        if ( flag & AUD_EXEC_ENVP ) printf ( "exec_envp " );
        printf ( "\n" );
    }
}
