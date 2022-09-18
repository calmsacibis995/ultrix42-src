#ifndef lint
static	char	*sccsid = "@(#)mkswapconf.c	4.2	(ULTRIX)	9/10/90";
#endif

/************************************************************************
 *									*
 *			Copyright (c) 1985,86, 89 by			*
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
/************************************************************************
 *
 *			Modification History
 *
 * 30-Aug-90 -- skc (Sumanta Chatterjee)
 *      Added support for generating shadow device structures.  This
 *      will allow root file system to be structured on a shadow device.
 *   
 * 12-Jun-89 -- gg (Ganesan Gopal)
 *	Changed to support "swap on none"
 *
 * 17-Feb-89 -- map (Mark Parenti)
 *	Only include setconf() if machine is MIPS.
 *
 * 01-Dec-88 -- Tim Burke
 *	Allow mscp disk devices in the range 0 to MAXDISKS. 002
 *
 * 25-Feb-86 -- jrs
 *	Changed to support "swap on boot" similar to "swap on generic"
 *
 *	Stephen Reilly, 04-Apr-85
 * 001- We now allow logical devices up to 31.
 *
 ***********************************************************************/
/*
 * Build a swap configuration file.
 */
#include "config.h"

#include <stdio.h>
#include <ctype.h>

swapconf()
{
	register struct file_list *fl;
	struct file_list *do_swap();

	fl = conf_list;
	while (fl) {
		if (fl->f_type != SYSTEMSPEC) {
			fl = fl->f_next;
			continue;
		}
		fl = do_swap(fl);
	}
}

struct file_list *
do_swap(fl)
	register struct file_list *fl;
{
	FILE *fp;
	char  swapname[80], *cp;
	register struct file_list *swap;
        register struct shad_device *shad;        
	dev_t dev;
        int i;

	if (eq(fl->f_fn, "generic") || eq(fl->f_fn, "boot")) {
		fl = fl->f_next;
		return (fl->f_next);
	}
	(void) sprintf(swapname, "swap%s.c", fl->f_fn);
	fp = fopen(path(swapname), "w");
	if (fp == 0) {
		perror(path(swapname));
		exit(1);
	}
	fprintf(fp, "#include \"../h/param.h\"\n");
	fprintf(fp, "#include \"../h/conf.h\"\n");
	fprintf(fp, "\n");
	/*
	 * If there aren't any swap devices
	 * specified, just return, the error
	 * has already been noted.
	 */
	swap = fl->f_next;
	if (swap == 0 || swap->f_type != SWAPSPEC) {
		(void) unlink(path(swapname));
		fclose(fp);
		return (swap);
	}
	fprintf(fp, "dev_t\trootdev = makedev(%d, %d);\n",
		major(fl->f_rootdev), minor(fl->f_rootdev));
	fprintf(fp, "dev_t\tdumpdev = makedev(%d, %d);\n",
		major(fl->f_dumpdev), minor(fl->f_dumpdev));
	fprintf(fp, "\n");
	fprintf(fp, "struct\tswdevt swdevt[] = {\n");
	if(!eq(fl->f_fn, "none")) {
		do {
			dev = swap->f_swapdev;
			fprintf(fp, "\t{ makedev(%d, %d),\t%c,\t%d },\t/* %s */\n",
		    	major(dev), minor(dev), (dev == fl->f_rootdev)?'1':'0', swap->f_swapsize, swap->f_fn);
			swap = swap->f_next;
		} while (swap && swap->f_type == SWAPSPEC);
	}
	fprintf(fp, "\t{ 0, 0, 0 }\n");
	fprintf(fp, "};\n");

        /*
         * Handle the case if there is a shadowed configuration device.
         */
        shad = shad_tabp;
        if (shad) {
            fprintf(fp, "struct\tshaddevt shaddevtbl[] = {\n");

            do {
                dev = shad->shad_devt;
                fprintf(fp, "\t{%d, %d ",dev, shad->num_of_const);
                for (i = 0; i < shad->num_of_const; i++) {
                    fprintf(fp, ", %d", shad->constituents[i]);                
                }
            
                fprintf(fp, "\t}, \n");
                shad = shad->next_shad;    
            } while (shad);

            fprintf(fp, "\t{0, 0} \n");
            fprintf(fp, "}; \n");
        }                

	if (machine == MACHINE_MIPS)
	   fprintf(fp, "\nsetconf(){}\n\n");
	fclose(fp);
	return (swap);
}

static	int devtablenotread = 1;
static	struct devdescription {
	char	*dev_name;
	int	dev_major;
	struct	devdescription *dev_next;
} *devtable;

/*
 * Given a device name specification figure out:
 *	major device number
 *	partition
 *	device name
 *	unit number
 * This is a hack, but the system still thinks in
 * terms of major/minor instead of string names.
 */
dev_t
nametodev(name, defunit, defpartition)
	char *name;
	int defunit;
	char defpartition;
{
	char *cp, partition;
	register int unit;
	register int maj_no;
	register int maxdisk = NONMSCP_MAXDISK;
	register struct devdescription *dp;

	cp = name;
	if (cp == 0) {
		fprintf(stderr, "config: internal error, nametodev\n");
		exit(1);
	}
	while (*cp && !isdigit(*cp))
		cp++;
	unit = *cp ? atoi(cp) : defunit;
	if (name) {
		if (strncmp(name,"ra",2) == 0) {
			maxdisk = MSCP_MAXDISK;
		}
	}
	if (unit < 0 || unit > maxdisk) {		/* 001, 002 */
		fprintf(stderr,
"config: %s: invalid device specification, unit out of range\n", name);
		unit = defunit;			/* carry on more checking */
	}
	if (*cp) {
		*cp++ = '\0';
		while (*cp && isdigit(*cp))
			cp++;
	}
	partition = *cp ? *cp : defpartition;
	if (partition < 'a' || partition > 'h') {
		fprintf(stderr,
"config: %c: invalid device specification, bad partition\n", *cp);
		partition = defpartition;	/* carry on */
	}
	if (devtablenotread)
		initdevtable();
	for (dp = devtable; dp->dev_next; dp = dp->dev_next)
		if (eq(name, dp->dev_name))
			break;
	if (dp == 0) {
		fprintf(stderr, "config: %s: unknown device\n", name);
		return (NODEV);
	}
	/*
	 * A unit number greater than UNITS_PER_MAJOR means that a higher 
	 * instance of the mscp major number should be used.
 	 * An non-mscp device which passes a large invalid unit number should
	 * have already been weeded out by this point.
	 */
	maj_no = dp->dev_major;
	if (unit > UNITS_PER_MAJOR) {
		maj_no += unit/UNITS_PER_MAJOR;
		unit = (unit%UNITS_PER_MAJOR);
	}
	return (makedev(maj_no, (unit << 3) + (partition - 'a')));
}

/*
 * This is identical to nametodev function.  The only difference is
 * that shadow devices are not "partitioned".  Thus, while calculating
 * the makedev, the minor number input should be just the unit value.
 */
dev_t
shadnametodev(name, defunit, defpartition)
	char *name;
	int defunit;
	char defpartition;
{
	char *cp, partition;
	register int unit;
	register int maj_no;
	register int maxdisk = NONMSCP_MAXDISK;
	register struct devdescription *dp;

	cp = name;
	if (cp == 0) {
		fprintf(stderr, "config: internal error, shadnametodev\n");
		exit(1);
	}
	while (*cp && !isdigit(*cp))
		cp++;
	unit = *cp ? atoi(cp) : defunit;
        
	if (unit < 0 || unit > maxdisk) {		
		fprintf(stderr,
"config: %s: invalid device specification, unit out of range\n", name);
		unit = defunit;			/* carry on more checking */
	}
	if (*cp) {
		*cp++ = '\0';
		while (*cp && isdigit(*cp))
			cp++;
	}
	partition = *cp ? *cp : defpartition;
	if (partition < 'a' || partition > 'h') {
		fprintf(stderr,
"config: %c: invalid device specification, bad partition\n", *cp);
		partition = defpartition;	/* carry on */
	}
	if (devtablenotread)
		initdevtable();
	for (dp = devtable; dp->dev_next; dp = dp->dev_next)
		if (eq(name, dp->dev_name))
			break;
	if (dp == 0) {
		fprintf(stderr, "config: %s: unknown device\n", name);
		return (NODEV);
	}

	maj_no = dp->dev_major;

	return (makedev(maj_no, unit));
}

char *
devtoname(dev)
	dev_t dev;
{
	char buf[80]; 
	register struct devdescription *dp;
	register int unit_no;

	if (devtablenotread)
		initdevtable();

	for (dp = devtable; dp->dev_next; dp = dp->dev_next) {
		if (major(dev) == dp->dev_major)
			break;
		/*
		 * If the device being named is an mscp device.
		 * And the matching devtable entry is an mscp device the
		 * unit number also has to be examined for a match.
		 * "ra" devices are stored in devices.vax as MSCP_MIN.
		 */
		if (((major(dev) >= MSCP_MIN) && (major(dev) <= MSCP_MAX)) &&
			(dp->dev_major == MSCP_MIN)) {
			break;
		}
	}
	if (dp == 0)
		dp = devtable;
	unit_no = minor(dev) >> 3;
	/*
	 * MSCP devices can occupy more than one major number.
	 * For this reason the unit number is a combination of
	 * major/minor numbers.
	 */
	if ((major(dev) > MSCP_MIN) && (major(dev) <= MSCP_MAX)) {
		unit_no += (UNITS_PER_MAJOR * (major(dev) - MSCP_MIN));
	}
	sprintf(buf, "%s%d%c", dp->dev_name,
		unit_no, (minor(dev) & 07) + 'a');
	return (ns(buf));
}

initdevtable()
{
	char buf[BUFSIZ];
	int maj;
	register struct devdescription **dp = &devtable;
	FILE *fp;

        sprintf(buf, "devices.%s", machinename);

	fp = fopen(buf, "r");
	if (fp == NULL) {
		fprintf(stderr, "config: can't open %s\n", buf);
		exit(1);
	}
	while (fscanf(fp, "%s\t%d\n", buf, &maj) == 2) {
		*dp = (struct devdescription *)malloc(sizeof (**dp));
		(*dp)->dev_name = ns(buf);
		(*dp)->dev_major = maj;
		dp = &(*dp)->dev_next;
	}
	*dp = 0;
	fclose(fp);
	devtablenotread = 0;
}
