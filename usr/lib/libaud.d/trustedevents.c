#ifndef lint
static char *sccsid = "@(#)trustedevents.c	4.1	ULTRIX	8/8/90";
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
 *   08 Aug 90 - scott
 *      add audit_start event
 *      move into libaud.a
 *
*/
/* trusted event names */

char *trustedevent[] = {
    "audit_suspend",        /* 512 */
    "audit_log_change",     /* 513 */
    "audit_shutdown",       /* 514 */
    "audit_log_creat",      /* 515 */
    "audit_xmit_fail",      /* 516 */
    "audit_reboot",         /* 517 */
    "audit_log_overwrite",  /* 518 */
    "audit_daemon_exit",    /* 519 */
    "audgen8",              /* 520 */
    "audit_setup",          /* 521 */
    "login",                /* 522 */
    "XServerStartup",       /* 523 */
    "XServerShutdown",      /* 524 */
    "XServerDac",           /* 525 */
    "XClientStartup",       /* 526 */
    "XClientShutdown",      /* 527 */
    "XClientIPC",           /* 528 */
    "XObjectCreate",        /* 529 */
    "XObjectRename",        /* 530 */
    "XObjectDestroy",       /* 531 */
    "XObjectDac",           /* 532 */
    "XObjectRead",          /* 533 */
    "XObjectWrite",         /* 534 */
    "auth_event",           /* 535 */
    "audit_start",          /* 536 */
    "#537",                 /* 537 */
    "#538",                 /* 538 */
    "#539",                 /* 539 */
    "#540",                 /* 540 */
    "#541",                 /* 541 */
    "#542",                 /* 542 */
    "#543",                 /* 543 */
    "#544",                 /* 544 */
    "#545",                 /* 545 */
    "#546",                 /* 546 */
    "#547",                 /* 547 */
    "#548",                 /* 548 */
    "#549",                 /* 549 */
    "#550",                 /* 550 */
    "#551",                 /* 551 */
    "#552",                 /* 552 */
    "#553",                 /* 553 */
    "#554",                 /* 554 */
    "#555",                 /* 555 */
    "#556",                 /* 556 */
    "#557",                 /* 557 */
    "#558",                 /* 558 */
    "#559",                 /* 559 */
    "#560",                 /* 560 */
    "#561",                 /* 561 */
    "#562",                 /* 562 */
    "#563",                 /* 563 */
    "#564",                 /* 564 */
    "#565",                 /* 565 */
    "#566",                 /* 566 */
    "#567",                 /* 567 */
    "#568",                 /* 568 */
    "#569",                 /* 569 */
    "#570",                 /* 570 */
    "#571",                 /* 571 */
    "#572",                 /* 572 */
    "#573",                 /* 573 */
    "#574",                 /* 574 */
    "#575"                  /* 575 */
};
