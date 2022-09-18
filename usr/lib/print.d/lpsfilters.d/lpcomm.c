#ifndef lint
static char sccsid[] = "@(#)lpcomm.c	4.2      ULTRIX 	11/15/90";
#endif
/* 
 * lpcomm.c -- Communication routines
 *
 * Author: Mike Augeri
 * Date: 05-APR-1990
 *
 * History:
 *   12-APR-1990  mva  Change "ifdef ULTRIX" to "ifndef VMS" so that sources
 *		       will compile on non-ULTRIX UNIX systems.  Also, put
 *		       my print statements behind an "ifdef VMS" and
 *		       "if (debug > n)" statement.
 *   15-MAY-1990  mva  Fixed the problem with hibernate returning while ast
 *                     completed was still not true.  The hibernate was
 *                     returning immediately due to an outstanding wake from
 *                     an earlier exit from the ast handler.  Also, change
 *                     debug options.
 *   15-MAY-1990  mva  Fixed the problem with uninitialized value of retval in
 *                     LP_C_probe routine.  Also changed LP_C_gets and
 *                     LP_C_getc routines to use global stdinFD as index rather
 *                     than using fileno(stdin) as index.
 *   16-MAY-1990  mva  Modified LP_C_getc routine for use with the lprc module.
 *                     The lprc module is somewhat unique in that it has two
 *                     open channels, one of them the network as usual, the
 *                     other is stdin for reading input form keyboard.  Using
 *                     sys$qio to read stdin is what made the change to the
 *                     routine necessary.
 *   11-JUL-1990  mva  Remove extern from stdinFD declaration.
 *   11-JUL-1990  mva  Change to paprelay.h.
 *   13-JUL-1990  mva  Move issue_read and ast_handler to vms_utilities.
 *   13-JUL-1990  mva  Add include for ultrix_utilities.h
 *   16-JUL-1990  mva  Initialize stdinFD to -1
 *   23-JUL-1990  jwfm Replace second two instance of strncpy w/ memcpy
 *	 	       so we can pass binary data.  Change the debug code
 *		       in LP_C_send so it uses fwrite instead of fprintf.
 *   23-AUG-1990  jwfm Add file discripter mapping.  Map VMS file discriptors
 *                     to a number from 0 to 31 so we don't 64k buffers.
 *   05-NOV-1990  mva  Many changes including a rewrite of the LP_C_getc
 *                     routine for VMS and modification of the LP_C_probe
 *                     routine; some cleanup done, but much more can be done.
 */

/* $Header: lpcomm.c,v 4.0 88/12/12 14:50:54 reid Exp $
   $Source: /mnt/reid/skunkworks/src/RCS/lpcomm.c,v $

This module contains communications subroutines used by the client programs.
The client programs include the relay server, the lpd filters, and the
various accounting listeners.

The communication between server and client is via a packet protocol
described in the Relay Protocol document. That packet protocol is implemented
on top of whatever network mechanism is available.

This version of the comm software implements the packet protocol on top of
TCP/IP.

	Brian Reid
	DEC Western Research
 
 * 
 */

#ifndef VMS
#include <sys/file.h>
#include <sys/types.h>		/* system data types */
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <signal.h>		/* signal handler */
#include <setjmp.h>		/* unwind handler */
#include "ultrix_utilities.h"
#define SETJMP_LOADED
#else VMS
#include <iodef.h>		/* i/o definitions */
#include <ssdef.h>		/* system service defintions */
#include "vms_utilities.h"
#endif VMS
#include <stdio.h>		/* standard i/o definitions */
#include <string.h>
#include "lps.h"
#include "paprelay.h"

/* definitions of external procedures and variables */

int stdinFD = -1;

static char  *inputBuffers[AD];	/* p. 82 of K&R says these guaranteed =0 */
static int    firstUnused[AD];	/* pointer to first unused byte in buffer */
static int    lastByteIn[AD];   /* last byte in buffer memory */
/* temporarily make probeCache non-static so can see in vms_utilities module */
int    probeCache[AD];	/* avoid redundant calls to select() */
static int    sendFlag=0;	/* for sending urgent data */

#ifdef VMS
extern char *qioBuffers[AD];	/* used by ast qio read operation */
extern int rcv_pending[AD];
extern int ast_completed[AD];

extern word fd_map[AD];         /* One entry for each "ULTRIX" fd, holds VMS 
				   fd so we can map it back for sys calls */
extern char fd_used[AD];        /* Data structure to keep track of which slots
				   in fd_map are used.  USED if used, EMPTY
                                   other wise */
#endif VMS


/* 
 *		L P _ C _ r e c v
 *
 * Read and parse one message from a socket open for input. These messages
 * can come either from server or client. Because of the peculiar structure
 * of the messages, it is necessary to parse them in order to locate their
 * end (the end of a message is determined by a count field imbedded in the
 * middle of the record).
 * 
 * This procedure and its subroutine incP embody all of the complexity of the
 * stream-to-record conversion, and are in theory the only things that will
 * have to change if (when) the semantics of the network channel changes.
 *
 * Inputs:		int ServerFD, *Opcode, *ID, *Length; char *Data
 * Outputs:		same
 * Return:		>0 if a message was available, 0 if no message
 *			available, and <0 if EOF
 * Action:		If no messages are available from the server, 
 *			return immediately with a return value of False.
 *			If there are messages, then the oldest message is
 *			removed from the arrival queue and unpacked into
 *			the four variables provided.
 * Side effects:	Removes one message from the arrival message queue.
 *
 */    

LP_C_recv (ServerFD, Opcode, ID, Length, Data)
int ServerFD;
char *Opcode;
int *ID, *Length;
char *Data; 
{
    int *jsub;			/* pointer to buffer subscript */
    char *jc;			/* char pointer to buffer origin */
    char *startField;		/* flag beginning of each field */

#ifdef VMS
    if (debug > 7)
	fprintf(stderr,"Entering LP_C_recv, ast_completed[%d] = %d\n",
	    ServerFD,ast_completed[ServerFD]);
#endif VMS
/* test to see if there is a buffer allocated for this FD, and if there is
   not, then create an empty buffer. */
    if (inputBuffers[ServerFD] == (char *) 0) {
        inputBuffers[ServerFD] = (char *) malloc (1+PACKET_BUFFER_SIZE);
	lastByteIn[ServerFD] = -1;
	firstUnused[ServerFD] = 0;
	probeCache[ServerFD] = FALSE;
    }

    jc = inputBuffers[ServerFD];
    jsub = &firstUnused[ServerFD];
    if (incrP(ServerFD,0)) {
#ifdef VMS
        if (debug > 7)
	    fprintf(stderr,"Exiting LP_C_recv via incrP preload check, ast_completed[%d] = %d\n",
		ServerFD,ast_completed[ServerFD]);
#endif VMS
	goto endfile; /* preload the buffer */
    }
    
    /*
     * sync on packet beginning; if this is necessary, there is a bug
     * somewhere, but in the interests of resilience we put this in here
     * anyhow.
     *
     * mva -- I believe the following fprintf is responsible for putting
     * "garbage" (e.g., nulls) into logFile or stderr.
     */
    while (jc[*jsub] != RELAY_SOM) {
	fprintf(stderr,"%c",jc[*jsub]);
	if(incrP(ServerFD,1)) {
#ifdef VMS
	    if (debug > 7)
		fprintf(stderr,"Exiting LP_C_recv via incrP RELAY_SOM check, ast_completed[%d] = %d\n",
		    ServerFD,ast_completed[ServerFD]);
#endif VMS
	    goto endfile;
	}
    }
				/* Get field 1 */
    if(incrP(ServerFD,1)) goto endfile;
    while (jc[*jsub] == ' ') if(incrP(ServerFD,1)) goto endfile;
    startField = &jc[*jsub];
    while (jc[*jsub] != ' ') if(incrP(ServerFD,1)) goto endfile;
    jc[*jsub] = (char) NULL;
    strncpy(Opcode,startField,OP_MAX_SIZE);

				/* Get field 2 */
    if(incrP(ServerFD,1)) goto endfile;
    while (jc[*jsub] == ' ') if(incrP(ServerFD,1)) goto endfile;
    startField = &jc[*jsub];
    while (jc[*jsub] != ' ') if(incrP(ServerFD,1)) goto endfile;
    jc[*jsub] = (char) NULL;
    *ID = atoi(startField);    

				/* Get field 3 */
    if(incrP(ServerFD,1)) goto endfile;
    while (jc[*jsub] == ' ') if(incrP(ServerFD,1)) goto endfile;
    startField = &jc[*jsub];
    while (jc[*jsub] != ' ') if(incrP(ServerFD,1)) goto endfile;
    jc[*jsub] = (char) NULL;
    *Length = atoi(startField);    

				/* Get field 4 */
    if (*Length > 0) {
	if(incrP(ServerFD,1)) goto endfile;
	startField = &jc[*jsub];
    
    /* because incrP guarantees that the resulting incremented pointer
       will point to valid data, we want to increment by one less than
       the actual buffer size here, to avoid issuing a (blocking) read
       for the data that is actually in the next frame. This makes the
       count off by one */
	if(incrP(ServerFD,(*Length)-1)) goto endfile;
	memcpy(Data,startField, *Length);
    /* here we repair the count so that it is no longer off by one */
	firstUnused[ServerFD]++;
    }
    Data[*Length] = (char) NULL;

/* We have now finished processing this packet. Slide the data in the
   buffer so that the next call will have lots of buffer space. */
    if (firstUnused[ServerFD] > DECNET_MAX_FRAME) {
        register char *ip,*jp;
	register int n,shiftAmount;
	/* debug code */
	char c1,cn;
	int i1,in;
	/* end debug code */
	
  /* first check to make sure that there actually is data in the buffer */
	if (firstUnused[ServerFD] > lastByteIn[ServerFD]) {
	    firstUnused[ServerFD] = 0;
	    lastByteIn[ServerFD] = -1;
	} else {
	    ip = &(inputBuffers[ServerFD][firstUnused[ServerFD]]);
	    jp = inputBuffers[ServerFD];
	    /* debug code */
	    c1 = *ip;
	    cn = inputBuffers[ServerFD][lastByteIn[ServerFD]];
	    i1 = firstUnused[ServerFD];
	    in = lastByteIn[ServerFD];
	    /* end debug code */
	    shiftAmount = lastByteIn[ServerFD] - firstUnused[ServerFD] + 1;
	    for(n=0; n <= shiftAmount; n++)
		*jp++ = *ip++;
	    lastByteIn[ServerFD] = lastByteIn[ServerFD] - firstUnused[ServerFD];
	    firstUnused[ServerFD] = 0;
	    /* debug code */
	    if(debug > 7) {
		if ( (c1 != inputBuffers[ServerFD][0]) ||
		  (cn != inputBuffers[ServerFD][lastByteIn[ServerFD]]) ) {
		    fprintf(stderr,"%s  %s:  bad buffer shift invariant: (%d-%d) '%c'-'%c' became (0-%d) '%c'-'%c'\n",
			progName, LPStime(), i1,in,c1,cn,lastByteIn[ServerFD],
			inputBuffers[ServerFD][0],
			inputBuffers[ServerFD][lastByteIn[ServerFD]]);
		}
	    }
	    /* end debug code */
	}
    }

    if (debug > 6)
	fprintf(stderr,"%s  %s:  %d->rmsg(%s,%d,%d,%s)\n",
	    progName, LPStime(), ServerFD,Opcode, *ID, *Length, Data);

#ifdef VMS
    if (debug > 7)
	fprintf(stderr,"Exiting LP_C_recv via TRUE return, ast_completed[%d] = %d\n",
	    ServerFD,ast_completed[ServerFD]);
#endif VMS
    return(1);

/* We branch here when the channel-handling subroutine detects an EOF */
endfile: 
#ifdef VMS
    if (debug > 7)
	fprintf(stderr,"Exiting LP_C_recv via endfile, ast_completed[%d] = %d\n",
	    ServerFD,ast_completed[ServerFD]);
#endif VMS
    return(-1);
}

/*
 *		i n c r P
 *
 * Increment the buffer pointer for the specified FD, and make sure that
 * there is data for it to point to.
 *
 * Inputs:		int ServerFD, delta
 * Outputs:		none
 * Action:		Increments the input buffer pointer for the file
 *			denoted by ServerFD by "delta". If the result of
 *			that incrementation causes the buffer pointer to
 *			point off the end of the message, then read more
 *			characters until there is data under the new pointer.
 * Side Effects:	Reads data from ServerFD if necessary. Does not 
 *			return until enough data is available.
 */

static incrP(FD,delta)
int FD,delta;
{
    char *buf;
    int  spaceRemaining;
    int ReadStatus;
#ifdef VMS
    char *tmp;
#endif VMS

#ifdef VMS
    if (debug > 8)
	fprintf(stderr,"Entering incrP, ast_completed[%d] = %d\n",
	    FD,ast_completed[FD]);
#endif VMS

    firstUnused[FD] += delta;
    probeCache[FD] = FALSE;

    while (firstUnused[FD] > lastByteIn[FD]) {
	buf = inputBuffers[FD];
	spaceRemaining = PACKET_BUFFER_SIZE - lastByteIn[FD] + 1;
#ifndef VMS
	ReadStatus = recv(FD, &buf[1+lastByteIn[FD]], spaceRemaining, 0);
	if (ReadStatus < 0) {
	    sprintf(errbuf,"%s  %s:  recv(%d) failed %d",
	        progName,LPStime(),FD,ReadStatus);
	    perror(errbuf);
	    return(TRUE);	/* signal EOF */
	}
	if (ReadStatus == 0) return(TRUE);
	lastByteIn[FD] += ReadStatus;
#else VMS
	/*
	 * The following bcopy is expedient, but may not be the best thing to
	 * do as far as performance is concerned.  At this time I don't have
	 * any other ideas.  I considered copying pointers instead of buffers,
	 * but the code works on the assumption that messages are appended to
	 * the input buffer.  (Check to see how we know when the buffer has to
	 * be flushed -- spaceRemaining?)
	 */
	if (!ast_completed[FD]) {
	    if (debug > 8)
		fprintf(stderr,"Exiting incrP return TRUE, ast_completed[%d] = %d\n",
		    FD,ast_completed[FD]);
	    return(TRUE);
	}
	/* Else we have a message, so copy it to input buffer */
	bcopy(qioBuffers[FD],&buf[1+lastByteIn[FD]],qio_transfer_size(FD));
	lastByteIn[FD] += qio_transfer_size(FD);
	rcv_pending[FD] = FALSE;
	ast_completed[FD] = FALSE;
#endif VMS
    }
#ifdef VMS
    if (debug > 8)
	fprintf(stderr,"Exiting incrP return FALSE, ast_completed[%d] = %d\n",
	    FD,ast_completed[FD]);
#endif VMS
    return(FALSE);
}

/*
 *		L P _ C _ p r o b e
 *
 * Test to see whether or not there is a message waiting to be read.
 *
 * Inputs:		int FD
 * Outputs:		TRUE if a msg is waiting, else FALSE
 * Side Effects:	none
 *
 */
LP_C_probe(FD)
int FD;
{
    char *buffer;
    int jp;
    int retval;
    int SelectStatus;		/* status returned to us from select() */
    long SelectMask;		/* mask given by us to select() */
#ifndef VMS
    struct timeval timeout;
#endif VMS

#ifdef VMS
    if (debug > 8)
	fprintf(stderr,"Entering LP_C_probe, ast_completed[%d] = %d\n",
	    FD,ast_completed[FD]);
#endif VMS
    if (FD < 0) return (FALSE);
/* The cached copy of the probe information makes sure that we don't call
   select any more than necessary. It is not very fast, at least not
   with sockets.  */

    if (probeCache[FD]) {
#ifdef VMS
	if (debug > 8)
	    fprintf(stderr,"Exiting LP_C_probe via probeCache check, ast_completed[%d] = %d\n",
		FD,ast_completed[FD]);
#endif VMS
	return(TRUE);
    }
    buffer = inputBuffers[FD];
    if (buffer == 0) goto notInBuffer; /* not yet allocated */
    jp = firstUnused[FD];
    if (jp < 0 || jp > lastByteIn[FD]) goto notInBuffer;
    while (jp < lastByteIn[FD]) {
        if (buffer[jp++] == RELAY_SOM) {
	    retval = TRUE;
	    goto collect;
	}
    }

/* No data in buffer. Check to see if any is pending on socket */
notInBuffer: 
#ifndef VMS
    timeout.tv_sec = 0; timeout.tv_usec = 0;
    SelectMask = 1<<FD;
    SelectStatus = select(32, &SelectMask, 0, 0, &timeout);
    retval = (SelectStatus > 0);
#else VMS
    if (ast_completed[FD])
	retval = TRUE;
    else
	retval = FALSE;
#endif VMS
collect: 
    probeCache[FD] = retval;
#ifdef VMS
    if (debug > 8)
	fprintf(stderr,"Exiting LP_C_probe, ast_completed[%d] = %d\n",
	    FD,ast_completed[FD]);
#endif VMS
    return(retval);
}

/*
 *
 *		L P _ C _ g e t s
 *
 * This routine behaves identically to the stdio routine "gets", namely
 * it reads a string from standard input into the buffer that is its argument.
 * The only difference is that after it is done reading, it resets the
 * probe cache so that the client code will no longer make assumptions
 * about whether or not there is input.
 *
 *
 */
char *LP_C_gets(str)
char *str;
{
    probeCache[stdinFD] = FALSE;
    return(gets(str));
}

/*
 *
 *		L P _ C _ g e t c
 *
 * This routine behaves identically to the stdio routine "getc", namely
 * it reads a character from standard input and returns it.
 * The only difference is that after it is done reading, it resets the
 * probe cache so that the client code will no longer make assumptions
 * about whether or not there is input.
 *
 * The assumption here is that we get "an interrupt per character" and the
 * read from stdin or qioBuffers[stdinFD] returnes a single character.
 */
LP_C_getc()
{
#ifndef VMS
    probeCache[stdinFD] = FALSE;
    return(getc(stdin));
#else VMS
    static char *qiobuf;

    probeCache[stdinFD] = FALSE;
    rcv_pending[stdinFD] = FALSE;
    ast_completed[stdinFD] = FALSE;
    qiobuf = qioBuffers[stdinFD];
    return(*qiobuf);
#endif VMS
}


/* 
 *		L P _ C _ w a i t F o r M s g
 *
 * If there is a message waiting on one of the indicated FD's, then return
 * immediately. If not, then block until a message is available.
 *
 * Inputs:		int FD1, FD2
 * Outputs:		none
 * Side Effects:	none
 * Action:		Blocks until a message is available, then returns.
 *
 * The temptation was strong to write a generalized N-way block routine,
 * using arrays of FD's, but the truth is that for this application you
 * really only need 2-way waiting.
 */
void LP_C_waitForMsg (FD1,FD2)
int FD1,FD2;
{
    long SelectMask;
    long ExceptMask;

#ifdef VMS
    if (debug > 7) {
	fprintf(stderr,"Entering LP_C_waitForMsg\n");
	fprintf(stderr,"  ast_completed[%d] = %d, rcv_pending[%d] = %d, probeCache[%d] = %d\n",
	    FD1,ast_completed[FD1],FD1,rcv_pending[FD1],FD1,probeCache[FD1]);
	fprintf(stderr,"  ast_completed[%d] = %d, rcv_pending[%d] = %d, probeCache[%d] = %d\n",
	    FD2,ast_completed[FD2],FD2,rcv_pending[FD2],FD2,probeCache[FD2]);
    }
    /*
     * If we have an AST completed on a specific channel, we want to set
     * probeCache TRUE so that the following test will allow us to return
     * and process the message.
     *
     * We explicitly check for the second channel (which will always be the
     * channel assigned to stdin) because the second channel is initialized
     * to -1 and the compiler will allow you to read ast_completed[-1] without
     * generating an access violation.  However, we can't set the value!!!
     */
    if (ast_completed[FD1])
	probeCache[FD1] = TRUE;
    if (FD2 >= 0 && ast_completed[FD2])
	    probeCache[FD2] = TRUE;
#endif VMS

    if (FD1 >= 0 && LP_C_probe(FD1))
	return;
    if (FD2 >= 0 && LP_C_probe(FD2))
	return;

#ifndef VMS
    SelectMask = 0;
    if (FD1 >= 0)
	SelectMask |= (1<<FD1);
    if (FD2 >= 0)
	SelectMask |= (1<<FD2);
    ExceptMask = SelectMask;

    select(32, &SelectMask, 0, &ExceptMask, 0);

    SelectMask |= ExceptMask;
    if (FD1>=0 && (SelectMask & (1<<FD1)))
	probeCache[FD1] = TRUE;
    if (FD2>=0 && (SelectMask & (1<<FD2)))
	probeCache[FD2] = TRUE;
#else VMS
    if (FD1 >= 0)
	if (!rcv_pending[FD1])
	    issue_read(FD1);
    if (FD2 >= 0)
	if (!rcv_pending[FD2])
	    issue_read(FD2);
    if (debug > 7) {
	fprintf(stderr,"In LP_C_waitForMsg before call to HIBER\n");
	fprintf(stderr,"  ast_completed[%d] = %d, rcv_pending[%d] = %d, probeCache[%d] = %d\n",
	    FD1,ast_completed[FD1],FD1,rcv_pending[FD1],FD1,probeCache[FD1]);
	fprintf(stderr,"  ast_completed[%d] = %d, rcv_pending[%d] = %d, probeCache[%d] = %d\n",
	    FD2,ast_completed[FD2],FD2,rcv_pending[FD2],FD2,probeCache[FD2]);
    }
    if (FD2 == -1 ) {
	while (!ast_completed[FD1]) {
	    if (debug > 7) fprintf(stderr,"In LP_C_waitForMsg in if with one FD\n");
	    SYS$HIBER();
        }
    }
    else {
	while (!ast_completed[FD1] && !ast_completed[FD2]) {
	    if (debug > 7) fprintf(stderr,"In LP_C_waitForMsg in if with two FDs\n");
	    SYS$HIBER();
	}
    }
    if (debug > 7) {
	fprintf(stderr,"In LP_C_waitForMsg following return from HIBER\n");
	fprintf(stderr,"  ast_completed[%d] = %d, rcv_pending[%d] = %d, probeCache[%d] = %d\n",
	    FD1,ast_completed[FD1],FD1,rcv_pending[FD1],FD1,probeCache[FD1]);
	fprintf(stderr,"  ast_completed[%d] = %d, rcv_pending[%d] = %d, probeCache[%d] = %d\n",
	    FD2,ast_completed[FD2],FD2,rcv_pending[FD2],FD2,probeCache[FD2]);
    }
    /*
     * After returning from the sys$hiber, update the probeCache.
     */
    if (ast_completed[FD1])
	probeCache[FD1] = TRUE;
    if (FD2 >= 0 && ast_completed[FD2])
	    probeCache[FD2] = TRUE;
    if (debug > 7) {
	fprintf(stderr,"Exiting LP_C_waitForMsg\n");
	fprintf(stderr,"  ast_completed[%d] = %d, rcv_pending[%d] = %d, probeCache[%d] = %d\n",
	    FD1,ast_completed[FD1],FD1,rcv_pending[FD1],FD1,probeCache[FD1]);
	fprintf(stderr,"  ast_completed[%d] = %d, rcv_pending[%d] = %d, probeCache[%d] = %d\n",
	    FD2,ast_completed[FD2],FD2,rcv_pending[FD2],FD2,probeCache[FD2]);
    }
#endif VMS
    return;
}

/*
 *		L P _ C _ w a i t F o r R e p l y
 *
 * Called from user code when the code needs to wait for a specific reply
 * from the server. LP_C_waitForReply repeatedly calls LP_C_recv and checks
 * to see if the received message is the one we are waiting for. If the
 * message is the one we are waiting for, then LP_C_waitForReply returns
 * after copying the message fields into its arguments. If the message does
 * not arrive, then we wait forever.
 * 
 * Inputs:	 	int ServerFD		   incoming FD
 *			OtherFD			   outgoing FD
 *			Opcode, ID, Length; Data   standard frame
 *			int dproc()		   callback procedure
 * Outputs:		Opcode, Length, Data
 * Returns:		True if successful, False if end of file
 * Side Effects:	reads and removes messages from server sockete
 *			until it finds one matching "ID". If messages
 *		        appear on client sockets while waiting, those
 *			messages are properly dispatched.
 */

int LP_C_waitForReply(ServerFD, OtherFD, Opcode, ID, Length, Data, dproc)
 int ServerFD,OtherFD;
 char *Opcode;
 int ID, *Length;
 char *Data;
 int (*dproc)();
{
    int MesgID,Status;

#ifdef VMS
    if (debug > 7)
	fprintf(stderr,"Entering LP_C_waitForReply, ast_completed[%d] = %d\n", ServerFD,
	    ast_completed[ServerFD]);
#endif VMS
    MesgID = -1;		/* Impossible value for a message ID */
    while (MesgID != ID) {
	if (debug>7)
	    fprintf(stderr,"%s  %s:  waiting for message %d\n",
		progName, LPStime(), ID);
	LP_C_waitForMsg(ServerFD,OtherFD);
	if (LP_C_probe(ServerFD)) {
	    Status = LP_C_recv(ServerFD, Opcode, &MesgID, Length, Data);
	    if (Status < 0) {
#ifdef VMS
		if (debug > 7)
		    fprintf(stderr,"Exiting LP_C_waitForReply via status < 0, ast_completed[%d] = %d\n",
			ServerFD,ast_completed[ServerFD]);
#endif VMS
		return(FALSE);
	    }
	    if (MesgID == ID && 
	          (!strcmp(Opcode,OP_REPLY) || !strcmp(Opcode,OP_NAK))
	        ) {
#ifdef VMS
			if (debug > 7)
			    fprintf(stderr,"Exiting LP_C_waitForReply via MesgID check, ast_completed[%d] = %d\n",
				ServerFD,ast_completed[ServerFD]);
#endif VMS
			return(TRUE);
	    }
	    if ( dproc != 0) {
		(*dproc)(TRUE, Opcode, MesgID, *Length, Data);
	    }
	}
	if (LP_C_probe(OtherFD)) {
	    if (debug>7)
		fprintf(stderr,"%s  %s:  input on otherFD while waiting.\n",
		    progName, LPStime());
	    Status = LP_C_recv(OtherFD, Opcode, &MesgID, Length, Data);
	    if (Status < 0) return(FALSE);
	    if ( (dproc) !=  0) {
		(*dproc)(FALSE, Opcode, MesgID, *Length, Data);	    
	    }
	}

    }
#ifdef VMS
    if (debug > 7)
	fprintf(stderr,"Exiting LP_C_waitForReply, ast_completed[%d] = %d\n", ServerFD,
	    ast_completed[ServerFD]);
#endif VMS
    return(TRUE);		/* in theory this is unreachable */
}


/*
 *
 *		L P _ C _ s e n d
 *
 * This routine composes and sends a message to the print server. The
 * message format is specified by the "Print Server Protocol".
 *
 * Inputs:		ServerFD (integer)	FD to send to
 *			Opcode (string)		opcode
 *			mdata (string)		what to send
 *			ID (integer)		message number, or zero
 *			count (int)		byte count, or zero
 * Outputs:		none
 * Returns:		ID (integer)		generated message number
 *
 * If the byte count is zero, then we use strlen() to count bytes. If it
 * is nonzero, then the data can contain nulls.
 */

int SequenceID=0;		/* integer sequence ID number */

int LP_C_send(ServerFD, Opcode, mdata, ID, mlen)
int ServerFD;
char *Opcode, *mdata;
int ID;
{ char packetBuffer[2*DECNET_MAX_FRAME];
  char *ic,*jc;
  static int pow10[10]=
    {1,1,10,100,1000,10000,100000,1000000,10000000,100000000};
  int nlen,tlen,i,n,r;
  int fixedLength;			/* size of fixed part of packet */
  int packetLength;
#ifdef VMS
    int WriteStatus;
    unsigned short iosb[4];
#endif VMS

#ifdef VMS
    if (debug > 7)
	fprintf(stderr,"Entering LP_C_send, ast_completed[%d] = %d\n", ServerFD,
	    ast_completed[ServerFD]);
#endif VMS
    jc = packetBuffer;
    if (mdata == (char *) 0)
        ic = "";
    else
        ic = mdata;
    if (ID == 0) ID = ++SequenceID;

/* Build the outgoing frame, laboriously by hand. We used to do this with
   sprintf, but it used too much cpu time and was actually limiting the
   printer speed on loaded microvax clients. */

    *jc++ = (char) RELAY_SOM;

    for (i=0;i<strlen(Opcode);i++) *jc++ = Opcode[i];
    *jc++ = ' ';
    
    r = ID;
    for(n=fakelog10(r); n>0; n--) {	/* the frame ID */
        *jc++ = '0'+ (r / pow10[n]);
	r %= pow10[n];
    }
    *jc++ = ' '; *jc = (char) NULL;

    fixedLength = strlen(packetBuffer);
    fixedLength += fakelog10(DECNET_MAX_FRAME-fixedLength);
    if (mlen == 0) mlen = strlen(ic);
    nlen = mlen;

    if (nlen + fixedLength >= DECNET_MAX_FRAME)
        nlen = DECNET_MAX_FRAME - fixedLength - 1;

    tlen = nlen;
    for(n=fakelog10(tlen); n>0; n--) {	/* the size */
        *jc++ = '0'+ (tlen / pow10[n]);
	tlen %= pow10[n];
    }

    *jc++ = ' ';

    memcpy(jc,ic,nlen);		/* the data */
    ic += nlen;
    jc += nlen;
    packetLength = (int) (jc - packetBuffer);
    *jc++ = (char) NULL;
    
    if (debug > 6) {
	fprintf(stderr, "%s  %s:  %d<-smsg(", progName, LPStime(),ServerFD);
	/* Using fwrite to output the packet allows for imbeded NULLs */
	fwrite(packetBuffer, sizeof(char), packetLength, stderr);
	fputs(")\n", stderr); }     /* put closing parentheses on output */
 
#ifndef VMS
    signal(SIGPIPE, SIG_IGN);
#endif VMS
    if (debug > 7 ) {
	if (packetLength > DECNET_MAX_FRAME) {
	    fprintf(stderr,"%s  %s:  assertion failed, send(%d chars)\n",
		progName, LPStime(), packetLength);
	}
    }

#ifndef VMS
    if (send(ServerFD, packetBuffer, packetLength, sendFlag) < 0) {
	sprintf(errbuf,"%s  %s:  LP_C_send(%d) failed",
		progName, LPStime(),ServerFD);
	perror(errbuf);
	if (jump_write_error) {
	    longjmp(write_error, ServerFD);
	}
    }
#else VMS
    /*
     * The "send" should be "non-blocking", but for now, implement the same
     * as it is in ULTRIX.
     */
    WriteStatus = SYS$QIOW(0, fd_map[ServerFD], IO$_WRITEVBLK, &iosb, 0, 0,
	&packetBuffer, packetLength, 0, 0, 0, 0);
    if (WriteStatus != SS$_NORMAL) {
	fprintf(stderr,"%s  %s:  qiow(write chan = %d) failed, status = %d\n",
	        progName,LPStime(),ServerFD,WriteStatus);
	exit(0);
    }
    WriteStatus = iosb[0];
    if (WriteStatus != SS$_NORMAL) {
	fprintf(stderr,"%s  %s:  qiow(write chan = %d) failed, status = %d\n",
	        progName,LPStime(),ServerFD,WriteStatus);
	exit(0);
    }
#endif VMS    


/* Check to see if we fragmented, and do something about it */

    if (mlen > nlen) {
	if (debug > 7)
	    fprintf(stderr,"%s  %s:  frame fragmented.\n",
		progName, LPStime());
        LP_C_send(ServerFD, Opcode, ic, ID, mlen-nlen);
    }
#ifdef VMS
    if (debug > 7)
	fprintf(stderr,"Exiting LP_C_send, ast_completed[%d] = %d\n", ServerFD,
	    ast_completed[ServerFD]);
#endif VMS
    return(ID);
}

/*
 *
 *		L P _ C _ s e n d _ u r g e n t
 *
 * This routine composes and sends an urgent message to the print server.
 * It is identical to LP_C_send (see above) save that it sends an OP_NULL
 * with the urgent flag turned on before sending the real message. This
 * Is accomplished by setting the MSG_OOB flag bit in the socket. Note that
 * the bytes that are sent as "urgent" are not actually the ones that we
 * are trying to expedite, but are, rather, a "sentinel" that comes before
 * them to clear out the path.
 *
 * Inputs:		ServerFD (integer)
 *			Opcode (string)
 *			mdata (string)
 *			ID (integer)
 *			mlen (integer)
 *
 * Returns:		ID (integer)
 *
 */


int LP_C_send_urgent(ServerFD, Opcode, mdata, ID, mlen)
int ServerFD;
char *Opcode, *mdata;
int ID,mlen;
{
    int retval;
#ifndef VMS
    sendFlag = MSG_OOB;		/* enable TCP Urgent Data */
#endif VMS
    LP_C_send(ServerFD, OP_NULL, " ", 0, 1);
    sendFlag = 0;		/* disable TCP Urgent Data */
    retval=LP_C_send(ServerFD, Opcode, mdata, ID, mlen);
    return(retval);
}

/*
 *		L P _ C _ c l o s e
 *
 * Shut down a communication channel and release all of the resources
 * associated with it.
 *
 * Inputs:		int FD		FD to close
 * Outputs: 		none
 * Returns:		none
 * Action:		close the FD and release the buffers
 */

void LP_C_close(FD)
int FD;
{
    int status;

    if (FD < 0) return;
#ifndef VMS
    close(FD);
#else VMS
    unmap_fd(FD);                  /* Mark "ULTIRX" FD empty */ 
    status = sys$dassgn(fd_map[FD]);
    if (debug > 0) {
	fprintf(stderr,"%s  %s:  deassigned channel = %d, status is...\n",
	    progName, LPStime(), fd_map[FD]);
	get_message(status);		/* get message string into errbuf */
	fprintf(stderr,"%s\n",errbuf);
	fflush(stderr);
	fsync(fileno(stderr));
    }
#endif VMS
    free(inputBuffers[FD]);
    inputBuffers[FD] = (char *) 0;
    lastByteIn[FD] = -1;
    probeCache[FD] = FALSE;
    return;
}
 
/*
 *		A s s i g n D i r e c t
 *
 * This routine is called to establish a direct connection to the printer.
 * 
 * Inputs:		nodename		string name of printer node
 * Outputs:		none
 * Returns:		socket FD open on printer 
 * Actions:		allocates FD, opens connection to printer,
 *			and binds that FD to the connection.
 *
 */
AssignDirect(nodename)
char *nodename;
{
	return(open_channel(nodename));
}

/*
 *		E n a b l e K e e p a l i v e
 *
 * Turn on the TCP socket option that does a "keep alive" protocol between
 * the two ends of a socket. We turn it on here in a separate subroutine,
 * rather than when the socket is created, because there are so many different
 * places where connections are made and it's easier just to wait until the
 * connection is made and then set the option.
 *
 * Inputs:		AFD		the socket file descriptor
 * Outputs:		none
 * Returns:		none
 *
 */
EnableKeepalive(AFD)
int AFD;
{
    int one=1;
    int retval;

#ifndef VMS
    retval=setsockopt(AFD, SOL_SOCKET, SO_KEEPALIVE, &one, sizeof(int));
    if (retval == 0) return;
    fprintf(stderr,"%s  %s:  setsockopt(%d) failed to enable keepalive.\n",
	progName, LPStime(), AFD);
#endif VMS
    return;
}


/* 
 *
 *		f a k e l o g 1 0
 *
 * Quick and dirty integer log10
 *
 * Inputs:		int N
 * Outputs:		none
 * Returns:		ceiling(log10(N))
 * Side effects:	none
 */
int fakelog10(N)
int N;
{
    if (N<10) return(1);
    if (N<100) return(2);
    if (N<1000) return(3);
    if (N<10000) return(4);
    if (N<100000) return(5);
    if (N<1000000) return(6);
    if (N<10000000) return(7);
    if (N<100000000) return(8);
    if (N<1000000000) return(9);
    return(13);			/* log10(32-bit infinity) */
}

/*
 *		g e t v a r
 *
 * Get the value of a variable out of a frame symbol list, with exactly
 * the same mechanism as "getenv" uses to get a variable out of the
 * environment.
 *
 * Inputs:		char *envstring		list to pick apart
 *			char *target		var name to search for
 * Outputs:		none
 * Returns		char *result
 *
 * Action: searches "envstring" for a name that matches "target".
 * (envstring is a string like "NAME=value^ANAME1=value1")
 * Getvar returns a pointer to the first character of "value" if "target"
 * matches "NAME". If no match is found, it returns NULL;
 *
 * The returned value is a pointer to a static string inside getvar, and
 * you must use it or copy it before calling getvar again.
 *
 */
char *getvar(envstring, target)
char *envstring;
char *target;
{
    int i,j,k,l;
    int slen;			/* length of source string "envstring" */
    int tlen;			/* length of target string "target" */
    static char valueString[DECNET_MAX_FRAME];

    slen = strlen(envstring);
    tlen = strlen(target);
    for (i=0; i<=slen; i++) {	/* for each variable */
	for (j=0; j<=tlen; j++) {
	    if ((target[j] == (char) NULL) &&
	        (envstring[i+j] == '=')) {
   /* it matches. Copy to static string and return a pointer to it. */
		    l=0;
   		    for(k=i+j+1; k<=slen; k++) {
		        valueString[l++] = envstring[k];
			if ((envstring[k] == RELAY_ARG_SEP) ||
			    (envstring[k] == (char) NULL)) break;
		    }
		    valueString[--l] = (char) NULL;
		    return(valueString);
	    }
	    if (target[j] == envstring[i+j]) continue; /* so far, so good */
   /* mismatch. abort inner loop and fast-forward outer loop */
   	    for (k=i; k<=slen; k++) {
	        if ((envstring[k] == RELAY_ARG_SEP)) {
		    i = k;
		    break;
		}
		if ((envstring[k] == (char) NULL)) {
		    return ((char *) NULL);
		}
	    }
	    break;
	}
    }
    return( (char *) NULL);
}

/*
 *
 *		s m a t c h
 *
 * String matcher. Return a pointer to the location at which string b
 * is found as a substring in string a, or else return null if it is not
 * found.
 *
 * Before you get excited that this is not a Boyer-Moore matcher, or some
 * other high-powered scheme, please look at the places where it is used
 * and realize that it doesn't need to be fast.
 *
 * Inputs		char *a; char *b;
 * Outputs:		none
 * Result:		pointer to location in a of first char of b
 * Side effects:	none
 *
 */
char *smatch(a,b)
char *a,*b;
{
    int alen,blen,i,j;

    alen = strlen(a);
    blen = strlen(b);
    if (blen > alen) return((char *) NULL);
    if (alen == 0 && blen == 0) return(a);
    for (j=0; j<=alen; j++) {
        for (i=0; i<blen; i++) {
	    if (a[j+i] != b[i]) goto breaker;
	}
	return(&a[j]);
breaker: ;
    }
    return((char *) NULL);
}
