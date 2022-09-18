#ifndef lint
static	char	*sccsid = "@(#)getroot.c	4.1  (ULTRIX)        7/2/90";
#endif lint
/************************************************************************
 *									*
 *			Copyright(c) 1987 by				*
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
 ************************************************************************/


/************************************************************************
 *
 * Name: getroot.c
 *
 * Modification History
 * 
 * May 26, 1989 - Tim Burke
 *	Allow ra disks to occupy a range of major numbers to support
 *	larger numbers of disks.
 *
 * May 10, 1989 - Alan Frechette
 *	Figure out network boot device if booting from network.
 *
 * Feb 12, 1989 - Alan Frechette
 *	New sizer code which supports multiple architectures.
 *	Restructured this code and cleaned it up considerably.
 *	Based on the original V3.0 sizer by Tungning Cherng.
 *
 ***********************************************************************/

#include "sizer.h"
#include <sys/fs_types.h>

/****************************************************************
*    getroot							*
*								*
*    Get the root, swap, and dump devices.			*
****************************************************************/
getroot(displayflag)
int displayflag;
{
	int pt, i, index, majornum, minornum;
	long offset;
	dev_t devicenum;
	int on_network;
	char dname[10], outstring[200];
	static char part[] = {'a','b','c','d','e','f','g','h'};
	static char *cname[] = {"root", "swap", "dumps"};

	if(displayflag == NODISPLAY)
		fprintf(fp, "config\t\tvmunix\t");

	/* Read "root", "swap", and "dump" devices from kernel memory */
	for(index = NL_rootdev, i = 0; index <= NL_dumpdev; index++, i++) {
    		if(nl[index].n_type == N_UNDF)
			quitonerror(-12);
		offset = reset_anythg(index);
		offset = lseek(kmem, offset, 0);
		read(kmem, &devicenum, sizeof(devicenum));

		/* Pick off "majornum" number and "minornum" number */
		majornum = devicenum >> 8;
		minornum = ((devicenum & 0xff) / 8);
		pt = ((devicenum & 0xff) % 8);
		on_network = 0;

		/* Figure out device name based on "majornum" number */
		switch(majornum) {
		case 0: 
			sprintf(dname,"hp%d",minornum);
			break;
		case 3: 
			sprintf(dname,"rk%d",minornum);
			break;
		case 11:
			sprintf(dname,"rb%d",minornum);
			break;
		case 14: 
			sprintf(dname,"rl%d",minornum);
			break;
		case 19:
			sprintf(dname,"rd%d",minornum);
			break;
		case 21:
			sprintf(dname,"rz%d",minornum);
			break;
		/*
		 * MSCP (ra) type disks are represented by the range of
		 * major numbers 23-30.  The unit number (minornum) is
		 * no longer held in the upper 5 bits of the minor
		 * number.  Rather each major number represents 32
		 * disks.  The unit number is now the upper 5 bits of
		 * the minor number added to the appropriate multiple
		 * of 32 from the first major number (23).
		 */
		case 23:
			sprintf(dname,"ra%d",minornum);
			break;
		case 24:
			minornum += 32;
			sprintf(dname,"ra%d",minornum);
			break;
		case 25:
			minornum += (32 * 2);
			sprintf(dname,"ra%d",minornum);
			break;
		case 26:
			minornum += (32 * 3);
			sprintf(dname,"ra%d",minornum);
			break;
		case 27:
			minornum += (32 * 4);
			sprintf(dname,"ra%d",minornum);
			break;
		case 28:
			minornum += (32 * 5);
			sprintf(dname,"ra%d",minornum);
			break;
		case 29:
			minornum += (32 * 6);
			sprintf(dname,"ra%d",minornum);
			break;
		case 30:
			minornum += (32 * 7);
			sprintf(dname,"ra%d",minornum);
			break;
		default:
			getonnetwork(dname);
			on_network = 1;
			break;
		}

		if(displayflag == NODISPLAY) {
			if(!getconfig_string(cname[i])) {
				if(index == NL_swdevt && on_network)
					continue;
				if(index == NL_dumpdev && on_network)
					continue;
				if(!on_network)
				    fprintf(fp, "%s on %s%c  ",
						cname[i], dname, part[pt]);
				else
				    fprintf(fp, "%s on %s  ", 
						cname[i], dname);
			}
		}
		else {
			fprintf(stdout, "%s\n",dname);
			break;
		}
	}
	if(displayflag == NODISPLAY)
		fprintf(fp, "\n\n");
}

/****************************************************************
*    getonnetwork						*
*								*
*    Get the network boot device name.				*
****************************************************************/
getonnetwork(rootname)
char *rootname;
{
	int roottype;
	long offset;
#ifdef vax
	struct rpb rpb;

	/* Read the "rpb" structure from kernel memory */
    	if(nl[NL_rpb].n_type == N_UNDF)
		quitonerror(-7);
	offset = reset_anythg(NL_rpb);
	lseek(kmem,offset,0);
	read(kmem,&rpb,sizeof(rpb));

	/* Read the "roottype" from kernel memory */
    	if(nl[NL_roottype].n_type == N_UNDF)
		quitonerror(-14);
	offset = reset_anythg(NL_roottype);
	lseek(kmem,offset,0);
	read(kmem,&roottype,sizeof(roottype));
	if(roottype == GT_NFS) {
		switch(rpb.devtyp) {
		case BTD$K_QNA:
			strcpy(rootname,"qe0");
			break;
		case BTD$K_LANCE:
			strcpy(rootname,"se0");
			break;
		default:
			strcpy(rootname,"boot");
			break;
		}
	}
	else
		strcpy(rootname,"boot");
#endif vax
#ifdef mips
	/* Read the "roottype" from kernel memory */
    	if(nl[NL_roottype].n_type == N_UNDF)
		quitonerror(-14);
	offset = reset_anythg(NL_roottype);
	lseek(kmem,offset,0);
	read(kmem,&roottype,sizeof(roottype));
	if(roottype == GT_NFS)
		strcpy(rootname,"se0");
	else
		strcpy(rootname,"boot");
#endif mips
}
