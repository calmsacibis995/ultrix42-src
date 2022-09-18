#ifndef lint
static char *sccsid = "@(#)kern_auditcalls.c	4.4	ULTRIX	11/13/90";
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
 *      self-audit audcntl
 *
 *   12 Nov 90 - scott
 *      add GET_AUDSTYLE, SET_AUDSTYLE
*/

#include "../h/types.h"
#include "../h/param.h"
#include "../h/signal.h"
#include "../h/user.h"
#include "../h/kmalloc.h"
#include "../h/socket.h"

/* audcntl system call */
audcntl()
{
    struct a {
        int req;
        char *argp;
        int len;
        int cntl;
        audit_ID_t audit_id;
    } *uap = (struct a *)u.u_ap;
    char local[SYSCALL_MASK_LEN+TRUSTED_MASK_LEN];
    int len;
    int before, after;
    static eventl = 0;          /* local u.u_event */
    static char nargl = 0;      /* local u.u_narg  */
    int i;

    before = DO_AUDIT(u.u_event);

    switch ( uap->req ) {

    case GET_SYS_AMASK:
        len = uap->len < SYSCALL_MASK_LEN ? uap->len : SYSCALL_MASK_LEN;
        if ( u.u_uid != 0 ) u.u_error = EACCES;
        else if ( len < 0 ) u.u_error = EINVAL;
        else {
            smp_lock ( &lk_auditmask, LK_RETRY );
            bcopy ( syscallauditmask, local, len );
            smp_unlock ( &lk_auditmask );
            u.u_error = copyout ( local, uap->argp, len );
        }
        u.u_r.r_val1 = len;
        break;

    case SET_SYS_AMASK:
        len = uap->len < SYSCALL_MASK_LEN ? uap->len : SYSCALL_MASK_LEN;
        if ( u.u_uid != 0 ) u.u_error = EACCES;
        else if ( len < 0 ) u.u_error = EINVAL;
        else {
            if ( u.u_error = copyin ( uap->argp, local, len ) ) break;
            smp_lock ( &lk_auditmask, LK_RETRY );
            bcopy ( local, syscallauditmask, len );
            smp_unlock ( &lk_auditmask );
        }
        u.u_r.r_val1 = len;
        break;

    case GET_TRUSTED_AMASK:
        len = uap->len < TRUSTED_MASK_LEN ? uap->len : TRUSTED_MASK_LEN;
        if ( u.u_uid != 0 ) u.u_error = EACCES;
        else if ( len < 0 ) u.u_error = EINVAL;
        else {
            smp_lock ( &lk_auditmask, LK_RETRY );
            bcopy ( trustedauditmask, local, len );
            smp_unlock ( &lk_auditmask );
            u.u_error = copyout ( local, uap->argp, len );
        }
        u.u_r.r_val1 = len;
        break;

    case SET_TRUSTED_AMASK:
        len = uap->len < TRUSTED_MASK_LEN ? uap->len : TRUSTED_MASK_LEN;
        if ( u.u_uid != 0 ) u.u_error = EACCES;
        else if ( len < 0 ) u.u_error = EINVAL;
        else {
            if ( u.u_error = copyin ( uap->argp, local, len ) ) break;
            smp_lock ( &lk_auditmask, LK_RETRY );
            bcopy ( local, trustedauditmask, len );
            smp_unlock ( &lk_auditmask );
        }
        u.u_r.r_val1 = len;
        break;

    case GET_PROC_AMASK:
        len = uap->len < SYSCALL_MASK_LEN+TRUSTED_MASK_LEN ? uap->len : SYSCALL_MASK_LEN+TRUSTED_MASK_LEN;
        if ( u.u_uid != 0 ) u.u_error = EACCES;
        else if ( len < 0 ) u.u_error = EINVAL;
        else u.u_error = copyout ( u.u_auditmask, uap->argp, len );
        u.u_r.r_val1 = len;
        break;

    case SET_PROC_AMASK:
        len = uap->len < SYSCALL_MASK_LEN+TRUSTED_MASK_LEN ? uap->len : SYSCALL_MASK_LEN+TRUSTED_MASK_LEN;
        if ( u.u_uid != 0 ) u.u_error = EACCES;
        else if ( len < 0 ) u.u_error = EINVAL;
        else u.u_error = copyin ( uap->argp, u.u_auditmask, len );
        u.u_r.r_val1 = len;
        break;

    case GET_PROC_ACNTL:
        if ( u.u_uid != 0 ) u.u_error = EACCES;
        else u.u_r.r_val1 = u.u_audit_cntl;
        break;

    case SET_PROC_ACNTL:
        if ( u.u_uid != 0 ) u.u_error = EACCES;
        else {
            u.u_audit_cntl = uap->cntl;
            u.u_r.r_val1 = u.u_audit_cntl;
        }
        break;

    case SET_AUDSWITCH:
        if ( u.u_uid != 0 ) u.u_error = EACCES;
        else {
            u.u_r.r_val1 = audswitch;
            if ( uap->cntl == '\0' ) {
                audswitch = 0;
                nargl = u.u_narg;
                eventl = u.u_event;
            }
            /* this bypasses generating audit record first time
               the audswitch is turned on (and subsequent times until
               first time audit is turned off); that's ok */
            else {
                audswitch = 1;
                if ( eventl == 0 ) return;
                u.u_narg = nargl;
                u.u_event = eventl;
            }
        }
        break;

    case FLUSH_AUD_BUF:
        if ( u.u_uid != 0 ) u.u_error = EACCES;
        else u.u_r.r_val1 = audit_rec_build ( 0, (char **)0, (char *)0, 0, 0, 0, AUD_FLU );
        break;

    case GETPAID:
        if ( u.u_auid < 0 ) u.u_error = EPERM;
        else u.u_r.r_val1 = u.u_auid;
        break;

    case SETPAID:
        if ( u.u_uid != 0 ) u.u_error = EPERM;
        else if ( uap->audit_id <= 0 ) u.u_error = EINVAL;
        else u.u_auid = uap->audit_id;
        break;

    case GET_AUDSWITCH:
        if ( u.u_uid != 0 ) u.u_error = EACCES;
        else u.u_r.r_val1 = audswitch;
        break;

    case UPDEVENTS:
        u.u_error = EOPNOTSUPP;
        break;

    case SET_AUDSTYLE:
        if ( u.u_uid != 0 ) u.u_error = EACCES;
        else {
            u.u_r.r_val1 = audstyle;
            audstyle = uap->cntl;
        }
        break;

    case GET_AUDSTYLE:
        if ( u.u_uid != 0 ) u.u_error = EACCES;
        else u.u_r.r_val1 = audstyle;
        break;

    default:    /* not supported */
        u.u_error = EINVAL;
        break;

    }

    /* must do own auditing here to catch transitions of turning audit on/off */
    after = DO_AUDIT(u.u_event);
    if ( before | after )
        audit_rec_build ( u.u_event, u.u_ap, aud_param[u.u_event],
        u.u_error, u.u_r.r_val1, (int *)0, AUD_HDR|AUD_PRM|AUD_RES );
}


/* audgen system call */
audgen()
{
    struct a {
        unsigned code;
        char *tokenmask;
        char **params;
    } *uap = (struct a *)u.u_ap;

    char tokenmask[AUD_NPARAM];     /* mask of param types          */
    char *ad_buf;                   /* ptr to audit record data     */
    int result = NO_RESULT;         /* operation result             */
    int error = 0;                  /* operation error              */
    int narg;                       /* # of arguments               */
    int data_len = -1;              /* length of data string arg    */
    int len = 0;                    /* length of audit record data  */
    int buf_indx;                   /* buf # returned from build    */
    struct aud_client_info *aci_p;  /* ptr to aud_client_info       */
    int failed = 0;                 /* set to 1 if T_ERROR used     */
    char *ptr;
    int i, j;
    short k;

/* macros to insert data into audit record buffer */
#define SIZ_TOKEN 1
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

    /* this should be changed later when priv mechanism exists */
    if ( u.u_uid != 0 ) {
        u.u_error = EACCES;
        return;
    }

    /* check code number */
    if ( uap->code >= MIN_TRUSTED_EVENT+N_TRUSTED_EVENTS || uap->code < MIN_TRUSTED_EVENT ) {
        u.u_error = EINVAL;
        return;
    }

    /* fetch tokenmask */
    copyinstr ( uap->tokenmask, tokenmask, AUD_NPARAM, &narg );

    /* check tokenmask */
    for ( i = 0; i < narg; i++ ) switch ( tokenmask[i]&0xff ) {
        case T_AUID:    case T_UID:
        case T_PID:     case T_PPID:
        case T_DEV:     case T_HOSTID:
        case T_ACCRGHT: case T_INTP:
        case T_MSGHDR:  case T_HOSTADDR:
        case T_RUID:
            u.u_error = EINVAL;
            return;
        case T_ERROR:
            failed = 1;
            break;
        default:
            break;
    }

    /* check auditability of trusted event */
    if ( audswitch == 0 ) return;
    if ( (i = AUDIT_EVENT_T ( uap->code-MIN_TRUSTED_EVENT )) == 0 ) return;
    if ( (failed == 1) && ((i&0x01) == 0) ) return;
    if ( (failed == 0) && ((i&0x02) == 0) ) return;
    if ( u.u_audit_cntl & AUDIT_OFF ) return;

    /* build header of audit record using audit_rec_build */
    if ( (ad_buf = (char *)audit_rec_build ( uap->code, (char **)0,
    (char *)0, 0, 0, &buf_indx, AUD_HDR )) == (char *)0 ) return;

    /* add parameters to audit record */
    for ( i = 0; i < narg && tokenmask[i]; i++ ) {

        /* special case result and error */
        if ( tokenmask[i] == T_RESULT ) result = fuword(uap->params++);
        else if ( tokenmask[i] == T_ERROR ) error = fuword(uap->params++);

        /* sockets */
        else if ( tokenmask[i] == T_SOCK ) {
            ptr = (char *)fuword(uap->params++);
            if ( ptr == (char *)-1 ) continue;
            j = sizeof(struct sockaddr);
            INSERT_AUD4 ( AUD_BUF_SIZ, ad_buf, len, tokenmask[i], ptr, j );
        }

        /* X client info structure */
        else if ( tokenmask[i] == T_X_CLIENT_INFO ) {
            ptr = (char *)fuword(uap->params++);
            if ( ptr == (char *)-1 ) continue;
            j = sizeof (struct aud_client_info);
            aci_p = (struct aud_client_info *)&ad_buf[len+1];
            INSERT_AUD5 ( AUD_BUF_SIZ, ad_buf, len, tokenmask[i], ptr, j );
            bcopy ( &aci_p->userlen, &k, sizeof k );
            bcopy ( &aci_p->user, &ptr, sizeof ptr );
            if ( (j = (int)k) > 0 )
                INSERT_AUD4 ( AUD_BUF_SIZ, ad_buf, len, T_CHARP, ptr, j );
            bcopy ( &aci_p->hostlen, &k, sizeof k );
            bcopy ( &aci_p->host, &ptr, sizeof ptr );
            if ( (j = (int)k) > 0 )
                INSERT_AUD4 ( AUD_BUF_SIZ, ad_buf, len, T_SOCK, ptr, j );
        }

        /* user null-terminated strings */
        else if ( A_TOKEN_PTR(tokenmask[i]) ) {
            ptr = (char *)fuword(uap->params++);
            if ( ptr == (char *)-1 ) continue;
            INSERT_AUD2 ( AUD_BUF_SIZ, ad_buf, len, tokenmask[i], ptr, j );
        }

        /* all other user short,int-sized objects */
        else if ( !A_TOKEN_PTR(tokenmask[i]) ) {
            j = A_TOKEN_LENGTH(tokenmask[i]);
            INSERT_AUD5 ( AUD_BUF_SIZ, ad_buf, len, tokenmask[i], uap->params++, j );
        }

        /* check length here (additional check to INSERT_AUD macros) */
        if ( len >= AUD_BUF_SIZ - (AUD_BUF_HDR+AUD_BUF_TRAILER) ) {
            u.u_error = E2BIG;
            len -= AUD_BUF_TRAILER;
            break;
        }
    }

    /* update ptr in audit_rec_build() */
    audit_rec_build ( 0, (char **)0, (char *)0, 0, len, &buf_indx, AUD_PTR );

    /* build trailer of audit record, and pass to /dev/audit */
    audit_rec_build ( 0, (char **)0, (char *)0, error, result, &buf_indx, AUD_RES );
}
