#ifndef lint
static char SccsId[]="@(#)main.c	4.1\tLPS_ULT_DNU\t7/2/90";
#endif
/*
 *    main.c            LAPS filter main programm
 *    
 *    03-Mar-1988   Created - Ajay Kachrani
 *   
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
 *
 *    EDIT HISTORY
 *    -----------
 *
 *    V1.000  5-May-1988   APK  Catch all signals including SIGINT!
 *                              --Fix half to the QAR#4 (other half
 *                                is being fixed by EUEG!
 *-----------------------Begin V2.0-----------------------------------------
 *    V0.015  13-Oct-1988  APK  Output a/c infor only if job accepted
 *    V0.014  12-Oct-1988  APK  Implement line_oriented_protocol in msg output
 *    V0.013   7-Sep-1988  APK  When laps_abort is true;exit with EX_ERR
 *    V0.012  25-Aug-1988  APK  Add job retry by using exit status 1.
 *    V0.011  28-Jul-1988  APK  Update resource reporting format
 *    V0.010  18-Jul-1988  APK  Add report_reource routine to o/p resource tag
 *    V0.009  11-Jul-1988  APK  Send eos when laps_abort is true 
 *    V0.008  28-Jun-1988  APK  Invoke interrupt_printer from the intr_handler
 *    V0.007   8-Jun-1988  APK  Set nonblock_io OFF from intr_handler
 *    V0.006  22-Apr-1988  APK  Send eos from the intr_handler
 *    V0.005  21-Apr-1988  APK  Added macro to print accounting-sheets printed
 *    V0.004  20-Apr-1988  APK  Added handler to print messages for operator
 *                              and user prefix with U and O resp.
 *    V0.003  15-Apr-1988  APK  Updated call interface with daemon
 *    V0.002  16-Mar-1988  APK  Added SIGINT handler
 *    V0.001  03-Mar-1988  APK  Created   
 *    
 */
#include <stdio.h>		/* standard i/o definitions */
#include <signal.h>             /* software signal definitions */
#include <string.h>             /* string/char. handling definitions */
#include "laps.h"               /* laps definitions */
/*
 * define exit codes
 */
#define EX_SUC	 0
#define EX_RETRY 1
#define EX_ERR	 2
/*
 *  macro definitions
 */
#define acct(page_count) fprintf(stderr,"Asheets printed %u\n",page_count)

int abort_flag = 0;		/* flag to indicate we are aborting the print job */
void intr_handler();            /* interrupt handler for SIGINT */
int print_msg();                /* conditional handler to print U and O messages */
char *server_node;              /* laps-arg1 printserver node name */
char *user_name;                /* laps-arg2 username */
char *client_node;              /* laps-arg3 client node name */
char *client_job_id;            /* laps-arg4 job id in lpd database */
FIELD32  total_page_cnt = 0;    /* total page count */
char *msg_ptr;                  /* pointer to message buffer */

/*
 *		m a i n
 *
 * The flow of control is:
 *	1. connect to print server specified in the argument list
 *	2. read data from stdin and write it in LAPS format to
 *	   the printer
 *	3. close the printer
 */
main(argc, argv)
int argc;
char *argv[];
{
	int status;
	int len;
	char read_buf[MAX_DAT_SIZE];
        int ret_ptr;
        extern int laps_abort;
   
	/*
	 * parse the arguments 
	 */
	if (argc < 5)
	{
		fprintf(stderr, "Olpscomm: error: Usage: lpscomm server_node user_name client_node client_job_id\n");
		exit(EX_ERR);
	}
	server_node = argv[1];
        user_name = argv[2];
        client_node = argv[3];
        client_job_id = argv[4];
	/*
	 * if fifth arg supplied, use it as input file name
	 */
	if (argc > 5)
	{
		if (freopen(argv[5], "r", stdin) == NULL)
		{
			sprintf(read_buf, "Olpscomm: error: can't open file %s", argv[5]);
			print_error(read_buf);
		}
	}
        /*
         * catch signals 
         */
        signal (SIGINT, intr_handler);
        signal (SIGHUP, intr_handler);
        signal (SIGQUIT, intr_handler);
        signal (SIGILL, intr_handler);
        signal (SIGEMT, intr_handler);
        signal (SIGBUS, intr_handler);
        signal (SIGSEGV, intr_handler);
        signal (SIGSYS, intr_handler);
        signal (SIGTERM, intr_handler);
        signal (SIGPIPE, intr_handler);
        /*
         *    establish a conditional handler for laps status messages
         */
         set_cond_handler (print_msg);
	/*
	 * "open" the specified printer
	 */
	if (open_printer(server_node, user_name, strlen(user_name),
                         client_node, strlen(client_node), atoi(client_job_id), 0) < 0)
		print_error ("Olpscomm: error: can't access printer");
	/*
	 * read data from stdin, and write it to the printer
	 */
    	while (((len = read(0, read_buf, sizeof(read_buf))) > 0) && (laps_abort == 0))
		if (write_printer(read_buf, len, 0) < 0)
			print_error("Olpscomm: error sending to printer");
        /*
         * send end_of_stream and close the link
         */
	if ((write_printer(read_buf, 0, 0) < 0) || (close_printer(abort_flag) < 0))
		print_error("Olpscomm: error closing printer");

        /*
         *  If laps_abort is true then exit with EX_ERR. This is possible,
         *  while writing! write_printer and close_printer continue to
         *  function even when the "laps_abort" is true.
         */
        if (laps_abort == 1)
               print_error ("Ulpscomm: abort was initiated by print server.");

        acct(total_page_cnt);
	exit(EX_SUC);
}

/*
 *		p r i n t _ e r r o r
 *
 * This routine is called to print an error message and exit. The error code is contained
 * in "errno". If the external int "laps_abort" havs been set, no error message will be
 * printed, since it is assumed that a condition message has already been seen.
 *
 * This routine also output accounting information, if connect at the printer
 * was accepted.
 *
 * Non-aborting job in abnormal state will exit with EX_RETRY status,
 * indicating lpd that the job should go through job retry.
 *
 * Returns:		No return from this routine
 *
 * Inputs:
 *	laps_abort	= non-0 if aborting as a result of a previous condition record
 *	txt		= text string to insert in message
 */
print_error(txt)
char *txt;
{
	extern int laps_abort;
        extern FIELD16 con_accepted;
        extern FIELD32 total_page_cnt;     

	if (laps_abort == 0)
		perror(txt);

        if (con_accepted)
                acct(total_page_cnt);

        if ((laps_abort == 0) && (abort_flag == 0))
        	exit(EX_RETRY);
        else
                exit(EX_ERR);

}

/*
 *              intr_handler
 *
 * This routine is called when job entry is deleted using lprm or lpd
 * sending SIGINT signal to abort the job. This routine sends 
 * end-of-stream and close printer if any active link to a printer.
 *
 * Implicit_Inputs:     cur_con_state  = current connection state
 *                      sock           = DECnet socket
 *                      total_page_cnt = Total Page Count
 * 
 * Returns:    No returns from this routine
 *
 */
static void intr_handler(sigtype)
int sigtype;
{ 
     static char *sigNames[] = {
         "SIGHUP", "SIGINT", "SIGQUIT", "SIGILL", "SIGTRAP",
         "SIGIOT", "SIGEMT", "SIGFPE", "SIGKILL", "SIGBUS",
         "SIGSEGV", "SIGSYS", "SIGPIPE", "SIGALRM", "SIGTERM"
	 };
      char *signame;
      extern FIELD32 total_page_cnt;
      extern FIELD16 con_accepted;
      extern int sock;
      int off = 0;
      char eos_buf;

      if (sigtype == SIGINT) 
           fprintf (stderr, "Ulpscomm: %s: Job %u has been interrupted during execution\n", server_node, atoi(client_job_id));      
      else {
           if (sigtype > 0 && sigtype <= SIGTERM)
                signame = sigNames[sigtype-1];
           else
                signame = "Unknown";
           fprintf (stderr, "Ulpscomm: %s: signal  %d  (%s) received. Aborting Job %u.\n", 
                server_node, sigtype, signame, atoi(client_job_id));
           fprintf (stderr, "Olpscomm: %s: signal  %d  (%s) received. Aborting Job %u.\n", 
                 server_node, sigtype, signame, atoi(client_job_id));

      }
      abort_flag = 1;

      nonblock_io (sock, off);

      switch (con_accepted)
      {

            case 1:
                 /*
                  *  link is ON and the job is active at server 
                  *
                  *  first, send expedite abort
                  */
                 if (interrupt_printer () != NULL)
		 {
                        disconn_ps(sock);
                        print_error("Olpscomm: error sending interrupt");
                        break;
		 }
                 /*
                  *  now, send eos and close_printer
                  */
                 if ((write_printer(eos_buf, 0, 0) < 0) 
                                || (close_printer(abort_flag) < 0))
                       print_error("Olpscomm: error closing printer");

                 break;

	    default:
                 /*
                  * If socket is opened then disconnect
                  */
                 if (sock > 0)
                        disconn_ps(sock);
                 break;
       }
      if (con_accepted)
           acct(total_page_cnt);

      exit (EX_SUC);
}

/*
 *              p r i n t _ m s g 
 *
 *    This routine is called to process condition messages to 
 *    write on stderr using "Line oriented protocol";
 * 
 *    i.e. From the message buffer extract line by line and 
 *         use output_msg to o/p the buffer.
 *
 *
 *    Inputs:
 *         route        = routing information
 *         msg_buf      = message buffer containg message to be outputed
 *         msg_len      = length of the message in the message buffer
 *
 */
print_msg(route, msg_buf, msg_len)
FIELD8 route;
char *msg_buf;
int msg_len;
{
     char out_buf[MAX_COND_REC_SIZE];  /* buffer ready to output on stderr */
     int ftag;                         /* specify to output full_tag or not */

     msg_ptr = msg_buf;

     strcpy(out_buf,"");
     
     getline(out_buf);
  
     output_msg(route, out_buf, (ftag=1));

     if (msg_ptr == 0)
           return;

     getline(out_buf);

     while (msg_ptr != 0) {
           output_msg(route, out_buf, (ftag=0));
           getline(out_buf);
     }

output_msg(route, out_buf, (ftag=0));
return;

}

/*
 *              o u t p u t _ m s g 
 *
 *    This routine is called to output condition messages to stderr.
 *
 *               adds U for User prefix,
 *               ands O for Operator prefix
 *
 *    Inputs:
 *         route        = routing information
 *         msg_buf      = message buffer containg message to be outputed
 *         ftag         = specify to output full_tag or not
 *
 */
output_msg(route, msg_buf, ftag)
FIELD8 route;
FIELD8 *msg_buf;
int ftag;
{

     if (route & LAPS_ROU_USER) {
         if (ftag==1)
            fprintf (stderr, "Ulpscomm: %s: %s\n", server_node, msg_buf);
         else
            fprintf (stderr, "U%s\n", msg_buf);
     }
     if (route & LAPS_ROU_OPERATOR) {
         if (ftag==1)
             fprintf (stderr, "Olpscomm: %s: %s\n", server_node, msg_buf);
         else
             fprintf (stderr, "O%s\n",msg_buf);
     }

     return;
}

/*
 *              r e p o r t _ r e s o u r c e
 * 
 *    This routine is called to output resources information received from
 *    the print server to stderr. This routine adds R prefix to the message.
 *
 *    Inputs:
 *         res              = Address of the resource block
 *         reslen           = Length of the resource block
 *                            (0 resource length outputs empty resource tag 
 *                             indicating end of available resources)
 *
 *
 */
report_resource(res, reslen)
FIELD8 *res;
int reslen;
{
        FIELD8 *rptr, rlen;
        int cur_len;
        FIELD8 tmp_buf[255];
        
        if (reslen == 0)
             fprintf (stderr, "R\n");
        else  
	{    
             /*
              * format the resource data - init output buffer
              */
             bzero(tmp_buf, sizeof(tmp_buf));
             /*
              * Add R (resource) tag
              */
             rptr = tmp_buf;
             strncpy(rptr, "R", strlen("R"));
             cur_len = strlen ("R");
             /*  
              * Add resource type  
              */
             rptr = &tmp_buf[cur_len];
             rlen = *(FIELD8 *)res++;
             strncat(rptr, res, rlen);
             cur_len += rlen;
             res += rlen;
             /* 
              *  Add delimeter " " (space)
              */
             rptr = &tmp_buf[cur_len];
             strncat(rptr, " ", strlen(" "));
             cur_len += strlen(" ");
             /* 
              *  Add  resource number 
              */
             rptr = &tmp_buf[cur_len];
             rlen = *(FIELD8 *)res++;
             strncat(rptr, res, rlen);
             cur_len += rlen;
             res += rlen;
             /* 
              *  Add delimeter " " (space) 
              */
             rptr = &tmp_buf[cur_len];
             strncat(rptr, " ", strlen(" "));
             cur_len += strlen(" ");
             /* 
              *  Add  resource version number 
              */
             rptr = &tmp_buf[cur_len];
             rlen = *(FIELD8 *)res++;
             strncat(rptr, res, rlen);
             /* 
              *   Output formatted resource information 
              */
             fprintf (stderr, "%s\n", tmp_buf);
	}

        return;
}

/*
 *              g e t l i n e
 *  
 * This routine extracts line from the msg_ptr string buffer.
 *
 *     input:    msg_ptr = extract line and update buffer; 
 *                         set "nil" for empty buffer.
 *
 *    output:    outbuf  = extracted line.
 *
 *   
 */
getline (outbuf)
char outbuf[];
{
              int i;
              char *ptr;

              if ( (ptr = index(msg_ptr,'\n')) == 0 ) {
                     strcpy(outbuf, msg_ptr);
                     msg_ptr = ptr;
                     return;
	      }
              i = ptr - msg_ptr;
              strncpy(outbuf, msg_ptr, i);
              outbuf[i] = '\0';
              msg_ptr = ++ptr;

              return;
}
