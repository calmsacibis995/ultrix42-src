#ifndef lint
static char *sccsid = "@(#)eli.c	4.1	ULTRIX	7/2/90";
#endif lint
/************************************************************************
 *									*
 *			Copyright (c) 1986 by				*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any  other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/
/*
*
*
* File Name: eli.c
*
* Source file description: Control program which provides user interface
*	to elcsd process for modification of error logging system parameters
*
* Functions: 
*	main		process user switches
*	helprtn		print help
*	singl		start elcsd in single user mode		
*	sndmsg		send mesg to elcsd via UDP
*	elierror	report eli error
*	elexec		execute elcsd in specified mode
*	eliexit		exit from eli program
*	elcsd_lock	lock elcsd in memory (for testing only)
*	warnon		turn on kernel missed error warning msgs
*	warnoff		turn off kernel missed error warning msgs
*	sig_catch	catch user signals
*
* Usage: eli <-f> <switch> where <switch> is:
*	-f	force action of following switch
*	-h	help
*	-d	disable error logging
*	-e	(re)enable error logging
*	-s	enable error logging in single user mode
*	-n	enable error logging in windowing mode only
*	-i	initialize kernel error log buffer
*	-r	reconfigure error logging, per config file
*	-l	log a status message into the error log
*	-m	lock elcsd in memory (for testing only, UNDOCUMENTED)
*	-q	disable missed error messages
*	-w 	re-enable missed error messages
*
* Compile: see makefile
*
* Modification history:
*
* 22 Nov 88	Randall Brown
*	Fixed a segmentation fault on PMAX for 'eli -h'
*
* 09 Jun 88	map
*	Changed signal handlers to void.
*
* 03 Jun 86	bjg
*	Add -w, -q; signal cleanup; rerun lint;
*
* 28 Apr 86	bjg
*	Add -m option (undocumented)
*
* 24 Apr 86	bjg
*	Change Error Message for Unsuccessful Disable Attempt
*
* 01 Apr 86	bjg
*	Add -l switch to log a status message into the error log
*
* 20 Jan 86	bjg
*	Initial Creation
*/

#include <sys/types.h>
#include <sys/time.h>
#include <sys/errlog.h> 
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <elcsd.h>

#define SUCCESS 0
#define ERROR -1
#define MAX_SZ 256 - (sizeof(short))

char *help_str[] = { "eli [-f] -[dehilnqrsw]\neli options:\n", 
		"-d disables error logging\n", 
		"-e enables error logging in multi-user mode\n",
		"-f forces the subsequent option;no prompts\n",
		"-h prints information about the eli command\n", 
		"-i initializes the kernel errorlog buffer\n",
		"-l logs a one-line status message to the kernel errorlog buffer\n",
		"-n disables error logging to disk;use -e to re-enable\n",
		"-q suppresses the missed error message from appearing on the console\n",
		"-r reconfigures error logging;use after changing /etc/elcsd.conf\n",
		"-s enables error logging in single-user mode\n",
		"-w enables the missed error message to appear on the console\n",
		"\0" };
int alrmflg = 0;
int forceflg = 0;

/*
*
*
* Function: main()
*
* Function description: Process switches
*
* Arguments:
*	argc 	number of args
*	argv	switches
*
* Return value: None
*
* Side effects: None
*
*/

main(argc,argv)
	int argc;
	char **argv;
{

	short type;
	int rval = SUCCESS;
	char **pargv = argv;	/* ptr to argv */
	void sigcatch();
	
	(void) signal(SIGINT, sigcatch);
	(void) signal(SIGQUIT, sigcatch);
	(void) signal(SIGTERM, sigcatch);
	(void) signal(SIGHUP, sigcatch); 

	if (argc == 1) {
		/* exec helprtn */
		helprtn();
		exit();
	}

	if (geteuid()) {
		printf("eli: Must be super-user\n");
	}
	else {
	    pargv = argv;
	    if (**++pargv == '-' && *(*pargv + 1) == 'f') {
		forceflg = 1;
		argv++;	/* skip over -f */
	    }

	    if (**++argv == '-') {
		switch (*++*argv) {
		case 'h':
			/* exec help rtn */
			helprtn();
			break;
		case 'd':
			/* send elcsd mesg to die */
			if (pidcheck() == 0) {
				printf("Error Logging Already Disabled\n");
			}
			else {
				type = ELDISABLE;
				rval = sndmsg(&type);
				if (rval == SUCCESS) {
					printf("Error Logging Disabled\n");
				}
			}
			break;
		case 'e':
			/* invoke /etc/elcsd */
			rval = enable();
			break;
		case 's':
			/* invoke /etc/elcsd -s */
			/* Reprompt: */
			rval = singl();
			break;

		case 'n':
			/* invoke /etc/elcsd -n */
			rval = wonly();
			break;
		case 'i':
			rval = initit();
			if (rval == SUCCESS) {
				printf("Error Logging Buffer Initialized\n");
			}
			break;
		case 'r':
			/* send elcsd mesg to reconfigure */
			type = ELRECONFIG;
			rval = sndmsg(&type);
			if (rval == SUCCESS) {
				printf("Error Logging Reconfigured\n");
			}
			break;
		case 'l':
			/* log a mesg into the kernel error log buffer */
			user_log(argv);
			break;
		case 'm':	/* UNDOCUMENTED */
			if (elcsd_lock() == SUCCESS) {
			    type = ELLOCK;
			    rval = sndmsg(&type);
			    if (rval == SUCCESS) {
				printf("elcsd locked (use only for testing)\n");
			    }
			}
			break;
		case 'q':
			if (warnoff() == SUCCESS) {
				printf("Warnings Disabled until Reboot or Re-enabled\n");
			}
			break;			
		case 'w':
			if (warnon() == SUCCESS) {
				printf("Warnings Re-Enabled\n");
			}
			break;	
		default:
			fprintf(stderr,"eli: Invalid arg %s, type \"eli -h\" for help\n",*argv);
			eliexit(0);
		}
	    }
	}
	eliexit(rval);
}
/* Help Routine merely prints out help string */
helprtn()
{
	char **ptr = help_str;
	while (**ptr != '\0') {
		printf(*ptr++);
	}

			
}
/*
*
*
* Function: enable()
*
* Function description: Enable error logging.
*
* Arguments: None
*
* Return value: SUCCESS or ERROR
*
* Side effects: None
*
*/
enable()
{
	char reply[80];
	int rval = SUCCESS;
	if (pidcheck() > 0) {
		if (forceflg) {
			reply[0] = 'y';
		}
		else {
			printf("Error Logging is Already Enabled\n");
			printf("Do you want to Reenable Error Logging? (y) ");
			(void)gets(reply);
		}
		if (reply[0] == 'y' || reply[0] == 'Y' || reply[0] == '\0') {
			/* execute shell procedure here */

			if (elexec((char *)0) == SUCCESS) {	
				rval = SUCCESS;
			}
			else {
				rval = ERROR;
			}
		}
		else {
			printf("eli: Request Aborted\n");
			rval = ERROR;
		}
	}
	else { /* pid not set */
		if (elexec((char *)0) == SUCCESS) {	
			rval = SUCCESS;
		}
		else {
			rval = ERROR;
		}
	}
	return(rval);
}
/*
*
*
* Function: pidcheck()
*
* Function description: Get the pid of the current elcsd from kernel
*
* Arguments: None
*
* Return value: pid or ERROR
*
* Side effects: None
*
*/
pidcheck()
{
	int fd;
	long pid = 0;

	fd = open("/dev/errlog", O_RDONLY);	
	if (fd < 0) {
		printf("eli: Cannot open /dev/errlog\n");
		printf("eli: Request aborted\n");
		eliexit(ERROR);
	}
	if ((ioctl(fd, ELGETPID, &pid)) < 0) {
		printf("eli: Cannot retrieve pid\n");
		printf("eli: Request aborted\n");
		return(ERROR);
	}
	else {
		return(pid);
	}

}
/*
*
*
* Function: initit()
*
* Function description: Reinitialize the error log buffer and its ptrs
*
* Arguments: None
*
* Return value: SUCCESS or ERROR
*
* Side effects: None
*
*/
initit()
{
	int fd;
	char reply[80];
	int rval = SUCCESS;

	if (forceflg) {
		reply[0] = 'y';
	}
	else {
		printf("Caution: All Error Log data (not yet to disk) will be LOST\n");
		printf("Do you still want to reinitialize? (y) ");
		(void)gets(reply);
	}
	if (reply[0] == 'y' || reply[0] == 'Y' || reply[0] == '\0') {
		fd = open("/dev/errlog", O_RDONLY);	
		if (fd < 0) {
			printf("eli: Cannot open /dev/errlog\n");
			printf("eli: Request aborted\n");
			rval = ERROR;
		}
		else {
			if (ioctl(fd, (int)ELREINIT, (char *)0) < 0) 
				rval = ERROR;
		}
	}
	else {
		printf("eli: Request aborted\n");
		rval = ERROR;
	}

	return(rval);

}
/*
*
*
* Function: singl()
*
* Function description: Execute elcsd in single user mode if user sure
*		he's in single user mode
*
* Arguments: None
*
* Return value: SUCCESS or ERROR
*
* Side effects: None
*
*/

singl()
{
	char reply[80];
	int rval = SUCCESS;
	/* Prompt Again */
	if (pidcheck() > 0) {
		if (forceflg) {
			reply[0] = 'y';
		}
		else {
			printf("Error Logging is Already Enabled\n");
			printf("Do you want to Reenable Error Logging? (y) ");
		}
		(void)gets(reply);
		if (reply[0] == 'y' || reply[0] == 'Y' || reply[0] == '\0') {
			if (forceflg) {
				reply[0] = 'y';
			}
			else {
				printf("Caution: Are you in Single User Mode? (y) ");
			(void)gets(reply);
			}
			if (reply[0] == 'y' || reply[0] == 'Y' || reply[0] == '\0') {
			/* check that hostname set up */
		    	    if (rcloc() == SUCCESS) {
			
				if (elexec("-s") == SUCCESS) {
					rval = SUCCESS;
				}
				else {
					rval = ERROR;
				}
			    }
			    else {	/* rcloc failed */
				printf("eli: Request Aborted\n");
				rval = ERROR;
			    }
			}
			else {		/* answered no */
				printf("eli: Request Aborted\n");
				rval = ERROR;
			}
		
		}
		else {		/* answered no */
			printf("eli: Request Aborted\n");
			rval = ERROR;
		}
	  }
	  else { /* pid not set */
		if (forceflg) {
			reply[0] = 'y';
		}
		else {
			printf("Caution: Are you in Single User Mode? (y) ");
			(void)gets(reply);
		}
		if (reply[0] == 'y' || reply[0] == 'Y' || reply[0] == '\0') {

    		    /* check that hostname set up */
	    	    if (rcloc() == SUCCESS) {
			if (elexec("-s") == SUCCESS) {	
				rval = SUCCESS;
			}
			else {
				rval = ERROR;
			}
		    }
		    else {	/* rcloc() failed */
			printf("eli: Request Aborted\n");
			rval = ERROR;
		    }
		}
		else {	/* answered no */
			printf("eli: Request Aborted\n");
			rval = ERROR;
		}
	  }


	return(rval);

}
/*
*
*
* Function: rcloc()
*
* Function description: Check /etc/rc.local that /bin/hostname entry
* is there and that it has been executed.
*
*
* Arguments: None
*
* Return value: SUCCESS or ERROR
*
* Side effects: None
*
*/
rcloc()
{
	int fd;
	char buf[512];
	char *cp;
	char hostname[20];
	char *hp = hostname;
	int flg = 0;

	fd = open("/etc/rc.local", O_RDONLY, 0);
	if (fd < 0) {
		printf("Can't open /etc/rc.local\n");
		return(ERROR);
	}
	if (read(fd, buf, sizeof(buf)) <= 0) {
		printf("Can't read /etc/rc.local\n");
		return(ERROR);
	}
	cp = buf;
	while (flg == 0 && cp < &buf[511]) {
		if (strncmp(cp, "/bin/hostname", 13) != 0) {
			/* skip to next line */
			while (*cp++ != '\n' && cp < &buf[511]) ;
		}
		else {
			flg = 1;
		}
	}
	if (flg == 0) {
		printf("\n You must execute /bin/hostname <hostname>");
		return(ERROR);
	}
	/* skip over /bin/hostname */
	cp+=13;
	while (*cp == ' ' && *cp != '\n') cp++ ;

	while (*cp != '\n'){
		*hp++ = *cp++;
	}
	*hp = '\0';
	if (strlen(hostname) == 0) {
		printf("No hostname specified in /bin/hostname in /etc/rc.local\n");
		return(ERROR);
	}
	else {
		return(hnexec(hostname));
	}
}
/*
*
*
* Function: wonly()
*
* Function description: Execute elcsd in window only mode if user sure
*		he wants to do this.
*
* Arguments: None
*
* Return value: SUCCESS or ERROR
*
* Side effects: None
*
*/
wonly()
{
	char reply[80];
	int rval = SUCCESS;
	/* Prompt Again */
	if (pidcheck() > 0) {
		if (forceflg) {
			reply[0] = 'y';
		}
		else {
			printf("Error Logging is Already Enabled\n");
			printf("Do you want to Reenable Error Logging? (y) ");
			(void)gets(reply);		 
		}
		if (reply[0] == 'y' || reply[0] == 'Y' || reply[0] == '\0') {
			if (forceflg) {
				reply[0] = 'y';
			}
			else {
				printf("Caution: Error Log Contents Will NOT be Written To disk\n ");
				printf("Do you still want windowing only mode? (y) ");
				(void)gets(reply);
			}
			if (reply[0] == 'y' || reply[0] == 'Y' 
				|| reply[0] == '\0') {
			/* execute shell procedure here */

				if (elexec("-n") == SUCCESS) {	
					rval = SUCCESS;
				}
				else {
					rval = ERROR;
				}
			}
			else {
				printf("eli: Request Aborted\n");
				rval = ERROR;
			}
		}
		else {	/* answered no */
			printf("eli: Request Aborted\n");
			rval = ERROR;
		}
	  }
	  else { /* pid not set */
		if (forceflg) {
			reply[0] = 'y';
		}
		else {
			printf("Caution: Are you in Single User Mode? (y) ");
			(void)gets(reply);
		}
		if (reply[0] == 'y' || reply[0] == 'Y' || reply[0] == '\0') {
			if (elexec("-n") == SUCCESS) {	
				rval = SUCCESS;
			}
			else {
				rval = ERROR;
			}
		}
		else {	/* answered no */
			printf("eli: Request Aborted\n");
			rval = ERROR;
		}
	  }

	return(rval);
}
/*
*
*
* Function: sndmsg(type)
*
* Function description: Set up UDP socket; send message to elcsd.
*
* Arguments: 
*	cp	ptr to type  - specifies action requested by 
*			user, which will be transmitted to elcsd
*
* Return value: SUCCESS or ERROR
*
* Side effects: None
* 
*/

sndmsg(type)
short *type;
{
	int sc;
	int x;
	int sctolen;
	struct sockaddr_un scun, scto;
	short ack;
	void alrmrtn();

	sc = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (sc < 0) {
	    printf("eli: Cannot Create Socket\n");
	    return(ERROR);
	}
	scun.sun_family = AF_UNIX;
	(void)unlink(elisckt);
	bcopy(elisckt,(char *)scun.sun_path,sizeof(elisckt));

	if (bind(sc, (char *)&scun, sizeof(scun)) < 0) {
	    printf("eli: Cannot Perform Bind\n");
	    return(ERROR);
	}

	sctolen = sizeof(scto);
	scto.sun_family = AF_UNIX;
 	bcopy(elcsckt,(char *)scto.sun_path,sizeof(elcsckt));
	x = sendto(sc, type, sizeof(*type), 0, (char *)&scto, sctolen);
	if (x < 0) {
	    printf("eli: Cannot communicate with elcsd\n");
	    elierror(*type);
	    return(ERROR);
	}
	/* wait for reply */
	(void)signal(SIGALRM, alrmrtn);
	(void)alarm(30);

	x = recvfrom(sc, &ack, sizeof(ack), 0, 0, 0);
	if (alrmflg == 1) {
		if (x < 0) {
			printf("eli: No response from elcsd\n");
			elierror(*type);
			return(ERROR);
		}
	}
	if (x > 0 && ack == *type) {
		return(SUCCESS);		
	}
	else {
		printf("eli: improper ack from elcsd\n");
		printf("eli: Request Aborted\n");
		return(ERROR);
	}

}
/*
*
*
* Function: alrmrtn
*
* Function description: Set flag indicating that timeout has occurred,
*		when awaiting for reply from elcsd
*
* Arguments: None
*
* Return value: None
*
* Side effects: None
*
*/

void
alrmrtn() 
{
	alrmflg = 1;
}

/*
*
*
* Function: elierror(type)
*
* Function description: Print appropriate user error message based on 
*		type passed
*
* Arguments:	
*	type	type of error
*
* Return value: None
*
* Side effects: None
*
*/

elierror(type) 
short type;
{

	if (type == ELDISABLE) {
		printf("Couldn't Disable Error Logging\n");
	}
	else if (type == ELINITIALIZE) {
		printf("Can't Initialize;Error Logging Disabled\n");
	}
	else if (type == ELRECONFIG) {
		printf("Can't Reconfigure;Error Logging Disabled\n");
	}

}
/*
*
*
* Function: hnexec(arg)
*
* Function description: Fork and execute /bin/hostname
*
* Arguments: 
*	arg	pointer to  hostname to be set
*
* Return value:  SUCCESS or ERROR
*
* Side effects: None
*
*/

hnexec(arg)
char *arg;
{
	int x;
	int cpid;
	cpid = fork();
	if (cpid == 0) {	/* child */
		x = execl("/bin/hostname", "/bin/hostname", arg, 0);

		/* doc says execl returns value; lint complains though */
		if (x < 0) {
		    printf("eli: Cannot Exec /bin/hostname\n");
		    return(ERROR);
                }		
	}
	else if (cpid < 0) {
		printf("eli: Cannot Fork Child\n");
		return(ERROR);
	}
	return(SUCCESS);
}
/*
*
*
* Function: elexec(arg)
*
* Function description: Fork and execute elcsd process with 
*	appropriate argument based upon user requested switch
*
* Arguments: 
*	arg	pointer to user switch requested
*
* Return value:  SUCCESS or ERROR
*
* Side effects: None
*
*/

elexec(arg)
char *arg;
{
	int x;
	int cpid;
	cpid = fork();
	if (cpid == 0) {	/* child */
		if (arg) {
			x = execl("/etc/elcsd", "/etc/elcsd", arg, 0);
		}
		else {
			x = execl("/etc/elcsd", "/etc/elcsd",  0);	
		}
		/* doc says execl returns value; lint complains though */
		if (x < 0) {
		    printf("eli: Cannot Exec /etc/elcsd\n");
		    eliexit(ERROR);
                }		
	}
	else if (cpid < 0) {
		printf("eli: Cannot Fork Child\n");
		return(ERROR);
	}
	printf("Error Logging Enabled\n");
	return(SUCCESS);
}
/*
*
*
* Function: eliexit()
*
* Function description: Remove Socket file, created in Unix Domain; Exit
*
* Arguments: 
*	rval	exit value
*
* Return value: rval
*
* Side effects: None
*
*/

eliexit(rval)
int rval;
{
	(void)unlink(elisckt);
	exit(rval);
}
user_log(argv)
char **argv;
{

	char buffer[MAX_SZ];
	char **pargv;
	
	pargv = argv;
	pargv++;
	if (forceflg) {
		strcpy(buffer, *pargv);
	}
	else {
		printf("Enter string: ");
		(void)fgets(buffer, MAX_SZ, stdin);
	}
	(void)logerr(ELMSGT_INFO, buffer);
}
/*
*
*
* Function: elcsd_lock()
*
* Function description: Lock elcsd into memory (no swap) (undocumented)
*
* Arguments: None
*
* Return value: SUCCESS or ERROR
*
* Side effects: None
*
*/
elcsd_lock()
{
	int fd;
	char reply[80];
	int rval = SUCCESS;

	if (forceflg) {
		reply[0] = 'y';
	}
	else {
		printf("Warning: use only for testing\n");
		printf("Do you want to lock elcsd in memory? (y) ");
		(void)gets(reply);
	}
	if (reply[0] == 'y' || reply[0] == 'Y' || reply[0] == '\0') {
		fd = open("/dev/errlog", O_RDONLY);	
		if (fd < 0) {
			printf("eli: Cannot open /dev/errlog\n");
			printf("eli: Request aborted\n");
			rval = ERROR;
		}
		else {
			if (ioctl(fd, (int)ELREINIT, (char *)0) < 0) 
				rval = ERROR;
		}
	}
	else {
		printf("eli: Request aborted\n");
		rval = ERROR;
	}

	return(rval);

}
void sigcatch()
{
	printf("\neli: Terminated by signal\n");
	exit(ERROR);
}
/*
*
*
* Function: warnoff()
*
* Function description: Disable kernel "Missed Error" console messages
*
* Arguments: 
*
* Return value: SUCCESS or ERROR
*
* Side effects: None
*
*/
warnoff()
{
	int fd;
	int rval = SUCCESS;
	char reply[80];

	if (forceflg) {
	    reply[0] = 'y';
	}
	else {
	        printf("Caution: Missed Error warnings will be disabled until next reboot or re-enabled.\n");
	        printf("Do you still want to disable warnings? (y) ");
	        (void)gets(reply);
	}

        if (reply[0] == 'y' || reply[0] == 'Y' || reply[0] == '\0') {
		fd = open("/dev/errlog", O_RDONLY);	
		if (fd < 0) {
			printf("eli: Cannot open /dev/errlog\n");
			printf("eli: Request aborted\n");
			rval = ERROR;
		}
		else {
			if (ioctl(fd, (int)ELWARNOFF, (char *)0) < 0) {
				printf("eli: Could not Disable Warnings\n");
				rval = ERROR;
			}
		}
	}
	else {
		printf("eli: Request aborted\n");
		rval = ERROR;
	}
	return(rval);
}
/*
*
*
* Function: warnon()
*
* Function description: Enable kernel "Missed Error" console messages
*
* Arguments: 
*
* Return value: SUCCESS or ERROR
*
* Side effects: None
*
*/
warnon()
{
	int fd;
	int rval = SUCCESS;

	fd = open("/dev/errlog", O_RDONLY);	
	if (fd < 0) {
		printf("eli: Cannot open /dev/errlog\n");
		printf("eli: Request aborted\n");
		rval = ERROR;
	}
	else {
		if (ioctl(fd, (int)ELWARNON, (char *)0) < 0) {
			printf("eli: Could not Re-Enable Warnings\n");
			rval = ERROR;
		}
	}
	return(rval);
}
