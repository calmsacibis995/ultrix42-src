#ifndef lint
static	char	*sccsid = "@(#)res_auth.c	4.2	(ULTRIX)	9/4/90";
#endif lint
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
 * Modification History:
 *
 * 14-Jun-89	bbrown
 *	Added AUTHEN resolver code.
 */

#if defined(lint) && !defined(DEBUG)
#define DEBUG
#define AUTHEN
#endif

#ifdef AUTHEN

#include <sys/types.h>
#include <netinet/in.h>
#include <time.h>
#include <krb.h>
#include <arpa/nameser.h>
#include <resolv.h>


#ifndef NULL
#define NULL 0
#endif 

#define INT_EXT 0
#define INT_INT 1

map_t m_credtype[] = {
	"kerberos",	AUTH_KRB,
};

map_t m_credval[] = {
	"one",	ONE,
};

int
m_str_val(mapper, len, str)
	map_t *mapper;
	int len;
	char *str;
{
        int looper;

	for(looper = 0; looper < len; looper++) {
		if (!strcmp(str, mapper[looper].token))
			return(mapper[looper].val);
	}
	return(-1);
}

char *
m_val_str(mapper, len, val)
	map_t *mapper;
	int len;
	int val;
{
        int looper;

	for(looper = 0; looper < len; looper++) {
		if (val == mapper[looper].val)
			return(mapper[looper].token);
	}
	return((char *)NULL);
}
		
int mk_tcred_dname(string, type, class, dname, int_or_ext)
	char *string;
	int type;
	int class;
	char *dname;
{
	char *cp;
	int int_len;
	char *len;

	cp = string;

	if(int_or_ext == INT_EXT)
		cp++;

	switch(class) {
	      case C_IN: {
		      strcpy(cp, "IN-");
		      break;
	      }
	      case C_CHAOS: {
		      strcpy(cp, "CHAOS-");
		      break;
	      }
	      case C_HS: {
		      strcpy(cp, "HS-");
		      break;
	      }
	      case C_ANY: {
		      strcpy(cp, "ANY-");
		      break;
	      }
	      default: {
		      return(RET_BAD);
	      }
	}

	switch(type) {
	      case T_A: {
		      strcat(cp, "A");
		      break;
	      }
	      case T_NS: {
		      strcat(cp, "NS");
		      break;
	      }
	      case T_MD: {
		      strcat(cp, "MD");
		      break;
	      }
	      case T_MF: {
		      strcat(cp, "MF");
		      break;
	      }
	      case T_CNAME: {
		      strcat(cp, "CNAME");
		      break;
	      }
	      case T_SOA: {
		      strcat(cp, "SOA");
		      break;
	      }
	      case T_MB: {
		      strcat(cp, "MB");
		      break;
	      }
	      case T_MG: {
		      strcat(cp, "MG");
		      break;
	      }
	      case T_MR: {
		      strcat(cp, "MR");
		      break;
	      }
	      case T_NULL: {
		      strcat(cp, "NULL");
		      break;
	      }
	      case T_WKS: {
		      strcat(cp, "WKS");
		      break;
	      }
	      case T_PTR: {
		      strcat(cp, "PTR");
		      break;
	      }
	      case T_HINFO: {
		      strcat(cp, "HINFO");
		      break;
	      }
	      case T_MINFO: {
		      strcat(cp, "MINFO");
		      break;
	      }
	      case T_MX: {
		      strcat(cp, "MX");
		      break;
	      }
	      case T_TXT: {
		      strcat(cp, "TXT");
		      break;
	      }
	      case T_UINFO: {
		      strcat(cp, "UINFO");
		      break;
	      }
	      case T_UID: {
		      strcat(cp, "UID");
		      break;
	      }
	      case T_GID: {
		      strcat(cp, "GID");
		      break;
	      }
	      case T_UNSPEC: {
		      strcat(cp, "UNSPEC");
		      break;
	      }
	      case T_UNSPECA: {
		      strcat(cp, "UNSPECA");
		      break;
	      }
	      case T_AXFR: {
		      strcat(cp, "AXFR");
		      break;
	      }
	      case T_MAILB: {
		      strcat(cp, "MAILB");
		      break;
	      }
	      case T_MAILA: {
		      strcat(cp, "MAILA");
		      break;
	      }
	      case T_ANY: {
		      strcat(cp, "ANY");
		      break;
	      }
	      case T_CRED: {
		      strcat(cp, "CRED");
		      break;
	      }
	      default: {
		      return(RET_BAD);
	      }
	}

	if (int_or_ext == INT_EXT)
		*string = strlen(cp);

	cp += strlen(cp);

	if(int_or_ext == INT_EXT) {
		len = cp++;
		
		while(*dname != '\0') {
			for(int_len = 0; *dname != '.' && *dname != '\0';
			    int_len++, *cp++ = *dname++) {
				if (*dname == '\\') {
					dname++;
					int_len ++;
					*cp++ = *dname++;
				}
			}
			*len = int_len;
			len = cp++;
			if(*dname != '\0')
				dname++;
		}
		*len = 0;
	} else if(strlen(dname) > 0) {
		*cp++ = '.';
		bcopy(dname, cp, strlen(dname) + 1);
	}

	if(int_or_ext == INT_EXT)
		return(strlen(string) + 1);
	else
		return(strlen(string));
}

res_mkl_krbcred(version, principle, instance, cred, buf, rr, rrlen, dname,
		type, class)
	int version;	/* type of credentials to make */
	char *principle;/* name of the principle to talk with */
	char *instance;	/* name of the instance to talk with */
	CREDENTIALS *cred; /* cred struct for principle principle (return) */
	char *buf;	/* a pointer to the beginning of the message */
	char *rr;	/* place to put the new resource record */
	long rrlen;	/* length of the resource record area */
	char *dname;	/* domain of the data */
	int type;	/* type of data the auth rr authenticates */
	int class;	/* class of data the auth rr authenticates */
{
	register char *cp;
	static char realm[REALM_SZ];
	int	err;
	C_Block bcksum[2];
	long cksum;
	char string[MAXDNAME];
	int num;
	struct timeval time_v;
	KTEXT_ST auth;

	cp = rr;
	if((num = mk_tcred_dname(string, type, class, dname, INT_EXT))
	   == RET_BAD)
		return(RET_BAD);

	if((rrlen -= num) < 0)
		return(KRBCRED_TOOSMALL);

	bcopy(string, cp, num);
	cp += num;

	if((rrlen -= sizeof(u_short)) < 0)
		return(KRBCRED_TOOSMALL);
	putshort(T_CRED, cp);
	cp += sizeof(u_short);

	if((rrlen -= sizeof(u_short)) < 0)
		return(KRBCRED_TOOSMALL);
	putshort(class, cp);
	cp += sizeof(u_short);

	if((rrlen -= sizeof(u_long)) < 0)
		return(KRBCRED_TOOSMALL);
	putlong(0L, cp);
	cp += sizeof(u_long);
 
#define HESTKT_LIFE 96
	err = krb_get_lrealm(realm, 0);

	if (err != KSUCCESS) {
#ifdef DEBUG
		if (_res.options & RES_DEBUG)
			printf("krb_get_lrealm returned %d\n", err);
#endif
		return(KRBCRED_REALM);
	}

	err = krb_get_cred(principle, instance, realm, cred);

	if (err == RET_NOTKT)
		if (err = !get_ad_tkt(principle, instance, realm, HESTKT_LIFE))
			err = krb_get_cred(principle, instance, realm, cred);
	if (err) {
#ifdef DEBUG
		if (_res.options & RES_DEBUG)
			printf("kerberos error %d getting ticket\n", err);
#endif
		return(KRBCRED_CRED);
	}

	if(gettimeofday(&time_v, (struct timezone *)NULL)) {
#ifdef DEBUG
		if (_res.options & RES_DEBUG)
			printf("gettimeofday error %d\n", err);
#endif
		return(KRBCRED_TIME);
	}
		
	if(((cred->lifetime * 5) * 60) + cred->issue_date < time_v.tv_sec + 5){
		if (err = !get_ad_tkt_options(GETTKTOPT_GETFRESH, principle,
					      instance, realm,
					      HESTKT_LIFE))
			err = krb_get_cred(principle, instance, realm, cred);
	}
	if (err) {
#ifdef DEBUG
		if (_res.options & RES_DEBUG)
			printf("kerberos error %d getting ticket\n", err);
#endif
		return(KRBCRED_FRESH);
	}
 
	bzero(bcksum, sizeof(bcksum));
	cksum = quad_cksum((char *)buf, bcksum, cp - buf, 2, cred->session);

	err = krb_mk_req(&auth, principle, instance, realm, cksum);
	if (err != KSUCCESS) {
#ifdef DEBUG
		if (_res.options & RES_DEBUG)
			printf("krb_mk_req returned %d\n", err);
#endif
		return(KRBCRED_MKREQ);
	}

	if((rrlen -= sizeof(u_short)) < 0)
		return(KRBCRED_TOOSMALL);
	putshort(auth.length + (sizeof(u_short) * 2), cp);
	cp += sizeof(u_short);

	if((rrlen -= sizeof(u_short)) < 0)
		return(KRBCRED_TOOSMALL);
	putshort(AUTH_KRB, cp);
	cp += sizeof(u_short);

	if((rrlen -= sizeof(u_short)) < 0)
		return(KRBCRED_TOOSMALL);
	putshort(version, cp);
	cp += sizeof(u_short);

	if((rrlen -= auth.length) < 0)
		return(KRBCRED_TOOSMALL);
	bcopy(auth.dat, cp, auth.length);
	cp += auth.length;

	return(cp - (char *)rr);
}
	

res_mks_krbcred(version, principle, instance, ad, buf, rr, rrlen, dname,
		type, class)
	int version;	/* type of credentials to make */
	char *principle;/* principle to make the auth rr for */
	char *instance;	/* instance to make the auth rr for */
	AUTH_DAT *ad;	/* auth data to place in the rr */
	char *buf;	/* pointer to the buffer which will contain the rr */
	char *rr;	/* pointer to a rr pointer */
	int rrlen;	/* size of the rr buffer */
	char *dname;	/* name of the domain of the data */
	int type;	/* type of the data */
	int class;	/* class of the data */
{
	char string[MAXDNAME];
	long cksum;
	C_Block bcksum[2];
	CREDENTIALS cred;
	static char realm[REALM_SZ];
	char *cp;
	int num;
	struct timeval time_v;
	int err;

	cp = rr;

	if((num = mk_tcred_dname(string, type, class, dname, INT_EXT))
	   == RET_BAD)
		return(RET_BAD);

	if((rrlen -= num) < 0)
		return(KRBCRED_TOOSMALL);

	bcopy(string, cp, num);
	cp += num;

	if((rrlen -= sizeof(u_short)) < 0)
		return(KRBCRED_TOOSMALL);

	putshort(T_CRED, cp);
	cp += sizeof(u_short);

	if((rrlen -= sizeof(u_short)) < 0)
		return(KRBCRED_TOOSMALL);

	putshort(class, cp);
	cp += sizeof(u_short);

	if((rrlen -= sizeof(u_long)) < 0)
		return(KRBCRED_TOOSMALL);

	putlong((u_long)0, cp);
	cp += sizeof(u_long);


	putshort((u_short)(sizeof(u_short) * 2) + (sizeof(u_long) * 2), cp);
	cp += sizeof(u_short);

	if((rrlen -= sizeof(u_short)) < 0)
		return(KRBCRED_TOOSMALL);

	putshort(AUTH_KRB, cp);
	cp += sizeof(u_short);

	if((rrlen -= sizeof(u_short)) < 0)
		return(KRBCRED_TOOSMALL);

	putshort(version, cp);
	cp += sizeof(u_short);

	gettimeofday(&time_v, NULL);

	if((rrlen -= sizeof(u_long)) < 0)
		return(KRBCRED_TOOSMALL);

	putlong(time_v.tv_sec, cp);
	cp += sizeof(u_long);

	if(ad == NULL) {

		if(krb_get_lrealm(realm, 0) != KSUCCESS) {
#ifdef DEBUG
			if (_res.options & RES_DEBUG)
				printf("krb_get_lrealm returned %d\n", err);
#endif
			return(KRBCRED_REALM);
		}

		if((err = krb_get_cred(principle, instance, realm, &cred))
		   != KSUCCESS) {
#ifdef DEBUG
			if (_res.options & RES_DEBUG)
				printf("get_cred error %d\n", err);
#endif
			return(KRBCRED_CRED);
		}
	
		cksum = quad_cksum((char *)buf, bcksum, cp - buf, 2,
				cred.session);
	} else {
		cksum = quad_cksum((char *)buf, bcksum, cp - buf, 2,
				ad->session);
	}
		

	if((rrlen -= sizeof(u_long)) < 0)
		return(KRBCRED_TOOSMALL);

	putlong(cksum, cp);
	cp += sizeof(u_long);

	return(cp - rr);
}

res_rds_krbcred(version, principle, instance, cred, buf, rr, rrlen, dname,
		type, class)
	int version;	/* type of credentials to read */
	char *principle;/* name of the principle to talk with */
	char *instance;	/* name of the instance to talk with */
	CREDENTIALS *cred; /* cred struct for principle principle */
	char *buf;	/* pointer to the buffer which contains the rr */
	char *rr;	/* pointer to the auth rr to read */
	int rrlen;	/* length of the rr */
	char *dname;	/* domain of the data that must be authen */
	int type;	/* type of data the rr should authenticate */
	int class;	/* class of data the rr should authenticate */
{
	int err;
/*	char *data;*/
	int length;
	int auth_type;
	int auth_version;
	long timemsg;
	struct timeval time;
	static char realm[REALM_SZ];
	CREDENTIALS cred_buf;
/*	char *cksummsg;
	int cksum;*/
	long cksummsg;
	long cksum;
	C_Block bcksum[2];
	char string[MAXDNAME];
	char dnbuf[MAXDNAME];
	int n;
	char *cp;
	int num;
	int rr_type;
	int rr_class;
	

	if(version != ONE)
		return(KRBCRED_NOTSUPP);

	cp = rr;

	/* get kerberos ticket */
	if((n = dn_expand(buf, rr + rrlen, cp, dnbuf, sizeof(dnbuf))) < 0)
		return(KRBCRED_EXPAND);
	
	cp += n;

	if((num = mk_tcred_dname(string, type, class, dname, INT_INT))
	   == RET_BAD)
		return(KRBCRED_NAME);

	if(bcmp(string, dnbuf, strlen(string)))
		return(KRBCRED_NAMECMP);

	rr_type = _getshort(cp);
	cp += sizeof(u_short);

	rr_class = _getshort(cp);
	cp += sizeof(u_short);

	if(rr_class != class)
		return(KRBCRED_CLASS);

	if(rr_type != T_CRED)
		return(KRBCRED_TYPE);

	cp +=  sizeof(u_long);

	length = _getshort(cp);
	cp += sizeof(u_short);

/*	data = cp;*/
	auth_type = _getshort(cp);
	cp += sizeof(u_short);

	auth_version = _getshort(cp);
	cp += sizeof(u_short);

	if(auth_type != AUTH_KRB)
		return(KRBCRED_AUTHTYPE);

	if( auth_version != version)
		return(KRBCRED_AUTHVER);
	
	timemsg = _getlong(cp);
	cp += sizeof(u_long);

/*	cksummsg = cp;*/

	cksummsg = _getlong(cp);

	gettimeofday(&time, NULL);

	bzero(bcksum, sizeof(bcksum));
	
	if ((err = krb_get_lrealm(realm, 0)) != KSUCCESS) {
#ifdef DEBUG
		if (_res.options & RES_DEBUG)
			printf("krb_get_lrealm returned %d\n", err);
#endif
		return(KRBCRED_REALM);
	}

	if(cred == NULL) {
		if((err = krb_get_cred(principle, instance, realm, &cred_buf))
		   != KSUCCESS) {
#ifdef DEBUG
			if (_res.options & RES_DEBUG)
				printf("get_cred error %d\n", err);
#endif
			return(KRBCRED_CRED);
		}
		cred = &cred_buf; 

	}
	cksum = quad_cksum((char *)buf, bcksum, cp - (char *)buf, 2,
			   cred->session);

/*	if (!bcmp(&cksum, cksummsg, length - (cksummsg - data) ) &&
	    abs(timemsg - time.tv_sec) < 5 * 60)
		return(RET_OK);
*/
	if (cksum == cksummsg && abs(timemsg - time.tv_sec) < 5 * 60)
		return((cp - rr) + sizeof(u_long) );

	return(KRBCRED_TIME);

}

res_rdl_krbcred(version, ad, buf, rr, rrlen, dname, type, class, from)
	int version;	/* type of credentials to read */
	AUTH_DAT *ad;	/* auth data contained in the rr */
	char *buf;	/* pointer to the buffer which contains the rr */
	char *rr;	/* poiner to a rr */
	int rrlen;	/* length of the rr */
	char *dname;	/* domain of the data that must be authen */
	int type;	/* type of data the rr should authenticate */
	int class;	/* class of data the rr should authenticate */
        struct sockaddr_in *from;  /* address the msg came from */
{
	char *cp;
	int auth_type;
	int auth_version;
	KTEXT_ST cred;
	char string[MAXDNAME];
	char dnbuf[MAXDNAME];
	int length;
	int num;
	int rr_type;
	int rr_class;
	int n;
	int err;
	long cksum;
	C_Block bcksum[2];

	if(version != ONE)
		return(KRBCRED_NOTSUPP);

	cp = rr;

	/* get kerberos ticket */
	if((n = dn_expand(buf, rr + rrlen, cp, dnbuf, sizeof(dnbuf))) < 0)
		return(KRBCRED_EXPAND);
	
	cp += n;

	if((num = mk_tcred_dname(string, type, class, dname, INT_INT))
	   == RET_BAD)
		return(KRBCRED_NAME);

	if(bcmp(string, dnbuf, strlen(string)))
		return(KRBCRED_NAMECMP);

	rr_type = _getshort(cp);
	cp += sizeof(u_short);

	rr_class = _getshort(cp);
	cp += sizeof(u_short);

	if(rr_class != class)
		return(KRBCRED_CLASS);

	if(rr_type != T_CRED)
		return(KRBCRED_TYPE);

	/* skip over ttl */
	cp += sizeof(u_long); 

	cred.length = _getshort(cp);
	cp += sizeof(u_short);

	length = (cp - rr ) + cred.length;

	auth_type = _getshort(cp);
	cp += sizeof(u_short);

	auth_version = _getshort(cp);
	cp += sizeof(u_short);

	if(auth_type != AUTH_KRB)
		return(KRBCRED_AUTHTYPE);

	if(auth_version != version)
		return(KRBCRED_AUTHVER);


	bcopy(cp, cred.dat, cred.length - (sizeof(u_short) * 2));

	err = krb_rd_req(&cred, NULL, NULL, from->sin_addr, ad, NULL);

	if (err != KSUCCESS) {
#ifdef DEBUG

		if (err == RD_AP_BADD && debug) {
			fprintf(ddt, "from->sin_addr = %s, ",
				inet_ntoa(from->sin_addr));
			fprintf(ddt, "ad.address = %s\n",
				inet_ntoa(ad->address));
		}

		if (debug)
			fprintf(ddt,"kerberos error %d\n", err);
#endif
		return(KRBCRED_CRED);

	} else {

		bzero(bcksum, sizeof(bcksum));
		cksum = quad_cksum((char *)buf, bcksum,
				   cp - buf - (3 * sizeof(u_short)),
				   2, ad->session);

		if (cksum != ad->checksum) {
#ifdef DEBUG
			if (debug)
				fprintf(ddt,"crypto checsum failed\n");
#endif				    
		return(KRBCRED_CKSUM);

		} else {

#ifdef DEBUG
			if (debug)
				fprintf(ddt,
					"kerberos authenticated %s%s%s@%s\n",
 					ad->pname, ad->pinst[0] == 0 ?"":".",
					ad->pinst, ad->prealm);
#endif
			return(length);
		}

	}
}

char *res_dotname_head(str)
     char *str;
{
	char *dest;
	char *cpdest;
	char *cpsrc;

	if((dest = (char *)malloc(MAXDNAME)) == NULL)
		return(NULL);

	for(cpsrc = str, cpdest = dest; *cpsrc != '.' && *cpsrc != '\0';
	    cpsrc++, cpdest++) {

                *cpdest = *cpsrc;
	}

	*cpdest = '\0';
	return(dest);
}

res_dname_rmhead(dname)
	u_char **dname;
{
	u_short length;

	length = (u_short)**dname;
	*dname += length + 1;
}

res_dotname_rmhead(str)
	char **str;
{
        char *cp;

	for(cp = *str; *cp != '.' && *cp != '\0'; cp++);

	if (*cp == '\0')
		*str = cp;
	else
		*str = cp + 1;
}
#endif AUTHEN


