/*	@(#)audit.h	4.5	(ULTRIX)	11/9/90*/

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
 *   07 Jul 89 - scott
 *      created file
 *
 *   06 Aug 90 - scott
 *	add audit of mmap, munmap
 *	add T_SHORT, T_DEVNAME
 *	add AUDIT_START
 *
 *   09 Nov 90 - scott
 *	add audstyle support
*/


#ifdef KERNEL
#include "../h/ansi_compat.h"
#include "../h/types.h"
#include "../h/time.h"
#include "../h/param.h"
#else
#include <ansi_compat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/param.h>
#endif /* KERNEL */

#define AUD_BUF_HDR 38
#define AUD_BUF_SIZ 4096
#define AUD_BUF_TRAILER 35
#define AUD_NPARAM 8
#define NO_RESULT -1
#define N_SYSCALLS 264
#define SYSCALL_MASK_LEN ((N_SYSCALLS*2-1)/8+1)
#define TRUSTED_MASK_LEN ((N_TRUSTED_EVENTS*2-1)/8+1)

/* global data */
#ifdef KERNEL
extern char syscallauditmask[];
extern char trustedauditmask[];
extern char aud_param[][AUD_NPARAM];
extern char aud_shm[][AUD_NPARAM];
extern int  audswitch;
extern int  audsize;
extern char audstyle;
#endif /* KERNEL */

/* audcntl options */
#define GET_SYS_AMASK       0
#define SET_SYS_AMASK       1
#define GET_TRUSTED_AMASK   2
#define SET_TRUSTED_AMASK   3
#define GET_PROC_AMASK      4
#define SET_PROC_AMASK      5
#define GET_PROC_ACNTL      6
#define SET_PROC_ACNTL      7
#define SET_AUDSWITCH       8
#define FLUSH_AUD_BUF       9
#define GETPAID            10
#define SETPAID            11
#define GET_AUDSWITCH      12
#define UPDEVENTS          13
#define GET_AUDSTYLE       14
#define SET_AUDSTYLE       15

/* audcntl flags */
#define AUDIT_OR        0x00
#define AUDIT_AND       0x01
#define AUDIT_OFF       0x02
#define AUDIT_USR       0x04

/* audstyle flags */
#define AUD_EXEC_ARGP   0x01
#define AUD_EXEC_ENVP   0x02

/* token types/length (first bit 0: long; first bit 1: short) */
/* 0x01-0x1f are reserved for tokens for ptrs */
#define A_TOKEN_LENGTH(x)       ((x)&0x80 ? sizeof(short) : sizeof(long))
#define A_TOKEN_PTR(x)          ((x) >= 0x01 && (x) <= 0x1f ? 1 : 0)

#define T_ACCRGHT           0x01
#define T_CHARP             0x02
#define T_INTP              0x05
#define T_MSGHDR            0x06
#define T_SOCK              0x07
#define T_LOGIN             0x08
#define T_HOMEDIR           0x09
#define T_SHELL             0x0a
#define T_PRIVSTR           0x0b
#define T_DEVNAME           0x0c
#define T_SERVICE           0x0d
#define T_LOGIN2            0x0e

#define T_X_ATOM            0x20
#define T_DESCRIP           0x21
#define T_AUID              0x22
#define T_AUID2             0x23
#define T_X_CLIENT          0x24
#define T_RUID              0xa5
#define T_X_CLIENT_INFO     0x26
#define T_NCPU              0xa7
#define T_DEV               0xa8
#define T_DEV2              0xa9
#define T_ERROR             0x2a
#define T_EVENT             0x2b
#define T_HOSTID            0x2c
#define T_HOSTID2           0x2d
#define T_INT               0x2e
#define T_IPC_GID           0xaf

#define T_IPC_MODE          0xb0
#define T_IPC_UID           0xb1
#define T_LENGTH            0x32
#define T_PID               0xb3
#define T_PID2              0xb4
#define T_PPID              0xb5
#define T_PPID2             0xb6
#define T_SHORT             0xb7
#define T_RUID2             0xb8
#define T_RESULT            0x3a
#define T_TV_SEC            0x3c
#define T_TV_USEC           0x3d
#define T_TZ_DST            0x3e
#define T_TZ_MIN            0x3f

#define T_UID               0xc0
#define T_UID2              0xc1
#define T_X_PROPERTY        0x42
#define T_X_RES_CLASS       0x43
#define T_X_RES_TYPE        0x44
#define T_X_RES_ID          0x45
#define T_GNODE_DEV         0xc6
#define T_GNODE_ID          0x47
#define T_HOSTADDR          0x4c
#define T_HOSTADDR2         0x4d

/* non-kernel audit events */
#define MIN_TRUSTED_EVENT    512
#define AUDIT_SUSPEND        512
#define AUDIT_LOG_CHANGE     513
#define AUDIT_SHUTDOWN       514
#define AUDIT_LOG_CREAT      515
#define AUDIT_XMIT_FAIL      516
#define AUDIT_REBOOT         517
#define AUDIT_LOG_OVERWRITE  518
#define AUDIT_DAEMON_EXIT    519
#define AUDGEN8              520
#define AUDIT_SETUP          521
#define LOGIN                522
#define X_SERVER_STARTUP     523
#define X_SERVER_SHUTDOWN    524
#define X_SERVER_DAC         525
#define X_CLIENT_STARTUP     526
#define X_CLIENT_SHUTDOWN    527
#define X_CLIENT_IPC         528
#define X_OBJECT_CREATE      529
#define X_OBJECT_RENAME      530
#define X_OBJECT_DESTROY     531
#define X_OBJECT_DAC         532
#define X_OBJECT_READ        533
#define X_OBJECT_WRITE       534
#define AUTH_EVENT           535
#define AUDIT_START          536

/* trusted event info */
#define N_TRUSTED_EVENTS     64

#ifndef __AUDIT__
/* X secure client information structure */
struct aud_client_info {
    int             hostid;
    audit_ID_t      auid;
    unsigned short  uid;
    short           pid;
    short           ppid;
    short           userlen;
    char           *user;
    unsigned short  ruid;
    unsigned short  family;
    short           pad;
    short           hostlen;
    char           *host;
};
#define __AUDIT__ 1
#endif /* __AUDIT__ */


/* check auditability of syscall (by syscall #) according to auditmask */
#ifdef KERNEL
#define AUDIT_EVENT_K(event) ( (syscallauditmask[(event)>>2] >> ((3-((event)&0x3))<<1)) & 0x3 )
#define AUDIT_EVENT_U(event) ( (u.u_auditmask[(event)>>2] >> ((3-((event)&0x3))<<1)) & 0x3 )
#define AUDIT_EVENT_T(event) ( (trustedauditmask[(event)>>2] >> ((3-((event)&0x3))<<1)) & 0x3 )
#define DO_AUD(event) \
    (u.u_audit_cntl & AUDIT_OFF) ? 0 : \
    ((u.u_audit_cntl & AUDIT_AND) ? \
        AUDIT_EVENT_K(event) & AUDIT_EVENT_U(event) : \
    ((u.u_audit_cntl & AUDIT_USR) ? \
        AUDIT_EVENT_U(event) : \
        AUDIT_EVENT_K(event) | AUDIT_EVENT_U(event) ))

#define DO_AUDIT(event) \
    ( (audswitch == 1 && (event) > 0) \
    && \
    ( ((u.u_error == 0) && (DO_AUD(event)&0x02)) || \
    (u.u_error && (DO_AUD(event)&0x01)) ) )

/* perform audit_rec_build if auditing on for that event/proc */
#define AUDIT_CALL(code,error,retval,flags,intp,spec) \
{ \
    if ( DO_AUDIT(code) ) \
        audit_rec_build ( (code), u.u_ap, aud_param[code], (error), (retval), (intp), (flags) ); \
}
#define AUDIT_CALL_SHM(error,retval,flags,intp,spec) \
{ \
    if ( DO_AUDIT(N_SYSCALLS-1-spec) ) \
        audit_rec_build ( (N_SYSCALLS-1-spec), u.u_ap, aud_shm[(spec)], (error), (retval), (intp), (flags) ); \
}
#endif /* KERNEL */

/* audit_rec_build operations */
#ifdef KERNEL
#define AUD_GNO 0x01
#define AUD_HDR 0x02
#define AUD_PRM 0x04
#define AUD_RES 0x08
#define AUD_FLU 0x10
#define AUD_PTR 0x20
#endif /* KERNEL */

/* adjust buf for system audit_mask by setting event succeed/fail bits */
#define A_SYSMASK_SET(buf,event,succeed,fail) \
    { if ( (event) >= MIN_TRUSTED_EVENT ) { \
        buf[((event)-MIN_TRUSTED_EVENT)>>2] &= ~( 0x3 << ((3-(((event)-MIN_TRUSTED_EVENT)%4))<<1) ); \
        buf[((event)-MIN_TRUSTED_EVENT)>>2] |= ( (((succeed)<<1) | (fail)) << ((3-(((event)-MIN_TRUSTED_EVENT)%4))<<1) ); \
      } \
      else { \
        buf[(event)>>2] &= ~( 0x3 << ((3-((event)%4))<<1) ); \
        buf[(event)>>2] |= ( (((succeed)<<1) | (fail)) << ((3-((event)%4))<<1) ); \
      } \
    }

/* adjust buf for user audit_mask by setting event succeed/fail bits */
#define A_PROCMASK_SET(buf,event,succeed,fail) \
    { if ( (event) >= MIN_TRUSTED_EVENT ) { \
        buf[((event)-MIN_TRUSTED_EVENT+N_SYSCALLS)>>2] &= \
        ~( 0x3 << ((3-(((event)-MIN_TRUSTED_EVENT+N_SYSCALLS)%4))<<1) ); \
        buf[((event)-MIN_TRUSTED_EVENT+N_SYSCALLS)>>2] |= \
        ( (((succeed)<<1) | (fail)) << ((3-(((event)-MIN_TRUSTED_EVENT+N_SYSCALLS)%4))<<1) ); \
      } \
      else { \
        buf[(event)>>2] &= ~( 0x3 << ((3-((event)%4))<<1) ); \
        buf[(event)>>2] |= ( (((succeed)<<1) | (fail)) << ((3-((event)%4))<<1) ); \
      } \
    }

#ifdef __vax
/* system call audit mask for full auditing */
#define FULL_AUDIT \
"\054\070\377\317\314\077\
\074\014\060\077\340\014\
\003\017\063\060\014\000\
\000\300\314\000\310\200\
\017\000\300\000\074\003\
\017\317\360\070\374\003\
\014\003\377\303\000\000\
\314\000\000\000\000\000\
\000\000\000\000\000\000\
\000\000\000\000\000\000\
\000\000\000\300\060\377"
#endif /* __vax */

#ifdef __mips
/* system call audit mask for full auditing */
#define FULL_AUDIT \
"\054\070\377\317\314\077\
\074\014\060\077\340\014\
\003\017\063\060\014\003\
\060\300\314\000\310\200\
\017\000\300\000\074\000\
\017\317\360\070\374\003\
\014\000\014\000\000\014\
\060\377\360\300\074\000\
\000\000\000\000\000\000\
\000\000\000\000\000\000\
\000\000\000\300\060\377"
#endif /* __mips */

/*  2 bits per syscall: audit success, audit failure
  0 -  23: 0...2... 4...6... 8...1... 2...4... 6...8... 2...2...
           00101100 00111000 11111111 11001111 11001100 00111111
           \054     \070     \377     \317     \314     \077

 24 -  47: 4...6... 8...3... 2...4... 6...8... 4...2... 4...6...
           00111100 00001100 00110000 00111111 11100000 00001100
           \074     \014     \060     \077     \340     \014

#ifdef __vax
 48 -  71: 8...5... 2...4... 6...8... 6...2... 4...6... 8...7...
           00000011 00001111 00110011 00110000 00001100 00000000
           \003     \017     \063     \060     \014     \000

 72 -  95: 2...4... 6...8... 8...2... 4...6... 8...9... 2...4...
           00000000 11000000 11001100 00000000 11001000 10000000
           \000     \300     \314     \000     \310     \200
#endif __vax
#ifdef __mips
 48 -  71: 8...5... 2...4... 6...8... 6...2... 4...6... 8...7...
           00000011 00001111 00110011 00110000 00001100 00000011
           \003     \017     \063     \060     \014     \003

 72 -  95: 2...4... 6...8... 8...2... 4...6... 8...9... 2...4...
           00110000 11000000 11001100 00000000 11001000 10000000
           \060     \300     \314     \000     \310     \200
#endif __mips

 96 - 119: 6...8... 0...2... 4...6... 8...1... 2...4... 6...8...
           00001111 00000000 11000000 00000000 00111100 00000011
           \017     \000     \300     \000     \074     \003

120 - 143: 2...2... 4...6... 8...3... 2...4... 6...8... 4...2...
           00001111 11001111 11110000 00111000 11111100 00000011
           \017     \317     \360     \070     \374     \003

#ifdef __vax
144 - 167: 4...6... 8...5... 2...4... 6...8... 6...2... 4...6...
           00001100 00000011 11111111 11000011 00000000 00000000
           \014     \003     \377     \303     \000     \000

168 - 191: 8...7... 2...4... 6...8... 8...2... 4...6... 8...9...
           11001100 00000000 00000000 00000000 00000000 00000000
           \314     \000     \000     \000     \000     \000
#endif __vax
#ifdef __mips
144 - 167: 4...6... 8...5... 2...4... 6...8... 6...2... 4...6...
           00001100 00000000 00001100 00000000 00000000 00001100
           \014     \000     \014     \000     \000     \014

168 - 191: 8...7... 2...4... 6...8... 8...2... 4...6... 8...9...
           00110000 11111111 11110000 11000000 00111100 00000000
           \060     \377     \360     \300     \074     \000
#endif __mips

192 - 215: 2...4... 6...8... 2...2... 4...6... 8...1... 2...4...
           00000000 00000000 00000000 00000000 00000000 00000000
           \000     \000     \000     \000     \000     \000

216 - 239: 6...8... 2...2... 4...6... 8...3... 2...4... 6...8...
           00000000 00000000 00000000 00000000 00000000 00000000
           \000     \000     \000     \000     \000     \000

240 - 263: 4...2... 4...6... 8...5... 2...4... 6...8... 6...2...
           00000000 00000000 00000000 11000000 00110000 11111111
           \000     \000     \000     \300     \060     \377
*/
