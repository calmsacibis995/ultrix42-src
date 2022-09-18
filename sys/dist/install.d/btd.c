#ifndef lint
static	char	*sccsid = "@(#)btd.c	4.2	(ULTRIX)	10/9/90";
#endif

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
 ************************************************************************
 *
 * Modification History
 *
 * 17-Sep-1990 Joe Szczypek
 *	Added TURBOchannel console support.  
 *
 * 18-Dec-1989 Jon Wallace
 *	Added multiple controller support for tz and rz devices.
 *
 * 14-Aug-1989 Jon Wallace
 *	Added Calypso support.
 *
 * 24-May-1989 Jon Wallace
 *	Added MipsFAIR support.
 *
 * 01-May-1989 Jon Wallace
 *	Added TEAMATE support.
 *
 * 29-Jun-88 tresvik
 * 	Added support for tape units other than 0.  This was needed to
 *	to support Lynx and Pvax.  
 * 
 * 7-Jun-88 Jon Wallace
 * 	added PVAX support for TZ30.  Note UWS distribution assumed.
 *
 * 23-sep-88 Jon Wallace
 *	Modified Pvax code to handle dual controllers
 *
 * 2-mar-1989 Jon Wallace
 *      Modified code to include TEAMMATE II support.
 *
 */

#include <sys/file.h>
#include <sys/types.h>
#include <machine/cpuconf.h>
#ifdef vax
#include "../machine/vax/rpb.h"
#include "../sas/vax/vmb.h"
#endif vax
#include "../h/sysinfo.h"

main()
{
#ifdef vax
    struct rpb info;
    int fd;

    if((fd = open("/dev/kmem", O_RDONLY)) == -1){
	perror("Couldn't open /dev/kmem");
	exit(1);
    }

    lseek(fd, 0x80000000, 0);
    read(fd, &info, sizeof(struct rpb));
    switch (info.devtyp)
    {
	case BTD$K_QNA:
	case BTD$K_KA640_NI:
		printf("NETWORK");
		break;

	case BTD$K_SII:
		printf("rz%d", info.unit/100);
		break;

	case BTD$K_TK50:
	case BTD$K_AIE_TK50:
		printf("tms%d", info.unit);
		break;

	case BTD$K_KA640_TAPE:
		/*
		 * for the PVAX we assume UWS distribution
		 */
		if ((info.cpu == C_VAXSTAR) && 
		    (info.cpu_subtype == ST_VAXSTAR) &&
                        (info.ws_display_type)) {
			printf("tz%d", ((info.unit/100) + ((info.ctrllr - 1) * 8))); 
			break;
		}
		if (info.ws_display_type)
			printf("st%d", info.unit);
		break;

	case BTD$K_KA420_DISK:
		printf("rz%d", ((info.unit/100) + ((info.ctrllr - 1) * 8)));
		break;

	default:
		break;
    }
#endif vax
#ifdef mips
#define BOOTDEVLEN 80
	char	bootdev[BOOTDEVLEN];
	char	bootctlr[4];
	char	constype[4];
	char	*cp, cn;

	getsysinfo(GSI_BOOTDEV, bootdev, sizeof(bootdev));
	getsysinfo(GSI_BOOTCTLR, bootctlr, sizeof(bootctlr));
	getsysinfo(GSI_CONSTYPE, constype, sizeof(constype));

	if (strncmp(constype,"TCF0", 4)==0) {
		if(!strncmp(bootdev+2,"mop",3)) {
			printf("NETWORK");
		}
		if(!strncmp(bootdev+2,"rz",2)) {
			cn = atoi(bootctlr);
			printf("rz%d",cn * 8 + atoi(bootdev+4));
		}	
		if(!strncmp(bootdev+2,"tz",2)) {
			cn = atoi(bootctlr);
			printf("tz%d",cn * 8 + atoi(bootdev+4));
		}	

	} else {

		if (!strncmp(bootdev, "mop", 3)) {
			printf("NETWORK");
		} 
		else if (!strncmp(bootdev, "tm", 2)) {
			cp = bootdev;
			while ((*cp++ != ',') && *cp);
			printf("tms%d", atoi(cp)); 
		}
		else if (!strncmp(bootdev, "tz", 2)) {
			cp = bootdev;
			while ((*cp++ != '(') && *cp);
			cn = atoi(cp);
			while ((*cp++ != ',') && *cp);
			printf("tz%d", cn * 8 + atoi(cp)); 
		}
		else if (!strncmp(bootdev, "rz", 2)) {
			cp = bootdev;
			while ((*cp++ != '(') && *cp);
			cn = atoi(cp);
			while ((*cp++ != ',') && *cp);
			printf("rz%d", cn * 8 + atoi(cp)); 
		}
		else
			printf("%s", bootdev);
	}
#endif mips
exit(0);
}
