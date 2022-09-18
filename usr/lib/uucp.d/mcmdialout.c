#ifndef lint
static char sccsid[] = "@(#)mcmdialout.c	4.1 (decvax!larry) 7/2/90";
#endif

/* Warning: this dialout() routine has a non-standard argument list */
/*
 * From: ikonas!mcm
 * Here is the dialout.c I use.  It works, but it could
 * be smarter.  Note that the input parameters are different
 * than the dialout routine supplied with [old versions of] UUCP.
 * 
 * I had lots of problems with the modem sending
 * result codes since I am using the same modem for both incomming and
 * outgoing calls.  I'd occasionally miss the result code (getty would
 * grab it), and the connect would fail.  Worse yet, the getty would
 * think the result code was a user name, and send garbage to it while
 * it was in the command state.  I turned of ALL result codes, and hope
 * for the best.  99% of the time the modem is in the correct state.
 * Occassionally it doesn't connect, or the phone was busy, etc., and
 * uucico sits there trying to log in.  It eventually times out, calling
 * dialend() in the process, so it resets itself for the next attempt.
 * 
 * For the first few weeks we had the modem, we controlled it with the L.sys
 * information.  It was a long, convoluted sequence, but it worked.  The
 * reliability with this routine is a quite a bit higher.
 */

/*
 * New dialout routine to work with Hayes' SMART MODEM
 * 13-JUL-82, Mike Mitchell
 * Modified 23-MAR-83 to work with Tom Truscott's (rti!trt)
 * version of UUCP	(ikonas!mcm)
 */
/* The modem should be set to NOT send any result codes to
 * the system (switch 3 up, 4 down). This end will figure out
 *  what is wrong.
 * To hang up by dropping DTR (recommended), #define USEDTR below.
 * The DTR line must be connected, i.e. switch 1 up.  This code
 * hangs up the phone by using DTR.
 * If USEDTR is not defined the "+++" SmartModem hangup sequence is used.
 */


/* decvax!larry -  we dont use this routine */


#define	USEDTR

#include <sys/types.h>
#include <signal.h>
#include "uucp.h"

#define F_NAME	0
#define F_TIME	1
#define F_LINE	2
#define F_CLASS	3
#define F_PHONE	4
#define F_LOGIN	5
#define MAXPH 60

struct Devices {
	char D_type[10];
	char D_line[10];
	char D_calldev[10];
	char D_class[10];
	int D_speed;
	};

static int Dnf = -1;
char dnname[20];

dialout(telno, flds)
char *telno, *flds[];
{
	char dcname[20], phone[MAXPH+10], c = 0;
	struct Devices dev;
	int sigalrm(), (*sal)();
#ifdef	SYSIII
	struct termio ttbuf;
#endif
	int status;
	unsigned timelim;
	FILE *dfp;

	dfp = fopen(DEVFILE, "r");
	ASSERT(dfp != NULL, "CAN'T OPEN", DEVFILE, 0);
	while ((status = rddev(dfp, &dev)) != FAIL) {
		if (strcmp(flds[F_CLASS], dev.D_class) != SAME)
			continue;
		if (strcmp(flds[F_LINE], dev.D_type) != SAME)
			continue;
		if (mlock(dev.D_line) == FAIL)
			continue;
		sprintf(dnname, "/dev/%s", dev.D_calldev);
		if ((Dnf = open(dnname, 2)) > 0)
			break;
		delock(dev.D_line);
	}
	fclose(dfp);
	if (status == FAIL) {
		logent("DEVICE", "NO");
		DEBUG(4, "NO DEVICE %s", "");
		return(CF_NODEV);
	}

	sprintf(dcname, "/dev/%s", dev.D_line);
	fixline(Dnf, dev.D_speed);
	DEBUG(4, "dc - %s, ", dcname);
	DEBUG(4, "acu - %s\n", dnname);

/*
 * if I wanted to get cute, I'd create a table with ACU types and
 * the addresses of routines to call to dial the phone.  I'd then
 * check the ACU type with the dev.D_type information.  That would
 * give me a subroutine to call to dial the phone.  I only have
 * a Smart Modem, so I don't need to get fancy.
 */
	sprintf(phone, "\rATDT%s\r", telno);

	write(Dnf, phone, strlen(phone));
	sal = signal(SIGALRM, sigalrm);

/* calculate delay time for the other system to answer the phone.
 * Default is 15 seconds, add 2 seconds for each comma in the phone
 * number.
 */
	timelim = 15;
	while(*telno)
		if (*telno++ == ',')
			timelim += 2;
	alarm(timelim);
	read(Dnf, &c, 1);
	alarm(0);
	signal(SIGALRM, sal);
/*
 * we dup the descriptor because it gets closed before the dialend()
 * routine is called.
 */
	return(dup(Dnf));
	}

#include <sgtty.h>
dialend()
{
#ifndef	USEDTR
	int sigalrm(), (*sal)();
	char chr;
#endif
	struct sgttyb work;

	if (Dnf > -1) {
		DEBUG(4, "Hanging up fd = %d\n", Dnf);
#ifndef	USEDTR
/*
 * The following code attempts to reset the modem.  I found
 * it is easier (and more reliable) to hang up the phone by
 * dropping DTR.  This code also depends upon the result codes
 * being sent from the modem.
 */
		sal = signal(SIGALRM, sigalrm);
		alarm(3);
		while(read(Dnf, &chr, 1) == 1)
			alarm(3);
		write(Dnf, "+++", 3);
		alarm(3);
		while(read(Dnf, &chr, 1) == 1)
			alarm(3);
		write(Dnf, "\rATZ\r", 5);
		alarm(3);
		while(read(Dnf, &chr, 1) == 1)
			alarm(3);
		alarm(0);
		signal(SIGALRM, sal);
#else
/*
 * code to drop DTR -- change to 0 baud then back to default.
 */
		gtty(Dnf, &work);
		work.sg_ispeed = B0;
		work.sg_ospeed = B0;
		stty(Dnf, &work);
		work.sg_ispeed = B1200;
		work.sg_ospeed = B1200;
		stty(Dnf, &work);
		sleep(2);

/*
 * now raise DTR -- close the device & open it again.
 */
		close(Dnf);
		sleep(2);
		Dnf = open(dnname, 2);
#endif
/*
 * Since we have a getty sleeping on this line, when it wakes up it sends
 * all kinds of garbage to the modem.  Unfortunatly, the modem likes to
 * execute the previous command when it sees the garbage.  The previous
 * command was to dial the phone, so let's make the last command reset
 * the modem.
 */
		sleep(2);
		write(Dnf, "\rATZ\r", 5);

		close(Dnf);
		Dnf = -1;
		}
	}
sigalrm()
{
	signal(SIGALRM, sigalrm);
}
