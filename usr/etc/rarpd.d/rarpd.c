#ifndef lint
static  char    *sccsid = "@(#)rarpd.c	4.2  (ULTRIX)        10/16/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1989 by				*
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
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/


/* rarpd.c
 *
 *	This is the top level program for the RARP daemon.  
 *	NOTE:
 *		The rarpd doesn't recognize ARP packets sent
 *	to it. Such packets are ignored. The reason for such
 *	behaviour is: there should be no reason to send a ARP
 *	packet withing RARP as ARP is always available.
 *	
 *	HISTORY:
 *
 *	26-OCT-89 jsd
 *	Allow device to be optional (use pf0, ie. first network interface)
 *
 *	Created by : Uttam Shikarpur Nadig
 *	Date: 20th February 1989	
 *
 *	Usage: rarpd devicename	[-v] [-f filename]
 *		The device is specified as qe0, de0 etc.
 */


/* The standard stuff */

#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <sys/file.h>
#include <limits.h>
#include <strings.h>
#include <netdb.h>
#include <sys/socket.h>
#ifndef CTRACE	/* signal.h gives ctrace problems */
#include <signal.h>
#endif CTRACE
#include <sys/ioctl.h>
#include <syslog.h>
#include <sys/stat.h>
#include <nlist.h>

/* packet filter specific includes */

#include <net/pfilt.h>

/* Some definitions */

/* #define DEBUG 1		/* The debug option - prevents forking */ 
#define PACK_SIZE 1000 	 	/*buffer size to receive a rarp packet */
#define SEND_SIZE 100		/* The RARP packet being sent back */
#define MAX_LINE 256    	/* The length of the longest line allowable
			 	* in the file containing the mappings.
		  	 	*/
#define MAXHOSTLEN 256		/* The max. length of a host name */
#define READ_INTERVAL 6000	/* Time interval for reading "ether_file"
				 * -- in seconds
				 */
#define MSG_SIZE 1024		/* The maximum length of a syslog 
				 * message.
				 */
#define NONRARP	0		/* A NON RARP PACKET */

/* The packet type, for the filter. Everything at that
 * level is byte swapped */

#define F_RARP	  0x3580 	/* 0x8035 - RARP packet */
#define F_REQREVR 0x0300	/* 0x0003 - RARP request reverse type */

/* For user level interactions */
#define RARP   0x8035 


int verbose = 0;		/* The verbose flag; turned on
			  	 * by the -v flag
			  	 */
int debug = 0;			/* for debugging purpose ONLY */

typedef struct _mapper {
	u_char *ether;			/* The hardware address */
	u_char *ip;	 		/* The logical address */
	char   hostname[MAXHOSTLEN];	/* name of the host */
	struct _mapper *next;		/* Ptr to next struct. */
} mapper;

extern mapper *enet_list;
extern char *ether_file;	/* The mapping file */
extern char *progname;		/* The name of the program */
int valid_count;		/* # of correct entries in the /etc/ethers
				 * or the equivalent file
				 */

char device[64];		   /* The device on which the filter is
			 	    * operating. - qe0, ln0 etc. */
char *ether_file = "/etc/ethers";  /* The file for mapping ethernet
			 	    *  addr => internet
				    */
char *progname;			   /*  program name */
int disable = 0;		   /* disable re-reading the ethers file */
long time_mod;			   /* time when the ethers file was last
				    * modified
				    */
char *system = "/vmunix";	   /* The kernel image being used */

struct nlist nl[] = {
	{ "_Pfilt_read" },
	{ ""},
};
main (argc,argv)
int argc;
char **argv;

{
	char pack_buf[PACK_SIZE];	/* Buffer for the RARP packet */
	char send_buf[SEND_SIZE];	/* Buffer to send a resp. out */
	int pack_len;			/* Length of the pack. received */
	int fid;			/* The file descriptor */
	int res;			/* result of a call */
	int i;
	struct itimerval *value;	/* for timing puposes */
	struct itimerval *ovalue;	/* for timing puposes */
	void time_out(), read_mapping();
	char log_msg[MSG_SIZE];		/* message for syslog */
	int msg_len;			/* the message length */
	struct stat stat_val;		/* for stat'n "ether_file" */

	progname = argv[0];

	if (getuid()) {
		 fprintf (stderr, "%s: not super user\n", argv[0]);
		 exit(1);
	}

	openlog(progname, LOG_PID); /* opening syslog */

	syslog(LOG_INFO, "Started");

	/* Complete all the sanity-checks before forking 
		1) Check the command line.
		2) Check if packet filter has been installed.
		3) Check if the device is present.
		4) Check if the /etc/ethers file is present.
	*/
	parse_cmdline(argc,argv);  /* read the command line arguments */

	/* check if the packet filter has been configured */

	nlist (system, nl);
	if (nl[0].n_type == 0) {
		fprintf(stderr, "%s: cannot find symbol Pfilt_read in %s\n", progname, system);
		fprintf(stderr, "option PACKETFILTER does not appear to be configured in your kernel.\n");
		syslog(LOG_ERR, "Packet Filter is not configured in /vmunix");
		exit(1);
	}

	/* Check if the device is present */
	if (pfopen(device, 2) < 0) {
		syslog(LOG_ERR, "%m: %s", device);
		perror(device);
		exit(1);
	}
	close (device);


	/* stat the ether_file */
	if (stat(ether_file, &stat_val) < 0) {
		syslog(LOG_ERR, "%m\(%s\)", ether_file);
		perror(ether_file);
		usage();
	}

	time_mod = stat_val.st_mtime;

#ifndef DEBUG
	if (fork())
		exit(0);
	else {
		(void) open("/", O_RDONLY);
		(void) dup2(0, 1);
		(void) dup2(0, 2);
		{ int tt = open("/dev/tty", O_RDWR);
			if (tt > 0) {
				(void) ioctl(tt, TIOCNOTTY, 0);
				(void) close(tt);
			} else {
				syslog(LOG_WARNING, "%m: Cannot disassociate from controlling terminal");
			}
	       }
	}

#endif DEBUG

	/* Read in the mapping file */
	read_mapping();

	/* All the data required is now available
	 * in the list pointed to by the enet_list
	 * pointer.
	 * Time to activate the filter.
	 */

	fid = set_filter(device);	/* Set the filter */

	/* flush the queue before commencing */
	if (ioctl(fid, EIOCFLUSH, 0) < 0) 
		syslog(LOG_WARNING, "%m: could not flush input queue");

	/* set the timer, inorder to check the status of the
	 * "ether_file" periodicaly at period defined by
	 * READ_INTERVAL
	 */

	/* first allocate memory  for timers */
	if ((value=(struct itimerval *)malloc(sizeof(struct itimerval)))
						== NULL) {
		syslog(LOG_ERR, "%m: could not malloc value itimerval struct");
		exit(1);
	}

	if ((ovalue=(struct itimerval *)malloc(sizeof(struct itimerval)))
						== NULL){
		syslog(LOG_ERR, "%m: could not malloc ovalue itimerval struct");
		exit(1);
	}

	/* load the timer */
	value->it_value.tv_sec = READ_INTERVAL;
	value->it_value.tv_usec = 0;

	/* load the re-load field */
	value->it_interval.tv_sec = READ_INTERVAL;
	value->it_interval.tv_usec = 0;

	while(1) {
		if (!disable) {
			/* start the timer */
			setitimer (ITIMER_REAL, value, ovalue);
			/* enable the signal */
			(void) signal (SIGALRM, time_out);
		}
		pack_len = listen_4_rarp(fid, pack_buf, sizeof(pack_buf));
		if (debug)
			syslog(LOG_DEBUG, "received %d bytes; processing packet", pack_len);
		res = get_ip_addr(pack_buf, send_buf, fid ); /* Get the ip mapping */
		if ((res == NONRARP) && verbose) {
			/* a RARP packet was not received
			 * happens when filter screws up
			 */
			sprintf(log_msg, "Received packet from ->");
			msg_len = delimit(log_msg, 0, '>') + 1;
			for (i = 6; i < 12 ; i++, msg_len += 3)
				sprintf(log_msg + msg_len,"%.2x:", pack_buf[32+i] & 0xFF);
			continue;
		}
		if (res < 0) {
			sprintf(log_msg, "Not in %s file->", ether_file);
			/* search for the delimiter */
			msg_len = delimit(log_msg, 0, '>') + 1;
			for (i = 0; i < 6 ; i++,msg_len += 3)
				sprintf(log_msg + msg_len , "%.2x:", pack_buf[32+i] & 0xFF);
			/* log into syslog */
			syslog(LOG_ERR, "%s", log_msg);
			if (verbose) 
				syslog(LOG_ERR, "No response sent");
			continue;
		}
		if (verbose && res > 0 ) {
			sprintf(log_msg, "Received IP address request for ->");
			/* search for the delimiter */
			msg_len = delimit(log_msg, 0, '>') + 1;
			for (i = 0; i < 6 ; i++, msg_len += 3) 
				sprintf(log_msg + msg_len,"%.2x:", pack_buf[32+i] & 0xFF);
			syslog(LOG_INFO, "%s", log_msg);
		}
		if(verbose) {
			sprintf(log_msg,"The IP address is ->");
			/* search for the delimiter */
			msg_len = delimit(log_msg, 0, '>') + 1;
			for (i = 0; i < 4 ; i++) {
				sprintf(log_msg + msg_len, "%d.", send_buf[38+i] & 0xFF);
				msg_len = delimit(log_msg + msg_len, msg_len, '.') + 1;
			}
			syslog(LOG_INFO, "%s", log_msg);

			if (debug)
				syslog(LOG_DEBUG, "Going to send packet");
		}
		send_rarp_resp(fid, send_buf, res);	
	}
}


/**********************************************************************
 * Read the command line parameters. The options available
 * are: 
 *	-v		Verbose
 *	-f filename	The filename containing mappings
 *
 * The -v flag invokes the verbose mode of operation. The
 * -f option causes a file other than /etc/ethers to be
 * read inorder to gain knowledge of the ethernet to 
 * internet address mapping. Any
 * other option is ignored.
 *
 *********************************************************************/


parse_cmdline(argc,argv)
int argc;
char **argv;

{
	int i;
		
	/* Set up default network device */
	sprintf(device, "%s", "pf0");

	if (argc > 1) {	/* if more than 1 arg, check options */
		for (i=1; --argc>0; i++) {
			if (argv[i][0] != '-') {
			/* Ethernet interface given on command line */
				sprintf(device, "%s", argv[i]);
				continue;
			}

			switch (argv[i][1]) {
				case 'v':	/* turn verbose on */
					verbose = 1;
					break;

				case 'f':	/* the file to be read */
					ether_file = argv[i+1];
					i++;
					break;
				
				case 'n':	/* disable the timer */
					disable = 1;
					break;

				case 'D':	/* turn debug on */
					debug = 1;
					break;

				default:
					fprintf(stderr, "%s: unknown option, %s\n", progname, argv[i]);
					syslog(LOG_ERR, "unknown option, %s\n", argv[i]);
					usage();
				break;
			}
		}
  	}
}

/**********************************************************************
 * time_out is the interupt handler routine for SIGALRM
 *
 **********************************************************************/

void
time_out()
{
	struct stat stat_val;

	(void) signal (SIGALRM, SIG_IGN);

	/* First, go and stat the file in question */
	if (stat(ether_file, &stat_val) < 0) {
		syslog(LOG_ERR, "%m \(%s\)", ether_file);
		goto enable;
	}
	
	if (time_mod < stat_val.st_mtime) {
		read_mapping();
		if (debug || verbose) {
			syslog(LOG_INFO, "%s modified on %s",
				ether_file, ctime(&stat_val.st_mtime));
			syslog(LOG_INFO, "Re-read %s", ether_file);
		}
		time_mod = stat_val.st_mtime; /* update */
	}
	enable: (void) signal (SIGALRM, time_out);
}

usage()
{
	fprintf(stderr,"usage: %s [ device ] [-v] [-f filename] [-n]\n", progname);
	exit(1);
}


/************************************************************* 
 *	 
 *	These routines are used to parse the file containing 
 * 	the Ethernet to Internet address mappings. The routine
 *	"get_ip_addr" reads the list of mappings and fills in
 *	appropriate IP addr. for the machine.
 *
 *	The format of the file containing mappings should be:
 *
 *	ether_addr <space or tab(s)> hostname <anything else>
 *
 *	where,
 *
 *	ether_addr is of the form : "a:b:c:d:e:f"
 *	and hostname is of the form "myhost"
 *
 *	NOTE:
 *	a, b, c, d, e and f MUST be in HEXADECIMAL
 *
 *	A "#" in the FIRST column ONLY implies a comment follows and 
 *	anything from that till the end of the line is ignored.
 *	The parser checks ONLY the first 2 fields
 *	of a line and reads in the data.
 *
 *	eg.  8:0:2b:2:8b:0	hishostname	#This field is not parsed
 *	     8:0:2b:2:8c:1 	herhostname	# comment goes here
 *	     8:0:2b:2:8b:0	myhostname	  ... and here	
 *
 *	HISTORY:
 *
 *	Created by : Uttam Shikarpur Nadig
 *      Date: 22 February 1989
 *
 */

/****************************************************************
 *	read_mapping reads the file "file" and creates a
 * mapping of Ethernet to Internet address in the mapper structure.
 *
 ***************************************************************/

mapper *enet_list = NULL; 	/* structure that contains the mappings */

void
read_mapping()
{

	FILE *fileid;		/* The data base file */
	char buff[MAX_LINE];	/* Buffer to read a line */
	int line_count;		/* # of lines in the file */
	int parse_result;	/* The result of the parse */
	int i;
	mapper *current;	/* The current positon in the list */
	mapper *previous;	/* The one but last positon in the list */
	mapper *tmp;		/* a temporary pointer */
	char log_msg[MSG_SIZE];	/* for logging in messages */
	int msg_len;		/* the current length */
	int mapsiz=0; 		/* Size of the mapper struct */

	if (debug)
		syslog(LOG_DEBUG, "Entered read_mapping");


	/* ignore SIGHUP for now */
	signal(SIGHUP, SIG_IGN);

	/* Ok, time to open the database for reading */
	fileid = fopen(ether_file, "r");
	if (fileid == NULL) {
		syslog(LOG_ERR, "%m\(%s\)", ether_file);
		exit(1);
	}
	if (debug)
		syslog(LOG_DEBUG, "Opened %s file", ether_file);

	/* 
	 * First read in the total number of lines.
	 */
	for (line_count = 1; fgets(buff, sizeof(buff), fileid) != NULL; ++line_count)
		; /* nop */
	rewind(fileid);

	if (debug)
		syslog(LOG_DEBUG, "%d lines in file", line_count);

	/* Allocate space for the table. If this is the second time this
	 * is being done deallocate the memory already allocated
	 */
	 tmp = current = enet_list;
	 if (enet_list != NULL) { /* free memory already allocated */
		do {
			free(current);
			current = tmp->next;
			tmp = current;
		} while (current != NULL);
		if (debug)
			syslog(LOG_DEBUG, "Freed up memory");
	 }
	/* Now allocate memory as required */
	if (( current = enet_list = (mapper *)malloc(sizeof(mapper) * line_count)) == NULL) {
		syslog(LOG_ERR, "%m: could not malloc mapper structure");
		exit(1);
	}
	current->next = NULL; 
	line_count = 0;  /* initialize */
	valid_count = 0; /* initialize */
	/* Read in the data from the file */
	while (fgets (buff, sizeof (buff), fileid) != NULL) {
		line_count++;
		if ((parse_result = parse_line(buff, current)) < 0) {
			syslog(LOG_ERR, "error in %s at line %d",
					ether_file, line_count);
			continue;	/* Read the next line */
		}
		if (parse_result > 0) {
			caddr_t tmp2 = 0;	/* temporary */
			valid_count++;	/* the line read in was valid */
			if (debug) {
				syslog(LOG_DEBUG,"Parse result is : %d", parse_result);
				sprintf(log_msg, "mapper->ether : ");
				/* find the de-limiter */
				for (msg_len = 0; log_msg[msg_len] != ':';msg_len++)
					; /* nop */
				for (i = 0; i < 6; i++, msg_len++)
					sprintf(log_msg + msg_len + i +1,
							"%.2x", current->ether[i]);
				syslog(LOG_DEBUG, "%s", log_msg);
		
				sprintf(log_msg, "mapper->ip : ");
		
				/* find the de-limiter */
				msg_len = delimit(log_msg, 0, ':') + 1;
		
				for (i = 0; i < 4; i++) {
					sprintf(log_msg + msg_len, "%d.", current->ip[i] & 0xFF);
					msg_len = delimit(log_msg + msg_len, msg_len, '.') + 1;
				}
				syslog(LOG_DEBUG, "%s", log_msg);
				/* lastly, the host name */
				sprintf(log_msg, "mapper->hostname : %s",
								current->hostname);
				/* put it into the syslog */
				syslog(LOG_DEBUG, "%s", log_msg);
			}
			mapsiz = sizeof(mapper);
			tmp2 = (caddr_t) current;
			tmp2 += mapsiz;
			current->next = (mapper*)tmp2;
			current = current->next;
			current->next = NULL;
		}
	}
	/* First, check for the number of valid lines read in */
	if (valid_count == 0){ /* Either the file was empty or none
			        *  of the entries were correct
			        */
		fprintf(stderr, "%s : Warning : no valid entries in %s\n",
					progname, ether_file);
		syslog(LOG_ERR, "no valid entries in %s", ether_file);
	} else { /* atleast one valid entry was found */
		previous = (current - sizeof(mapper));/*Allocated one too many*/
		previous->next = NULL;
	}
	rewind(fileid);		/* rewind */
	close(fileid);		/* close the file */

	/* reset SIGHUP signal */
	signal(SIGHUP, read_mapping);
}


/****************************************************************
 *
 *	Parses an input line read from the RARP database. The
 * expected format of the entry is :
 *	a:b:c:d:e:f <sp or tab(s)> hostname
 *	Anything after a "#" till the end of that line is ignored.
 *
 ****************************************************************/

parse_line(line, pointer)
char *line;	 /* The entry read from the RARP database */
mapper *pointer; /* the mapper structure that needs to filled up */
{

	 int count;			/* The numb. of fields scanned */
	 char hostname[MAXHOSTLEN];	/* The host name */
	 u_char enet[20];		/* The ether net address */

	 if ((line[0] == '#'))
		return(0);	/* It is a comment line */

	 if ((count = sscanf(line, "%s%s", enet, hostname)) < 2)
		return(0); /* ignore the line as a comment as it has
			    * less than 2 fields
			    */

	 /* time to do some conversions */
	 if (conv_enet(enet, pointer) < 0 ) /* convert the ethernet address */
	 	return(-1);
	 if (conv_ip(hostname, pointer) < 0 ) /* convert the ip address */
	 	return(-1);
	 return(1);
}

/****************************************************************
 *	
 *	conv_enet scans the the string to see if it of the type
 * a:b:c:d:e:f. If so, the ":" from the string are removed and
 * and stored into the list pointed to by "pointer."
 *
 ***************************************************************/

 conv_enet(enet, pointer)
 u_char *enet;
 mapper *pointer;

{
	unsigned int numb[6];		/* for temp. storage */
	int i;

	if (debug)
		syslog(LOG_DEBUG, "Entered conv_enet ...");

	if (sscanf(enet, "%x%*c%x%*c%x%*c%x%*c%x%*c%x", &numb[0], &numb[1],
			&numb[2], &numb[3], &numb[4], &numb[5] ) != 6) {
		syslog(LOG_ERR, "syntax error while parsing Ethernet addr");
		return(-1);	/* Six fields were not present */
	}

	if ((pointer->ether = (u_char *)malloc (sizeof (u_char) * 7)) == NULL){
		syslog(LOG_ERR, "Cannot allocate memory in conv_enet.");
		exit(1);
	}

	for (i = 0; i < 6; i++) 
		pointer->ether[i] = numb[i] & 0xFF;

	pointer->ether[6] = '\0'; /* null terminate */
	if (debug)
		syslog(LOG_DEBUG,"Finished conv_ether successfully ...");

	return(0);	/* ALL is clear on the conv_enet front */
}


 /******************************************************************
  *  conv_ip address takes the hostname given and converts that
  * to the ip format and stores it in the ip field.
  ******************************************************************/
conv_ip (hostname, pointer)
char *hostname;
mapper *pointer;

{
	struct hostent *host_info;	

	/* get the "hostname" characteristics */	
	host_info = gethostbyname(hostname);

	if (host_info == NULL) {
		syslog(LOG_ERR, "Host not found: %s", hostname);
		fprintf(stderr, "%s: unknown host: %s\n", progname, hostname);
		return(-1); /* not a host name */
	}

	/* Allocate memory */
	if (( pointer->ip = (u_char *)malloc (sizeof (u_char) * 4)) == NULL ) {
		syslog(LOG_ERR, "malloc failed in conv_ip");
		exit(1);
	}
	bcopy (host_info->h_addr, pointer->ip, 4);
	bcopy (hostname, pointer->hostname, strlen(hostname));
	if (debug)
		syslog(LOG_DEBUG, "Finished conv_ip successfully ... ");
	return(0);
}


/***************************************************************
 *	
 *	This routine does a linear search on the list pointed
 * to by enet_list and finds the appropriate internet address
 * for the given ethernet hardware address. In case the address
 * is not found an error is returned
 *
 *
 *	A RARP message consists of the following bytes of
 * data. The format being:
 *
 *	 -------------------------------------------------------------
 *	|destination addr(6) | source addr (6) | Packet type-RARP(2)  |
 *	 -------------------------------------------------------------
 *	|addr. space(2)| protocol(2) | hardware addr length(1) 	      |
 *	 -------------------------------------------------------------
 *	|logical addr length(1) | RARP type(1) | src hardware addr(n) |
 *	 -------------------------------------------------------------
 *	|src IP addr (m) | target hardware addr (n)		      |
 *	 -------------------------------------------------------------
 *	|	target IP addr (m) - THE DESIRED IP ADDRESS	      |
 *	 -------------------------------------------------------------
 *
 *	Where 'n' and 'm' are the values in the hardware addr. length
 * field and logical addr. length field respectively.
 *
 *	A valid assumption made is that ethernet address are 6 bytes
 * long and all IP addresses are 4 bytes long.
 *
 **********************************************************************/

get_ip_addr(recv_pack, send_pack, fid)
u_char *recv_pack;		/* The packet received */
u_char *send_pack;		/* The packet to be sent */
int fid;			/* The device file descriptor */

{

	mapper *list_ptr;
	u_char ether_buff[7];
	struct endevp devparams;	/* to fetch the device params. */
	char lhostname[MAXHOSTLEN];	/* The name of the local host */
	struct hostent *lhost;		/* local host characteristics */
	int i,result;
	char log_msg[MSG_SIZE];	/* the syslog message buffer */
	int msg_len = 0;		/* the pointer to current location */

	if (debug) {	/* Print the entire packet received */
		syslog(LOG_DEBUG, "THE CONTENTS OF THE PACKET RECEIVED ARE: ");
		sprintf(log_msg,"Destination(6) : "); 
		/* search for the delimiter */
	 	msg_len = delimit(log_msg, msg_len, ':') + 1;
		for (i = 0; i < 6; i++,msg_len += 3)
			sprintf(log_msg + msg_len, "%.2x:", recv_pack[i] & 0xFF); 
		syslog(LOG_DEBUG, "%s", log_msg);
		sprintf(log_msg , "Source(6) : "); 

		/* search for the delimiter */
		msg_len = delimit(log_msg, 0, ':') + 1;
		for (i = 6; i < 12; i++,msg_len += 3)
			sprintf(log_msg + msg_len, "%.2x:", recv_pack[i] & 0xFF); 
		syslog(LOG_DEBUG, "%s", log_msg);
		sprintf(log_msg , "Packet type(2): 0x"); 

		/* search for the delimiter */
		msg_len = delimit(log_msg, 0, 'x') + 1;
		for (i = 12; i < 14; i++, msg_len += 2)
			sprintf(log_msg + msg_len , "%.2x", recv_pack[i] & 0xFF); 
		syslog(LOG_DEBUG, "%s", log_msg);

		/* check if the packet was of RARP type
		 * if not don't print the rest
		 */
		if (recv_pack[12] != 0x80 || recv_pack[13] != 0x35)
			return(NONRARP); /* received a NON-RARP packet */
		sprintf(log_msg , "Hardware addr. space type(2): 0x"); 

		/* search for the delimiter */
		msg_len = delimit(log_msg, 0, 'x') + 1;

		for (i = 14; i < 16; i++, msg_len += 2)
			sprintf(log_msg + msg_len, "%.2x", recv_pack[i] & 0xFF); 
		syslog(LOG_DEBUG, "%s", log_msg);
		sprintf(log_msg, "Protocol addr. space type(2): 0x"); 

		/* search for the delimiter */
		msg_len = delimit(log_msg, 0, 'x') + 1;
		for (i = 16; i < 18; i++,msg_len += 2)
			sprintf(log_msg + msg_len, "%.2x", recv_pack[i] & 0xFF); 
		syslog(LOG_DEBUG, "%s", log_msg);
		sprintf(log_msg, "Hardware addr. length(1): 0x"); 

		/* search for the delimiter */
		msg_len = delimit(log_msg, 0, 'x');
		sprintf(log_msg + msg_len + 1, "%.2x", recv_pack[18] & 0xFF); 
		syslog(LOG_DEBUG, "%s", log_msg);
		sprintf(log_msg, "IP addr. length(1): 0x"); 

		/* search for the delimiter */
		msg_len = delimit(log_msg, 0, 'x') + 1;
		sprintf(log_msg + msg_len, "%.2x", recv_pack[19] & 0xFF); 
		syslog(LOG_DEBUG, "%s", log_msg);
		sprintf(log_msg, "RARP type(2): 0x");

		/* search for the delimiter */
		msg_len = delimit(log_msg, 0, 'x') + 1;
		for (i = 20; i < 22; i++,msg_len += 2)
			sprintf(log_msg + msg_len, "%.2x", recv_pack[i] & 0xFF); 
		syslog(LOG_DEBUG, "%s", log_msg);
		sprintf(log_msg, "Source hardware addr.(6):"); 

		/* search for the delimiter */
		msg_len = delimit(log_msg, 0, ':') + 1;

		for (i = 22; i < 28; i++,msg_len += 3)
			sprintf(log_msg + msg_len, "%.2x:", recv_pack[i] & 0xFF); 
		syslog(LOG_DEBUG, "%s", log_msg);
		sprintf(log_msg, "Source IP addr.(4): "); 

		/* search for the delimiter */
		msg_len = delimit(log_msg, 0, ':') + 1;
		for (i = 28; i < 32; i++, msg_len += 2)
		sprintf(log_msg + msg_len, "%d.", recv_pack[i] & 0xFF); 
		syslog(LOG_DEBUG, "%s", log_msg);
		sprintf(log_msg, "Target hardware addr.(6):"); 

		/* search for the delimiter */
		msg_len = delimit(log_msg, 0, ':') + 1;
		for (i = 32; i < 38; i++, msg_len += 3)
			sprintf(log_msg + msg_len, "%.2x:", recv_pack[i] & 0xFF); 
		syslog(LOG_DEBUG, "%s", log_msg);
		sprintf(log_msg, "Target IP addr.(4):"); 

		/* search for the delimiter */
		msg_len = delimit(log_msg, 0, ':') + 1;
		for (i = 38; i < 42; i++, msg_len += 2)
		sprintf(log_msg + msg_len, "%d.", recv_pack[i] & 0xFF); 

		/* Put this information into the syslog */
		syslog(LOG_DEBUG, "%s", log_msg);
	}
	/* check if the packet was of RARP type
	 * if not don't print the rest
 	 * This was tracked to not clearing the buffer
	 * before using the filter. (Left here for reference.)
	 */
	if (recv_pack[12] != 0x80 || recv_pack[13] != 0x35)
		return(-1);	

	/* get the ether_net address which needs to be mapped */
	ether_buff[0] = recv_pack[32];	
	ether_buff[1] = recv_pack[33];	
	ether_buff[2] = recv_pack[34];	
	ether_buff[3] = recv_pack[35];	
	ether_buff[4] = recv_pack[36];	
	ether_buff[5] = recv_pack[37];	
	ether_buff[6] = '\0';		/* NULL terminate */

	/* Time to string compare and fetch out the right logical address 
	 * But, first find out if we have any valid entries by checking
	 * the variable valid count.
	 */
	if (valid_count == 0)
		return (-1);

	list_ptr = enet_list;

	while(1) {	/* not an infinite loop; returns */
		if ((result = bcmp(ether_buff, list_ptr->ether, 6)) == 0) {
			/* We have the mapping; */
			/* Build the packet to be sent */

			/* The destination address */ 
			send_pack[0] = recv_pack[6];
			send_pack[1] = recv_pack[7];
			send_pack[2] = recv_pack[8];
			send_pack[3] = recv_pack[9];
			send_pack[4] = recv_pack[10];
			send_pack[5] = recv_pack[11];

			/* The Source address; added by the driver */
			send_pack[6] = 0x00;
			send_pack[7] = 0x00;
			send_pack[8] = 0x00;
			send_pack[9] = 0x00;
			send_pack[10] = 0x00;
			send_pack[11] = 0x00;

			/* The Ethernet packet type - RARP */
			send_pack[12] = recv_pack[12];
			send_pack[13] = recv_pack[13];
		
			/* The Hardware addr. space - ar$hrd */
			send_pack[14] = recv_pack[14];
			send_pack[15] = recv_pack[15];

			/* The logical address space - ar$pro */
			send_pack[16] = recv_pack[16];
			send_pack[17] = recv_pack[17];

			/* The Hardware address length - ar$hln */
			send_pack[18] = recv_pack[18];

			/* The logical address length - ar$pln */
			send_pack[19] = recv_pack[19];

			/* RARP reply type - ar$op; 4 */
			send_pack[20] = 0x00;
			send_pack[21] = 0x04;

			/* The host hardware info -ar$sha; need to get it */
			if (ioctl(fid, EIOCDEVP, &devparams) < 0) {
				syslog(LOG_ERR, "%m:device");
				exit(1);
			}
			/* NOTE: The host is assumed to have an ethernet
			 * address of 6 bytes.
			 */
			send_pack[22] = devparams.end_addr[0]; 
			send_pack[23] = devparams.end_addr[1]; 
			send_pack[24] = devparams.end_addr[2]; 
			send_pack[25] = devparams.end_addr[3]; 
			send_pack[26] = devparams.end_addr[4]; 
			send_pack[27] = devparams.end_addr[5]; 
			
			/* The host IP address - ar$spa; need to fetch it */
			if ((gethostname(lhostname, sizeof(lhostname))) < 0) {
				syslog(LOG_ERR, "%m: gethostname");	
				exit(1);
			}
			/* get the host info */
			do {
				lhost = gethostbyname(lhostname);
			} while (lhost != NULL && lhost->h_addrtype != AF_INET);

			if (lhost == NULL) {
				syslog(LOG_ERR, "%m:gethostbyname");
				exit(1);
			}

			/* NOTE: An assumption is made that the
			 * length of the address returned is contained
			 * in 4 bytes.
			 * The address is returned in the network
			 * byte order.
			 */
			send_pack[28] = lhost->h_addr[0];
			send_pack[29] = lhost->h_addr[1];
			send_pack[30] = lhost->h_addr[2];
			send_pack[31] = lhost->h_addr[3];

			/* The targets hardware address - ar$tha.
			 * This can be different from the source 
			 * of the RARP request.
			 */
			send_pack[32] = recv_pack[32];
			send_pack[33] = recv_pack[33];
			send_pack[34] = recv_pack[34];
			send_pack[35] = recv_pack[35];
			send_pack[36] = recv_pack[36];
			send_pack[37] = recv_pack[37];

			/* The reason for this whole thing ...!
			 * the targets IP address - ar$tpa.
			 */
			send_pack[38] = list_ptr->ip[0];
			send_pack[39] = list_ptr->ip[1];
			send_pack[40] = list_ptr->ip[2];
			send_pack[41] = list_ptr->ip[3];
			if (debug) {	/* Print the entire packet to be sent */
				syslog(LOG_DEBUG, "THE CONTENTS OF THE PACKET TO BE SENT ARE: ");
				sprintf(log_msg,"Destination(6) : "); 

				/* search for the delimiter */
				msg_len = delimit(log_msg, 0, ':') + 1;
				for (i = 0; i < 6; i++,msg_len += 3)
					sprintf(log_msg + msg_len, "%.2x:", send_pack[i] & 0xFF); 

				syslog(LOG_DEBUG, "%s", log_msg);
				sprintf(log_msg , "Source(6) : "); 

				/* search for the delimiter */
				msg_len = delimit(log_msg, 0, ':') + 1;

				for (i = 6; i < 12; i++,msg_len += 3)
					sprintf(log_msg + msg_len, "%.2x:", send_pack[i] & 0xFF); 
				syslog(LOG_DEBUG, "%s", log_msg);
				sprintf(log_msg , "Packet type(2): 0x"); 

				/* search for the delimiter */
				msg_len = delimit(log_msg, 0, 'x') + 1;

				for (i = 12; i < 14; i++,msg_len += 2)
					sprintf(log_msg + msg_len , "%.2x", send_pack[i] & 0xFF); 
				syslog(LOG_DEBUG, "%s", log_msg);
				sprintf(log_msg , "Hardware addr. space type(2): 0x"); 

				/* search for the delimiter */
				msg_len = delimit(log_msg, 0, 'x') + 1;

				for (i = 14; i < 16; i++, msg_len++)
					sprintf(log_msg + msg_len, "%.2x", send_pack[i] & 0xFF); 
				syslog(LOG_DEBUG, "%s", log_msg);
				sprintf(log_msg, "Protocol addr. space type(2):0x"); 

				/* search for the delimiter */
				msg_len = delimit(log_msg, 0, 'x') + 1;
				for (i = 16; i < 18; i++,msg_len += 2)
					sprintf(log_msg + msg_len, "%.2x", send_pack[i] & 0xFF); 
				syslog(LOG_DEBUG, "%s", log_msg);
				sprintf(log_msg, "Hardware addr. length(1): 0x"); 

				/* search for the delimiter */
				msg_len = delimit(log_msg, 0, 'x');

				sprintf(log_msg + msg_len + 1, "%.2x", send_pack[18] & 0xFF); 
				syslog(LOG_DEBUG, "%s", log_msg);
				sprintf(log_msg, "IP addr. length(1): 0x"); 

				/* search for the delimiter */
				msg_len = delimit(log_msg, 0, 'x');

				sprintf(log_msg + msg_len + 1, "%.2x", send_pack[19] & 0xFF); 
				syslog(LOG_DEBUG, "%s", log_msg);
				sprintf(log_msg, "RARP type(2): 0x");

				/* search for the delimiter */
				msg_len = delimit(log_msg, 0, 'x') + 1;
				for (i = 20; i < 22; i++,msg_len++)
					sprintf(log_msg + msg_len, "%.2x", send_pack[i] & 0xFF); 
				syslog(LOG_DEBUG, "%s", log_msg);
				sprintf(log_msg, "Source hardware addr.(6):"); 

				/* search for the delimiter */
				msg_len = delimit(log_msg, 0, ':') + 1;

				for (i = 22; i < 28; i++,msg_len += 3)
					sprintf(log_msg + msg_len, "%.2x:", send_pack[i] & 0xFF); 

				syslog(LOG_DEBUG, "%s", log_msg);
				sprintf(log_msg, "Source IP addr.(4): "); 

				/* search for the delimiter */
				msg_len = delimit(log_msg, 0, ':') + 1;

				for (i = 28; i < 32; i++) {
					sprintf(log_msg + msg_len, "%d.", send_pack[i] & 0xFF); 
					msg_len = delimit (log_msg + msg_len, msg_len, '.') + 1;
				}
				syslog(LOG_DEBUG, "%s", log_msg);
				sprintf(log_msg, "Target hardware addr.(6):"); 

				/* search for the delimiter */
				msg_len = delimit(log_msg, 0, ':') + 1;
				for (i = 32; i < 38; i++, msg_len += 3)
					sprintf(log_msg + msg_len, "%.2x:", send_pack[i] & 0xFF); 
				syslog(LOG_DEBUG, "%s", log_msg);
				sprintf(log_msg, "Target IP addr.(4):"); 

				/* search for the delimiter */
				msg_len = delimit(log_msg, 0, ':') + 1;
				for (i = 38; i < 42; i++) {
					sprintf(log_msg + msg_len, "%d.", send_pack[i] & 0xFF); 
					msg_len = delimit (log_msg + msg_len, msg_len, '.') + 1;
				}
				/* Put this information into the syslog */
				syslog(LOG_DEBUG, "%s", log_msg);
			}
			return(42); 	/* return # of bytes sent */
		} else {
			/* go to the next element in the list */
			if ((list_ptr = list_ptr->next) == NULL)/* end of list*/
				return(-1);
			else
				continue;
		}
	}
}


 /*********************************************************************
  * search for a delimiter
  ********************************************************************/

delimit(buffer, len, delimiter)
char *buffer;
int len;
char delimiter;
{
	int i;

	for (i = 0; buffer[i] != delimiter; i++, len++)
		;	/* nop */
	return(len);
}

int Fid;
struct endevp DevParams;

/*
 * Program to detect broadcast packets for
 * Reverse ARP using the packet filter.
 * The filter need not be run in the promiscuous
 * mode; the filter accepts all broadcast packets.
 */
set_filter(devname)
char *devname;
{
	struct eniocb IOCB;
	short enmode;
	int backlog = -1;
	char *packet;
	/* Open the device specified - should not fail */
	if ((Fid = pfopen(devname, 2)) < 0) {
		syslog(LOG_ERR, "%m: %s", devname);
		exit(1);
	}
	if (ioctl(Fid, EIOCDEVP, &DevParams) < 0) {
		syslog(LOG_ERR, "%m %s", devname);
		exit(1);
	}
	if (verbose) 
		PrintParams();
		
	IOCB.en_rtout = 100;	/* in clock ticks */

	if (ioctl(Fid, EIOCSETP, &IOCB) < 0) {
		syslog(LOG_ERR, "%m %s", devname);
		exit(1);
	}

	/* set the filter */
	enmode = ENBATCH|ENTSTAMP|ENPROMISC|ENNONEXCL;
	if (ioctl(Fid, EIOCMBIC, &enmode) < 0) {
		syslog(LOG_ERR, "%m %s", devname);
		exit(1);
	}

	if (ioctl(Fid, EIOCSETW, &backlog) < 0) {
		syslog(LOG_ERR, "%m %s", devname);
		exit(1);
	}
	BuildFilter();
	return(Fid);		/* return the file descriptor */	
}

/*
 * Filter checks for a reverse ARP packet.
 *
 * Design:
 *	The filter checks the type of the packet. If it
 *	isn't a RARP packet, it gets junked. 
 *	After confirming it to be a RARP packet
 *	the type field of the RARP packet is checked. Only
 *	type 3 packets - request reverse - are accepted.
 *	Packet types 1,2 and 4 are ignored. If the
 *	packet passes the test, it is checked to see
 * 	if it was a broadcast RARP.
 *	This design provides the minimum amount of
 *	processing overhead.
 * 
 */

BuildFilter()
{

	struct enfilter Filter;

	Filter.enf_Priority =  36;/* The priority */
	Filter.enf_FilterLen = 0; /* The length of the filter */

	/* Check the type field in the EtherHeader */
	Filter.enf_Filter[Filter.enf_FilterLen++] = ENF_PUSHWORD + 6;
	Filter.enf_Filter[Filter.enf_FilterLen++] = ENF_PUSHLIT | ENF_CAND;	
	Filter.enf_Filter[Filter.enf_FilterLen++] = F_RARP;
	
	/* Check the type field in the RARP packet */
	Filter.enf_Filter[Filter.enf_FilterLen++] = ENF_PUSHWORD + 10;
	Filter.enf_Filter[Filter.enf_FilterLen++] = ENF_PUSHLIT | ENF_CAND;	

	/* Check if it is a request packet of RARP */
	Filter.enf_Filter[Filter.enf_FilterLen++] = F_REQREVR;

	/* If the above test is true go and check the 
	 * destination address. Is it a broadcast packet?
	 */

	/* Check the first word of destination address */
	Filter.enf_Filter[Filter.enf_FilterLen++] = ENF_PUSHWORD + 0;
	Filter.enf_Filter[Filter.enf_FilterLen++] = ENF_PUSHLIT | ENF_CAND;
	Filter.enf_Filter[Filter.enf_FilterLen++] = 0xFFFF; 


	/* Check the second word of destination address */
    	Filter.enf_Filter[Filter.enf_FilterLen++] = ENF_PUSHWORD + 1;
	Filter.enf_Filter[Filter.enf_FilterLen++] = ENF_PUSHLIT | ENF_CAND;
	Filter.enf_Filter[Filter.enf_FilterLen++] = 0xFFFF;

	/* Check the third word of destination address */
	Filter.enf_Filter[Filter.enf_FilterLen++] = ENF_PUSHWORD + 2;
	Filter.enf_Filter[Filter.enf_FilterLen++] = ENF_PUSHLIT | ENF_EQ;
	Filter.enf_Filter[Filter.enf_FilterLen++] = 0xFFFF;


	/* Set the filter */
	if (ioctl(Fid, EIOCSETF, &Filter) < 0) {
		syslog(LOG_ERR, "%m: could not set filter");
		exit(1);
	}	
}

PrintParams()
{
	char log_msg[MSG_SIZE]; /* for logging in messages */
	int msg_len;            /* the current length */
	int i;

	syslog(LOG_INFO, "DEVICE PARAMETERS:");
	sprintf(log_msg, "Device type: ");
	msg_len = delimit (log_msg,0,':') + 1;

	switch (DevParams.end_dev_type) {
		case ENDT_3MB:
			sprintf(log_msg + msg_len, "ENDT_3MB");
			break;

		case ENDT_BS3MB:
			sprintf(log_msg + msg_len, "ENDT_BS3MB");
			break;

		case ENDT_10MB:
			sprintf(log_msg + msg_len, "ENDT_10MB");
			break;

		default:
			sprintf(log_msg + msg_len, "%d\n", DevParams.end_dev_type);
			break;
	}
	syslog(LOG_INFO, "%s", log_msg);
	syslog(LOG_INFO, "Address Length: %d", DevParams.end_addr_len);
	syslog(LOG_INFO, "Header Length: %d", DevParams.end_hdr_len);
	syslog(LOG_INFO, "MTU: %d", DevParams.end_MTU);
	sprintf(log_msg, "Hardware address -> ");
	msg_len = delimit (log_msg, 0, '>') + 1;

	for (i = 0; i < 6; i++, msg_len++) {
		sprintf( log_msg + msg_len, "%.2x", 0xFF & DevParams.end_addr[i]);
		msg_len += 2;
		sprintf(log_msg + msg_len, ":");
	}
	syslog(LOG_INFO, "%s", log_msg);
	sprintf(log_msg, "Broadcast address -> ");
	msg_len = delimit (log_msg, 0, '>') + 1;

	for (i = 0; i < 6; i++, msg_len++) {
		sprintf( log_msg + msg_len, "%.2x", 0xFF & DevParams.end_broadaddr[i]);
		msg_len += 2;
		sprintf(log_msg + msg_len, ":");
	}
	syslog(LOG_INFO, "%s", log_msg);
}

PrintBits(len, data)
int len;
u_char *data;
{
	char log_msg[MSG_SIZE]; /* for logging in messages */
	int msg_len;            /* the current length */
	int i;
	unsigned int j;
	unsigned int typ, type1, type2;

	type1 = data[12];
	type2 = data[13];
	typ   = type1 << 8;
	typ   = (typ | type2)&0xFFFF;/* just paranoid */

	/* check the packet type received */
	switch(typ) {
		case RARP:
			sprintf(log_msg, "RARP Broadcast from Host -> ");
			break;

		default: /* Should NEVER happen, unless the filter bombs */
			syslog(LOG_ERR, "Unknown Broacast detected ....!!!");

			syslog(LOG_ERR, "The type received was %x", typ & 0xFFFF);
			sprintf(log_msg, "Unknown Broadcast from Host -> ");
			break;
	}
	/* Print the source ethernet address */
	msg_len = delimit (log_msg, 0, '>') + 1;
	for (i = 6; i < 12; i++, msg_len++) {
		sprintf( log_msg + msg_len, "%.2x", 0xFF & data[i]);
		msg_len += 2;
		sprintf(log_msg + msg_len, ":");
	}
	syslog(LOG_INFO, "%s", log_msg);
}

/*
 * This contains routines to read and write packets from and
 * to the network.
 */
/*****************************************************************
 * 	listen_4_rarp reads the buffer of the device in 
 * question and retuns the contents of the packet when
 * a packet arrives.
 *
 ****************************************************************/
listen_4_rarp (fid, buffer, buf_size)
int fid;
char *buffer;
int buf_size;
{
	int buflen;
	long clock;	/* to keep track of time in verbose */

	while (1) {
		buflen = read(fid, buffer, buf_size);
		if (buflen < 0) {
			perror("rarpd: read:");
			return(-1);
		} else {
			if (buflen == 0) {	/* nothing as yet */
				continue;
			} else {
				if (verbose) {
					syslog(LOG_INFO, "Received %d bytes:", buflen);
					PrintBits(buflen, buffer);
				}
				return(buflen);	/* Received a packet */
			}
		}
	}
}

/*****************************************************************
 *	send_rarp_resp is a routine to write back the RARP reply back
 * on the device. It sends out a response to the RARP request
 * received.
 *
 ****************************************************************/

send_rarp_resp (fid, buffer, buf_size)
int fid;
char *buffer;
int buf_size;
{
	int nbytes;

	if ((nbytes = write (fid, buffer, buf_size)) < 0) {
		syslog(LOG_ERR, "%m write to device failed");
		exit(1);
	}

	if (debug || verbose)
		syslog(LOG_INFO, "Sent out response");

}
