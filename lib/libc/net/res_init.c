#ifndef lint
static	char	*sccsid = "@(#)res_init.c	4.2	(ULTRIX)	9/4/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1987-1990 by			*
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
static char sccsid[] = "@(#)res_init.c	6.6 (Berkeley) 5/12/87";
*/

/*
 * Modification History:
 *
 * 06-Mar-90	sue
 *	Removed a warning message when cannot find domain name in 
 *	/etc/resolv.conf or hostname.
 *
 * 13-Nov-89	logcher
 *	Changed the checking for res_conffile to check if file exists,
 *	then check the time of the file before opening.  This is to
 *	reduce the number of times this file is opened and closed.
 *
 * 21-Jan-88	logcher
 *	Changed the name conffile to res_conffile because of a
 *	build break with the same name in /etc/inetd.c
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <stdio.h>
#ifdef AUTHEN
#include <netdb.h>
#endif AUTHEN
#include <arpa/nameser.h>
#include <resolv.h>

#if defined(lint) && !defined(DEBUG)
#define DEBUG
#endif

extern cred_st *cred;

/*
 * Resolver configuration file. Contains the address of the
 * inital name server to query and the default domain for
 * non fully qualified domain names.
 */

#ifdef CONFFILE
char    *res_conffile = CONFFILE;
#else
char    *res_conffile = "/etc/resolv.conf";
#endif

/*
 * Resolver state default settings
 */

struct state _res = {
    RES_TIMEOUT,               	/* retransmition time interval */
    4,                         	/* number of times to retransmit */
    RES_DEFAULT,		/* options flags */
    1,                         	/* number of name servers */
};

/*
 * Set up default settings.  If the configuration file exist, the values
 * there will have precedence.  Otherwise, the server address is set to
 * INADDR_ANY and the default domain name comes from the gethostname().
 *
 * The configuration file should only be used if you want to redefine your
 * domain or run without a server on your machine.
 *
 * Return 0 if completes successfully, -1 on error
 */
res_init()
{
    register FILE *fp;
    register char *cp, **pp;
    char buf[BUFSIZ];
#ifdef AUTHEN
    char name[BUFSIZ];
#endif AUTHEN
    extern u_long inet_addr();
    extern char *index();
    extern char *strcpy(), *strncpy();
    extern char *getenv();
    int n = 0;    /* number of nameserver records read from file */
    struct stat res_stat;

#ifdef AUTHEN
    int looper;
    char *temp;
    struct hostent *host_st;
    struct in_addr addr;
#endif AUTHEN

    if (stat(res_conffile, &res_stat) == 0)
	if ((res_stat.st_mtime != (time_t)_res.mtime) || (_res.defdname[0] == '\0')) {
    	    _res.nsaddr.sin_addr.s_addr = INADDR_ANY;
    	    _res.nsaddr.sin_family = AF_INET;
    	    _res.nsaddr.sin_port = htons(NAMESERVER_PORT);
    	    _res.nscount = 1;
    	    _res.defdname[0] = '\0';

	    if ((fp = fopen(res_conffile, "r")) != NULL) {
       		/* read the config file */
       		while (fgets(buf, sizeof(buf), fp) != NULL) {
		    /* read default domain name */
		    if (!strncmp(buf, "domain", sizeof("domain") - 1)) {
			cp = buf + sizeof("domain") - 1;
			while (*cp == ' ' || *cp == '\t')
			    cp++;
			if (*cp == '\0')
			    continue;
			(void)strncpy(_res.defdname, cp, sizeof(_res.defdname));
			_res.defdname[sizeof(_res.defdname) - 1] = '\0';
			if ((cp = index(_res.defdname, '\n')) != NULL)
			    *cp = '\0';
			continue;
		    }
		    /* read nameservers to query */
		    if (!strncmp(buf, "nameserver", sizeof("nameserver") - 1) && (n < MAXNS)) {
			cp = buf + sizeof("nameserver") - 1;
			while (*cp == ' ' || *cp == '\t')
			    cp++;
			if (*cp == '\0')
			    continue;

#ifdef AUTHEN
			/* call gethostbyname local version to get the
			 * domain names of the hosts whose addresses
			 * appear in the resolv.conf file
			 */

/*			temp = cp;
			while (*temp != ' ' && *temp != '\t' && *temp != '\n')
			    temp++;*/
			addr.s_addr = inet_addr(cp);

			if ((host_st = (struct hostent *)gethostbyaddr_local( &(addr.s_addr), sizeof(struct in_addr), AF_INET)) == NULL)
			    return(-1);

			if(!strcmp(host_st->h_name, "localhost")) {
			    gethostname(name, sizeof(name));
			    strcpy(_res.ns_list[n].dname, name);
			} else {
			    strcpy(_res.ns_list[n].dname, host_st->h_name);
			}
			res_dotname_head(_res.ns_list[n].dname);
#endif AUTHEN

			_res.ns_list[n].addr.sin_addr.s_addr = inet_addr(cp);
			if (_res.ns_list[n].addr.sin_addr.s_addr == (unsigned)-1) 
			    _res.ns_list[n].addr.sin_addr.s_addr = INADDR_ANY;
			_res.ns_list[n].addr.sin_family = AF_INET;
			_res.ns_list[n].addr.sin_port = htons(NAMESERVER_PORT);

			if ( ++n >= MAXNS) { 
			    n = MAXNS;
#ifdef DEBUG
			if ( _res.options & RES_DEBUG )
			    printf("MAXNS reached, reading resolv.conf\n");
#endif DEBUG
			}
			continue;
		    }
		}
		if ( n > 1 ) 
		    _res.nscount = n;
		(void) fclose(fp);
		/*
		 * Set the mtime to the time of the file
		 */
		(time_t)_res.mtime = res_stat.st_mtime;
	    }
    }
    if (_res.defdname[0] == 0) {
        if (gethostname(buf, sizeof(_res.defdname)) == 0 && (cp = index(buf, '.')))
            (void)strcpy(_res.defdname, cp + 1);
	else
	    return(-1);
    }

    /* Allow user to override the local domain definition */
    if ((cp = getenv("LOCALDOMAIN")) != NULL)
        (void)strncpy(_res.defdname, cp, sizeof(_res.defdname));

    /* find components of local domain that might be searched */
    pp = _res.dnsrch;
    *pp++ = _res.defdname;
    for (cp = _res.defdname, n = 0; *cp; cp++)
	if (*cp == '.')
	    n++;
    cp = _res.defdname;
    for (; n >= LOCALDOMAINPARTS && pp < _res.dnsrch + MAXDNSRCH; n--) {
	cp = index(cp, '.');
	*pp++ = ++cp;
    }
    _res.options |= RES_INIT;

#ifdef AUTHEN
    /*	 Hard code the default auth field for now. Eventually add it to the 
	 res.conf file
        authentication	kerberos one

	*/

    _res.defauthtype = AUTH_KRB;
    _res.defauthversion = ONE;

    
    if((cred = (cred_st *)malloc(sizeof(cred_st) * _res.nscount)) == NULL) {
#ifdef DEBUG
	    if (_res.options & RES_DEBUG)
		    perror("bad malloc in res_init");
#endif DEBUG	    
	    
	    return(-1);
    }

    for (looper = 0; looper <  _res.nscount; looper++) {
	    cred[looper].len_ar = 0;
	    cred[looper].buf[0] = '\0';
    }
#endif AUTHEN

    return(0);
}
