#ifndef lint
static	char	*sccsid = "@(#)miscd.c	4.1	(ULTRIX)	7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1985 by				*
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
 *   This software is  derived  from  software  received  from  the	*
 *   University    of   California,   Berkeley,   and   from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is  subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/*-----------------------------------------------------------------------
 *	Modification History
 *
 *	6/8/88 -- map
 *		Changed signal handlers to void.
 *
 *	4/5/85 -- jrs
 *		Created to serve under inetd to implement some of
 *		the cheap internet services.  Based on a concept by
 *		Marshall Rose of UC Irvine and Chris Kent of Purdue.
 *
 *-----------------------------------------------------------------------
 */

#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>

#include <netinet/in.h>

#include <netdb.h>
#include <syslog.h>

#define	TIMEOUT	120
#define	BUFFERCNT 4096
#define	LINECNT 80

char	*daemoname;

void	timeout();
FILE	*popen();

/*
 *	This program performs some of the small internet utility functions.
 *	It runs subservient to inetd.
 *
 *	The main routine determines the port and protocol.  We look it
 *	up in the /etc/services table and then call the action switcher
 */

main(argc, argv)
	int argc;
	char **argv;
{
	int on = 1, socklen;
	int tcpsock;
	struct sockaddr_in sock;
	struct servent *serv;
	char servnam[16];

	/* This is a real hack to try and find if we are tcp or udp.
	   If a better way can be found this should be replaced */

	socklen = sizeof (sock);
	if (getpeername(0, &sock, &socklen) < 0) {
		tcpsock = 0;
	} else {
		tcpsock = 1;
	}
	socklen = sizeof (sock);
	if (getsockname(0, &sock, &socklen) < 0) {
		openlog(argv[0], LOG_PID);
		syslog(LOG_ERR, "getsockname: %m");
		closelog();
		exit(1);
	}
	serv = getservbyport(sock.sin_port, tcpsock? "tcp": "udp");
	if (serv == NULL) {
		openlog(argv[0], LOG_PID);
		syslog(LOG_ERR, "getservbyport failed");
		closelog();
		exit(1);
	}
	(void) signal(SIGALRM, timeout);
	if (tcpsock == 0) {
		(void) alarm(TIMEOUT);
	} else {
		if (setsockopt(0, SOL_SOCKET, SO_KEEPALIVE, &on, sizeof (on)) < 0) {
			openlog(argv[0], LOG_PID);
			syslog(LOG_WARNING, "setsockopt (SO_KEEPALIVE): %m");
			closelog();
		}
	}
	(void) strcpy(servnam, serv->s_name);
	daemoname = argv[0];
	doit(0, servnam, tcpsock);
}

/*
 *	This routine acts as the main switcher for all functionality
 *	Note that the code is designed to handle both tcp and udp
 *	sockets.  When invoked on tcp sockets, the server will terminate
 *	immediately when the connection is closed.  For the udp case,
 *	we will stay alive until two minutes go by without incoming
 *	packets.
 */

doit(f, servnam, tcpsock)
	int f;
	char *servnam;
	int tcpsock;
{
	int count;
	char buffer[BUFFERCNT];
	char line[LINECNT];
	struct sockaddr from;
	int fromlen;
	int rotate;
	struct timeval nowtime;
	struct timezone nowzone;
	unsigned long nowbias;

	/* simple echo of recieved packet */

	if (strcmp(servnam, "echo") == 0) {
		if (tcpsock == 0) {
			while (1) {
				fromlen = sizeof(from);
				count = recvfrom(f, buffer, sizeof(buffer),
					0, &from, &fromlen);
				(void) alarm(TIMEOUT);
				if (count < 0) {
					openlog(daemoname, LOG_PID);
					syslog(LOG_ERR, "recvfrom: %m");
					closelog();
					exit(1);
				}
				if (sendto(f, buffer, count, 0, &from, fromlen)
							< 0) {
					openlog(daemoname, LOG_PID);
					syslog(LOG_ERR, "sendto: %m");
					closelog();
					exit(1);
				}
			}
		} else {
			while ((count = read(f, buffer, sizeof(buffer))) > 0) {
				(void) alarm(TIMEOUT);
				if (write(f, buffer, count) != count) {
					break;
				}
				(void) alarm(0);
			}
		}
	
	/* throw away imcoming data (without error) */

	} else if (strcmp(servnam, "discard") == 0) {
		if (tcpsock == 0) {
			while (1) {
				fromlen = sizeof(from);
				count = recvfrom(f, buffer, sizeof(buffer),
					0, &from, &fromlen);
				(void) alarm(TIMEOUT);
				if (count < 0) {
					openlog(daemoname, LOG_PID);
					syslog(LOG_ERR, "recvfrom: %m");
					closelog();
					exit(1);
				}
			}
		} else {
			while ((count = read(f, buffer, sizeof(buffer))) > 0) {
				;
			}
		}

	/* return ascii text giving our list of users */

	} else if (strcmp(servnam, "systat") == 0) {
		if (tcpsock == 0) {
			while (1) {
				fromlen = sizeof(from);
				count = recvfrom(f, buffer, sizeof(buffer),
					0, &from, &fromlen);
				(void) alarm(TIMEOUT);
				if (count < 0) {
					openlog(daemoname, LOG_PID);
					syslog(LOG_ERR, "recvfrom: %m");
					closelog();
					exit(1);
				}
				count = invoke("who", buffer, BUFFERCNT);
				if (sendto(f, buffer, count, 0, &from, fromlen)
							< 0) {
					openlog(daemoname, LOG_PID);
					syslog(LOG_ERR, "sendto: %m");
					closelog();
					exit(1);
				}
			}
		} else {
			count = invoke("who", buffer, BUFFERCNT);
			(void) alarm(TIMEOUT);
			(void) write(f, buffer, count);
			(void) alarm(0);
		}

	/* return ascii string giving our idea of time */

	} else if (strcmp(servnam, "daytime") == 0) {
		if (tcpsock == 0) {
			while (1) {
				fromlen = sizeof(from);
				count = recvfrom(f, buffer, sizeof(buffer),
					0, &from, &fromlen);
				(void) alarm(TIMEOUT);
				if (count < 0) {
					openlog(daemoname, LOG_PID);
					syslog(LOG_ERR, "recvfrom: %m");
					closelog();
					exit(1);
				}
				count = invoke("date", buffer, BUFFERCNT);
				if (sendto(f, buffer, count, 0, &from, fromlen)
							< 0) {
					openlog(daemoname, LOG_PID);
					syslog(LOG_ERR, "sendto: %m");
					closelog();
					exit(1);
				}
			}
		} else {
			count = invoke("date", buffer, BUFFERCNT);
			(void) alarm(TIMEOUT);
			(void) write(f, buffer, count);
			(void) alarm(0);
		}

	/* return ascii text buffer of short length - we use fortune to gen */

	} else if (strcmp(servnam, "quote") == 0) {
		if (tcpsock == 0) {
			while (1) {
				fromlen = sizeof(from);
				count = recvfrom(f, buffer, sizeof(buffer),
					0, &from, &fromlen);
				(void) alarm(TIMEOUT);
				if (count < 0) {
					openlog(daemoname, LOG_PID);
					syslog(LOG_ERR, "recvfrom: %m");
					closelog();
					exit(1);
				}
				count = invoke("/usr/games/fortune",
						buffer, BUFFERCNT);
				if (sendto(f, buffer, count, 0, &from, fromlen)
							< 0) {
					openlog(daemoname, LOG_PID);
					syslog(LOG_ERR, "sendto: %m");
					closelog();
					exit(1);
				}
			}
		} else {
			count = invoke("/usr/games/fortune", buffer, BUFFERCNT);
			(void) alarm(TIMEOUT);
			(void) write(f, buffer, count);
			(void) alarm(0);
		}

	/* generate character pattern (silly for udp, but..) */

	} else if (strcmp(servnam, "chargen") == 0) {
		(void) strcpy(line, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
		(void) strcat(line, "abcdefghijklmnopqrstuvwxyz");
		(void) strcat(line, "0123456789");
		(void) strcat(line, "!@#$%^&*()-=+[]{}");
		rotate = 0;
		if (tcpsock == 0) {
			while (1) {
				fromlen = sizeof(from);
				count = recvfrom(f, buffer, sizeof(buffer),
					0, &from, &fromlen);
				(void) alarm(TIMEOUT);
				if (count < 0) {
					openlog(daemoname, LOG_PID);
					syslog(LOG_ERR, "recvfrom: %m");
					closelog();
					exit(1);
				}
				*buffer = '\0';
				for (count = 0; count < 8; count++) {
					(void) strcat(buffer, &line[rotate]);
					(void) strncat(buffer, line, rotate);
					(void) strcat(buffer, "\r\n");
					rotate = (rotate + 1) % strlen(line);
				}
				count = strlen(buffer);
				if (sendto(f, buffer, count, 0, &from, fromlen)
							< 0) {
					openlog(daemoname, LOG_PID);
					syslog(LOG_ERR, "sendto: %m");
					closelog();
					exit(1);
				}
			}
		} else {
			while (1) {
				(void) strcpy(buffer, &line[rotate]);
				(void) strncat(buffer, line, rotate);
				(void) strcat(buffer, "\r\n");
				rotate = (rotate + 1) % strlen(line);
				count = strlen(buffer);
				(void) alarm(TIMEOUT);
				if (write(f, buffer, count) != count) {
					break;
				}
				(void) alarm(0);
			}
		}

	/* return longword giving biased time in seconds since 1900 */

	} else if (strcmp(servnam, "time") == 0) {
		if (tcpsock == 0) {
			while (1) {
				fromlen = sizeof(from);
				count = recvfrom(f, buffer, sizeof(buffer),
					0, &from, &fromlen);
				(void) alarm(TIMEOUT);
				if (count < 0) {
					openlog(daemoname, LOG_PID);
					syslog(LOG_ERR, "recvfrom: %m");
					closelog();
					exit(1);
				}
				(void) gettimeofday(&nowtime, &nowzone);
				nowbias = htonl(nowtime.tv_sec + 2208988800l);
				if (sendto(f, &nowbias, sizeof(nowbias), 0,
						&from, fromlen) < 0) {
					openlog(daemoname, LOG_PID);
					syslog(LOG_ERR, "sendto: %m");
					closelog();
					exit(1);
				}
			}
		} else {
			(void) gettimeofday(&nowtime, &nowzone);
			nowbias = htonl(nowtime.tv_sec + 2208988800l);
			(void) alarm(TIMEOUT);
			(void) write(f, &nowbias, sizeof(nowbias));
			(void) alarm(0);
		}

	/* we have been called to do something we don't know how */

	} else {
		openlog(daemoname, LOG_PID);
		syslog(LOG_WARNING, "unimplemented service: %s/%s", servnam,
					tcpsock? "tcp": "udp");
		closelog();
	}
}

/*
 *	Gracefully handle final alarm for udp shutdown
 */

void
timeout()
{
	exit(0);
}

/*
 *	Invoke given command and return buffer full of output.
 *	Add carriage returns to each line as that's how internet
 *	likes to see things.
 */

invoke(command, buffer, size)
char *command;
char *buffer;
int size;
{
	int	count;
	register char	*bp;
	FILE	*cmd;

	if ((cmd = popen(command, "r")) == NULL) {
		openlog(daemoname, LOG_PID);
		syslog(LOG_ERR, "command failure: %m");
		closelog();
		exit(1);
	}
	count = 0;
	bp = buffer;
	while (fgets(bp, size - count - 1, cmd) != NULL) {
		bp = buffer + strlen(buffer) - 1;
		if (*bp != '\n') {
			break;
		}
		*bp++ = '\r';
		*bp++ = '\n';
		*bp = '\0';
		count = bp - buffer;
	}
	(void) pclose(cmd);
	return(count);
}
