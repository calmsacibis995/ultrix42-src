#ifndef lint
static  char    *sccsid = "@(#)condefs.c	4.1	Ultrix	7/2/90";
#endif lint

/*************************
 * definitions for dialer routines
 *************************/


/************************************************************************
 *                                                                      *
 *                      Copyright (c) 1984 by                           *
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
 *   This software is  derived  from  software  received  from  the     *
 *   University    of   California,   Berkeley,   and   from   Bell     *
 *   Laboratories.  Use, duplication, or disclosure is  subject  to     *
 *   restrictions  under  license  agreements  with  University  of     *
 *   California and with AT&T.                                          *
 *                                                                      *
 *   The information in this software is subject to change  without     *
 *   notice  and should not be construed as a commitment by Digital     *
 *   Equipment Corporation.                                             *
 *                                                                      *
 *   Digital assumes no responsibility for the use  or  reliability     *
 *   of its software on equipment which is not supplied by Digital.     *
 *                                                                      *
 ************************************************************************/


#include "uucp.h"
extern int nulldev(), nodev(), Acuopn(), diropn(), dircls();

#ifdef DATAKIT
int dkopn();
#endif
#ifdef DN11
int dnopn(), dncls();
#endif
#ifdef HAYES
int hysopn(), hyscls();
#endif
#ifdef HAYESQ
int hysqopn(), hysqcls();  /* a version of hayes that doesn't use ret codes */
#endif
#ifdef DF0
int df0opn(), df0cls();
#endif
#ifdef DF112
int df112opn(), df112cls();
#endif
#ifdef PNET
int pnetopn();
#endif
#ifdef VENTEL
int ventopn(), ventcls();
#endif
#ifdef  UNET
#include <UNET/unetio.h>
#include <UNET/tcp.h>
int unetopn(), unetcls();
#endif UNET
#ifdef VADIC
int vadopn(), vadcls();
#endif VADIC
#ifdef  RVMACS
int rvmacsopn(), rvmacscls();
#endif
#ifdef MICOM
int micopn(), miccls();
#endif MICOM

#ifdef GENERIC  /* A real way of handling MOST modems */
int genopn(), gencls();
#endif

struct condev condevs[] = {
{ "DIR", "direct", diropn, nulldev, dircls },
#ifdef DATAKIT
{ "DK", "datakit", dkopn, nulldev, nulldev },
#endif
#ifdef PNET
{ "PNET", "pnet", pnetopn, nulldev, nulldev },
#endif
#ifdef  UNET
{ "UNET", "UNET", unetopn, nulldev, unetcls },
#endif UNET
#ifdef MICOM
{ "MICOM", "micom", micopn, nulldev, miccls },
#endif MICOM
#ifdef DN11
{ "ACU", "dn11", Acuopn, dnopn, dncls },
#endif
#ifdef HAYES
{ "ACU", "hayes", Acuopn, hysopn, hyscls },
#endif HAYES
#ifdef HAYESQ   /* a version of hayes that doesn't use result codes */
{ "ACU", "hayesq", Acuopn, hysqopn, hysqcls },
#endif HAYESQ
#ifdef DF0
{ "ACU", "DF02", Acuopn, df0opn, df0cls },
{ "ACU", "DF03", Acuopn, df0opn, df0cls },
#endif
#ifdef DF112
{ "ACU", "DF112", Acuopn, df112opn, df112cls },
#endif
#ifdef VENTEL
{ "ACU", "ventel", Acuopn, ventopn, ventcls },
#endif VENTEL
#ifdef VADIC
{ "ACU", "vadic", Acuopn, vadopn, vadcls },
#endif VADIC
#ifdef RVMACS
{ "ACU", "rvmacs", Acuopn, rvmacsopn, rvmacscls },
#endif RVMACS
#ifdef GENERIC
{"ACU", "generic", Acuopn, genopn, gencls },
#endif

/* Insert new entries before this line */
{ NULL, NULL, NULL, NULL, NULL } };
