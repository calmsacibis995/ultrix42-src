/*
 * Xttylib.c:	Creates a new instance of a terminal window
 *
 * Derived from:
 *	"$Header: Xttylib.c,v 10.7 86/04/22 15:18:10 jg Rel $"
 */
#ifndef lint
static char *sccsid = "@(#)Xttylib.c	4.1	Ultrix	7/3/90";
#endif lint

#include <stdio.h>
#include <signal.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sgtty.h>
#include "Xtty.h"

TTYWindow *CreateTTYWindow()
{
	TTYWindow *t;

	if ((t = (TTYWindow *) malloc(sizeof(TTYWindow))) ==
		NULL) return NULL;

	if (Start_slave_xterm(t) == 0) {
	    free((char *)t);
	    fprintf(stderr, "Couldn't start slave xterm\n");
	    return NULL;
	}

	return t;
}

int ttyMasterPty;
int keepMasterOpen = 0;
char *xdbpath;

int Start_slave_xterm(t)
	TTYWindow *t;
{
#define BUFSIZE 20

	char ttyName[BUFSIZE];
	char Sbuf[BUFSIZE], inputBuffer[BUFSIZE];
	int bytesRead, len;
	int tslot; 

	ttyMasterPty = GetPty(ttyName);
	if (ttyMasterPty == -1) return 0;

	if ((t->pid = vfork()) < 0) return 0;

	if (t->pid == 0) {
	    sprintf(Sbuf, "-S%c%c%d", ttyName[8], ttyName[9], ttyMasterPty);

	    tslot = ttyslot();

	    (void) execlp(xdbpath, xdbpath, Sbuf, (char *) 0);
            (void) fprintf(stderr, "dbx: exec of %s failed.\n",xdbpath);

	} else {
	    if (!keepMasterOpen) close(ttyMasterPty);

 /*
  * Added this printf so we can kill the dxterm if it doesn't go away
  * by itself.
  */
            (void) printf("%d dxtermpid \n", t->pid);

	    /* Open the slave end of the pty */

	    ttyName[5] = 't';	/* Change /dev/pty?? to /dev/tty?? */

	    t->file = open(ttyName, O_RDWR, 0777);
	    
	    if (t->file < 0) {
		/* Couldn't open the tty--better get rid of the process */
		kill (t->pid, SIGINT);
		return 0;
	    }

	    /* Read the windowid from the pty */

	    len = 0;
	    while ((bytesRead = read(t->file, inputBuffer + len,
		    sizeof(int) - len)) > 0) len += bytesRead;

	    /* Flush the rest of the garbahge */

	    ioctl(t->file, TIOCFLUSH, (struct sgttyb *) NULL);

	}
	return 1;
#undef BUFSIZE
}

int GetPty(name)
	char *name;
{
	register int devindex, letter;
	int fd;

	strcpy(name, "/dev/ptyp0");

	for (letter = 0; letter < 11; letter++) {
	    name[8] = "pqrstuvwxyz"[letter];
	    
	    for (devindex = 0; devindex < 16; devindex++) {
		name[9] = "0123456789abcdef"[devindex];
		if ((fd = open (name, O_RDWR)) >= 0) return fd;
	    }
	}
	
	return -1;
}	
