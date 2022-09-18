

# ifndef lint
static char *sccsid = "@(#)udp.c	4.1	(ULTRIX)	7/2/90";
# endif not lint

/****************************************************************
 *								*
 *  Licensed to Digital Equipment Corporation, Maynard, MA	*
 *		Copyright 1985 Sun Microsystems, Inc.		*
 *			All rights reserved.			*
 *								*
 ****************************************************************/
/**/
/*
 *	Modification history:
 *	~~~~~~~~~~~~~~~~~~~~
 *
 *	revision			comments
 *	--------	-----------------------------------------------
 *
 *	01-Jun-89	Fred Glover
 *			Update for nfssrc 4.0
 *
 *	22-Nov-88	Fred Glover 
 *			Modify hash() to guarantee positive offset
 *
 *	18-Jan-88	fries
 *			Added Header and Copyright notice.
 *
 *	
 */

/*
 * this file consists of routines to support call_udp();
 * client handles are cached in a hash table;
 * clntudp_create is only called if (site, prog#, vers#) cannot
 * be found in the hash table;
 * a cached entry is destroyed, when remote site crashes
 */

#include <stdio.h>
#include <rpc/rpc.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <netdb.h>
#define MAX_HASHSIZE 100


char *malloc();
char *xmalloc();
static int mysock = RPC_ANYSOCK;
extern int debug;
extern int HASH_SIZE;

struct cache {
	char *host;
	int prognum;
	int versnum;
	int sock;
	CLIENT *client;
	struct cache *nxt;
};

struct cache *table[MAX_HASHSIZE];
int cache_len = sizeof(struct cache);

hash(name)
	char *name;
{
	int len, i;
	unsigned int c;		/* Ultrix Mod - fsg */

	c = 0;
	len = strlen(name);
	for (i = 0; i< len; i++) {
		c = c +(int) name[i];
	}
	c = c %HASH_SIZE;
	return(c);
}

/*
 * find_hash returns the cached entry;
 * it returns NULL if not found;
 */
struct cache *
find_hash(host, prognum, versnum)
	char *host;
	int prognum, versnum;
{
	struct cache *cp;

	cp = table[hash(host)];
	while ( cp != NULL) {
		if (strcmp(cp->host, host) == 0 &&
		 cp->prognum == prognum && cp->versnum == versnum) {
			/*found */
			return(cp);
		}
		cp = cp->nxt;
	}
	return(NULL);
}

struct cache *
add_hash(host, prognum, versnum)
	char *host;
	int prognum, versnum;
{
	struct cache *cp;
	int h;

	if ((cp = (struct cache *) xmalloc(cache_len)) == NULL ) {
		return(NULL);	/* malloc error */
	}
	if ((cp->host = xmalloc(strlen(host)+1)) == NULL ) {
		free(cp);
		return(NULL);	/* malloc error */
	}
	(void) strcpy(cp->host, host);
	cp->prognum = prognum;
	cp->versnum = versnum;
	h = hash(host);
	cp->nxt = table[h];
	table[h] = cp;
	return(cp);
}

void
delete_hash(host) 
	char *host;
{
	struct cache *cp;
	struct cache *cp_prev = NULL;
	struct cache *next;
	int h;

	/* if there is more than one entry with same host name;
	 * delete has to be recurrsively called */

	h = hash(host);
	next = table[h];
	while ((cp = next) != NULL) {
		next = cp->nxt;
		if (strcmp(cp->host, host) == 0) {
			if (cp_prev == NULL) {
				table[h] = cp->nxt;
			}
			else {
				cp_prev->nxt = cp->nxt;
			}
			if (debug)
				printf("delete hash entry (%x), %s \n", cp, host);
			clnt_destroy(cp->client);
			free(cp->host);
			free(cp);
		}
		else {
			cp_prev = cp;
		}
	}
}

call_udp(host, prognum, versnum, procnum, inproc, in, outproc, out, valid_in, t)
	char *host;
	xdrproc_t inproc, outproc;
	char *in, *out;
	int valid_in;
	int t;
{
	struct sockaddr_in server_addr;
	enum clnt_stat clnt_stat;
	struct hostent *hp;
	struct timeval timeout, tottimeout;
	struct cache *cp;

	/* Get a long lived socket to talk to the status monitor with */
	if (mysock == RPC_ANYSOCK) {
		int	dontblock = 1;
		mysock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
                if (mysock < 0) {
			fprintf(stderr,"rpc.lockd : cannot send because socket could not be created.\n");
                        return(-1);
                }
                /* attempt to bind to prov port */
                (void)bindresvport(mysock, (struct sockaddr_in *)0);
                /* the sockets rpc controls are non-blocking */
                (void)ioctl(mysock, FIONBIO, (char *) &dontblock);
	}
	if ((cp = find_hash(host, prognum, versnum)) == NULL) {
		if ((cp = add_hash(host, prognum, versnum)) == NULL) {
			fprintf(stderr, "udp cannot send due to out of cache\n");
			return(-1);
		}
		if (debug)
			printf("(%x):[%s, %d, %d] is a new connection\n", cp, host, prognum, versnum);

		if ((hp = gethostbyname(host)) == NULL)
			return ((int) RPC_UNKNOWNHOST);

		timeout.tv_usec = 0;
		if (t == 0) {	/* Ultrix Mod - fsg */
			timeout.tv_sec = 0;
		} else {
			timeout.tv_sec = 5;
		}

		bcopy(hp->h_addr, &server_addr.sin_addr, hp->h_length);
		server_addr.sin_family = AF_INET;
		server_addr.sin_port =  0;
		if ((cp->client = clntudp_create(&server_addr, prognum,
 	 	versnum, timeout, &mysock)) == NULL)
			return ((int) rpc_createerr.cf_stat);

	}
	else {
		if (valid_in == 0) { /* cannot use cache */
		if (debug)
			printf("(%x):[%s, %d, %d] is a new connection\n", cp, host, prognum, versnum);
			if ((hp = gethostbyname(host)) == NULL)
				return ((int) RPC_UNKNOWNHOST);
			/* get rid of previous client struct */
			if (cp->client != NULL) clnt_destroy(cp->client);

			timeout.tv_usec = 0;
			if (t == 0) {	/* Ultrix Mod - fsg */
				timeout.tv_sec = 0;
			} else {
				timeout.tv_sec = 5;
			}

			bcopy(hp->h_addr, &server_addr.sin_addr, hp->h_length);
			server_addr.sin_family = AF_INET;
			server_addr.sin_port =  0;
			if ((cp->client = clntudp_create(&server_addr, prognum,
 	 		versnum, timeout, &mysock)) == NULL)
				return ((int) rpc_createerr.cf_stat);
		}
	}

	cp->sock = mysock;
	tottimeout.tv_sec = t;
	tottimeout.tv_usec = 0;
	clnt_stat = clnt_call(cp->client, procnum, inproc, in,
	    outproc, out, tottimeout);
	return ((int) clnt_stat);
}
