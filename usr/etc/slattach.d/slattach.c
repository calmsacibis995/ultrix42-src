#ifndef lint
static char	*sccsid="@(#)slattach.c	4.3	(ULTRIX)	2/1/91";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1988 by				*
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
#ifndef lint
static char sccsid[] = "slattach.c	4.1 (Berkeley) 2/17/86";
#endif
*/

/*
 * TODO:
 *	-Redial on failure (if option set to do so).
 */
#include <stdio.h>
#include <sys/param.h>
#include <sgtty.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <netdb.h>
#include <fcntl.h>
#include <syslog.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <setjmp.h>
#include <ctype.h>

#define DEFAULT_BAUD	2400
#define MAXHOSTLEN 255
#define SLHOSTS "/etc/sliphosts"

int	slipdisc = SLPDISC;

char	devname[32];
char	ifconfigbuf[255];
char 	argbuf[256];
char 	gbuf[1024], respbuf[1024];
int fd = 0;

/************************************************************************
 *			Modification History				*
 *
 * 09/25/89	R. Bhanukitsiri
 *		Make syslog log errno.
 *
 * # comment */
/* 9+ allow destination gateway mask speed device modemtype phone# login */
/* 8+ allow destination gateway mask speed device hw login */
/* 6 destination gateway mask speed device */
/*									*
 ************************************************************************/


#define ALIAS		0
#define DESTINATION 	1
#define GATEWAY		2
#define MASK		3
#define SPEED		4
#define DEVICE		5
#define TYPE		6
#define PHONE		7
#define LOGIN_INFO	8

#define MAXARGS 40
char *arptr[MAXARGS];
char *modem_type, *phone, *master; 
extern int errno;

/* Parity control during login procedure */
#define P_ZERO  0
#define P_ONE   1
#define P_EVEN  2
#define P_ODD   3


#ifndef IFF_D1
/*
 * These defines for device specific interface flags will eventually
 * get added to net/if.h.  Until then ...
 */
#define IFF_D1	0x8000
#define IFF_D2	0x4000
#define IFF_D3	0x2000
#endif

int set_flags;		/* interface flag bits to set */
int clear_flags;	/* interface flag bits to clear */

huphandler()
{
	int pid;
	syslog(LOG_ERR, "HUP - shutdown");
	exit(0);
}
		
int sldebug = 0;
	
main(argc, argv)
	int argc;
	char *argv[];
{

	register FILE *fp;
	int login = 0;
	struct sgttyb sgtty;
	int n, pid=0, typeofslip=0, incoming = 0;
	int unit = 0;
	int	speed = 0;

	if ((argc > 2) && (argv[1][0] != '-' && argv[1][0] != '+')) {
	  fprintf(stderr,
		  "slattach: usage slattach [{+|-}{c|e|i} ...] [system]\n");
	  exit(1);
	}
 
	while (argc > 2 && (argv[1][0] == '-' || argv[1][0] == '+')) {
		switch (argv[1][1]) {
		case 'c':	/* enable or disable hdr compression */
			if (argv[1][0] == '-')
				clear_flags |= IFF_D1;
			else
				set_flags |= IFF_D1;
			break;
		case 'e':	/* enable or disable hdr.comp. `auto-enable' */
			if (argv[1][0] == '-')
				clear_flags |= IFF_D2;
			else
				set_flags |= IFF_D2;
			break;
		case 'i':	/* enable or disable icmp discard */
			if (argv[1][0] == '-')
				clear_flags |= IFF_D3;
			else
				set_flags |= IFF_D3;
			break;
		default:
			fprintf(stderr,"%s: unknown flag `%s'\n", argv[0],
				argv[1]);
			exit(1);
		}
		--argc;
		++argv;
	}

	/* Become root */

	setuid(geteuid());

	if(argc == 1)
		typeofslip = setup(getlogin());
	else
		typeofslip = setup(argv[1]);

	if(!typeofslip) {
		fprintf(stderr, "slattach: Unknown host\n");
		exit(1);
	}
	openlog("slattach", LOG_ERR);
	if(strcmp(arptr[SPEED], "any") != 0) {
		speed = findspeed(arptr[SPEED] ? atoi(arptr[SPEED]) : DEFAULT_BAUD);
		modem_type = arptr[TYPE];
		phone = arptr[PHONE];
		/* hw means no phone # */
		if(strcmp(modem_type, "hw") == 0)
			login = PHONE;
		else
			login = LOGIN_INFO;
	} else {
		incoming++;
	}
	master = arptr[DESTINATION];

	if(sldebug)
		syslog(LOG_ERR,"type %d %d\n", typeofslip, login);

	if (!incoming && speed == 0) {
		fprintf(stderr, "unknown speed %s", argv[2]);
		exit(1);
	}
	/*
	if(!incoming) {
		for (n = 0; n < 20; n++)
			(void) close(n);
		(void) open("/", 0);
		(void) dup2(0, 1);
		(void) dup2(0, 2);
		n = open("/dev/tty", 2);
		if (n >= 0) {
			ioctl(n, TIOCNOTTY, (char *)0);
			(void) close(n);
		}
	}
	*/
	if(modem_type != 0 && strcmp(modem_type, "hw") != 0) {
		if((fd = open(arptr[DEVICE], O_RDWR|O_NDELAY)) < 0) {
			syslog(LOG_ERR, arptr[DEVICE]);
			exit(1);
		}
		if(ioctl(fd, TIOCSINUSE) < 0) {
			syslog(LOG_ERR,"Shared line in use: %m");
			exit(1);
		}	
	} else {
		if ((fd = open(arptr[DEVICE], O_RDWR|O_NDELAY)) < 0) {
			syslog(LOG_ERR,arptr[DEVICE]);
			exit(1);
		}
	}
	if(incoming) { /* leave speed alone */
		if (ioctl(fd, TIOCGETP, &sgtty) < 0) {
			syslog(LOG_ERR,"ioctl(TIOCGETP): %m");
			exit(1);
		}
		speed = sgtty.sg_ospeed;
		syslog(LOG_ERR,"speeds %d %d\n", sgtty.sg_ospeed, sgtty.sg_ispeed);
	}

	/* Enable hangup handler */
	(void) signal(SIGHUP, huphandler);
	(void) signal(SIGINT, huphandler);

	sgtty.sg_flags = RAW;
	sgtty.sg_flags &= ~ANYP; /* NONE */
	sgtty.sg_ispeed = sgtty.sg_ospeed = speed;
	if (ioctl(fd, TIOCSETP, &sgtty) < 0) {
		syslog(LOG_ERR,"ioctl(TIOCSETP): %m");
		exit(1);
	}
	if(sldebug)
		syslog(LOG_ERR, "modem_type %s %d\n", modem_type, login);
	if(modem_type != NULL) {
		if(strcmp(modem_type, "hw") != 0 && phone && agetent(gbuf, modem_type) != 0) {
			char tmpbuf[1024];
			gen_setup(gbuf, fd);
			if(phone) {
				if(!gen_dialer(phone, "sl_network")) {
					syslog(LOG_ERR,"Can't get remote system");
					exit(1);
				}
			if(login) 
				dologin(login);
			}
		} else if(strcmp(modem_type, "hw") == 0 && login) {
			/* These lines fool generic dialing routines */
			extern int FDD, stupidi, debugn, lsleep;
			int temp = 0;
			FDD = fd;
			stupidi = debugn = lsleep = 0;
	/* Ignore modem signals in case line is configured incorrectly */
			ioctl(fd, TIOCNMODEM, &temp);

			dologin(login);
		}
	}
	sgtty.sg_flags = RAW;
	sgtty.sg_flags &= ~ANYP; /* NONE */
	sgtty.sg_ispeed = sgtty.sg_ospeed = speed;
	if (ioctl(fd, TIOCSETP, &sgtty) < 0) {
		syslog(LOG_ERR,"ioctl(TIOCSETP): %m");
		exit(1);
	}
	if (ioctl(fd, TIOCSETD, &slipdisc) < 0) {
		syslog(LOG_ERR,"ioctl(TIOCSETD): %m");
		exit(99);
	}
	if(ioctl(fd, TIOCGETD, &unit) < 0) {
		syslog(LOG_ERR,"ioctl(TIOCGETD): %m");
		exit(1);
	}
	if(master != NULL) { /* Setting up a comm link */
		sprintf(ifconfigbuf,"/etc/ifconfig sl%d %s %s netmask %s", unit, arptr[GATEWAY], master, arptr[MASK]);
		syslog(LOG_ERR, "create interface: %s", ifconfigbuf);
	}

	if ((pid = fork())>0) {
		system(ifconfigbuf);
		exit(0);
	}
	wait(pid);

/* If we are receiving a call then allow compression.  It may then be
 * enabled by the machine calling us.
 */

	if (incoming) {
	  clear_flags = 0;
	  set_flags = IFF_D2;
	}
	
	if (set_flags || clear_flags) {
	  int s;
	  struct	ifreq ifr;

	  sprintf(ifr.ifr_name, "sl%d", unit);

	  s = socket(AF_INET, SOCK_DGRAM, 0);
	  if (s < 0) {
	    perror("ifattach: socket");
	    syslog(LOG_INFO, "socket: %m");
	    cleandeath(1);
	  }
	  
	  if (ioctl(s, SIOCGIFFLAGS, (caddr_t)&ifr) < 0) {
	    perror("ioctl (SIOCGIFFLAGS)");
	    syslog(LOG_INFO, "SIOCGIFFLAGS: %m");
	    cleandeath(1);
	  }
	  ifr.ifr_flags &=~ clear_flags;
	  ifr.ifr_flags |=  set_flags;
	  if (ioctl(s, SIOCSIFFLAGS, (caddr_t)&ifr) < 0) {
	    perror("ioctl (SIOCSIFFLAGS)");
	    syslog(LOG_INFO, "SIOCSIFFLAGS: %m");
	    cleandeath(1);
	  }
	}
	
	for(;;)
		sigpause(0);
}

struct sg_spds {
	int sp_val, sp_name;
}       spds[] = {
#ifdef B50
	{ 50, B50 },
#endif
#ifdef B75
	{ 75, B75 },
#endif
#ifdef B110
	{ 110, B110 },
#endif
#ifdef B150
	{ 150, B150 },
#endif
#ifdef B200
	{ 200, B200 },
#endif
#ifdef B300
	{ 300, B300 },
#endif
#ifdef B600
	{ 600, B600 },
#endif
#ifdef B1200
	{ 1200, B1200 },
#endif
#ifdef B1800
	{ 1800, B1800 },
#endif
#ifdef B2000
	{ 2000, B2000 },
#endif
#ifdef B2400
	{ 2400, B2400 },
#endif
#ifdef B3600
	{ 3600, B3600 },
#endif
#ifdef B4800
	{ 4800, B4800 },
#endif
#ifdef B7200
	{ 7200, B7200 },
#endif
#ifdef B9600
	{ 9600, B9600 },
#endif
#ifdef EXTA
	{ 19200, EXTA },
#endif
#ifdef EXTB
	{ 38400, EXTB },
#endif
	{ 0, 0 }
};

findspeed(speed)
	register int speed;
{
	register struct sg_spds *sp;

	sp = spds;
	while (sp->sp_val && sp->sp_val != speed) {
		sp++;
	}
	return (sp->sp_name);
}

setup(host)
char *host;
{
	FILE *fp;
	int c;

	if((fp = fopen(SLHOSTS, "r")) == 0)
		return(1);
	while(fgets(argbuf, 256, fp) != NULL) {
		c = getargs(argbuf, arptr);
		if(strcmp(host, arptr[ALIAS]) == 0) {
			fclose(fp);
			return(c);
		}	
	}
	fclose(fp);
	return(0);
}

/*
 *	The following code is adapted from uucico to present a
 *	somewhat smarter interface to dialing slip hosts
 */
getargs(s, arps)
char *s, *arps[];
{
	int i;

	for(i=0; i<MAXARGS; i++)
		arps[i] = NULL;
	i = 0;
	while (1) {
		arps[i] = NULL;
		while (*s == ' ' || *s == '\t')
			*s++ = '\0';
		if (*s == '\n')
			*s = '\0';
		if (*s == '\0')
			break;
		arps[i++] = s++;
		while (*s != '\0' && *s != ' '
			&& *s != '\t' && *s != '\n')
				s++;
	}
	return(i);
}

dologin(loginind)
{
	register char *want, *altern;
        extern char *index();
        int k, ok;
	char **login = &arptr[loginind];

	while(*login) {
		if(sldebug)
			syslog(LOG_ERR, "login: %x %s\n", login, *login);
		want = *login;
		ok = 1;
		while(ok != 0) {
			altern = index(want, '-');
			if(altern != NULL)
				*altern++ = '\0';
			ok = expect(want);
			if(ok == 0)
				break;
			if(altern == NULL)
				return;
			want = index(altern, '-');
			if(want != NULL)
				*want++ = '\0';
			sendthem(altern);
		}
		login++;
		sleep(2);
		if(sldebug)
			syslog(LOG_ERR, "login: %x %s\n", login, *login);
		sendthem(*login);
		login++;
	}	
}

jmp_buf Sjbuf;

alarmtr()
{
	signal(SIGALRM, alarmtr);
	longjmp(Sjbuf, 1);
}

#define MR 4096
expect(str)
char *str;
{
	static char rdvec[MR];
	register char *rp = rdvec;
	int kr;
	char nextch;

	if (strcmp(str, "\"\"") == 0)
		return(0);
	*rp = 0;
	if (setjmp(Sjbuf)) {
		return(-1);
	}
	signal(SIGALRM, alarmtr);
	alarm(10);
	if(sldebug)
		syslog(LOG_ERR,"expect %s\n", str);
	while (notin(str, rdvec)) {
		kr = read(fd, &nextch, 1);
		if (kr <= 0) {
			alarm(0);
			return(-1);
		}
		{
		int c;
		c = nextch & 0177;
		}
		if ((*rp = nextch & 0177) != '\0')
			rp++;
		if (rp >= rdvec + MR) {
			alarm(0);
			return(-1);
		}
		*rp = '\0';
	}
	alarm(0);
	return(0);
}

char par_tab[128];

sendthem(str)
char *str;
{
	register char *strptr;
	int i=0;
	int n, cr = 1;
	static int p_init = 0;

	if(!p_init) {
		p_init++;
		bld_partab(P_EVEN);
	}
	if(prefix("BREAK", str)) {
		sscanf(&str[5], "%1d", &i);
		if(i<=0 || i>10)
			i=3;
		genbrk(i);
		return;
	}

	if(prefix("PAUSE", str)) {
		sscanf(&str[5], "%1d", &i);
		if(i<=0 || i>10)
			i=3;
		sleep((unsigned)i);
		return;
	}

	if(strcmp(str, "EOT") == 0) {
		p_chwrite('\04');
		return;
	}

	/* Send a '\n' */
	if (strcmp(str, "LF") == 0)
		str = "\\n\\c";

	/* Send a '\r' */
	if (strcmp(str, "CR") == 0)
		str = "\\r\\c";

	/* Set parity as needed */
	if (strcmp(str, "P_ZERO") == 0) {
		bld_partab(P_ZERO);
		return;
	}
	if (strcmp(str, "P_ONE") == 0) {
		bld_partab(P_ONE);
		return;
	}
	if (strcmp(str, "P_EVEN") == 0) {
		bld_partab(P_EVEN);
		return;
	}
	if (strcmp(str, "P_ODD") == 0) {
		bld_partab(P_ODD);
		return;
	}

	/* If "", just send '\r' */
	if (strcmp(str, "\"\"") != 0)
	for (strptr = str; *strptr; strptr++) {
		if (*strptr == '\\') switch(*++strptr) {
		case 's':
			*strptr = ' ';
			break;
		case 'd':
			sleep(1);
			continue;
		case 'r':
			*strptr = '\r';
			break;
		case 'b':
			if (isdigit(*(strptr+1))) {
				i = (*++strptr - '0');
				if (i <= 0 || i > 10)
					i = 3;
			} else
				i = 3;
			/* send break */
			genbrk(i);
			continue;
		case 'c':
			if (*(strptr+1) == '\0') {
				cr = 0;
				continue;
			}
			continue;
		default:
			if (isdigit(strptr[1])) {
				i = 0;
				n = 0;
				while (isdigit(strptr[1]) && ++n <= 3)
					i = i*8 + (*++strptr - '0');
				p_chwrite(i);
				continue;
			}
			strptr--;
		}
		p_chwrite(*strptr);
	}

	if (cr)
		p_chwrite('\r');

}

p_chwrite(c)
int c;
{
	char t[2];
	t[0] = par_tab[c&0177];
	t[1] = '\0';
	write(fd, t, 1);
}
		
/*
 * generate parity table for use by p_chwrite.
 */
bld_partab(type)
int type;
{
	register int i, j, n;

	for (i = 0; i < sizeof(par_tab); i++) {
		n = 0;
		for (j = i&(sizeof(par_tab)-1); j; j = (j-1)&j)
			n++;
		par_tab[i] = i;
		if (type == P_ONE
		 || (type == P_EVEN && (n&01) != 0)
		 || (type == P_ODD && (n&01) == 0))
			par_tab[i] |= sizeof(par_tab);
	}
}

notin(sh, lg)
register char *sh, *lg;
{
        while (*lg != '\0') {
                if (wprefix(sh, lg))
                        return(0);
                else
			lg++;
        }
        return(1);
}

prefix(s1, s2)
register char *s1, *s2;
{
	register char c;
	
	while((c = *s1++) == *s2++)
		if (c == '\0')
			return(1);
	return(c == '\0');
}

wprefix(s1, s2)
register char *s1, *s2;
{
	register char c;
	while ((c = *s1++) != '\0')
		if (*s2 == '\0'  ||  (c != *s2++  &&  c != '?'))
		return(0);
	return(1);
}

#define STBNULL (struct sgttyb *) 0

genbrk(bnulls)
register int bnulls;
{
	register int ret;
#ifdef  TIOCSBRK
	ret = ioctl(fd, TIOCSBRK, STBNULL);
#ifdef  TIOCCBRK
	ret = write(fd, "\0\0\0\0\0\0\0\0\0\0\0\0", bnulls);
	sleep(1);
	ret = ioctl(fd, TIOCCBRK, STBNULL);
#endif
#endif
}

cleandeath(error)
{
#ifdef notdef
        if (ioctl(fd, TIOCSETD, &olddisc) < 0) {
                fputs(dev, stderr);
                perror(": restore discipline:");
                syslog(LOG_INFO, "%s: restore discipline: %m", dev);
        } else if (ioctl(fd, TIOCSETP, &otty) < 0) {
                fputs(dev, stderr);
                perror(": restore parameters:");
                syslog(LOG_INFO, "%s: restore parameters: %m", dev);
        }
#endif
        exit(error);
}
