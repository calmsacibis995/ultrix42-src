#ifndef lint
static char *sccsid = "@(#)hesupd.c	4.2      ULTRIX  2/14/91";
#endif lint

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

/************************************************************************
 *									*
 *	hesupd - hesiod update daemon					*
 *									*
 *	Description: The hesiod receives hesiod database update requests*
 *	from a TCP socket connection. The updates are parsed and if they*
 *	are valid they are applied to the database. A response is sent  *
 *	back to the requestor indicating success or failure. 		*
 *									*
 *	Since the old password is sent across the network this presents *
 *	a potential for a breakin (unless you use DESNCs).To reduce this*
 *	risk the entire mesage is jumbled by passwd and unjumbled here. *
 *									*
 *									*
 ************************************************************************/

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <sys/fs.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <sys/lock.h>
#include <sys/svcinfo.h>
#include <netinet/in.h>
#include <errno.h>
#include <syslog.h>
#include <signal.h>
#include <stdio.h>
#include <netdb.h>
#include <krb.h>
#include <auth.h>
#define		PASSCHG		1
#define		HESUPD_PORT	800
#define		HESUPDACK	6
#define		HESUPDNAK	9

struct hesupdmsg {
		char	newcrypt[64];
		int 	opcode;
		int	hesuid;
		char	oldpwd[32];
		};
extern struct passwd *getpwuid(), *getpwuid_bind();
char *index();
extern FILE *heslog;
int newsock, authtype, auth_bind;
char defhome[] = "/var/dss/namedb/src";
char bindmaster[] = "bindmaster";
FILE *heslog = NULL;
char *homebase = defhome;
char namebuf[ANAME_SZ];
char *ptr;

main (argc,argv) 
        int argc;
        char *argv[];
{
	static struct sockaddr sock, sock2;
	static struct hesupdmsg hupmsg;
	char * mytime();
	int s, socklen, amount, i, rfds, nfound, t, j;
	int on=1;
	char hesupdbuf[sizeof(struct hesupdmsg)];
        char *arg, *seclass;
	short hesupd_port;
	int pid;
	struct sigvec vec;
	int res, fd, cmd;
	extern int errno;
	struct svcinfo *svcp;
	struct servent *sp;
	struct hostent *hp;
	struct hostent mhp; /* bind master hostent */
	struct hostent lhp; /* local host hostent  */
	struct sockaddr_in sin;
	char host[32];
	int hlen = 32;
	struct timeval timeout;
	int ret;
	int looplim;
	int ackmsg=HESUPDACK;
	int nakmsg=HESUPDNAK;
        /*
         * must be superuser to run
         */
        if (geteuid() != 0){
                (void) fprintf(stderr, "hesupd:  must be super user\n");
                (void) fflush(stderr);
                exit(1);
        }
	/* hesupd can only be run on a master bind server */
	gethostname(host, hlen);
	hp = gethostbyname(host);
	if(hp == 0)
		{
		fprintf(stderr,"hesupd cant lookup %s\n",host);
		exit(1);
		}
	hp = gethostbyname(bindmaster);
	if(hp == 0)
		{
		fprintf(stderr,"hesupd cant lookup %s\n",bindmaster);
		exit(1);
		}
       	if(strcmp(host,hp->h_name))
 		{
 		fprintf(stderr,"hesupd can only be run on a bind primary\n");
 		exit(0);
 		}
	sp=getservbyname("hesupd","tcp");
	if(sp == 0)
		{
		fprintf(stderr,"hesupd is a unknown tcp service\n");
		exit(1);
		}
        /* initialize my log file */
	heslog= fopen("/usr/adm/hesupd.log","a+");
	if(heslog == NULL)
		{
		fprintf(stderr,"Hesupd: Cant open log file\n");
		exit(0);
		}
	if((svcp = getsvc()) == NULL)
		{
		fprintf(stderr,"Hesupd: Cannot access security type\n");
		exit(0);
		}
	authtype=svcp->svcauth.seclevel;
	for (i = 0, auth_bind = 0; svcp->svcpath[SVC_AUTH][i] != SVC_LAST; i++)
		if (svcp->svcpath[SVC_AUTH][i] == SVC_BIND) {
			auth_bind = 1;
			break;
		}
/*	NO ARGUMENTS IMPLEMENTED
*	Process arguments 
*	-d hesupd home directory
*
*      while (argc > 1 && argv[1][0] == '-') {
*               argc--;
*               arg = *++argv;
*               switch (arg[1]) {
*               case 'd':              
*                       if (arg[2])
*                               homebase = &arg[2];
*                       else if (argc > 1) {
*                               argc--;
*                               homebase = *++argv;
*                       }
*                       break;
*	}
*	}
*/
        if (chdir(homebase) < 0) {
                fprintf(stderr, "hesupd: can't chdir to %s\n",homebase);
                exit(1);
        }
	/**** do fork of daemon */

	close(heslog);
        pid = fork();
        if (pid < 0) {
                     perror("hesupd fork problem");
                     exit(1);
        }
        if (pid != 0)
                     exit(0);
        for (t = 0; t < 20; t++)
                     (void) close(t);
        open("/", 0);
        dup2(0, 1);
        dup2(0, 2); 
	sigsetmask(0);
	sp=getservbyname("hesupd","tcp");
	if(sp == 0)
		{
		fprintf(heslog,"hesupd is a unknown tcp service\n");
		exit(1);
		}
	hp = gethostbyname(bindmaster);
	if(hp == 0)
		{
		fprintf(stderr,"hesupd cant lookup %s\n",bindmaster);
		exit(1);
		}
        /* initialize my log file */
	heslog= fopen("/usr/adm/hesupd.log","a+");
	if(heslog == NULL)
		{
		fprintf(heslog,"Hesupd: Cant open log file\n");
		exit(0);
		}
	fprintf(heslog,"bindmaster=%s\n",hp->h_name);
	fflush(heslog);
	fprintf(heslog,"Allocating socket\n");
	fflush(heslog);
	if ((s=socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		fprintf(heslog, "server: could not create a socket\n");
		fflush(heslog);
		exit(1);
	}
	bzero((char *)&sin, sizeof(sin));
        bcopy(hp->h_addr,(char *)&sin.sin_addr,hp->h_length);
        sin.sin_family = AF_INET;
        sin.sin_port = sp->s_port;
	fprintf(heslog,"port=%x\n",sin.sin_port);
	fprintf(heslog,"addr=%s\n",inet_ntoa(sin.sin_addr));
	
	fprintf(heslog,"Setting socket options\n");
        /*setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));*/
	ret = bind(s, (char *) &sin, sizeof(struct sockaddr_in));
	if (ret < 0) {
		(void)close(s);
		fprintf(heslog,"hesupd bind error = %d\n",errno);
		fprintf(heslog, "server: could not bind a name to a socket\n");
		fflush(heslog);
		exit(1);
	}
	fprintf(heslog,"InitiaL listen on socket\n");
	if (listen(s,25) < 0) {
		fprintf(heslog, "server: could not listen to a socket!\n");
		fflush(heslog);
		exit(1);
	}
	if(authtype >= SEC_UPGRADE && auth_bind) {
                if(gethostname(namebuf, sizeof(namebuf)) == -1) {
                        fprintf(heslog,"gethostname failure\n");
                }

		if((ptr = index(namebuf, '.')) != (char *)0)
			*ptr = '\0';

                if(krb_svc_init("hesiod", namebuf, (char *)NULL, 0,
                        (char *)NULL, "/var/dss/kerberos/tkt/tkt.hesupd")
                                != RET_OK) {
                        fprintf(heslog,"Kerberos initialization failure\n", 2);
                }
        }

	socklen = sizeof(sock2);
/****************************** accept connections forever ********************/
	while(1)
		{
		fprintf(heslog,"hesupd waiting for a connection\n");
		fflush(heslog);
		newsock = accept(s, &sock2, &socklen);
		if(newsock < 0 ) {
			fprintf(heslog, "hesupd: failed connection accept!\n");
			fflush(heslog);
			exit(1);
			}
		fprintf(heslog, "hesupd: Accepted a connection on the socket %d\n", newsock);
		fflush(heslog);

/******************* receive hesiod update request ****************************/
		bzero((char *)&hupmsg, sizeof(struct hesupdmsg));
		amount = recv (newsock, (char *)&hupmsg, sizeof(struct hesupdmsg), 0);
		if (amount != sizeof(struct hesupdmsg)) {
			fprintf(heslog, "hesupd: recv error! received %d bytes\n",amount);
			fflush(heslog);
		}
		else 	{
			/* reread the svc.conf file if it has changed */
			if((svcp = getsvc()) == NULL) {
				fprintf(stderr,
					"Hesupd: Cannot access security type\n");
				exit(0);
				}

			authtype = svcp->svcauth.seclevel;

			/* check to see if bind must be kerberos authenticated */
			for (i = 0, auth_bind = 0;
					svcp->svcpath[SVC_AUTH][i] != SVC_LAST;
					i++)
				if (svcp->svcpath[SVC_AUTH][i] == SVC_BIND) {
					auth_bind = 1;
					break;
					}

			/* if bind is kerberos authenticated, re-init kerberos */
			if(authtype >= SEC_UPGRADE && auth_bind) {
		                if(gethostname(namebuf, sizeof(namebuf)) == -1) {
		                        fprintf(heslog,"gethostname failure\n");
			                }

				if((ptr = index(namebuf, '.')) != (char *)0)
					*ptr = '\0';

				/* krb_svc_init will not reinit our cred if our
					tgt has not expired */
		                if(krb_svc_init("hesiod", namebuf, (char *)NULL,
					0, (char *)NULL,
					"/var/dss/kerberos/tkt/tkt.hesupd")
		                                != RET_OK) {
		                        fprintf(heslog,
						"Kerberos initialization failure\n", 2);
		        	        }
			        }

			if(procmsg(&hupmsg))
				amount=send(newsock,(char *)&ackmsg,4,0);
			else	amount=send(newsock,(char *)&nakmsg,4,0);
			if(amount < 0)
				{
                        	fprintf(heslog, "hesupd: send error!\n");
                        	fflush(heslog);
				}
			}
		close(newsock);
		}
}
int
procmsg(hup)
struct hesupdmsg *hup;
{
        struct passwd *hesiod_pwuid;
	int retval=0;
	unmix(hup);	
/************* 	check if this uid is being served by hesiod ******************/
	if(hup->opcode != PASSCHG)
		return(0);
        hesiod_pwuid = getpwuid_bind( (int) hup->hesuid);
	if(hesiod_pwuid == NULL)
		return(0);
	fflush(heslog);
	switch(hup->opcode)
		{
		case	PASSCHG:
			switch(authtype)
				{
				case	SEC_BSD:
					fprintf(heslog,"SEC_BSD\n");
					retval=chpw_bsd(hup->hesuid,hup->oldpwd,hup->newcrypt);
					break;
				case	SEC_UPGRADE:
					fprintf(heslog,"SEC_UPGRADE\n");
					retval=chpw_trans(hup->hesuid,hup->oldpwd,hup->newcrypt);
					break;
				case	SEC_ENHANCED:
					fprintf(heslog,"SEC_ENHANCED\n");
					retval=chpw_c2(hup->hesuid,hup->oldpwd,hup->newcrypt);
					break;
				}
			if(retval)
				fprintf(heslog,"good password update for uid %d\n",hup->hesuid);
			else	fprintf(heslog,"bad password update for uid %d\n",hup->hesuid);
			fflush(heslog);
			return(retval);
		
		default:
                	fprintf(heslog, "hesupd: bad opcode error!\n");
                	fflush(heslog);
			return(0);	
		}
}

/**** unshake and unstir ****/
unmix(hup)
struct hesupdmsg *hup;
{
        unsigned char *hesbuf= (unsigned char *) hup;
        unsigned char tmp=0;
        int i,j,len;
        len = sizeof(struct hesupdmsg) - 1;
        for (i=0,j=3; i <=len; i++,j++)
                hesbuf[i]-= j % 5;
        tmp= (( hesbuf[len] << 3 ) | (hesbuf[0] >> 5));
        for (i=0;i < len; i++)
                {
                hesbuf[i]= (( hesbuf[i] << 3 ) | (hesbuf[i+1] >> 5));
                }
        hesbuf[len]=tmp;
        for (i=0,j=11; i <=len; i++,j++)
                hesbuf[i]-= j % 15;
}

AUTHORIZATION *
getauthuid_hesiod(uid)
int uid;
{
	static AUTHORIZATION auth;
        char uidbuf[10], **pp;
	AUTHORIZATION *_auth = (AUTHORIZATION *) NULL;
        setent_bind(0);
        sprintf(uidbuf, "%u", uid);
        pp = (char **) hes_auth_resolve(uidbuf, "auth");
        endent_bind();
        if(pp != NULL)
                if(*pp) {
                        binauth(*pp, &auth);
                        while(*pp)
                                free(*pp++);
                        _auth = &auth;
                }
        else
                return(NULL);
        return _auth;
}
