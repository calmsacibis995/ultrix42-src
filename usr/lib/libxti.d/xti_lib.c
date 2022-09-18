#ifndef lint
static char *sccsid = "@(#)xti_lib.c	4.2 (ULTRIX) 11/14/90";
#endif /* lint */

/************************************************************************
 *
 * Copyright (c) Digital Equipment Corporation, 1990
 * All Rights Reserved.  Unpublished - rights reserved
 * under the copyright laws of the United States.
 * 
 * The software contained on this media is proprietary
 * to and embodies the confidential technology of 
 * Digital Equipment Corporation.  Possession, use,
 * duplication or dissemination of the software and
 * media is authorized only pursuant to a valid written
 * license from Digital Equipment Corporation.
 *
 * RESTRICTED RIGHTS LEGEND   Use, duplication, or 
 * disclosure by the U.S. Government is subject to
 * restrictions as set forth in Subparagraph (c)(1)(ii)
 * of DFARS 252.227-7013, or in FAR 52.227-19, or in
 * FAR 52.227-14 ALT.III, as applicable.
 *
 ************************************************************************/

/************************************************************************
 *   			Modification History			        *
 *
 *  07/20/87	hu	Original code.
 *  12/02/87	mcmenemy Added additional library functionality
 *  01/19/88    mcmenemy Added connection-less support
 *  02/01/88    mcmenemy Added DECnet-ULTRIX support
 *  03/29/88    mcmenemy Fixed dynamic indexing + added debug code
 *  04/18/88    mcmenemy Enhance getname and null terminate  database to 
 *			 get rid of SIGSEGV
 *  04/21/88    mcmenemy Add check_xtifd routine                        
 *  05/17/88    mcmenemy Add event peek routine
 *  06/29/88    mcmenemy Add xti_sync for xti library use
 *  08/25/88    mcmenemy Update to Final Draft for XPG 3
 *  08/29/88    mcmenemy Make corrections for modified event handling.
 *  11/22/88    mcmenemy Add additional error checking
 *  11/28/88    mcmenemy Clean-up various data stuctures and routines.
 *  12/02/88    mcmenemy Add auxillary routines for t_alloc()
 *  01/05/89    mcmenemy put null entry to end of lookup table and
 *                       update iso entry.
 *  02/10/89    mcmenemy Replace d_table.dcb references by table macro
 *  02/22/89    mcmenemy Performance changes.
 *  05/02/89    mcmenemy LINT.
 *  07/14/89    mcmenemy Fix XPG3 conformance bugs
 *  02/27/90	Ron B.   Split the "osi" "name" argument in t_open to
 *			 accommodate COTS and CLTS.
 *  11/14/90    heather  Merge changes from 2/27/90, above, and from 
 *                       DECnet into ULTRIX source pool.
 *
 ************************************************************************/

/*
 * XTI Library: xti_lib.c
 *
 * This module provides support routines used by the xti.c module to
 * provide the transport layer programming interface defined in the X/OPEN
 * Portability Guide: Networking Services. 
 */
#define XTI 1
#define table(fd) ((*d_table.dcb)[fd]) 
#include <stdio.h>
#include <errno.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/socketvar.h>

#include "xti_lib.h"


struct xti_lookup xti_tab[] = {
  { "tcp", AF_INET, SOCK_STREAM, IPPROTO_TCP,
      16, sizeof(struct tcp_options), 0, -2 ,-2 ,-2 ,T_COTS_ORD},
  { "udp", AF_INET, SOCK_DGRAM, IPPROTO_UDP,
      16, -2, 0, -2, -2, -2, T_CLTS},
#ifdef XTIOSI
  { "cots", AF_OSI, SOCK_SEQPACKET, OSIPROTO_COTS,
      sizeof(struct sockaddr_osi) + 10 + OSI_NLEN + OSI_TLEN,
      sizeof(struct isoco_options), -1, 16, 32, 64, T_COTS},
  { "clts", AF_OSI, SOCK_DGRAM, OSIPROTO_CLTS,
      sizeof(struct sockaddr_osi) + 10 + OSI_NLEN + OSI_TLEN,
      sizeof(struct isocl_options), -1, -2, -2, -2, T_CLTS},
  { "osi", AF_OSI, SOCK_SEQPACKET, OSIPROTO_COTS,    /* backward compatible */
      16, sizeof(struct isoco_options), -1, 16, 32, 64, T_COTS},
#endif
#ifdef XTINSP
  { "decnet_sequence", AF_DECnet, SOCK_SEQPACKET, 0,
      sizeof(struct sockaddr_dn), sizeof(struct accessdata_dn), -1, 16,
      sizeof(struct optdata_dn), sizeof(struct optdata_dn), T_COTS},
#endif
  { "",0,0,0,0,0,0,0,0,0,0}, /* end of table indicator */
};

#ifdef SERV
extern FILE *ftmp;
#endif


struct xti_lookup *
getname(name)
     register char *name;
{
  extern struct xti_lookup xti_tab[];
  struct xti_lookup *c;
  char *p, *q;

  for (c = xti_tab; p = c->x_name; c++) {
    if (*p == 0) {                /* end of list */
      return((struct xti_lookup *) -1);  /* not in table */
    }

    for (q = name; *q == *p++; q++) {
      if (*q == 0)		/* exact match */
	return (c);
    }
  }
  return ((struct xti_lookup *) -1);	/* not in table */
}

int
update_XTI_state(fd, resfd,  old_state)
int fd;		        /* entry in table */
int resfd;
int old_state;
{

  unsigned int i;		     /* scratch variable */
  int tmp_status;
  struct tp_states_s {
    int old_state;
    int new_state;
  } tp_states;

#ifdef INIT
  printf("\nState %-15s",xti_state_name[t_getstate(fd)]);
#endif

#ifdef SERV
  fprintf(ftmp,"\nState %-15s",xti_state_name[t_getstate(fd)]);
  fflush(ftmp);
#endif

  for (i = 0; i < ( sizeof(sean) / sizeof(struct state_table) ); i++ ) {

    if (table(fd).state == sean[i].state) {
      if (table(fd).event == sean[i].event) {

#ifdef INIT
	printf("Event %-15s Action %-08X",
	       xti_event_name[table(fd).event],
	       sean[i].ptr_to_action);
#endif

#ifdef SERV
	fprintf(ftmp,"Event %-15s Action %-08X",
	       xti_event_name[table(fd).event],
	       sean[i].ptr_to_action);
	fflush(ftmp);
#endif

	if (sean[i].ptr_to_action != NULL) {
	         /* execute action routine */
	  tmp_status = (*sean[i].ptr_to_action)(fd, resfd);
	  if (tmp_status == -1) return(-1);
	}
	table(fd).state = sean[i].next_state;
	
	tp_states.old_state = old_state;
	tp_states.new_state = sean[i].next_state;
	
	tmp_status = setsockopt(fd, SOL_SOCKET, SO_XTITPSTATE,
				(char *) &tp_states, 
				sizeof(tp_states));

	/* fix, because of kernel changing state behind our backs */

	if (fd == resfd) {
	  if (tmp_status == -1 && errno == EOPNOTSUPP) {
	    tmp_status = 0;
	  }
	}

	if (tmp_status == -1) return(-1);

#ifdef INIT
	printf("Next State %s\n",xti_state_name[table(fd).state]);
#endif

#ifdef SERV
	fprintf(ftmp,"Next State %s\n",xti_state_name[table(fd).state]);
	fflush(ftmp);
#endif
	return(0);
      }
    }
  }
return(-1);
}

int
check_XTI_state(entry, potent_event)
int entry;
int potent_event;
{

int i;		     /* scratch variable */

/*
 * Check to make sure that we are a proper service type.
 *
 */


  for (i = 0; i < ( sizeof(sean) / sizeof(struct state_table) ); i++ ) {
    if (table(entry).state == sean[i].state) {
      if (potent_event == sean[i].event) {
	return(0);
      }
    }
  }
return(-1);
}


int
action_1(entry, resfd)
int entry;
int resfd;
{
    
  /*
   * this action routine will set the count of outstanding indications to 0.
   */
    
  table(entry).cnt_outs_con_ind = 0;

  return(0);
}

int
action_2(entry, resfd)
int entry;
int resfd;
{
  
  /*
   * this action routine will increment the count of outstanding indications
   */
    
  table(entry).cnt_outs_con_ind += 1;

  return(0);
}

int
action_3(entry, resfd)
int entry;
int resfd;
{
  
  /*
   * this action routine will decrement the count of outstanding indications
   */
    
  table(entry).cnt_outs_con_ind -= 1;

  return(0);

}

int
action_4(entry, resfd)
int entry;
int resfd;
{
  
  if (resfd != -1 ) { /* we have a valid descriptor */
    table(resfd).event = XTI_PASS_CONN;

    /* should be in T_DATAXFER state (done in kernel) just in case check */

    if (t_getstate(resfd) != T_DATAXFER) 
      return(update_XTI_state(resfd, -1, t_getstate(resfd)));

  }
  
  return(0);
}

int
action_3_4(entry, resfd)
int entry;
int resfd;
{

  int tmp_status;
  (void) action_3(entry, resfd);
  tmp_status = action_4(entry, resfd);
  return(tmp_status);
}

void
map_err_to_XTI(src_err, dst_err)
     int src_err;  /* really errno */
     int *dst_err; /* really address of t_errno */
{
  
  switch(src_err) {
  case EACCES:
    *dst_err  = TACCES;
    break;
  case ENOTSOCK:
    *dst_err = TBADF;
    break;
  case EBADF:
    *dst_err = TBADF;
    break;
  case EWOULDBLOCK:
    *dst_err = TNODATA;
    break;
  case EINPROGRESS:
    *dst_err = TNODATA;
    break;
  case EOPNOTSUPP:
    *dst_err = TNOTSUPPORT;
    break;
  case EADDRNOTAVAIL:
    *dst_err = TADDRBUSY;
    break;
  case EADDRINUSE:
    *dst_err = TADDRBUSY;
    break;
  default:
    *dst_err = TSYSERR; /* have user get real error from errno */
    break;
  };
	 
};

int
check_xtifd(fd)
int fd;
{

int status;
int xti_epvalid;
int tmp_size;

tmp_size = sizeof(xti_epvalid);
status = getsockopt(fd, SOL_SOCKET, SO_XTIFDVALID, 
		    (char *) &xti_epvalid,
		    &tmp_size);

if (status < 0) return(0);

return(xti_epvalid);

}


/*
 *	XTI_PEEK - peek at the current event on a transport endpoint
 */

int
xti_peek(fd, evtinfo_ptr)
int fd;
struct xti_evtinfo *evtinfo_ptr;

{


  int status;
  int tmp_size;
  struct xti_evtinfo tmp_event;

  if (!(check_xtifd(fd))) {
    t_errno = TBADF;
    return(-1);
  }

  tmp_size = sizeof(struct xti_evtinfo);
  status = getsockopt(fd, SOL_SOCKET, SO_XTIPEEKEVENT,
		      (char *) &tmp_event,
		      &tmp_size);

  if (status < 0) {
    map_err_to_XTI(errno, &t_errno);
    return(-1);
  }

  if (evtinfo_ptr != T_NULL) *evtinfo_ptr = tmp_event;

  return(0);


}

#ifdef old

/*
 *	XTI_SYNC - synchronize transport library
 */

int 
xti_sync (fd)
int fd;
{

  int status;
  int tmp_state;
  int tmp_size;

  if (d_table.dcb == T_NULL) {
    t_errno = TBADF;
    return(-1);
  }

  status = setsockopt(fd, SOL_SOCKET, SO_XTISYNC, (char *) 0, 0);

  if (status < 0) {
    map_err_to_XTI(errno,&t_errno);		
    return (-1);
  }

  tmp_size = sizeof(tmp_state);
  status = getsockopt(fd, SOL_SOCKET, SO_XTITPSTATE,
		      (char *) &tmp_state,
		      &tmp_size);

  if (status < 0) {
    map_err_to_XTI(errno,&t_errno);		
    return (-1);
  }

  table(fd).active_flag = 1;   /* make an active descriptor */
 
  return(tmp_state);

}


pt()
{
  int i;

  if (d_table.dcb == NULL) {
    printf("No table\n");
    return (0);
  }

  for (i = 0; i < getdtablesize(); i++) {
    printf("fd=%d\n",i);
    printf("sequence = %d\n",table(i).sequence);
    printf("qlen = %d\n",table(i).qlen);
    printf("cnt_outs_con_ind = %d\n",table(i).cnt_outs_con_ind);
    printf("active_flag = %d\n",table(i).active_flag);
    printf("state = %d\n",table(i).state);
    printf("event = %d\n",table(i).event);
    printf("t_info.addr = %d\n",table(i).info.addr);
    printf("t_info.options = %d\n",table(i).info.options);
    printf("t_info.tsdu = %d\n",table(i).info.tsdu);
    printf("t_info.etsdu = %d\n",table(i).info.etsdu);
    printf("t_info.connect = %d\n",table(i).info.connect);
    printf("t_info.discon = %d\n",table(i).info.discon);
    printf("t_info.servtype = %d\n",table(i).info.servtype);
    printf("family = %d\n",table(i).family);
    printf("xti_proto = %d\n",table(i).xti_proto);
  }

  return(0);

}

pte(i)
     int i;
{
  
  if (d_table.dcb == NULL) {
    printf("No table\n");
    return (0);
  }

  printf("fd=%d\n",i);
  printf("sequence = %d\n",table(i).sequence);
  printf("qlen = %d\n",table(i).qlen);
  printf("cnt_outs_con_ind = %d\n",table(i).cnt_outs_con_ind);
  printf("active_flag = %d\n",table(i).active_flag);
  printf("state = %d\n",table(i).state);
  printf("event = %d\n",table(i).event);
  printf("t_info.addr = %d\n",table(i).info.addr);
  printf("t_info.options = %d\n",table(i).info.options);
  printf("t_info.tsdu = %d\n",table(i).info.tsdu);
  printf("t_info.etsdu = %d\n",table(i).info.etsdu);
  printf("t_info.connect = %d\n",table(i).info.connect);
  printf("t_info.discon = %d\n",table(i).info.discon);
  printf("t_info.servtype = %d\n",table(i).info.servtype);
  printf("family = %d\n",table(i).family);
  printf("xti_proto = %d\n",table(i).xti_proto);

  return(0);
  
}

#endif old

char *
  allocate_addr(struct_type, in_ptr, opt_addr, udata_addr, sizea)
int struct_type;
char *in_ptr;
char *opt_addr;
char *udata_addr;
int sizea;
{
  char *tmp;

  if ((sizea < 0) || (in_ptr == T_NULL)) {
    t_errno = TSYSERR;
    errno = EINVAL;
    if (opt_addr) free(opt_addr);
    if (udata_addr) free(udata_addr);
    return(T_NULL);
  }

  tmp = (char *)malloc( (unsigned) sizea); /* allocate memory */

  if (tmp == T_NULL) {
    if (opt_addr) free(opt_addr);
    if (udata_addr) free(udata_addr);
    return(T_NULL);
  }

  switch (struct_type) {
  case T_BIND_STR:

    ((struct t_bind *) in_ptr)->addr.buf = tmp;
    ((struct t_bind *) in_ptr)->addr.len = 0;
    ((struct t_bind *) in_ptr)->addr.maxlen = sizea;
    break;

  case T_CALL_STR:

    ((struct t_call *) in_ptr)->addr.buf = tmp;
    ((struct t_call *) in_ptr)->addr.len = 0;
    ((struct t_call *) in_ptr)->addr.maxlen = sizea;
    break;

  case T_UNITDATA_STR:

    ((struct t_unitdata *) in_ptr)->addr.buf = tmp;
    ((struct t_unitdata *) in_ptr)->addr.len = 0;
    ((struct t_unitdata *) in_ptr)->addr.maxlen = sizea;
    break;

  case T_UDERROR_STR:

    ((struct t_uderr *) in_ptr)->addr.buf = tmp;
    ((struct t_uderr *) in_ptr)->addr.len = 0;
    ((struct t_uderr *) in_ptr)->addr.maxlen = sizea;
    break;

  default:
    t_errno = TNOSTRUCTYPE;
    tmp = T_NULL;
    break;

  };

  return(tmp);
}

char *
  allocate_opt(struct_type, in_ptr, addr_addr, udata_addr, sizeo)
int struct_type;
char *in_ptr;
char *addr_addr;
char *udata_addr;
int sizeo;
{
  char *tmp;

  if ((sizeo < 0) || (in_ptr == T_NULL)) {
    t_errno = TSYSERR;
    errno = EINVAL;
    if (addr_addr) free(addr_addr);
    if (udata_addr) free(udata_addr);
    return(T_NULL);
  }

  tmp = (char *)malloc( (unsigned) sizeo); /* allocate memory */

  if (tmp == T_NULL) {
    if (addr_addr) free(addr_addr);
    if (udata_addr) free(udata_addr);
    return(T_NULL);
    }


  switch(struct_type) {

  case T_OPTMGMT_STR:

    ((struct t_optmgmt *) in_ptr)->opt.buf = tmp;
    ((struct t_optmgmt *) in_ptr)->opt.len = 0;
    ((struct t_optmgmt *) in_ptr)->opt.maxlen = sizeo;
    break;

  case T_CALL_STR:

    ((struct t_call *) in_ptr)->opt.buf = tmp;
    ((struct t_call *) in_ptr)->opt.len = 0;
    ((struct t_call *) in_ptr)->opt.maxlen = sizeo;
    break;

  case T_UNITDATA_STR:

    ((struct t_unitdata *) in_ptr)->opt.buf = tmp;
    ((struct t_unitdata *) in_ptr)->opt.len = 0;
    ((struct t_unitdata *) in_ptr)->opt.maxlen = sizeo;
    break;

  case T_UDERROR_STR:

    ((struct t_uderr *) in_ptr)->opt.buf = tmp;
    ((struct t_uderr *) in_ptr)->opt.len = 0;
    ((struct t_uderr *) in_ptr)->opt.maxlen = sizeo;
    break;

  default:
    t_errno = TNOSTRUCTYPE;
    tmp = T_NULL;
    break;

  };
  return(tmp);
}

char *
  allocate_udata(struct_type, in_ptr, opt_addr, addr_addr, sizeu)
int struct_type;
char *in_ptr;
char *opt_addr;
char *addr_addr;
int sizeu;
{
  char *tmp;

  if ((sizeu < 0) || (in_ptr == T_NULL)) {
    t_errno = TSYSERR;
    errno = EINVAL;
    if (opt_addr) free(opt_addr);
    if (addr_addr) free(addr_addr);
    return(T_NULL);
  }

  tmp = (char *)malloc( (unsigned) sizeu); /* allocate memory */

  if (tmp == T_NULL) {
    if (opt_addr) free(opt_addr);
    if (addr_addr) free(addr_addr);
    return(T_NULL);
    }

  switch (struct_type) {

    case T_CALL_STR:
    
    ((struct t_call *) in_ptr)->udata.buf = tmp;
    ((struct t_call *) in_ptr)->udata.len = 0;
    ((struct t_call *) in_ptr)->udata.maxlen = sizeu;
    break;

  case T_DIS_STR:

    ((struct t_discon *) in_ptr)->udata.buf = tmp;
    ((struct t_discon *) in_ptr)->udata.len = 0;
    ((struct t_discon *) in_ptr)->udata.maxlen = sizeu;
    break;

  case T_UNITDATA_STR:

    ((struct t_unitdata *) in_ptr)->udata.buf = tmp;
    ((struct t_unitdata *) in_ptr)->udata.len = 0;
    ((struct t_unitdata *) in_ptr)->udata.maxlen = sizeu;
    break;

  default:
    t_errno = TNOSTRUCTYPE;
    tmp = T_NULL;
    break;

  };

  return(tmp);
}

