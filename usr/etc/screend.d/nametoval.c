#ifndef lint
static char *sccsid = "@(#)nametoval.c	4.2	(ULTRIX)	10/12/90";
#endif
/*
 * nametoval.c
 *
 * Translations from an IP sort of name to a number
 *
 * Modification history:
 *
 * 19 December 1988	Jeffrey Mogul/DECWRL
 *	Created.
 *
 */
/*
 *               Copyright 1989, 1990 Digital Equipment Corporation
 *                          All Rights Reserved
 * 
 * 
 * Permission to use, copy, and modify this software and its documentation
 * is hereby granted only under the following terms and conditions.  Both
 * the above copyright notice and this permission notice must appear in
 * all copies of the software, derivative works or modified versions, and
 * any portions threof, and both notices must appear in supporting
 * documentation.
 * 
 * Users of this software agree to the terms and conditions set forth herein,
 * and hereby grant back to Digital a non-exclusive, unrestricted, royalty-free
 * right and license under any changes, enhancements or extensions made to the
 * core functions of the software, including but not limited to those affording
 * compatibility with other hardware or software environments, but excluding
 * applications which incorporate this software.  Users further agree to use
 * their best efforts to return to Digital any such changes, enhancements or
 * extensions that they make and inform Digital of noteworthy uses of this
 * software.  Correspondence should be provided to Digital at:
 * 
 *                       Director of Licensing
 *                       Western Research Laboratory
 *                       Digital Equipment Corporation
 *                       100 Hamilton Avenue
 *                       Palo Alto, California  94301  
 * 
 * Comments and bug reports may also be sent using electronic mail to:
 * 			screend-reports@decwrl.dec.com
 * 
 * 	>> This software may NOT be distributed to third parties. <<
 *   
 * THE SOFTWARE IS PROVIDED "AS IS" AND DIGITAL EQUIPMENT CORP. DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS.   IN NO EVENT SHALL DIGITAL
 * EQUIPMENT CORPORATION BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netdb.h>
#include <stdio.h>

NetNameToNumber(name)
char *name;
{
	struct netent *nep;
	
	nep = getnetbyname(name);
	if (nep == NULL) {
	    name_error(name, "net");
	    return(0);
	}
	return(nep->n_net);
}

MaskNameToNumber(name)
char *name;
{
	struct hostent *hep;
	struct in_addr ia;
	
	hep = gethostbyname(name);
	if (hep == NULL) {
	    name_error(name, "netmask");
	    return(0);
	}
	bcopy(hep->h_addr_list[0], &ia, sizeof(ia));
	return(ia.s_addr);
}

SubnetNameToNumber(name)
char *name;
{
	struct netent *nep;
	
	nep = getnetbyname(name);
	if (nep == NULL) {
	    name_error(name, "subnet");
	    return(0);
	}
	return(nep->n_net);
}

HostNameToNumber(name)
char *name;
{
	struct hostent *hep;
	struct in_addr ia;
	
	hep = gethostbyname(name);
	if (hep == NULL) {
	    name_error(name, "host");
	    return(0);
	}
	bcopy(hep->h_addr_list[0], &ia, sizeof(ia));
	if (hep->h_addr_list[1]) {
	    warn_multi(name, hep->h_addr_list[0]);
	    /* XXX fix this right! XXX */
	}
	return(ia.s_addr);
}

PortNameToNumber(name)
char *name;
{
	struct servent *sep;
	
	sep = getservbyname(name, NULL);	/* XXX Fishy XXX */
	if (sep == NULL) {
	    name_error(name, "port");
	    return(0);
	}
	return(sep->s_port);
}

char *
PortNumberToName(number)
int number;
{
	struct servent *sep;
	
	sep = getservbyport(number, NULL);	/* XXX Fishy XXX */
	if (sep == NULL) {
	    return(NULL);
	}
	return(sep->s_name);
}

ProtoNameToNumber(name)
char *name;
{
	struct protoent *pep;
	
	pep = getprotobyname(name);
	if (pep == NULL) {
	    name_error(name, "protocol");
	    return(0);
	}
	return(pep->p_proto);
}

char *
ProtoNumberToName(number)
char *number;
{
	struct protoent *pep;
	
	pep = getprotobynumber(number);
	if (pep == NULL) {
	    return(NULL);
	}
	return(pep->p_name);
}

struct ICMPName {
	char *name;
	char *abbrev;
	int type;
};
struct ICMPName ICMPNames[] = {
	/* XXX the long string names are pretty bad XXX */
	{"echoreply", "EchoRep", ICMP_ECHOREPLY},
	{"unreachable", "Unreach", ICMP_UNREACH},
	{"sourcequench", "Quench", ICMP_SOURCEQUENCH},
	{"redirect", "Redirect", ICMP_REDIRECT},
	{"echo", "EchoReq", ICMP_ECHO},
	{"timeexceeded", "TimeXcd", ICMP_TIMXCEED},
	{"parameterproblem", "Param", ICMP_PARAMPROB},
	{"timestamp", "TStampReq", ICMP_TSTAMP},
	{"timestampreply", "TStampRep", ICMP_TSTAMPREPLY},
	{"informationrequest", "InfoReq", ICMP_IREQ},
	{"informationrreply", "InfoRep", ICMP_IREQREPLY},
	{"addressmaskrequest", "MaskReq", ICMP_MASKREQ},
	{"addressmaskreply", "MaskRep", ICMP_MASKREPLY},
	{0,0}
};

ICMPNameToNumber(name)
register char *name;
{
	register struct ICMPName *icmpnp = ICMPNames;
	
	while (icmpnp->name) {
		if (strcmp(icmpnp->name, name) == 0)
			return(icmpnp->type);
		icmpnp++;
	}
	name_error(name, "ICMP type");
	return(0);
}

char *
ICMPNumberToName(number)
register int number;
{
	register struct ICMPName *icmpnp = ICMPNames;
	
	while (icmpnp->name) {
		if (icmpnp->type == number)
			return(icmpnp->name);
		icmpnp++;
	}
	return(NULL);
}

char *
ICMPNumberToAbbrev(number)
register int number;
{
	register struct ICMPName *icmpnp = ICMPNames;
	
	while (icmpnp->name) {
		if (icmpnp->type == number)
			return(icmpnp->abbrev);
		icmpnp++;
	}
	return(NULL);
}

name_error(name, kind)
char *name;
char *kind;
{
	static char message[128];
	
	sprintf(message, "%s `%.64s' not found", kind, name);
	yyerror(message);
}

warn_multi(name, iap)
char *name;
struct in_addr *iap;
{
	static char message[128];
	
	sprintf(message,
	   "Warning: host %s has multiple addresses, using only [%s]", name,
					inet_ntoa(iap->s_addr));
	yywarn(message);
}
