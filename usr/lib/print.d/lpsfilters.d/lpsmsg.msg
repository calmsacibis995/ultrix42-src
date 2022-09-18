.TITLE		LPS Error Messages
.FACILITY	LPS,1017/SYSTEM
.IDENT		'VERSION 3.0-45'

!BOGUS VERSION WITH GABBE STUFF PUT AT BOTTOM
!++ edit history
!
!  3.0-45	29-Jun-1988 17:16  EL   PCO 335 - Add new LPS20, LPS40E
!		and PostScript messages
!  3.0-36	10-Jun-1988 10:33 JDJ	PCO 322 - add returnstatus offset
!		and added UNKMSG
!  3.0-28	04-May-1988 10:24 JDJ	PCO 317 - add CFGERROR & PRSERVICE
!  1.0-39	23-Apr-1987 13:20 APK	PCO 233 - add CANTCHECKPNT
!  1.0-32	14-Apr-1987 11:00 MJD	PCO 212 - add NETRECOVER, RESUBMIT,
!	and NETFAIL messages.
!  1.0-19	22-Mar-1987 13:01 APK	PCO 195 - add LAYUPIGNORED message
!  1.0-14	16-Mar-1987 18:00 MJD   PCO 18  - add LJCEXPERR message
!  1.0-08	13-Mar-1987 17:00 MJD   PCO 67  - add EVTLOGDISAB message
!  1.0-07	13-Mar-1987 10:10 APK   PCO 181 - add PRNAME message
!  1.0-06       10-Mar-1987 17:50 MJD   PCO 18 - add PROTUNSUP, INTERINPROG
!		Move NOLAYUPDIR, NOLAYUPFIL, and SHEETLIMOBS to new area to
!		protect message file upward compatability.
!  1.0-05	06-Mar-1987 16:00 APK   PCO 178 - add NOLAYUPDIR, NOLAYUPFIL
!	and SHEETLIMOBS messges for symbiont
!  1.2-01 	18-Feb-1987 14:55 JZW   PCO 150 - 7 messages added for
!		error processing
!  1.2-00	05-Feb-1987 09:30 JZW	PCO 12  - Added ATOPMN message
!  0.11-31      30-Oct-1986 07:56 MJD   PCO 136 - Add REBOOTREQD message.
!  0.11-20      23-Oct-1986 07:56 MJD   PCO 92 - Add JOBIDENT message.
!  0.11-19	23-Oct-1986 10:46 MJD   PCO 126 - Add NOLOGNAM message.
!  0.11-18      20-Oct-1986 15:30 RJD   Add DECNETOFF and RSMNETERR
!  0.10-42      04-SEP-1986 08:39 JZW   NOACTJOB MADE AN ERROR severity
!		so that it will work
!  0.10-34	26-Aug-1986 18:21 MJD	Update DISABLED message (for PCO 25)
!  0.10-24	24-Aug-1986 16:27 MJD   Add DISABLED (for PCO 25).
!  0.10-20	20-Aug-1986 15:40 WEM	Add RSMDISC
!  0.9-9	31-Jul-1986 10:17 WEM	Add UNRCHABL, NONETMBX, UNDERLEN,
!	and NOACTJOB.
!  0.9-8	18-Jul-1986 10:42 WEM	Add NOCDBAVL and CDBEOFFN for better
!	reporting of CDB failures.
!  0.9-7	16-Jul-1986 15:38 WEM	Add PSWDNOTVER and PRVCONEXC messages.
!	PRVCONEXC replaces STNREJREQ, and PSWDNOTVER provides additional info
!	during attempts to change the server's password
!  0.9-6	10-Jul-1986 15:32 WEM	Run SPELL on message file;  catch
!	spelling errors in ABORTSRVMGT and INVOBJID message texts.
!  0.9-5	01-Jul-1986 14:52 WEM	Reinstate LPS$_INVCTRBLK, for use by
!	PostScript
!  0.9-4	20-Jun-1986 11:58 WEM	Add fao_count to cantusetrn
!  0.9-3	19-Jun-1986 08:21 WEM	Reinstate certain resource messages,
!	make a few minor text changes.
!  0.9-2	19-Jun-1986 07:35 WEM   Add RONOCONN, fix typos in message
!	descriptions, change severity of PSWDNOTOK (error to warning) and
!	CLSERRLOG (error to inform).
!  0.9-1	13-Jun-1986 10:10 WEM	Add CANTUSETRN, reinstate SENDTIMOUT,
!	CANTHAPPEN, and INVDATA, fix many typos and inconsistencies.
!  0.9		01-Jun-1986 00:00 WEM	Reorganization for Baselevel 9.
!	Obsolete messages removed, all remaining messages fully documented.
!
!-- end of edit history

!*****************************************************************************
!*									     *
!*  COPYRIGHT (c) 1986, 1987, 1988			     		     *
!*  by DIGITAL EQUIPMENT CORPORATION, Maynard, Mass.			     *
!*									     *
!*  This software is furnished under a license and may be used and  copied   *
!*  only  in  accordance  with  the  terms  of  such  license and with the   *
!*  inclusion of the above copyright notice.  This software or  any  other   *
!*  copies  thereof may not be provided or otherwise made available to any   *
!*  other person.  No title to and ownership of  the  software  is  hereby   *
!*  transferred.							     *
!*									     *
!*  The information in this software is subject to change  without  notice   *
!*  and  should  not  be  construed  as  a commitment by Digital Equipment   *
!*  Corporation.							     *
!*									     *
!*  Digital assumes no responsibility for the use or  reliability  of  its   *
!*  software on equipment which is not supplied by Digital.		     *
!*									     *
!*****************************************************************************
                                             

 
.LITERAL LPS$$LPS__ENG__BASE = 33
.LITERAL LPS$$LPS__POST__BASE = 545
.LITERAL LPS$$LPS__MGMT__BASE = 1057
.LITERAL LPS$$LPS__START__BASE = 1569
.LITERAL LPS$$LPS__RES__BASE = 2081
.LITERAL LPS$$LPS__PSDS__BASE = 2593
.LITERAL LPS$$CLI__PAR__BASE = 3105
.LITERAL LPS$$CLI__RTNSTAT__BASE = 3617
.LITERAL LPS$$LPS__DEVCTL__BASE = 3617
                                                                  
!++
!
! FACILITY:
!   LPS Print Server
!
! ABSTRACT:
!    This message source contains the source for the messages generated
!   by the Local area Print Server software.                  
!
! CREATION DATE: Aug 20, 1984
!
!--

!**********
! generic messages - used by various components of the print server
!**********

.BASE	1				!BASE_(LPS$_, 1, 1

.SEVERITY    SUCCESS

NORMAL		<Normal completion>
!++
! Used by RSM - RD
! Used by SM  - JW
! GENERATED BY:	All
! DISPLAYED TO:	Internal
! CAUSE:	Successful completion of some operation
! USER ACTION:	
! Author:
!--

SUCCESS		<Successful completion>
!++
! Used by RSM - RD
! Used by SM  - JW
! GENERATED BY:	Menu system
! DISPLAYED TO: System Manager
! CAUSE:	Requested operation succeeded
! USER ACTION:	
! Author:
!--

CONTINUE	<Continue>
!++
! Used by RSM - RD
! Used by SM  - JW
! GENERATED BY:
! DISPLAYED TO:	Int
! CAUSE:	
! USER ACTION:	
! Author:
!--

.SEVERITY    INFORMATION

!+++
!obsolete - JW
!BYTHEWAY 	<Additional Information: !AS >/FAO_COUNT=1
!INTERRFIX	<Internal Error Fixed>
!---

.SEVERITY    WARNING

STRTOOLON	<String is too long - it has been truncated>
!++
! GENERATED BY: Program Control
! DISPLAYED TO: User, Operator
! CAUSE:	A string passed internally is too large for the receiving
!		buffer.
! USER ACTION:	This is not a serious error, but it should not occur;  an
!		SPR should be submitted.
! AUTHOR:	WEM
!--

.SEVERITY    ERROR

.SEVERITY    FATAL

OSERROR		<Operating System error detected - error was: !UL>/FAO_COUNT=1
!++
! GENERATED BY: Postscript
! DISPLAYED TO: Operator
! CAUSE:	An irrecoverable Operating System error has been encountered by
!		the Postscript Translator in the PrintServer.
! USER ACTION:	Reboot the PrintServer.  Please submit an SPR.  
! AUTHOR:	MJD, 5/27/86
!--

PROBUNKNWN	<Problem Unknown>
!++
! GENERATED BY: Postscript  
! DISPLAYED TO: Operator
! CAUSE:	An irrecoverable printer error has been encountered by
!		the Postscript Translator in the PrintServer.
! USER ACTION:	Reboot the PrintServer.  Please submit an SPR.  
! AUTHOR:	MJD, 5/27/86
!--

REBOOTREQD	<Please reboot the PrintServer - FATAL ERROR>
!++
! GENERATED BY: LAPS Job Controller (LAPS$MONITOR process) 
! DISPLAYED TO: Operator
! CAUSE:	An irrecoverable error has been encountered by
!		the Postscript Translator in the PrintServer.
! USER ACTION:	Reboot the PrintServer.  Please submit an SPR.  
! AUTHOR:	MJD, 10/30/86
!--

!+++
!obsolete..
!INTERRACT    	<Internal Error Active>
!---
                  
!**********
! messages which are generated or fielded by PSDS
!**********

.BASE	LPS$$LPS__PSDS__BASE
 
.SEVERITY    SUCCESS

PENDING		<$QIO operation is pending>
!++
! GENERATED BY: Symbiont
! DISPLAYED TO: Internal
! CAUSE:	symlaps operation is pending.  the symbiont must wait on
!		an event flag and then complete the operation
! USER ACTION:	none
! Author:	MCL
!--

USERDATA	<!AS>/FAO_COUNT=1
!++
! GENERATED BY: PostScript.
! DISPLAYED TO: User.
! CAUSE:	Data generated by PostScript operators print, =, ==, etc.
! USER ACTION:	none
! AUTHOR:	WEM
!--

.SEVERITY    INFORMATIONAL

SYNC	 	<Last Printer Job Complete, Ready for next job>
!++
! GENERATED BY: Postscript.  Consumed by LAPS$MONITOR process.
! DISPLAYED TO: Internal.
! CAUSE:	Last Postscript job has finished.  Next job may begin.
! USER ACTION:	none
! AUTHOR:	MJD, 5/27/86
!--

JOBSTART	<Job !UL Start>/FAO_COUNT=1
!++
! GENERATED BY:	Symbiont
! DISPLAYED TO: User
! CAUSE:	Generated by symbiont when LAPS connection has been accepted
! USER ACTION:	none
! Author:	MCL
!--

JOBFINISH	<Job !UL Finish>/FAO_COUNT=1
!++
! GENERATED BY:	Symbiont, Laps Job Controller
! DISPLAYED TO: User, Operator
! CAUSE:	Generated by symbiont when LAPS connection has been closed.
!		Also Generated by Laps Job Controller when last message has
!		been sent to Symbiont.
! USER ACTION:	none
! AUTHOR:	MCL; Modified by MJD, 5/27/86
!--

JOBCONN		<Job !UL Connect>/FAO_COUNT=1
!++
! GENERATED BY:	Laps Job Controller
! DISPLAYED TO: Operator
! CAUSE:	Generated by Laps Job Controller when Job has been accepted
!		for processing by PrintServer.
! USER ACTION:	none
! AUTHOR:	MJD,  5/27/86
!--
!++
! GENERATED BY: LAPS
! DISPLAYED TO: Operator
! CAUSE:	DECnet connect
! USER ACTION:	n/a
! AUTHOR:	J. Z. Wolf
!--

PROCSTART	<Process !AS Started>/FAO_COUNT=1
!++
! GENERATED BY: Status collector,Laps, FFM
! DISPLAYED TO: Internal
! CAUSE:	Process successfully started up
! USER ACTION:	n/a
! AUTHOR:	J. Z. Wolf
!--

ABORTPEND	<Pending Job !UL Aborted>/FAO_COUNT=1
!++
! GENERATED BY: LAPS JOB CONTROLLER
! DISPLAYED TO: User, Operator *** ? Mike, is this true ? ***
! CAUSE:	A job which was in Pending state was deleted from the LAPS
!	        Job Queue by a Server Management Abort Job command.
! USER ACTION:  none
! AUTHOR:	MJD, 5/27/86
!--

SENDTIMOUT      <Timeout trying to send a message>
!++
! GENERATED BY: Server Management (via LPSUTIL)
! DISPLAYED TO: User. *** ? Mike, is this true ? ***
! CAUSE:	Server Management was unable to send a message to Printer within
!		a specific time period.  
! USER ACTION:  Retry operation.  
! AUTHOR:	MJD;   6/9/86
!--

!+++
!obsolete..
!TRYAGAIN	<Retry the last operation>
!PASSWORD	<The PostScript password is: !AS>/FAO_COUNT=1
!NOMORE		<No More>
!UNAVAIL		<Unavailable>
!---

PAUSE		<Printer is paused (job is pending)>
!++
! GENERATED BY: Print Server Job Controller (PC_SOJ)
! DISPLAYED TO: Operator
! CAUSE:	The printer has been paused, though use of the server
!		mgmt. command to put the server in single job mode.
! USER ACTION:  Use server mgmt. commands to either resume the printer
!		or place it in continuous mode.
! AUTHOR:	WEM - 5-Jun-1986
!--

RESUME		<Printer is resuming>
!++
! GENERATED BY: Print Server Job Controller (PC_SOJ)
! DISPLAYED TO: Operator
! CAUSE:	The printer is continuing, though use of the server
!		mgmt. commands to resume the printer.
! USER ACTION:  None.  The server will pause on every job until the server
!		is placed into continuous mode.
! AUTHOR:	WEM - 5-Jun-1986
!--

CONNFAIL	<Unable to connect to print server !AS >/FAO_COUNT=1
!++
! GENERATED BY:	Symbiont - in routine LPS$OPEN
! DISPLAYED TO: User, Client Operator
! CAUSE:	Generated by symbiont when there is an error when trying
!		to generate a DNA or LAPS connection to the Printer.  The
!		actual error generated will be displayed as a continuation
!		of this error.  This error is necessary to lower the severity
!		of the connection error, so that the symbiont will continue
!		to try to connect to the PrintServer and not abort the job.
! USER ACTION:	Depends on the error that follows the CONNFAIL message.
! AUTHOR:	Mary C. Lantiegne - LAS Engineering
!--

.SEVERITY    WARNING

BADPKTTYP   	<Invalid packet type>
!++
! GENERATED BY: PC_dispatch. Consumed by LAPS JOB CONTROLLER.
! DISPLAYED TO: Internal
! CAUSE:	A packet sent to Printer Control did not have the proper
!		data type set in its packet header. 
! USER ACTION:  none
! AUTHOR:	MJD, 5/27/86
!--

!obsolete...
!NOUSERID	<Client did not provide User ID>
!USRDATLOST	<User data reply packet deleted due to error>
!BADDATCLS	<Invalid data class in packet header>
!EOJTOOSOON	<Packet to send after EOJ already sent>

PAUSEABORT	<Job aborted - Printer is still paused>              
!++
! GENERATED BY: Print Server Job Controller (PC_SOJ)
! DISPLAYED TO: Operator
! CAUSE:	A job abort has been issued for a job waiting for the printer
!		to be resumed by server mgmt.
! USER ACTION:  None.  The server will pause on every job until the server
!		is placed into continuous mode.
! AUTHOR:	WEM - 5-Jun-1986
!--

ACCPTDISA	<Job acceptance is disabled - job rejected>
!++
! GENERATED BY: Print Server Job Controller (PC_ACCEPTJOBS)
! DISPLAYED TO: Operator
! CAUSE:	The print server is not accepting jobs, and an attempt
!		to connect to it has been rejected.
! USER ACTION:  Enter the server mgmt. command to enable job acceptance.
! AUTHOR:	WEM - 5-Jun-1986
!--

.SEVERITY    ERROR


SCINTERR        <SC - Internal Error>
!++
! GENERATED BY: status collector, message handler
! DISPLAYED TO: error_logger, operator
! CAUSE:	Some exception has occurred that  could not be handled
! USER ACTION:	may have to reboot if severe enough otherwise will resolve
!		itself
! AUTHOR:	J. Z. Wolf
!--

NOPRTCRT        <Unable to create port !AS in Status Collector>/FAO_COUNT=1
!++
! GENERATED BY: Status Collector
! DISPLAYED TO: Operator, Error_logger
! CAUSE:	Was unable to open one of its ports
! USER ACTION:	Will have to reboot
! AUTHOR:	J. Z. Wolf
!--

NOPRCCRE        <Unable to create process !AS>/FAO_COUNT=1
!++
! GENERATED BY: status Collector
! DISPLAYED TO: Operator
! CAUSE:	status collector could not start the message handler process
! USER ACTION:	reboot
! AUTHOR:	J. Z. Wolf
!--
!++
! GENERATED BY:	LAPS JOB CONTROLLER
! DISPLAYED TO: Operator
! CAUSE:	Resource error prevented creation of LAPS$PROTOCOL process
! USER ACTION:	Submit SPR with description of your PrintServer configuration
!		and the circumstances under which this error occurred.
! AUTHOR:	MJD, 5/27/86
!--

INVCONRES   	<Invalid print server response to connect>
!++
! GENERATED BY: SYMLAPS 
! DISPLAYED TO: User
! CAUSE:        PrintServer returned an invalid response to the Symbiont's
!		connect request.
! USER ACTION:	Please submit SPR with description of your PrintServer 
!               configuration and the circumstances under which this 
!		error occurred.
! AUTHOR:	MJD, 5/27/86
!--

STNREJREQ   	<Print server rejected connect request>
!++ 
! GENERATED: LPSCDBUTIL 
! DISPLAYED: Remote Server Manager 
! CAUSE    : attempt to connect to the print server failed 
! ACTION   : try later 
! 
! RJD  23-May-1986 
!-- 

LPSPCLERR   	<Job aborted due to print server protocol error>
!++
! GENERATED BY:  SYMLAPS 
! DISPLAYED TO:  User
! CAUSE:	 PrintServer *** ? Mike, please fill this in ? ***
! USER ACTION:	 Please submit SPR with description of your PrintServer 
!                configuration and the circumstances under which this 
!		 error occurred.
! AUTHOR:	 MJD, 5/27/86
!--

BADVERSION	<Protocol version is not supported>
!++
! GENERATED BY:  SYMLAPS 
! DISPLAYED TO:  User
! CAUSE:	 Symbiont and PrintServer LAPS Protocol versions are not compatible.
! USER ACTION:   Install compatible version of Symbiont and PrintServer.
! AUTHOR:	 MJD, 5/27/86
!--

INVJOBTER   	<Invalid print server to response to end-of-job>
!++
! GENERATED BY:  SYMLAPS 
! DISPLAYED TO:  User
! CAUSE:	 Server sent invalid response to LAPS end-of-job message.
! USER ACTION:	 Please submit SPR with description of your PrintServer 
!                configuration and the circumstances under which this 
!		 error occurred.
! AUTHOR:	 MJD, 5/27/86
!--
                         
JOBABORT	<Job !UL Aborted>/FAO_COUNT=1
!++
! GENERATED BY: LAPS/postscript
! DISPLAYED TO: Operator *** ? Jeff, is this correct ? ***
! CAUSE: LAPS/postscript successfully aborted a job
! USER ACTION: n/a
! AUTHOR:  J. Z. Wolf
!--

! GENERATED BY:	LAPS Job Controller
! DISPLAYED TO: User (at Symbiont-level) *** ? Mike, is this correct ? ***
! CAUSE:	The indicated print job was aborted for one of several reasons:
!		 (1) User deleted an active job via a Symbiont command, or
!		 (2) Postscript requested that job be aborted due to
!		     a printer error or an internal error, or
!		 (3) The Logical Link between the PrintServer and Symbiont was 
!		     broken, or 
!		 (4) the Symbiont process crashed, or
!		 (5) Internal error occurred in the LAPS Job Controller.
! USER ACTION:  Identify which of these problems caused the abort to occur and
!		take the appropriate action:
!		 (1) None.
!		 (2) Another error message will accompany the abort.  This
!		     error message will give you more information about the
!		     required action.
!		 (3) Check the DECnet setup between the PrintServer and
!		     the Symbiont.
!		 (4),(5) Submit an SPR.
! AUTHOR:	MJD,  5/27/86
!--

ABORTSRVMGT     <Job !UL Aborted by Server Management>/FAO_COUNT=1
!++
! GENERATED BY:	LAPS JOB CONTROLLER
! DISPLAYED TO: User *** ? Mike, is this correct ? ***
! CAUSE:	Job was aborted via Server Management Abort Job command.
! USER ACTION:	none
! AUTHOR:	MJD,  5/27/86
!--

NOJOB		<No Such Job>
!++
! Used by RSM - Used by other components??? - RD
!--
!++
! GENERATED BY:	LAPS JOB CONTROLLER, Menu System
! DISPLAYED TO: Server Mgr. (by Server Mgmt)
! CAUSE:	User attempted to abort job from Server Mgmt which did
!		not exist when the abort request was received by the
!		LAPS Job Controller
! USER ACTION:	none
! AUTHOR:	MJD, 5/27/86
! AUTHOR:	J. Z. Wolf
!--

IDENTTIMOUT	<Controller ident message never received>	    ![0.5-6]
!++
! GENERATED BY:	LAPS JOB CONTROLLER
! DISPLAYED TO: Operator
! CAUSE:	Print Controller identification information not received from
!		PostScript within 15 second startup time.  This information
!		is used for the SHOW PRINTSERVER CHARACTERISTICS menu option.
! USER ACTION:	If other error msgs accompany this error, analyze error based
!		on those messages.  This message by itself does not indicate
!		a serious failure.
! AUTHOR:	MJD, 5/27/86
!--

EVLLNKLST	<Link lost to Supporting Host, disabling event logging>
!++
! GENERATED BY: message handler
! DISPLAYED TO: operator        ............
! CAUSE:	A failure has occurred in communications with the Supporting
!		Host.  It may be due to some problem with the network, or
!		the Supporting Host may have crashed.
! USER ACTION:	None.  Event logging may be re-enabled after the problem
!		has been corrected. 
! AUTHOR:	J. Z. Wolf
!--

!+++
!obsolete..
!PAGLIMEXC  	<User requested higher page limit than permitted>
!EOJOOS	    	<Status or Data received after end-of-job>	    ![0.5-6]
!IDENTOOS    	<Controller ident message received out of sequence> ![0.5-6]
!EOJNOJOB	<End of Job received but job not executing>	    ![0.5-6]
!ABONOJOB	<Abort Job received but job not executing>	    ![0.5-6]
!RESNOJOB	<Resource request received but job not executing>   ![0.5-6]
!---
 
.SEVERITY    FATAL

PROCABORT	<Process !AS aborted>/FAO_COUNT=1
!++
! GENERATED BY:	Any process in a failure state
! DISPLAYED TO:	operator, error_logger
! CAUSE:	Fatal error  encountered, process in a state where it can not continue
! USER ACTION:	Check error and reboot
! AUTHOR:	J. Z. Wolf
!--

LJCINTERR	<Laps Init Process error, reason = !UL>/FAO_COUNT=1
!++
! GENERATED BY: LAPS Job Controller
! DISPLAYED TO: Operator *** ? Mike, is this correct ? ***
! CAUSE:	LAPS Job Controller's Initialization component encountered 
!		an error during startup or while processing a client 
!		connect request.
! USER ACTION:	Reboot the PrintServer.  Please submit an SPR.  
! AUTHOR:	MJD, 6/3/86
!--

LJCMONERR	<Laps Monitor Process error, reason = !UL>/FAO_COUNT=1
!++
! GENERATED BY: LAPS Job Controller
! DISPLAYED TO: Operator *** ? Mike, is this correct ? ***
! CAUSE:	LAPS Job Controller's Monitor component encountered 
!		an error.
! USER ACTION:	Reboot the PrintServer.  Please submit an SPR.  
! AUTHOR:	MJD, 6/3/86
!--

LJCPROTERR	<Laps Protocol Process error, reason = !UL>/FAO_COUNT=1
!++
! GENERATED BY: LAPS Job Controller
! DISPLAYED TO: Operator *** ? Mike, is this correct ? ***
! CAUSE:	LAPS Job Controller's protocol component encountered 
!		an error during processing a print job.
! USER ACTION:	Reboot the PrintServer.  Please submit an SPR.  
! AUTHOR:	MJD, 6/3/86
!--

TRNFAIL		<Translation from !AS to PostScript failed.>/FAO_COUNT=1
!++                                                               
! GENERATED BY:	Symbiont
! DISPLAYED TO: User, Client Operator
! CAUSE:	Generated by symbiont when the translator from data_type
!		to Postscript generates any severe error that is captured
!		by the Symbiont's exception handler.  The symbiont will
!		not call the translator again!
! USER ACTION:	Submit an SPR.  Include the file that the user was trying
!		to PRINT and the PRINT request.
! AUTHOR:	Mary C. Lantiegne - LAS Engineering
!--

CANTUSETRN	<Translator from !AS to PostScript is unusable.>/FAO_COUNT=1
!++
! GENERATED BY: Symbiont
! DISPLAYED TO: User, Client Operator
! CAUSE:	If the translator has, at any time, generated a severe error,
!		it will not be called again.  This is error is generated
!		if someone tries to print a file that causes that translator
!		to be called.
! USER ACTION:	The manager should restart the queue.  Any jobs that generated
!		the LPS$_cantusetrn error will be in a holding state.  The
!		manager should remove them from that holding state once
!		the queue has been restarted.
! AUTHOR:	Mary C. Lantiegne - LAS Engineering
!--

.SEVERITY   WARNING


INVCTRBLK	<Invalid Control Block received by controller>
!++
! GENERATED BY: PostScript
! DISPLAYED TO: Internal
! CAUSE:	An invalid control block has been sent to PostScript.  This
!		is an internal failure which is not serious.
! USER ACTION:	None.  An SPR should be sent if this message ever appears on
!		the console or in the event log.
! AUTHOR:	Gene Morgan
!--

DISABLED    	<Print server is currently disabled and cannot accept jobs>
!++
! GENERATED BY: Client
! DISPLAYED TO: User
! CAUSE:	Job acceptance is disabled on the PrintServer.  Client will
!	        continue attempting to connect to PrintServer.
!		
! USER ACTION:	Re-enable Job acceptance from Server Management.
! AUTHOR:	Michael J Dailey;   24-Aug-1986
!--

.SEVERITY INFORMATIONAL
!++
! GENERATED BY:	LAPS JOB CONTROLLER
! DISPLAYED TO: Error log only
! CAUSE:	Print job has started.  Informational msg which always follows
!               %LPS-I-JOBCONN message.
! USER ACTION:	None - normal message.
! AUTHOR:	Michael J. Dailey;  23-Oct-1986
!--

JOBIDENT   <Server Job=!UL   Client Job=!UL   User=!AS>/FAO_COUNT=3


.SEVERITY WARN 
NOLOGNAM  <cannot translate logical SYS$NODE (DECnet not running)>
!++
! GENERATED BY:	Symbiont
! DISPLAYED TO: Client Operator
! CAUSE:	Generated by symbiont when the DECnet is not yet running.
! 
! USER ACTION:	Start DECnet.
!
! Author:	MJ Dailey; 10/23/86
!--

.SEVERITY FATAL

PSSFTERR	<Fatal PrintServer Software Error>
!++
! GENERATED BY:	Status Collector/Message Handler
! DISPLAYED TO:	Console
! CAUSE:	generated by sc or mh when an event has occurred from which
!		it is impossible or ludicrus (crash is imminent)
! USER ACTION:	Reboot system. MOst problems of this kind are one of a kind
!		If persists or happens frequently then submit an SPR.
! AUTHOR:	Jeffrey Z. Wolf - Hardcopy Engineering (DPS)
!--

.SEVERITY WARN

UNCMPLTRNMH	<Message handler Unable To Complete Transaction, Message May Be Lost>
!++                             
! GENERATED BY:	Message Handler
! DISPLAYED TO:	Error Logger
! CAUSE:	Processing of a status update could not be completed.
!		Possible reasons:
!		Unable to receive the status message (Receive failed)
!		Unable to send/forward the status message (Send failed)
! USER ACTION:	If seen infrequently - None. If seen frequently, submit
!		an SPR
! AUTHOR:	Jeffrey Z. Wolf - Hardcopy Engineering (DPS)
!--

UNCMPLTRN	<Status Process Unable To Complete Transaction, Message May Be Lost>
!++
! GENERATED BY:	Status Collector
! DISPLAYED TO:	Error Logger
! CAUSE:	Processing of a status update could not be completed.
!		Possible reasons:
!		Unable to receive the status message (Receive failed)
!		Unable to send/forward the status message (Send failed)
! USER ACTION:	If seen infrequently - None. If seen frequently, submit
!		an SPR
! AUTHOR:	Jeffrey Z. Wolf - Hardcopy Engineering (DPS)
!--

UNCMPLQTRN	<Query Process Unable to Complete Transaction, Response May Be Lost>
!++
! GENERATED BY:	Status Collector
! DISPLAYED TO:	Console
! CAUSE:	Processing of a query (initiated b Server Management) Could
!		Not be completed. Possible Reasons:
!		Incomming connection not accepted
!		Receive of query Failed
!		Unable to Send response
! USER ACTION:	If seen infrequently - None. If Seen frequently submit
!		an SPR
! AUTHOR:	Jeffrey Z. Wolf - Hardcopy Engineering (DPS)
!--

UNKMSG   	<Invalid message received from PrintServer>
!++
! GENERATED BY: SYMLAPS 
! DISPLAYED TO: User
! CAUSE:        PrintServer returned a message outside the range of the
!		LPS facility.
! USER ACTION:	Ensure that no invalid PostScript returnstatus operator
!		is contained within the print job.
! AUTHOR:	JDJ, 6/10/88
!--

!++!**********
! messages which are generated or fielded by resource management
!**********

.BASE	LPS$$LPS__RES__BASE

.SEVERITY    SUCCESS

RESINPROG	<Resource Fault in progress>
!++
! GENERATED BY:	FFM Remote
! DISPLAYED TO: event log
! CAUSE:	Record progress of font fault.
! USER ACTION:	If error msgs follow this error, analyze error based
!		on those messages.  
! AUTHOR:	WEM
!--

PSWDOK		<Valid password presented to print server>
!++
! GENERATED BY:	FFM Info
! DISPLAYED TO: Operator.
! CAUSE:	A PostScript program presented a valid password to the Print
!		Server.
! USER ACTION:	Unauthorized use of the Print Server password by PostScript
!		programs can jeopardize the print jobs which follow.
! AUTHOR:	WEM
!--

RESAVAIL	<Resource !AS is available>/FAO_COUNT=1
!++
! GENERATED BY:	FFM Info
! DISPLAYED TO: Internal
! CAUSE:	A resource requested by PostScript is available.
! USER ACTION:	None.
! AUTHOR:	WEM
!--

!future...
!TRAYSWITCH	<Requested paper tray is now available>
!---

.SEVERITY    INFORMATIONAL

RESFAULT	<Resource Fault - !AS resource needed>/FAO_COUNT=1
!++
! GENERATED BY:	PostScript
! DISPLAYED TO: Internal, event log.
! CAUSE:	A PostScript program is requesting a file-type resource
!		(a font or other postscript-oriented file)
! USER ACTION:	none.
! AUTHOR:	WEM
!--

PSWDFAULT	<Password Fault - password verification request>
!++
! GENERATED BY:	PostScript
! DISPLAYED TO: Internal, event log.
! CAUSE:	A PostScript program is requesting the password to be verified.
! USER ACTION:	none.
! AUTHOR:	WEM
!--

TRAYFAULT	<Tray Fault - tray switch request>
!++
! GENERATED BY:	PostScript
! DISPLAYED TO: Internal, event log.
! CAUSE:	A PostScript program is requesting operator intervention
!		for the handling of trays.
! USER ACTION:	none.  FFM, which gets the request, will initiate a dialog
!		with the operator separate from this message.
! AUTHOR:	WEM
!--

.SEVERITY    WARNING

RESUNAVAIL	<Resource !AS Unavailable>/FAO_COUNT=1
!++
! GENERATED BY:	FFM
! DISPLAYED TO: Internal, event log.
! CAUSE:	A file-type resource (a font or other postscript-oriented file)
!		requested by PostScript is not available.
! USER ACTION:	none.
! AUTHOR:	WEM
!--

RESCMPLTE	<Resource !AS Complete>/FAO_COUNT=1
!++
! GENERATED BY:	PostScript
! DISPLAYED TO: Internal, event log.
! CAUSE:	A file-type resource (a font or other postscript-oriented file)
!		has been successfully transferred between PSDS and PostScript.
! USER ACTION:	none.
! AUTHOR:	WEM
!--


!+++
!future...
!RESSUB		<Resource !AS Substitute>/FAO_COUNT=1
!RESSUGG	<Resource substitution - new string is !AS>/FAO_COUNT=1
!future...                                                                          
!TRAYNOSWITCH	<Requested paper tray change is denied>
!TRAYDEFAULT	<Requested paper tray change is denied - using default tray>
!TRAYTIMEOUT	<Requested paper tray change not serviced within timeout period>
!---

.SEVERITY    ERROR

RESERR		<Resource !AS error>/FAO_COUNT=1
!++
! GENERATED BY:	FFM Remote
! DISPLAYED TO: Internal to PostScript, event log.
! CAUSE:	An error has been detected in the processing of a resource
!		fault request (see LPS$_RESFAULT)
! USER ACTION:	none.
! AUTHOR:	WEM
!--

RESNOSERV	<No Resource Server is currently available>
!++
! GENERATED BY:	FFM Remote
! DISPLAYED TO: Internal to PostScript, event log.
! CAUSE:	A resource server is not available to service a resource
!		fault request (see LPS$_RESFAULT)
! USER ACTION:	none.
! AUTHOR:	WEM
!--

RESABORT     	<Resource !AS abort>/FAO_COUNT=1
!++
! GENERATED BY:	PostScript
! DISPLAYED TO: Internal to PostScript and FFM, event log.
! CAUSE:	A resource was not successfully loaded (see LPS$_RESFAULT)
! USER ACTION:	none.
! AUTHOR:	WEM
!--

!+++
!future...
!TRAYABORT	<Requested paper tray change failed.>
!---

.SEVERITY    WARNING

PSWDNOTOK	<Invalid password presented to print server>
!++
! GENERATED BY:	FFM Info
! DISPLAYED TO: Operator, event log.
! CAUSE:	A PostScript program presented an invalid password to the Print
!		Server.
! USER ACTION:	A user may be trying to "break into" the print server.
!		Unauthorized use of the Print Server password by PostScript
!		programs can jeopardize the print jobs which follow.
! AUTHOR:	WEM
!--

!**********
! these messages are generated by each major component of the
! print server and sent to the initialization process within PSDS
! the print server will not start accepting jobs until each
! component has sent its own initialization message
!**********

.BASE	LPS$$LPS__START__BASE 

.SEVERITY    INFORMATIONAL

SCSTART         <Status Collector is started>
!++
! GENERATED BY:  status collector
! DISPLAYED TO:  INTERNAL - sent back to PSDS job to signal startup, used for
!		synchronization
! CAUSE:        Startup
! USER ACTION:  none
! AUTHOR:     J. Z. Wolf
!--

LAPSSTART	<LAPS Process started>     
!++
! GENERATED BY:	LAPS JOB CONTROLLER
! DISPLAYED TO: Internal - sent to psds_job
! CAUSE:	LAPS$INIT process has been started.
! USER ACTION:	none
! AUTHOR:	MJD, 5/27/86
!--

FFMSTART	<FFM Process started>
!++
! GENERATED BY: FontFile Manager
! DISPLAYED TO: INTERNAL - sent back to PSDS job to signal startup, used for
!		synchronization
! CAUSE:        Startup
! USER ACTION:  none
! AUTHOR:	WEM
!--

SMSTART		<Server Management started>
!++
! GENERATED BY:  Server management
! DISPLAYED TO:  INTERNAL - sent to psds-job
! CAUSE:         signals startup
! USER ACTION:   none
! AUTHOR:        J. Z. Wolf
!--
                
MSHDLRSTART	<Message Handler Process started>
!++                      
! GENERATED BY:  Message handler
! DISPLAYED TO:  INTERNAL - sent to psds_job
! CAUSE:         startup
! USER ACTION:   none
! AUTHOR:        J. Z. Wolf
!--

PRINTSTART	<Printing Subsystem started>
!++
! GENERATED BY:  PostScript
! DISPLAYED TO:  INTERNAL - sent to psds_job
! CAUSE:         startup
! USER ACTION:   none
! AUTHOR:        WEM
!--

!**********
! messages for handling of client-detected errors
!**********

.BASE	LPS$$CLI__PAR__BASE

.SEVERITY    ERROR

KEYNOTREC	<The keyword !AS in the /PARAMETERS qualifier was not recognized>/FAO_COUNT=1
!++
! GENERATED BY:	Symbiont
! DISPLAYED TO: User
! CAUSE:	Generated by symbiont when a unrecognized switch was parsed
!		from the /PARAMETER switch of the print command
! USER ACTION:	Check spelling and/or documentation about switch the user
!		is trying to enter and resubmit PRINT request with proper
!		/PARAMETER switch
! Author:	MCL
!--

PARSYNERR	<Syntax error in the /PARAMETERS qualifier at or near !AS >/FAO_COUNT=1
!++
! GENERATED BY:	Symbiont
! DISPLAYED TO: User
! CAUSE:	Generated by symbiont when there is a syntax error in the
!		/PARAMETER switch
! USER ACTION:	Check documentation and resubmit PRINT request with proper
!		syntax in /PARAMETER switch
! Author:	MCL
!--

DEVSYNERR	<Syntax error in /ON qualifier. !AS not valid print server name>/FAO_COUNT=1
!++
! GENERATED BY:	Symbiont
! DISPLAYED TO: User, Client Operator
! CAUSE:	Generated by symbiont if there is an error in parsing the
!		node name of the /ON switch of the INIT/QUEUE command
! USER ACTION:	Stop and delete the queue and then reinitialize the queue
!		with the proper node name.
! Author:	MCL
!--

INVCONSOLE      <Invalid Console Device>
!++
! GENERATED BY: Server management 
! DISPLAYED TO:  Operator
! CAUSE:         invalid console device found
! USER ACTION:   fix terminal settings to VT100/200 compatible terminal
! AUTHOR:       JZW
!--

TRNNOTFND	<Translator from !AS to PostScript not found >/FAO_COUNT=1
!++
! GENERATED BY:	Symbiont
! DISPLAYED TO: User, Client Operator
! CAUSE:	Generated by symbiont when the translator from data_type
!		to Postscript is not found in the SYS$LIBRARY directory
! USER ACTION:	check LPS$<queue-name>_PARAMETER and /PARAMETER switch of
!		PRINT command to be sure the "DATA_TYPE=<data-type>" is
!		a supported data_type or has a user written translator,
!		TRN$<data-type>_PS.EXE in the SYS$LIBRARY directory.
! Author:	MCL
!--


INTERINPROG	<Interrupt already in progress - request denied>/FAO_COUNT=1
!++
! GENERATED BY:	Symbiont
! DISPLAYED TO: Not Displayed, this is an internal status.
! CAUSE:	Generated by symbiont in LPS$interrupt_stream routine
!		when LPS$interrupt_stream has already been called during
!		current job.
! USER ACTION:	None.
! Author:	MJD; 3/10/87
!--

PROTUNSUP	<Function not supported in active LAPS protocol version>/FAO_COUNT=1
!++
! GENERATED BY:	Symlaps
! DISPLAYED TO: Not Displayed, this is an internal status.
! CAUSE:	Generated by Symlaps when current protocol version number
! 		does not support indicated function.
!               Currently only happens when both the following conditions are met:
!		(1) A LPS40 V1.2 symbiont connects to an LPS40 V1.0 Server 
!		and defaults to LAPS V0.3.
!		(2) LPS40 V1.2 symbiont calls LPS$Interrupt or 
!		LPS$interrupt_complete.
!		
! USER ACTION:	None.  Symbiont should handle this condition by starting
!		a V1.0 rundown sequence.
!
! Author:	MJD;  3/10/87
!--

.SEVERITY    ERROR

NOLAYUPDIR	<Layup definition area is not accessible>
!++
! GENERATED BY:	Symbiont
! DISPLAYED TO: User
! CAUSE:	Generated by symbiont when the translation of logical LPS$LAYUP
!		fail to access layup definition area.
! USER ACTION:	Check with system manager and ask him to define LPS$LAYUP
!		logical name pointing to layup_def area
! Author:	APK
!--

NOLAYUPFIL	<Layup definition module !AS is not accessible>/FAO_COUNT=1
!++
! GENERATED BY:	Symbiont
! DISPLAYED TO: User
! CAUSE:	Generated by symbiont when the user specified layup definition
!		module.LUP was not found in LPS$LAYUP area
! USER ACTION:	Check the spelling and/or the  existance of the specified 
!		module in LPS$LAYUP: area.
! Author:	APK
!--

.SEVERITY    INFORMATIONAL

SHEETLIMOBS	<Sheet_limit parameter is obsolete. Page_limit is being used instead>
!++
! GENERATED BY: Symbiont
! DISPLAYED TO: User.
! CAUSE:	Due to layup_definition implemenation, this parameter will
!		be implemented for number of pages not sheets.
! USER ACTION:	none
! AUTHOR:	APK
!--
 
PRNAME		<Message from PrintServer !AS>/FAO_COUNT=1
!++
! GENERATED BY: Symbiont
! DISPLAYED TO: Print OPERATOR along with status messages originate from printer
! CAUSE:	If the status message is originated from printer then they
!		will be preceded by PRNAME message to let the operator
!		know that they originated from particular PrintSever.
! USER ACTION:	Operator should take the stated action follow this message
!		for the printer named in this message.
! AUTHOR:	APK
!--

.SEVERITY    WARNING

LAYUPIGNORED	<Layup_definition parameter ignored; since Number_up=0 was specified>
!++
! GENERATED BY: Symbiont
! DISPLAYED TO: User.
! CAUSE:	If Number_up=0 and Layup_definition both parameters are 
!		specified then layup is ignored.
! USER ACTION:	Specify value for Number_up > 0, or donot specify Number_up
!		at all such that symbiont will default Number_up=1.
! AUTHOR:	APK
!--
NETRECOVER	<Recoverable network error - print job !UL requeued>/FAO_COUNT=1
!++
! GENERATED BY: Symbiont
! DISPLAYED TO: User.
! CAUSE:	A network error has occurred, causing your job to stop printing.
!		The symbiont will re-queue your job, but will be unable
!		guarantee that all files will be printed.
!		
!	
! USER ACTION:	Check output and re-submit.
! AUTHOR:	MJD
!--
.SEVERITY ERROR
RESUBMIT	<Unrecoverable checkpointed job !UL in hold - please resubmit>/FAO_COUNT=1
!++
! GENERATED BY: Symbiont
! DISPLAYED TO: User, Operator
! CAUSE:	The checkpointed job found with /COPIES or /JOB__COUNT.
!		Due to VMS V4.x bug, symbiont can not uncheckpoint this 
!		job and put it in hold.
!	
! USER ACTION:	Re-submit your print job.
! 
! ASIDE (NOT FOR PUBLICATION):  This is message is a warning for a VAX/VMS
!               V4.x bug in SJC$_NOCHECKPOINT.  Apparently, when a job
!		is requeued, the VMS job controller does not clear the
!		job or file counts.  After VMS V5.0, we can print this
!		job normally.
!
! AUTHOR:	MJD
!--
	.SEVERITY FATAL
NETFAIL		<Unrecoverable network error on job !UL - queue restart required>/FAO_COUNT=1
!++
! GENERATED BY: Symbiont
! DISPLAYED TO: User.
! CAUSE:	A network error has occurred, causing your print queue and
!		print job to stop printing.   
!		
!	
! USER ACTION:	Check associated continuation msg and re-submit your print job.
! AUTHOR:	MJD
!--
.SEVERITY WARNING
CANTCHECKPNT	<Checkpointed job !UL is requeued>/FAO_COUNT=1
!++
! GENERATED BY: Symbiont
! DISPLAYED TO: User
! CAUSE:	Multi-file checkpointed  job was requested for printing with
!		first request not being the very first file of the job. This
!		would be result of the STOP/QUEUE on multi file job.
!	
! USER ACTION:	Check output and resubmit
! AUTHOR:	APK
!--

!**********
! System Management Messages -
! messages which are generated or fielded by print server management
!**********

.BASE	LPS$$LPS__MGMT__BASE
             
.SEVERITY    INFORMATION

INTERRLOG       <Event log Has Been Initialized>
!++
! Used by SM  - JZW
! GENERATED BY:	Server Management
! DISPLAYED TO:	Message Handler (Event Log)
! CAUSE:	Server Mgmt Initialize Event Log command
! USER ACTION:	None
! Author:       JZW
!--

UNLMTEXC        <Unprivileged connection limit exceeded - retrying...>
!++
! GENERATED BY: Remote Server Management
! DISPLAYED TO: Remote Server Manager
! CAUSE: 	The limit on the number of connections (4) has been exceeded
! USER ACTION:	Wait for RSM to give up retrying, then try again if necessary.
! Author:	RJD 6-jun-1986
!--

SMRETRYCONN     <Print server not responding to connect request - retrying...>
!++
! GENERATED BY: Remote Server Management
! DISPLAYED TO: Remote Server Manager
! CAUSE:	an attempt to make a connection was made before there was a job
!		on the print server to handle the connect
! USER ACTION:	Wait, the connection is retried automatically
! Author:	RJD 6-jun-1986
!--

.SEVERITY    WARNING

NOCHANGE	<Current and new passwords must be different - password not changed>
!++
! Used by RSM - RD
! GENERATED BY: Server management 
! DISPLAYED TO: Server manager 
! CAUSE:        Server manager attempts to change the system password but
!		inputs the same password for both the old and new passwords
! USER ACTION:  Reenter password correctly
! AUTHOR:       J. Z. Wolf
!--

.SEVERITY    ERROR

UNOPERRLOG      <Unable to open Event Log file !AS>/FAO_COUNT=1
!++
! GENERATED BY: Message handler 
! DISPLAYED TO: operator
! CAUSE:	An attempt to open the event log file failed  
! USER ACTION:  check supporting host in server management
! AUTHOR:      	J. Z. Wolf 
!--

.SEVERITY    INFORMATION

CLSERRLOG	<Event Log has been Disabled and Closed>
! GENERATED BY:	server management 
! DISPLAYED TO: event logger
! CAUSE:        event logging was disabled  
! USER ACTION:  none
! AUTHOR:	J. Z. Wolf 

.SEVERITY    ERROR

MNUNOTFND	<Unable to open Menu Script file !AS>/FAO_COUNT=1
!++
! GENERATED:	LPS_MANAGEMENT (VMS program)
! DISPLAYED:	Remote Server Manager
! CAUSE:	unable to find/access the LPS$MNU.ENG file
! ACTION:	retry - if it reoccurs check for file in LPS40 directory
! Author:	RJD  23-May-1986
!--
!++             
! GENERATED BY: Server management
! DISPLAYED TO: Operator
! CAUSE:        For some reason, the menu file could not be opened
! USER ACTION:  Check existence and protection of file LPSMNU.ENG.
!		Also check for correct account and proxy setup.
!		Correct any problems, then reboot LPS40.
! AUTHOR:       J. Z. Wolf
!--

INVNODEID       <Nodeid has improper form>
!++
! GENERATED: LPS_MANAGEMENT
! DISPLAYED: Remote Server Manager
! CAUSE    : user-supplied nodeid has invalid nodeid syntax
! ACTION   : reenter nodeid correctly
!
! RJD  23-May-1986
!--

NOSUCHNODE      <Node doesn't exist or isn't known to the Supporting Host>
!++
! GENERATED: LPS_MANAGEMENT
! DISPLAYED: Remote Server Manager
! CAUSE    : user-supplied nodeid isn't a known node
! ACTION   : reenter nodeid correctly
!
! RJD  23-May-1986
!--

NOSUCHPS        <Node is not a print server>
!++
! GENERATED: LPS_MANAGEMENT
! DISPLAYED: Remote Server Manager
! CAUSE    : user-supplied nodeid isn't a Print Server node
! ACTION   : reenter the nodeid of a Print Server
!
! Author:	RJD  23-May-1986
!--

.SEVERITY    ERROR

ENGNTRSPD       <Print engine not responding to inquiry>
!++
! Used by RSM - RD
! GENERATED BY: Server Management 
! DISPLAYED TO: Server manager 
! CAUSE:        A wait on a port connect directly or indirectly to postscript
!		timed out, we assume there is something wrong with the engine
! USER ACTION:  May resolve itself, else may have to reboot
! AUTHOR:       J. Z. Wolf
!--

INSYNFAIL	<Initialization/Synchronization failed>
!++
! GENERATED BY: Server Initialization, SMP gateway process
! DISPLAYED TO: Operator, event log
! CAUSE:        An error occurred which prevented the Print Server from
!		starting up successfully.
! USER ACTION:	The Print Server is probably unusable.  A reboot will typically
!		resolve the problem, unless an underlying hardware problem
!		is the cause.
! AUTHOR:       WEM
!--

CANTHAPPEN 	<Action should never happen>
!++
! Used by:	Status collector
! GENERATED BY: Status Collector 
! DISPLAYED TO: Operator
! CAUSE:        Attempt to cleanup internal linked list in Status Collector failed.
! USER ACTION:  Retry failed operation.  Submit SPR.
! AUTHOR:       MJD;   6/9/86
!--

!+++
!obsolete..
!DO		<Execute>
!---

INVCOMMAND	<Invalid command>
!++
! Used by RSM - RD
! GENERATED BY: Server management 
! DISPLAYED TO: server manager 
! CAUSE:        a command that we do not recognize was presented to 
!		server management
! USER ACTION:  Should never happen.  Submit SPR.
! AUTHOR:       J. Z. Wolf
!--

INVCHAR		<Invalid character entered>
!++
! GENERATED BY: Server management 
! DISPLAYED TO: Server manager 
! CAUSE:        A character not of the expected type was submitted
! USER ACTION:  reenter with corrections
! AUTHOR:       J. Z. Wolf
!--

INVCLASS	<Invalid class>
!++
! GENERATED BY: server management 
! DISPLAYED TO: server manager 
! CAUSE:        An invalid command class was entered
! USER ACTION:  Should never happen.  Submit SPR.
! AUTHOR:       J. Z. Wolf
!--

INVPARM		<Invalid parameter>
!++
! Used by RSM - RD
! GENERATED BY:  Server management
! DISPLAYED TO:  Server manager
! CAUSE:         A parameter not consistent with the command was found
! USER ACTION:   Should never happen.  Submit SPR.
! AUTHOR:        J. Z. Wolf
!--

INVPASSWRD	<Invalid password> 
!++ 
! Used by RSM - RD 
! GENERATED BY: Server management  
! DISPLAYED TO: Server manager  
! CAUSE:        At enable privileges or change password prompts, user 
!		did not enter correct password 
! USER ACTION:  Reenter password correctly 
! AUTHOR:       J. Z. Wolf 
!-- 

BADTIMVAL	<Invalid time value>
!++
! GENERATED BY: Server management 
! DISPLAYED TO: Server manager 
! CAUSE:        In a call to ELN/VMS time conversion routine an invalid
!		parameter was included
! USER ACTION:  Re-enter time value correctly
! AUTHOR:       J. Z. Wolf
!--

NOROOM		<No room to add anything more>   
!++
! Used by RSM - Used by other components??? - RD
! GENERATED BY: Server management 
! DISPLAYED TO: internal. 
! CAUSE:        An attempt to place a parameter into a parameter area failed
!		because of lack of room
! USER ACTION:  n/a
! AUTHOR:       J. Z. Wolf
!--

UNIMPL          <Function is not implemented>
!++
! Used by RSM - RD
! GENERATED BY: Server management 
! DISPLAYED TO: Internal.
! CAUSE:        user attempted to execute a command that was not implemented
! USER ACTION:  Don't do it again
! AUTHOR:       J. Z. Wolf
!--

USERABORT	<Action aborted by the user>
!++
! Used by RSM - RD
! GENERATED BY: Server management 
! DISPLAYED TO: Server manager 
! CAUSE:        User was presented with a prompt for data and then had second
!		THOUGHTS. USER RESPONDED with a carriage return
! USER ACTION:  none
! AUTHOR:       J. Z. Wolf
!--

!+++
!obsolete..
! NODISK	<No disk is available>
! NURSERIES  	<Access of requested resource failed>
! INVMSGBLK   	<Creation of invalid message block attempted>
! CANTREAD    	<Read of resource file failed -- for file !AS>/FAO_COUNT=1
! CANTCLOSE   	<Close of resource file failed -- for file !AS>/FAO_COUNT=1
!obsolete...
! ***EDF	<Error deleting file>
! ***EXISTS	<Exists>
! ***FEX	<File already exits>
! ***FNF	<File does not exist>
! ***INVFILSYN	<Invalid file specification>
! ***NONPRIV	<Non privileged state>
! ***PRIV	<Privileged state>
! ***MESS3	< This is message 3>                         
! ***FATERRABT  <Job aborted due to fatal internal server error>
! ***INVFILRES  <Invalid response in file state>

INVDATA    	<Invalid Data>
!++
! GENERATED BY: Server Mgmt 
! DISPLAYED TO: Server Manager
! CAUSE:	
! USER ACTION:  
! AUTHOR:	MJD; 6/9/86
!--

!++
!obsolete...
! NOCDBSEL	<Please SELECT the CDB you wish to operate on>
! NOCDB		<The CDB you have selected does not exist>
! MEMSEL	<You cannot LOAD/SAVE the current CDB>
!---

NOTFOUND	<Entry not found>
!++
! GENERATED BY:  status Collector
! DISPLAYED TO:  event logger
! CAUSE:         While searching for some data in its internal database,
! the status collector failed to find what it was looking for
! USER ACTION:   none
! AUTHOR:        J. Z. Wolf
!--

OBJNOTFND	<Object of the search was not found>
!++
! GENERATED BY: Server management 
! DISPLAYED TO: internal to server management 
! CAUSE:        was search a parameter area for a parameter and failed
! USER ACTION:  none
! AUTHOR:       J. Z. Wolf 
!--

INVOBJTYP	<Invalid Object Type>
!++
! Used by RSM - RD
! GENERATED BY: Server management 
! DISPLAYED TO: Internal to server management 
! CAUSE:        An invalid object type was issued in a command
! USER ACTION:  none
! AUTHOR:       J. Z. Wolf
!--

INVOBJID	<Invalid Object ID>
!++
! GENERATED BY: Server management 
! DISPLAYED TO: internal to server management 
! CAUSE:        an invalid object ID was issued in a command
! USER ACTION:  none
! AUTHOR:       J. Z. Wolf
!--

INVDATTYP	<Invalid Data Type>
!++
! GENERATED BY: Server management 
! DISPLAYED TO: internal to server management 
! CAUSE:        an invalid data type was issued in a command
! USER ACTION:  none
! AUTHOR:       J. Z. Wolf
!--

FAILED		<Failed>
!++
! Used by SM and RSM 
! GENERATED BY: Server management 
! DISPLAYED TO: Server manager 
! CAUSE:        A command issued by a server manager failed
! USER ACTION:  resubmit command if desired
! AUTHOR:       J. Z. Wolf
!--
 
SMNOTRSPD       <Connection failure - print server not responding>
!++
! GENERATED BY: Remote Server Management
! DISPLAYED TO: Remote Server Manager
! CAUSE:	multiple attempts to connect to the print server have failed
! USER ACTION:	try again after a couple of minutes - if the condition persists
!              	submit an SPR
! Author:	RJD 6-jun-1986
!--

.SEVERITY    FATAL

UNRECCDBFLD	<Unrecognized CDB field - should never happen>
!++
! Used by RSM - RD
! GENERATED BY: Remote Server Management
! DISPLAYED TO:	Internal
! CAUSE:	
! USER ACTION:	
! Author:
!--
                     
NOPRIV		<No privilege for attempted operation>
!++
! Used by RSM - RD
! GENERATED BY: Server management 
! DISPLAYED TO: Internal.
! CAUSE:        Server attempted to execute a privileged command while in
!		a non privileged mode
! USER ACTION:  Can't happen in real life
! AUTHOR:       J. Z. Wolf
!--

NORSMJOB        <No active Remote Server Management job on that print server>
!++
! GENERATED: LPS_MANAGEMENT
! DISPLAYED: Remote Server Manager
! CAUSE    : user-supplied nodeid is a Print Server which doesn't have a
!            RSM job listening for a connect -- shouldn't happen
! ACTION   : reboot the Print Server
!
! Author:	RJD  23-May-1986
!--
                         
RSMBUG          <Fatal bug in the Remote Server Management code>
!++
! GENERATED: LPS_MANAGEMENT
! DISPLAYED: Remote Server Manager
! CAUSE    : unexpected error returned on initial uif_bind call
! ACTION   : try again and report condition to the developers (via SPR)
! Author:	RJD  23-May-1986
!--

.SEVERITY    WARNING

RONOCONN        <Operator DISABLED - unable to make connection to Server>
!++
! GENERATED BY: Remote Operator Process
! DISPLAYED TO: Remote Server Manager
! CAUSE: The remote operator process is unable to make a connection to the
!        Print Server so the operator status is returned to DISABLED
! USER ACTION: Try again
! Author: RJD 18-jun-1986                                           
!--

.SEVERITY    ERROR

PRVCONEXC	<Print server privileged connection limit exceeded>
!++
! GENERATED: LPSCDBUTIL
! DISPLAYED: Remote Server Manager
! CAUSE    : attempted to exceed the limit on privileged remote managers
! ACTION   : try later
!
! RJD  16-Jul-1986
!--

PSWDNOTVER	<Password verification error - password not changed>
!++
! Used by Server Management (remote and local)
! GENERATED BY: Server management 
! DISPLAYED TO: Server manager 
! CAUSE:        When changing the password, the user did not enter the
!		same new password twice
! USER ACTION:  Reenter password correctly
! AUTHOR:       Gene Morgan 16-Jul-1986
!--

NOCDBAVL	<No CDB file available - print server configuration not changed>
!++
! Used by RSM - RD
! GENERATED BY: Server Management
! DISPLAYED TO:	server manager
! CAUSE:	No CDB exists in the LPS$SUPPORT area
! USER ACTION:	Use Save CDB to create a CDB file.
! Author:	Jeff, fill this in please
!--
                     
CDBEOFFN	<CDB has invalid format - print server configuration not changed>
!++
! Used by server management
! GENERATED BY: Server Management
! DISPLAYED TO:	server manager
! CAUSE:	Load CDB encountered an illegally formatted CDB file.  This
!		is typically due to a failure during a previous Save CDB.
! USER ACTION:	Delete the invalid CDB file if an older version exists, or
!		re-create the desired CDB file by using server management
!		to set the desired values and doing another Save CDB.
! Author:	Jeff, fill this in please
!--

.SEVERITY    ERROR

UNRCHABL        <Print server node is currently unreachable>
!++
! Used by RSM - RD
! GENERATED BY: Server Management
! DISPLAYED TO:	server manager
! CAUSE:	Node given is not currently reachable
! USER ACTION:	Try again later
! Author:	RJD
!--

NONETMBX        <Remote Server Management requires NETMBX privilege>
!++
! Used by RSM - RD
! GENERATED BY: Remote Server Management
! DISPLAYED TO:	remote server manager
! CAUSE:	RSM was run without NETMBX privilege
! USER ACTION:	set proc/priv=netmbx then try again
! Author:	RJD
!--

UNDERLEN        <The minimum required terminal page length is 24 lines>
!++
! Used by RSM - RD
! GENERATED BY: Remote Server Management
! DISPLAYED TO:	remote server manager
! CAUSE:	Terminal page length is less than 24 lines
! USER ACTION:	Increase the page length then try again
! Author:	RJD
!--

.SEVERITY    ERROR

NOACTJOB	<No job currently active on print server>
!++
! Generated By: Server management ( In Client )
! Displayed To: Server Manager
! Cause:	User issued SHOW ACTIVE JOB CHARACTARISTICS
!		When no job was active
! User Action:	None
! Author:	J.Z.W.
!--

.SEVERITY    ERROR

RSMDISC		<Error occurred - connection to print server lost>
!++
! Generated By: Remote Server management 
! Displayed To: Server Manager
! Cause:	The connection to the print server was broken.  This
!               could be because the print server was turned off or
!               the network link was broken.
! User Action:	Check if the print server is still operating, if so
!               try starting up a remote management process.  If this
!               fails, it is probably a network problem.  If the print
!               server has halted then reboot.
! Author:	RJD
!--

.SEVERITY    ERROR

DECNETOFF	<DECnet is off - Server management requires DECnet be on>
!++
! GENERATED BY:   Network service
! DISPLAYED TO:   Server Manager
! CAUSE:          DECnet is off on supporting host
! USER ACTION:    Startup DECnet then try again
! AUTHOR:         RJD  10/20/86
!--

RSMNETERR	<Unexpected network error>
!++
! GENERATED BY:   Remote Server Management
! DISPLAYED TO:   Server manager
! CAUSE:          Unexpected network error received during initial connect
!                 to print server
! USER ACTION:    try again - if condition persists submit an SPR
! AUTHOR:         RJD  10/20/86
!--

.SEVERITY	WARNING

ATOPMN		<Attempting to open menu file>
!++
! GENERATED BY:	Server Management
! DSIPLAYED TO: Server Manager
! CAUSE:	Current inability to open lpsmnu.eng file
! USER ACTION:	User must identify the underlying problem and then rectify it
!		1) If the server's node number is not in the supporting
!		hosts network database, add it
!		2) If the lpsmnu.eng file is missing, get it
!		3) Use remote server management to point the server to a
!		supporting host whose environment is more favorable 
!		Once the problem is rectified, the menu will come up
! Author:	JZW 02/05/87
!--
.SEVERITY	WARNING

EVTLOGDISAB     <Event logging disabled - event log not initialized>
!++
! GENERATED BY:	Server Management
! DISPLAYED TO: Server Manager
! CAUSE:	Event logging was disabled when user attempted 
!		to initialize event log.
! USER ACTION:	Enable event log and then re-try Initialize Event log command.
! Author:	MJD; 03/13/87
!--

.SEVERITY FATAL

.SEVERITY FATAL
LJCEXPERR	<Laps-Interrupt Process error, reason = !AS>/FAO_COUNT=1
!++
! GENERATED BY: LAPS Job Controller
! DISPLAYED TO: Server Manager
! CAUSE:	LAPS Job Controller's Interrupt component encountered 
!		an error during startup or while processing a client's 
!		DELETE/ENTRY request.
! USER ACTION:	Reboot the PrintServer.  Please submit an SPR.  
! AUTHOR:	MJD, 3/16/86
!--


!**********
! The following message codes are generated by PostScript
!**********

.BASE	LPS$$LPS__POST__BASE
                            
.SEVERITY    INFORMATIONAL

IDLE		<Idle>
!++
! GENERATED BY:   Postscript
! DISPLAYED TO:   Server management
! CAUSE:          Request for status of Print engine
! USER ACTION:    None
! AUTHOR:         J. Z. Wolf
!--

ACTIVE		<Active>
!++
! GENERATED BY:   Postscript
! DISPLAYED TO:   Server management
! CAUSE:          Request for status of print controller
! USER ACTION:    none
! AUTHOR:         J. Z. Wolf
!--

.SEVERITY    WARNING

EXITSRVR	<exitserver has been executed - permanent state may be changed>
!++
! GENERATED BY: PostScript
! DISPLAYED TO: Operator, User
! CAUSE:        exitserver operator has been successfully invoked
! USER ACTION:  Unauthorized users of this operator could change
!		the state of the print server in such a way as to have a
!		negative impact on following jobs.
! AUTHOR:       WEM
!--

.SEVERITY    ERROR

FLUSHING	<Rest of Job (to EOJ) will be ignored>
!++
! GENERATED BY: PostScript
! DISPLAYED TO: User
! CAUSE:        Some error or other event has occurred which is causing
!		PostScript to ignore the rest of the job.
! USER ACTION:  None.  The message(s) which precede this message should
!		provide clues as to why the job is being flushed.
! AUTHOR:       WEM
!--

.SEVERITY    WARNING

!++
!	POSTSCRIPT MESSAGES:  THIS APPLIES TO THE FOLLOWING MESSAGES
!
! GENERATED BY: PostScript
! DISPLAYED TO: User
! CAUSE:        An error has occurred within the PostScript job.
! USER ACTION:  If this error occurs while printing a file of a data-type
!		other than PostScript, It indicates an error in the translation
!		process;  in this case, an SPR should be submitted.  If
!		the error occurs while printing a PostScript file, the
!		PostScript program (or the program which generated the
!		PostScript program) is in error;  the problem should be
!		fixed and the job resubmitted.
! AUTHOR:       WEM
!--

DICTFULL	<dictfull: No more room in dictionary - offending command is !AS>/FAO_COUNT=1
DICTSTKOV	<dictstackoverflow: Too many begins>
DICTSTKUF	<dictstackunderflow: Too many ends>
EXECSTKOV	<execstackoverflow: exec nesting is too deep - offending command is !AS>/FAO_COUNT=1
INTERUPT	<interrupt: The job has been interrupted>
INVACC		<invalidaccess: Attempt to access restricted object or capability - offending command is !AS>/FAO_COUNT=1
INVEXIT		<invalidexit: exit not in loop>
INVFILACC	<invalidfileaccess: Bad file access string - offending command is !AS>/FAO_COUNT=1
INVFONT		<invalidfont: Bad font name or dictionary - offending command is !AS>/FAO_COUNT=1
INVREST		<invalidrestore: Improper restore - offending command is !AS>/FAO_COUNT=1
IOERROR		<ioerror: System I/O error occurred - offending command is !AS>/FAO_COUNT=1
LIMCHK		<limitcheck: Implementation limit exceeded - offending command is !AS>/FAO_COUNT=1
NOCURPT		<nocurrentpoint: Path is empty - offending command is !AS>/FAO_COUNT=1
RANGECHK	<rangecheck: Argument out of bounds - offending command is !AS>/FAO_COUNT=1
STKOFLO		<stackoverflow: Operand stack overflow - offending command is !AS>/FAO_COUNT=1
STKUFLO		<stackunderflow: Operand stack underflow - offending command is !AS>/FAO_COUNT=1
SYNERR		<syntaxerror: Input ended in string or procedure body - offending command is !AS>/FAO_COUNT=1
TIMOUT		<timeout: Time limit exceeded>
TYPCHK		<typecheck: Argument of wrong type - offending command is !AS>/FAO_COUNT=1
UNDEF		<undefined: Name not known - offending command is !AS>/FAO_COUNT=1
UNDFILNAM	<undefinedfilename: File not found - offending command is !AS>/FAO_COUNT=1
UNDEFRES	<undefinedresult: Number overflow or underflow - offending command is !AS>/FAO_COUNT=1
UNMATCH		<unmatchedmark: Expected mark not on stack - offending command is !AS>/FAO_COUNT=1
UNREGIST	<unregistered: PostScript has encountered a system error - offending command is !AS>/FAO_COUNT=1
VMERROR		<VMerror: PostScript virtual memory exhausted - offending command is !AS>/FAO_COUNT=1
CFGERROR	<configurationerror: !AS - offending command is !AS>/FAO_COUNT=2
PSINCONSIST	<PostScript controller state is inconsistent>
PSQUITOPER	<PostScript quit operator has been executed>
PSFATALSYS	<PostScript fatal error at PC !UL>/FAO_COUNT=1

!++
!	END OF POSTSCRIPT MESSAGES
!--

!**********
! The following codes are all the printer (print engine) conditions
!**********

.BASE	LPS$$LPS__ENG__BASE

!++
!	ENGINE MESSAGES:  THIS APPLIES TO THE FOLLOWING MESSAGES
!
! GENERATED BY: PostScript
! DISPLAYED TO: Operator
! CAUSE:        An engine-related event has been detected.
! USER ACTION:  Correct the problem in one of the following ways:
!		(1) follow the instructions on the print engine.
!		(2) call the key operator (especially if key light is on).
!		(3) call field service (especially if wrench light is on).
!		(4) submit an SPR (if error is LPS$_PRTHDWERR).
! NOTE:		In General:
!		Informational (-I-) messages indicate the print server will
!				need normal servicing soon.  Call the key
!				operator or field service.
!		Warning (-W-) messages can be handled simply by correcting
!				the problem.  Excessive occurrence of one
!				of these conditions should be reported to
!				field service.
!		Error (-E-) messages may require the print server to be
!				powered off and back on - field service
!				should be notified.  These errors may be
!				hardware or software.
!		Fatal (-F-) messages indicate the print server has had some
!				serious failure.  Typically the print server
!				will have to be powered off.  Sometimes
!				powering the server off then on will cause
!				the problem to go away.  Field service
!				should be always be called when one of these
!				errors are seen.
! AUTHOR:       WEM
!--             


.SEVERITY    SUCCESS			! codes which reflect normal operation

PRNTREDY	<Print Engine ready>
!++
! Used by RSM - Used by other components??? - RD
! GENERATED BY:
! DISPLAYED TO:
! CAUSE:	
! USER ACTION:	
! Author:
!--

PRNTERRCORR	<Print Engine error has been corrected>
!++
! GENERATED BY: PostScript
! DISPLAYED TO: Operator
! CAUSE:	Previous printer condition has been cleared
! USER ACTION:	None
! Author:	WEM
!--
.SEVERITY    INFORMATIONAL		! codes which reflect the printer may
					! need attention soon.

MAINT50K	<Print Engine 50,000 page maintenance required>
MAINT100K	<Print Engine 100,000 page maintenance required>
MAINT200K	<Print Engine 200,000 page maintenance required>
OILLOW		<Print Engine oil supply low>
TNRLOW		<Print Engine toner supply low>
WARMUP		<Print Engine is in warmup state>

!+++
! Following codes are NOT used by the LPS-40 software
!
!future...
!TRAYSWI	<Switching to alternate paper source>
!
! Following conditions are NOT in the LN02-V print engine specification and are
! therefore NOT reported by the LPS-40 software
!
!DEVREPL	<Print Engine developer needs replacement>
!DRMREPL	<Print Engine drum needs replacement>
!STPLSLOW	<Print Engine staples supply low>
!---

.SEVERITY    WARNING			! codes which reflect the printer needs
					! normal servicing

EJECTJAM	<Print Engine paper eject section is jammed>
TRANSJAM	<Print Engine transport section is jammed>
UPTRAYJAM	<Print Engine upper paper tray is jammed>
			! Text modified 29-Jun-1988
			! EDIT:
			! PCO:		335
			! AUTHOR:	EL
LOTRAYJAM	<Print Engine lower paper tray is jammed>
			! Text modified 29-Jun-1988
			! EDIT:
			! PCO:		335
			! AUTHOR:	EL
LCITJAM		<Print Engine large capacity paper tray is jammed>
UPTRAYEMP	<Print Engine upper paper tray is empty>
			! Text modified 29-Jun-1988
			! EDIT:
			! PCO:		335
			! AUTHOR:	EL
LOTRAYEMP	<Print Engine lower paper tray is empty>
			! Text modified 29-Jun-1988
			! EDIT:
			! PCO:		335
			! AUTHOR:	EL
LCITEMP		<Print Engine large capacity paper tray is empty>
COVOPN		<Print Engine cover is open>
TNRMALADJ	<Print Engine toner cartridge setting is incorrect>
TRNMALADJ	<Print Engine transport path setting is incorrect>
FUSMALADJ	<Print Engine fusing unit setting is incorrect>
FDSTAKJAM	<Face down stacker is jammed>
FDSTAKFULL	<Face down stacker is full>
FDSTAKOPEN	<Face down stacker cover is open>
LCSTAKJAM	<Large capacity stacker is jammed>
LCSTAKFULL	<Large capacity stacker is full>
LCSTAKOPEN	<Large capacity stacker cover is open>
!+
! Do not use the following six messages [EL 29-Jun-1988]
!-
MBSTAKJAM	<Mailbox stacker is jammed>
MBSTAKFULL	<Mailbox stacker is full>
MBSTAKOPEN	<Mailbox stacker cover is open>
DUPSTAKJAM	<Duplex stacker is jammed>
DUPSTAKFULL	<Duplex stacker is full>
DUPSTAKOPEN	<Duplex stacker cover is open>
!+
!-
OFFLINE		<Print Engine has gone off line>
PRNOTRDY	<Print Engine is not ready>
TNREND		<Print Engine toner supply is exhausted>
LCITCOVOPEN	<Large capacity tray cover is open>

!+++
! Following conditions are NOT in the LN02-V print engine specification and are
! therefore NOT reported by the LPS-40 software
!
!STPLSEMP	<Printer Out of Staples>
!---

.SEVERITY    ERROR			! codes which reflect the printer needs
					! professional help.

PRHDWERR	<Print Engine hardware error: !AS>/FAO_COUNT=1
PRUNKERR	<Print Engine unknown error status: !UL, !UL>/FAO_COUNT=2

.SEVERITY    FATAL			! codes which reflect the printer is
					! in immed. need of professional help

POWERERR	<Print Engine power supply unit error - FATAL ERROR>
DRIVEERR	<Print Engine driving unit error - FATAL ERROR>
FUSINGERR	<Print Engine fusing error - FATAL ERROR>
FEEDERR		<Print Engine paper feed unit error - FATAL ERROR>
OPTICERR	<Print Engine optical unit error - FATAL ERROR>
FDSTAKERR	<Face down stacker unit error - FATAL ERROR>
LCSTAKERR	<Large capacity stacker unit error - FATAL ERROR>
MBSTAKERR	<Mailbox stacker unit error - FATAL ERROR>
DUPSTAKERR	<Duplexer unit error - FATAL ERROR>
			! Text modified 29-Jun-1988
			! EDIT:
			! PCO:		335
			! AUTHOR:	EL

.SEVERITY    WARNING			! Stopgap LPS20 Service request

PRSERVICE	<Printer requires service: code = !UL !UL>/FAO_COUNT=2

!+
! Following messages (through FSCALL inclusive) added in edit ________
! for PCO 335.
! DATE: 29-Jun-1988
! AUTHOR: EL
!-

.SEVERITY    INFORMATIONAL		! codes which reflect the printer may
					! need attention soon.
MAINT20K	<Print Engine 20,000 page maintenance required> ![LPS20]
MAINT40K	<Print Engine 40,000 page maintenance required> ![LPS20]
MAINT300K	<Print Engine 300,000 page maintenance required> ![LPS20]
ONLINE		<Print Engine has gone online>

.SEVERITY    WARNING			! codes which reflect the printer needs
					! normal servicing
DUPJAM		<Duplexer unit is jammed>			![LPS40E]
DUPOPEN		<Duplexer cover is open>		![LPS40E]
DUPUNSEAT	<Duplexer paper path is not properly seated> ![LPS20]
FUSERUNSEAT	<Fuser unit absent or improperly seated>  ![LPS20]
CLEANUNSEAT	<Cleaning unit absent or improperly seated> ![LPS20]
OPCUNSEAT	<OPC drum absent or improperly seated> ![LPS20]
DEVUNSEAT	<Developer unit absent or improperly seated> ![LPS20]
OUTCOVOPEN	<Paper output cover is open>		![LPS20]
SIDECOVOPEN	<Print engine side cover is open>	![LPS20]
UPTRAYOUT	<Upper paper tray is missing or misaligned> ![LPS20]
LOTRAYOUT	<Lower paper tray is missing or misaligned> ![LPS20]
LCSSHELFOUT	<LCOS paper shelf is missing or misaligned> ![LPS40E]
SIDEFULL	<Side output tray full>			![LPS20]
UPFULL		<Upper LCOT full>			![LPS20]
LOFULL		<Lower LCOT full>			![LPS20]
ADJTRAY		<Side tray not correctly positioned> ![LPS20]
NOTRAY		<Unavailable paper destination selected> ![LPS40E]
CLEANFULL	<Cleaning unit full - needs replacement> ![LPS20]
REPLDRUM	<OPC drum needs replacement> ![LPS20]
DENSADJUST	<Image density adjustment required> ![LPS20]
UPWRONGMEDIA	<Incorrect media in upper paper tray>	![LPS20]
LOWRONGMEDIA	<Incorrect media in lower paper tray>	![LPS20]
LCWRONGMEDIA	<Incorrect media in LCIT>		![LPS20]
WRONGMEDIA	<Media incompatible with duplexer or output device> ![LPS40E,20]
LCOTSMALL	<Media too small for LCOS - going to top output tray> ![LPS40E]
LCOTDIFF	<Media differs from LCOS setting - going to top tray> ![LPS40E]

! LPS20 paper jam reports

DPXXJAM		<Please clear paper jam caused by DPX transport station>
DPXFJAM		<Please clear paper jam caused by DPX feed path>
FUSJAM		<Please clear paper jam caused by fusing unit>
LOXJAM		<Please clear paper jam caused by lower transport station>
UPXJAM		<Please clear paper jam caused by upper transport station>
MAINXJAM	<Please clear paper jam caused by main transport station>

! Use the following LPS40 messages when reporting jams in these locations:

!EJECTJAM	Eject station
!UPTRAYJAM	Upper paper tray feed path
!LOTRAYJAM	Lower paper tray feed path
!LCITJAM	LCIT feed path

.SEVERITY FATAL
FSCALL		<Call Field Service to correct error> ![LPS40E]

!++
!	END OF PRINT ENGINE MESSAGES
!--

! START OF TEST STUFF ADDED BY CG - FROM GABBE
.BASE	LPS$$LPS__DEVCTL__BASE

.BASE	LPS$$LPS__DEVCTL__BASE

!++
! These messages are for layup
!--

.SEVERITY WARNING

LOSTPATH	<The current path may have been lost.>
NOCOPYPG	<"copypage" is not supported by multi-page layup.>
PPSGTNUP	<Pages per sheet are greater than number up. Pages per sheet set to number up.>
FPGGTPPS	<First page is greater than pages per sheet.  First page set to pages per sheet.>
NOSETTRAY	<Tray selection is not supported by multi-page layup.>

.SEVERITY ERROR

ZEROAREA	<Layup definition margins result in no usable sheet area.>
NEGAREA		<Layup definition margins overlap one another.>

!++
! End of layup messages
!--

!++
! For LPS$$SetContext
!--

.SEVERITY ERROR

SIZNOTAVL	<No !AS size medium is loaded in !AS.>/FAO_COUNT=2
SIZNOTSUP	<!AS size medium is not supported by !AS.>/FAO_COUNT=2
SIZNOTRAY	<!AS size medium in not in the !AS tray in !AS.>/FAO_COUNT=3

.SEVERITY FATAL

FTLDEVCTL	<Fatal device control library problem.  Config error:  !AS on !AS>/FAO_COUNT=2

.SEVERITY INFORMATIONAL

UPPRPGLIM	<The upper page limit has been reached.  Remaining pages will be flushed.>

!++
! LPS$$SetOutputTray
!--

.SEVERITY INFORMATIONAL

TRAYSUBST	<Output will be delivered to the !AS tray on !AS.>/FAO_COUNT=2

.SEVERITY ERROR

TRYNOTRDY	<!AS tray on !AS is !AS.>/FAO_COUNT=3

!++
! LPS$$SetSides
!--

.SEVERITY ERROR

DPLXNOSUP	<!AS does not support duplex printing.>/FAO_COUNT=1
TUMBNOSUP	<!AS does not support tumble printing.>/FAO_COUNT=1
NOINPTRAY	<Input tray selection not supported for !AS.>/FAO_COUNT=1
NOOUTTRAY	<Output tray, "!AS", not supported on !AS.>/FAO_COUNT=2

.SEVERITY WARNING

BADLAYDEF	<!AS on line !AS in layup definition.>/FAO_COUNT=2

! These messages are used by the persistent prologue loader to find out what 
! the hell is going on in the printer.

.SEVERITY INFORMATIONAL

PROLOGIS	<Prologue !AS, version !AS>/FAO_COUNT=2

BADPASSWD	<Bad password>

! END OF LPSMSG.MSG

.END
