#ifndef lint
static char sccsid[] = "@(#)iplpscomm.c	4.2      LPS_ULT_IP 	11/15/90";
#endif
/*
 *****************************************************************************
 *									     *
 *  COPYRIGHT (c) 1987, 1988, 1989       				     *
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
 */
/* 
 *  iplpscomm.c  - iplpscomm filter main program
 * 
 *  23-Mar-1989    Created - Ajay Kachrani
 *                 The iplpscomm uses the existing code from the 
 *                 IP-LPS V1.0 code, where possible.
 *
 * IPLPSCOMM filter. Called by lpd as follows:
 *	stdin		file to be printed
 *	stdout		we ignore (close) this
 *	stderr		error logging and accounting info 
 *                         A accounting
 *                         O operator errors
 *                         U user errors
 *                         R resource reporting
 *
 * Arguments are:
 *   iplpscomm printerName userName hostName hostJobid [filename]
 *
 * ---------------------------------------------------------------------
 ******************LPS V4.0 and ULTRIX, V4.next**********************
 *
 *    X4.0-10 11-JUL-1990  MVA  Modify to use PAP (paprelay).
 *    **********************************
 *
 *    X4.0-11 20-Oct-1990  APK  Change "exit (0)"s to "exit (EX_TRYAGAIN)" so
 *                              that when received NAK from the PrintServer
 *                              or network error causes jobs to requeue rather
 *                              then lost!
 *    **********************************
 *
 *
 ************************************ ULTRIX, V4.0**********************
 *    V0.001  23-Mar-1989  APK  Created   
 *    *********************************
 *
 *    V0.002  21-Jun-1989  APK  Fix OP_INFO packet for LPS20 front pannel bug
 *    **********************************
 *
 *    V0.003  10-Jul-1989  APK  Remove creation of STATUS file in Spooling dir
 *                              Which is inconsistenc with BSD Spooler
 *    **********************************
 *
 **********************************LPS/ULTRIX, V2.0**********************
 * 
 *    V2.001  19-Dec-1989  APK  Log error when NAK (PrintServer Not accepting
 *                              job) received from PrintServers
 *    *********************************
 * 
 */

#include <stdio.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/signal.h>
#include <sys/un.h>
#include <sys/file.h>
#include <errno.h>
#include <ctype.h>
#include <sys/time.h>
#include <setjmp.h>
#define SETJMP_LOADED

#include "lps.h"
#include "paprelay.h"

/*
 *  macro definitions
 */
#define acct(page_count) fprintf(stderr,"Asheets printed %u\n",page_count)
     
#define BUFFERSIZE 256
/* global variables */

	/*    Variables set from options */

char	*printerName;
char	*userName;		
char	*userHost;		       
int     client_job_id;
FILE    *printStream;
char	printerNode[128];

char	progName[128];			/* from argv[0] */
int  	debug=0;
int	verbose=0;

	/*     other variables      */

int	pageCount;			/* from parsing EOJ reply */
char	errbuf[256];

int	serverFD;			/* FD open to print server or relay */

char	serverJobNumber[128];		/* used in StartPrintSession() */
char	serverID[128];
char	serverNode[128];
char	thisHost[256];

int	jump_read_error=0,		/* flags asking for longjmp on */
	jump_write_error=0;		/* I/O errors in comm pkg */
jmp_buf	read_error,
	write_error;


/* external procedure definitions */

char *mktemp();

/* forward procedure definitions */

char *LPStime();
void SignalHandler();
void HandleServerFrame();
int LogServerFrame();


/*
 *		m a i n   p r o g r a m
 *
 */

main(argc, argv)
int argc;
char *argv[];
{

	int j;

        printStream = stdin;        /* default:  print from stdin */
	ProcessOptions(argc,argv);
	gethostname(thisHost,sizeof(thisHost));

#ifdef DEBUG
	if (debug > 1) {
	    fprintf(stderr,"O%s	%s:",
				   progName, LPStime());
	    for (j=0; j<argc; j++) {
		fprintf(stderr,"O %s",argv[j]);
	    }
	    fprintf(stderr,"O\n");
	}
#endif DEBUG
	fprintf(stderr, "R\n");
	fprintf(stderr,"O%s	%s: Start job number %d for %s@%s\n",
	    progName, LPStime(), client_job_id, userName, userHost);
	fprintf(stderr,"U%s	%s: Start job number %d for %s@%s\n",
	    progName, LPStime(), client_job_id, userName, userHost);
        
	PrintFile(printStream);

#ifdef DEBUG
	if (debug > 0)
	    fprintf(stderr,"O%s	%s: normal exit.\n",progName,LPStime());
#endif DEBUG
	exit(EX_SUCCESS);
 }

/*
 *		P r o c e s s O p t i o n s
 *
 * This routine is called once from main(). It reads the command line,
 * decodes the calling options, sets global variables as required, and
 * returns.
 * 
 * Inputs:		argc, argv
 * Returns:		nothing
 * Actions:		Parses argv and sets global variables accordingly
 *
 */
ProcessOptions(argc,argv)
int argc;
char *argv[];
{
    char *left;
    char *getenv();
    char read_buf [BUFFERSIZE];

    /*
     * get the program name
     */
    left = rindex(argv[0],'/');
    if (left) {
	    left++;
	    strcpy(progName,left);
    } else {
	    strcpy(progName,argv[0]);
    }
	if (argc < 5)
	{
		fprintf(stderr, "O%s: error: Usage: %s server_node user_name client_node client_job_id\n", progName, progName);
		exit(EX_DISCARD);
	}
	printerName = argv[1];
        userName = argv[2];
        userHost = argv[3];
        client_job_id = atoi(argv[4]);
	/*
	 * if fifth arg supplied, use it as input file name
	 */
	if (argc > 5)
	{
	    if (freopen(argv[5], "r", printStream) == NULL)
	     {
		  sprintf(read_buf, "Oiplpscomm: error: can't open file %s", argv[5]);
		  perror(read_buf);
                  exit (EX_DISCARD);
		}
	}

    return;
}

/* 
 *		S t a r t P r i n t S e s s i o n
 * Begin a new print session on our newly-assigned printer. This routine
 * is called once from main() after the printer has been assigned to us
 * but before we do anything with it.
 *
 * Inputs:		Hname	name of this machine
 *			SID	name of this session
 * Outputs:		serverJobNumber
 *
 */

StartPrintSession(Hname, SID)
 char *Hname, *SID;
{
    int ID, Length, ntries;
    char buffer[2+DECNET_MAX_FRAME];
    char Opcode[OP_MAX_SIZE];

    sprintf(buffer,"SESSIONID=%s\1HOST=%s", SID, Hname);

    ntries = 0;
    while (TRUE) {
	ID = LP_C_send(serverFD, OP_SSN, buffer, 0, 0);
	if ( ! LP_C_waitForReply(serverFD, -1, Opcode, ID, &Length,
		    buffer, (int (*)())0)) { 
            if (!strcmp(Opcode,OP_NAK)) {
	      fprintf (stderr, "O%s       %s: PrintServer currently not accepting jobs.\n",progName,LPStime());	  
              fprintf (stderr, "U%s       %s: PrintServer currently not accepting jobs.\n",progName,LPStime());
	    }
	    exit(EX_TRYAGAIN) ;
	}
	if (!strcmp(Opcode,OP_REPLY)) break;
	if (ntries<6) ntries++;
	sleep(1<<ntries);
    }
    strcpy(serverJobNumber, getvar(buffer, "JOBNO"));
    strcpy(serverID, getvar(buffer, "SERVERID"));
    strcpy(serverNode, getvar(buffer,"NODE"));

    fprintf (stderr, "O%s: serverNode = %s\n", progName, serverNode);

    fprintf(stderr,"O%s	%s: Start session, %s Server ID is %s\n",
	    progName, LPStime(), printerName, serverJobNumber);

    fprintf(stderr,"U%s	%s: Start session, %s Server ID is %s\n",
	    progName, LPStime(), printerName, serverJobNumber);
}

/*
 *		P r i n t F i l e
 *
 * Called once from main() to print the file.
 *
 * Inputs:		dataStream (a stream already open, to be printed)
 *
 * Action:		opens a printer connection, sends the file to it,
 *			and closes the connection.
 *
 */
PrintFile(dataStream)
FILE *dataStream;
{
    int ID, Length, stat;
    int ErrVal;
    char srcId [256];
    char buffer[2+DECNET_MAX_FRAME];
    char Opcode[OP_MAX_SIZE],sOpcode[OP_MAX_SIZE];
    char sData[2+DECNET_MAX_FRAME];
    int sID, sLength;  
    char *j;

    /*
     *   Open printer 
     */
    serverFD = AssignDirect (printerName);
    EnableKeepalive (serverFD);
    StartPrintSession(thisHost, "direct-connect");

#ifdef DEBUG
    if (debug > 3) fprintf(stderr,"O%s	%s: printing FD %d to printer %d\n",
	progName, LPStime(), fileno(dataStream), serverFD);
#endif DEBUG

    signal(SIGHUP, SignalHandler);
    signal(SIGINT, SignalHandler);
    signal(SIGQUIT, SignalHandler);
    signal(SIGILL, SignalHandler);
    signal(SIGEMT, SignalHandler);
    signal(SIGBUS, SignalHandler);
    signal(SIGSEGV, SignalHandler);
    signal(SIGSYS, SignalHandler);
    signal(SIGTERM, SignalHandler);

    jump_write_error = TRUE;
    if ((ErrVal=setjmp(write_error)) > 0) {
        LP_C_close(ErrVal);
	fclose(dataStream);
	exit(EX_TRYAGAIN);
    }

/* 
 * send up-to-date identification information
 *
 * We can send note (NOTE=%s\1) field, e.g. Job name or Data_Type
 * when we decide to use extended interface.
 */
    sprintf (srcId, "client jobId %d", client_job_id);
    sprintf (buffer, 
             "NOTE=%s\1USERID=%s\1SESSIONID=%s\1HOSTNAME=%s",
             progName, userName, srcId, userHost);

    LP_C_send(serverFD, OP_SOJ, buffer, 0, 0);

/* start a new (sub) job */
    LP_C_send(serverFD, OP_SOD, (char *) NULL, 0, 0);

/* now send the data stream, to eof */
    while (!feof(dataStream)) {
        int nitems;

	/* the "-20" in the following line allows for header bytes */
	nitems=fread(buffer,1,DECNET_MAX_FRAME-20,dataStream);
	if (nitems == 0 && !feof(dataStream)) {
	    sprintf(errbuf,"%s	%s: error reading input; aborting",progName,LPStime());
	    perror(errbuf);
	    exit(EX_TRYAGAIN);
	}

	if (nitems > 0) LP_C_send(serverFD, OP_DATA, buffer, 0, nitems);

     /* always look to see if there is any output returned for us. */
	if (LP_C_probe(serverFD)) {
	    stat = LP_C_recv(serverFD, sOpcode, &sID, &sLength, sData);
	    if (stat > 0) {
	        HandleServerFrame(sOpcode, sID, sLength, sData);
	    }
	}
    }
    ID = LP_C_send(serverFD, OP_EOD, NULL, 0, 0);
    fclose(dataStream);
    if ( ! LP_C_waitForReply(serverFD, -1, Opcode, ID, &Length, buffer, LogServerFrame))
	exit(EX_TRYAGAIN);
    fprintf(stderr,"O%s	%s: End of job, %s\n",
	progName, LPStime(), buffer);
    fprintf(stderr,"U%s	%s: End of job, %s\n",
	progName, LPStime(), buffer);
    LP_C_close(serverFD);
    jump_write_error = FALSE;

/* Local accounting. Now that we have released the printer, send the 
   accounting infor to lpd */
	
    j = getvar(buffer, "PAGES");
    if (j == (char *) NULL) return;
    pageCount = atoi(j);

    acct (pageCount);

    return;
    
}

/*
 *
 *		H a n d l e S e r v e r F r a m e
 *
 * Called when we receive a frame from the server. Some frames are commands
 * to us, but most frames are just data that get written to the log file.
 *
 * Inputs	Opcode, ID, Length, Data (4 standard frame components)
 * Action	Examines the Opcode to decide what to do. If it is OP_KILL,
 *		then we shut down and exit. If it is OP_DATA, we write it
 *		to the log file. If it is anything else, we just ignore it.
 * Side Effects Exits the program if OP_KILL received.
 *
 */
void HandleServerFrame(Opcode, ID, Length, Data)
char *Opcode;
int  ID;
int  Length;
char *Data;
{
 /* If this is an OP_KILL coming from the printer or relay server, then 
    we just die without asking questions. Usually this condition is caused
    by something we've done wrong, anyhow, and there will already be
    error messages explaining why. */

    ID = Length;		/* suppress saber error message */
    Length = ID;		/* ditto */
    if (!strcmp(Opcode, OP_KILL)) {
	fprintf(stderr,"O%s	%s: print server requested job abort.\n",
	    progName, LPStime());
	LP_C_close(serverFD);
	serverFD = -1;
	exit(EX_TRYAGAIN);
    }

 /* If this is user data, then write it to the log file. */

    if (!(strcmp(Opcode, OP_DATA) && strcmp(Opcode, OP_EMSG))) {
        fprintf(stderr,"U%s	%s++%s\n",progName, LPStime(), Data);
    }
}

/*
 *
 *              L o g S e r v e r F r a m e
 *
 * Called via outcall from LP_C_waitForReply to handle frames that arrive
 * while we are sending data or waiting for the EOJ reply.
 *
 * Inputs       FromServer: Boolean true if this msg is server->client
 *              Opcode: string opcode of message
 *              MesgID: integer message ID
 *              Length: integer length
 *              Data: string Text part of message
 * Action	Calls HandleServerFrame
 */

int LogServerFrame(FromServer, Opcode, MesgID, Length, Data)
int FromServer;
char *Opcode;
int MesgID;
int Length;
char *Data;
{
    if (!FromServer) return;    /* We don't know what to do with client msgs */
    HandleServerFrame(Opcode, MesgID, Length, Data);
    return;
}

/*
 *		S i g n a l H a n d l e r
 *
 * Called automatically when a SIGINT signal is sent, which asks us to abort.
 * 
 * Inputs:		sigtype, sigcode, unused
 * Action:		Sends an OP_KILL to the relay server, then
 *			terminates the program.
 *
 */

static void SignalHandler(sigtype)
int sigtype;
{
     static char *sigNames[] = {
         "SIGHUP", "SIGINT", "SIGQUIT", "SIGILL", "SIGTRAP",
	 "SIGIOT", "SIGEMT", "SIGFPE", "SIGKILL", "SIGBUS",
	 "SIGSEGV", "SIGSYS", "SIGPIPE", "SIGALRM", "SIGTERM" 
     };
     static int reEntries = 0;
     char *signame;

     /* prevent horrible loops when something is badly wrong */
     reEntries++;
     if (reEntries > 1) abort(0);

     if (sigtype == SIGINT) {
	fprintf(stderr, "O%s	%s: Print job killed by user.\n", progName, LPStime());
	fprintf(stderr, "U%s	%s: Print job killed by user.\n", progName, LPStime());
	 LP_C_send_urgent(serverFD, OP_KILL, NULL, 0, 0);
	 LP_C_close(serverFD);
	exit (EX_DISCARD);
     } else {
	if (sigtype > 0 && sigtype <= SIGTERM)
	    signame = sigNames[sigtype-1];
	else
	    signame = "Unknown";
	fprintf(stderr, "%s	%s: signal %d (%s) received. Aborting.\n",
		progName, LPStime(), sigtype, signame);
 	LP_C_send(serverFD, OP_KILL, NULL, 0, 0);
	LP_C_close(serverFD);
	abort(0);
     }
}


/*
 *		L P S t i m e
 *
 * Determine the current time and format it into a global string.
 * 
 * Inputs:		None
 * Returns:		pointer to current time string
 *
 */
char *LPStime()
{
	long lt;
	char *ts;
 
	time(&lt);
	ts = ctime(&lt);
	ts[19] = 0;
	ts += 4;
	return(ts);
}
