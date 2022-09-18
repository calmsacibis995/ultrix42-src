/* Connection_Mode Server Example */

#ifndef lint
static char *sccsid = "@(#)cots_server.c	4.1 (ULTRIX)	11/13/90";
#endif lint
 
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/file.h>
#include <netinet/in.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <sgtty.h>
#include <netdb.h>
#include <syslog.h>
#include <xti.h>

int	net,net1,n,n1;
extern	int errno;

main(argc, argv)
	char *argv[];
{
	int fromlen;
	struct sockaddr_in from;

	int status;

	status =  get_income();
	if (status != 0) 
	  exit(1);
	else {
	  sleep(10);
	  exit(0);
        }
}


doit(f, seq)
     int f,seq;
{
  int t_rcv_flags; 
  struct hostent *hp;
  char rcv_buf[512];
  char snd_buf[512];
  int n;

    
  while (1) {
    n = t_rcv(f,rcv_buf, sizeof(rcv_buf) ,&t_rcv_flags);

    if (n < 0) {
      if (t_errno != TNODATA) {
	t_error("rexample: t_rcv error");
	t_unbind(f);
	t_close(f);
	exit(1);
      }
      else {
	t_error("rexample: NO data available");
      }
    }
    if (n > 0) break;
  }

  printf("t_rcv received %d bytes\n",n);

  if (t_rcv_flags & T_EXPEDITED)
    printf("data is expedited\n");
  else
    printf("data is normal\n");
  
  printf("calling t_snd with %d bytes of regular data\n",sizeof(snd_buf));
  n = t_snd(f, &snd_buf[0],sizeof(snd_buf) , 0); 

  if (n < 0) {
    if (t_errno == TLOOK) {
      printf("Generated a %X TLOOK error\n",t_look(f));
      (void) t_unbind(f);
      (void) t_close(f);
      exit(1);
    }
    t_error("rexample: t_snd error");
    (void) t_unbind(f);
    (void) t_close(f);
    exit(1);
  }
  printf("t_snd sent %d bytes\n",n);

  while (1) {
    n = t_rcvrel(f);

    if (n < 0) {
      if (t_errno != TLOOK && t_errno != TNOREL) {
	t_error("rexample: error in t_rcvrel:");
	t_unbind(f);
	t_close(f);
	exit(1);	
      }
      else {
	if (t_errno == TLOOK) {
	  t_error("TLOOK error");
	  t_unbind(f);
	  t_close(f);
	  exit(1);
	}
	t_error("rexample: NO T_ORDREL available");
      }
    }
    if (n == 0) break;
  }

  n = t_sndrel(f, (struct t_call *) 0);

  if (n < 0) {
    t_error("rexample: error in t_sndrel:");
    t_unbind(f);
    t_close(f);
    exit(1);
  }

  t_unbind(f);
  t_close(f);
  exit(0);
}


int
get_income()
{
  struct sockaddr_in sname;
  struct servent *sp;
  int i;
  int child;

  struct t_call t_list_call;
  struct t_call *t_list_ptr;
  struct t_call t_snddis_call;
  struct t_bind t_bind_addr_req;
  struct t_bind t_bind_addr_req1;
  struct t_bind t_bind_addr_ret;
  struct t_info t_open_info;   /* transport char. from transport */
  int t_status;

  /*
   * Call t_open - establish a transport endpoint
   *
   */

  if ((net = t_open("tcp", O_RDWR, &t_open_info)) < 0) {
    t_error("rexample: t_open error");
    exit(1);
  }
    
  /*
   * t_bind - bind an address to a transport endpoint
   *
   */
  
  sname.sin_port = 200;         /* load port # */
  sname.sin_family = AF_INET;
  sname.sin_addr.s_addr = 0;


  t_bind_addr_req.addr.len = sizeof (struct sockaddr_in);
  t_bind_addr_req.addr.buf = (char *) &sname;
  t_bind_addr_req.qlen = 1;
  t_bind_addr_ret.addr.maxlen = sizeof (struct sockaddr_in);
  t_bind_addr_ret.addr.buf = (char *) &sname;
       

  if ((t_bind(net, &t_bind_addr_req, &t_bind_addr_ret)) < 0) {
    t_error("rexample: t_bind error");
    exit(1);
  }


  t_list_ptr = (struct t_call *) t_alloc(net, T_CALL_STR, T_ADDR);
  bcopy(&sname, t_list_ptr->addr.buf, t_list_ptr->addr.maxlen);

  t_status = t_listen(net, t_list_ptr);

  if (t_status < 0) {
    if (t_errno != TNODATA) {
      t_error("rexample: t_listen error");
      t_unbind(net);
      t_close(net);
      exit(1);
    }
  }

  printf("Have a incomming connection with sequence # %d\n",
	 t_list_ptr->sequence);
  printf("attempting to accept sequence # %d\n",
	     t_list_ptr->sequence);

  net1 = get_endpoint();
  if (t_status = t_accept(net,net1,t_list_ptr) < 0) {
    t_error("rexample: t_accept error");
    if (t_errno == TLOOK) {
      printf("event %x came in\n",t_look(net1));
    }
    exit(1);
  }
      
  fcntl(net1,F_SETOWN, getpid());
  child = fork();

  if (child == 0) {
    t_unbind(net);
    t_close(net);
    t_sync(net1);
    doit(net1, t_list_ptr->sequence);
  }
  else
    {
      printf("Forking Child process =%d for fd = %d seq=%d\n",
	     child,net1, t_list_ptr->sequence);
      t_unbind(net1);
      t_close(net1);
      t_free(t_list_ptr, T_CALL_STR);
    }
  return(0);
}

int
get_endpoint()
{
  struct sockaddr_in sname;
  struct servent *sp;
  int tmp_net;

  struct t_call t_list_call;
  struct t_bind t_bind_addr_req;
  struct t_bind t_bind_addr_req1;
  struct t_bind t_bind_addr_ret;
  struct t_info t_open_info;   /* transport char. from transport */
  int t_status;

	
/*
 * Call t_open - establish a transport endpoint
 *
 */

  if ((tmp_net = t_open("tcp", O_RDWR, &t_open_info)) < 0) {
    t_error("rexample: t_open error");
    exit(1);
  }

  /*
   * t_bind - bind an address to a transport endpoint
   *
   */

  sname.sin_port = 0;
  sname.sin_family = AF_INET;
  sname.sin_addr.s_addr = 0;


  t_bind_addr_req.addr.len = sizeof (struct sockaddr_in);
  t_bind_addr_req.addr.buf = (char *) &sname;
  t_bind_addr_req.qlen = 0;
  t_bind_addr_ret.addr.maxlen = sizeof (struct sockaddr_in);
  t_bind_addr_ret.addr.buf = (char *) &sname;
       

  if ((t_bind(tmp_net, &t_bind_addr_req, &t_bind_addr_ret)) < 0) {
    t_error("rexample: t_bind error");
    exit(1);
  }
  return(tmp_net);
}

