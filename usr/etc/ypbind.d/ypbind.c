#ifndef lint
static  char    *sccsid = "@(#)ypbind.c	4.5  (ULTRIX)        3/1/91";
#endif lint

/****************************************************************
 *								*
 *  Licensed to Digital Equipment Corporation, Maynard, MA	*
 *		Copyright 1985 Sun Microsystems, Inc.		*
 *			All rights reserved.			*
 *								*
 ****************************************************************/

/*
 * This constructs a list of servers by domains,
 * and keeps more-or-less up to dat track of
 * those server's reachability.
 */

/*	02/06/89	jhw 	added ypbind -S  security option */
/*	04/14/89	jhw	added SUN 4.0 code		*/
/* 	08/30/89	jhw	backed out SUNs reserve port check */
/*	12/14/89	jhw	added nulling of client structs */
/*      08/08/90        terry   added ypbind -X initial bind option */ 
/*      01/24/91        terry   bug fixes to ypbind_send_setdom() and */
/*                              ypbind_set_binding() to match Sun's   */
/*                              code and fix the problem of ypbind not*/
/*                              working when no options are used.    */
/*      02/27/91        terry   support new file descriptor handling */ 

#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <rpc/rpc.h>
#include <sys/dir.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypv1_prot.h>
#include <rpcsvc/ypclnt.h>

/*
 * The domain struct is the data structure used by the yp binder to remember
 * mappings of domain to a server.  The list of domains is pointed to by
 * known_domains.  Domains are added when the yp binder gets binding requests
 * for domains which are not currently on the list.  Once on the list, an
 * entry stays on the list forever.  Bindings are initially made by means of
 * a broadcast method, using functions ypbind_broadcast_bind and
 * ypbind_broadcast_ack.  This means of binding is re-done any time the domain
 * becomes unbound, which happens when a server doesn't respond to a ping.
 * current_domain is used to communicate among the various functions in this
 * module; it is set by ypbind_get_binding.
 *  
 */
struct domain {
	struct domain *dom_pnext;
	char dom_name[MAXNAMLEN + 1];
	unsigned short dom_vers;	/* YPVERS or YPOLDVERS */
	bool dom_boundp;
	CLIENT *ping_clnt;
	struct in_addr dom_serv_addr;
	unsigned short int dom_serv_port;
	int dom_report_success;		/* Controls msg to /dev/console*/
	int dom_broadcaster_pid;
};
static int ping_sock = RPC_ANYSOCK;
struct domain *known_domains = (struct domain *) NULL;
struct domain *current_domain;		/* Used by ypbind_broadcast_ack, set
					 *   by all callers of clnt_broadcast */
struct domain *broadcast_domain;	/* Set by ypbind_get_binding, used
					 *   by the mainline. */
SVCXPRT *tcphandle;
SVCXPRT *udphandle;

#define BINDING_TRIES 3			/* Number of times we'll broadcast to
					 *   try to bind default domain.  */
#define PINGTOTTIM 20			/* Total seconds for ping timeout */
#define PINGINTERTRY 10
#define SETDOMINTERTRY 20
#define SETDOMTOTTIM 60
/*** udp timeout stuff *****/

#define UDPINTER_TRY 5                  /* Secs between tries for udp*/
#define UDPTIMEOUT UDPINTER_TRY*3       /* Total timeout for udp */
#define CALLINTER_TRY 10                /* Secs between callback tries*/
#define CALLTIMEOUT CALLINTER_TRY*2     /* Total timeout for callback */
struct timeval udp_intertry = {
        UDPINTER_TRY,
        0
};
struct timeval udp_timeout = {
        UDPTIMEOUT,
        0
};
struct timeval tcp_timeout = {
        180,    /* timeout for map enumeration (could be long) */
        0
};
#ifdef VERBOSE
int silent = FALSE;
#else
int silent = TRUE;
#endif

extern int errno;

void dispatch();
void ypbind_dispatch();
void ypbind_olddispatch();
void ypbind_get_binding();
void ypbind_set_binding();
void ypbind_send_setdom();
struct domain *ypbind_point_to_domain();
bool ypbind_broadcast_ack();
void ypbind_ping();
void ypbind_init_default();
void broadcast_proc_exit();

extern bool xdr_ypdomain_wrap_string();
extern bool xdr_ypbind_resp();

/******************************************************************************/
/*	usage - ypbind -S domainname,server1[,server2,server3,server4]        */
/*	The -S option allows the system administrator to startup ypbind       */
/*	in a secure mode.  The -S option locks this ypbind process into the   */
/*	specified domainname and server. 				      */
/*	It will not accept any other system as its server.  		      */
/* 	Consequently the secure option should only be used where either the   */
/* 	system itself is a server and you are setting it to itself or when a  */
/* 	very reliable server is available in your environment.                */
/******************************************************************************/

char srvname[4][64];
char srvaddr[4][64];
char setdomain[64];
struct hostent *hp;
struct sockaddr_in sock[4];
struct sockaddr_in *sin;
struct sockaddr_in myaddr;
int locked=0;
int preflock= -1;
int x_init_bind= 0;       /* x_init_bind is set during the initial attempt   */
                          /* to bind, if both the -S and -X options are used */

main(argc, argv)
	int argc;
	char *argv[];
{
	int pid;
	int i,j,k,t;
	int readfds;
	char *pname;
	char *securarg;
	extern char *optarg;    
	bool true;
	int exit_arg=0;         /* exit_arg is set if the -X option is used  */
	int arg;

	/*
	 * must be superuser to run 
	 */
	if (geteuid() != 0){
		(void) fprintf(stderr, "ypbind:  must be super user\n");
		(void) fflush(stderr);
		exit(1);
	}
	/* initialize my address reference */
	get_myaddress(&myaddr);
	/*
	 * unset any old YPBIND processes
	 */
	(void) pmap_unset(YPBINDPROG, YPBINDVERS);
	(void) pmap_unset(YPBINDPROG, YPBINDOLDVERS);
	/* 
	 * process security arguments
	 */
	while ((arg = getopt(argc, argv, "XS:")) != EOF) {
                switch (arg) {

		case 'X':       /* Initial Bind Option */
		        exit_arg = TRUE;
		        break;

/******ULTRIX SECUREYP *******/
                case 'S':               /* specify secure domainname.server */
                        strcpy(&securarg, &optarg);
			for ( t=0,j=0,i=0,k=0;securarg[i] != '\0'; i++ )
			{
			if(!locked)
				if(securarg[i] == ',')
					{
					setdomain[t]='\0';
					setdomainname(setdomain,strlen(setdomain));
					locked=1;
					}
				else setdomain[t++]=securarg[i];
			else	{
				if(securarg[i] == ',')
					{
					srvname[k++][j]='\0';
					if(k > 3)
						{
						fprintf(stderr,"Maximum of four servers selectable\n");
						exit(1);
						}
					
					j=0;
					}
				else	srvname[k][j++]=securarg[i];
				}
			}
			/* save number of servers in locked flag */
			if(!locked)
				{
				fprintf(stderr,"Usage: ypbind -S domain,server,server\n Up to four servers.\n");
				exit(1);
				}
			locked=k+1;
			srvname[k][j]='\0';
			for (i=0;i<locked;i++)
				{
        		bzero((caddr_t)&sock[i], sizeof (struct sockaddr_in));
        		sin = &sock[i];
        		sin->sin_family = AF_INET;
        		sin->sin_port = 0;
        		sin->sin_addr.s_addr = inet_addr(srvname[i]);
        		if (sin->sin_addr.s_addr == -1) {
				hp=gethostbyname(srvname[i]);
				if (hp == NULL) {
                        		fprintf(stderr, "arp: %s: unknown host\n", srvname[i]);
                        		exit(1);
					}
                		bcopy((char *)hp->h_addr, (char *)&sin->sin_addr,sizeof sin->sin_addr);
				}
			/* show preference to the local server */
			if(sin->sin_addr.s_addr == myaddr.sin_addr.s_addr)
				preflock=i;
			}
                break;
/*******ULTRIX SECUREYP *******/
		}
	}

	/*  Initial Bind Option (-X)
	 * 
	 *  If the -X option is specified, YPbind will initially attempt
	 *  to bind to a server BEFORE backgrounding itself.  If YPbind
	 *  fails to bind to a server after BINDING_TRIES attempts, YPbind
	 *  will exit.  
	 */

	 if (exit_arg) {

   	 	/* Initial binding with -S option */
    		if (locked) {
      			x_init_bind = TRUE;   
     	 		ypbind_locked (preflock,YPVERS);
  			x_init_bind = FALSE;
    		}

    		/* Initial binding without -S option */
    		else {
     			ypbind_init_default();
    		}
  
		/* Exit YPbind if the initial binding failed */

    		if (!(current_domain->dom_boundp)) {
      		(void) fprintf (stderr,"ypbind exiting: cannot bind to server.\n");
      		(void) fflush (stderr);
      		exit(1);
    		}
  	}

 	if (silent) {

	  	pid = fork();
		
		if (pid == -1) {
			(void) fprintf(stderr, "ypbind:  fork failure.\n");
			(void) fflush(stderr);
			abort();
		}
	
		if (pid != 0) {
			/*
			 * Gross hack for last minute fix for release 3.0 FCS.
			 * Make sure our child is registered and running
			 * before we exit; the real solution is to do the
			 * registration BEFORE the fork, but for historical
			 * reasons it was not done.  This prevents racing
			 * with other processes started by rc.local.
			 */
			for(;;) {
				if (pmap_getport(&myaddr,
				    YPBINDPROG, YPBINDVERS, IPPROTO_UDP))
					exit(0);
				sleep(2);
			}
		}
	
		for (t = 0; t < 20; t++) {
			(void) close(t);
		}
 		(void) open("/", 0);
 		(void) dup2(0, 1);
 		(void) dup2(0, 2);

 		t = open("/dev/tty", 2);
	
 		if (t >= 0) {
 			(void) ioctl(t, TIOCNOTTY, (char *)0);
 			(void) close(t);
 		}
	}

	if ((int) signal(SIGCHLD, broadcast_proc_exit) == -1) {
		(void) fprintf(stderr,
		    "ypbind:  Can't catch broadcast process exit signal.\n");
		(void) fflush(stderr);
		abort();
	}

	if ((tcphandle = svctcp_create(RPC_ANYSOCK,
	    RPCSMALLMSGSIZE, RPCSMALLMSGSIZE)) == NULL) {
		(void) fprintf(stderr, "ypbind:  can't create tcp service.\n");
		(void) fflush(stderr);
		abort();
	}

	if (!svc_register(tcphandle, YPBINDPROG, YPBINDVERS,
	    ypbind_dispatch, IPPROTO_TCP) ) {
		(void) fprintf(stderr,
		    "ypbind:  can't register tcp service.\n");
		(void) fflush(stderr);
		abort();
	}

	if ((udphandle = svcudp_bufcreate(RPC_ANYSOCK,
	    RPCSMALLMSGSIZE, RPCSMALLMSGSIZE)) == (SVCXPRT *) NULL) {
		(void) fprintf(stderr, "ypbind:  can't create udp service.\n");
		(void) fflush(stderr);
		abort();
	}

	if (!svc_register(udphandle, YPBINDPROG, YPBINDVERS,
	    ypbind_dispatch, IPPROTO_UDP) ) {
		(void) fprintf(stderr,
		    "ypbind:  can't register udp service.\n");
		(void) fflush(stderr);
		abort();
	}

	if (!svc_register(tcphandle, YPBINDPROG, YPBINDOLDVERS,
	    ypbind_olddispatch, IPPROTO_TCP) ) {
		(void) fprintf(stderr,
		    "ypbind:  can't register tcp service.\n");
		(void) fflush(stderr);
		abort();
	}

	if (!svc_register(udphandle, YPBINDPROG, YPBINDOLDVERS,
	    ypbind_olddispatch, IPPROTO_UDP) ) {
		(void) fprintf(stderr,
		    "ypbind:  can't register udp service.\n");
		(void) fflush(stderr);
		abort();
	}
	/* If we are running in locked mode lock up the domain and server */
	if(locked)
		{
	 	ypbind_locked(preflock,YPVERS);
		(void) fprintf(stderr,"YPBIND is locked\n");
		}
	for (;;) {
		readfds = svc_fds;
		errno = 0;

		switch ( (int) select(32, &readfds, NULL, NULL, NULL) ) {

		case -1:  {
		
			if (errno != EINTR) {
			    (void) fprintf (stderr,
			    "ypbind: bad fds bits in main loop select mask.\n");
			}

			break;
		}

		case 0:  {
			(void) fprintf (stderr,
			    "ypbind:  invalid timeout in main loop select.\n");
			break;
		}

		default:  {
			svc_getreq (readfds);
			break;
		}
		
		}
	}
}

/*
 * ypbind_dispatch and ypbind_olddispatch are wrappers for dispatch which
 * remember which protocol the requestor is looking for.  The theory is,
 * that since YPVERS and YPBINDVERS are defined in the same header file, if
 * a request comes in on the old binder protocol, the requestor is looking
 * for the old yp server.
 */
void
ypbind_dispatch(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	dispatch(rqstp, transp, (unsigned short) YPVERS);
}

void
ypbind_olddispatch(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	dispatch(rqstp, transp, (unsigned short) YPOLDVERS);
}

/*
 * This dispatches to binder action routines.
 */
void
dispatch(rqstp, transp, vers)
	struct svc_req *rqstp;
	SVCXPRT *transp;
	unsigned short vers;
{
	switch (rqstp->rq_proc) {

	case YPBINDPROC_NULL:
		if (!svc_sendreply(transp, xdr_void, 0) ) {
			(void) fprintf(stderr,
			    "ypbind:  Can't reply to rpc call.\n");
		}

		break;

	case YPBINDPROC_DOMAIN:
		ypbind_get_binding(rqstp, transp, vers);
		break;

	case YPBINDPROC_SETDOM:
		ypbind_set_binding(rqstp, transp, vers);
		break;

	default:
		svcerr_noproc(transp);
		break;

	}
}

/*
 * This is a Unix SIGCHILD handler which notices when a broadcaster child
 * process has exited, and retrieves the exit status.  The broadcaster pid
 * is set to 0.  If the broadcaster succeeded, dom_report_success will be
 * be set to -1.
 */

void
broadcast_proc_exit()
{
	int pid;
	union wait wait_status;
	register struct domain *pdom;

	pid = 0;

	for (;;) {
		pid = wait3(&wait_status, WNOHANG, NULL);

		if (pid == 0) {
			return;
		} else if (pid == -1) {
			return;
		}
		
		for (pdom = known_domains; pdom != (struct domain *)NULL;
		    pdom = pdom->dom_pnext) {
			    
			if (pdom->dom_broadcaster_pid == pid) {
				pdom->dom_broadcaster_pid = 0;

				if ((wait_status.w_termsig == 0) &&
				    (wait_status.w_retcode == 0))
					pdom->dom_report_success = -1;

			}
		}
	}

}

/*
 * This returns the current binding for a passed domain.
 */
void
ypbind_get_binding(rqstp, transp, vers)
	struct svc_req *rqstp;
	register SVCXPRT *transp;
	unsigned short vers;
{
	char domain_name[YPMAXDOMAIN + 1];
	char *pdomain_name = domain_name;
	char *pname;
	struct ypbind_resp response;
	bool true;
	char outstring[YPMAXDOMAIN + 256];
	int broadcaster_pid;
	struct domain *v1binding;
	if (!svc_getargs(transp, xdr_ypdomain_wrap_string, &pdomain_name) ) {
		svcerr_decode(transp);
		return;
	}

	if ( (current_domain = ypbind_point_to_domain(pdomain_name, vers) ) !=
	    (struct domain *) NULL) {

		/*
		 * Ping the server to make sure it is up.
		 */
		 
		if (current_domain->dom_boundp) {
			ypbind_ping(current_domain);
		}
		/*
		 * Bound or not, return the current state of the binding.
		 */

		if (current_domain->dom_boundp) {
			response.ypbind_status = YPBIND_SUCC_VAL;
			response.ypbind_respbody.ypbind_bindinfo.ypbind_binding_addr =
			    current_domain->dom_serv_addr;
			response.ypbind_respbody.ypbind_bindinfo.ypbind_binding_port = 
			    current_domain->dom_serv_port;
		} else {
			response.ypbind_status = YPBIND_FAIL_VAL;
			response.ypbind_respbody.ypbind_error =
			    YPBIND_ERR_NOSERV;
		}
		    
	} else {
		response.ypbind_status = YPBIND_FAIL_VAL;
		response.ypbind_respbody.ypbind_error = YPBIND_ERR_RESC;
	}

	if (!svc_sendreply(transp, xdr_ypbind_resp, &response) ) {
		(void) fprintf(stderr,
		    "ypbind:  Can't respond to rpc request.\n");
	}

	if (!svc_freeargs(transp, xdr_ypdomain_wrap_string, &pdomain_name) ) {
		(void) fprintf(stderr,
		    "ypbind:  ypbind_get_binding can't free args.\n");
	}
	if(locked && (!current_domain->dom_boundp))
		ypbind_locked(preflock,vers);

	else 	if ((current_domain) && (!current_domain->dom_boundp) &&
	    	(!current_domain->dom_broadcaster_pid)) {

		/* The current domain is unbound, and there is no broadcaster 
		 * process active now.  Fork off a child who will yell out on 
		 * the net.  Because of the flavor of request we're making of 
		 * the server, we only expect positive ("I do serve this
		 * domain") responses.
		 */
		broadcast_domain = current_domain;
		broadcast_domain->dom_report_success++;
		pname = current_domain->dom_name;
			
		if ( (broadcaster_pid = fork() ) == 0) {
			(void) clnt_broadcast(YPPROG, vers,
			    YPPROC_DOMAIN_NONACK, xdr_ypdomain_wrap_string,
			    &pname, xdr_int, &true, ypbind_broadcast_ack);
			    
			if (current_domain->dom_boundp) {
				
				/*
				 * Send out a set domain request to our parent
				 */
				ypbind_send_setdom(pname,
				    current_domain->dom_serv_addr,
				    current_domain->dom_serv_port, vers);
				    
				if (current_domain->dom_report_success > 0) {
					(void) sprintf(outstring,
					 "ypbind: server for domain \"%s\" OK",
					    pname);
					writeit(outstring);
				}
					
				exit(0);
			} else {
				/*
				 * Hack time.  If we're looking for a current-
				 * version server and can't find one, but we
				 * do have a previous-version server bound, then
				 * suppress the console message.
				 */
				if (vers == YPVERS && ((v1binding =
				   ypbind_point_to_domain(pname, YPOLDVERS) ) !=
				    (struct domain *) NULL) &&
				    v1binding->dom_boundp) {
					    exit(1);
				}
				
				(void) sprintf(outstring,
	      "\nypbind: server not responding for domain \"%s\"; still trying", pname);
				writeit(outstring);
				exit(1);
			}

		} else if (broadcaster_pid == -1) {
			(void) fprintf(stderr,
			    "ypbind:  broadcaster fork failure.\n");
		} else {
			current_domain->dom_broadcaster_pid = broadcaster_pid;
		}
	}
}

static int
writeit(s)
	char *s;
{
	FILE *f;

	if ((f = fopen("/dev/console", "w")) != NULL) {
		(void) fprintf(f, "%s.\n", s);
		(void) fclose(f);
	}
	
}

/*
 * This sends a (current version) ypbind "Set domain" message back to our
 * parent.  The version embedded in the protocol message is that which is passed
 * to us as a parameter.
 */
void
ypbind_send_setdom(dom, addr, port, vers)
	char *dom;
	struct in_addr addr;
	unsigned short int port;
	unsigned short int vers;
{
	struct ypbind_setdom req;
	int socket;
	struct timeval timeout;
	struct timeval intertry;
	CLIENT *client;

	strcpy(req.ypsetdom_domain, dom);
	req.ypsetdom_addr = addr;
	req.ypsetdom_port = port;
	req.ypsetdom_vers = vers;
	myaddr.sin_port = htons(udphandle->xp_port);
	socket = RPC_ANYSOCK;
	timeout.tv_sec = SETDOMTOTTIM;
	intertry.tv_sec = SETDOMINTERTRY;
	timeout.tv_usec = intertry.tv_usec = 0;


	if ((client = clntudp_bufcreate (&myaddr, YPBINDPROG, YPBINDVERS,
	    intertry, &socket, RPCSMALLMSGSIZE, RPCSMALLMSGSIZE) ) != NULL) {
	        client->cl_auth = authunix_create_default();
		clnt_call(client, YPBINDPROC_SETDOM, xdr_ypbind_setdom,
		    &req, xdr_void, 0, timeout);
		auth_destroy(client->cl_auth);
		clnt_destroy(client);
		close(socket);
	} else {
		clnt_pcreateerror(
		    "ypbind(ypbind_send_setdom): clntudp_create error");
	}
}

/*
 * This sets the internet address and port for the passed domain to the
 * passed values, and marks the domain as supported.  This accepts both the
 * old style message (in which case the version is assumed to be that of the
 * requestor) or the new one, in which case the protocol version is included
 * in the protocol message.  This allows our child process (which speaks the
 * current protocol) to look for yp servers on behalf old-style clients.
 */
void
ypbind_set_binding(rqstp, transp, vers)
	struct svc_req *rqstp;
	register SVCXPRT *transp;
	unsigned short vers;
{
	struct ypbind_setdom req;
	struct ypbind_oldsetdom oldreq;
	unsigned short version;
	struct in_addr addr;
        struct sockaddr_in *who;
	unsigned short int port;
	char *domain;
	int i;

	if (vers == YPVERS) {

		if (!svc_getargs(transp, xdr_ypbind_setdom, &req) ) {
			svcerr_decode(transp);
			return;
		}

		version = req.ypsetdom_vers;
		addr = req.ypsetdom_addr;
		port = req.ypsetdom_port;
		domain = req.ypsetdom_domain;
	} else {

		if (!svc_getargs(transp, _xdr_ypbind_oldsetdom, &oldreq) ) {
			svcerr_decode(transp);
			return;
		}

		version = vers;
		addr = oldreq.ypoldsetdom_addr;
		port = oldreq.ypoldsetdom_port;
		domain = oldreq.ypoldsetdom_domain;
	}

        /* find out who originated the request */
        who = svc_getcaller(transp);

        /* Now check the credentials */
        if (rqstp->rq_cred.oa_flavor == AUTH_UNIX) {
                if (((struct authunix_parms *)rqstp->rq_clntcred)->aup_uid != 0)
 {
                        fprintf(stderr,"ypbind: Set domain request to host %s,",
                                        inet_ntoa(addr));
                        fprintf(stderr," from host %s, failed (not root).\n",
                                        inet_ntoa(who->sin_addr));
                              svcerr_systemerr(transp);
                        return;
                }
         } else {
                fprintf(stderr, "ypbind: Set domain request to host %s,",
                                inet_ntoa(addr));
                fprintf(stderr," from host %s, failed (credentials).\n",
                                inet_ntoa(who->sin_addr));
                svcerr_weakauth(transp);
                return;
        }

	if (!svc_sendreply(transp, xdr_void, 0) ) {
		fprintf(stderr, "ypbind:  Can't reply to rpc call.\n");
	}

	if(locked)
		{
		if(strcmp(setdomain,domain) == 0)
			{
			if((current_domain = ypbind_point_to_domain(domain,version)) != (struct domain *) NULL) 
			for (i=0;i<locked;i++)
				{
        			sin = &sock[i];
				if(sin->sin_addr.s_addr == addr.s_addr)
					{
					ypbind_locked(i,version);
					}
				}
			}
		}
	else if ( (current_domain = ypbind_point_to_domain(domain,
	    version) ) != (struct domain *) NULL) {
		current_domain->dom_serv_addr = addr;
		current_domain->dom_serv_port = port;
		current_domain->dom_boundp = TRUE;
		current_domain->ping_clnt = (CLIENT *)NULL;
	}
}
/*
 * This returns a pointer to a domain entry.  If no such domain existed on
 * the list previously, an entry will be allocated, initialized, and linked
 * to the list.  Note:  If no memory can be malloc-ed for the domain structure,
 * the functional value will be (struct domain *) NULL.
 */
static struct domain *
ypbind_point_to_domain(pname, vers)
	register char *pname;
	unsigned short vers;
{
	register struct domain *pdom;
	
	for (pdom = known_domains; pdom != (struct domain *)NULL;
	    pdom = pdom->dom_pnext) {
		if (!strcmp(pname, pdom->dom_name) && vers == pdom->dom_vers)
			return (pdom);
	}
	
	/* Not found.  Add it to the list */
	
	if (pdom = (struct domain *)malloc(sizeof (struct domain))) {
		pdom->dom_pnext = known_domains;
		known_domains = pdom;
		strcpy(pdom->dom_name, pname);
		pdom->dom_vers = vers;
		pdom->dom_boundp = FALSE;
		pdom->ping_clnt = (CLIENT *)NULL;
		pdom->dom_report_success = -1;
		pdom->dom_broadcaster_pid = 0;
	}
	
	return (pdom);
}

/*
 * This is called by the broadcast rpc routines to process the responses 
 * coming back from the broadcast request. Since the form of the request 
 * which is used in ypbind_broadcast_bind is "respond only in the positive  
 * case", the internet address of the responding server will be picked up 
 * from the saddr parameter, and stuffed into the domain.  The domain's
 * boundp field will be set TRUE.  Because this function returns TRUE, 
 * the first responding server will be the bound server for the domain.
 */
bool
ypbind_broadcast_ack(ptrue, saddr)
	bool *ptrue;
	struct sockaddr_in *saddr;
{
	int i;
	/* if ypbind is in locked mode only bind to the selected servers */
	/* This code should never be hit but just in case its here  */
	if(locked)
		{
		if ( (current_domain = ypbind_point_to_domain(setdomain,YPVERS) ) !=
	    (struct domain *) NULL)
			exit;
		for (i=0;i<locked;i++)
			{
        		sin = &sock[i];
			if(sin->sin_addr.s_addr == saddr->sin_addr.s_addr)
				{
				ypbind_locked(i,YPVERS);
				}
			}
		}
	else	{
		current_domain->dom_boundp = TRUE;
		current_domain->dom_serv_addr = saddr->sin_addr;
		current_domain->dom_serv_port = saddr->sin_port;
		current_domain->ping_clnt = (CLIENT *)NULL;
		}
	return(TRUE);
}

/*
 * This checks to see if a server bound to a named domain is still alive and
 * well.  If he's not, boundp in the domain structure is set to FALSE.
 */
void
ypbind_ping(pdom)
	struct domain *pdom;

{
	struct sockaddr_in addr;
	enum clnt_stat clnt_stat;
	struct timeval timeout;
	struct timeval intertry;
	
	timeout.tv_sec = PINGTOTTIM;
	timeout.tv_usec = intertry.tv_usec = 0;
	if (pdom->ping_clnt == (CLIENT *)NULL) {
		bzero(&addr, sizeof(struct sockaddr_in));
		intertry.tv_sec = PINGINTERTRY;
		addr.sin_addr = pdom->dom_serv_addr;
		addr.sin_family = AF_INET;
		addr.sin_port = pdom->dom_serv_port;
		ping_sock = RPC_ANYSOCK;
		if ((pdom->ping_clnt = clntudp_bufcreate(&addr, YPPROG,
		    pdom->dom_vers, intertry, &ping_sock,
		    RPCSMALLMSGSIZE, RPCSMALLMSGSIZE)) == (CLIENT *)NULL) {
			clnt_pcreateerror("ypbind_ping) clntudp_create error");
			pdom->dom_boundp = FALSE;
			return;
		}
	}
	if ((clnt_stat = (enum clnt_stat) clnt_call(pdom->ping_clnt,
	    YPPROC_NULL, xdr_void, 0, xdr_void, 0, timeout)) != RPC_SUCCESS) {
		pdom->dom_boundp = FALSE;
		clnt_destroy(pdom->ping_clnt);	
		pdom->ping_clnt = (CLIENT *)NULL;
		close(ping_sock);
	}
}

/*
 * Preloads the default domain's domain binding. Domain binding for the
 * local node's default domain for both the current version, and the
 * previous version will be set up.  Bindings to servers which serve the
 * domain for both versions may additionally be made.  
 * 
 * Ypbind_init_default returns after binding or after BINDING_TRIES attempts
 * to bind for each version.  
 */
static void
ypbind_init_default()
{
	char domain[256];
	char *pname = domain;
	int true;
	int binding_tries;

	if (getdomainname(domain, 256) == 0) {

		/* Exit if the default domainname is unspecified */
	  	if (strcmp(domain,"") == 0) {
      			(void) fprintf(stderr,"ypbind exiting: default domainname not specified\n");
      			(void) fflush(stderr);
      			exit(1);
    		}

  		current_domain = ypbind_point_to_domain(domain, YPVERS);
		current_domain->dom_boundp = FALSE;
		if (current_domain == (struct domain *) NULL) {
			abort();
		}
		
		for (binding_tries = 0;
		    ((!current_domain->dom_boundp) &&
		    (binding_tries < BINDING_TRIES) ); binding_tries++) {
			(void) clnt_broadcast(YPPROG, current_domain->dom_vers,
			    YPPROC_DOMAIN_NONACK, xdr_ypdomain_wrap_string,
			    &pname, xdr_int, &true, ypbind_broadcast_ack);
			
		}
		
		current_domain = ypbind_point_to_domain(domain, YPOLDVERS);

		if (current_domain == (struct domain *) NULL) {
			abort();
		}
		
		for (binding_tries = 0;
		    ((!current_domain->dom_boundp) &&
		    (binding_tries < BINDING_TRIES) ); binding_tries++) {
			(void) clnt_broadcast(YPPROG, current_domain->dom_vers,
			    YPPROC_DOMAIN_NONACK, xdr_ypdomain_wrap_string,
			    &pname, xdr_int, &true, ypbind_broadcast_ack);
			
		}
	}
}
ypbind_locked(srvpref,version)
int srvpref;
unsigned short version;
{
	struct dom_binding domb;
	enum clnt_stat clnt_stat;
	int i,j,done,true;
	char domain[256];
	char *pname = setdomain;
	int count;

	if (getdomainname(domain, 256) == 0) {
	 	
		/* Exit if the default domainname is unspecified */
	  	if (strcmp(domain,"") == 0) {
      			(void) fprintf(stderr,"ypbind exiting: default domainname not specified\n");
      			(void) fflush(stderr);
      			exit(1);
    		}

  		current_domain = ypbind_point_to_domain(domain, version);
		current_domain->dom_boundp = FALSE;
	}

	/* check if there is a preferred local server */
	if(srvpref != -1)
		i=srvpref;
	else i=0;

	/* Loop until we find a server to bind to.
	 * If the Initial Binding option (-X) is set and this is the initial
	 * attempt to bind, return after binding or after BINDING_TRIES 
	 * attempts to bind to each server. 
	 */

	done=0;
	true=0;
	count=BINDING_TRIES * locked;   /* For Initial Bind only */

	while(!done)
	{
  		if (x_init_bind) {
      			count--;
      			if (count == 0) {
				done = 1;
      			}
    		}
        sin = &sock[i];
	i = (i+1) % locked;
        domb.dom_server_addr.sin_addr = sin->sin_addr;
        domb.dom_server_addr.sin_family = AF_INET;
        domb.dom_server_addr.sin_port = htons((u_short) 0);
        domb.dom_server_port = htons((u_short) 0);
        domb.dom_socket = RPC_ANYSOCK;
        if (domb.dom_client = clntudp_create(&(domb.dom_server_addr),
            YPPROG, version, udp_intertry, &(domb.dom_socket))) 
		{
                if((clnt_stat = (enum clnt_stat) clnt_call(domb.dom_client, YPPROC_DOMAIN,xdr_ypdomain_wrap_string,&pname,xdr_int,&true,udp_timeout)) == RPC_SUCCESS)			
			if(true)
				{
                    		done=1;
				current_domain->dom_boundp = TRUE;
				current_domain->dom_serv_addr = sin->sin_addr;
				current_domain->dom_serv_port = sin->sin_port;
				current_domain->ping_clnt = (CLIENT *)NULL;
				}
                clnt_destroy(domb.dom_client);
                close(domb.dom_socket);
		}
	}
}


