#ifndef lint
static char *sccsid = "@(#)laps.c	4.1      ULTRIX 	7/2/90";
#endif
/*
 *    laps.c	Local Area Print Service
 *
 *****************************************************************************
 *									     *
 *  COPYRIGHT (c) 1986, 1987, 1988       				     *
 *  By DIGITAL EQUIPMENT CORPORATION, Maynard, Mass.			     *
 *									     *
 *  This software is furnished under a license and may be used and  copied   *
 *  only  in  accordance  with  the  terms  of  such  license and with the   *
 *  inclusion of the above copyright notice.  This software or  any  other   *
 *  copies  thereof may not be provided or otherwise made available to any   *
 *  other person.  No title to and ownership of  the  software  is  hereby   *
 *  transferred.      							     *
 *									     *
 *  The information in this software is subject to change  without  notice   *
 *  and  should  not  be  construed  as  a commitment by Digital Equipment   *
 *  Corporation.							     *
 *									     *
 *  Digital assumes no responsibility for the use or  reliability  of  its   *
 *   software on equipment which is not supplied by Digital.		     *
 *									     *
 *****************************************************************************
 * EDIT HISTORY
 * --------------
 *
 * V2.000  27-Jan-1989  APK  Fix unalinged data access problem for MIPS
 *                           (1) By adding ifdef MIPS -INSERT16 macro, and
 *                           (2) temporary coping data before referencing it.
 *
 * V1.000   5-May-1989  APK  Change user infor from "host::username" to
 *                             "username@host"  -- QAR#13
 *
 *---------------------Begin V2.0---------------------------------------
 *  V0.012   2-Nov-1988 APK  Sleep when write blocks to avoid CPU hog!
 *  V0.011  18-Jul-1988 APK  Implement Resource Reporting
 *  V0.010  15-Jul-1988 APK  Implement RETURN_STATUS
 *                           Also loop around while waiting for EOS and EOJ sts
 *  V0.009  12-Jul-1988 APK  write and close printer should be executed
 *                           even when laps_abort is true 
 *  V0.008   8-Jul-1988 APK  proc_status should not proceed if laps_abort is
 *                            true and connect is not accepted since server
 *                            brakes the link resulting in read to hang.
 *  V0.007  28-Jun-1988 APK  Implement Out_Of_Band message for interrup
 *  V0.006   3-Jun-1988 APK  close stdout if LAPS_TRACE is not defined
 *  V0.005  31-May-1988 APK  Update write_printer so that it write only
 *                             if it wouldnot block.
 *  V0.004  24-May-1988 APK  Update the LAPS_PROTOCOL version number
 *  V0.003  21-Apr-1988 APK  Update total_page_count in decode_status block
 *  V0.002  20-Apr-1988 APK  Donot read_msg when laps_abort is true
 *  V0.001  16-Mar-1988 APK  Updated open_printer to include client::username 
 *                           and job_id in printer connection block
 *
 */
#include <stdio.h>			/* standard i/o library */
#include <errno.h>			/* error numbers */
#include "laps.h"			/* laps protocol definitions */
#include "lpsmsg.h"                     /* lps message definitions */
#include "descrip.h"			/* VMS descriptor definitions */
/*
 * VMS Message file masks needed for Return_Status implementation
 */
#define MESSAGE_MASK       0xfff8
#define FACILITY_MASK      0xfff0000
#define CONTROL_MASK       0xf0000000
#define MESSAGE_OFFSET     3
#define FACILITY_OFFSET    16
#define CONTROL_OFFSET     26

#define RTNSTAT_FACILITY   (LPS$_FACILITY << FACILITY_OFFSET)
#define RTNSTAT_MESSAGE    (LPS$$CLI__RTNSTAT__BASE << MESSAGE_OFFSET)
#define RETURNSTATUS_START (RTNSTAT_FACILITY + RTNSTAT_MESSAGE + 0x8000)

#define PUT8(msgp, data) (*(FIELD8 *)(msgp)++ = ((FIELD8)data))

#ifdef vax
#define PUT16(msgp, data) (*(FIELD16 *)(msgp)++ = ((FIELD16)data))
#define INSERT8(msgp, data) (*(FIELD8 *)(msgp) = ((FIELD8)data))
#define INSERT16(msgp, data) (*(FIELD16 *)(msgp) = ((FIELD16)data))
#endif vax

#ifdef mips
#define PUT16(p, v) { FIELD16 tmp = (v); bcopy(&tmp, (p), 2); (p) += 2; }   
#define INSERT8(p, v)  { ((FIELD8 *)(p))[0] = (v); }         
#define INSERT16(p, v) { register FIELD8 *q = (FIELD8 *) (p); \
                          if ((int)q & 1) { \
                              register int w = (v); \
                              *q++ = w & 0xFF; w >>= 8; \
                              *q   = w & 0xFF; \
			  } else { \
                              *(FIELD16 *)q = (v); \
			  } \
			 }
#endif mips

#ifndef ECONNREFUSED
#define ECONNREFUSED	61
#define EMSGSIZE	40
#define ENOPROTOOPT	42
#endif
 
/*
 * buffers for sending/receiving laps messages
 */
FIELD8 xmit_buf[LAPS_BUF_SIZE];		/* transmit buffer */
FIELD8 *xmit_ptr;			/* current position in transmit buffer */
int xmit_len;				/* current amount of data in transmit buffer */
 
FIELD8 rcv_buf[LAPS_BUF_SIZE];		/* receive buffer */
FIELD8 *rcv_ptr;			/* current position in receive buffer */
int rcv_len;				/* current amount of data in receive buffer */
FIELD8 rcv_msg_type;			/* block type received */
FIELD8 rcv_blk_type;			/* block type received */
FIELD16 rcv_blk_len;			/* length of block received */
/*
 * buffers for sending laps interrupt message
 */
FIELD8 intr_buf[LAPS_BUF_SIZE];		/* interrupt buffer */
FIELD8 *intr_ptr;			/* current position in interrupt buffer */
int intr_len;				/* current amount of data in interrupt buffer */
/*
 * Resource reporting flags/count 
 */
int accepted_flag;                      /* hold onto acceptance   */
int rcount_flag;                        /* have we already receive resource count block? */
FIELD8 rrecv;                           /* current count of resource recvd */
FIELD8 rpromised;                       /* number of resources we should get */
/*
 * status variables
 */
int sock;				/* socket to server */
int trace_flag;				/* trace messages to stdout */
#define TRACE_MSGS	(1 << 0)	/* flag to trace messages (at all) */
#define TRACE_DATA	(1 << 1)	/* flag to trace data contents */
#define TRACE_ASCII	(1 << 2)	/* flag to trace data in ascii */
int laps_abort;			        /* abort current job */
int (*cond_handler)();			/* pointer to condition rec handler */
FIELD32 cur_page_cnt;                   /* current page count */
FIELD16 cur_job_state;			/* current job state */
FIELD16 cur_con_state;			/* current connection state */
FIELD16 con_accepted;                   /* boolean for connection accpeted */
struct pro_ver station_pro_ver;		/* print station protocol version */
FIELD32 station_job_id;			/* job id at print station */
struct pro_ver lo_ver = {		/* lowest version supported */
	PRO_VER_MAJOR_LO, PRO_VER_MINOR_LO, PRO_VER_EDIT_LO, PRO_VER_USER_LO };
struct pro_ver hi_ver = {		/* highest version supported */
	PRO_VER_MAJOR_HI, PRO_VER_MINOR_HI, PRO_VER_EDIT_HI, PRO_VER_USER_HI };

/*
 *		l a p s _ i n i t
 *
 * This routine called to initialize laps variables.
 * It also closes stdout, if LAPS_TRACE isn't defined
 *
 * Returns:		NULL if success
 *			EOF if fatal error
 *
 * Inputs:		None
 */
laps_init()
{
	char *cp, *getenv();
 
	/*
	 * init trace_flag to environment variable "LAPS_TRACE"
	 */
	if ((cp = getenv("LAPS_TRACE")) != NULL)
		trace_flag = atoi(cp);
        else
                close (1);
	/*
	 * reset transmit and receive buffer variables
	 */
	xmit_len = 0;
	xmit_ptr = xmit_buf;
	rcv_len = 0;
	rcv_ptr = rcv_buf;
 
	return(NULL);
}

/*
 *		o p e n _ p r i n t e r
 *
 * This routine is called to create a connection to, and start a
 * stream on a LAPS printer.
 *
 * Returns:		socket number for printer
 *			-1 if fatal error
 *
 * Inputs:
 *	server		= Node name of server
 *      user            = Name of user 
 *      userl           = Length of user name
 *      client          = Node name of client
 *      clientl         = Length of client node name
 *      job_id          = Job ID in Daemon database
 *	printer		= number of printer on server
 *
 */
open_printer(server, user, userl, client, clientl, job_id, printer)
char *server;
char *user;
int userl;
char *client;
int clientl;
int job_id;
int printer;
{
	int status;
	FIELD8 *ptr;
        FIELD8 tmp_buf[100]; /* buffer for building connect data */
        int len;
 
	/*
	 * init internal variables
	 */
	if (laps_init() != NULL)
		return(-1);
        /* 
         * init resource reporting variables
         */
         accepted_flag = rcount_flag = rrecv = rpromised = 0;
	/*
	 * connect to the specified printer
	 */
	if ((sock = conn_ps(server)) < 0)
		return(-1);
        /*
         * format the connect data
         */
         bzero(tmp_buf, sizeof(tmp_buf));
         ptr = tmp_buf;
         *((int *)ptr) = job_id;
         ptr = &tmp_buf[4];
         len = (clientl + strlen ("@") + userl);
         *((short *)ptr) = len;
         ptr = &tmp_buf[6];
         strncpy(ptr, user, userl);
         strncat(ptr, "@", strlen("@"));
         strncat(ptr, client, clientl);
	/*
	 * send a connect message, hi and low version numbers
	 */
	init_msg_hdr(LMT_CONNECT);
        insert_block(LBT_CON_DATA, tmp_buf, len + 4 + 2);
	insert_block(LBT_CON_HI_VER, &hi_ver, PRO_VER_LEN);
	insert_block(LBT_CON_LO_VER, &lo_ver, PRO_VER_LEN);
	if (flush_xmit() != NULL)
	{
		disconn_ps(sock);
		return(-1);
	}
	/*
	 * read the received status message
	 */
	cur_con_state = 0;
        con_accepted = 0;
	while (cur_con_state == 0)
	{
		if (proc_status() != NULL)
		{
			disconn_ps(sock);
			return(-1);
		}
		switch (cur_con_state)
		{
		        case 0:
                                /*
                                 * If accepted and received promised resources
                                 */
                                if (accepted_flag && (rrecv == rpromised))
				{
                                      con_accepted = 1;
                                      break;
				}
                                /*
                                 *  else assume PENDING
                                 */
			case LBV_STA_C_PENDING:
				cur_con_state = 0;
				continue;
				break;
			case LBV_STA_C_COMPLETED:
			case LBV_STA_C_ACCEPTED:
                                /*
                                 * If already received accepted message-ERROR
                                 */
                                if (accepted_flag)
				{
                                       disconn_ps(sock);
                                       errno = ECONNREFUSED;
                                       return(-1);
			        }
                                else
                                {
                                       accepted_flag = 1;
                                       if (rrecv == rpromised)
                                            con_accepted = 1;
                                       else  /* assume pending */
			               {
                                            cur_con_state = 0;
                                            continue;
                     		       }
				}
                                break;
			case LBV_STA_C_JOB_ABORT:
			case LBV_STA_C_DISABLED:
			default:
				disconn_ps(sock);
				errno = ECONNREFUSED;
				return(-1);
				break;
		}
	}
        /*
         * output null resource tag indicating end of resource reporting
         */
        report_resource(ptr, 0);   
	return(sock);
}

/*
 *		w r i t e _ p r i n t e r
 *
 * This routine is called to write data to the printer.
 *
 * Beware: Write is done using non-blocking I/O
 *
 * Returns:		0 on success
 *			-1 on error
 *
 * Inputs:
 *	buf		= Pointer to data
 *	len		= Length of data to be written
 *                        (a length of 0 indicates end-of-stream)
 *	type		= Type of data (0 takes default of PostScript)
 *
 */
write_printer(buf, len, type)
FIELD8 *buf;
int len;
FIELD8 type;
{
	int status, proceed, on = 1, off = 0;
         /*
         * loop reading status messages - until socket is ready for write
         */
        proceed = 0;
        do
	{
	    /*
	     * process any status records which may have arrived
	     */
	     if (proc_asynch() != NULL)
	     {
		    disconn_ps(sock);
		    return(-1);
	     }
             /*
              * write in nonblocking mode and check if EWOULDBLOCK
              */
             nonblock_io (sock, on);
             errno = 0;
	     status = send_data_msg(buf, len, len ? 0 : 1, LBV_DAT_PRINT_DATA, type);
             nonblock_io (sock, off);
	     switch (status)
             {
	         case NULL :
                           proceed = 0;   
                           break;
		 default :
                            if (errno == EWOULDBLOCK) {
                                sleep (2);
                                proceed = 1;
			    }
                            else
        	            {
                                disconn_ps(sock);
                                return(-1);
	                    }
	      }
	} while (proceed == 1);
	/*
         * if the length was 0, then this is a "end-of-stream" request.
	 * process the expected status.
	 */
        if (len == 0)
	{
                cur_con_state = 0;
                while (cur_con_state == 0)
		{
	               if (proc_status() != NULL)
	               {
                               fprintf (stderr, "Olpscomm: error from process_status: eos sent\n");
		               disconn_ps(sock);
		               return(-1);
	               }
	               switch (cur_con_state)
	               {
		                case 0:
                                     continue;
                                     break;
		                case LBV_STA_C_COMPLETED:
			              break;
			        case LBV_STA_C_JOB_ABORT:
                                      /*
                                       * if abort is reported then this is okay
                                       * otherwise, it's an error
                                       */
                                      if (laps_abort)
                                            break;
		                 default:
			              errno = EIO;
			              disconn_ps(sock);
			              return(-1);
	                }
	       }
	}
	return(NULL);
}

/*
 *		c l o s e _ p r i n t e r
 *
 * This routine is called to terminate access to a printer.
 *
 * Returns:		0 on success
 *			-1 on failure
 *
 * Inputs:
 *	abort		= 0 if not aborting print job
 *			= not 0 if aborting
 */
close_printer(abort)
int abort;
{
	FIELD8 reason;
 
	/*
	 * process any status records which may have arrived
	 */
	if (proc_asynch() != NULL)
	{
		disconn_ps(sock);
		return(-1);
	}
	/*
	 * set close reason
	 */
	if (abort)
		reason = LBV_EOJ_ABNORMAL;
	else
		reason = LBV_EOJ_OK;
	/*
	 * send an EOJ message
	 */
	if (send_eoj_msg(reason) != NULL)
	{
		disconn_ps(sock);
		return(-1);
	}
	/*
	 * wait for, and process status message
	 */
        cur_con_state = 0;
        while (cur_con_state == 0)
	{
	        if (proc_status() != NULL)
	        {
                        fprintf (stderr, "Olpscomm: error from process_status: eoj sent\n");
		        disconn_ps(sock);
		        return(-1);
	        }
	        switch (cur_con_state)
	        {
                        case 0:
                               continue;
                               break;
		        case LBV_STA_C_RELEASED:
			       break;
		        case LBV_STA_C_JOB_ABORT:
		               /*
		                * job aborted - if we asked for it, this is ok,
		                * otherwise, it's an error
		                */
		               if (abort)
		         	    break;
		        default:
			       disconn_ps(sock);
			       return(-1);
			       break;
	        }
        }
	disconn_ps(sock);
	return(0);
}

/*
 *            i n t e r r u p t _ p r i n t e r
 *
 * This routine is called to send OOB interrupt message to printer.
 *
 * Returns:      NULL on sccess
 *               -1 on error
 *
 * Inputs:       None
 *
 */
interrupt_printer()
{
      FIELD8 type;
 
      /*
       * build a control message
       */
      init_interrupt_msg_hdr(LMT_CONTROL);
      /*
       * insert the data description and data type
       */
      type = LBV_CTL_INTERRUPT;
      insert_interrupt_block(LBT_CTL_CMD, &type, LBL_CTL_CMD);
      /*
       * send the interrupt message off 
       */
      if (flush_interrupt() != NULL)
      {     
             disconn_ps (sock);
             return (-1);
      }
 
      return (NULL);
}

/*
 *		s e t _ c o n d _ h a n d l e r
 *
 * This routine is called to define a condition record handler.
 *
 * Returns:		None
 *
 * Inputs:
 *	handler		= Address of handler,
 *			  or NULL to cancel handler
 */
set_cond_handler(handler)
int (*handler)();
{
	cond_handler = handler;
}

/*
 *		i n i t _ m s g _ h d r
 *
 * This routine is called to initialize the header of a new message to be
 * built. The message type is inserted, and the block count and length
 * fields are set to 0.
 *
 * Returns:		NULL if success,
 *			EOF if error flushing previous buffer contents
 *
 * Inputs:
 *	msg_type	Type of message being built
 */
init_msg_hdr(msg_type)
FIELD8 msg_type;
{
	struct msg_hdr *msg_ptr;
	/*
	 * if there is data currently in the xmit buffer, send it
	 */
	if (xmit_len != 0)
	{
		if (flush_xmit() != NULL)
			return(EOF);
	}
	/*
	 * init the message header
	 */
	msg_ptr = (struct msg_hdr *)xmit_buf;
	msg_ptr->msg_type = msg_type;
	msg_ptr->msg_blkcnt = 0;
	msg_ptr->msg_len = 0;
 
	xmit_ptr = xmit_buf + MSG_HDR_LEN;
	xmit_len = MSG_HDR_LEN;
 
	return(NULL);
}

/*
 *		i n i t _ i n t e r r u p t _ m s g _ h d r
 *
 * This routine is called to initialize the interrupt header of a control 
 * message to be built. The message type is inserted, and the block count and 
 * length fields are set to 0.
 *
 * Returns:		NULL
 *
 * Inputs:
 *	msg_type	Type of message being built
 */
init_interrupt_msg_hdr(msg_type)
FIELD8 msg_type;
{
	struct msg_hdr *msg_ptr;
	/*
	 * init the message header
	 */
	msg_ptr = (struct msg_hdr *)intr_buf;
	msg_ptr->msg_type = msg_type;
	msg_ptr->msg_blkcnt = 0;
	msg_ptr->msg_len = 0;
 
	intr_ptr = intr_buf + MSG_HDR_LEN;
	intr_len = MSG_HDR_LEN;
 
	return(NULL);
}

/*
 *		i n s e r t _ b l o c k
 *
 * This routine is called to build a block within a message. If there 
 * is not enough room, it flushes the current message, and starts a new one.
 * It builds the block header, and copies the data supplied into the block.
 *
 * Returns:		NULL on success
 *			EOF on fatal error flushing the previous message
 *
 * Inputs:
 *	type		Type of block being built
 *	data		Data to be inserted into block
 *	len		Length of block to be built
 */
insert_block(type, data, len)
FIELD8 type;
FIELD8 *data;
FIELD16 len;
{
	struct msg_hdr *msg_ptr;
	struct blk_hdr *blk_ptr;
 
	msg_ptr = (struct msg_hdr *)xmit_buf;
	/*
	 * check to see if the message being built is large enough to
	 * hold the requested block - if not flush the current message
	 * and start a new one of the same type
	 */
	if ((xmit_len + BLK_HDR_LEN + len) > sizeof(xmit_buf))
	{
		if (xmit_len == 0)	/* block is just too big */
		{
			errno = EMSGSIZE;
			return(EOF);
		}
		if (init_msg_hdr(msg_ptr->msg_type) != NULL)
			return(EOF);
	}
	/*
	 * build the block header
	 */
	blk_ptr = (struct blk_hdr *)xmit_ptr;
	blk_ptr->blk_type = type;
	INSERT16(blk_ptr->blk_len, len);
	/*
	 * insert the block data
	 */
	bcopy(data, (xmit_ptr + BLK_HDR_LEN), len);
	/*
	 * update the message header
	 */
	msg_ptr->msg_blkcnt +=1;
	msg_ptr->msg_len += (len + BLK_HDR_LEN);
	/*
	 * update the transmit message pointer and length
	 */
	xmit_ptr += (len + BLK_HDR_LEN);
	xmit_len += (len + BLK_HDR_LEN);
 
	return(NULL);
}

/*
 *		i n s e r t _ i n t e r r u p t _ b l o c k
 *
 * This routine is called to build a interrupt block within a message.If there 
 * is not enough room, it flushes the current message, and starts a new one. 
 * It builds the block header, and copies the data supplied into the block.
 *
 * Returns:		NULL on success
 *			EOF on fatal error flushing the previous message
 *
 * Inputs:
 *	type		Type of block being built
 *	data		Data to be inserted into block
 *	len		Length of block to be built
 */
insert_interrupt_block(type, data, len)
FIELD8 type;
FIELD8 *data;
FIELD16 len;
{
	struct msg_hdr *msg_ptr;
	struct blk_hdr *blk_ptr;
 
	msg_ptr = (struct msg_hdr *)intr_buf;
	/*
	 * check to see if the message being built is large enough to
	 * hold the requested block - if not flush the current message
	 * and start a new one of the same type
	 */
	if ((intr_len + BLK_HDR_LEN + len) > sizeof(intr_buf))
	{
		if (intr_len == 0)	/* block is just too big */
		{
			errno = EMSGSIZE;
			return(EOF);
		}
		if (init_interrupt_msg_hdr(msg_ptr->msg_type) != NULL)
			return(EOF);
	}
	/*
	 * build the block header
	 */
	blk_ptr = (struct blk_hdr *)intr_ptr;
	blk_ptr->blk_type = type;
	INSERT16(blk_ptr->blk_len, len);
	/*
	 * insert the block data
	 */
	bcopy(data, (intr_ptr + BLK_HDR_LEN), len);
	/*
	 * update the message header
	 */
	msg_ptr->msg_blkcnt +=1;
	msg_ptr->msg_len += (len + BLK_HDR_LEN);
	/*
	 * update the transmit message pointer and length
	 */
	intr_ptr += (len + BLK_HDR_LEN);
	intr_len += (len + BLK_HDR_LEN);
 
	return(NULL);
}

/*
 *		f l u s h _ x m i t
 *
 * This routine is called to flush the transmit buffer and reset the
 * buffer pointer and length.
 *
 * Returns:		NULL if success
 *			EOF on error
 *
 * Inputs:		none
 */
flush_xmit()
{
	int status;
 
	status = write_ps(sock, xmit_buf, xmit_len);
	xmit_len = 0;
	if (status < 0)
		return(EOF);
	if (trace_flag)
		trace_xmit(xmit_buf);
	return(NULL);
}

/*
 *		f l u s h _ i n t e r r u p t
 *
 * This routine is called to flush the interrupt buffer and reset the
 * buffer pointer and length.
 *
 * Returns:		NULL if success
 *			EOF on error
 *
 * Inputs:		none
 */
flush_interrupt()
{
	int status;
 
	status = interrupt_ps(sock, intr_buf, intr_len);
	intr_len = 0;
	if (status < 0)
		return(EOF);
	if (trace_flag)
		trace_xmit(intr_buf);
	return(NULL);
}

/*
 *		r e a d _ m s g
 *
 * This routine is called to read in the next message.
 *
 * Returns		Message type read
 *			EOF on fatal error
 *
 *	rcv_msg_type	= message type received
 *
 * Inputs:		None
 */
FIELD8 read_msg()
{
	rcv_len = read_ps(sock, rcv_buf, sizeof(rcv_buf));
	if (rcv_len <= 0)
	{
		rcv_len = 0;
		return(EOF);
	}
	if (trace_flag)
		trace_rcv(rcv_buf);
	rcv_msg_type = ((struct msg_hdr *)rcv_buf)->msg_type;
	rcv_ptr = rcv_buf + MSG_HDR_LEN;
	rcv_len -= MSG_HDR_LEN;
	if (rcv_len != ((struct msg_hdr *)rcv_buf)->msg_len)
	{
		rcv_len = 0;
		errno = EMSGSIZE;
		return(EOF);
	}
	return(rcv_msg_type);
}

/*
 *		r e a d _ b l o c k
 *
 * This routine is called to get a block from a received message.
 *
 * Returns:		Pointer to data portion of block read
 *			NULL if no more blocks in current message
 *	rcv_msg_type	= message type
 *	rcv_blk_type	= block type
 *	rcv_blk_len	= length of block
 *
 * Inputs:		None
 */
FIELD8 *read_block()
{
	int status;
	struct msg_hdr *msg_ptr;
	struct blk_hdr *blk_ptr;
        FIELD16 *tmp;
	msg_ptr = (struct msg_hdr *)rcv_buf;
	/*
	 * if no blocks in buffer
	 */
	if (rcv_len == 0)
		return(NULL);
	/*
	 * get the address of the next block, and update the message pointers
	 * and length
	 */
	blk_ptr = (struct blk_hdr *)rcv_ptr;
       /*	
        *  To correct unalinged access on MIPS, USE tmp, instead of 
        *  the following!!:
        *      rcv_blk_len = *(FIELD16 *)(blk_ptr->blk_len); 
        */
        tmp = (FIELD16 *)(blk_ptr->blk_len);         
	bcopy (tmp, &rcv_blk_len, 2);

	rcv_blk_type = blk_ptr->blk_type;
	msg_ptr->msg_blkcnt--;
	rcv_ptr += (rcv_blk_len + BLK_HDR_LEN);
	rcv_len -= (rcv_blk_len + BLK_HDR_LEN);
 
	if (msg_ptr->msg_blkcnt == 0)
		rcv_len = 0;
	return(((FIELD8 *)blk_ptr) + BLK_HDR_LEN);
}

/*
 *		p r o c _ s t a t u s
 *
 * This function is called to process a status message. The contents
 * of each block is stored in the appropriate variable.
 *
 * Returns:		NULL if success
 *			EOF if error
 *
 * Inputs:		NONE
 */
proc_status()
{
	FIELD8 *ptr;
	FIELD8 mtype;
	int got_status = 0;
	int got_data = 0;
 
        /*
         *  If laps_abort is set and connection is not accepted then
         *  return error to disconnect the link. Actually server should
         *  disconnect - BUT there is a discripancy in LAPS protocol!!
         *
         */
 
	while (!got_status)
	{
                 if (laps_abort && !con_accepted)
                         return(EOF);
          
 		while ((mtype = read_msg()) != LMT_STATUS)
		{
			if (mtype == (FIELD8)EOF)
				return(EOF);
		}
 
		got_data = 0;
		while ((ptr = read_block()) != NULL)
		{
			if (rcv_blk_type == LBT_STA_DATA)
				got_data = 1;
			if (decode_status_block(rcv_blk_type, ptr, rcv_blk_len) != NULL)
				return(EOF);
		}
		if (!got_data)
			got_status = 1;
	}
	return(NULL);
}

/*
 *		p r o c _ a s y n c h
 *
 * This function is called to process any messages which arrived
 * asynchronously. These are expected to be condition records delivered
 * as status messages.
 *
 * Returns:		NULL if success
 *			EOF if error
 *
 * Inputs:		NONE
 */
proc_asynch()
{
	FIELD8 *ptr;
	FIELD8 mtype;
	int status;
 
	while (poll_msg(sock) != NULL)
	{
		mtype = read_msg();
		if (mtype == (FIELD8)EOF)
			return(EOF);
 
		switch (mtype)
		{
			case LMT_STATUS:
				while ((ptr = read_block()) != NULL)
				{
					if (decode_status_block(rcv_blk_type, ptr, rcv_blk_len) != NULL)
						return(EOF);
				}
				break;
			default:
				break;
		}
	}
	return(NULL);
}

/*
 *		d e c o d e _ s t a t u s _ b l o c k
 *
 * This routine is called to store information returned in a status block.
 *
 * Returns:		NULL on success
 *			EOF on error
 *
 * Inputs:
 *	type		= Block type
 *	ptr		= Pointer to data in block
 *	len		= Length of data in block
 *
 *  NOTE:      
 *  To correct unalinged access problem on MIPS, USE tmp, instead of 
 *  referencing struct directly (see code below between "!!"):
 *
 */
decode_status_block(type, ptr, len)
FIELD8 type;
FIELD8 *ptr;
int len;
{
        extern FIELD32 total_page_cnt;            /* total page count */
	int status = NULL;
 
	if (len == 0)
		return(NULL);	/* ignore 0 length blocks */
	errno = ENOPROTOOPT;	/* assume protocol error */
	switch (type)
	{
		case LBT_STA_PAGE_CNT:
			if (len != LBL_STA_PAGE_CNT)
				status = EOF;
			else {
                             FIELD32 *tmp;
                             tmp = (FIELD32 *)ptr;
                             bcopy (tmp, &cur_page_cnt, 4);

			   /*!!  cur_page_cnt = *(FIELD32 *)ptr; !!*/
                             total_page_cnt += cur_page_cnt;
			}
			break;
		case LBT_STA_JOB_STATE:
			if (len != LBL_STA_JOB_STATE)
				status = EOF;
			else {
                                FIELD16 *tmp;
                                tmp = (FIELD16 *)ptr;
                                bcopy (tmp, &cur_job_state, 2);

				/*!! cur_job_state = *(FIELD16 *)ptr; !!*/
			      }
			break;
		case LBT_STA_CON_STATE:
			if (len != LBL_STA_CON_STATE)
				status = EOF;
			else {
                                FIELD16 *tmp;
                                tmp = (FIELD16 *)ptr;
                                bcopy (tmp, &cur_con_state, 2);

				/*!! cur_con_state = *(FIELD16 *)ptr; !!*/
			      }
			break;
		case LBT_STA_PRO_VER:
			if (len != LBL_STA_PRO_VER)
				status = EOF;
			else
				bcopy(ptr, &station_pro_ver, PRO_VER_LEN);
			break;
		case LBT_STA_JOB_ID:
			if (len != LBL_STA_JOB_ID)
				status = EOF;
			else
				station_job_id = *(FIELD32 *)ptr;
			break;
		case LBT_STA_DATA:
			proc_cond_rec(ptr, len);
			break;
		case LBT_STA_RES_AVAIL:
                        if (len > LBL_STA_RES_AVAIL)
                                status = EOF;
                        else
                                if ((cur_con_state != LBV_STA_C_ACCEPTED) &&
                                    (!accepted_flag))
                                         status = EOF;
                        else
			{
                                report_resource(ptr, len);
                                rrecv++;
			}
			break;		  
		case LBT_STA_RES_COUNT:   /* resource promised block */
                        if (len != LBL_STA_RES_COUNT)
                                status = EOF;
                        else /* res. count is already recd OR accpted-ERROR */
			{
                                if (rcount_flag || 
                                    (cur_con_state != LBV_STA_C_ACCEPTED))
                                         status = EOF;
                                else  /* more than 1 count */
				{
                                    rcount_flag = 1;
                                    rpromised = *(FIELD8 *)ptr;
				}
			}
                        break;                
		case LBT_STA_RETURNSTATUS:
                        ((struct cond_hdr *)ptr)->msg_code +=
                                                  RETURNSTATUS_START;
                        if (((((struct cond_hdr *)ptr)->msg_code) &
                                  FACILITY_MASK) > RTNSTAT_FACILITY)
			{
                             ((struct cond_hdr *)ptr)->msg_code = LPS$_UNKMSG;
                             ((struct cond_hdr *)ptr)->argc = 0;
			}
			proc_cond_rec(ptr, len);
                        break;                            
		default:
			break;
	}
	return(status);
}

/*
 *		s e n d _ d a t a _ m s g
 *
 * This routine is called to send a data message to a printer
 *
 * Returns:		NULL on success
 *			EOF on failure
 *
 * Inputs:
 *	buf		= Pointer to data to be written
 *	len		= Length of data
 *	eos		= End-of-stream flag
 *	desc		= Description of data
 *	type		= Type of data
 */
send_data_msg(buf, len, eos, desc, type)
FIELD8 *buf;
int len;
int eos;
FIELD8 desc;
FIELD8 type;
{
	FIELD8 *ptr;
	FIELD8 flags;
 
	init_msg_hdr(LMT_DATA);
	/*
	 * insert message flags if needed for end-of-stream
	 */
	if (eos)
	{
		flags = LBV_DAT_EOD;
		insert_block(LBT_DAT_MSGFLGS, &flags, LBL_DAT_MSGFLGS);
	}
	/*
	 * insert the data description
	 */
	if (desc == 0)
		desc = LBV_DAT_PRINT_DATA;
	insert_block(LBT_DAT_DESC, &desc, LBL_DAT_DESC);
	if (desc == LBV_DAT_PRINT_DATA)
	{
		/*
		 * default print data type to postscript
		 */
		if (type == 0)
			type = 14;
		insert_block(LBT_DAT_TYPE, &type, LBL_DAT_TYPE);
	}
	if (desc == LBV_DAT_RESOURCE_DATA)
	{
		/*
		 * default resource data type to "font"
		 */
		if (type == 0)
			type = 1;
		insert_block(LBT_DAT_TYPE, &type, LBL_DAT_TYPE);
	}
	/*
	 * insert the actual data, and send the message off
	 */
	insert_block(LBT_DAT_DATA, buf, len);
	return(flush_xmit());
}

/*
 *		s e n d _ e o j _ m s g
 *
 * This routine is called to send an end-of-job message to a printer
 *
 * Returns:		NULL on success
 *			EOF on failure
 *
 * Inputs:
 *	reason		= Reason job is being ended
 */
send_eoj_msg(reason)
FIELD8 reason;
{
	init_msg_hdr(LMT_EOJ);
	/*
	 * insert reson for eoj
	 */
	insert_block(LBT_EOJ_REASON, &reason, LBL_EOJ_REASON);
	/*
	 * send the message off
	 */
	return(flush_xmit());
}

/*
 * LAPS trace data
 */
char *trc_msg_types[] =
{
	"UNKNOWN",
	"CONNECT",
	"DATA",
	"CONTROL",
	"END-OF-JOB",
	"STATUS",
	"RESOURCE FAULT",
	"RESOURCE STATUS"
};
 
char *trc_conn_blks[] =
{
	"UNKNOWN",
	"HIGH VERSION",
	"DATA",
	"LOW VERSION",
	"PRINTER"
};
 
char *trc_data_blks[] =
{
	"UNKNOWN",
	"DESCRIPTION",
	"TYPE",
	"DATA",
	"FLAGS"
};
 
char *trc_ctl_blks[] =
{
	"UNKNOWN",
	"COMMAND",
	"DATA",
	"LOW VERSION",
	"RESOURCE SERVER"
};
 
char *trc_eoj_blks[] =
{
	"UNKNOWN",
	"REASON",
	"DATA"
};
 
char *trc_sta_blks[] =
{
	"UNKNOWN",
	"PAGE COUNT",
	"JOB STATE",
	"CONNECTION STATE",
	"PROTOCOL VERSION",
	"JOB ID",
	"DATA",
	"UPLINE DATA",
	"LOW VERSION",
	"HIGH VERSION"
};
 
char *trc_rf_blks[] =
{
	"UNKNOWN",
	"NAME",
	"TYPE",
	"LOW VERSION",
	"HIGH VERSION"
};
 
char *trc_rs_blks[] =
{
	"UNKNOWN",
	"DISPOSITION",
	"STATUS",
	"SUBSTITUTE FILE",
	"SUBSITUTE SPECIFICATION",
	"LOW VERSION",
	"HIGH VERSION",
	"RESOURCE VERSION"
};
 
char **trc_blk_tbls[] =
{
	NULL,
	trc_conn_blks,
	trc_data_blks,
	trc_ctl_blks,
	trc_eoj_blks,
	trc_sta_blks,
	trc_rf_blks,
	trc_rs_blks
};
 
int trc_blk_limits[] =
{
	0,
	sizeof(trc_conn_blks) / sizeof(trc_conn_blks[0]),
	sizeof(trc_data_blks) / sizeof(trc_data_blks[0]),
	sizeof(trc_ctl_blks) / sizeof(trc_ctl_blks[0]),
	sizeof(trc_eoj_blks) / sizeof(trc_eoj_blks[0]),
	sizeof(trc_sta_blks) / sizeof(trc_sta_blks[0]),
	sizeof(trc_rf_blks) / sizeof(trc_rf_blks[0]),
	sizeof(trc_rs_blks) / sizeof(trc_rs_blks[0])
};

/*
 *		t r a c e _ r c v
 *
 * This routine is called to trace a LAPS message received.
 *
 * Returns:		Nothing
 *
 * Inputs:
 *	msgp		= Pointer to message
 */
trace_rcv(msgp)
register FIELD8 *msgp;
{
	trace_msg(msgp, "->");
}

/*
 *		t r a c e _ x m i t
 *
 * This routine is called to trace a LAPS message transmitted.
 *
 * Returns:		Nothing
 *
 * Inputs:
 *	msgp		= Pointer to message
 */
trace_xmit(msgp)
register FIELD8 *msgp;
{
	trace_msg(msgp, "<-");
}

/*
 *		t r a c e _ m s g
 *
 * This routine is called to format and print a LAPS message.
 *
 * Returns:		None
 *
 * Inputs:
 *	msgp		= Pointer to message
 *	direction	= Pointer to string to indicate "received" or "transmit"
 */
trace_msg(msgp, direction)
FIELD8 *msgp;
char *direction;
{
	struct msg_hdr *msg_ptr;
	int i;
	int len;
	FIELD8 *ptr;
	FIELD8 msg_type;
	char tmpbuf[10];
	char *msg_string;
 
	msg_ptr = (struct msg_hdr *)msgp;
	msg_type = msg_ptr->msg_type;
	if (msg_type > (sizeof(trc_msg_types)/sizeof(trc_msg_types[0])))
	{
		sprintf(tmpbuf, "%d", msg_type);
		msg_type = 0;
		msg_string = tmpbuf;
	} else {
		msg_string = trc_msg_types[msg_type];
	}
 
	printf("\n%s %s message (%d blocks), length = %d\n",
	direction, msg_string, msg_ptr->msg_blkcnt, 
	msg_ptr->msg_len);
	ptr = msgp + MSG_HDR_LEN;
	for (i = msg_ptr->msg_blkcnt; i; i--)
	{
		trace_blk(ptr, msg_type);
		len = *(FIELD16 *)(((struct blk_hdr *)ptr)->blk_len);
		ptr += (len + BLK_HDR_LEN);
	}
}

/*
 *		t r a c e _ b l k
 *
 * This routine is called to format and print a LAPS block
 *
 * Returns:		Nothing
 *
 * Inputs:
 *	blkp		= Pointer to block
 *	msg_type	= Type of message block was in
 */
trace_blk(blkp, msg_type)
FIELD8 *blkp;
FIELD8 msg_type;
{
	int i;
	int len;
	int cur_x;
	int bol, eol;
	unsigned char c;
	char tmpbuf[20];
	char *blk_string;
	FIELD8 *datap = blkp + BLK_HDR_LEN;
	struct blk_hdr *blk_ptr = (struct blk_hdr *)blkp;
	FIELD8 blk_type = blk_ptr->blk_type;
 
	if (blk_type > trc_blk_limits[msg_type])
	{
		sprintf(tmpbuf, "%d", blk_type);
		blk_type = 0;
		blk_string = tmpbuf;
	} else {
		blk_string = trc_blk_tbls[msg_type][blk_type];
	}
	len = *(FIELD16 *)(blk_ptr->blk_len);
	printf("   %s block, length = %d\n", blk_string, len);
 
	if ((msg_type != LMT_DATA) || (blk_type != LBT_DAT_DATA) ||
	(trace_flag & TRACE_DATA))
	{
		if (len != 0)
		{
			for ( i = 0 ; i < len ; i++ )
			{
				cur_x = i % 16;
				if (cur_x == 0)
				{
					bol = 1;
					bzero(tmpbuf, sizeof(tmpbuf));
				} else
					bol = 0;
				if ((cur_x == 15) || ((i + 1) == len))
					eol = 1;
				else
					eol = 0;
 
				c = *datap++ & 0xff;
				printf("%s%03u%s", bol ? "     " : " ",
				c, eol ? "\n" : "");
 
				if (trace_flag & TRACE_ASCII)
				{
					tmpbuf[cur_x] = ((c > 037) ? c : '.');
					if (eol)
						printf("     %s\n", tmpbuf);
				}
			}
		}
	}
}
