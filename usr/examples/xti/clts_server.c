/* Connectionless_mode Client Example */

#ifndef lint
static char *sccsid = "@(#)clts_server.c	4.1 (ULTRIX)	11/13/90";
#endif lint
 
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <setjmp.h>
#include <netdb.h>
#include <xti.h>
#include <fcntl.h>

int	net;
extern	int errno;
struct sockaddr_in sin;
char	*hostname;
char	hnamebuf[32];
struct t_call t_conn_sndcall;
struct t_call t_conn_rcvcall;
struct t_info t_open_info;   /* transport char. from transport */
struct t_unitdata unitdata;
int t_rcv_flags;
char snd_buf[6000];
char rcv_buf[6000];
struct hostent *host;
int scc,n;

main(argc, argv)
     int argc;
     char *argv[];
{

  host = gethostbyname("nil");
  
  if (host) {
    sin.sin_family = host->h_addrtype;
    bcopy(host->h_addr, (caddr_t)&sin.sin_addr, host->h_length);
    hostname = host->h_name;
  }

  sin.sin_port = 0;          /* don't set port till time to do connect */

	
/*
 * Call t_open - establish a transport endpoint
 *
 */

  if ((net = t_open("udp", O_RDWR, &t_open_info)) < 0) {
    t_error("iexamless: t_open error");
    return(1);
  }

	
/*
 * t_bind - bind an address to a transport endpoint
 *
 */

  if ((t_bind(net, 0, 0)) < 0) {
    t_error("iexamless: t_bind error");
    exit(1);
  }

  sin.sin_port = 200;
  unitdata.addr.len = sizeof(sin);
  unitdata.addr.buf = (char *) &sin;
  unitdata.opt.len = 0;
  unitdata.udata.len = sizeof(snd_buf);
  unitdata.udata.buf = snd_buf;
  unitdata.opt.len = 0;

  n = t_sndudata(net, &unitdata);

  if (n < 0) {
    if (t_errno != TNODATA) {
      t_error("iexamless: t_sndudata error");
      (void) t_close(net);
      exit(1);
    }
  }
    
  t_close(net);
  exit(0);
}

