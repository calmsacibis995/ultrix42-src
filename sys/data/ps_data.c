/*
 * @(#)ps_data.c	4.1	(ULTRIX)	7/2/90
 */
/************************************************************************
 *									*
 *			Copyright (c) 1985 by				*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any  other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   This software is  derived  from  software  received  from  the	*
 *   University    of   California,   Berkeley,   and   from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is  subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

#include "ps.h"

#define EXTERNAL_SYNC

#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/ioctl.h"
#include "../h/map.h"
#include "../h/buf.h"
#include "../h/conf.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/uio.h"

#include "../io/uba/ubareg.h"
#include "../io/uba/ubavar.h"
#include "../io/uba/psreg.h"

#define MAXAUTOREFRESH			(4)
#define MAXAUTOMAP			(4)
#define MAXDBSIZE			(0177777/2)

#ifdef	BINARY
extern	struct	uba_device *psdinfo[];

extern	struct ps {
	char		ps_open;
	short int 	ps_uid;
	struct {
		enum { SINGLE_STEP_RF, AUTO_RF, TIME_RF } state;
		enum { RUNNING_RF, SYNCING_RF, WAITING_MAP, STOPPED_RF } mode;
		unsigned short int sraddrs[MAXAUTOREFRESH];
		short int nsraddrs;
		short int srcntr;
		char waiting;
		char stop;
		int icnt;
		int timecnt;
	} ps_refresh;
	struct {
		enum { ON_DB, OFF_DB } state;
		unsigned short int dbaddrs[2];
		unsigned short int dbsize;
		short int rbuffer;
	} ps_dbuffer;
	struct {
		enum { SINGLE_STEP_MAP, AUTO_MAP } state;
		enum { RUNNING_MAP, WAITING_RF, WAITING_START } mode;
		unsigned short int maddrs[MAXAUTOMAP];
		short int nmaddrs;
		short int mcntr;
		short int outputstart;
		char waiting;
		char stop;
		int icnt;
	} ps_map;
	struct {
		short int ticked;
		short int missed;
		int icnt;
	} ps_clock;
	struct {
		int icnt;
	} ps_hit;
	int ps_strayintr;
	int last_request;
	int strayrequest;
	int ps_icnt;
} ps[];
extern	int	nNPS;
#else

struct	uba_device *psdinfo[NPS];

struct ps {
	char		ps_open;
	short int 	ps_uid;
	struct {
		enum { SINGLE_STEP_RF, AUTO_RF, TIME_RF } state;
		enum { RUNNING_RF, SYNCING_RF, WAITING_MAP, STOPPED_RF } mode;
		unsigned short int sraddrs[MAXAUTOREFRESH];
		short int nsraddrs;
		short int srcntr;
		char waiting;
		char stop;
		int icnt;
		int timecnt;
	} ps_refresh;
	struct {
		enum { ON_DB, OFF_DB } state;
		unsigned short int dbaddrs[2];
		unsigned short int dbsize;
		short int rbuffer;
	} ps_dbuffer;
	struct {
		enum { SINGLE_STEP_MAP, AUTO_MAP } state;
		enum { RUNNING_MAP, WAITING_RF, WAITING_START } mode;
		unsigned short int maddrs[MAXAUTOMAP];
		short int nmaddrs;
		short int mcntr;
		short int outputstart;
		char waiting;
		char stop;
		int icnt;
	} ps_map;
	struct {
		short int ticked;
		short int missed;
		int icnt;
	} ps_clock;
	struct {
		int icnt;
	} ps_hit;
	int ps_strayintr;
	int last_request;
	int strayrequest;
	int ps_icnt;
} ps[NPS];

int	nNPS = NPS;
#endif
