#ifndef lint
static char *sccsid = "@(#)xti.c	4.2 (ULTRIX)	11/14/90";
#endif lint

/***********************************************************************
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
 ***********************************************************************/

/************************************************************************
 *			Modification History				*
 *
 *  07/20/87	hu	Original code.
 *  12/02/87	mcmenemy Added additional library functionality
 *  01/19/88    mcmenemy Added connection-less support
 *  02/01/88    mcmenemy clean-up - prepare to release 1st prototype 
 *                       which includes DECnet Ultrix support
 *  03/07/88    mcmenemy Update to Revision 2 (24-feb-88) at GRENOBLE
 *  03/29/88    mcmenemy Fix dynamic indexing.
 *  04/06/88    mcmenemy Make changes for BL2
 *  08/25/88    mcmenemy Update to Final Draft for XPG 3
 *  10/09/88    mcmenemy Get rid of clearing some events that will be kernel
 *  11/09/88    mcmenemy Clean-up (ie. move ENEVENT to socket call and
 *                       explicitly disable events in T_UNBIND
 *  11/26/88    mcmenemy Add *_CONOPT support,enhance T_OPTMGMT, finish T_MORE.
 *  12/01/88    mcmenemy Add t_alloc() and t_free() support.
 *  12/06/88    mcmenemy Cleanup NSP code ,add XTIXPG4 defines and OSI code.
 *  01/06/89    mcmenemy Modify t_accept handling of socket address.
 *  01/08/89    mcmenemy Add AF_OSI code for defer mode in t_listen
 *  02/02/89    mcmenemy Add code to clear T_DISCONNECT event in t_rcvdis().
 *  02/10/89    mcmenemy Clean-up d_table usage.
 *  02/20/89    mcmenemy Add additional error return from t_bind.
 *  02/21/89    mcmemeny Performance enhancements
 *  03/08/89    mcmenemy Change event handling algorithmn
 *  03/09/89    mcmenemy Add address generation to t_bind + mapping errors
 *                       returns to match verification spec.
 *  03/10/89    mcmenemy Put update check code back into t_close
 *  03/10/89    mcmenemy In t_snddis make sure call is valid before checking
 *                       call->sequence
 *  03/13/89    mcmenemy In t_snddis call XTIABORT setsockopt even in
 *                       T_DATAXFER state (instead of shutdown - which
 *                       would generate a T_ORDREL event.
 *  03/15/89    mcmenemy Change handling of error return for
 *                       TNOTSUPPORT/TBADDATA
 *  04/04/89    mcmenemy Fix t_unbind , make setsockopt call to unbind(zero)
 *                       pertinent fields without deleting control blocks.
 *			 This will allow for t_bind()-t-unbind() loops to work.
 *                       Place event enabling where endpoint becomes active
 *                       (ie. t_bind())
 *  04/05/89    mcmenemy If rcvcall is not used in t_connect, don't use it.
 *  04/24/89    mcmenemy Make t_sndrel conformance to spec.
 *  04/27/89    mcmenemy Check for NULL call parameter in t_snddis.
 *  04/27/89    mcmenemy Re-add PEEK flag in t_rcv to set/clear T_MORE flag
 *  05/02/89    mcmenemy LINT.
 *  05/10/89    mcmenemy Add additional event syncronization in t_look.
 *                       This feature may be turned off by XTI_UNEVENT.
 *  07/14/89    mcmenemy Fix XPG3 Conformance bugs
 *  09/12/89	Ron B.   Fix segmentation fault in t_rcv on MIPS.
 *  11/22/89	Ron B.   Retrieve any user data sent by the caller in the
 *			 t_listen call.
 *  02/23/90	Ron B.   Returns the responding protocol address to the user
 *			 in t_connect call.
 *  03/05/90	Ron B.	 Switch over to use new osi.h.
 *			 When checking the length of protocol address against
 *			 the maximum protocol address size in t_info table,
 *			 take "-1" and "-2" into account.
 *			 Rewrite how t_accept does the checking.
 *			 Change to use new socket option name.
 *  04/11/90	Ron B.	 Make sure that we don't linger on t_close for OSI.
 *  05/17/90	Ron B.	 Fix accept checking for TCP.
 *  08/08/90    gray     Do not set acceptmode, nor listen if CLTS in
 *                       t_bind(). Inititalise seq. no in t_listen().
 *                       Allow auto-addr generation for OSI. (courtesy
 *                       Matt Thomas).
 *  11/06/90    gray     Misc bug fixes to prevent various user areas
 *                       being overwritten. Conformance fixes to most
 *                       functions.
 *  11/14/90    heather  Merge changes, including from 11/22/89 above, 
 *                       into ULTRIX source pool.
 *
 ************************************************************************/

/*
 * XTI Library: xti.c
 *
 * This module provides the transport layer programming interface defined
 * in the X/OPEN Portability Guide: Networking Services. 
 */

/*LINTLIBRARY*/

#define XTI 1

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <malloc.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/xti.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include "xti_lib.h"

extern char *allocate_addr();
extern char *allocate_opt();
extern char *allocate_udata();
extern void bcopy();
extern void perror();
extern void map_err_to_XTI();

/*
 * macro
 */

#define table(fd) ((*d_table.dcb)[fd]) /* define macro for dynamic table */
#define MAX(x,y) ((x > y) ? x : y )


/*
 *	T_ACCEPT - accept a connect request
 */
int t_accept (fd, resfd, call)
int	fd;             /* where the connection indication arrived */
int	resfd;          /* where the connection is to be established */
struct	t_call *call;
{
#ifdef XTINSP
    struct accessdata_dn nsp_accessdata;
#endif

    int status;
    int optl;
    int old_state;
    int chklen;			/* total length of accept_check structure */
    struct xti_evtinfo evtinfo;
    struct accept_check {
	int resfd;
	int seqnum;
	union {
	    struct sockaddr generic;
#ifdef	XTIOSI
	    struct sockaddr_osi osi;
#endif	XTIOSI
	} addr;		/* sockaddr_osi must be the last one in structure */
    } *chk;

    if (!(check_xtifd(fd))) {
	t_errno = TBADF;
	return(-1);
    }

    if (!(check_xtifd(resfd))) {
	t_errno = TBADF;
	return(-1);
    }

    /*
     * Must make sure we have dynamic table built.
     */
    if (d_table.dcb == T_NULL) {
	t_errno = TBADF;
	return(-1);
    }

    old_state = table(fd).state;
    if ( (check_XTI_state(fd, XTI_ACCEPT1) == -1) &&
	 (check_XTI_state(fd, XTI_ACCEPT2) == -1) &&
	 (check_XTI_state(fd, XTI_ACCEPT3) == -1) ) {
	t_errno = TOUTSTATE;
	return(-1);
    }

    if ( (fd != resfd) && (check_XTI_state(resfd ,XTI_LISTEN) == -1) ) {
	t_errno = TOUTSTATE;
	return(-1);
    }

    if ( (fd != resfd) && (table(resfd).state != T_IDLE)) { 
	t_errno = TOUTSTATE;
	return(-1);
    }

    if (table(fd).info.servtype != T_COTS && 
	table(fd).info.servtype != T_COTS_ORD) {
	t_errno = TNOTSUPPORT;
	return(-1);
    }

    if (fd != resfd) {
	if (table(resfd).info.servtype != T_COTS && 
	    table(resfd).info.servtype != T_COTS_ORD) {
	    t_errno = TNOTSUPPORT;
	    return(-1);
	}
    }

    /*
     * do some up-front checking
     */

    if (call->sequence <= 0) {
	t_errno = TBADSEQ;
	return(-1);
    }

  /*
   * If the user has received other indications on this endpoint and has
   * not responded to them, DO NOT allow the user to accept a connection on
   * the same endpoint
   */

  if ((resfd == fd) && (table(fd).cnt_outs_con_ind > 1)) {
    t_errno = TBADF;
    return(-1);
  }

  if (table(fd).info.addr != -1 && table(fd).info.addr != -2)
    if (call->addr.len > table(fd).info.addr) {
      t_errno = TBADADDR;
      return(-1);
    }

  /*
   * Determine the actual length of the accept_check since the addr portion
   * can be variable length.
   */
  switch(table(fd).family) {
    case AF_INET:
      chklen = sizeof(struct accept_check);
      break;
#ifdef XTIOSI
    case AF_OSI:
      chklen = sizeof(struct accept_check) +
	       ((struct sockaddr_osi *)call->addr.buf)->osi_length;
      break;
#endif XTIOSI

#ifdef XTINSP
    case AF_DECnet:
      chklen = 0;		/* we don't yet support NSP */
      break;
#endif XTINSP
  }

  /*
   * Get buffer to pass information to be checked to the kernel.
   * Then, load the buffer with the information.  This buffer
   * will be passed to the kernel for actual checking.
   */
  chk = (struct accept_check *)malloc(chklen);

  chk->seqnum = call->sequence;
  chk->resfd = resfd;

  bcopy( call->addr.buf, (char *)&chk->addr.generic, (int)call->addr.len);

  /*
   * Check to see if any event pending.
   */
  status = xti_peek(fd, &evtinfo);
  if (status == -1) {
    free(chk);
    return(-1);
  }

  if (evtinfo.evtarray[ffs(T_LISTEN)] ||
      evtinfo.evtarray[ffs(T_DISCONNECT)]) {
    free(chk);
    t_errno = TLOOK;
    return(-1);
  }
  
  /*
   * Send user data (if any) 
   */
  
  if (call->udata.len > 0) {

    if (table(resfd).info.connect == T_NOTSUPPORTED) {
      t_errno = TBADDATA;
      return(-1);
    }

    if (call->udata.len > (table(resfd).info.connect)) {
      t_errno = TBADDATA;
      return(-1);
    }
      
    switch(table(fd).family) {
    
    case AF_INET:
      break;
#ifdef XTIOSI
    case AF_OSI:
      {
	char *usrdat;
	int usrdatlen = table(resfd).info.connect;

	usrdat = (char *)malloc(usrdatlen);
	if (usrdat == NULL) {
	  t_errno = TSYSERR;
	  errno = ENOBUFS;
	  return(-1);
	}
	bzero(usrdat, usrdatlen);
	bcopy(call->udata.buf, usrdat, call->udata.len);

	status = setsockopt(resfd, OSIPROTO_COTS, TOPT_OPTCONDATA, 
			    usrdat, call->udata.len);
	free(usrdat);
	if (status == -1) {
	  map_err_to_XTI(errno,&t_errno);
	  return(-1);
	}
	break;
      }
#endif
#ifdef XTINSP
    case AF_DECnet:

      
      optl = call->udata.len;
      status = setsockopt(resfd, DNPROTO_NSP, DSO_CONDATA, 
			  call->udata.buf,
			  optl );

      if (status == -1) {
	map_err_to_XTI(errno,&t_errno);
	return(-1);
      }
      break;
#endif
    };
  }


  /*
   * Send option data (if any) 
   */

  if (call->opt.len > 0) {
    
    if (table(fd).info.options == T_NOTSUPPORTED) {
      t_errno = TNOTSUPPORT;
      return(-1);
    }

    if (call->opt.len > table(fd).info.options) {
      t_errno = TBADOPT;
      return(-1);
    }

    switch(table(fd).family) {
#ifdef XTIOSI
    case AF_OSI:
      { 
	int tmp_len;

	tmp_len = call->opt.len;
	status = setsockopt(resfd, OSIPROTO_COTS, TOPT_XTICONOPTS,
			    call->opt.buf,
			    tmp_len );
	  
	if (status == -1) {
	  map_err_to_XTI(errno,&t_errno);
	  if (t_errno == TNOTSUPPORT) t_errno = TBADOPT; /* re-map */
	  return(-1);
	}
      }
      break;
#endif
    case AF_INET:
      if (table(fd).xti_proto == IPPROTO_TCP) {
	int tmp_len;

	tmp_len = call->opt.len;
	status = setsockopt(resfd, IPPROTO_TCP, TCP_CONOPT,
			    call->opt.buf,
			    tmp_len );

	  
	if (status == -1) {
	  map_err_to_XTI(errno,&t_errno);
	  if (t_errno == TNOTSUPPORT) t_errno = TBADOPT; /* re-map */
	  return(-1);
	}
      }

      break;

#ifdef XTINSP
    case AF_DECnet:
      
      bcopy(call->opt.buf,(char *) &nsp_accessdata, call->opt.len);
      optl = sizeof(struct accessdata_dn);
      status = setsockopt(resfd, DNPROTO_NSP, DSO_CONACCESS, 
			  (char *) &nsp_accessdata, 
			  optl );
      
      if (status == -1) {
	map_err_to_XTI(errno,&t_errno);
	return(-1);
      }
      break;
#endif

    };
  }


  /*
   * deferred accept
   */
  status = setsockopt(fd, SOL_SOCKET, SO_XTIACCEPTCHK, (char *)chk, chklen);
  if (status < 0) {
    if (errno == EINVAL) { /* special case */
      free(chk);
      t_errno = TBADSEQ;
      return(-1);
    }

    if (errno == EADDRNOTAVAIL ||
	errno == EPROTONOSUPPORT) { /* special case */
      free(chk);
      t_errno = TBADADDR;
      return(-1);
    }

    map_err_to_XTI(errno,&t_errno);
    free(chk);
    return(-1);
  }
  free(chk);

  status = setsockopt(resfd, SOL_SOCKET, SO_XTISYNC, (char *) 0, 0);

  if (status < 0) {
    map_err_to_XTI(errno,&t_errno);
    return(-1);
  }

  /*
   * Accept the connection
   */

  switch(table(fd).family) {
#ifdef XTIOSI
  case AF_OSI:

    status = setsockopt(resfd, OSIPROTO_COTS, TOPT_ACCEPT, (char *)0, 0);
    if (status == -1) {
      map_err_to_XTI(errno,&t_errno);
      return(-1);
    }

    break;
#endif
  case AF_INET:
    switch(table(fd).xti_proto) {
    
    case IPPROTO_TCP:
    
      status = setsockopt(resfd, IPPROTO_TCP, TCP_CONACCEPT, (char *) 0,0);
    
      if (status < 0) {
	t_errno = TSYSERR;
	return(-1);
      }
      break;

    default:
      break;
    };
    break;

#ifdef XTINSP
  case AF_DECnet:

    status = setsockopt(resfd, DNPROTO_NSP, DSO_CONACCEPT, (char *) 0, 0);

    if (status == -1) {
      map_err_to_XTI(errno,&t_errno);
      return(-1);
    }

    break;
#endif
  default:
    break;
    };

  /*
   * re-load new states.
   */

  if (resfd != fd) {
    old_state = t_getstate(fd); /* re-get new state */
    (void) t_getstate(resfd);
  }


	    
  /*
   *	Update XTI state tables
   *
   */

  if ( (table(fd).cnt_outs_con_ind == 1) && (fd == resfd)) {
    table(fd).event = XTI_ACCEPT1;   /* T_ACCEPT successful */
    if (update_XTI_state(fd, resfd, old_state) == -1) {
      t_errno = TOUTSTATE;
      return(-1);
    }
  }
  else 
    if ( (table(fd).cnt_outs_con_ind == 1) && (fd != resfd)) {
      if (table(resfd).active_flag != 1) { 
	t_errno = TOUTSTATE;
	return(-1);
      }

	

      /*
       *	Update XTI state tables
       *
       */

      table(fd).event = XTI_ACCEPT2;   /* T_ACCEPT successful */

      if (update_XTI_state(fd, resfd, old_state) == -1) {
	t_errno = TOUTSTATE;
	return(-1);
      }
    }
    else 

      /*
       *	Update XTI state tables
       *
       */

      /*
       * This case impiles that fd != resfd
       */

      if (table(fd).cnt_outs_con_ind > 1) {
	
	table(fd).event = XTI_ACCEPT3;   /* T_ACCEPT successful */

	if (update_XTI_state(fd, resfd, old_state) == -1) {
	  t_errno = TOUTSTATE;
	  return(-1);
	}
      }
  return (0);
}

/*
 * 	T_ALLOC - allocate a library structure (optional)
 */

char 
  *t_alloc (fd, struct_type, fields)
int fd;
int struct_type;
int fields;
{
  char *tmp_ptr = T_NULL;
  char *tmp_addr = T_NULL;
  char *tmp_opt = T_NULL;
  char *tmp_udata = T_NULL;
  int size;
  
  if (!(check_xtifd(fd))) {
    t_errno = TBADF;
    return(T_NULL);
  }

  switch(struct_type) {
    
  case T_BIND_STR:

    tmp_ptr = (char *)malloc(sizeof(struct t_bind));
    
    if (tmp_ptr == T_NULL) {
      t_errno = TSYSERR;
      errno = ENOBUFS;
      return(T_NULL);
    }

    if (fields & T_ADDR) { /* ADDRESS */
      if (table(fd).info.addr == -1) {		/* can't allocate infinity */
	free(tmp_ptr);
	t_errno = TSYSERR;
	errno = EINVAL;
	return(T_NULL);
      }
      tmp_addr = allocate_addr(struct_type, tmp_ptr, tmp_opt, tmp_udata,
			       (int) table(fd).info.addr);
      if (tmp_addr == T_NULL) {
	free(tmp_ptr);
	t_errno = TSYSERR;
	errno = ENOBUFS;
	return(T_NULL);
      }
    }
    else {
      ( (struct t_bind *) tmp_ptr)->addr.maxlen = T_NULL;
      ( (struct t_bind *) tmp_ptr)->addr.len = T_NULL;
      ( (struct t_bind *) tmp_ptr)->addr.buf = T_NULL;
    }

    break;

  case T_OPTMGMT_STR:

    tmp_ptr = (char *)malloc(sizeof(struct t_optmgmt));

    if (tmp_ptr == T_NULL) {
      t_errno = TSYSERR;
      errno = ENOBUFS;
      return(T_NULL);
    }

    if (fields & T_OPT) { /* OPTION */
      tmp_opt = allocate_opt(struct_type, tmp_ptr, tmp_addr, tmp_udata,
			      (int) table(fd).info.options);
      if (tmp_opt == T_NULL) {
	free(tmp_ptr);
	t_errno = TSYSERR;
	errno = ENOBUFS;
	return(T_NULL);
      }
    }
    else {
      ( (struct t_optmgmt *) tmp_ptr)->opt.maxlen = T_NULL;
      ( (struct t_optmgmt *) tmp_ptr)->opt.len = T_NULL;
      ( (struct t_optmgmt *) tmp_ptr)->opt.buf = T_NULL;
    }

    break;

  case T_CALL_STR:
    tmp_ptr = (char *)malloc(sizeof(struct t_call));

    if (tmp_ptr == T_NULL) {
      t_errno = TSYSERR;
      errno = ENOBUFS;
      return(T_NULL);
    }

    if (fields & T_ADDR) { /* ADDRESS */
      if (table(fd).info.addr == -1) {		/* can't allocate infinity */
	free(tmp_ptr);
	t_errno = TSYSERR;
	errno = EINVAL;
	return(T_NULL);
      }
      tmp_addr = allocate_addr(struct_type, tmp_ptr, tmp_opt, tmp_udata,
			       (int) table(fd).info.addr);
      
      if (tmp_addr == T_NULL) {
	free(tmp_ptr);
	t_errno = TSYSERR;
	errno = ENOBUFS;
	return(T_NULL);
      }
    }
    else {
      ( (struct t_call *) tmp_ptr)->addr.maxlen = T_NULL;
      ( (struct t_call *) tmp_ptr)->addr.len = T_NULL;
      ( (struct t_call *) tmp_ptr)->addr.buf = T_NULL;
    }

    if (fields & T_OPT) { /* OPTION */
      tmp_opt = allocate_opt(struct_type, tmp_ptr, tmp_addr, tmp_udata,
			       (int) table(fd).info.options);
      
      if (tmp_opt == T_NULL) {
	free(tmp_ptr);
	t_errno = TSYSERR;
	errno = ENOBUFS;
	return(T_NULL);
      }
    }
    else {
      ( (struct t_call *) tmp_ptr)->opt.maxlen = T_NULL;
      ( (struct t_call *) tmp_ptr)->opt.len = T_NULL;
      ( (struct t_call *) tmp_ptr)->opt.buf = T_NULL;
    }

    if (fields & T_UDATA) { /* UDATA */
      size = MAX(table(fd).info.connect, 
		 table(fd).info.discon);

      if (size == -2) {
        free(tmp_ptr);
        t_errno = TSYSERR;
        errno = EINVAL;
        return(T_NULL);
      }

      tmp_udata = allocate_udata(struct_type, tmp_ptr, tmp_opt, tmp_addr,
				 size);
      
      if (tmp_udata == T_NULL) {
	free(tmp_ptr);
	t_errno = TSYSERR;
	errno = ENOBUFS;
	return(T_NULL);
      }
    }
    else {
      ( (struct t_call *) tmp_ptr)->udata.maxlen = T_NULL;
      ( (struct t_call *) tmp_ptr)->udata.len = T_NULL;
      ( (struct t_call *) tmp_ptr)->udata.buf = T_NULL;
    }

    break;

  case T_DIS_STR:
    tmp_ptr = (char *)malloc(sizeof(struct t_discon));

    if (tmp_ptr == T_NULL) {
      t_errno = TSYSERR;
      errno = ENOBUFS;
      return(T_NULL);
    }

    if (fields & T_UDATA) { /* UDATA */
      if (table(fd).info.discon == -2) {
	free(tmp_ptr);
	t_errno = TSYSERR;
	errno = EINVAL;
	return(T_NULL);
      }
      tmp_udata = allocate_udata(struct_type, tmp_ptr, tmp_opt, tmp_addr,
			       (int) table(fd).info.discon);
      
      if (tmp_udata == T_NULL) {
	free(tmp_ptr);
	t_errno = TSYSERR;
	errno = ENOBUFS;
	return(T_NULL);
      }
    }
    else {
      ( (struct t_discon *) tmp_ptr)->udata.maxlen = T_NULL;
      ( (struct t_discon *) tmp_ptr)->udata.len = T_NULL;
      ( (struct t_discon *) tmp_ptr)->udata.buf = T_NULL;
    }

    break;

  case T_UNITDATA_STR:
    tmp_ptr = (char *)malloc(sizeof(struct t_unitdata));

    if (tmp_ptr == T_NULL) {
      t_errno = TSYSERR;
      errno = ENOBUFS;
      return(T_NULL);
    }

    if (fields & T_ADDR) { /* ADDRESS */
      if (table(fd).info.addr == -1) {		/* can't allocate infinity */
	free(tmp_ptr);
	t_errno = TSYSERR;
	errno = EINVAL;
	return(T_NULL);
      }
      tmp_addr = allocate_addr(struct_type, tmp_ptr, tmp_opt, tmp_udata,
			      (int) table(fd).info.addr);
      if (tmp_addr == T_NULL) {
	free(tmp_ptr);
	t_errno = TSYSERR;
	errno = ENOBUFS;
	return(T_NULL);
      }
    }
    else {
      ( (struct t_unitdata *) tmp_ptr)->addr.maxlen = T_NULL;
      ( (struct t_unitdata *) tmp_ptr)->addr.len = T_NULL;
      ( (struct t_unitdata *) tmp_ptr)->addr.buf = T_NULL;
    }

    if (fields & T_OPT) { /* OPTION */
      tmp_opt = allocate_opt(struct_type, tmp_ptr, tmp_addr, tmp_udata,
			      (int) table(fd).info.options);
      if (tmp_opt == T_NULL) {
	free(tmp_ptr);
	t_errno = TSYSERR;
	errno = ENOBUFS;
	return(T_NULL);
      }
    }
    else {
      ( (struct t_unitdata *) tmp_ptr)->opt.maxlen = T_NULL;
      ( (struct t_unitdata *) tmp_ptr)->opt.len = T_NULL;
      ( (struct t_unitdata *) tmp_ptr)->opt.buf = T_NULL;
    }

    if (fields & T_UDATA) { /* UDATA */
      if (table(fd).info.tsdu == -1) {		/* can't allocate infinity */
	free(tmp_ptr);
	t_errno = TSYSERR;
	errno = EINVAL;
	return(T_NULL);
      }
      tmp_udata = allocate_udata(struct_type, tmp_ptr, tmp_opt, tmp_addr,
			       (int) table(fd).info.tsdu);
      if (tmp_udata == T_NULL) {
	free(tmp_ptr);
	t_errno = TSYSERR;
	errno = ENOBUFS;
	return(T_NULL);
      }
    }
    else {
      ( (struct t_unitdata *) tmp_ptr)->udata.maxlen = T_NULL;
      ( (struct t_unitdata *) tmp_ptr)->udata.len = T_NULL;
      ( (struct t_unitdata *) tmp_ptr)->udata.buf = T_NULL;
    }

    break;
  case T_UDERROR_STR:
    tmp_ptr = (char *)malloc(sizeof(struct t_uderr));

    if (tmp_ptr == T_NULL) {
      t_errno = TSYSERR;
      errno = ENOBUFS;
      return(T_NULL);
    }

    if (fields & T_ADDR) { /* ADDRESS */
      if (table(fd).info.addr == -1) {		/* can't allocate infinity */
	free(tmp_ptr);
	t_errno = TSYSERR;
	errno = EINVAL;
	return(T_NULL);
      }
      tmp_addr = allocate_addr(struct_type, tmp_ptr, tmp_opt, tmp_udata,
			       (int) table(fd).info.addr);
      if (tmp_addr == T_NULL) {
	free(tmp_ptr);
	t_errno = TSYSERR;
	errno = ENOBUFS;
	return(T_NULL);
      }
    }
    else {
      ( (struct t_uderr *) tmp_ptr)->addr.maxlen = T_NULL;
      ( (struct t_uderr *) tmp_ptr)->addr.len = T_NULL;
      ( (struct t_uderr *) tmp_ptr)->addr.buf = T_NULL;
    }

    if (fields & T_OPT) { /* OPTION */
      tmp_opt = allocate_opt(struct_type, tmp_ptr, tmp_addr, tmp_udata,
			       (int) table(fd).info.options);
      if (tmp_opt == T_NULL) {
	free(tmp_ptr);
	return(T_NULL);
      }
    }
    else {
      ( (struct t_uderr *) tmp_ptr)->opt.maxlen = T_NULL;
      ( (struct t_uderr *) tmp_ptr)->opt.len = T_NULL;
      ( (struct t_uderr *) tmp_ptr)->opt.buf = T_NULL;
    }

    break;
  case T_INFO_STR:
    tmp_ptr = (char *)malloc(sizeof(struct t_info));

    if (tmp_ptr == T_NULL) {
      t_errno = TSYSERR;
      errno = ENOBUFS;
      return(T_NULL);
    }

    break;
  default:
    t_errno = TNOSTRUCTYPE;
    return(T_NULL);
    break;
  };

  return(tmp_ptr);
}

/*
 *	T_BIND - bind an address to a transport endpoint
 */

int
t_bind(fd, req, ret)
int fd;
struct t_bind *req;
struct t_bind *ret;
{
  int old_state;
  struct sockaddr_in *tmp_inet_addr; /* ptr to internet address for address
				      * generation */

  int status;
  int xti_evtenable;
  struct t_bind fake_req;
  char acceptmode;
  int acceptmodel;
  int optl;
  int buffer_overflow = 0;

  if (!(check_xtifd(fd))) {
    t_errno = TBADF;
    return(-1);
  }

  if (d_table.dcb == T_NULL) { /* must make sure we have dynamic table built */
    t_errno = TBADF;
    return(-1);
  }

  old_state = table(fd).state;

  if (check_XTI_state(fd, XTI_BIND) == -1) {
    t_errno = TOUTSTATE;
    return(-1);
  }

  if (table(fd).info.servtype != T_COTS && 
      table(fd).info.servtype != T_COTS_ORD &&
      table(fd).info.servtype != T_CLTS) {
    t_errno = TNOTSUPPORT;
    return(-1);
  }

  /* do this here instead of t_open - because of t_bind()-t_unbind() */

  xti_evtenable = 1;   /* endpoint has events enabled */

  status = setsockopt(fd, SOL_SOCKET, SO_XTIENEVENT,
		      (char *) &xti_evtenable,
		      sizeof(xti_evtenable));
      
  if (status < 0) {
    map_err_to_XTI(errno, &t_errno);
    return(-1);
  }

retest:
  if (req != T_NULL && req->addr.len != T_NULL && req->addr.buf != T_NULL) {

    if (table(fd).info.addr != -1 && table(fd).info.addr != -2)
      if (req->addr.len > table(fd).info.addr) {
        t_errno = TBADADDR;
        return(-1);
      }

    /*
     *
     * Update internal XTI data structure
     */
  
    table(fd).qlen = ( (SOMAXCONN < req->qlen) ? SOMAXCONN : req->qlen);

    if ((bind (fd, (struct sockaddr *) req->addr.buf,
	       (int) req->addr.len)) < 0) {
      if (errno == EADDRNOTAVAIL) {
	req = T_NULL; /* force automatic address generation */
	goto retest;
      }
      if (errno == EINVAL) { /* re-map (ie) short address */
	t_errno = TBADADDR;
	return(-1);
      }
      map_err_to_XTI(errno,&t_errno);
      return (-1);
    } 
  } 
  else { /* Automatic address generation */

    struct t_bind tmp_bind, *tmp;
    
    tmp = &tmp_bind;
    req = tmp;
    
    if (table(fd).info.addr == -1) {		/* can't allocate infinity */
      t_errno = TNOADDR;
      return(-1);
    }

    req->addr.len = table(fd).info.addr;
    req->addr.buf = (char *)malloc(req->addr.len);
    req->addr.maxlen = 0;

    table(fd).qlen = 0;
    req->qlen = 0;

    switch (table(fd).family) {
#ifdef XTIOSI
    case AF_OSI:
      bzero(req->addr.buf, req->addr.len);
      ((struct sockaddr_osi *) req->addr.buf)->osi_family = AF_OSI;
      if (table(fd).info.servtype == T_CLTS)
        ((struct sockaddr_osi *) req->addr.buf)->osi_proto = OSIPROTO_CLTS;
      else
        ((struct sockaddr_osi *) req->addr.buf)->osi_proto = OSIPROTO_COTS;
      req->addr.len = sizeof(struct sockaddr_osi);
      break;
#endif      
    case AF_INET:
      bzero(req->addr.buf, req->addr.len);
      tmp_inet_addr = (struct sockaddr_in *) req->addr.buf;
      tmp_inet_addr->sin_family = AF_INET;
      tmp_inet_addr->sin_port = 0;
      tmp_inet_addr->sin_addr.s_addr = INADDR_ANY;
      break;
#ifdef XTINSP
    case AF_DECnet:
      free(req->addr.buf);
      t_errno = TNOADDR;
      return(-1);
      break;
#endif
    };
    
    if ((bind (fd, (struct sockaddr *) req->addr.buf,
	       (int) req->addr.len)) < 0) {
      if (errno == EINVAL) { /* re-map (ie) short address */
	t_errno = TBADADDR;
	return(-1);
      }
      map_err_to_XTI(errno,&t_errno);
      return (-1);
    }
    free(req->addr.buf); /* free up tmp space */    
    fake_req = *req; /* copy req in case ret != T_NULL */

  } /* end of Automatic address generation */



  if (ret != T_NULL) {
    int tmp_len;

    if (req == T_NULL) req = &fake_req;

    if (req->addr.len > ret->addr.maxlen) {
      buffer_overflow = 1;
    }

    /*
     * go to kernel to get sockname
     */
    
    if (!buffer_overflow) {
      tmp_len = ret->addr.maxlen;
      status = getsockname(fd, ret->addr.buf, &tmp_len);
  
      if (status == -1) {
        map_err_to_XTI(errno,&t_errno);
        return(-1);
      }
      ret->addr.len = tmp_len;
    } /* if not overflow */
    
    /*
     * Return actual qlen
     */
    
    ret->qlen = table(fd).qlen;

  } /* if ret != NULL */

  /*
   * code for listen - this is needed because we have to be
   * able to enqueue connections after this call.
   */

  if (table(fd).info.servtype == T_CLTS)
    table(fd).qlen = 0;

  if (table(fd).qlen > 0) { /* we must do a listen here */
    switch(table(fd).family) {
#ifdef XTINSP
    case AF_DECnet:

      acceptmode = ACC_DEFER;
      optl = 1;
      status = setsockopt(fd, DNPROTO_NSP, DSO_ACCEPTMODE,
			  (char *) &acceptmode,
			  optl );
      if (status == -1) {
	map_err_to_XTI(errno,&t_errno);
	return(-1);
      }
      break;
#endif
    case AF_INET:

      acceptmodel = ACC_DEFER;
      optl = sizeof(acceptmodel);
      status = setsockopt(fd, IPPROTO_TCP, TCP_ACCEPTMODE, 
			  (char *) &acceptmodel,
			  optl );
      if (status == -1) {
	map_err_to_XTI(errno,&t_errno);
	return(-1);
      }
      break;

#ifdef XTIOSI
    case AF_OSI:

      acceptmode = ACCEPTMODE_DEFERED;
      optl = 1;
      status = setsockopt(fd, OSIPROTO_COTS, TOPT_ACCEPTMODE,
			  (char *) &acceptmode,
			  optl );
      if (status == -1) {
	map_err_to_XTI(errno,&t_errno);
	return(-1);
      }
      break;
#endif
    default:
      break;
    };

    status = listen(fd, table(fd).qlen);
    if (status == -1) {
      map_err_to_XTI(errno,&t_errno);
      return(-1);
    }
  }
  /*
   *	Update XTI state tables
   *
   */


  table(fd).event = XTI_BIND;	   /* T_BIND successful */
  
  if (update_XTI_state(fd, -1, old_state) == -1) {
    t_errno = TOUTSTATE;
    return(-1);
  }

  if (buffer_overflow) {
    t_errno = TBUFOVFLW;
    return(-1);
  }

  return (0);
}


/*
 *	T_CLOSE - close a transport endpoint
 */

int 
t_close(fd)
int fd;
{

  int status;
  struct linger ling_value;
  int old_state;

  if (!(check_xtifd(fd))) {
    t_errno = TBADF;
    return(-1);
  }

  if (d_table.dcb == T_NULL) { /* must make sure we have dynamic table built */
    t_errno = TBADF;
    return(-1);
  }

  old_state = table(fd).state;

  if (table(fd).info.servtype != T_COTS && 
      table(fd).info.servtype != T_COTS_ORD &&
      table(fd).info.servtype != T_CLTS) {
    t_errno = TNOTSUPPORT;
    return(-1);
  }

  switch(table(fd).family) {
#ifdef XTINSP
  case AF_DECnet:
    /*
     * Make sure we disable linger
     */
    ling_value.l_onoff = 0;
    ling_value.l_linger = 1;
    status = setsockopt(fd, SOL_SOCKET, SO_LINGER,
			(char *) &ling_value,
			sizeof(ling_value));
    if (status == -1) {
      map_err_to_XTI(errno,&t_errno);
      return(-1);
    }
    break;
#endif

  case AF_INET:
    ling_value.l_onoff = 1;
    ling_value.l_linger = 0;
    status = setsockopt(fd, SOL_SOCKET, SO_LINGER,
		        (char *) &ling_value,
		        sizeof(ling_value));

    if (status == -1) {
      map_err_to_XTI(errno,&t_errno);
      return(-1);
    }
    break;
  }

  /*
   *	Update XTI state tables
   *  
   * Update internal tables + kernel before blowing
   * away endpoint
   *
   */
  
  table(fd).event = XTI_CLOSED;
  if (update_XTI_state(fd, -1, old_state) == -1) {
    t_errno = TOUTSTATE;
    return(-1);
  }
  
  table(fd).active_flag = -1; /* no longer valid */
  table(fd).sequence = 0; 
  table(fd).qlen = 0;
  
  if (close(fd) < 0) {
    map_err_to_XTI(errno,&t_errno);      
    return (-1);
  }
  
  return (0);
}


/*
 *	T_CONNECT - establish a connection with another transport endpoint
 */

int t_connect (fd, sndcall, rcvcall)
     int fd;
     struct t_call *sndcall;
     struct t_call *rcvcall;
{

#ifdef XTINSP
  struct accessdata_dn nsp_accessdata;
#endif

  int status;
  int optl;
  int old_state;
  struct xti_evtinfo evtinfo;
  int buffer_overflow = 0;
  
  if (!(check_xtifd(fd))) {
    t_errno = TBADF;
    return(-1);
  }

  if (d_table.dcb == T_NULL) { /* must make sure we have dynamic table built */
    t_errno = TBADF;
    return(-1);
  }

  old_state = table(fd).state;
  
  if ( (check_XTI_state(fd, XTI_CONNECT1) == -1) &&
      (check_XTI_state(fd, XTI_CONNECT2) == -1) ) {
    t_errno = TOUTSTATE;
    return(-1);
  }
  
  if (table(fd).info.servtype != T_COTS && 
      table(fd).info.servtype != T_COTS_ORD) {
    t_errno = TNOTSUPPORT;
    return(-1);
  }

  /*
   * check for any incoming T_DISCONNECT's or T_LISTEN's
   */
  
  status = xti_peek(fd, &evtinfo);
  
  if (status == -1) return(-1);

  if (evtinfo.evtarray[ffs(T_LISTEN)] ||
      evtinfo.evtarray[ffs(T_DISCONNECT)]) {
    t_errno = TLOOK;
    return(-1);
  }

  /* check address */

  if (table(fd).info.addr != -1 && table(fd).info.addr != -2)
    if (sndcall->addr.len > table(fd).info.addr) {
      t_errno = TBADADDR;
      return(-1);
    }

  /*
   * Send USER DATA
   */

  if (sndcall->udata.len > 0) {

    if (table(fd).info.connect == T_NOTSUPPORTED) {
      t_errno = TBADDATA;
      return(-1);
    }

    if (sndcall->udata.len > table(fd).info.connect) {
      t_errno = TBADDATA;
      return(-1);
    }
    switch (table(fd).family) {
#ifdef XTIOSI
    case AF_OSI:
      {
	char *usrdat;
	int usrdatlen = table(fd).info.connect;

	usrdat = (char *)malloc(usrdatlen);
	if (usrdat == NULL) {
	  t_errno = TSYSERR;
	  errno = ENOBUFS;
	  return(-1);
	}
	bzero(usrdat, usrdatlen);
	bcopy(sndcall->udata.buf, usrdat, sndcall->udata.len);

	status = setsockopt(fd, OSIPROTO_COTS, TOPT_OPTCONDATA, 
			    usrdat, sndcall->udata.len);
	free(usrdat);
	if (status == -1) {
	  map_err_to_XTI(errno,&t_errno);
	  return(-1);
	}
	break;
      }
#endif
    case AF_INET:
      break;
#ifdef XTINSP
    case AF_DECnet:

      status = setsockopt(fd, DNPROTO_NSP, DSO_CONDATA, 
			  sndcall->udata.buf,
			  sndcall->udata.len );
      
      if (status == -1) {
	map_err_to_XTI(errno,&t_errno);
	return(-1);
      }
      break;
#endif
    }; /* end switch */
  }  /* end user data */

  /*
   * Send option data (if any) 
   */

  if (sndcall->opt.len > 0) {

    if (table(fd).info.options == T_NOTSUPPORTED) {
      t_errno = TNOTSUPPORT;
      return(-1);
    }

    if (sndcall->opt.len > table(fd).info.options) {
      t_errno = TBADOPT;
      return(-1);
    }

    switch(table(fd).family) {
#ifdef XTIOSI
    case AF_OSI:
      {
	int tmp_len;

	tmp_len = sndcall->opt.len;
	status = setsockopt(fd, OSIPROTO_COTS, TOPT_XTICONOPTS,
			    sndcall->opt.buf,
			    tmp_len );
	if (status == -1) {
	  map_err_to_XTI(errno,&t_errno);
	  if (t_errno == TNOTSUPPORT) t_errno = TBADOPT; /* re-map */
	  return(-1);
	}
      }
      break;
#endif
    case AF_INET:
      if (table(fd).xti_proto == IPPROTO_TCP) {
	int tmp_len;

	tmp_len = sndcall->opt.len;
	status = setsockopt(fd, IPPROTO_TCP, TCP_CONOPT,
			    sndcall->opt.buf,
			    tmp_len );

#ifdef XTISECURE
#define SECURE_OPT_SIZE_ALIGN 12
	{
	  char *tmp;
	  char *tmp1;
	  struct tcp_options tmp_tcpopt;
	  short security;
	  short compartment;
	  short handling;
	  long tcc;
	  int tmp_stat;

	  bcopy(sndcall->opt.buf, &tmp_tcpopt, 
		sizeof(struct tcp_options));

	  security = htons(tmp_tcpopt.secopt.security);
	  compartment = htons(tmp_tcpopt.secopt.compartment);
	  handling = htons(tmp_tcpopt.secopt.handling);
	  tcc = htonl(tmp_tcpopt.secopt.tcc);
	  
	  if (!(security == T_UNUSED &&
	      compartment == T_UNUSED &&
	      handling == T_UNUSED &&
	      tcc == T_UNUSED)) {

	    tmp = (char *)malloc(SECURE_OPT_SIZE_ALIGN); 
	    tmp1 = tmp;

	    *tmp++ = 130; /* SECURITY */
	    *tmp++ = SECURE_OPT_SIZE_ALIGN -1;
	  
	    bcopy(&security, tmp, sizeof(short));
	    tmp += sizeof(short);
	    bcopy(&compartment, tmp, sizeof(short));
	    tmp += sizeof(short);
	    bcopy(&handling, tmp, sizeof(short));
	    tmp += sizeof(short);
	    bcopy(&tcc, tmp, sizeof(long));
	    tmp += sizeof(long) - 1;
	    *tmp = 0;

	    tmp_stat = setsockopt(fd, IPPROTO_IP, IP_OPTIONS, 
				  tmp1, SECURE_OPT_SIZE_ALIGN);
      
	    if (tmp_stat == -1) {
	      map_err_to_XTI(errno,&t_errno);
	      return(-1);
	    }

	    /* 
	   * force success for CONOPT because security options 
	   * will cause error 
	   */

	  status = 0; 

	  }
	}
#endif


	if (status == -1) {
	  map_err_to_XTI(errno,&t_errno);
	  if (t_errno == TNOTSUPPORT) t_errno = TBADOPT; /* re-map */
	  return(-1);
	}
      }

      break;
#ifdef XTINSP
    case AF_DECnet:
      
      bcopy(sndcall->opt.buf,(char *) &nsp_accessdata, sndcall->opt.len);
      optl = sizeof(struct accessdata_dn);
      status = setsockopt(fd, DNPROTO_NSP, DSO_CONACCESS, 
			  (char *) &nsp_accessdata, 
			  optl );
      
      if (status == -1) {
	map_err_to_XTI(errno,&t_errno);
	return(-1);
      }
      break;
#endif
    };
  }

 /*
  * Initialize rcvcall if specified
  */
  if (rcvcall) {
    rcvcall->addr.len = 0;
    rcvcall->opt.len = 0;
    rcvcall->udata.len = 0;
  }

  if ((connect(fd, (struct sockaddr *) sndcall->addr.buf,
	       (int) sndcall->addr.len)) == -1) {
    map_err_to_XTI(errno,&t_errno);
    
    if (t_errno == TNODATA) {
      if (rcvcall) {
	rcvcall->addr.len = sndcall->addr.len;	
	rcvcall->addr.buf = sndcall->addr.buf;
	rcvcall->opt.buf = 0;
	rcvcall->udata.buf = 0;
      }

      /*
       *	Update XTI state tables
       *
       */
      
      table(fd).event = XTI_CONNECT2;

      if (update_XTI_state(fd, -1, old_state) == -1) {
	t_errno = TOUTSTATE;
	return(-1);
      }
      return (-1);
    }
      
    /*
     * Check for user (disconnect) data
     */
    
    if (rcvcall)
      if (rcvcall->udata.maxlen > 0) {
	if (table(fd).info.discon == T_NOTSUPPORTED) {
	  t_errno = TNOTSUPPORT;
	  return(-1);
	}
	
	if (rcvcall)
	  switch (table(fd).family) {
#ifdef XTIOSI
	  case AF_OSI:
	    optl = rcvcall->udata.maxlen;
	    status = getsockopt(fd, OSIPROTO_COTS, TOPT_OPTDISDATA, 
				(char *) rcvcall->udata.buf, 
				&optl );
	    
	    if (status == -1) {
	      map_err_to_XTI(errno,&t_errno);
	      return(-1);
	    }
	    
	    rcvcall->udata.len = optl;
	    break;
#endif	    
	  case AF_INET:
	    break;
#ifdef XTINSP
	  case AF_DECnet:
	
	    optl = rcvcall->udata.maxlen;
	    status = getsockopt(fd, DNPROTO_NSP, DSO_DISDATA, 
				rcvcall->udata.buf, 
				&optl );
	    
	    if (status == -1) {
	      map_err_to_XTI(errno,&t_errno);
	      return(-1);
	    }
	    
	    rcvcall->udata.len = optl;
	    break;
#endif	
	  };
      }
    return (-1);
  } /* end of connect error */
  else {
    if (rcvcall) {
      if (rcvcall->addr.maxlen >= sndcall->addr.len) {
        rcvcall->addr.len = sndcall->addr.len;
        bcopy(sndcall->addr.buf, rcvcall->addr.buf,
              rcvcall->addr.maxlen);
      }	else {
        buffer_overflow = 1;
      }

      if (rcvcall->udata.maxlen > 0) {
	switch(table(fd).family) {
#ifdef XTIOSI
	case AF_OSI:
	  {
            char *usrdat;
	    int usrdatlen = table(fd).info.connect;

	    usrdat = (char *)malloc(usrdatlen);
	    if (usrdat == NULL) {
	      t_errno = TSYSERR;
	      errno = ENOBUFS;
	      return(-1);
	    }
	    bzero(usrdat, usrdatlen);

	    status = getsockopt(fd, OSIPROTO_COTS, TOPT_OPTCONDATA,
				usrdat, &usrdatlen);
	    if (status == -1) {
	      free(usrdat);
	      map_err_to_XTI(errno,&t_errno);
	      return(-1);
	    }

	    if (rcvcall->udata.maxlen < usrdatlen) {
	      buffer_overflow = 1;
	    }

	    if (!buffer_overflow) {
              bcopy(usrdat, rcvcall->udata.buf, usrdatlen);
	      rcvcall->udata.len = usrdatlen;
            }
	    free(usrdat);
	    break;
	  }
#endif
	case AF_INET:
	  break;
#ifdef XTINSP
	case AF_DECnet:
	    
	  optl = rcvcall->udata.maxlen;
	  status = getsockopt(fd, DNPROTO_NSP, DSO_CONDATA,
			      rcvcall->udata.buf,
			      &optl );

	  if (status == -1) {
	    map_err_to_XTI(errno,&t_errno);
	    return(-1);
	  }
	
	  rcvcall->udata.len = optl;
	  break;
#endif
	};
      }
    } /* if rcvcall */

    if (rcvcall)
      if (rcvcall->opt.maxlen > 0) {

        if (rcvcall->opt.maxlen < table(fd).info.options) {
          buffer_overflow = 1;
        }
      
	if (!buffer_overflow) {
          switch(table(fd).family) {
	  case AF_INET:
	
	    if (table(fd).xti_proto == IPPROTO_TCP) {
	      int tmp_len;
	    
	      /* here we go, let's get it from the kernel */
	  
	      tmp_len = rcvcall->opt.maxlen;
	      status = getsockopt(fd, IPPROTO_TCP, TCP_CONOPT,
				  rcvcall->opt.buf,
				  &tmp_len );
	  
	      rcvcall->opt.len = tmp_len;
	    
	      if (status == -1) {
	        map_err_to_XTI(errno,&t_errno);
	        return(-1);
	      }
	    }
	  
	    break;
#ifdef XTIOSI
	  case AF_OSI:
	    {
	      int tmp_len;
	    
	      tmp_len = rcvcall->opt.maxlen;
	      status = getsockopt(fd, OSIPROTO_COTS, TOPT_XTICONOPTS,
				  rcvcall->opt.buf,
				  &tmp_len );
	  
	      rcvcall->opt.len = tmp_len;
	    
	      if (status == -1) {
	        map_err_to_XTI(errno,&t_errno);
	        return(-1);
	      }
	    }
	    break;
#endif	  
#ifdef XTINSP
	  case AF_DECnet:
	    break;
#endif
	  };
        }
      }
    
    /*
     *	Update XTI state tables
     *
     */
    

    table(fd).event = XTI_CONNECT1;
    
    if (update_XTI_state(fd, -1, old_state) == -1) {
      t_errno = TOUTSTATE;
      return(-1);
    }

    if (buffer_overflow) {
      t_errno = TBUFOVFLW;
      return(-1);
    }

    return (0);
  }
}

/*
 *	T_ERROR - produce error message (optional)
 */

char *t_errlist[] = {
  "Error 0",
  "Incorrect addr. format",		/* TBADADDR	1  */	
  "Incorrect option format",		/* TBADOPT	2  */	
  "Incorrect permissions",		/* TACCES	3  */	
  "Illegal transport fd",      		/* TBADF	4  */	
  "Couldn't allocate addr",		/* TNOADDR	5  */	
  "Out of state",	       		/* TOUTSTATE	6  */	
  "Bad call sequence number",		/* TBADSEQ	7  */	
  "System error",	       		/* TSYSERR	8  */	
  "Event requires attention",		/* TLOOK	9  */	
  "Illegal amount of data",		/* TBADDATA	10 */	
  "Buffer not large enough",		/* TBUFOVFLW	11 */	
  "Flow control",	       		/* TFLOW	12 */	
  "No data",				/* TNODATA	13 */	
  "Discon_ind not found on q",		/* TNODIS	14 */ 	
  "Unitdata error not found",		/* TNOUDERR	15 */	
  "Bad flags",				/* TBADFLAG	16 */	
  "No ord rel found on q",		/* TNOREL	17 */	
  "Primitive not supported",		/* TNOTSUPPORT	18 */	
  "State is in process of changing",	/* TSTATECHNG	19 */	
  "Unsupported struc-type requested",	/* TNOSTRUCTYPE	20 */	
  "Invalid transport provider name",	/* TBADNAME	21 */	
  "Qlen is zero",	                /* TBADQLEN	22 */	
  "Address in use",	                /* TADDRBUSY	23 */	
  };

	
int t_nerr = (sizeof t_errlist / sizeof t_errlist[0]);
int t_errno;   /* definition for t_errno */

int
t_error (errmsg)
char *errmsg;
{
  extern int t_errno;
  extern char *t_errlist[];
  extern int t_nerr;

  if (t_errno <0 || t_errno > t_nerr) {
	t_errno=TSYSERR;
	errno=EINVAL;
	return(-1); /* protect ourselves */
  }

  write(2,errmsg,strlen(errmsg));
  write(2,":\t",strlen(":\t"));
  write(2,t_errlist[t_errno],strlen(t_errlist[t_errno]));
  write(2,"\n",sizeof("\n"));

  if (t_errno == TSYSERR) {
    perror("");
  }
  return(0);
}

/*
 *	T_FREE - free a library structure (optional)
 */

int 
t_free (ptr, struct_type)
char *ptr;
int struct_type;
{

  
  if (ptr == T_NULL) {
    t_errno = TSYSERR;
    errno = EINVAL;
    return(-1);
  }

  switch(struct_type) {
    
  case T_BIND_STR:

    if (((struct t_bind *)ptr)->addr.buf) 
      free(((struct t_bind *)ptr)->addr.buf);
    free(ptr);

    break;

  case T_OPTMGMT_STR:

    if (((struct t_optmgmt *)ptr)->opt.buf) 
      free(((struct t_optmgmt *)ptr)->opt.buf);
    free(ptr);

    break;
    
  case T_CALL_STR:

    if (((struct t_call *)ptr)->addr.buf)
      free(((struct t_call *)ptr)->addr.buf);
    if (((struct t_call *)ptr)->opt.buf) 
      free(((struct t_call *)ptr)->opt.buf);
    if (((struct t_call *)ptr)->udata.buf) 
      free(((struct t_call *)ptr)->udata.buf);
    free(ptr);
    break;
    
  case T_DIS_STR:

    if (((struct t_discon *)ptr)->udata.buf) 
      free(((struct t_discon *)ptr)->udata.buf);
    free(ptr);

    break;
  
  case T_UNITDATA_STR:

    if (((struct t_unitdata *)ptr)->addr.buf) 
      free(((struct t_unitdata *)ptr)->addr.buf);
    if (((struct t_unitdata *)ptr)->opt.buf) 
      free(((struct t_unitdata *)ptr)->opt.buf);
    if (((struct t_unitdata *)ptr)->udata.buf) 
      free(((struct t_unitdata *)ptr)->udata.buf);
    free(ptr);

    break;

  case T_UDERROR_STR:

    if (((struct t_uderr *)ptr)->addr.buf) 
      free(((struct t_uderr *)ptr)->addr.buf);
    if (((struct t_uderr *)ptr)->opt.buf) 
      free(((struct t_uderr *)ptr)->opt.buf);
    free(ptr);

    break;

  case T_INFO_STR:

    free(ptr);
    break;

  default:

#ifdef XTIXPG4
    t_errno = TNOSTRUCTYPE;
#else
    t_errno = TSYSERR;
    errno = EINVAL;
#endif

    return(-1);
    break;
  };

  return(T_NULL);
}

/*
 *	T_GETINFO - get protocol-specific service information (optional)
 */

int 
t_getinfo (fd, info)
int fd;
struct t_info *info;
{
  
  int status;

  if (!(check_xtifd(fd))) {
    t_errno = TBADF;
    return(-1);
  }

  if (d_table.dcb == T_NULL) { /* must make sure we have dynamic table built */
    t_errno = TBADF;
    return(-1);
  }
  
  if (info != T_NULL) {
    int tmp_len;
	
    /* here we go, let's get it from the kernel */
    tmp_len = sizeof(struct t_info);
    status = getsockopt(fd, SOL_SOCKET, SO_XTITPDFLT,
			(char *) info,
			&tmp_len );

    if (status == -1) {
      map_err_to_XTI(errno,&t_errno);
      return(-1);
    }
  }
  return (0);
}

/*
 * 	T_GETSTATE - get the current state (optional)
 */

int 
t_getstate(fd)
int fd;
{

  int tmp_state;
  int tmp_size;
  int status;

  if (!(check_xtifd(fd))) {
    t_errno = TBADF;
    return(-1);
  }

  if (d_table.dcb == T_NULL) { /* must make sure we have dynamic table built */
    t_errno = TBADF;
    return(-1);
  }

  if (table(fd).active_flag != 1) {
    t_errno = TBADF;
    return(-1);
  }

  if (table(fd).state < XTI_UNINIT ||
      table(fd).state > XTI_INREL) {
    t_errno = TSTATECHNG;
    return(-1);
  }

  tmp_size = sizeof(tmp_state);
  status = getsockopt(fd, SOL_SOCKET, SO_XTITPSTATE,
		      (char *) &tmp_state,
		      &tmp_size);
  
  if (status < 0) {
    map_err_to_XTI(errno,&t_errno);		
    return (-1);
  }

  table(fd).state = tmp_state;

  return(tmp_state);
}


/*
 * 	T_LISTEN - listen for a connect request
 */

int t_listen (fd, call)
int fd;
struct t_call *call;
{
  int tmp_resfd; /* temp. fd to hold connection */
  int status;
  
#ifdef XTINSP
  struct accessdata_dn nsp_accessdata;
  int optl;
#endif
  int old_state;
  int tmp_size;
  int tmp_len;
  struct xti_evtinfo evtinfo;
  int buffer_overflow = 0;

  if (!(check_xtifd(fd))) {
    t_errno = TBADF;
    return(-1);
  }

  if (d_table.dcb == T_NULL) { /* must make sure we have dynamic table built */
    t_errno = TBADF;
    return(-1);
  }

  old_state = table(fd).state;
  
  if (check_XTI_state(fd, XTI_LISTEN) == -1) {
    t_errno = TOUTSTATE;
    return(-1);
  }

  if (table(fd).info.servtype != T_COTS && 
      table(fd).info.servtype != T_COTS_ORD) {
    t_errno = TNOTSUPPORT;
    return(-1);
  }

  /*
   * check for any incoming T_DISCONNECT's
   */

  status = xti_peek(fd, &evtinfo);

  if (status == -1) return(-1);

  if (evtinfo.evtarray[ffs(T_DISCONNECT)]) {
    t_errno = TLOOK;
    return(-1);
  }

  if (table(fd).qlen == 0) {
    t_errno = TBADQLEN;
    return(-1);
  }
  
  if (call == T_NULL) {
    t_errno = TNOADDR;
    return(-1);
  }

  if (call->addr.maxlen == 0 || call->addr.buf == T_NULL) {
    t_errno = TNOADDR;
    return (-1);
  }

  call->opt.len = 0;
  call->udata.len = 0;

  tmp_len = call->addr.maxlen;
  if ((tmp_resfd = accept (fd, (struct sockaddr *) call->addr.buf,
			   (int *) &tmp_len)) < 0) {
    map_err_to_XTI(errno,&t_errno);
    return (-1);
  } 
  else {
    
    call->addr.len = tmp_len; /* fill in len for user */

    if (call->addr.maxlen < table(fd).info.addr) {
      buffer_overflow = 1;
    }
    
    /*
     * get SEQUENCE number from kernel
     */
    
    call->sequence = 0;
    tmp_size = sizeof(tmp_size);
    status = getsockopt(tmp_resfd, SOL_SOCKET, SO_XTISEQNUM,
			(char *) &call->sequence,
			&tmp_size);
    
    if (status < 0) {
      map_err_to_XTI(errno, &t_errno);
      return(-1);
    }
    
    table(fd).sequence = call->sequence; /* assoc. seq. with this fd */
    
  }
 
  /*
   * Option data
   */

  if (call->opt.maxlen > 0) {
    
    if (table(fd).info.options == T_NOTSUPPORTED) {
      t_errno = TNOTSUPPORT;
      return(-1);
    }
   
    if (call->opt.maxlen < table(fd).info.options) {
      buffer_overflow = 1;
    }

    if (!buffer_overflow) { 
      switch(table(fd).family) {
      case AF_INET:
      
        if (table(fd).xti_proto == IPPROTO_TCP) {
	
	  /* here we go, let's get it from the kernel */
	
	  tmp_len = call->opt.maxlen;
	  status = getsockopt(tmp_resfd, IPPROTO_TCP, TCP_CONOPT,
			      call->opt.buf,
			      &tmp_len );
	
	  call->opt.len = tmp_len;
	
	  if (status == -1) {
	    map_err_to_XTI(errno,&t_errno);
	    return(-1);
	  }

        }
        break;
#ifdef XTIOSI
      case AF_OSI: 
        {
          char *usrdat;
          int usrdatlen = table(fd).info.connect;

	  tmp_len = call->opt.maxlen;
	  status = getsockopt(tmp_resfd, OSIPROTO_COTS, TOPT_XTICONOPTS,
			      call->opt.buf,
			      &tmp_len );
	
	  call->opt.len = tmp_len;
	
	  if (status == -1) {
	    map_err_to_XTI(errno,&t_errno);
	    return(-1);
	  }
        }
        break;
#endif
#ifdef XTINSP
      case AF_DECnet:
      
        optl = sizeof(struct accessdata_dn);
        status = getsockopt(tmp_resfd, DNPROTO_NSP, DSO_CONACCESS, 
			    (char *) &nsp_accessdata, 
			    &optl );
      
        if (status == -1) {
	  map_err_to_XTI(errno,&t_errno);
	  return(-1);
        }
      
        call->opt.len = optl;
        bcopy(&nsp_accessdata,call->opt.buf,call->opt.len);
        break;
#endif
      }; /* end switch */
    } /* end if !buffer_overflow */
  } /* end option data */
  
  /*
   * user data
   */

  if (call->udata.maxlen > 0) {
    
    if (table(fd).info.connect == T_NOTSUPPORTED) {
      t_errno = TNOTSUPPORT;
      return(-1);
    }
    
    if (call->udata.maxlen < table(fd).info.connect) {
      buffer_overflow = 1;
    }

    if (!buffer_overflow) {
      switch(table(fd).family) {
#ifdef XTIOSI
      case AF_OSI:
        {
          char *usrdat;
          int usrdatlen = table(fd).info.connect;

          usrdat = (char *)malloc(usrdatlen);
          if (usrdat == NULL) {
            t_errno = TSYSERR;
            errno = ENOBUFS;
            return(-1);
          }
          bzero(usrdat, usrdatlen);

          status = getsockopt(tmp_resfd, OSIPROTO_COTS, TOPT_OPTCONDATA,
                              usrdat, &usrdatlen);
          if (status == -1) {
            free(usrdat);
            map_err_to_XTI(errno,&t_errno);
            return(-1);
          }

          bcopy(usrdat, call->udata.buf, usrdatlen);
          call->udata.len = usrdatlen;
        }
        break;

#endif XTIOSI
#ifdef XTINSP
      case AF_DECnet:
      
        optl = call->udata.maxlen;
        status = getsockopt(tmp_resfd, DNPROTO_NSP, DSO_CONDATA, 
			    call->udata.buf, 
			    &optl );
      
        if (status == -1) {
  	  map_err_to_XTI(errno,&t_errno);
	  return(-1);
        }
      
        call->udata.len = optl;
        break;
#endif
      default:
        break;
      
      };  /* end switch */
    }  /* end if not buffer_overflow */
  }  /* end user data */
  
  /*
   *	Update XTI state tables
   */
  
  
  table(fd).event = XTI_LISTEN;
  
  /* T_LISTEN successful */
  
  if (update_XTI_state(fd, tmp_resfd, old_state) == -1) {
    t_errno = TOUTSTATE;
    return(-1);
  }

  if (buffer_overflow) {
    t_errno = TBUFOVFLW;
    return(-1);
  }

  return (0);
}

/*
 *	T_LOOK - look at the current event on a transport endpoint
 */

int
t_look(fd)
int fd;
{

  int status;
  int xti_evt;
  int tmp_size;
  int tmp_len;
  struct xti_evtinfo evtinfo;

  if (!(check_xtifd(fd))) {
    t_errno = TBADF;
    return(-1);
  }


  status = xti_peek(fd, &evtinfo);

  if (status < 0) {
    map_err_to_XTI(errno, &t_errno);
    return(-1);
  }

  /* Special: t_look now has event precedence
   *
   * (Connect events) highest
   * T_LISTEN
   * T_CONNECT
   * (Data events)
   * T_EXDATA
   * T_DATA
   * (Disconnect events)
   * T_DISCONNECT
   * T_ORDREL
   * (Flow control events)
   * T_GOEXDATA
   * T_GODATA
   * (error events)
   * T_UDERR
   */

  if (evtinfo.evtarray[ffs(T_LISTEN)]) return(T_LISTEN);
  if (evtinfo.evtarray[ffs(T_CONNECT)]) return(T_CONNECT);
  if (evtinfo.evtarray[ffs(T_EXDATA)]) return(T_EXDATA);
#ifndef XTI_UNEVENT
  tmp_size = 0;
  tmp_len = sizeof(int);
  status = getsockopt(fd, SOL_SOCKET, SO_XTIREADEX,
		      (char *) &tmp_size,
		      &tmp_len );
  if (status < 0) {
    map_err_to_XTI(errno, &t_errno);
    return(-1);
  }
  if (tmp_size > 0) return(T_EXDATA);
#endif XTI_UNEVENT
  if (evtinfo.evtarray[ffs(T_DATA)]) return(T_DATA);
#ifndef XTI_UNEVENT
  tmp_size = 0;
  status = ioctl(fd, FIONREAD,(char *) &tmp_size );
  if (status < 0) {
    map_err_to_XTI(errno, &t_errno);
    return(-1);
  }
  if (tmp_size > 0) return(T_DATA);
#endif XTI_UNEVENT
  if (evtinfo.evtarray[ffs(T_DISCONNECT)]) return(T_DISCONNECT);
  if (evtinfo.evtarray[ffs(T_ORDREL)]) return(T_ORDREL);

  if (evtinfo.evtarray[ffs(T_GOEXDATA)] ||
      evtinfo.evtarray[ffs(T_GODATA)]) {
    
    if (evtinfo.evtarray[ffs(T_GOEXDATA)])
      xti_evt = T_GOEXDATA;
    else
      xti_evt = T_GODATA;
    
    tmp_size = sizeof(xti_evt);
    status = setsockopt(fd, SOL_SOCKET, SO_XTICLREVENT,
			(char *) &xti_evt,
			tmp_size);

    if (status < 0) {
      map_err_to_XTI(errno, &t_errno);
      return(-1);
    }
    return(xti_evt);
  }

  if (evtinfo.evtarray[ffs(T_UDERR)]) return(T_UDERR);

  /* No events */
  return(0);
}

/*
 *	T_OPEN - establish a transport endpoint
 */


int t_open (name, oflag, info)
char *name;
int oflag;
struct t_info *info;
{
  extern struct xti_lookup *getname();
  register struct xti_lookup *c;
  int fd = -1;
  int status;
  int tmp_len;
  struct t_info tmp_info;
  int xti_epvalid;

  int old_state;

  /* debug code */

  if (d_table.dcb == T_NULL) { /* must initalize dynamic descriptor table */
    d_table.dcb = (struct d_entry (*)[])
                    malloc( (unsigned) 
			   ((sizeof(struct d_entry) * getdtablesize())) );
  }

  if ( d_table.dcb == T_NULL) {
    map_err_to_XTI(ENOMEM, &t_errno);
    return(-1);
  }


  if ((int) (c = getname(name)) != -1) {
    if ((fd = socket (c->x_af, c->x_type, c->x_protocol)) == -1) {
      map_err_to_XTI(errno,&t_errno);
      return(-1);
    }
    else { 

      if (oflag != O_RDWR && oflag != (O_RDWR|O_NONBLOCK) ) {
	t_errno = TBADFLAG;
	return(-1);
      }

      xti_epvalid = 1;   /* endpoint is valid for XTI operations */

      status = setsockopt(fd, SOL_SOCKET, SO_XTIFDVALID,
			  (char *) &xti_epvalid,
			  sizeof(xti_epvalid));
      
      if (status < 0) {
	map_err_to_XTI(errno, &t_errno);
	return(-1);
      }

      if (oflag & O_NONBLOCK) {
	int f_flags;
	f_flags = fcntl(fd, F_GETFL, 0);

	/* May change when POSIX things change */

#define XTIPOSIX
#ifdef XTIPOSIX
	f_flags |= FNBLOCK;
#else
	f_flags |= FNDELAY;
#endif

	fcntl(fd, F_SETFL, f_flags);
      }
    
      /* here we go, let's get it from the kernel */
      
      tmp_len = sizeof(struct t_info);

      status = getsockopt(fd, SOL_SOCKET, SO_XTITPDFLT, 
			  (char *) &tmp_info,
			  &tmp_len );

      if (status == -1) {
	map_err_to_XTI(errno,&t_errno);
	return(-1);
      }

      if (info != T_NULL) {
	*info = tmp_info;
      }

    }

    /*
     * save info structure into descriptor table
     */

    table(fd).info = tmp_info;   /* save structure */
    table(fd).family = c->x_af;   /* address family */
    table(fd).xti_proto = c->x_protocol; /* protocol number */
  }
  else {
    t_errno = TBADNAME;
    return(-1);
  }

  /*
   *	Update XTI state tables
   *
   */

  table(fd).active_flag = 1;   /* have an active descriptor */
  table(fd).state = XTI_UNINIT;  /* default state */
  table(fd).event = XTI_OPENED;	  /* T_OPEN successful */
  old_state = table(fd).state;

  if (update_XTI_state(fd, -1, old_state) == -1) {
    t_errno = TOUTSTATE;
    return(-1);
  }

  return (fd);
  
}


/*
 *	T_OPTMGMT - manage options for a transport endpoint (optional)
 */

int t_optmgmt (fd, req, ret)
int fd;
struct t_optmgmt *req;
struct t_optmgmt *ret;
{

  int status;
  int old_state;

  if (!(check_xtifd(fd))) {
    t_errno = TBADF;
    return(-1);
  }

  if (d_table.dcb == T_NULL) { /* must make sure we have dynamic table built */
    t_errno = TBADF;
    return(-1);
  }

  old_state = table(fd).state;
  
  if (check_XTI_state(fd, XTI_IDLE) == -1) {
    t_errno = TOUTSTATE;
    return(-1);
  }

  if (req != T_NULL && ret != T_NULL) {
    switch (req->flags) {

    case T_NEGOTIATE:
      {
	int tmp_len;
	
	if ( (int) req->opt.len <= 0) {
	  t_errno = TBADOPT;
	  return(-1);
	} else if (req->opt.buf == T_NULL) {
	  t_errno = TSYSERR;
	  errno = EFAULT;
	  return(-1);
	}
	
	if ( (int) ret->opt.maxlen <= 0) {
	  t_errno = TBUFOVFLW;
	  return(-1);
	} else if (ret->opt.buf == T_NULL) {
	  t_errno = TSYSERR;
	  errno = EFAULT;
	  return(-1);
	}

	switch (table(fd).family) {
#ifdef XTIOSI
	case AF_OSI:

	  tmp_len = req->opt.len;
	  status = setsockopt(fd, OSIPROTO_COTS, TOPT_XTINEGQOS,
			      req->opt.buf,
			      tmp_len );
	  
	  ret->opt.len = tmp_len;
	  if (status == -1) {
	    map_err_to_XTI(errno,&t_errno);
	    return(-1);
	  }
	  
	  tmp_len = ret->opt.maxlen;
	  status = getsockopt(fd, OSIPROTO_COTS, TOPT_XTINEGQOS,
			      ret->opt.buf,
			      &tmp_len );
	  
	  ret->opt.len = tmp_len;
	  
	  if (status == -1) {
	    map_err_to_XTI(errno,&t_errno);
	    return(-1);
	  }

	  break;
#endif
	case AF_INET:

	  if (table(fd).xti_proto == IPPROTO_TCP) {
	    
	    tmp_len = req->opt.len;
	    status = setsockopt(fd, IPPROTO_TCP, TCP_NEGQOS,
				req->opt.buf,
				tmp_len );
	    
	    ret->opt.len = tmp_len;
	    if (status == -1) {
	      map_err_to_XTI(errno,&t_errno);
	      return(-1);
	    }
	    
	    tmp_len = ret->opt.maxlen;
	    status = getsockopt(fd, IPPROTO_TCP, TCP_NEGQOS,
				ret->opt.buf,
				&tmp_len );

	    ret->opt.len = tmp_len;

	    if (status == -1) {
	      map_err_to_XTI(errno,&t_errno);
	      return(-1);
	    }
	  } /* if TCP */

	  break;

	default:
	  t_errno = TNOTSUPPORT;
	  return(-1);

	}; /* end of switch */

	ret->flags = 0;
	break;
      }

      break;

    case T_CHECK:
      {
	int tmp_len;
	
	if ( (int) req->opt.len <= 0) {
	  t_errno = TBADOPT;
	  return(-1);
	  }
	  
	if (req->opt.buf == T_NULL) {
	  t_errno = TSYSERR;
	  errno = EFAULT;
	  return(-1);
	}
	
	switch (table(fd).family) {
#ifdef XTIOSI
	case AF_OSI:
	  
	  tmp_len = req->opt.len;
	  status = setsockopt(fd, OSIPROTO_COTS, TOPT_XTICHKQOS,
			      req->opt.buf,
			      tmp_len );
	  
	  if (status == -1) {
	    ret->flags =  T_FAILURE;
	    map_err_to_XTI(errno,&t_errno);
	    return(-1);
	  }
	    
	  break;
#endif
	case AF_INET:

	  if (table(fd).xti_proto == IPPROTO_TCP) {
	      
	    tmp_len = req->opt.len;
	    status = setsockopt(fd, IPPROTO_TCP, TCP_CHKQOS,
				req->opt.buf,
				tmp_len );
	    
	    if (status == -1) {
	      ret->flags =  T_FAILURE;
	      map_err_to_XTI(errno,&t_errno);
	      return(-1);
	    }
	  }
	  break;
	  
	default:
	  t_errno = TNOTSUPPORT;
	  return(-1);

	}; /* end of switch */

	ret->flags = T_SUCCESS;
      }
      break;
	
    case T_DEFAULT:
      {
	int tmp_len;
	    
	if (req->opt.len != 0) {
	  t_errno = TBADOPT;
	  return(-1);
	}
	    
	if ( (int) ret->opt.maxlen <= 0) {
	  t_errno = TBUFOVFLW;
	  return(-1);
	} else if (ret->opt.buf == T_NULL) {
	  t_errno = TSYSERR;
	  errno = EFAULT;
	  return(-1);
	}

	switch (table(fd).family) {
#ifdef XTIOSI
	case AF_OSI:

	  tmp_len = ret->opt.maxlen;
	  status = getsockopt(fd, OSIPROTO_COTS, TOPT_XTIDFLTQOS,
			      ret->opt.buf,
			      &tmp_len );
	  
	  ret->opt.len = tmp_len;
		
	  if (status == -1) {
	    map_err_to_XTI(errno,&t_errno);
	    return(-1);
	  }

	  break;
#endif	      
	case AF_INET:

	  if (table(fd).xti_proto == IPPROTO_TCP) {
	    tmp_len = ret->opt.maxlen;
	    status = getsockopt(fd, IPPROTO_TCP, TCP_DFLTQOS,
				ret->opt.buf,
				&tmp_len );
	    
	    ret->opt.len = tmp_len;
		
	    if (status == -1) {
	      map_err_to_XTI(errno,&t_errno);
	      return(-1);
	    }
	  }
	  break;

	default:
	  t_errno = TNOTSUPPORT;
	  return(-1);
	      
	}; /* end of switch */

	ret->flags = 0;
      }
      break;

    default:
      t_errno = TBADFLAG;
      return(-1);
    }; /* end of switch */
  } else {
    t_errno = TSYSERR;
    errno = EFAULT;
    return(-1);
  }

  /*
   *	Update XTI state tables
   *
   */
    
  table(fd).event = XTI_OPTMGMT;
    
  if (update_XTI_state(fd, -1, old_state) == -1) {
    t_errno = TOUTSTATE;
    return(-1);
  }
  return(0);
}


/*
 *	T_RCV - receive data or expedited data sent over a connection
 */

int t_rcv (fd, buf, nbytes, flags)
int fd;
char *buf;
unsigned nbytes;
int *flags;
{
  int rbytes=0;	/* actual #bytes received by transport provider */
  int status;
  int old_state;
  char *tmp_buf = NULL;
  int peek_expd;
  int peek_normal;
  struct xti_evtinfo evtinfo;


  if (d_table.dcb == T_NULL) { /* must make sure we have dynamic table built */
    t_errno = TBADF;
    return(-1);
  }

  if (!(table(fd).active_flag)) {
    t_errno = TBADF;
    return(-1);
  }

  old_state = table(fd).state;

  if (check_XTI_state(fd, XTI_RCV) == -1) {
    t_errno = TOUTSTATE;
    return(-1);
  }

  if (table(fd).info.servtype != T_COTS && 
      table(fd).info.servtype != T_COTS_ORD) {
    t_errno = TNOTSUPPORT;
    return(-1);
  }

  if (flags == T_NULL) {
    t_errno = TSYSERR;
    errno = EINVAL;
    return(-1);
  }

retry:
  *flags = 0; /* initalize flags */
  
  status = xti_peek(fd, &evtinfo);

  if (status == -1) return(-1);

  /* SPECIAL logic:
   *
   * Do not return TLOOK error for T_DISCONNECT | T_ORDREL if we have
   * either T_EXDATA | T_DATA event.
   * In addition, we give precedence of T_EXDATA over T_DATA
   */

  if ((evtinfo.evtarray[ffs(T_DISCONNECT)] || 
       evtinfo.evtarray[ffs(T_ORDREL)]) &&
      ( (!evtinfo.evtarray[ffs(T_DATA)]) &&
       (!evtinfo.evtarray[ffs(T_EXDATA)]))) {
    t_errno = TLOOK;
    return(-1);
  }
      
  switch(table(fd).family) {
  case AF_INET:
    break;
  case AF_DECnet:
    break;
#ifdef XTIOSI
  case AF_OSI:
    tmp_buf = (char *)malloc(nbytes+1);

    if (tmp_buf == T_NULL) {
      t_errno = TSYSERR;
      errno = ENOBUFS;
      return(-1);
    }
    break;
#endif
  };

  peek_expd = evtinfo.evtarray[ffs(T_EXDATA)];

  if (peek_expd != 0) {
    switch(table(fd).family) {
    case AF_INET:
      break;
    case AF_DECnet:
      break;
#ifdef XTIOSI
    case AF_OSI:
      peek_expd = recv (fd, tmp_buf, (int) nbytes+1,
			MSG_OOB | MSG_PEEK); /* get count */
      if (peek_expd <= nbytes)
	*flags &= ~T_MORE;
      else
	*flags |= T_MORE;
      break;
#endif
    };

    rbytes = recv (fd, buf, (int) nbytes, MSG_OOB);
    *flags |= T_EXPEDITED;
  }
  else {

    switch(table(fd).family) {
    case AF_INET:
      break;
    case AF_DECnet:
      break;
#ifdef XTIOSI
    case AF_OSI:
      peek_normal = recv (fd, tmp_buf, (int) nbytes+1, MSG_PEEK);
      /*
       * If we get ENOTEMPTY error, then we got expedited data while
       * we were waiting.  So, go back and try to get it.  Otherwise,
       * we got normal data.
       */
      if (peek_normal < 0 && errno == ENOTEMPTY) {
        if (tmp_buf) free(tmp_buf);
        goto retry;
      }
      if (peek_normal <= nbytes)
	*flags &= ~T_MORE;
      else
	*flags |= T_MORE;
      break;
#endif
    };

    rbytes = recv (fd, buf, (int) nbytes, 0);
  }

  if (tmp_buf) free(tmp_buf);

  if (rbytes < 0 && errno == ENOTEMPTY)
      goto retry;

  if (rbytes < 0) {
    map_err_to_XTI(errno,&t_errno);		
    return (-1);
  }

  /*
   *	Update XTI state tables
   *
   */
  
  table(fd).event = XTI_RCV;
  
  if (table(fd).state != T_DATAXFER) { /* performance code */
    if (update_XTI_state(fd, -1, old_state) == -1) {
      t_errno = TOUTSTATE;
      return(-1);
    }
  }

  return (rbytes);
}



/*
 *	T_RCVCONNECT - receive the confirmation from a connect request
 */

int t_rcvconnect (fd, call)
int fd;
struct t_call *call;
{

  int optl;
  int status;
  int status1;
  int old_state;
  int tmp_size;
  struct xti_evtinfo evtinfo;
  int buffer_overflow = 0;
  int tmp_len;

  if (!(check_xtifd(fd))) {
    t_errno = TBADF;
    return(-1);
  }

  if (d_table.dcb == T_NULL) { /* must make sure we have dynamic table built */
    t_errno = TBADF;
    return(-1);
  }

  old_state = table(fd).state;

  if (check_XTI_state(fd, XTI_RCVCONNECT) == -1) {
    t_errno = TOUTSTATE;
    return(-1);
  }

  if (table(fd).info.servtype != T_COTS && 
      table(fd).info.servtype != T_COTS_ORD) {
    t_errno = TNOTSUPPORT;
    return(-1);
  }

  if ( (status = xti_peek(fd, &evtinfo)) == -1) return(-1);

  if (evtinfo.evtarray[ffs(T_DISCONNECT)]) {
    t_errno = TLOOK;
    return(-1);
  }

  if (evtinfo.evtarray[ffs(T_CONNECT)] == 0) {
    t_errno = TNODATA;
    return(-1);
  }
  else {
    /*
     * get rid of T_CONNECT event
     */

    tmp_size = sizeof(status1);
    status1 = T_CONNECT;
    status = setsockopt(fd, SOL_SOCKET, SO_XTICLREVENT,
			(char *) &status1,
			tmp_size);

    if (status < 0) {
      map_err_to_XTI(errno, &t_errno);
      return(-1);
    }
  }

  if (call != T_NULL) {

    /*
     * Address information
     */

    if (call->addr.maxlen > 0) {
      if (call->addr.maxlen < table(fd).info.addr) {
        buffer_overflow = 1;
      }

      if (!buffer_overflow) {
        /*
         * go to kernel to get sockname
         */

        tmp_len = call->addr.maxlen;
        status = getsockname(fd, call->addr.buf, &tmp_len);

        if (status == -1) {
          map_err_to_XTI(errno,&t_errno);
          return(-1);
        }
        call->addr.len = tmp_len;
      }
    }
    
    /*
     * User data
     */
 
    if (call->udata.maxlen > 0) {
      if (table(fd).info.connect == T_NOTSUPPORTED) {
	t_errno = TNOTSUPPORT;
	return(-1);
      }

      switch(table(fd).family) {

      case AF_INET:
	break;
#ifdef XTIOSI
      case AF_OSI:
	{
	  char *usrdat;
	  int usrdatlen = table(fd).info.connect;

	  usrdat = (char *)malloc(usrdatlen);
	  if (usrdat == NULL) {
	    t_errno = TSYSERR;
	    errno = ENOBUFS;
	    return(-1);
	  }
	  bzero(usrdat, usrdatlen);

	  status = getsockopt(fd, OSIPROTO_COTS, TOPT_OPTCONDATA,
			      usrdat, &usrdatlen);
	  if (status == -1) {
	    free(usrdat);
	    map_err_to_XTI(errno,&t_errno);
	    return(-1);
	  }

	  if (call->udata.maxlen < usrdatlen) {
	    buffer_overflow = 1;
	  }

	  if (!buffer_overflow) {
            bcopy(usrdat, call->udata.buf, usrdatlen);
	    call->udata.len = usrdatlen;
          }
	  break;
	}
#endif
#ifdef XTINSP
      case AF_DECnet:

	optl = call->udata.maxlen;
	status = getsockopt(fd, DNPROTO_NSP, DSO_CONDATA,
			    call->udata.buf,
			    &optl );

	if (status == -1) {
	  map_err_to_XTI(errno,&t_errno);
	  return(-1);
	}
      	
	call->udata.len = optl;
	break;
#endif	
      default:
	break;
      };  /* end switch */
    }  /* end user data */

    /*
     * Options
     */

    if (call->opt.maxlen > 0) {
      if (call->opt.maxlen < table(fd).info.options) {
        buffer_overflow = 1;
      }
  
      if (!buffer_overflow) {
        switch(table(fd).family) {
        case AF_INET:
	
	  if (table(fd).xti_proto == IPPROTO_TCP) {
	    int tmp_len;
	  
	    /* here we go, let's get it from the kernel */
	  
	    tmp_len = call->opt.maxlen;

	    status = getsockopt(fd, IPPROTO_TCP, TCP_CONOPT,
			        call->opt.buf,
			        &tmp_len );
	  
	    call->opt.len = tmp_len;
	    
	    if (status == -1) {
	      map_err_to_XTI(errno,&t_errno);
	      return(-1);
	    }
	  }
	  break;
#ifdef XTIOSI
        case AF_OSI:
	  {
	  int tmp_len;
	  
	  /* here we go, let's get it from the kernel */
	  
	  tmp_len = call->opt.maxlen;

	  status = getsockopt(fd, OSIPROTO_COTS, TOPT_XTICONOPTS,
			      call->opt.buf,
			      &tmp_len );
	  
	  call->opt.len = tmp_len;
	    
	  if (status == -1) {
	    map_err_to_XTI(errno,&t_errno);
	    return(-1);
	  }
	}
	break;
#endif
#ifdef XTINSP
      case AF_DECnet:
	break;
#endif
      }; /* end switch */
    }  /* end if !buffer_overflow */
  }  /* end options */
 }  /* end call !NULL */

  /*
   *	Update XTI state tables
   *
   */

  table(fd).event = XTI_RCVCONNECT;

  if (update_XTI_state(fd, -1, old_state) == -1) {
    t_errno = TOUTSTATE;
    return(-1);
  }

  if (buffer_overflow) {
    t_errno = TBUFOVFLW;
    return(-1);
  }
  return (0);
}


/*
 *	T_RCVDIS - retrieve information from disconnect
 */

int t_rcvdis
 (fd, discon)
int fd;
struct t_discon *discon;
{

  int status, status1;
  int optl;
  int old_state;
  int tmp_size;
  struct xti_evtinfo evtinfo;
  int buffer_overflow = 0;
#ifdef XTIOSI
  char *discondat;
  u_char reason;
#endif XTIOSI

  if (!(check_xtifd(fd))) {
    t_errno = TBADF;
    return(-1);
  }

  if (d_table.dcb == T_NULL) { /* must make sure we have dynamic table built */
    t_errno = TBADF;
    return(-1);
  }

  old_state = table(fd).state;

  if ( (check_XTI_state(fd, XTI_RCVDIS1) == -1) &&
      (check_XTI_state(fd, XTI_RCVDIS2) == -1) &&
      (check_XTI_state(fd, XTI_RCVDIS3) == -1) ) {
    t_errno = TOUTSTATE;
    return(-1);
  }

  if (table(fd).info.servtype != T_COTS && 
      table(fd).info.servtype != T_COTS_ORD) {
    t_errno = TNOTSUPPORT;
    return(-1);
  }

  status = xti_peek(fd, &evtinfo);

  if (status == -1) return(-1);

  if (evtinfo.evtarray[ffs(T_DISCONNECT)] == T_NULL) {
    t_errno = TNODIS;
    return(-1);
  } else {
    
    /*
     * get rid of T_DISCONNECT event
     */

    tmp_size = sizeof(status1);
    status1 = T_DISCONNECT;
    status = setsockopt(fd, SOL_SOCKET, SO_XTICLREVENT,
			(char *) &status1,
			tmp_size);

    if (status < 0) {
      map_err_to_XTI(errno, &t_errno);
      return(-1);
    }
  }

  switch (table(fd).family) {

#ifdef XTIOSI
  case AF_OSI:
    optl = table(fd).info.discon;

    discondat = (char *)malloc(optl);
    if (discondat == NULL) {
      t_errno = TSYSERR;
      errno = ENOBUFS;
      return(-1);
    }

    status = getsockopt(fd, OSIPROTO_COTS, TOPT_OPTDISDATA,
			discondat, &optl);

    if (status == -1) {
      free(discondat);
      map_err_to_XTI(errno,&t_errno);
      return(-1);
    }
	
    if ( (discon->udata.maxlen > 0) &&
         (discon->udata.maxlen < optl) ) {
        free(discondat);
	buffer_overflow = 1;
    }

    if (!buffer_overflow) {
      bcopy(discondat, discon->udata.buf, optl);
      discon->udata.len = optl;
      free(discondat);
    }

    optl = sizeof(discon->reason);
    status = getsockopt(fd, OSIPROTO_COTS, TOPT_DISREASON,
			&reason, &optl);
    if (status == -1) {
      map_err_to_XTI(errno,&t_errno);
      return(-1);
    }
	
    discon->reason = reason;
    break;
#endif

#ifdef XTINSP
  case AF_DECnet:
  
    optl = discon->udata.maxlen;
    status = getsockopt(fd, DNPROTO_NSP, DSO_DISDATA, 
			discon->udata.buf,
			&optl );

    if (status == -1) {
      map_err_to_XTI(errno,&t_errno);
      return(-1);
    }
	
    discon->reason = ( (struct optdata_dn *) discon->udata.buf)->opt_status;
    /* get disconnect reason */

    if (discon->udata.maxlen > 0) {
      if (discon->udata.maxlen < optl) {
	buffer_overflow = 1;
      }

      if (!buffer_overflow)
        discon->udata.len = 
          ( (struct optdata_dn *) discon->udata.buf)->opt_optl;

    }
    break;
#endif

  default:
    break;
  }

  /*
   *	Update XTI state tables
   *
   */

  if (table(fd).cnt_outs_con_ind == 0)
    table(fd).event = XTI_RCVDIS1;
  else
    if (table(fd).cnt_outs_con_ind == 1)
      table(fd).event = XTI_RCVDIS2;
    else
      if (table(fd).cnt_outs_con_ind > 1)
	table(fd).event = XTI_RCVDIS3;

  if (update_XTI_state(fd, -1, old_state) == -1) {
    t_errno = TOUTSTATE;
    return(-1);
  }

  if (buffer_overflow) {
    t_errno = TBUFOVFLW;
    return(-1);
  }

  return (0);
}


/*
 *   T_RCVREL - acknowledge receipt of an orderly release indication (optional)
 */

int 
t_rcvrel (fd)
int fd;
{

  int status;
  int status1;
  int old_state;
  int tmp_size;
  struct xti_evtinfo evtinfo;

  if (!(check_xtifd(fd))) {
    t_errno = TBADF;
    return(-1);
  }

  if (d_table.dcb == T_NULL) { /* must make sure we have dynamic table built */
    t_errno = TBADF;
    return(-1);
  }

  old_state = table(fd).state;

  if (check_XTI_state(fd, XTI_RCVREL) == -1) {
    t_errno = TOUTSTATE;
    return(-1);
  }

  if (table(fd).info.servtype != T_COTS_ORD) {
    t_errno = TNOTSUPPORT;
    return(-1);
  }

  status = xti_peek(fd, &evtinfo);

  if (status == -1) return(-1);

  if (evtinfo.evtarray[ffs(T_DISCONNECT)]) {
    t_errno = TLOOK;
    return(-1);
  }

  if (evtinfo.evtarray[ffs(T_ORDREL)] == T_NULL) {
    t_errno = TNOREL;
    return(-1);
  } else {
  
    /*
     * get rid of T_ORDREL event
     */

    tmp_size = sizeof(status1);
    status1 = T_ORDREL;
    status = setsockopt(fd, SOL_SOCKET, SO_XTICLREVENT,
			(char *) &status1,
			tmp_size);

    if (status < 0) {
      map_err_to_XTI(errno, &t_errno);
      return(-1);
    }
  }

  /*
   *	Update XTI state tables
   *
   */

  table(fd).event = XTI_RCVREL;
	    
  if (update_XTI_state(fd, -1, old_state) == -1) {
    t_errno = TOUTSTATE;
    return(-1);
  }
  return (0);
}


/*
 *	T_RCVUDATA - receive a data unit
 */

int
t_rcvudata (fd, unitdata, flags)
int fd;
struct t_unitdata *unitdata;
int *flags;
{

  int cc;
  int status;
  int old_state;
  struct xti_evtinfo evtinfo;

  if (flags != T_NULL) {
    *flags = 0;
  }

  if (!(check_xtifd(fd))) {
    t_errno = TBADF;
    return(-1);
  }

  if (d_table.dcb == T_NULL) { /* must make sure we have dynamic table built */
    t_errno = TBADF;
    return(-1);
  }

  if (table(fd).info.servtype != T_CLTS) {
    t_errno = TNOTSUPPORT;
    return(-1);
  }

  old_state = table(fd).state;

  if (check_XTI_state(fd, XTI_RCVUDATA) == -1) {
    t_errno = TOUTSTATE;
    return(-1);
  }
  
  if (unitdata->opt.maxlen > 0) {
    t_errno = TNOTSUPPORT;
    return(-1);
  }

  status = xti_peek(fd, &evtinfo);

  if (status == -1) return(-1);

  if (evtinfo.evtarray[ffs(T_UDERR)]) {
    t_errno = TLOOK;
    return(-1);
  }

  cc = recvfrom(fd, unitdata->udata.buf, (int) unitdata->udata.maxlen,
		0,
		(struct sockaddr *) unitdata->addr.buf, 
		(int *) &unitdata->addr.maxlen);

  unitdata->addr.len = unitdata->addr.maxlen;
  unitdata->udata.len = cc;

  if (cc == -1) {
    map_err_to_XTI(errno, &t_errno);		
    return(-1);
  }


  /*
   *	Update XTI state tables
   *
   */

  table(fd).event = XTI_RCVUDATA;
	    
  if (update_XTI_state(fd, -1, old_state) == -1) {
    t_errno = TOUTSTATE;
    return(-1);
  }
  return (0);
}

/*
 *	T_RCVUDERR - receive a unit data error indication
 */

int 
t_rcvuderr (fd, uderr)
int fd;
struct t_uderr *uderr;
{

  int status;
  int old_state;
  struct xti_evtinfo evtinfo;

  if (!(check_xtifd(fd))) {
    t_errno = TBADF;
    return(-1);
  }

  if (d_table.dcb == T_NULL) { /* must make sure we have dynamic table built */
    t_errno = TBADF;
    return(-1);
  }

  if (table(fd).info.servtype != T_CLTS) {
    t_errno = TNOTSUPPORT;
    return(-1);
  }
  
  old_state = table(fd).state;

  if (check_XTI_state(fd, XTI_RCVUDERR) == -1) {
    t_errno = TOUTSTATE;
    return(-1);
  }

  status = xti_peek(fd, &evtinfo);

  if (status == -1) return(-1);
  
  if (evtinfo.evtarray[ffs(T_UDERR)] == T_NULL) {
    t_errno = TNOUDERR;
    return(-1);
  }

  /*
   *	Update XTI state tables
   *
   */

  table(fd).event = XTI_RCVUDERR;
	    
  if (update_XTI_state(fd, -1, old_state) == -1) {
    t_errno = TOUTSTATE;
    return(-1);
  }
  return (0);
}


/*
 *	T_SND - send data or expedited data over a connection
 */

int t_snd (fd, buf, nbytes, flags)
int fd;
char *buf;
unsigned nbytes;
int flags;
{
  int sbytes;	/* # bytes actually sent by transport provider */
  int snd_flags; /* flags used on send call */
  int status;
  int old_state;
  int tmp_size;
  struct xti_evtinfo evtinfo;

  if (d_table.dcb == T_NULL) { /* must make sure we have dynamic table built */
    t_errno = TBADF;
    return(-1);
  }

/* use cache value for performace reasons
  if (!(check_xtifd(fd))) {
    t_errno = TBADF;
    return(-1);
  }
*/

  if (!(table(fd).active_flag)) {
    t_errno = TBADF;
    return(-1);
  }

  old_state = table(fd).state;

  if (check_XTI_state(fd, XTI_SND) == -1) {
    t_errno = TOUTSTATE;
    return(-1);
  }

  if (table(fd).info.servtype != T_COTS && 
      table(fd).info.servtype != T_COTS_ORD) {
    t_errno = TNOTSUPPORT;
    return(-1);
  }

  /*
   * check flags
   */

  snd_flags = 0; /* default */

  if (flags & T_MORE) snd_flags |= MSG_MORE;
  if (flags & T_EXPEDITED) snd_flags |= MSG_OOB;

  if (flags & ~(T_MORE|T_EXPEDITED)) {
    t_errno = TBADFLAG;
    return(-1);
  }

  status = xti_peek(fd, &evtinfo);

  if (status == -1) return(-1);

  if (evtinfo.evtarray[ffs(T_ORDREL)] ||
      evtinfo.evtarray[ffs(T_DISCONNECT)]) {
    t_errno = TLOOK;
    return(-1);
  }

  /*
   * Because T_GODATA and T_GOEXDATA never returns a TLOOK error we get rid of these
   * events here because we are sending out data.
   */
  
  if (evtinfo.evtarray[ffs(T_GODATA)] ||
      evtinfo.evtarray[ffs(T_GOEXDATA)]) { /* Is this right for OSI?? */

    tmp_size = sizeof(status);
    
    status = setsockopt(fd, SOL_SOCKET, SO_XTICLREVENT,
			(char *) &status,
			tmp_size);
	
    if (status < 0) {
      map_err_to_XTI(errno, &t_errno);
      return(-1);
    }
  }
  
  if ((sbytes = send (fd, buf, (int) nbytes, snd_flags)) < 0) {
    map_err_to_XTI(errno,&t_errno);		
    
    if (errno == EWOULDBLOCK)
      t_errno = TFLOW; /* override default mapping */
    else if (errno == EINVAL && (flags & T_EXPEDITED))
      t_errno = TBADFLAG;
    
    return (-1);
  }
  

  /*
   *	Update XTI state tables
   *
   */

  if (table(fd).state != T_DATAXFER) { /* performance code */
    table(fd).event = XTI_SND;
	    
    if (update_XTI_state(fd, -1, old_state) == -1) {
      t_errno = TOUTSTATE;
      return(-1);
    }
  }

  return (sbytes);
}


/*
 *	T_SNDDIS - send user-initiated disconnect request
 */

int t_snddis (fd, call)
     int fd;
     struct t_call *call;
{
  int status;
  int optl;
  int old_state;
  int tmp_sequence = 0;

#ifdef	XTIOSI
  char *discondat;
#endif
  
  struct linger ling_value;
  ling_value.l_onoff = 0;
  ling_value.l_linger = 1;

  if (!(check_xtifd(fd))) {
    t_errno = TBADF;
    return(-1);
  }

  if (d_table.dcb == T_NULL) { /* must make sure we have dynamic table built */
    t_errno = TBADF;
    return(-1);
  }

  /*
   * do some up-front checking
   */

  if (call != T_NULL) {
    if (table(fd).state == XTI_INCON && call->sequence <= 0) {
      t_errno = TBADSEQ;
      return(-1);
    } else {
      tmp_sequence = call->sequence;
    }
  } else if (table(fd).state == XTI_INCON) {
    t_errno = TBADSEQ;
    return(-1);
  }

  old_state = table(fd).state;
  
  if ( (check_XTI_state(fd, XTI_SNDDIS1) == -1) &&
      (check_XTI_state(fd, XTI_SNDDIS2) == -1) ) {
    t_errno = TOUTSTATE;
    return(-1);
  }

  if (table(fd).info.servtype != T_COTS && 
      table(fd).info.servtype != T_COTS_ORD) {
    t_errno = TNOTSUPPORT;
    return(-1);
  }

#ifdef XTINSP

  if (table(fd).family == AF_DECnet) {
    if (table(fd).resfd != -1) {
      
      status = setsockopt(table(fd).resfd, SOL_SOCKET, SO_LINGER,
			  (char *) &ling_value,
			  sizeof(ling_value));
      if (status == -1) {
	map_err_to_XTI(errno,&t_errno);
	return(-1);
      }
    }
    else {
      
      status = setsockopt(fd, SOL_SOCKET, SO_LINGER,
			  (char *) &ling_value,
			  sizeof(ling_value));
      
      if (status == -1) {
	map_err_to_XTI(errno,&t_errno);
	return(-1);
      }
    }
  }
#endif

  /*
   * Send user (disconnect) data (if any) 
   */

  if (call)
    if (call->udata.len > 0) {

      switch(table(fd).family) {
#ifdef XTIOSI
      case AF_OSI:
        if (table(fd).info.discon == T_NOTSUPPORTED) {
          t_errno = TNOTSUPPORT;
          return(-1);
        }

	if (call->udata.len > table(fd).info.discon) {
	  t_errno = TBADDATA;
	  return(-1);
	}
       
        discondat = (char *)malloc(table(fd).info.discon);
        if (discondat == NULL) {
          t_errno = TSYSERR;
          errno = ENOBUFS;
          return(-1);
        }

        optl = call->udata.len;
	bcopy(call->udata.buf, discondat, optl);
	
	status = setsockopt(fd,  OSIPROTO_COTS, TOPT_OPTDISDATA,
			    discondat, optl);
	free(discondat);
	if (status == -1) {
	  map_err_to_XTI(errno,&t_errno);
	  return(-1);
	}

        break;
#endif
#ifdef XTINSP
      case AF_DECnet:
	if (table(fd).info.discon == T_NOTSUPPORTED) {
	  t_errno = TNOTSUPPORT;
	  return(-1);
	}
       
	if (call->udata.len > table(fd).info.discon) {
	  t_errno = TBADDATA;
	  return(-1);
	}
       
	optl = call->udata.len;
	
	if (table(fd).resfd != -1) {
	
	  status = setsockopt(table(fd).resfd, DNPROTO_NSP, DSO_DISDATA, 
			      call->udata.buf,
			      optl );
	  
       
	  if (status == -1) {
	    map_err_to_XTI(errno,&t_errno);
	    return(-1);
	  }
	}
	else {
	  status = setsockopt(fd, DNPROTO_NSP, DSO_DISDATA, 
			      call->udata.buf,
			      optl );
	
	  if (status == -1) {
	    map_err_to_XTI(errno,&t_errno);
	    return(-1);
	  }
	  
	}
        break;
#endif
      default:
        break;
      }
    }
  /* 
   * do setsockopt to check sequence number(ie. T_INCON state) and proceed with
   * the reject of the CI / the abortive release of the connection.
   */
    
    
  switch(table(fd).family) {

#ifdef XTINSP
  case AF_DECnet:

    if (call) {
      status = setsockopt(table(fd).resfd, DNPROTO_NSP, DSO_CONREJECT,
			  (char *) &((struct optdata_dn *)call->udata.buf)->opt_status,
			  sizeof(((struct optdata_dn *)call->udata.buf)->opt_status));
	
      if (status == -1) {
	map_err_to_XTI(errno,&t_errno);
	return(-1);
      }
    }
    break;

#endif

#ifdef XTIOSI
  case AF_OSI:

    status = setsockopt(fd, SOL_SOCKET, SO_XTIABORT, 
			(char *) &tmp_sequence,
			sizeof(tmp_sequence));
      
    if (status < 0) {
      if (errno == EINVAL) {
	t_errno = TBADSEQ;
	return(-1);
      }
      map_err_to_XTI(errno,&t_errno);
      return(-1);
    }
    break;
#endif
  case AF_INET:

    switch(table(fd).xti_proto) {
      
    case IPPROTO_TCP:
      
      status = setsockopt(fd, SOL_SOCKET,SO_XTIABORT,
			  (char *) &tmp_sequence,
			  sizeof(tmp_sequence));
      
      if (status < 0) {
	if (errno == EINVAL) {
	  t_errno = TBADSEQ;
	  return(-1);
	}
	map_err_to_XTI(errno,&t_errno);
	return(-1);
      }
      break;
    
    default:
      t_errno = TNOTSUPPORT;
      return(-1);
      break;
    };
    break;
    
  default:
    t_errno = TNOTSUPPORT;
    return(-1);
    break;
  };

  
  /*
   *	Update XTI state tables
   *
   */
  
  if (table(fd).cnt_outs_con_ind <= 1)
    table(fd).event = XTI_SNDDIS1;
  else
    if (table(fd).cnt_outs_con_ind > 1)
      table(fd).event = XTI_SNDDIS2;
  
  if (update_XTI_state(fd, -1, old_state) == -1) {
    t_errno = TOUTSTATE;
    return(-1);
  }
  return (0);
}


/*
 *	T_SNDREL - initiate an orderly release (optional)
 */

int 
  t_sndrel (fd)
int fd;
{

  int status;
#ifdef XTINSP
  int optl;
#endif
  struct linger ling_value;
  int old_state;
  struct xti_evtinfo evtinfo;

  ling_value.l_onoff = 1;
  ling_value.l_linger = 1;

  if (!(check_xtifd(fd))) {
    t_errno = TBADF;
    return(-1);
  }

  if (d_table.dcb == T_NULL) { /* must make sure we have dynamic table built */
    t_errno = TBADF;
    return(-1);
  }
  
  old_state = table(fd).state;

  if (check_XTI_state(fd, XTI_SNDREL) == -1) {
    t_errno = TOUTSTATE;
    return(-1);
  }
  
  if (table(fd).info.servtype != T_COTS_ORD) {
    t_errno = TNOTSUPPORT;
    return(-1);
  }
  
  /*
   * See if we got a T_DISCONNECT in
   */

  status = xti_peek(fd, &evtinfo);

  if (status == -1) return(-1);
  
  if (evtinfo.evtarray[ffs(T_DISCONNECT)]) {
    t_errno = TLOOK;
    return(-1);
  }

  /* do shutdown */
  status = shutdown(fd, 1);   /* disallow sends  */
  if (status == -1) {
    map_err_to_XTI(errno, &t_errno);
    return(-1);
  }
  
  /*
   *	Update XTI state tables
   *
   */

  table(fd).event = XTI_SNDREL;
	    
  if (update_XTI_state(fd, -1, old_state) == -1) {
    t_errno = TOUTSTATE;
    return(-1);
  }
  return (0);
}


/*
 *	T_SNDUDATA - send a data unit
 */

int
t_sndudata (fd, unitdata)
int fd;
struct t_unitdata *unitdata;
{

  int cc; /* characters that were sent */
  int status;
  int old_state;
  struct xti_evtinfo evtinfo;

  if (!(check_xtifd(fd))) {
    t_errno = TBADF;
    return(-1);
  }

  if (d_table.dcb == T_NULL) { /* must make sure we have dynamic table built */
    t_errno = TBADF;
    return(-1);
  }

  if (table(fd).info.servtype != T_CLTS) {
    t_errno = TNOTSUPPORT;
    return(-1);
  }

  old_state = table(fd).state;

  if (check_XTI_state(fd, XTI_SNDUDATA) == -1) {
    t_errno = TOUTSTATE;
    return(-1);
  }

  /*
   * This is a proper check, but is not conformant to
   * current XTI spec.
   */
  if (table(fd).info.addr != -1 && table(fd).info.addr != -2)
    if (unitdata->addr.len > table(fd).info.addr) {
#ifdef XTIXPG4
      t_errno = TBADADDR;
      return(-1);
#else
      t_errno = TSYSERR;
      errno = EINVAL;
#endif
    
    }

  status = xti_peek(fd, &evtinfo);

  if (status == -1) return(-1);

  if (evtinfo.evtarray[ffs(T_UDERR)]) {
    t_errno = TLOOK;
    return(-1);
  }

  cc = sendto(fd, unitdata->udata.buf, (int) unitdata->udata.len, 0,
	 (struct sockaddr *) unitdata->addr.buf, (int) unitdata->addr.len);

  if (cc == -1) {
    map_err_to_XTI(errno,&t_errno);		
    return(-1);
  }


  /*
   *	Update XTI state tables
   *
   */

  table(fd).event = XTI_SNDUDATA;
	    
  if (update_XTI_state(fd, -1, old_state) == -1) {
    t_errno = TOUTSTATE;
    return(-1);
  }
  return(0);
}

/*
 *	T_SYNC - synchronize transport library
 */

int 
t_sync (fd)
int fd;
{

  int status;
  int tmp_state;
  int tmp_size;
  struct t_info tmp_info;
  struct protoinfo {
    int family; /* Protool Family */
    int xti_proto;  /* protocol within Family */
  } proto_info;

  if (d_table.dcb == T_NULL) { /* must initalize dynamic descriptor table */
    d_table.dcb = (struct d_entry (*)[])
                    malloc( (unsigned) ((sizeof(struct d_entry) * getdtablesize())) );
  }

  if ( d_table.dcb == T_NULL) {
    map_err_to_XTI(ENOMEM, &t_errno);
    return(-1);
  }

  if (table(fd).active_flag != 1) {
    status = setsockopt(fd, SOL_SOCKET, SO_XTISYNC, (char *) 0, 0);

    if (status < 0) {
      map_err_to_XTI(errno,&t_errno);		
      return (-1);
    }
  }

  tmp_size = sizeof(tmp_state);
  status = getsockopt(fd, SOL_SOCKET, SO_XTITPSTATE,
		      (char *) &tmp_state,
		      &tmp_size);

  if (status < 0) {
    map_err_to_XTI(errno,&t_errno);		
    return (-1);
  }

  tmp_size = sizeof(struct protoinfo);
  status = getsockopt(fd, SOL_SOCKET, SO_XTITPPROTO,
		      (char *) &proto_info,
		      &tmp_size);

  if (status < 0) {
    map_err_to_XTI(errno,&t_errno);		
    return (-1);
  }

  table(fd).active_flag = 1;   /* make an active descriptor */
  table(fd).state = tmp_state;  /* default state */

  (void) t_getinfo(fd, &tmp_info);

  table(fd).info = tmp_info;
  table(fd).family = proto_info.family;
  table(fd).xti_proto = proto_info.xti_proto;

  return(tmp_state);


}



/*
 *	T_UNBIND - disable transport endpoint and unbind socket
 */

int 
t_unbind (fd)
int fd;
{

  int status;
  int old_state;
  int xti_evtenable;
  int xti_unbindflag;
  struct xti_evtinfo evtinfo;

  if (!(check_xtifd(fd))) {
    t_errno = TBADF;
    return(-1);
  }

  if (d_table.dcb == T_NULL) { /* must make sure we have dynamic table built */
    t_errno = TBADF;
    return(-1);
  }

  old_state = table(fd).state;

  if (check_XTI_state(fd, XTI_UNBIND) == -1) {
    t_errno = TOUTSTATE;
    return(-1);
  }

  /*
   * check for any incoming T_LISTEN's
   */

  status = xti_peek(fd, &evtinfo);

  if (status == -1) return(-1);

  if (evtinfo.evtarray[ffs(T_LISTEN)]) {
    t_errno = TLOOK;
    return(-1);
  }

/*
 * Explicitly DISABLE EVENTS
 */

  xti_evtenable = 0;   /* endpoint has events disabled */

  status = setsockopt(fd, SOL_SOCKET, SO_XTIENEVENT,
		      (char *) &xti_evtenable,
		      sizeof(xti_evtenable));
      
  if (status < 0) {
    map_err_to_XTI(errno, &t_errno);
    return(-1);
  }

/*
 * Explicitly unbind socket (clear fields without deallocating control
 * blocks
 */

  xti_unbindflag = 1;   /* unbind endpoint */

  status = setsockopt(fd, SOL_SOCKET, SO_XTIUNBIND,
		      (char *) &xti_unbindflag,
		      sizeof(xti_unbindflag));
      
  if (status < 0) {
    map_err_to_XTI(errno, &t_errno);
    return(-1);
  }

  /*
   *	Update XTI state tables
   *
   */

  table(fd).event = XTI_UNBIND;
	    
  if (update_XTI_state(fd, -1, old_state) == -1) {
    t_errno = TOUTSTATE;
    return(-1);
  }
  return (0);
}
