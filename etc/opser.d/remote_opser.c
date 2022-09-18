
# ifndef lint
static char *sccsid = "@(#)remote_opser.c	4.1	(ULTRIX)	7/2/90";
# endif not lint

/************************************************************************
 *									*
 *			Copyright (c) 1984 by				*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any	other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or	reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/*
 * remote_opser.c
 *
 *	static char Sccsid[] = "@(#)remote_opser.c     1.0      8/08/86";
 *
 *	Modification History:
 *	
 *	 1/28/87 - fries
 *		Removed code to perform timeout when socket
 *		connection is lost. Instead added code to perform
 *		setsockopt SO_KEEPALIVE.
 *
 *	 1/13/87 - fries
 *		Added code to handle no entry for "root" password.
 *
 *	12/11/86 - fries
 *		Corrected bug - set up remote console device.
 *
 *	12/3/86 - fries
 *		Corrected bug - ignore SIGCHLD upon restart request.
 *
 *	9/12/86 - fries
 *		Corrected bug - added code to support ^C handling.
 *
 *	9/02/86 - fries
 *		Corrected bug - now remote opser properly handles delete
 *	        translation, line kill(Ctrl U), and echoing.
 *
 *	8/28/86 - fries
 *		Corrected bug - did not translate rubout into backspace
 *	        space, basckspace sequence.
 *
 *	8/22/86 - fries
 *		Added socket timeout.
 *
 *	8/13/86 - fries
 *		Corrected password(NO PTY reported) bug.
 *
 *	8/03/86 - fries
 *		Created remote opser file.
 *
 */

#include "opser.h"

#define GOOD 1
#define FATAL -1

struct timeval to, *timeout;
extern char *network_cmds[];
extern char *args[];
extern FILE *tfd, *flagf;
int   console;
extern int debug;
extern int remote_flag;
extern int sock_fd, pty_fd, tty_fd;
extern errno;
extern char remote[];
char   pty_buf[1024],*line;
char   buf[BUFSIZ];
int status, child_complete = 1, connect_lost = 0, xpid, pid, pid_saved;

char	*crypt();
int	child_done();
int	kill_it();
void	lost_it();

/* Slave Opser Code Follows... */
remote_opser()
{
	struct sgttyb tty;

	/* Open Remote console device */
	console = open("/dev/console",2);

    	ioctl(console, TIOCGETP, &tty);	/*get parameters*/
    	tty.sg_erase = '\177';		/*reset erase,kill,&interrupt*/
    	tty.sg_kill = '\025';
    	tty.sg_flags &= ~CBREAK;	/* turn off CBREAK and RAW */
    	tty.sg_flags &= ~RAW;
    	ioctl(console, TIOCSETP, &tty);	/* set console parameters */

	/* Set flag to indicate Remote Opser */
	remote_flag = 1;

	/* Set up timer to allow select to poll */
	to.tv_sec = (long)0;
	to.tv_usec= (long)0;
	timeout = &to;

	/* Set up network socket */
	if(setupsock() < 0)
	  return;

	/* Perform remote opser handling */
	doit();
}

/* Set up pty, tty and process commands input */
/* across network...                          */
doit()
{
	char 	c;
	char	cmd_buf[1024];
	int 	cmd_flag = 0;
	int	i, response, sock_cc, pty_cc;
	int	cmd_indx = 0;

		/**************************/
		/* Set up signal handlers */
		/**************************/
		signal(SIGTSTP, SIG_IGN);

		/* Child process completed */
		signal(SIGCHLD, child_done);

		/* ^C received from MASTER opser */
		signal(SIGURG, kill_it);

	        pid =  -getpid();
	        ioctl(sock_fd,SIOCSPGRP,&pid);

		/* TOP of Socket & Child(pty) Input/Output loop */
		for (;;) {
			int ibits = 0, obits = 0, nfds, nfnd;

			ibits |= (1<<pty_fd);
			ibits |= (1<<sock_fd);

			if( pty_fd > sock_fd) nfds = pty_fd + 1;
			else nfds = sock_fd+1;

			/* perform synchronous i/o multiplexing */
			nfnd = select(nfds, &ibits, &obits, 0, timeout);

			if(connect_lost)
				return;

			if (ibits & (1 << pty_fd)){
			   pty_cc = read(pty_fd, pty_buf, sizeof (pty_buf));

			        /* If data from child... */
			        if ( pty_cc > 0){
			           sock_cc = write(sock_fd, pty_buf, pty_cc);
			           write(console, pty_buf,pty_cc);
				
				   /* if Error... */
			           if (sock_cc < 0){
			 	      fatal_report(LOST_CONNECT);
				      return;
				   }
				}
			}
			
			/* If sock_fd has data to input... */
			if (ibits & (1 << sock_fd)) {
			   sock_cc = read(sock_fd, &c, 1);

			   /* If Error... */
			   if (sock_cc <= 0){
			      fatal_report(LOST_CONNECT);
			      return;
			   }
			   
			   /* If data input from socket... */
			   if (sock_cc){
			      if (c == '\033')
			         cmd_flag++;
			      if( cmd_flag){
			        if( c != '\033')
			          cmd_buf[cmd_indx++] = c;
			      }
			      else{
			        pty_cc = write(pty_fd, &c, 1);
				if(child_complete){
				   if(c == '\177')
				     sock_cc = write(sock_fd,"\b \b",3);
				   else
                                     sock_cc = write(sock_fd, &c, 1);
				}

			        /* If Error... */
			        if (sock_cc < 0){
			           fatal_report(LOST_CONNECT);
			           return;
			        }
			      }
			   }
			}
			
			/* If command has been assembled, then execute it */
			if(cmd_flag == 4){
			  cmd_flag = 0;
			  cmd_buf[cmd_indx] = '\0';
			  cmd_indx = 0;
			  child_complete = 0;
			  pid = 0;
			  i = parse(cmd_buf,network_cmds,cmd_indx);
	                  switch(i){
				case N_HALT:
		    		   haltsys();
				   break;
				case N_RESTART:
				   signal(SIGCHLD, SIG_IGN);
				   restart();
				   break;
				case N_BACKUP:
				   /* Inform users at client */
				   write(console,"\n",1);
				   write(console,
"***********************************************\n",49);
				   write(console,
"***********************************************\n",49);
				   write(console,
"****** INITIATING DUMP FROM MASTER OPSER ******\n",49);
				   write(console,
"****** PLEASE DO NOT DISTURB THE SYSTEM  ******\n",49);
				   write(console,
"***********************************************\n",49);
				   write(console,
"***********************************************\n",49);
				   command_fork("/etc/rdump",args);
				   break;
		                case N_DISMOUNT:
				   command_fork("/etc/umount",args);
				   break;
		                case N_FSCK:
				   command_fork("/etc/fsck",args);
				   break;
		                case N_PASSWORD:
				   child_complete = 1;
				   response = net_passwd();
				   if (response  == FATAL)return;
				   if (response == GOOD)
					send_status(0);
				   else
					send_status(1);
				   break;
		                case N_SHELL:
				   command_fork("/bin/sh",args);
				   break;
		                case N_SIZE:
				   command_fork("/etc/dump",args);
				   break;
		                default:
				   fatal_report(INV_CMD);
	                  }
			}/* end of command process */
		}/* end of forever loop */
}/* end of doit() function */

/* Fork off child and execute commands */
command_fork(command,args)
	char *args[];
	char *command;
	{
	char	c;
	int	i, pgrp, on = 1;

	/* look for a /dev/pty_ file */
	for (c = 'p'; c <= 'z'; c++) {
		struct stat stb;
		line = "/dev/ptyXX";
		line[strlen("/dev/pty")] = c;
		line[strlen("/dev/ptyp")] = '0';
		if (stat(line, &stb) < 0)
			break;

		/* try to open file */
		for (i = 0; i < 16; i++) {
			line[strlen("/dev/ptyp")] = "0123456789abcdef"[i];
			pty_fd = open(line, 2);/* open /dev/ptyp_ */
			if (pty_fd > 0)
				goto gotpty;
		}
	}
	fatal_report(NO_PTY);
	return;
	/*NOTREACHED*/
gotpty:

	ioctl(pty_fd, FIONBIO, &on);/* Set to raw i/o */

	/* Fork off child */
        pid = fork();

	pid_saved = pid;

	/* If fork failed... */
	if (pid < 0){
           fatal_report(FORK_FAIL);
	   return;
	}

	/* If child... */
	if (!pid) {
	   setpgrp(pid,0);
	   line[strlen("/dev/")] = 't'; /* change /dev/pty to /dev/tty */
	   tty_fd= open(line, 2);/* open /dev/ttyp_ */

	   /* If open failed... */
	   if (tty_fd < 0){
		fatal_report(NO_TTY);
		return;
	   }

/* This code sets up the "/dev/tty" driver to handle */
/* character echo, line kill and crt erase...        */
{
struct sgttyb tty_stuff;
int erase = LCRTBS|LCRTERA|LCRTKIL;

gtty(tty_fd,&tty_stuff);
tty_stuff.sg_flags |= ECHO;
stty(tty_fd,&tty_stuff);
ioctl(tty_fd,TIOCLBIS,&erase);
}
	   (void) close(pty_fd);  /* close /dev/pty */
           (void) dup2(tty_fd, 0);/* open stdin  - /dev/tty */
	   (void) dup2(tty_fd, 1);/* open stdout - /dev/tty */
           (void) dup2(tty_fd, 2);/* open stderr - /dev/tty */

	   /* child executes command requested */
           /* from MASTER opser                */
           execv(command,args);

	   /* if execl did not occur... */
	   send_status(1);

           /*NOTREACHED*/
        }
	sleep(1);
}

/* Set up CLIENT opser socket */
setupsock()
{
	int	on = 1, nameln,temp;
	FILE	*flagf;
	char	buf[80];
	struct 	hostent *hostent;
	struct 	servent *servent;
	struct 	sockaddr_in name;

	pid = -getpid();

	if((flagf = fopen(remote,"r")) == 0 ){
	  perror("Can't open /etc/remoteopser");
	  return(-1);
	}

	/* Get socket data from file */
	fscanf(flagf,"%d %d %d",&name.sin_family,&temp,
				&name.sin_port);
	name.sin_addr.s_addr = temp;
	fclose(flagf);

	/* Remove "/etc/remoteopser" file */
	system("rm -f /etc/remoteopser");

	/* Get a socket to use */
	if((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		fatal_report(NO_OPEN);
		return(-1);
	}
	nameln = sizeof(name);

	/* Connect socket to name */
	if(connect(sock_fd, &name, nameln) == -1){
	    fatal_report(NO_TTY);
	    return(-1);
	}

	/* Set Socket up to perform keep alive messages */
	if(setsockopt(sock_fd, SOL_SOCKET, SO_KEEPALIVE, &on, sizeof(on)) < 0){
	    fatal_report(NO_TTY);
	    return(-1);
	}

	/* SIGPIPE - socket connection lost */
	signal(SIGPIPE,lost_it);


	ioctl(sock_fd,SIOCSPGRP,&pid);

	return;
}

/* Report Fatal Error either on socket to Master Opser */
/* Or to user on console device                        */
fatal_report(code)
	int code;
	{
	char buf[10];

	switch(code){
	case NO_PTY:
		(void) write(sock_fd,"Lost pty connection to child\n",29);
		sprintf(buf,"\033\033%02d\033\033",NO_PTY);
		(void) write(sock_fd, buf, strlen(buf));
		break;
	case NO_TTY:
		write(console,"Can not open Socket to Master Opser\n",36);
		write(console,"Aborting Slave Opser mode\n",26);
		write(console,"Type \"r\" to restart system\n",27);
		break;
	case LOST_CONNECT:
	case NO_OPEN:

	close(0);
	close(1);
	close(2);
	(void) dup2(console, 0);/* open stdin at /dev/tty */
	(void) dup2(console, 1);/* open stdout at /dev/tty */
	(void) dup2(console, 2);/* open stderr at /dev/tty */
	close(console);

	fprintf(stderr,"\7\7\7\7\n");
	fprintf(stderr,"***********************************************\n");
	fprintf(stderr,"***********************************************\n");

	if(code == LOST_CONNECT)
	   fprintf(stderr,"Lost Connection to Master Opser\n");
	else
	   fprintf(stderr,"Can not open Connection to Master Opser\n");

	fprintf(stderr,"Aborting Slave Opser mode\n");
	fprintf(stderr,"Type `r' to restart this system\n");
	fprintf(stderr,"***********************************************\n");
	fprintf(stderr,"***********************************************\n");
	fflush(stderr);

	break;
	case FORK_FAIL:
		sprintf(buf,"\033\033%02d\033\033",FORK_FAIL);
		(void) write(sock_fd, buf, strlen(buf));
		break;
	case INV_CMD:
		sprintf(buf,"\033\033%02d\033\033",INV_CMD);
		(void) write(sock_fd, buf, strlen(buf));
		break;
	}
}

/* If <CTRL/C> received from the MASTER opser... */
kill_it()
	{
	 int my_pid;

	 my_pid = getpid();
	 if(debug){
	    printf("Got a SIGURG!\n");
	    fflush(stdout);
	 }
	 signal(SIGURG, kill_it);
	 if(pid){
	     killpg(pid_saved,SIGINT);
	     fflush(stdout);
	 }
	 else {
	     fflush(stdout);
	     kill(my_pid,SIGINT);
	 }
}

/* Send Status Returned from child process */
send_status(code)
	int code;
	{
	char status_code[6];

	/* package return code */
	sprintf(status_code,"\033\033%02d\033\033",code);

	/* write it out to the socket */
	if(write(sock_fd,status_code,strlen(status_code)) < 0)
	  fatal_report(LOST_CONNECT);
}

/*    SIGCHLD signal received     */
/* status of child process changed */
child_done()
	{
	int	sock_cc, pty_cc, nfnd, nfds, code;
	int	ibits = 0, obits = 0;
	union 	wait wt, *status;

	child_complete = 1;

	/* Set pointer */
	status = &wt;

	/* Re-enable signal handler */
	signal(SIGCHLD, child_done);

     /* Get input until there is no more */
     do{
	ibits = 1 << pty_fd;

	nfds  = pty_fd + 1;

	/* perform synchronous i/o multiplexing */
	nfnd = select(nfds, &ibits, &obits, 0, timeout);

	if (ibits & (1 << pty_fd))
             pty_cc = read(pty_fd, pty_buf, sizeof (pty_buf));
	else
	     break;

        if ( pty_cc > 0){
           sock_cc = write(sock_fd, pty_buf, pty_cc);
           write(console, pty_buf,pty_cc);
	   write(console,"****** ENTERED CHILD DONE CODE *****\n");

           if (sock_cc < 0){
	       fatal_report(LOST_CONNECT);
	       return;
	   }
	}
	else break;

	}while(1);

	ibits = 0;

	/* get child's status */
	wait(status);

	/* get status returned from child's exit */
        code = status->w_retcode;

	send_status(code);	

	/* Close pseudo terminal */
	close(pty_fd);
}		

/* Verifies user has password for client root */
/* Returns a 1 if the passwords compare       */
int net_passwd()
{
	register char  *pp, *namep;
	int	sock_cc, nfnd, nfds, ibits = 0, obits = 0;
	int	code;
	char	c, pbuf[9], *p;
	struct	passwd *pwd;

	/* Ask user to enter root password */
	write(sock_fd,"Password:",10);

	/* Set up pointer */
	p = pbuf;

     /* Get input until newline, EOD or buffer full */
     do{
	nfds  = sock_fd + 1;
	ibits = 1 << sock_fd;

	/* perform synchronous i/o multiplexing */
	nfnd = select(nfds, &ibits, &obits, 0, 0);

	if (ibits & (1 << sock_fd)){
           	sock_cc = read(sock_fd, &c, 1);

        	if (sock_cc <= 0){
	    		fatal_report(LOST_CONNECT);
	    		return(FATAL);
		}
	}
	if (!sock_cc)continue;

	if ((c != '\n')&&(c != EOF))
            *p++ = c;

	}while( c != '\n' && c != EOF && p < &pbuf[8]);

	*p = '\0';

	/* Clear input from socket(case of pbuf full) */
	while((c != '\n')&&(c != EOF)){
	   nfds  = sock_fd + 1;
	   ibits = 1 << sock_fd;

	   /* perform synchronous i/o multiplexing */
	   nfnd = select(nfds, &ibits, &obits, 0, 0);

	   if (ibits & (1 << sock_fd)){
           	sock_cc = read(sock_fd, &c, 1);

           	if (sock_cc <= 0){
	      		fatal_report(LOST_CONNECT);
	      		return(FATAL);
	   	}
	   }
	}/* End of do while */

	/* get password for root */
	pwd = getpwuid(0);

	/* Check to insure that there is an entry in */
	/* the /etc/passwd file for "root"           */
	if(pwd == NULL)
		return(0);

	/* Encrpt it */
	namep = crypt(pbuf, pwd->pw_passwd);

	/* Compare password entered           */
	/* to password from "etc/passwd" file */
	if (strcmp(namep, pwd->pw_passwd) == 0)
           return(1);
	else
           return(0);
}

/* SIGPIPE forces to this function */
/* if socket connection is lost    */
void lost_it()
	{
	 fatal_report(LOST_CONNECT);
	 connect_lost++;
	}
