#ifndef lint
static	char	*sccsid = "@(#)netsetup.c	4.3	(ULTRIX)	10/12/90";
#endif lint
/************************************************************************
 *									*
 *			Copyright (c) 1984 by				*
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

/*
 *  Sets up:
 *	/etc/hosts
 *	/etc/networks
 *	/etc/hosts.equiv
 *	/etc/dgateway
 *	/usr/spool/rwho directory
 *	/usr/hosts directory(s)
 *
 *  Modifies:
 *	/etc/rc.local
 *	/opr/restart
 *
 *  Does NOT handle:
 *	/usr/lib/sendmail.cf
 *	multiple interfaces (networks)
 *
 * Modification History:
 *
 * 20-Jul-90  -  Lea Gottfredsen
 *	Fixed check for setupisdone, no longer relies on networks file.
 *	There always is one now.  Also added question on routed.
 *
 * 13-Oct-89	robin
 *	ISIS pool merge and "ne" driver additions for chc
 *
 * 07-Mar-89  -  templin
 *	Added XNA driver support
 *
 * 04-Jan-88  -  logcher
 *	Fixed the minor bug in how /etc/networks is generated when 
 *	using subnets.
 *
 * 25-Aug-87  -  lp
 *	Fixed a minor bug in how /etc/networks file was being generated.
 *
 * 31-Jul-87  -  logcher
 *	Added hostname to printf for Class C network.  Updated
 *	network device name table.
 *
 * 11-Dec-86  -  Marc Teitelbaum
 *	Look in /sys/conf/MACHINE for the default network interface.
 *	This is better than guessing from CPU type, as BI machines
 *	can have a unibus.  /etc/sizer should have the last say on
 *	whats out there anyway.  Also, fix boundary conditions for
 *	network and host numbers to match documentation.
 *
 * 06-Aug-86  -  marc
 *	Rewritten from ULTRIX-11 netsetup.c
 *	Reworked prompts with Liza.
 *
 *	For internets consisting of multiple LANS and many hosts, it
 *	is expected that "netsetup install" will be used to setup a host
 *	on the single LAN it is directly connected to, and then the
 *	system administrator will rcp the /etc/hosts and /etc/networks
 *	files from some host on that LAN which acts as the central
 *	network administrative site.  I.e., i can't imagine anyone would
 *	want to use a program to type in 60 or more host names and internet
 *	addresses...  
 */
#include <stdio.h>
#include <sys/types.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/stat.h>
#include <strings.h>

/*
#define	DEBUG
*/

#ifndef	DEBUG
#define	HOSTS		"/etc/hosts"
#define	RC		"/etc/rc.local"
#define	RESTART		"/opr/restart"
#define	HOSTS_EQUIV	"/etc/hosts.equiv"
#define	NETWORKS	"/etc/networks"
#define	DGATEWAY	"/etc/dgateway"
#define RWHODIR		"/usr/spool/rwho"
#define	UHOSTS		"/usr/hosts"
#ifndef CONFDIR
#define CONFDIR		"/sys/conf"
#endif	CONFDIR
#else	DEBUG
#define	HOSTS		"./hosts"
#define	RC		"./rc.local"
#define	RESTART		"./restart"
#define	HOSTS_EQUIV	"./hosts.equiv"
#define	NETWORKS	"./networks"
#define	DGATEWAY	"./dgateway"
#define RWHODIR		"./rwho"
#define	UHOSTS		"./usr.hosts"
#undef CONFDIR
#define CONFDIR		"./conf"
#endif	DEBUG

#define	NO	0
#define	YES	1
#define	YESNO	2

#define TRUE    1
#define FALSE   0

#define eqsn(s1,s2,n) (strncmp(s1,s2,n) == 0)
#define plural(s) (index(s,' ')||index(s,'\t'))

/*
 * Non-int function declarations.
 */
FILE *edit();
char *prompt();
char *quote();
char *emalloc();
char *get_interface();

char hostname[256];

char *prmsg;
int subbits; 	/* number of bits of host used for subnet, 0 => no subnet */

/*
 * This is the list of interface names we will try to recognize from
 * the config file in determining the default network interface.
 * The first match wins - so watch the order when adding new entries.
 */
char *if_table[] = 
{
	"xna",	/* DEBNI - BI bus */
	"ni", 	/* DEBNT, DEBNA - BI bus */
	"de",	/* DEUNA, DELUA - UNIBUS */
	"qe",	/* DEQNA, DELQA - Q-Bus  */
	"ln",	/* DESVA, Busless LANCE network interface */
	"ne",	/* SGEC network interface */ 
	"fza",	/* DEFZA - TURBOchannel FDDI network interface */
	"dmc",	/* Point to Point */
	"scs",	/* SCS network driver - currently to the CI only*/
	0
};

struct dqueue {
	struct dqueue *dq_forw;
	struct dqueue *dq_back;
};

#define MAXDEVNAMLEN 9
struct device {
	struct dqueue dv_queue;	/* linkage */
	char dv_name[MAXDEVNAMLEN + 1];
};

/*
 * List of devices found in config file.
 */
struct dqueue device = { &device, &device };

main(argc, argv)
int argc;
char **argv;
{
	char gateway[256];
	char gateacct[256];
	char machabbrev[256];
	char machname[256];
	char netname[256];
	char netalias[256];  
	char trustname[256];
	char buf[512];
	char buf2[512];
	char s[256];
	char *cp, *xp;
	long netnum, machnum;
	struct in_addr addr, broadaddr, netmask, taddr1, taddr2;
	FILE *fp;
	struct hostent *hp;
	struct netent *np;
	long getnet(), getmach();
	int max;     /* max number of bits allowed for subnet on a
		      * given class network.  we allow up to the total
		      * host length minus three (we save 3 for the host).
		      */
	int mask; /* number of hostnumber bits (network - subnet) */
	int oneorzero; /* flag if we use all ones or all zero broadcast addr */
	int installflg = FALSE;
	int setupisdone = FALSE;
	int dosetup = FALSE;
	int routed = FALSE;
	int i;

	char *if_default = 0; 	/* default NI as determined by get_interface() */


	for (i = 3; i < 20; i++)
		close(i);
	dup2(1, 3);
#ifndef DEBUG
	if (getuid()) {
		fprintf(stderr, "\nYou must be root to set up your system on a local area network.\n");
		leave(1);
	}
#endif  DEBUG
	printf("\nYou will be asked a series of questions about your system.\n");
	printf("Default answers are shown in square brackets ([]).\n");
	printf("To use a default answer, press the RETURN key.\n\n");

	hostname[0] = '\0';
	if (gethostname(hostname, 255) < 0 || !hostname[0]) {
		/*
		 * Grab a line like
		 *	hostname myname
		 * or
		 *	/bin/hostname myname
		 * out of the /etc/rc file, to find the name.
		 */
		fp = fopen(RC, "r");
		if (fp != NULL) {
			cp = NULL;
			while (fgets(buf, 512, fp) != NULL) {
				if (strncmp(buf, "hostname", 8) == 0)
					cp = &buf[8];
				else if (strncmp(buf, "/bin/hostname", 13) == 0)
					cp = &buf[13];
				else
					continue;
				break;
			}
			if (cp != NULL) {
				while (*cp == ' ' || *cp == '\t')
					cp++;
				if (*cp) {
					xp = hostname;
					while (*cp && *cp != ' ' &&
						*cp != '\t' && *cp != '\n')
						*xp++ = *cp++;
					*xp = '\0';
				}
			}
			fclose(fp);
		}
	}
	if (!hostname[0]) {
		fprintf(stderr, "\nYour system does not have a name.\n");
		fprintf(stderr, "Check the /etc/rc.local file to be sure it\n");
		fprintf(stderr, "contains a hostname entry.\n\n");
		leave(1);
	}


	if (argc > 1 && (strcmp(argv[1], "install") == 0))
		installflg = TRUE; 

	sprintf(s,"grep -s \"ifconfig HDWR\" %s",RC);
	if (system(s))
		setupisdone = TRUE;

	if (setupisdone == FALSE && installflg == TRUE) {
		dosetup = TRUE;         /* No questions - just do it */
	} else if (setupisdone == FALSE && installflg == FALSE) {

prmsg="\n\
Your network does not appear to be set up.  Unless you intend to set it\n\
up by hand, you should set up the network files now.  Even if your\n\
system is not physically connected to a network, you need to set up the\n\
network files for some other commands to function properly.\n";

		fprintf(stderr,prmsg);
		if (yesno(YES, "\nDo you want to set up your system on a local area network now", hostname))
			dosetup = TRUE;
		else
			leave(1);
	}
	else if (setupisdone == TRUE && installflg == TRUE) {
			dosetup = TRUE;
	} /* else if setup is done and no install flag ==> no initial setup */

	if (dosetup == TRUE) {
		if (!yesno(YES,"Your system's name is \"%s\".  Is this correct", hostname)){
			
			fprintf(stderr, "\nPlease edit the /etc/rc.local file to contain the proper ");
			fprintf(stderr, "system name.\n");
			leave(1);
		}

prmsg="\n\
Your system can have one or more abbreviation names.  An abbreviation\n\
name reduces the number of keystrokes required for network commands. A\n\
common abbreviation name is the first letter of the system name.\n\n";

		printf(prmsg);
		do {

prmsg="\n\
Press the RETURN key if you do not want any abbreviation names for\n\
%s.  Otherwise, enter one or more abbreviation names, separated by\n\
blank spaces: ";

			if (prompt(machabbrev, 128, prmsg, hostname) == NULL)
			{
			    
			    i = yesno(YES,
			        "\nYou do not want any abbreviation names for %s.  Is this correct",
								hostname);
			} else {
			    i = yesno(YES,
				"\nThe abbreviation%s for %s %s %s.\nIs this correct",
					plural(machabbrev) ? "s" : "",
					hostname,
					plural(machabbrev) ?
						"are:" : "is:",
					quote(machabbrev) );
			}
		} while (i == NO);

	       /**************
		REMOVE DECNET GATEWAY SYUFF
		printf("\nA DECnet/ULTRIX-32 site can be used to act as ");
		printf("a gateway to\ngain access to DECnet.\n\n");
		do {
		   i=yesno(NO,"Do you have a site to act as a DECnet gateway");
		   if (i)
			j = yesno(YES, "Verify: There is a gateway site");
		   else
			j = yesno(YES, "Verify: No site to act as a gateway");
		} while (j == NO);
		if (i) {
			printf("\nInformation about the site name and the acc");
			printf("ount name to\nuse is kept in %s.\n", DGATEWAY);
			do {
				if (NULL == prompt(gateway, 32,
				   "What is the name of the gateway machine? "))
					continue;
				i = yesno(YES, "Verify: gateway is \"%s\"",
								gateway);
			} while (i == NO);
			do {
				if (prompt(gateacct, 32,
		    "What account name on \"%s\" should be used? [guest] ",
							gateway) == NULL)
					strcpy(gateacct, "guest");
				i = yesno(YES, "Verify: account is \"%s\"",
								gateacct);
			} while (i == NO);
			printf("\n***** Putting gateway information into ");
			printf("%s *****\n", DGATEWAY);
			if (!gateacct[0])
				strcpy(gateacct, "guest");
			fp = fopen(DGATEWAY, "w");
			if (fp == NULL) {
				perror(DGATEWAY);
				fprintf(stderr, "Cannot create %s\n", DGATEWAY);
			} else {
				fprintf(fp, "%s %s /etc/dgated\n", gateway,
						gateacct);
				fflush(fp);
				fclose(fp);
			}
			printf("\nIn order for the DECnet gateway software ");
			printf("to work, the \"%s\" ", gateacct);
			printf("account\nmust be set up on \"%s\"", gateway);
			printf(" so that \"%s\" on \"%s\" ",gateacct,hostname);
			printf("can\nrlogin as a trusted user.  In a little ");
			printf("bit you will be asked to add\nother hosts ");
			printf("to your %s file. Do not forget to ", HOSTS);
			printf("add \"%s\"\nat that time.\n", gateway);
		}
		******* END REMOVING DECNET GATEWAY SUFF *******/

		/* Get the network number for the network. */

		printf("\n");
		netnum = getnet(hostname);

prmsg="\n\
Subnetworks allow the systems on a given local area network to be on\n\
different network wires.  If your existing local area network is using\n\
subnet routing, you need to know how many bits of your host number\n\
are being reserved for specifying the subnetwork address.\n\
\n";

		printf(prmsg);

		subbits = 0;
		do {

prmsg="Are you setting up %s on a network that uses subnet routing";

			subbits = yesno(NO, prmsg, hostname);
			if (subbits) {
prmsg="\n\
You are setting up %s on a network that uses subnet routing.\n\
Is this correct";

			    i = yesno(YES, prmsg, hostname);
			} else {

prmsg="\n\
You will NOT set up %s on a network that uses subnet\n\
routing.  Is this correct";

			    i = yesno(YES, prmsg, hostname);
			}
		} while (!i);
			
		printf("\n");
		machnum = getmach(hostname, netnum);
		addr = inet_makeaddr(netnum, machnum);
		hp = gethostbyname(hostname);

		fp = edit(HOSTS);
		if (fp == NULL) {
			fprintf(stderr, "%s not set up\n", HOSTS);
		} else {
			printf("\n***** UPDATING %s WITH %s AND localhost *****\n",HOSTS,hostname);
			fprintf(fp, "g/^.*[ 	]localhost.*$/d\n");
			fprintf(fp, "0a\n");
			fprintf(fp, "127.0.0.1 localhost\n");
			fprintf(fp, ".\n");
			fprintf(fp, "w\n");
			if (hp) {
				fprintf(fp, "g/^.*[ 	]%s.*$/d\n", hostname);
				fprintf(fp, "w\n");
			}
			fprintf(fp, "a\n");
			fprintf(fp, "%s %s %s\n", inet_ntoa(addr),hostname,
					machabbrev);
			fprintf(fp, ".\n");
			fprintf(fp, "w\n");
			fprintf(fp, "q\n");
			fflush(fp);
			pclose(fp);
		}

		if (subbits) {
			if (IN_CLASSA(ntohl(addr.s_addr)))
				max = 21;
			else if (IN_CLASSB(ntohl(addr.s_addr)))
				max = 13;
			else
				max = 5;
			printf("\n");
			for (;;) {
				buf[0] = '\0';
				prompt(buf,512,"How many bits of the host number will be used for subnets? ");
				if (buf[0] == '\0')
					continue;
				else
					subbits = atoi(buf);

				if (subbits < 1 || subbits >= max) {
					printf("It must be a number between 1 & %d\n",max-1);
					continue;
				}
				if (yesno(YES,"\nYou are using %d bits of the host number for subnets.\nIs this correct", subbits))
					break;
			}
			netmask.s_addr = 0xffffffffL;
			netmask.s_addr <<= ((long)max + 3L - subbits);
		} else {
			subbits = 0;
			if (IN_CLASSA(ntohl(addr.s_addr)))
				netmask.s_addr = 0xff000000L;
			else if (IN_CLASSB(ntohl(addr.s_addr)))
				netmask.s_addr = 0xffff0000L;
			else
				netmask.s_addr = 0xffffff00L;
		}


/* Include top subbits of machine number when using subnets */
#define SUBMASK (subbits ? (0xffffffL<<(max+3-subbits)&machnum) : 0)

		netmask.s_addr = htonl(netmask.s_addr);
		taddr1 = inet_makeaddr(netnum, ~ntohl(netmask.s_addr) | SUBMASK);
		taddr2 = inet_makeaddr(netnum, 0L | SUBMASK);

prmsg="\n\
An Internet Protocol (IP) broadcast address is an address in which the\n\
bits of the host number are either all ones or all zeros.  You need to\n\
know what the other hosts on your existing local area network are using as\n\
their broadcast address.  If you are setting up your own network, use\n\
the default..\n\
\n\
WARNING! You must use the same broadcast address as all of the other\n\
hosts on your network.  This is very important!\n";
		
		printf(prmsg);

		sprintf(buf, "%s", inet_ntoa(taddr1));
		sprintf(buf2, "%s", inet_ntoa(taddr2));

prmsg="\n\
If you use all ones your IP broadcast address is: %s\n\
if you use all zeros your IP broadcast address is: %s\n\
\n\
Enter a \"1\" if you use all ones, or \"0\" if you use all zeros [1]: ";

		do {
			char resp[20];

			prompt(resp, 20, prmsg, buf, buf2);
			if (resp[0] == '\0')
				oneorzero = 1;
			else
				oneorzero = atoi(resp);

			if (oneorzero != 1 && oneorzero != 0) {
				fprintf(stderr,"\n\You must enter \"1\" or \"0\"\n");
				continue;
			}
			if (oneorzero == 1) 
				broadaddr = taddr1;
			else
				broadaddr = taddr2;
		} while (!yesno(YES,"\nYou are using all %s for your broadcast address.\nIs this correct", (oneorzero ? "ones" : "zeros")));

prmsg="\n\
Please specify the device name and unit number of your network\n\
interface.  This information is entered in the %s file so\n\
that the correct device is initialized when you bring the system to\n\
multiuser mode.  The device name consists of the network device\n\
as found in your system config file.  The following are some common\n\
network device names:\n\
\n\
        Device Name     Description\n\
        -----------     -----------\n\
          xna0          DEBNI - BI bus\n\
          xna4          DEMNA - XMI bus\n\
          ni0           DEBNT, DEBNA - BI bus\n\
          de0           DEUNA, DELUA - UNIBUS\n\
          qe0           DEQNA, DELQA - Q-Bus\n\
          ln0           DESVA, Busless LANCE network interface\n\
	  ne0		Second Generation Ethernet Controller interface\n\
	  fza0		DEFZA, TURBOchannel FDDI network interface\n\
          scs0          SCS network interface to CI\n\
\n";
		printf(prmsg, RC);

		/*
		 * Determine default network interface.
		 */
		if_default = get_interface(); 

		for(;;) {
askagain:				
			if (if_default)
				prompt(buf, 512, "What is the device name of your Network Interface [%s]? ", if_default);
			else
				prompt(buf, 512,"What is the device name of your Network Interface? ");

			if (buf[0] == '\0')
				if (if_default)
					strcpy(buf, if_default);
				else
					goto askagain;
				
			cp = buf;
			while (*cp == ' ' || *cp == '\t')
				cp++;
			xp = cp;
			while (*cp && *cp != ' ' && *cp != '\t' && *cp != '\n')
				cp++;
			*cp = '\0';
			if (yesno(YES, "\nYour Network interface is \"%s\".  Is this correct", xp))
				break;
		}
prmsg="\n\
The routed program can be invoked at boot time to manage the network\n\
routing tables.  Normally it is only run on systems with multiple\n\
interfaces.\n";
		printf(prmsg);
		do {
			i=(yesno(NO,"\nDo you want \/etc\/routed started each time %s is booted",hostname));
			if (i == NO) {
				i=yesno(YES,"Routed will not be started.  Is that correct");
				routed=FALSE;
			} else {
				i=yesno(YES,"Routed will be started.  Is that correct");
				routed=TRUE;
			}
		} while (i==NO);
		printf("\n***** UPDATING %s WITH network configuration information *****\n", RC);

		/*
		 * Update the /etc/rc.local file, WITH networking info.
		 */
		fp = edit(RC);
		if (fp == NULL) {
			fprintf(stderr, "%s is not set up for a network.\n", RC);
		} else {
			fprintf(fp, "/ifconfig.*`\\/bin\\/hostname`.*$/c\n");
			fprintf(fp, "/etc/ifconfig %s `/bin/hostname` ", xp);
			fprintf(fp, "broadcast %s ", inet_ntoa(broadaddr));
			fprintf(fp, "netmask %s\n", inet_ntoa(netmask));
			fprintf(fp, ".\n");
			if (routed) {
				fprintf(fp, "/routed/\n");
				fprintf(fp, ".,+2s/^#//\n");
			}
			fprintf(fp, "w\n");
			fprintf(fp, "q\n");
			fflush(fp);
			pclose(fp);
		}

		/*
		 * Set up /etc/networks here.  Supply a default network
		 * name and alias. If user enters a zero length string
		 * (<RETURN> only) use default.
		 */

		/*
		 * The network number that goes into /etc/networks is
		 * the logical OR of the real internet network number
		 * PLUS the subnet part (if any).  So /etc/networks
		 * actually contains names and numbers of subnets.  We
		 * derive the number by taking the broadcast address
		 * (which contains the subnet part) and masking off just
		 * the network-subnet piece.
		 */

#define	UC(b)	(((int)b)&0xff)

		if (IN_CLASSA(ntohl(addr.s_addr)))
			mask = IN_CLASSA_NSHIFT - subbits;
		else if (IN_CLASSB(ntohl(addr.s_addr)))
			mask = IN_CLASSB_NSHIFT - subbits;
		else
			mask = IN_CLASSC_NSHIFT;
		taddr1.s_addr = broadaddr.s_addr & netmask.s_addr;
		if (mask >= 24) {
			register char *p;
			p = (char *)&taddr1;
			sprintf(buf, "%d", UC(p[0]));
		} else if (mask >= 16) {
			register char *p;
			p = (char *)&taddr1;
			sprintf(buf, "%d.%d", UC(p[0]), UC(p[1]));
		} else if (mask >= 8) {
			register char *p;
			p = (char *)&taddr1;
			sprintf(buf, "%d.%d.%d", UC(p[0]), UC(p[1]), UC(p[2]));
		} else if (mask >= 0) {
			register char *p;
			p = (char *)&taddr1;
			sprintf(buf, "%d.%d.%d.%d", UC(p[0]), UC(p[1]), UC(p[2]), UC(p[3]));
		}
prmsg="\n\
Network numbers have names so you can refer to them by name rather than\n\
by number.  The network name is entered in the /etc/networks file.  If\n\
you are adding %s to an existing network, you should use the same\n\
names as the other hosts on the network use for the various network\n\
numbers.  If you are not adding %s to an existing network, you can\n\
name the network number whatever you want.  You can assign alias names\n\
for the network number.\n\
\n";

		printf(prmsg,hostname,hostname);

		do {
			if (NULL == prompt(netname, 128,
			"What is the name of network %s [ethernet]? ", buf))
				strcpy(netname, "ethernet");
			i = yesno(YES, "\nThe network name for %s is \"%s\".  Is this correct",
							hostname, netname);
		} while ( i == NO);
		if (!netname[0])
			strcpy(netname, "ethernet");
		printf("\n");
		do {
			if (prompt(netalias, 128,
			    "Enter any aliases for %s:  ", netname))
			{
				i = yesno(YES,
				    "\nThe alias%s for %s %s %s.\nIs this correct",
						plural(netalias) ? "es" : "",
						netname,
						plural(netalias) ? "are" : "is",
						quote(netalias));
			} else {
				i = yesno(YES,
				    "You do not have any aliases for %s.  Is this correct", netname);
			}
		} while (i == NO);
		printf("\n***** UPDATING %s WITH ", NETWORKS);
		printf("%s *****\n", netname);

		/* check for duplicate entry in /etc/networks and remove it */
		if ((np = getnetbyname(netname)) != NULL) {
			/* remove duplicate entry before adding same */
		       fprintf(stderr,"Removing duplicate entry, %s\n",netname);
			fp = edit(NETWORKS);
			if (fp == NULL) {
fprintf(stderr, "cannot remove duplicate entry (%s) \nfrom %s file.\n", netname, NETWORKS);
			} else {
				fprintf(fp, "/^%s/d\n",netname);
				fprintf(fp, "w\n");
				fprintf(fp, "q\n");
				fflush(fp);
				pclose(fp);
			}
		}
		/* add the netname to /etc/networks */
		fp = fopen(NETWORKS, "a");
		if (fp == NULL) {
			fprintf(stderr, "%s is not set up.\n", NETWORKS);
		} else {
			fprintf(fp, "%s %s%s%s\n", netname,
					buf,  /* buf is the network number figured above */
					netalias[0]? " " : "", netalias);
			/* add "loop" only if not there */
			if ((np = getnetbyname("loop")) == NULL)
				fprintf(fp, "loop 127 loopback\n");
			fflush(fp);
			fclose(fp);
		}
	}

	/*
	 * If user is just adding host names to the network, or at
	 * install time user selected to put the host on a network,
	 * (didn't exit above) then add host names to network.
	 * common to: first time setup and adding host names later.
	 */
	printf("\n***** ADD/CHANGE SYSTEMS IN %s *****\n", HOSTS);

prmsg="\n\
Enter the host name, abbreviations, network number, and host number for\n\
each host on the network.  Enter this information on separate lines\n\
when prompted.  This information is stored in the %s file.  When\n\
finished, press the RETURN key at the hostname prompt.\n";

	printf(prmsg, HOSTS);

	fp = edit(HOSTS);
	if (fp == NULL) {
		fprintf(stderr, "%s is not set up.\n", HOSTS);
	} else for (;;) {
		printf("\n");
		if (prompt(machname, 128,
		    "Enter the name of the host you want to add to %s: ", HOSTS) == NULL) {
			if (yesno(YES,"Are you finished adding hosts"))
				break;
			else
				continue;
		}
		if (!yesno(YES, "\nYou want to add host \"%s\".  Is this correct", machname))
			continue;
		hp = gethostbyname(machname);
		if (hp != NULL) {
			printf("\n%s already exists with address %s\n",
			      machname, inet_ntoa(*(long *)hp->h_addr));
			if (!yesno(NO, "Do you want to replace it"))
				continue;
			fprintf(fp, "g/%s[ 	]%s/d\n",
				inet_ntoa(*(long *)hp->h_addr), machname);
			fprintf(fp, "w\n");
			fflush(fp);
		}
		do {
			if (NULL == prompt(machabbrev, 128,
			    "\nEnter any abbreviations for \"%s\", separated by a blank space.\nIf you do not want any abbreviation names, press the RETURN key: ", machname))
			{
				i = yesno(YES,
					"\nYou do not want any abbreviation names for \"%s\".  Is this correct",
								machname);
			} else {
				i = yesno(YES,
				  "\nThe abbreviation%s for %s %s %s.\nIs this correct",
						plural(machabbrev) ? "s":"",
						machname,
						plural(machabbrev) ? "are":"is",
						quote(machabbrev));
			}
		} while (i == NO);
		netnum = getnet(machname);
		machnum = getmach(machname, netnum);
		addr = inet_makeaddr(netnum, machnum);
		hp = gethostbyaddr(&addr, sizeof(addr), AF_INET);
		if (hp != NULL) {
			printf("%s is already assigned to %s\n",
				inet_ntoa(addr), hp->h_name);
			if (!yesno(NO, "Remove %s from %s", hp->h_name, HOSTS))
				continue;
			fprintf(fp, "g/%s[ 	]%s/d\n",
				inet_ntoa(addr), hp->h_name);
			fprintf(fp, "w\n");
			fflush(fp);
		}
		fprintf(fp, "$a\n");
		fprintf(fp, "%s %s %s\n", inet_ntoa(addr),
					machname, machabbrev);
		fprintf(fp, ".\n");
		fprintf(fp, "w\n");
		fflush(fp);
	}
	if (fp) {
		fprintf(fp, "w\n");
		fprintf(fp, "q\n");
		fflush(fp);
		pclose(fp);
	}

	/*
	 * Set up /etc/hosts.equiv with trusted hosts here.
	 */
	if ((fp = edit(HOSTS_EQUIV)) == NULL) {
		fprintf(stderr, "%s is not set up\n", HOSTS_EQUIV);
	} else {

prmsg="\n\
Enter the names of trusted hosts.  Trusted hosts are systems you\n\
consider to be secure.  Be careful if you select trusted hosts.  Any\n\
users on a trusted host can log in to %s without password\n\
verification if they have a valid account on your system.  The names of\n\
the trusted hosts are stored in the %s file.  When you\n\
have finished entering the names of trusted hosts, press the RETURN key.\n\
";
		printf(prmsg, hostname, HOSTS_EQUIV);
		for (;;) {
			if (prompt(trustname, 128,
				   "\nEnter the name of a trusted host: ") == NULL)
			{
				if (yesno(YES,
				    "Have you finished entering the names of trusted hosts"))
					break;
				else
					continue;
			}
			if ((hp = gethostbyname(trustname)) == NULL) {
			    printf("\n\"%s\" is not in %s.\n", trustname, HOSTS);
			    printf("\"%s\" is not added to %s\n", trustname,
								HOSTS_EQUIV);
			} else {
				if (!yesno(YES,
				    "\n\"%s\" is a trusted host.  Is this correct",
								trustname))
					continue;
				fprintf(fp, "g/^%s$/d\n", trustname);
				fprintf(fp, "a\n%s\n", trustname);
				fprintf(fp, ".\n");
				fprintf(fp, "w\n");
				fflush(fp);
			}
		}
		fprintf(fp, "w\n");
		fprintf(fp, "q\n");
		fflush(fp);
		pclose(fp);
	}

	/*
	 * Set up /usr/hosts/*
	 */
	printf("\n***** SETTING UP %s DIRECTORY *****\n", UHOSTS);
	if (access(UHOSTS, 0) < 0) {
		mkdir(UHOSTS, 0755);
		chown(UHOSTS, 3, 3);
	}
	sethostent(1);
	while ((hp = gethostent()) != NULL) {
		if (strcmp(hp->h_name, "localhost") == 0)
			continue;
		sprintf(buf, "%s/%s", UHOSTS, hp->h_name);
		symlink("/usr/ucb/rsh", buf);
	}
	endhostent();

	/* Setup /usr/spool/rwho directory, OK if fails */
	mkdir(RWHODIR, 0755);
	chown(RWHODIR, 3, 3);
	leave(0);
}

/*
 * exit the program with status
 */
leave(status)
int status;
{
	if (status == 0)
		printf("\n***** NETWORK SETUP COMPLETE *****\n\n");
	exit(status);
}

char *
prompt(buf, len, str, a1, a2, a3, a4, a5)
char *str, *buf;
int len;
int a1, a2, a3, a4, a5;
{
	char mybuf[512];

	printf(str, a1, a2, a3, a4, a5);
	fflush(stdout);
	if (gets(mybuf) == NULL) {
		eof();
		return(NULL);
	}
	strncpy(buf, mybuf, len);
	buf[len-1] = '\0';
	if (buf[0] == '\0')
		return(NULL);
	return(buf);
}

yesno(t, s, a1, a2, a3, a4, a5)
char *s;
int t;
int a1, a2, a3, a4, a5;
{
	char buf[512];

	for (;;) {
		printf(s, a1, a2, a3, a4, a5);
		switch(t) {
		case 0:
			printf(" [no]? ");
			break;
		case 1:
			printf(" [yes]? ");
			break;
		default:
			printf(" (yes or no)? ");
			break;
		}
		fflush(stdout);
		buf[0] = '\0';
		if (gets(buf) == NULL) {
			eof();
			return(t);
		}
		if (buf[0] == 'y' || buf[0] == 'Y')
			return(1);
		if (buf[0] == 'n' || buf[0] == 'N')
			return(0);
		if ((t != 2) && (buf[0] == '\0' || buf[0] == '\n'))
			return(t);
	}
	/* To supress the silly warning about statement not reached, */
	/* we comment out the following:  */
	/* return(0); */
}

#define	MIN_A	0
#define	MAX_A	127
#define	MIN_B	(128L*256L)
#define	MAX_B	(191L*256L+255L)
#define	MIN_C	(192L*256L*256L)
#define	MAX_C	(223L*256L*256L+255L*256L+255L)
long
getnet(host)
char *host;
{
	struct in_addr addr, inet_num();
	long net, inet_lnaof(), inet_netof();
	char buf[512];
	static int msg = 0;
	char *cp;

	for(;;) {
	    if (msg) {
		printf ("\nEnter the network number for %s: ", host);
		fflush(stdout);
		if (gets(buf) == NULL) {
			eof();
			printf("\n");
			continue;
		}

		addr = inet_num(buf, 0);
		net = inet_netof(addr);
		cp = (char *)&addr.s_addr;
		if (addr.s_addr == 0L || inet_lnaof(addr) != 0L)
			addr.s_addr = -1L;
		else if (net > MIN_A && net < MAX_A)
			sprintf(buf, "%d", cp[0]&0xff);
		else if (net > MIN_B && net < MAX_B)
			sprintf(buf, "%d.%d", cp[0]&0xff, cp[1]&0xff);
		else if (net > MIN_C && net < MAX_C)
			sprintf(buf, "%d.%d.%d", cp[0]&0xff,
					cp[1]&0xff, cp[2]&0xff);
		else
			addr.s_addr = -1L;
		if (addr.s_addr != -1L) {
			if (!yesno(YES,"\nThe network number is %s.  Is this correct", buf))
				continue;
			return(net);
		} else {
			fprintf(stderr,"\n%s is an invalid network number.\n",buf);
		}
	    }
		msg = 1;

prmsg="\n\
For a Class A network, the network number is in the range 1 through\n\
126.  For a Class B network, the network number consists of two fields\n\
separated by periods.  The first field is in the range 128 through 191,\n\
and the second field is in the range 1 through 254.  For a Class C\n\
network, the network number consists of three fields separated by\n\
periods.  The first field is in the range 192 through 223, the second\n\
field is in the range 0 through 255, and the third field is in the\n\
range 1 through 254:\n\
\n\
   Class A:  1       through 126\n\
   Class B:  128.1   through 191.254\n\
   Class C:  192.0.1 through 223.255.254\n\
\n";
		printf(prmsg);
	}
}

struct in_addr
inet_num(cp, lhost)
	register char *cp;
	int lhost;
{
	long val, base;
	register int i, n;
	register char c;
	long parts[4], *pp = parts;

again:
	/*
	 * Collect number up to ``.''.
	 * Values are specified as for C:
	 * 0x=hex, 0=octal, other=decimal.
	 */
	val = 0; base = 10;
	if (*cp == '0')
		base = 8, cp++;
	if (*cp == 'x' || *cp == 'X')
		base = 16, cp++;
	while (c = *cp) {
		if (isdigit(c)) {
			val = (val * base) + (c - '0');
			cp++;
			continue;
		}
		if (base == 16 && isxdigit(c)) {
			val = (val << 4L) + (c + 10 - (islower(c) ? 'a' : 'A'));
			cp++;
			continue;
		}
		break;
	}
	if (*cp == '.') {
		/*
		 * Internet format:
		 *	a.b.c.d
		 *	a.b.c	(with c treated as 16-bits)
		 *	a.b	(with b treated as 24 bits)
		 */
		if (pp >= parts + 4) {
			val = -1;
			return (*(struct in_addr *)&val);
		}
		*pp++ = val, cp++;
		goto again;
	}
	/*
	 * Check for trailing characters.
	 */
	if (*cp && !isspace(*cp)) {
		val = -1;
		return (*(struct in_addr *)&val);
	}
	*pp++ = val;
	n = pp - parts;
	if (n > 4) {
		val = -1;
		return(*(struct in_addr *)&val);
	}
	val = 0;
	if (n == 1 && lhost)
		val = htonl(parts[0]);
	else {
		for (i = 0; i < n; i++) {
			if (parts[i] > 255 || parts[i] < 0) {
				val = -1;
				return(*(struct in_addr *)&val);
			}
			val = (val << 8L) + parts[i];
		}
		if (lhost == 0)
			for (; i < 4; i++)
				val <<= 8L;
		val = htonl(val);
	}
	return (*(struct in_addr *)&val);
}

long
getmach(n, net)
char *n;
long net;
{
	struct in_addr addr, addr2, inet_num();
	long lna, inet_lnaof();
	char buf[512];
	static msg = 0;
	char *cp;

	addr2 = inet_makeaddr(net, 0L);
	for(;;) {
	    if (msg) {
		printf("\nEnter the host number ");
		if (subbits)
			printf("(including the subnet number) ");
		printf("for %s: ", n);
		fflush(stdout);
		if (gets(buf) == NULL) {
			eof();
			printf("\n");
			continue;
		}

		addr = inet_num(buf, 1);
		if (addr.s_addr != -1L && addr.s_addr != 0L) {
			lna = inet_lnaof(addr);

/*@			cp = &addr.s_addr;	JSD */
/*@			printf("addr = %d.%d.%d.%d\n", cp[0]&0xff, cp[1]&0xff, cp[2]&0xff, cp[3]&0xff);		JSD */

			if (lna && ((IN_CLASSA(ntohl(addr2.s_addr)) &&
						(lna < 256L*256L*256L - 1)) ||
				    (IN_CLASSB(ntohl(addr2.s_addr)) &&
						(lna < 256L*256L - 1)) ||
				    (lna < 256 - 1)) )
			{
				cp = (char *)&addr.s_addr;
				if (IN_CLASSA(ntohl(addr2.s_addr)))
					sprintf(buf, "%d.%d.%d", cp[1]&0xff,
							cp[2]&0xff, cp[3]&0xff);
				else if (IN_CLASSB(ntohl(addr2.s_addr)))
					sprintf(buf, "%d.%d", cp[2]&0xff,
								cp[3]&0xff);
				else
					sprintf(buf, "%d", cp[3]&0xff);
				if (!yesno(YES,"\nThe Host number is %s.  Is this correct",buf))
					continue;
				return(lna);
			}
		}
	    }
	    msg = 1;
	    if (net < 128) {
		printf("You are setting up %s on a Class A network.\n", hostname);
		printf("\nValid Class A host numbers ");
		if (subbits)
			printf("(including the subnet number)\n");

prmsg="\
consist of three fields separated by\n\
periods.  The first two fields are in the range 0 through 255, and\n\
the third field is in the range 1 through 254:\n\
\n\
   0.0.1 through 255.255.254\n\
\n\
An alternate representation for the three bytes of information specifying\n\
the host number is one field in the range 1 through 16777214:\n\
\n\
   1 through 16777214\n\
\n";
			
		printf(prmsg);
		
	    } else if (net < 65536L) {
		printf("You are setting up %s on a Class B network.\n", hostname);
		printf("\nValid Class B host numbers ");
		if (subbits)
			printf("(including the subnet number)\n");

prmsg="\
consist of two fields separated by a\n\
period.  The first field is in the range 0 through 255, and the second\n\
field is in the range 1 through 254:\n\
\n\
   0.1 through 255.254\n\
\n\
An alternate representation for the two bytes of information specifying\n\
the host number is one field in the range 1 through 65534:\n\
\n\
   1 through 65534\n\
\n";
		printf(prmsg);

	    } else {
		printf("You are setting up %s on a Class C network.\n", hostname);
		printf("Valid Class C host numbers (including the subnet number) are in the range 1 through 254.\n");
	    }
	}
}

FILE*
edit(file)
char *file;
{
	FILE	*fp;
	char	buf[64];

	/*
	 * close stdout, popen the file for editing, restore
	 * stdout, and return the stream pointer.
	 */
	close(1);
	sprintf(buf, "/bin/ed - %s", file);
	fp = popen(buf, "w");
	dup2(3, 1);
	if (fp == NULL)
		fprintf(stderr, "Cannot open %s for editing.\n", file);
	return(fp);
}

/* quote() -  make string like: abc def ghi, become: "abc", "def", "ghi". */

#include <ctype.h>
#define iswhite(s) isspace(s)

char *
quote(s) 
	char *s; {

	int state, oldstate, white;
	static char buff[256];
	char *s2 = buff;

	state = oldstate = white = iswhite(' ');  /* prime it */
	while (*s) {
		if ((state = iswhite(*s)) != oldstate) {
			*s2++ = '\"';
			if (state == white)
				*s2++ = ',';
		}
		*s2++ = *s++;
		oldstate = state;
	}

	/* if last char was not a space put ending quote*/
	if (state == 0)       
		*s2++ = '\"';
	*s2 = '\0';

	/* strip off last comma */
	while (--s2 > buff) 	
		if (!iswhite(*s2)) {
			if (*s2 == ',')
				*s2 = ' ';
			break;
		}
	return(buff);
}

/* 
 *		e o f
 *
 * Unexpected EOF, try to recover.
 */
eof() {
	if (isatty(0) && (freopen(ttyname(0),"r",stdin) != NULL)) {
		printf("\n");
		return(0);
	}
	fprintf(stderr,"\nUnexpected EOF.  Exiting.\n");
	leave(1);
}

/*
 *
 *		g e t _ i n t e r f a c e
 *
 * Determine the network interface. Look in
 * the conf file for a possible choice.  Choices are
 * listed in (char *if_table[]).
 *
 */
char *
get_interface()
{
	FILE *cf;
	char config[512 + 10];
	register char *cp;
	register int i;
	char *ni = 0;

	sprintf(config, "%s/%s", CONFDIR, hostname);
	cp = config + sizeof(CONFDIR);
	upcase(cp);
	cf = fopen(config, "r");
	if (cf == NULL)
		return(0);
	/*
	 * Build list of hardware devices as found in config file.
	 */
	parse_config(cf);
	/*
	 * Find match.
	 */
	for (i=0; cp=if_table[i]; i++)
	{
		register int len = strlen(cp);
		register struct device *d = (struct device *)device.dq_forw;

		while (d != (struct device *)&device)
		{
			if (strncmp(d->dv_name, cp, len) == 0)
			{
				ni = d->dv_name;
				break;
			}
			d = (struct device *)d->dv_queue.dq_forw;
		}
		if (ni)
			break;
	}
	return (ni);
}

parse_config(cf)
FILE *cf;
{
	char buf[256];
	char type[256];
	char name[256];
	struct device *d;

	while (fgets(buf, 255, cf) != NULL)
	{
		if (sscanf(buf, " %s %s ", type, name) == 2)
		{
			if (strcmp(type, "device") != 0)
				continue;
			if (strlen(name) > MAXDEVNAMLEN)
				continue;
			/*
			 * Link it in.
			 */
			d = (struct device *)emalloc(sizeof(struct device));
			strcpy(d->dv_name, name);
			insque(d, device.dq_back);
		}
	}
	/***************
	for (d = device.dq_forw; d != &device ; d = d->dv_queue.dq_forw)
		printf("qelem: %s\n", d->dv_name);
	****************/
}

upcase(cp)
char *cp;
{
	while(*cp)
	{
		if (islower(*cp))
			*cp -= 'a' - 'A';
		cp++ ;
	}
}

char *
emalloc(size)
unsigned long size;
{
	char *p;
	p = (char *)malloc(size);
	if (p == 0)
	{
		fprintf(stderr, "Out of memory.\n");
		exit(2);
	}
	return(p);
}
