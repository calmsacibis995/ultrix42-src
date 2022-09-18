#ifndef lint
static	char	*sccsid = "@(#)hesiod.c	4.1	(ULTRIX)	7/3/90";
#endif lint
/************************************************************************
 *									*
 *			Copyright (c) 1984-1990 by			*
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
 * This file is part of the Hesiod library.
 *
 * Revision 1.7  89/11/16  06:49:31  probe
 * Uses T_TXT, as defined in the RFC.
 * 
 */
/*
 * Modification History:
 *
 * 07-Feb-90	sue
 *	Incorporated Hesiod changes from MIT to implement multiple
 *	"character strings" in a TXT record according to RFC1035.
 *
 * 13-Nov-89	sue
 *	Added fcloses of HesConfigfile.
 *
 * 14-Apr-89	logcher
 *	Added Hesiod routines.
 */

#include <stdio.h>
#include <errno.h>
#include <strings.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include <resscan.h>
#include <hesiod.h>

#define USE_HS_QUERY	/* undefine this if your higher-level name servers */
			/* don't know class HS */

char *HesConfigFile = HESIOD_CONF;
static char *Hes_LHS, *Hes_RHS;
static int Hes_Errno = HES_ER_UNINIT;

retransXretry_t NoRetryTime = { 0, 0};

hes_init()
{
	register FILE *fp;
	register char *key, *cp, **cpp;
	int len;
	char buf[MAXDNAME+7];
	char *calloc();

	Hes_Errno = HES_ER_UNINIT;
	Hes_LHS = NULL; Hes_RHS = NULL;
	if ((fp = fopen(HesConfigFile, "r")) == NULL) {
		/*
		 * use defaults compiled in
		 * no file or no access uses defaults
		 * but poorly formed file returns error
		 */
		Hes_LHS = DEF_LHS; Hes_RHS = DEF_RHS;
		if (Hes_RHS == NULL)
			Hes_Errno = HES_ER_CONFIG;
		else
			Hes_Errno = HES_ER_OK;
		return(Hes_Errno);
	}
	while(fgets(buf, MAXDNAME+7, fp) != NULL) {
		cp = buf;
		if (*cp == '#' || *cp == '\n')
			continue;
		while(*cp == ' ' || *cp == '\t')
			cp++;
		key = cp;
		while(*cp != ' ' && *cp != '\t' && *cp != '=')
			cp++;
		*cp++ = '\0';
		if (strcmp(key, "lhs") == 0)
			cpp = &Hes_LHS;
		else
			if (strcmp(key, "rhs") == 0)
				cpp = &Hes_RHS;
			else
				 continue;
		while(*cp == ' ' || *cp == '\t' || *cp == '=')
			cp++;
		if ((strcmp(key, "rhs") == 0) && (*cp != '.')) {
			Hes_Errno = HES_ER_CONFIG;
			if (fclose(fp) == EOF)
				Hes_Errno = HES_ER_UNINIT;
			return(Hes_Errno);
		}
		len = strlen(cp);
		*cpp = calloc((unsigned int) len, sizeof(char));
		(void) strncpy(*cpp, cp, len-1);
	}
	/* the LHS may be null, the RHS must not be null */
	if (Hes_RHS == NULL)
		Hes_Errno = HES_ER_CONFIG;
	else
		Hes_Errno = HES_ER_OK;
	if (fclose(fp) == EOF)
		Hes_Errno = HES_ER_UNINIT;
	return(Hes_Errno);
}

char *
hes_to_bind(HesiodName, HesiodNameType)
char *HesiodName, *HesiodNameType;
{
	register char *cp, **cpp;
	static char bindname[MAXDNAME];
	char *RHS;

	if (Hes_Errno == HES_ER_UNINIT || Hes_Errno == HES_ER_CONFIG)
		(void) hes_init();
	if (Hes_Errno == HES_ER_CONFIG)
		return(NULL);
	if (cp = index(HesiodName,'@')) {
		if (index(++cp,'.'))
			RHS = cp;
		else
			if (cpp = hes_resolve(cp, "rhs-extension"))
				RHS = *cpp;
			else {
				Hes_Errno = HES_ER_NOTFOUND;
				return(NULL);
			}
		(void) strcpy(bindname,HesiodName);
		*index(bindname,'@') = '\0';
	}
	else {
		RHS = Hes_RHS;
		(void) strcpy(bindname, HesiodName);
	}
	(void) strcat(bindname, ".");
	(void) strcat(bindname, HesiodNameType);
	if (Hes_LHS != NULL && Hes_LHS[0] != '\0') {
		if (Hes_LHS[0] != '.')
			(void) strcat(bindname,".");
		(void) strcat(bindname, Hes_LHS);
	}
	if (RHS[0] != '.')
		(void) strcat(bindname,".");
	(void) strcat(bindname, RHS);
	return(bindname);
}

char **
hes_resolve(HesiodName, HesiodNameType)
char *HesiodName, *HesiodNameType;
{
	register char *cp;
	static char *retvec[100];
	char *ocp, *dst;
	char *calloc();
	int i, j, n;
	struct nsmsg *ns, *_resolve();
	rr_t *rp;
	extern int errno;

	cp = hes_to_bind(HesiodName, HesiodNameType);
	if (cp == NULL)
		return(NULL);
	errno = 0;
	ns = _resolve(cp, C_HS, T_TXT, NoRetryTime);
	if (errno == ETIMEDOUT || errno == ECONNREFUSED) {
		Hes_Errno = HES_ER_NET;
		return(NULL);
	}
	if (ns == NULL || ns->ns_off <= 0) {
		Hes_Errno = HES_ER_NOTFOUND;
		return(NULL);
	}
	for(i = j = 0, rp = &ns->rr; i < ns->ns_off; rp++, i++) {
		if (rp->class == C_HS && rp->type == T_TXT) {
			/*
			 * skip CNAME records
			 */
			retvec[j] = calloc(rp->dlen + 1, sizeof(char));
			dst = retvec[j];
			ocp = cp = rp->data;
			while (cp < ocp + rp->dlen) {
			    n = (unsigned char) *cp++;
			    (void) bcopy(cp, dst, n);
			    cp += n;
			    dst += n;
			}
			*dst = 0;
			j++;
		}
	}
	retvec[j] = 0;
	return(retvec);
}


#ifdef AUTHEN
char **
hes_auth_resolve(HesiodName, HesiodNameType)
char *HesiodName, *HesiodNameType;
{
	register char *cp;
	static char *retvec[100];
	char *ocp, *dst;
	char *calloc();
	int i, j, n;
	struct nsmsg *ns, *_resolve(), *auth_resolve();
	rr_t *rp;
	extern int errno;

	cp = hes_to_bind(HesiodName, HesiodNameType);
	if (cp == NULL) return(NULL);
	errno = 0;
	ns = auth_resolve(cp, C_HS, T_TXT, NoRetryTime);
 	if (errno == ETIMEDOUT || errno == ECONNREFUSED) {
		Hes_Errno = HES_ER_NET;
		return(NULL);
/* change this error code somehow */
	} else if (errno == EPERM) {
		Hes_Errno = HES_ER_AUTH;
		return(NULL);
	}
	if (ns == NULL || ns->ns_off <= 0) {
		Hes_Errno = HES_ER_NOTFOUND;
		return(NULL);
	}
	for(i = j = 0, rp = &ns->rr; i < ns->ns_off; rp++, i++) {
		if (rp->class == C_HS && rp->type == T_TXT) {
			/*
			 * skip CNAME records
			 */
			retvec[j] = calloc(rp->dlen + 1, sizeof(char));
			dst = retvec[j];
			ocp = cp = rp->data;
			while (cp < ocp + rp->dlen) {
			    n = (unsigned char) *cp++;
			    (void) bcopy(cp, dst, n);
			    cp += n;
			    dst += n;
			}
			*dst = 0;
			j++;
		}
	}
	retvec[j] = 0;
	return(retvec);
}

#else AUTHEN

char **
hes_auth_resolve(HesiodName, HesiodNameType)
char *HesiodName, *HesiodNameType;
{
  return(hes_resolve(HesiodName, HesiodNameType));
}

#endif AUTHEN

int
hes_error()
{
	return(Hes_Errno);
}
