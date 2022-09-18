/* Connection_mode Client Example */

#ifndef lint
static char *sccsid = "@(#)cots_client.c	4.1 (ULTRIX)	11/13/90";
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

extern int errno;
int net;
struct t_info t_open_info;   /* transport char. from transport */
struct t_info t_getinfo_info;
struct tcp_options tcp_opts;
struct t_optmgmt t_optm_req;
struct t_optmgmt t_optm_ret;
struct sockaddr_in sin;
struct	servent *sp;
char	*hostname;
struct hostent *host;
#define MAXDSIZE 512
char snd_buf[MAXDSIZE];
char rcv_buf[MAXDSIZE];
int n;
int status;
struct t_call t_conn_sndcall;
struct t_call t_conn_rcvcall;
struct t_call t_rcvconn_call;

struct t_discon discon;
int t_rcv_flags;

main(argc, argv)
     int argc;
     char *argv[];
{

  char destin[255];

  if ((net = t_open("tcp", O_RDWR|O_NONBLOCK, &t_open_info)) < 0) {
    t_error("t_open failed");
    exit(t_errno);
  }

  status = t_getinfo(net, &t_getinfo_info);


  /*
   * t_bind - bind an address to a transport endpoint
   *
   */

  if (t_bind(net, 0, 0) < 0) {
    t_error("iexample: t_bind error");
    exit(1);
  }

  t_optm_req.opt.len = 0;
  t_optm_req.flags = T_DEFAULT;
  t_optm_ret.opt.maxlen = sizeof(struct tcp_options);
  t_optm_ret.opt.buf = (char *) &tcp_opts;

  status = t_optmgmt(net, &t_optm_req, &t_optm_ret);
  if (status < 0) {
    t_error("iexample: t_optmgmt error");
    exit(1);
  }
  t_optm_req.opt.len = 0;
  t_optm_req.flags = T_DEFAULT;
  t_optm_ret.opt.maxlen = sizeof(struct tcp_options);
  t_optm_ret.opt.buf = (char *) &tcp_opts;

  status = t_optmgmt(net, &t_optm_req, &t_optm_ret);
  if (status < 0) {
    t_error("iexample: t_optmgmt error");
    exit(1);
  }

  printf("host :");
  scanf("%s",destin);
  
  host = gethostbyname(destin);
  
  if (host) {
    sin.sin_family = host->h_addrtype;
    bcopy(host->h_addr, (caddr_t)&sin.sin_addr, host->h_length);
    hostname = host->h_name;
  }

  sin.sin_port = 200; /* try to connect to port 200 */
  t_conn_sndcall.addr.len = sizeof (struct sockaddr_in);
  t_conn_sndcall.addr.buf = (char *) &sin;
  t_conn_sndcall.opt.len = 0;
  t_conn_sndcall.udata.len = 0;
  t_conn_rcvcall.addr.maxlen = sizeof (struct sockaddr_in);
  t_conn_rcvcall.addr.buf = (char *) &sin;
  t_conn_rcvcall.opt.maxlen = sizeof(struct tcp_options);
  t_conn_rcvcall.opt.buf = (char *) &tcp_opts;
  t_conn_rcvcall.udata.maxlen = 0;
  t_rcvconn_call.addr.maxlen = sizeof (struct sockaddr_in);
  t_rcvconn_call.addr.buf = (char *) &sin;
  t_rcvconn_call.opt.maxlen = sizeof(struct tcp_options);
  t_rcvconn_call.opt.buf = (char *) &tcp_opts;
  t_rcvconn_call.udata.maxlen = 0;
  t_rcvconn_call.udata.buf = 0;
  
  if ((t_connect(net, &t_conn_sndcall, &t_conn_rcvcall)) < 0) {
    if (t_errno == TNODATA) {
      while (1) {
	status = t_rcvconnect(net, &t_rcvconn_call);

	if (status < 0) {
	  if (t_errno == TLOOK) {
	    printf("Event %x came in\n",t_look(net));
	    (void) t_unbind(net);
	    (void) t_close(net);
	    exit(1);
	  }
	  if (t_errno != TNODATA) {
	    t_error("iexample: t_rcvconnect()");
	    (void) t_unbind(net);
	    (void) t_close(net);
	    exit(1);
	  }
	}
	else
	  break;
      }
    } else {
      t_error("iexample: t_connect()");
      (void) t_unbind(net);
      (void) t_close(net);
      exit(1);
    }
  }

  printf("calling t_snd with %d bytes of regular data\n",sizeof(snd_buf));
  n = t_snd(net, &snd_buf[0],sizeof(snd_buf) , 0); 

  if (n < 0) {
    if (t_errno == TLOOK) {
      printf("Generated a %X TLOOK error\n",t_look(net));
      (void) t_unbind(net);
      (void) t_close(net);
      exit(1);
    }
    t_error("iexample: t_snd error");
    (void) t_unbind(net);
    (void) t_close(net);
    exit(1);
  }
  printf("t_snd sent %d bytes\n",n);
    
  while (1) {
    n = t_rcv(net, rcv_buf, sizeof(rcv_buf), &t_rcv_flags);

    if (n < 0) {
      if (t_errno != TNODATA) {
	t_error("iexample: t_rcv error");
	(void) t_unbind(net);
	(void) t_close(net);
	exit(1);
      }
      else {
	t_error("iexample: NO data available");
      }
    }
    if (n > 0) break;
  }

  printf("t_rcv received %d bytes\n",n);

  if (t_rcv_flags & T_EXPEDITED)
    printf("data is expedited\n");
  else
    printf("data is normal\n");

  n = t_sndrel(net, (struct t_call *) 0);

  if (n < 0) {
    t_error("iexample: error in t_sndrel:");
    t_unbind(net);
    t_close(net);
    exit(1);
  }

  while (1) {
    n = t_rcvrel(net);

    if (n < 0) {
      if (t_errno != TLOOK && t_errno != TNOREL) {
	t_error("iexample: error in t_rcvrel:");
	t_unbind(net);
	t_close(net);
	exit(1);
      }
      else {
	if (t_errno == TNOREL)
	  t_error("iexample: NO T_ORDREL available");
	else {
	  t_error("iexample: TLOOK event");
	  t_unbind(net);
	  t_close(net);
	  exit(1);
	}
      }
    }
    if (n == 0) break;
  }
  t_unbind(net);
  t_close(net);
  exit(0);
}

