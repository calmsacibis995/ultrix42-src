# ifndef lint
static char *sccsid = "@(#)mt.c	4.4	ULTRIX	1/22/91";
# endif not lint

/************************************************************************
 *									*
 *			Copyright (c) 1984-1989 by			*
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
 * mt --
 *   magnetic tape manipulation program
 */

/* ---------------------------------------------------------------------
 * Modification History
 *
 * January 8, 1991 by Robin Miller.
 *	Updated the print_category() function to check for density codes
 *	added for the TZK08 (DEV_54000_BPI) and the TLZ04 (DEV_61000_BPI).
 *
 * September 12, 1990 by Robin Miller.
 *	Updated the print_category() function to check for and display
 *	the density codes for QIC tapes.  Also rearranged code to allow
 *	checking for unspecified density.  This is now consistant with
 *	what the 'file' utility displays.
 *
 * July 5 1990	Bill Dallas
 *	Added retension for tzk10
 *
 * Nov  8 1989	Tim Burke
 *	Set the exit value to 0 on success.
 *	Changed the format of the `status` command to display the contents
 *	of the devget and mtiocget data structures.  Removed device dependant
 *	attribute code which was part of the old status mechanism.
 *
 * Jun 15 1989  tim 	Tim Burke
 *	Made massbus tapes the only vax-specific tapes.  Corrected a
 *	seg fault resulting from a null pointer reference if `mt status`
 *	was issued to a TZK50 in a mips box.
 *
 * Nov 23 1988  jag	John A. Gallant
 *	Moved the TZK50 entry into the tapes[], to allow the tape type from
 *	the driver to be recognized.
 *
 * Feb 11 1986  rsp     (Ricky Palmer)
 *	Removed "don't grok" error message.
 *
 * Sep 11 1986  fred	(Fred Canter)
 *	Bug fix to allow "mt status" to work with VAXstar TZK50.
 *
 * Sep  9 1986  fries	Corrected bugs whereas device was opened twice
 *			could not perform non-write functions to a write
 *			protected device.
 *
 * Aug 27 1986  fries	Made commands: clserex, clhrdsf, clsub, eoten
 *			and eotdis read only commands.
 *
 * Apr 28 1986  fries	Added commands: clserex, clhrdsf, clsub, eoten
 *			and eotdis. Added code to insure only superuser
 *			can change eot detect flag.
 *
 * Feb 10 1986  fries	Added commands: clserex, clhrdsf, clsub, eoten
 *			and eotdis. Added code to insure only superuser
 *			can change eot detect flag.
 *
 * Jan 29 1986  fries	Changed default tape definition DEFTAPE to
 *			DEFTAPE_NH to coincide with new mtio.h naming
 *			convention.
 *
 * Jul 17 1985	afd	on status command, check for tmscp device types
 *			and interpret the returned data accordingly.
 *
 * Jul 17 1985	afd	Added mt ops: "cache" and "nocache" to enable &
 *			disable caching on tmscp units.
 *
 * Dec 6 1984	afd	took out references to Sun tape devices and
 *			added tmscp device to the "tapes" table.
 *
 * Dec 6 1984	afd	derived from Berkeley 4.2BSD labeled
 * 			"@(#)mt.c	4.8 (Berkeley) 83/05/08"
 *
 * ---------------------------------------------------------------------
 */

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>

#include <sys/ioctl.h>
#include <sys/devio.h>
#include <sys/mtio.h>

#include <sys/errno.h>
#include <sys/file.h>

#ifndef MTSTATUS
#define MTSTATUS MTNOP
#endif  MTSTATUS

#define	equal(s1,s2)	(strcmp(s1, s2) == 0)


struct commands {
	char *c_name;
	int c_code;
	int c_ronly;
	int c_onlnck;
} com[] = {
	{ "weof",	MTWEOF,	0 , 1 },
	{ "eof",	MTWEOF,	0 , 1 },
	{ "fsf",	MTFSF,	1 , 1 },
	{ "bsf",	MTBSF,	1 , 1 },
	{ "fsr",	MTFSR,	1 , 1 },
	{ "bsr",	MTBSR,	1 , 1 },
	{ "rewind",	MTREW,	1 , 1 },
	{ "offline",	MTOFFL,	1 , 1 },
	{ "rewoffl",	MTOFFL,	1 , 1 },
	{ "status",	MTSTATUS, 1 , 0 },
	{ "cache",	MTCACHE, 1 , 0 },
	{ "nocache",	MTNOCACHE, 1 , 0 },
	{ "clserex",	MTCSE, 1 , 0 },
	{ "clhrdsf",	MTCLX, 1 , 0 },
	{ "clsub",	MTCLS, 1 , 0 },
	{ "eoten",	MTENAEOT, 1 , 0 },
	{ "eotdis",	MTDISEOT, 1 , 0 },
	{ "flush",	MTFLUSH, 0 , 1 },
	{ "retension",	MTRETEN, 1 , 1 },
	{ 0 }
};

int mtfd;
int generic_ioctl_suceed;

struct mtop mt_com;
struct mtget mt_status;
char *tape;

main(argc, argv)
	char **argv;
{
	char line[80], *getenv();
	register char *cp;
	register struct commands *comp;

	if (argc > 2 && (equal(argv[1], "-t") || equal(argv[1], "-f"))) {
		argc -= 2;
		tape = argv[2];
		argv += 2;
	} else
		if ((tape = getenv("TAPE")) == NULL)
			tape = DEFTAPE_NH;
	if (argc < 2) {
		fprintf(stderr, "usage: mt [ -f device ] command [ count ]\n");
		exit(1);
	}
	cp = argv[1];
	for (comp = com; comp->c_name != NULL; comp++)
		if (strncmp(cp, comp->c_name, strlen(cp)) == 0)
			break;
	if (comp->c_name == NULL) {
		fprintf(stderr, "mt: invalid command: \"%s\"\n", cp);
		exit(1);
	}

	if ((mtfd = statchk(tape, comp->c_ronly ? 0 : 2, comp->c_onlnck)) < 0) {
		if (errno)perror(tape);
		exit(1);
	}
	/* check for enable or disable eot(must be superuser) */
	if ((comp->c_code == MTENAEOT || comp->c_code == MTDISEOT) && geteuid())
           {
            fprintf(stderr, "mt: must be Superuser to perform %s\n",comp->c_name);
	    exit(1);
	    }

	if (comp->c_code != MTSTATUS) {
		mt_com.mt_op = comp->c_code;
		mt_com.mt_count = (argc > 2 ? atoi(argv[2]) : 1);
		if (mt_com.mt_count < 0) {
			fprintf(stderr, "mt: negative repeat count\n");
			exit(1);
		}
		if (ioctl(mtfd, MTIOCTOP, &mt_com) < 0) {
			fprintf(stderr, "%s %s %d ", tape, comp->c_name,
				mt_com.mt_count);

			/* The following test is in case you are past */
			/* the EOT and rewind, the rewind completes   */
			/* but an errno of ENOSPC is returned...      */
			/* ALL OTHER ERRORS are REPORTED              */
			if ((mt_com.mt_op == MTREW) && (errno == ENOSPC))
						;/* do nothing */
			else
				perror("failed");/* else perror */
			exit(2);
		}
	} 
	/* The command is MTSTATUS meaning that `mt status` was issued */
	else {
		status();
	}

	/* Success of the mt command; return 0 */
	exit(0);
}

/*
 * Print out tape status based on deviocget and mtiocget.
 */
status()
{
	print_devio(mtfd);
	print_mtio(mtfd);
}

/* Routine to obtain generic device status */
statchk(tape,mode, c_onlnck)
	char	*tape;
	int	mode, c_onlnck;
{
	int to;
	int error = 0;
	struct devget mt_info;
	
	generic_ioctl_suceed = 0;

	/* Force device open to obtain status */
	to = open(tape,mode|O_NDELAY);

	/* If open error, then error must be no such device and address */
	if (to < 0)return(-1);
	
	/* Get generic device status */
	if (ioctl(to,DEVIOCGET,(char *)&mt_info) < 0)return(to);

	/* Set flag indicating successful generic ioctl */
	generic_ioctl_suceed = 1;

	/* Check for device on line */
	if((c_onlnck) && (mt_info.stat & DEV_OFFLINE)){
	  fprintf(stderr,"\7\nError on device named %s - Place %s tape drive unit #%u ONLINE\n",tape,mt_info.device,mt_info.unit_num);
	  return(-2);
	}

	/* Check for device write locked when in write mode */
	if((c_onlnck) && (mt_info.stat & DEV_WRTLCK) && (mode != O_RDONLY)){
           fprintf(stderr,"\7\nError on device named %s - WRITE ENABLE %s tape drive unit #%u\n",tape,mt_info.device,mt_info.unit_num);
	   return(-3);
	 }
	   
	 /* All checked out ok, return file descriptor */
	 return(to);
}
/*
 * Display the contents of the mtget struct.
 * Args: fd a file descriptor of the already opened tape device.
 */
print_mtio(fd)
	int fd;
{
	struct mtget mt;
	if (ioctl(fd, MTIOCGET, (char *)&mt) < 0) {
		printf("\nmtiocget ioctl failed!\n");
		exit(1);
	}
	printf("\nMTIOCGET ELEMENT	CONTENTS");
	printf("\n----------------	--------\n");
	printf("mt_type			");
	switch(mt.mt_type) {
	case MT_ISTS:
		printf("MT_ISTS\n");
		break;
	case MT_ISHT:
		printf("MT_ISHT\n");
		break;
	case MT_ISTM:
		printf("MT_ISTM\n");
		break;
	case MT_ISMT:
		printf("MT_ISMT\n");
		break;
	case MT_ISUT:
		printf("MT_ISUT\n");
		break;
	case MT_ISTMSCP:
		printf("MT_ISTMSCP\n");
		break;
	case MT_ISST:
		printf("MT_ISST\n");
		break;
	case MT_ISSCSI:
		printf("MT_ISSCSI\n");
		break;
	default:
		printf("Unknown mt_type = 0x%x\n",mt.mt_type);
	}
	printf("mt_dsreg		%X\n", mt.mt_dsreg);
	printf("mt_erreg		%X\n", mt.mt_erreg);
	printf("mt_resid		%X\n", mt.mt_resid);
	printf("\n");
}
/*
 * Display the contents of the deviocget struct.
 * Args: fd a file descriptor of the already opened tape device.
 */
print_devio(fd)
	int fd;
{
	struct devget devinf;
	if (ioctl(fd, DEVIOCGET, (char *)&devinf) < 0) {
		printf("\ndevget ioctl failed!\n");
		exit(1);
	}
	printf("\nDEVIOGET ELEMENT	CONTENTS");
	printf("\n----------------	--------\n");
	printf("category		");
	switch(devinf.category) {
	case DEV_TAPE:
		printf("DEV_TAPE\n");
		break;
	case DEV_DISK:
		printf("DEV_DISK\n");
		break;
	case DEV_TERMINAL:
		printf("DEV_TERMINAL\n");
		break;
	case DEV_PRINTER:
		printf("DEV_PRINTER\n");
		break;
	case DEV_SPECIAL:
		printf("DEV_SPECIAL\n");
		break;
	default:
		printf("UNDEFINED VALUE (%x)\n", devinf.category);
		break;
	}
	printf("bus			");
	switch(devinf.bus) {
	case DEV_UB:
		printf("DEV_UB\n");
		break;
	case DEV_QB:
		printf("DEV_QB\n");
		break;
	case DEV_MB:
		printf("DEV_MB\n");
		break;
	case DEV_BI:
		printf("DEV_BI\n");
		break;
	case DEV_CI:
		printf("DEV_CI\n");
		break;
	case DEV_NB:	/* should be DEV_NB */
		printf("DEV_NB\n");
		break;
	case DEV_MSI:
		printf("DEV_MSI\n");
		break;
	case DEV_SCSI:
		printf("DEV_SCSI\n");
		break;
	case DEV_UNKBUS:
		printf("DEV_UNKBUS\n");
		break;
	default:
		printf("UNDEFINED VALUE (%x)\n", devinf.bus);
		break;
	}
	printf("interface		%s\n", devinf.interface);
	printf("device			%s\n", devinf.device);
	printf("adpt_num		%d\n", devinf.adpt_num);
	printf("nexus_num		%d\n", devinf.nexus_num);
	printf("bus_num			%d\n", devinf.bus_num);
	printf("ctlr_num		%d\n", devinf.ctlr_num);
	printf("slave_num		%d\n", devinf.slave_num);
	printf("dev_name		%s\n", devinf.dev_name);
	printf("unit_num		%d\n", devinf.unit_num);
	printf("soft_count		%U\n", devinf.soft_count);
	printf("hard_count		%U\n", devinf.hard_count);
	printf("stat			%X\n", devinf.stat);
	print_stat(devinf.stat);
	printf("category_stat		%X\n", devinf.category_stat);
	print_category(devinf.category_stat);
	
}

/*
 * Disect the stat field of a devio structure
 */
print_stat(stat)
	long stat;
{
	printf("\t\t\t");
	if (stat & DEV_BOM)
		printf("DEV_BOM ");
	if (stat & DEV_EOM)
		printf("DEV_EOM ");
	if (stat & DEV_OFFLINE)
		printf("DEV_OFFLINE ");
	if (stat & DEV_WRTLCK)
		printf("DEV_WRTLCK ");
	if (stat & DEV_BLANK)
		printf("DEV_BLANK ");
	if (stat & DEV_WRITTEN)
		printf("DEV_WRITTEN ");
	if (stat & DEV_CSE)
		printf("DEV_CSE ");
	if (stat & DEV_SOFTERR)
		printf("DEV_SOFTERR ");
	if (stat & DEV_HARDERR)
		printf("DEV_HARDERR ");
	if (stat & DEV_DONE)
		printf("DEV_DONE ");
	if (stat & DEV_RETRY)
		printf("DEV_RETRY ");
	if (stat & DEV_ERASED)
		printf("DEV_ERASED ");
	printf("\n");
}
/*
 * Disect the category_stat field of a devio structure
 */
print_category(stat)
	long stat;
{
	printf("\t\t\t");
	if (stat & DEV_TPMARK)
		printf("DEV_TPMARK ");
	if (stat & DEV_SHRTREC)
		printf("DEV_SHRTREC ");
	if (stat & DEV_RDOPP)
		printf("DEV_RDOPP ");
	if (stat & DEV_RWDING)
		printf("DEV_RWDING ");
	if (stat & DEV_LOADER)
		printf("DEV_LOADER ");

	if (stat & DEV_800BPI) {
		printf("DEV_800BPI");
	} else if (stat & DEV_1600BPI) {
		printf("DEV_1600BPI");
	} else if (stat & DEV_6250BPI) {
		printf("DEV_6250BPI");
	} else if (stat & DEV_6666BPI) {
		printf("DEV_6666BPI");
	} else if (stat & DEV_10240BPI) {
		printf("DEV_10240BPI");
	} else if (stat & DEV_38000BPI) {
		printf("DEV_38000BPI");
#ifdef DEV_38000_CP
	} else if (stat & DEV_38000_CP) {
		printf("DEV_38000_CP");
	} else if (stat & DEV_76000BPI) {
		printf("DEV_76000BPI");
	} else if (stat & DEV_76000_CP) {
		printf("DEV_76000_CP");
#endif DEV_38000_CP
	} else if (stat & DEV_8000_BPI) {
		printf("DEV_8000_BPI");
	} else if (stat & DEV_10000_BPI) {
		printf("DEV_10000_BPI");
	} else if (stat & DEV_16000_BPI) {
		printf("DEV_16000_BPI");
	} else if (stat & DEV_54000_BPI) {
		printf("DEV_54000_BPI");
	} else if (stat & DEV_61000_BPI) {
		printf ("DEV_61000_BPI");
	} else {
		printf("<unspecified density>");
	}
	printf("\n");
}

