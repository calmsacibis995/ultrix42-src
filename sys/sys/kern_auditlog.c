#ifndef lint
static char *sccsid = "@(#)kern_auditlog.c	4.2	ULTRIX	8/7/90";
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
 *   06 Aug 90 - scott
 *      modified for audit data recovery
 *      modified for size configurability
 *
*/

/*
auditopen(), auditclose(), auditread(), auditwrite(), auditsel(), initaud(),
kernaudwrite()

auditopen():    nop, return(0);
auditclose():   nop, return(0);

auditread():    perform a uiomove of up to # of chars specified,
                adjust /dev/audit memory buffers/ptrs

auditwrite():   check sizeof buffer to be written to /dev/audit,
                maintain circular buffer
                perform a uiomove of up to # of chars specified
                schedule wakeup of auditd

auditsel():     set return val so select() works properly

initaud():      initialize audit log buffer and pointers

kernaudwrite(): kernel auditwrite
*/

#include "../h/types.h"
#include "../h/param.h"
#include "../h/uio.h"
#include "../h/time.h"
#include "../h/proc.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/kmalloc.h"
#include "../h/systm.h"
#include "../h/file.h"
#include "../h/ksched.h"
#include "../h/errno.h"

#define BUF_SIZ ((audsize*1024)<<2)

struct {
    caddr_t buf;
    caddr_t head;
    caddr_t le;
    caddr_t tail;
    caddr_t save;
} aud_log;

struct proc *audp = (struct proc *)0; /* ptr to proc waiting on /dev/audit */
int aud_setup = 1;                    /* need to initialize                */
int rcoll = 0;                        /* 1: read collision happened        */


/* nop - return successful completion */
auditclose()
{
    return(0);
}


/* nop - return successful completion */
auditopen ( dev, mode )
dev_t dev;
int mode;
{
    return(0);
}


/* pass buffer data to user */
auditread ( dev, uio )
dev_t dev;
struct uio *uio;
{
    struct iovec *iov = uio->uio_iov;
    int rval = 0;
    int amt = 0;

    /* move as much data as possible from audit buffer to user */
    if ( (iov->iov_len <= 0) || (aud_log.head == aud_log.tail) ) return(0);

    smp_lock ( &lk_audlog, LK_RETRY );  /* wait lock */

    if ( aud_log.head > aud_log.tail )
        amt = iov->iov_len < aud_log.head-aud_log.tail ? iov->iov_len : aud_log.head-aud_log.tail;
    else if ( aud_log.head < aud_log.tail ) {
        if ( aud_log.tail >= aud_log.le ) {
            /* safe transition */
            aud_log.tail = aud_log.buf;
            aud_log.le = aud_log.head;
        }
        amt = iov->iov_len < aud_log.le-aud_log.tail ? iov->iov_len : aud_log.le-aud_log.tail;
    }
    rval = uiomove ( aud_log.tail, amt, UIO_READ, uio );
    aud_log.save = aud_log.tail;
    aud_log.tail += amt;

    /* wakeup anyone who got sleep'ed for auditor */
    wakeup ( (caddr_t)&aud_log );

    smp_unlock ( &lk_audlog );
    return ( rval );
}


/* set return val so select() works properly */
auditsel ( dev, rw )
dev_t dev;
int rw;
{
    int rval = 0;

    if ( aud_setup ) initaud();

    switch ( rw ) {

    case FREAD:     rval = (aud_log.head != aud_log.tail);
                    if ( rval == 0 ) {
                        if ( audp && audp->p_wchan == (caddr_t)&selwait ) {
                            rcoll = 1;
                        }
                        else {
                            audp = u.u_procp;
                        }
                    }
                    break;

    case FWRITE:    smp_lock ( &lk_audlog, LK_RETRY );
                    rval = ( (aud_log.head != aud_log.save) || (aud_log.save == aud_log.tail) );
                    smp_unlock ( &lk_audlog );
                    break;

    default:        rval = EINVAL;
                    break;

    }

    return ( rval );
}


/* insert data into aud_log.buf */
auditwrite ( dev, uio )
dev_t dev;
struct uio *uio;
{
    int size = BUF_SIZ < (AUD_BUF_SIZ<<4) ? (AUD_BUF_SIZ<<4) : BUF_SIZ;
    struct iovec *iov = uio->uio_iov;
    int rval = 0;

    /* initializations */
    if ( aud_setup ) initaud();

    /* move as much data as possible into audit buffer */
    if ( iov->iov_len <= 0 ) return(0);
    smp_lock ( &lk_audlog, LK_RETRY );  /* wait lock */

    /* test for buffer overflow; sleep offending proc */
    for ( ; ((aud_log.head == aud_log.save) && (aud_log.save != aud_log.tail))
      ||
    ((aud_log.head < aud_log.save) && (iov->iov_len >= aud_log.save-aud_log.head))
      ||
    ((aud_log.head > aud_log.save) && (iov->iov_len >= aud_log.save-aud_log.buf) &&
    (iov->iov_len >= aud_log.buf+size-aud_log.head)); ) {
        sleep_unlock ( &aud_log, PZERO, &lk_audlog );
        smp_lock ( &lk_audlog, LK_RETRY );
    }

    if ( aud_log.head >= aud_log.save ) {
        if ( iov->iov_len < aud_log.buf+size-aud_log.head ) {
            rval = uiomove ( aud_log.head, iov->iov_len, UIO_WRITE, uio );
            aud_log.head += iov->iov_len;
            aud_log.le = aud_log.head;
        }
        else if ( iov->iov_len < aud_log.save - aud_log.buf ) {
            rval = uiomove ( aud_log.buf, iov->iov_len, UIO_WRITE, uio );
            aud_log.le = aud_log.head;
            aud_log.head = aud_log.buf + iov->iov_len;
        }
        else cprintf ( "kern_auditlog: audit buffer overflow\n" );
    }
    else if ( aud_log.head < aud_log.save ) {
        if ( iov->iov_len < aud_log.save - aud_log.head ) {
            rval = uiomove ( aud_log.head, iov->iov_len, UIO_WRITE, uio );
            aud_log.head += iov->iov_len;
        }
        else cprintf ( "kern_auditlog: audit buffer overflow\n" );
    }
    else cprintf ( "kern_auditlog: audit buffer overflow\n" );
    
    /* wakeup waiting read processes */
    if ( audp ) {
        selwakeup ( audp, rcoll );
        audp = (struct proc *)0;
        rcoll = 0;
    }

    smp_unlock ( &lk_audlog );
    return ( rval );
}


/* initializations */
initaud()
{
    int size = BUF_SIZ < (AUD_BUF_SIZ<<4) ? (AUD_BUF_SIZ<<4) : BUF_SIZ;

    /* lock after initial test, then retest, so need take lock 1x only */
    smp_lock ( &lk_audlog, LK_RETRY );
    if ( aud_setup == 0 ) {
        smp_unlock ( &lk_audlog );
        return;
    }
    aud_setup = 0;
    smp_unlock ( &lk_audlog );

    KM_ALLOC ( aud_log.buf, caddr_t, size, KM_DEVBUF, KM_NOWAIT );
    if ( aud_log.buf == NULL ) panic ( "auditlog" );
    aud_log.head = aud_log.buf;
    aud_log.tail = aud_log.buf;
    aud_log.le = aud_log.buf + size;
    aud_log.save = aud_log.buf;
}


/* copy data from audit.c buffer to local buffer */
kernaudwrite ( buf, len )
char *buf;
int len;
{
    caddr_t buf_l;
    int size = BUF_SIZ < (AUD_BUF_SIZ<<4) ? (AUD_BUF_SIZ<<4) : BUF_SIZ;

    /* initializations */
    if ( aud_setup ) initaud();

    /* move as much data as possible into audit buffer */
    if ( len <= 0 ) return(0);
    smp_lock ( &lk_audlog, LK_RETRY );  /* wait lock */

    /* test for buffer overflow; sleep offending proc */
    for ( ; ((aud_log.head == aud_log.save) && (aud_log.save != aud_log.tail))
      ||
    ((aud_log.head < aud_log.save) && (len >= aud_log.save-aud_log.head))
      ||
    ((aud_log.head > aud_log.save) && (len >= aud_log.save-aud_log.buf) &&
    (len >= aud_log.buf+size-aud_log.head) ); ) {
        sleep_unlock ( &aud_log, PZERO, &lk_audlog );
        smp_lock ( &lk_audlog, LK_RETRY );
    }

    if ( aud_log.head >= aud_log.save ) {
        if ( len < aud_log.buf+size-aud_log.head ) {
            bcopy ( buf, aud_log.head, len );
            aud_log.head += len;
            aud_log.le = aud_log.head;
        }
        else if ( len < aud_log.save - aud_log.buf ) {
            buf_l = aud_log.buf;
            bcopy ( buf, buf_l, len );
            aud_log.le = aud_log.head;
            aud_log.head = aud_log.buf + len;
        }
        else cprintf ( "kern_auditlog: audit buffer overflow A: %d %x %x %x\n",
        len, aud_log.buf, aud_log.save, aud_log.head );
    }
    else if ( aud_log.head < aud_log.save ) {
        if ( len < aud_log.save - aud_log.head ) {
            bcopy ( buf, aud_log.head, len );
            aud_log.head += len;
        }
        else cprintf ( "kern_auditlog: audit buffer overflow B: %d %x %x %x\n",
        len, aud_log.buf, aud_log.save, aud_log.head );
    }
    else cprintf ( "kern_auditlog: audit buffer overflow C: %d %x %x %x\n",
    len, aud_log.buf, aud_log.save, aud_log.head );
    
    /* wakeup waiting read processes */
    if ( audp ) {
        selwakeup ( audp, rcoll );
        audp = (struct proc *)0;
        rcoll = 0;
    }

    smp_unlock ( &lk_audlog );
    return ( len );
}
