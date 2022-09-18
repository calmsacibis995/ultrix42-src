/*
#ifndef lint
static  char    *sccsid = "@(#)defs.h	4.1  (ULTRIX)        7/17/90";
#endif lint
*/

/*
 * Internal data structure definitions for SNMP Extended Agent.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <netdb.h>

#include <arpa/inet.h>
#include <protocols/snmp.h>
#include <protocols/snmperrs.h>

/* Supported MIB */
#define	MIB_VM		0
#define	MIB_DISK	1

/* Namelist definition. */
#define X_SUM		0
#define	X_TOTAL		1
#define	X_BOOTTIME	2
