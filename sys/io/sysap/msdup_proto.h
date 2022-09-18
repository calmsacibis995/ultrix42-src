/*	@(#)msdup_proto.h	4.1	(ULTRIX)	2/19/91	*/

/************************************************************************
 *                                                                      *
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
 *                                                                      *
 ************************************************************************/
/*
 *   Facility:	Systems Communications Architecture
 *		Diagnostic/Utilities Protocol (DUP) Class Driver
 *
 *   Abstract:	This file contains the routines specific to the DUP
 *		class driver.
 *
 *   Author:    Mayank Prakash		    5/24/90
 *
 *   $Header$
 *
 *   $Log$
 *
 */

#ifndef MSDUP_proto.h
#define MSDUP_proto.h	0

typedef enum 
  {SVR_OFF, SVR_AVL, SVR_ON_IDLE, SVR_ON_ACTIVE_RUN, SVR_ON_ACTIVE_WAIT}
  MSDUP_STATES;

/* In all the macros below, s is a pointer to the connection block to
   the DUP server. */

#define SvrState(s)  ((s)->state)		  /* The current server state */

/*
 * Test macros.
 * The following macros test the current state.
 */
#define SvrOffLine(s) (SvrState((s)) == SVR_OFF)
#define SvrAvailable(s) (SvrState((s)) == SVR_AVL)
#define SvrIdle(s) (SvrState((s)) ==  SVR_ON_IDLE)
#define SvrActiveRun(s) (SvrState((s)) == SVR_ON_ACTIVE_RUN)
#define SvrActiveWait(s) (SvrState((s)) == SVR_ON_ACTIVE_WAIT)

/*
 * Compound states.
 */
#define SvrActive(s) (SvrActiveRun((s)) || SvrActiveWait((s)))
#define SvrConnected(s) (SvrIdle((s)) || SvrActive((s)))
#define SvrDisconnected(s) (SvrAvailable((s)) || SvrOffLine((s)))
#define SvrDisconnecting(s) (SvrStateInTransit((s)) && SvrNextAvailable((s)))
#define SvrDown(s) (SvrDisconnected((s)) ||  SvrDisconnecting((s)))
/*
 * The following macros set the server state.
 */
#define SvrSetOffLine(s) {SvrState((s)) = SVR_OFF;}
#define SvrSetAvailable(s) {SvrState((s)) = SVR_AVL;}
#define SvrSetIdle(s) {SvrState((s)) =  SVR_ON_IDLE;}
#define SvrSetActiveRun(s) {SvrState((s)) = SVR_ON_ACTIVE_RUN;}
#define SvrSetActiveWait(s) {SvrState((s)) = SVR_ON_ACTIVE_WAIT;}

/*
 * Test, set, or clear the server state in transit flag.
 */
#define SvrStateInTransit(s) (((s))->inTransit != 0)
#define SvrSetInTransit(s) {((s))->inTransit = 1;}
#define SvrClrInTransit(s) {((s))->inTransit = 0;}

/*
 * Next state is the state the server will be in if the current state
 * changing event completes succesfully.
 * When we set the next state, we also want to mark the server as being
 * in transit. When the state changing event has succesfully completed, 
 * the current state should be set to the next state, and the server should
 * be marked as not being in transit any more.
 */
#define SvrNextState(s) (((s))->nextState)

#define SvrNextAvailable(s) (SvrNextState((s)) == SVR_AVL)
#define SvrNextActiveRun(s) (SvrNextState((s)) == SVR_ON_ACTIVE_RUN)

#define SvrNextSetOffLine(s) {SvrNextState((s)) = SVR_OFF; SvrSetInTransit((s));}
#define SvrNextSetAvailable(s) {SvrNextState((s)) = SVR_AVL; SvrSetInTransit((s));}
#define SvrNextSetIdle(s) {SvrNextState((s)) =  SVR_ON_IDLE; SvrSetInTransit((s));}
#define SvrNextSetActiveRun(s) {SvrNextState((s)) = SVR_ON_ACTIVE_RUN; SvrSetInTransit((s));}
#define SvrNextSetActiveWait(s) {SvrNextState((s)) = SVR_ON_ACTIVE_WAIT; SvrSetInTransit((s));}
#define SvrSetNext(s, status) if (status == 0) {SvrState((s)) = SvrNextState((s));};\
			       SvrClrInTransit((s))

       
#endif	/* MSDUP_proto.h */
