/*
 @(#)cippdscs.h	4.1  (ULTRIX)        7/2/90
 */

/************************************************************************
 *                                                                      *
 *                      Copyright (c) 1988 - 1989 by                    *
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
 *                                                                      *
 ************************************************************************
 *
 *
 *   Facility:	Systems Communication Architecture
 *		Computer Interconnect Port-to-Port Driver
 *
 *   Abstract:	This module contains Computer Interconnect Port-to-Port
 *		Driver( CI PPD ) data structure definitions visible to SCS.
 *
 *   Creator:	Todd M. Katz	Creation Date:	July 31, 1987
 *
 *   Modification History:
 *
 *   17-Jan-1989	Todd M. Katz		TMK0004
 *	1. Add padding when it is necessary to keep longword alignment.  While
 *	   some space is wasted such alignment is essential for ports of SCA to
 *	   hardware platforms which require field alignment and access type to
 *	   match( ie- only longword aligned entities may be longword accessed).
 *	2. Add the Finite State Machine status flag "nosanity_chk".  A local
 *	   port sets this bit when it wants all sanity checking of the port to
 *	   be bypassed.
 *
 *   17-Aug-1988	Todd M. Katz		TMK0003
 *	1. Rename structure errlogopt -> elogopt within CIPPDPCCB.
 *	2. Add fields contact( CI PPD port polling contact frequency ), burst(
 *	   CI PPD port polling burst size ), and ppddgs( Number of CI PPD
 *	   datagrams remaining to be allocated ) to structure CIPPDPCCB.  Add
 *	   field sysapname to union elogopt within structure CIPPDPCCB.
 *
 *   16-May-1988	Todd M. Katz		TMK0002
 *	1. Add the following bit maps to the CIPPDPCCB for the purpose of
 *	   keeping track of failures during path establishment on a per-path
 *	   basis:
 *		1) aflogmap  - tracks memory allocation failures.
 *		2) tmologmap - tracks protocol sequence timeouts.
 *	2. Add rswtype( remote system software type ) to the union declaration
 *	   errlogopt within CIPPDPCCB.
 *	3. Add cleanup and fkip as CIPPDPCCB finite state machine status flags.
 *
 *   08-Jan-1988	Todd M. Katz		TMK0001
 *	Formated module, revised comments, increased robustness, made CI
 *	PPD and GVP completely independent from underlying port drivers, and
 *	added SMP support.
 */

/* CI PPD Data Structure Definitions.
 */
typedef	struct _cippdpccb	{	/* CI PPD Specific Fields of PCCB    */
    struct _pbq	form_pb;		/* Formative path PB queue head	     */
    struct _pbq *open_pb[ CIPPD_MAXPATHS ]; /* PS_OPEN PB vector table	     */
    struct	{			/* Finite state machine status flags */
	u_long	fkip		:  1;	/*  Fork operation in progress	     */
	u_long	online		:  1;	/*  Port is online		     */
	u_long	sanity		:  1;	/*  Port polling in progress	     */
	u_long	nosanity_chk	:  1;	/*  Skip sanity checking	     */
	u_long	timer		:  1;	/*  Port timer is active	     */
	u_long	broken		:  1;	/*  Port is permanently broken	     */
	u_long	cleanup	 	:  1;	/*  Clean up in progress	     */
	u_long			: 25;
    } fsmstatus;
    u_short	contact;		/* Port polling contact frequency    */
    u_short	burst;			/* Port polling burst size	     */
    short	poll_due;		/* Port polling interval timer	     */
    short	poll_interval;		/* Current port polling interval     */
    short	timer_interval;		/* Current timer interval	     */
    u_char	next_port;		/* Next port to poll		     */
    u_char	poll_cable;		/* Port cable to use for polling     */
#define	ANY_CABLE	0		/*  Any polling cable		     */
#define	FIRST_CABLE	1		/*  First polling cable		     */
    u_char	max_cables;		/* Maximum number of CI cables	     */
    u_char	sanity_port;		/* REQID port being sanity checked   */
    u_short	ppddgs;			/* CI PPD datagrams left to allocate */
    u_char      aflogmap[ CIPPD_MAPSIZE ];/* Path establish no mem log map   */
    u_char      tmologmap[ CIPPD_MAPSIZE ];/* Path establish timeout log map */
    union		      {		/* Event logging optional information*/
	struct cippd_protocol protocol;	/*  CI PPD protocol information      */
	struct cippd_dbcoll   dbcoll;	/*  Database collision information   */
	u_long		      port_num;	/*  Remote port number		     */
	u_long		      rswtype;	/*  Remote system software type	     */
	u_char sysapname[ NAME_SIZE ];	/*  Name of local SYSAP crashing path*/
    } elogopt;
} CIPPDPCCB;

typedef struct _cippdpb	{		/* CI PPD Specific Fields of PB	     */
    struct	{			/* FSM path status flags	     */
	u_long	timer		:  1;	/*  CI PPD traffic timer activated   */
	u_long	path_closed	:  1;	/*  Path already closed by port	     */
	u_long	fkip		:  1;	/*  Fork operation in progress	     */
	u_long			: 29;
    } fsmpstatus;
    u_short	     dbiip;		/* SCA database insertion semaphore  */
    u_short	   		: 16;	/*  ( must be longword aligned )     */
    short	     due_time;		/* CI PPD traffic interval timer     */
    u_short	     retry;		/* REQID/SEND/STACK retry count	     */
    struct kschedblk forkb;		/* Fork block for path clean up	     */
} CIPPDPB;
