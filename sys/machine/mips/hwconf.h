/*
 * static	char	*sccsid = "@(#)hwconf.h	4.2	(ULTRIX)	9/4/90";
 */
/************************************************************************
 *									*
 *			Copyright (c) 1989 by				*
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

/*
 * Copyright 1985 by MIPS Computer Systems, Inc.
 */

/*
 * hdwconf.h -- hardware specific configuration information
 *
 * Modification History: 
 *
 * 07-Apr-89 -- afd
 *	Move macros for getting items from "systype" word to cpuconf.h
 *	Move defines for R2000a cpu type and PMAX systype to cpuconf.h
 *
 * 09-Nov-88 -- afd
 *	Add macros for getting items from "systype" word.
 *	Add defines for R2000a cpu type and PMAX systype.
 */
#ifdef KERNEL
#include "../h/ansi_compat.h"
#else
#include <ansi_compat.h>
#endif

/*
 * revision id for chips
 */
union rev_id {
	unsigned int	ri_uint;
	struct {

#ifdef __MIPSEB
		unsigned int	Ri_fill:16,
				Ri_imp:8,		/* implementation id */
				Ri_majrev:4,		/* major revision */
				Ri_minrev:4;		/* minor revision */
#endif /* __MIPSEB */
#ifdef __MIPSEL
		unsigned int	Ri_minrev:4,		/* minor revision */
				Ri_majrev:4,		/* major revision */
				Ri_imp:8,		/* implementation id */
				Ri_fill:16;
#endif /* __MIPSEL */
	} Ri;
};
#define	ri_imp		Ri.Ri_imp
#define	ri_majrev	Ri.Ri_majrev
#define	ri_minrev	Ri.Ri_minrev

struct imp_tbl {
	char *it_name;
	unsigned it_imp;
};

/*
 * NVRAM information
 */
#define ENV_MAXLEN	32
#define ENV_ENTRIES	6
struct promenv {
	char	name[ENV_MAXLEN];
	char	value[ENV_MAXLEN];

};

/*
 * contains configuration information for all hardware in system
 */
struct hw_config {
	unsigned	icache_size;
	unsigned	dcache_size;
	union rev_id	cpu_processor;
	union rev_id	fpu_processor;
	unsigned char	cpubd_type;
	unsigned char	cpubd_rev;
	char		cpubd_snum[5];
	int		cpubd_config;
	struct promenv	promenv[ENV_ENTRIES];
#ifdef TODO
	add memory board id prom information
#endif /* TODO */
};

/*
 * options to hdwconf() syscall
 */
#define HWCONF_GET	0
#define HWCONF_SET	1

#ifndef LOCORE
#ifdef KERNEL
extern struct hw_config hwconf;
#endif /* KERNEL */
#endif /* !LOCORE */
