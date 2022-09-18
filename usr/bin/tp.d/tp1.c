# ifndef lint
static char *sccsid = "@(#)tp1.c	4.1	ULTRIX	7/17/90";
# endif not lint

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

/* ------------------------------------------------------------------------
 * Modification History: /usr/src/etc/tp1.c
 *
 *  9 Sep 86 -- fries
 *      Modified code to perform only one open to get generic status and
 *	use device.
 *
 * 29 Apr 86 -- fries
 *      Added code to allow continuation if generic ioctl not supported
 *      for tape device.
 *
 * 20 Mar 86 -- fries
 *      Initial heading placed on code. Added code to check for type 
 *      of error causing EIO to be returned in errno.	
 *
 * ------------------------------------------------------------------------*/

#include "tp.h"

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#ifdef DEBUG
#include "mtio.h"
#include "devio.h"
#define DEVIOCGET 0
#else
#include <sys/mtio.h>
#include <sys/devio.h>
#endif
#include <sys/file.h>

main(argc,argv)
char **argv;
{
	register char c,*ptr;
	extern cmd(), cmr(),cmx(), cmt();

	tname = tc;
	command = cmr;
	if ((narg = rnarg = argc) < 2)	narg = 2;
	else {
		ptr = argv[1];	/* get first argument */
		parg = &argv[2];	/* pointer to second argument */
		while (c = *ptr++) switch(c)  {
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
				tc[8] = c;
				mt[8] = c;
				continue;

			case 'f':
				tname = *parg++;
				flags |= flm;
				narg--; rnarg--;
				continue;
			case 'c':
				flags |= flc;  continue;
			case 'd':
				setcom(cmd);  continue;
			case 'i':
				flags |= fli;  continue;
			case 'm':
				tname = mt;
				flags |= flm;
				continue;
			case 'r':
				flags &= ~flu;  setcom(cmr);  continue;
			case 's':
				flags |= fls; continue;
			case 't':
				setcom(cmt);  continue;
			case 'u':
				flags |= flu;  setcom(cmr);  continue;
			case 'v':
				flags |= flv;  continue;
			case 'w':
				flags |= flw;  continue;
			case 'x':
				setcom(cmx);  continue;
			default:
				useerr();
		}
	}
	optap();
	nptr = nameblk = (char *)malloc(1000);
	top = nptr + 1000;
	(*command)();
}

optap()
{
	extern cmr();
	int mode = 0;
	struct	stat	stat_buf;

	if ((flags & flm) == 0) {	/*  DECTAPE */
		tapsiz = TCSIZ;
		ndirent = TCDIRS;
		mode = 2;
		fio = open(tc,mode);
	} else {			/* MAGTAPE */
		tapsiz = MTSIZ;
		ndirent = MDIRENT;
		if(command == cmr) {
			mode = O_RDWR;
		   if((stat(tname,&stat_buf) >= 0)
		     &&(((stat_buf.st_mode & S_IFMT) == S_IFCHR)
		     ||((stat_buf.st_mode & S_IFMT)  == S_IFBLK)))
			fio = statchk(tname,mode);
		   else fio = creat(tname,0666);
		} else
			fio = statchk(tname,mode);
	}
	if (fio < 0)  {
		done();
	}
	ndentb = ndirent/TPB;
	edir = &dir[ndirent];
}

setcom(newcom)
int (*newcom)();
{
	extern cmr();

	if (command != cmr)  	useerr();
	command = newcom;
}

useerr()
{
	printf("Bad usage\n");
	done();
}

/*/* COMMANDS */

cmd()
{
	extern delete();

	if (flags & (flm|flc))	useerr();
	if (narg <= 2)			useerr();
	rddir();
	gettape(delete);
	wrdir();
	check();
}

cmr()
{
	if (flags & (flc|flm))		clrdir();
	else				rddir();
	getfiles();
	update();
	check();
}

cmt()
{
	extern taboc();

	if (flags & (flc|flw))	useerr();
	rddir();
	if (flags & flv)
		printf("   mode    uid gid tapa    size   date    time name\n");
	gettape(taboc);
	check();
}

cmx()
{
	extern extract();

	if (flags & (flc))		useerr();
	rddir();
	gettape(extract);
	done();
}

check()
{
	usage();
	done();
}

done()
{
	printf("End\n");
	exit(0);
}

encode(pname,dptr)	/* pname points to the pathname
			 * nptr points to next location in nameblk
			 * dptr points to the dir entry		   */

char	*pname;
struct	dent *dptr;
{
	register  char *np;
	register n;

	dptr->d_namep = np = nptr;
	if (np > top - NAMELEN)  {
		int size = top - nptr;
		nptr = realloc(nptr, 2 * size);
		if (nptr == NULL) {
			printf("Out of core\n");
			done();
		}
		top = nptr + 2 * size;
	}
	if((n=strlen(pname)) > NAMELEN) {
		printf("Pathname too long - %s\nFile ignored\n",pname);
		clrent(dptr);
	}
	else {
		nptr += n+1;
		strcpy(np, pname);
	}
}

decode(pname,dptr)	/* dptr points to the dir entry
			 * name is placed in pname[] */
char	*pname;
struct	dent *dptr;
{

	strcpy(pname, dptr->d_namep);
}

/* Routine to obtain generic device status */
statchk(tape,mode)
	char	*tape;
	int	mode;
{
	int to;
	struct devget mt_info;
	int error = 0;

	/* Force device open to obtain status */
	to = open(tape,mode|O_NDELAY);

	/* If open error, then error must be no such device and address */
	if (to < 0)return(-1);
	
	/* Get generic device status */
	if (ioctl(to,DEVIOCGET,(char *)&mt_info) < 0)return(-2);
	
	/* Check for device on line */
	if(mt_info.stat & DEV_OFFLINE){
	  fprintf(stderr,"\7\nError on device named %s - Place %s tape drive unit #%u ONLINE\n",tape,mt_info.device,mt_info.unit_num);
	  return(-3);
	}

	/* Check for device write locked when in write mode */
	else
	 if((mt_info.stat & DEV_WRTLCK) && (mode != O_RDONLY)){
           fprintf(stderr,"\7\nError on device named %s - WRITE ENABLE %s tape drive unit #%u\n",tape,mt_info.device,mt_info.unit_num);
	   return(-4);
	 }

	/* Re-Open as user requested */
	return(to);
}
