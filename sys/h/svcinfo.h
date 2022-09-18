/*	@(#)svcinfo.h	4.1	(ULTRIX)	7/2/90	*/
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

#define SVC_CONF "/etc/svc.conf"

/* databases indeces for svcadmin and svcpath in svcinfo*/
#define SVC_ALIASES 0
#define SVC_AUTH 1
#define SVC_GROUP 2
#define SVC_HOSTS 3
#define SVC_NETGROUP 4 
#define SVC_NETWORKS 5
#define SVC_PASSWD 6
#define SVC_PROTOCOLS 7
#define SVC_RPC 8
#define SVC_SERVICES 9

#define SVC_DATABASES 20
#define SVC_PATHSIZE 8

/* sources stored in array svcpath*/
#define SVC_LOCAL 0
#define SVC_YP 1
#define SVC_BIND 2
#define SVC_LAST 99

/* values for seclevel */
#define SEC_BSD 0
#define SEC_UPGRADE 1
#define SEC_ENHANCED 22

#define MAX_PASSWORD_LENGTH 16

extern int svc_lastlookup;
struct svcinfo {
        int svcdate;            /* Last mod date of /etc/svc.conf */


	int	svcpath[SVC_DATABASES][SVC_PATHSIZE]; /* indexed by databases
						 and choice 0=first choice
						 1=second choice, etc
						 value stored is source */

	struct  {
		int passlenmin;
		int passlenmax;
		int softexp;
		int seclevel;
	} svcauth;

};

extern struct svcinfo * getsvc();
extern int init_svc();

/**********  /etc/svc.conf file format **************/
/*
NOTE: white space allowed only after commas or newlines

database=service,
	service,
	service

database=service,service,service

PASSLENMIN=6
PASSLENMAX=16
SOFTEXP=604800
SECLEVEL=ENHANCED        (BSD | UPGRADE | ENHANCED)


database = aliases | auth | group | hosts | netgroup | networks |
	  passwd | protocols | rpc | services
source =  local | yp | bind

*/
/*
 * getsvc() libc routines
 *
 * #include <sys/svcinfo.h>
 *
 * struct svcinfo * getsvc()
 * int init_svc()
 *
 * getsvc() returns a pointer to the svcinfo structure.
 * If there is no svcinfo structure, then the /etc/svc.conf file
 * is parsed.  On failure getsvc() returns 0.
 */
