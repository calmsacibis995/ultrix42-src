#ifndef lint
static	char	*sccsid = "@(#)mkheaders.c	4.3	(ULTRIX)	12/6/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1984,86,87,88,90 by		*
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
 *
 * Modification History
 *
 * 20-Aug-90 -- Matt Thomas
 *	Emit a header file for all psuedo-devices, even if they haven't
 *	had any matching "files" lines (in which case use 0 as the count).
 *
 * 08-Jun-90 -- Paul Grist
 *	removed ifdef mips/vax from vba.h, vaxbi.h, and xmi.h, now
 * 	vec_intr.c gets made at system config time and the values
 *	are needed for both vax and mips.
 *
 * 20-Dec-89 -- Paul Grist
 *      Added VMEbus support - vba adapters. Modified so that headers
 *      will create/modify vba.h appropriately.
 *
 *  1-25-88 -- Ricky Palmer
 *	Added MSI support.
 *
 * 12-11-87 -- Robin L. and Larry C.	
 *	Added portclass support to the system.
 *
 * 11-Mar-86 -- jrs
 *	Fix problem with handing count specifier on bus command
 *
 * 06-Mar-86 -- jrs
 *	Fix up problems caused by Berekely using hard coded values
 *	instead of header file constants.
 *
 * 25-Feb-86 -- jrs
 *	Changed to allow multiple "needs" files per files.* line
 *
 *-----------------------------------------------------------------------
 */

/*
 * Make all the .h files for the optional entries
 */

#include <stdio.h>
#include <ctype.h>
#include "config.h"
#include "y.tab.h"

headers()
{
	register FILE *fp;
	register struct device *dp;
	register struct file_list *fl;
	int needndx;

	for (fl = ftab; fl != 0; fl = fl->f_next)
		for (needndx = 0; needndx < NNEEDS; needndx++) {
			if (fl->f_needs[needndx] == NULL) {
				break;
			}
			do_count(fl->f_needs[needndx], fl->f_needs[needndx],
						1);
		}

	for (dp = dtab; dp != 0; dp = dp->d_next) {
		if (dp->d_unit != -1 && dp->d_type == PSEUDO_DEVICE) {
			struct device *dp2;
			for (dp2 = dp->d_next; dp2; dp2 = dp2->d_next) {
				if (dp2->d_unit != -1
						&& dp2->d_type == PSEUDO_DEVICE
						&& eq(dp->d_name, dp2->d_name)) {
					dp2->d_counted = dp->d_counted;
					fprintf(stderr, "Warning: ignoring multiple references to pseudo-device %s\n", dp2->d_name);
				}
			}
			if (!dp->d_counted)
				do_header(dp->d_name, dp->d_name, 0);
		}
	}

	/* check to see if the SCS subsytem is configured in :
	 * currently if "uq", "bvpssp", "ci", or "msi" are configured
	 * then the scs subsystem is also included.
	 */
 
	if (isconfigured("uq") || isconfigured("bvpssp") 
			       || isconfigured("ci") || isconfigured("msi")) {
		if (scs_system_id.lol==0 && scs_system_id.hos==0) {
			printf("scs_sysid must be specified\n");
			exit(1);
		}


		{
		register int i;
		fp=fopen(path("scs_data.h"),"w");
		fprintf(fp,"#define SCS_NODE_NAME { ");
		for (i=0; ident[i] != '\0' && i < 8; i++)
			fprintf(fp, "'%c',", ident[i]);
		while(i++<8) {
			fprintf(fp, "' '");
			if(i < 8)
				fprintf(fp, ",");
		}
		fprintf(fp,"}\n");
		fprintf(fp,"#define SCS_SYSID { %d,%d }\n", scs_system_id.lol, 
			scs_system_id.hos);
		(void) fclose(fp);
		}
	}

        fp=fopen(path("vba.h"),"w");
        fprintf(fp,"#define NVBA %d \n",vba_bus.max_bus_num);
        fprintf(fp,"#define CVBA %d \n",vba_bus.cnt);
        (void) fclose(fp);

	fp=fopen(path("vaxbi.h"),"w");
	fprintf(fp,"#define NVAXBI %d \n",vaxbi_bus.max_bus_num);
	fprintf(fp,"#define CVAXBI %d \n",vaxbi_bus.cnt);
	(void) fclose(fp);

	fp=fopen(path("xmi.h"),"w");
	fprintf(fp,"#define NXMI %d \n",xmi_bus.max_bus_num);
	fprintf(fp,"#define CXMI %d \n",xmi_bus.cnt);
	(void) fclose(fp);

}

/*
 * count all the devices of a certain type and recurse to count
 * whatever the device is connected to
 */
do_count(dev, hname, search)
	register char *dev, *hname;
	int search;
{
	register struct device *dp, *mp;
	register int count;

	for (count = 0,dp = dtab; dp != 0; dp = dp->d_next)
		if (dp->d_unit != -1 && eq(dp->d_name, dev)) {
			if (dp->d_type == PSEUDO_DEVICE || dp->d_type == BUS) {
				count =
				    dp->d_slave != UNKNOWN ? dp->d_slave : 1;
				dp->d_counted = 1;
				break;
			}
			count++;
			/*
			 * Allow holes in unit numbering,
			 * assumption is unit numbering starts
			 * at zero.
			 */
			if (dp->d_unit + 1 > count)
				count = dp->d_unit + 1;
			if (search) {
				mp = dp->d_conn;
				if (mp != 0 && mp != TO_NEXUS && 
				    mp->d_conn != TO_NEXUS) {
					do_count(mp->d_name, hname, 0);
					search = 0;
				}
			}
		}
	do_header(dev, hname, count);
}

do_header(dev, hname, count)
	char *dev, *hname;
	int count;
{
	char *file, *name, *inw, *toheader(), *tomacro();
	struct file_list *fl, *fl_head;
	FILE *inf, *outf;
	int inc, oldcount;

	file = toheader(hname);
	name = tomacro(dev);
	
	inf = fopen(file, "r");
	oldcount = -1;
	if (inf == 0) {
		outf = fopen(file, "w");
		if (outf == 0) {
			perror(file);
			exit(1);
		}
		fprintf(outf, "#define %s %d\n", name, count);
		(void) fclose(outf);
		return;
	}
	fl_head = 0;
	for (;;) {
		char *cp;
		if ((inw = get_word(inf)) == 0 || inw == (char *)EOF)
			break;
		if ((inw = get_word(inf)) == 0 || inw == (char *)EOF)
			break;
		inw = ns(inw);
		cp = get_word(inf);
		if (cp == 0 || cp == (char *)EOF)
			break;
		inc = atoi(cp);
		if (eq(inw, name)) {
			oldcount = inc;
			inc = count;
		}
		cp = get_word(inf);
		if (cp == (char *)EOF)
			break;
		fl = (struct file_list *) malloc(sizeof *fl);
		fl->f_fn = inw;
		fl->f_type = inc;
		fl->f_next = fl_head;
		fl_head = fl;
	}
	(void) fclose(inf);
	if (count == oldcount) {
		for (fl = fl_head; fl != 0; fl = fl->f_next)
			free((char *)fl);
		return;
	}
	if (oldcount == -1) {
		fl = (struct file_list *) malloc(sizeof *fl);
		fl->f_fn = name;
		fl->f_type = count;
		fl->f_next = fl_head;
		fl_head = fl;
	}
	outf = fopen(file, "w");
	if (outf == 0) {
		perror(file);
		exit(1);
	}
	for (fl = fl_head; fl != 0; fl = fl->f_next) {
		fprintf(outf, "#define %s %d\n",
		    fl->f_fn, count ? fl->f_type : 0);
		free((char *)fl);
	}
	(void) fclose(outf);
}

/*
 * convert a dev name to a .h file name
 */
char *
toheader(dev)
	char *dev;
{
	static char hbuf[80];

	(void) strcpy(hbuf, path(dev));
	(void) strcat(hbuf, ".h");
	return (hbuf);
}

/*
 * convert a dev name to a macro name
 */
char *tomacro(dev)
	register char *dev;
{
	static char mbuf[20];
	register char *cp;

	cp = mbuf;
	*cp++ = 'N';
	while (*dev)
		*cp++ = toupper(*dev++);
	*cp++ = 0;
	return (mbuf);
}
