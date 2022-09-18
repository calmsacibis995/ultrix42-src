#ifndef lint
static	char	*sccsid = "@(#)getcpu.c	4.1  (ULTRIX)        7/2/90";
#endif lint
/************************************************************************
 *									*
 *			Copyright (c) 1987 by				*
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
 * Name: getcpu.c
 *
 * Modification History
 * 
 * July 6, 1989 - Alan Frechette
 *	Added check for a VAXSTAR cputype.
 *
 * May 02, 1989 - Alan Frechette
 *	Changes to deal with new unique "cpu" handling for both
 *	vax and mips architectures.
 *
 * Feb 12, 1989 - Alan Frechette
 *	New sizer code which supports multiple architectures.
 *	Restructured this code and cleaned it up considerably.
 *	Based on the original V3.0 sizer by Tungning Cherng.
 *
 ***********************************************************************/

#include "sizer.h"

/****************************************************************
*    getcpu							*
*    								*
*    Get cpu type that this program is running on.		*
****************************************************************/
getcpu(displayflag)
int displayflag;
{
	int index, cputype;
	long offset;

	/* Read the cpu type from kernel memory */
    	if(nl[NL_cpu].n_type == N_UNDF)
		quitonerror(-4);
	offset = reset_anythg(NL_cpu);
	offset = lseek(kmem, offset, 0);
	read(kmem, &cputype, sizeof(cputype));

	/* Find cpu type in the "cputbl" table */
	for(index = 0; index<CPUTBLSIZE-1; index++) {
		CPU = cputbl[index].cputype;
		if(CPU == cputype)
			break;
	}

	/* Figure out the boot device for this cpu */
	strcpy(BOOT,cputbl[index].bootdev);
	getsubcpu(NODISPLAY);

	/* Display the cpu type to standard output */
	if(displayflag == DISPLAY) {
		switch(CPU) {
		case MVAX_I:
			fprintf(stdout, "MVAX_I\n");
			break;
		case MVAX_II:
			fprintf(stdout, "MVAX_II\n");
			break;
		case VAXSTAR:
			fprintf(stdout, "VAXSTAR\n");
			break;
		default:
			fprintf(stdout, "%s\n", cputbl[index].cpuname);
			break;
		}
	}
	return(index);
}
/****************************************************************
*    getsubcpu							*
*    								*
*    Get cpu subtype that this program is running on.		*
****************************************************************/
getsubcpu(displayflag)
int displayflag;
{
	long offset;

	/* Read the cpu subtype from kernel memory */
    	if(nl[NL_cpu_subtype].n_type == N_UNDF) {
		CPUSUB = -1;
		return(CPUSUB);
	}
	offset = reset_anythg(NL_cpu_subtype);
	offset = lseek(kmem, offset, 0);
	read(kmem, &CPUSUB, sizeof(CPUSUB));

	/* Display the cpu subtype to standard output */
	if(displayflag == DISPLAY)
		fprintf(stdout, "%d\n", CPUSUB);
	return(CPUSUB);
}

/****************************************************************
*    getcpusubsub						*
*    								*
*    Get cpu sub subtype that this program is running on.	*
*    The cpu sub subtype is one level below the cpu subtype.	*
****************************************************************/
getcpusubsub()
{
	int cpusubsub;
	long offset;

	/* Read the cpu sub subtype from kernel memory */
	offset = reset_anythg(NL_cpu_sub_subtype);
	offset = lseek(kmem, offset, 0);
	read(kmem, &cpusubsub, sizeof(cpusubsub));
	return(cpusubsub);
}

/****************************************************************
*    getmaxcpu							*
*								*
*    Get the maximum number of cpu's in the system.		*
****************************************************************/
getmaxcpu()
{
	int maxcpu;
	long offset;

	/* Read the maximum number of cpu's from kernel memory */
	offset = reset_anythg(NL_cpu_avail);
	lseek(kmem, offset, 0);
	read(kmem, &maxcpu, sizeof(maxcpu));
	if(maxcpu==0 || maxcpu>62)
		maxcpu=1;
	return(maxcpu);
}
