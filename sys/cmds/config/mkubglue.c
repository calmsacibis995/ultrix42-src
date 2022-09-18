#ifndef lint
static char *sccsid = "@(#)mkubglue.c	4.2	ULTRIX	8/3/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1984,86,87,88, 90 by		*
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
 * Make the uba interrupt file ubglue.s for vax and scb_vec.c for mips
 */

/*************************************************************************
 *			Modification History
 *
 * 03-Aug-90	rafiey (Ali Rafieymehr)
 *	Added VAX9000 support.
 *
 * 06-Jun-90 	Pete Keilty
 *	Added ci support - dump_vec_adapt.
 *
 * 20-Dec-89    Paul Grist
 *      Added VMEbus support - vba adapters.
 *
 * 19-July-89	robin
 *	added vax device support to mips systems
 *
 * 14-Apr-88	robin
 *	Added ibus support.
 *
 * 24-Mar-88 -- darrell
 *	Added VAX60 (Firefox) support.
 *
 * 15-Feb-88 -- fred (Fred Canter)
 *	VAX420 (CVAXstar/PVAX) support.
 *
 * 20-Apr-87 -- afd
 *	Changed name CVAXQ to VAX3600
 *
 * 09-Mar-87 -- afd
 *	Added CVAXQ to direct-vectored vax list
 *
 * 2 jul 86 -- fred (Fred Canter)
 *	Include pseudo DMA code if VAXstar console SLU (ss) configured.
 *
 * 8 Apr 86 -- lp
 *	Added bvp support
 *
 * 05-Mar-86 -- jrs
 *	Added support for configuring direct bi devices.
 *
 * 19-FEB-86 -- jaw
 *	added other direct vectored VAX 8800.
 *
 * 11-Sep-85 -- jaw
 *	added other direct vectored VAX's (8200,MVAX).
 *
 * 5-May-85 -- Larry Cohen
 *	special case the dmz so that we can determine which octet is
 *	interrupting
 */

#include <stdio.h>
#include "config.h"
#include "y.tab.h"

ubglue()
{
	register FILE *fp;
	register struct device *dp, *mp;
	switch (machine) {
	case MACHINE_VAX:
		fp = fopen(path("ubglue.s"), "w");
		if (fp == 0) {
			perror(path("ubglue.s"));
			exit(1);
		}
		break;
	case MACHINE_MIPS:
		fp = fopen(path("scb_vec.c"), "w");
		if (fp == 0) {
			perror(path("scb_vec.c"));
			exit(1);
		}

		break;
	}

	for (dp = dtab; dp != 0; dp = dp->d_next) {
		mp = dp->d_conn;
		if (mp != 0 && 
		    ((mp != TO_NEXUS && !eq(mp->d_name, "mba"))
		     || eq(dp->d_name,"ci")) ) {
			struct idlst *id, *id2;

			for (id = dp->d_vec; id; id = id->id_next) {
				for (id2 = dp->d_vec; id2; id2 = id2->id_next) {
					if (id2 == id) {
						dump_vec(fp, id->id, 
							 dp->d_unit);
						break;
					}
					if (!strcmp(id->id, id2->id))
						break;
				}
			}
		}
	}
	/* dump out bus error vector for each bus connected to nexus */
	dump_vec_adapt("ci",fp);
	dump_vec_adapt("vaxbi",fp);
	dump_vec_adapt("xmi",fp);
	dump_vec_adapt("vba",fp);

	(void) fclose(fp);
}

int dmzr = 0;
int dmzx = 0;
/*
 * print an interrupt vector
 */
dump_vec(fp, vector, number)
	register FILE *fp;
	char *vector;
	int number;
{
	char nbuf[80];
	register char *v = nbuf;

	switch (machine) {
	case MACHINE_VAX:

		(void) sprintf(v, "%s%d", vector, number);
		fprintf(fp, "\t.globl\t_X%s\n\t.align\t2\n_X%s:\n\tpushr\t$0x3f\n",
			v, v);
		if (strncmp(vector, "dzx", 3) == 0)
			fprintf(fp, "\tmovl\t$%d,r0\n\tjmp\tdzdma\n\n", number);
		else if (strncmp(vector, "ssx", 3) == 0)
			fprintf(fp, "\tmovl\t$%d,r0\n\tjmp\tssdma\n\n", number);
		else if (strncmp(vector, "fcx", 3) == 0)
			fprintf(fp, "\tmovl\t$%d,r0\n\tjmp\tfcdma\n\n", number);
		else {
			if (strncmp(vector, "uur", 3) == 0) {
				fprintf(fp, "#ifdef UUDMA\n");
				fprintf(fp, "\tmovl\t$%d,r0\n\tjsb\tuudma\n", number);
				fprintf(fp, "#endif\n");
			}
			if (strncmp(vector, "dmzr", 4)==0 )  {
				vector[strlen(vector)-1] = '\0';
				fprintf(fp, "\tpushl\t$%d\n", (dmzr++ % 3));
			}
			if (strncmp(vector, "dmzx", 4)==0 ) {
				vector[strlen(vector)-1] = '\0';
				fprintf(fp, "\tpushl\t$%d\n", (dmzx++ % 3));
			}
			fprintf(fp, "\tpushl\t$%d\n", number);
			if (strncmp(vector, "dmz", 3)==0 )
				fprintf(fp, "\tcalls\t$2,_%s\n", vector);
			else
				fprintf(fp, "\tcalls\t$1,_%s\n", vector);
			fprintf(fp, "#if defined(VAX750) || defined(VAX730) || defined(VAX8200) || defined(MVAX) || defined(VAX420) || defined(VAX3600)  || defined(VAX60) || defined(VAX8800) || defined(VAX6200) || defined(VAX6400) || defined(VAX9000)\n");
			
			fprintf(fp,"\tmfpr\t$1,r0\n");
			fprintf(fp,"\tincl\tCPU_INTR(r0)\n");
			fprintf(fp,"#endif\n\tpopr\t$0x3f\nrei\n\n");
		}
		break;
      case MACHINE_MIPS:
		fprintf(fp,"X%s%d(stray_arg)\nint stray_arg;\n",vector,number);
		fprintf(fp,"/* stray_arg is not used here but locore calls it with\n");
		fprintf(fp," * an argument that is the offset into the scb data structure;\n");
		fprintf(fp," * which the stray interrupt routine uses to find where the\n");
		fprintf(fp," * stray came from.  The unused arg keeps everything consistent.\n");
		fprintf(fp," */\n{\n");
		fprintf(fp,"\textern %s();\n",vector);
		fprintf(fp,"\t%s(%d);\n",vector,number);
		fprintf(fp,"}\n");
		break;
	}
}
