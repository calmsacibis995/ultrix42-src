#ifndef lint
static	char	*sccsid = "@(#)mkioconf.c	4.1	(ULTRIX)	7/2/90";
#endif lint


/************************************************************************
 *									*
 *			Copyright (c) 1986,87,88, 89, 90 by		*
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
 *	Modification History
 *
 * 06-Jun-90 Pete Keilty
 *	Added ci support into dump_vec_adapt and dump_dispatch routines.
 *
 * 20-Dec-89    Paul Grist
 *      Added VMEbus support - vba adapters. 
 *
 * 10-17-89	Randall Brown
 *	added the function is_cpu_declared to determine if a particular
 *	cpu type was defined.
 *
 * 07-19-98	robin
 *	added support for vax devices on mips systems
 *
 * 06-14-89	jaw
 *	add support for xmi as an I/O bus and for the kdm xmi driver.
 *
 * 05-17-89	Tim Burke
 *	Change mscp unit wildcard from '?' to -1.
 *
 * 04-14-88 	robin
 *	added ibus support
 *
 *  1-25-88	Ricky Palmer
 *	Added MSI support.
 *
 * 12-20-87
 *	Add two field to the end of config_adpt structure and init to 0.
 *	The fields are the bus number and nexus number respectively.
 *
 * 12-11-87	Robin L. and Larry C.
 *	Added portclass support to the system.
 *
 * 27-Aug-86 -- jmartin
 *	Fix segmentation fault caused here by missing or misplaced
 *	"controller" statement (SMU-307).
 *
 * 15-Apr-86 -- afd
 *	Removed reference to MACHINE_MVAX
 *
 * 8 Apr 86  -- lp
 *	Added bvp support
 *
 * 06-Mar-86 -- jrs
 *	Fix problem when both bi and massbus devices exist
 *
 * 05-Mar-86 -- jrs
 *	Add support for configuring native bi devices
 *
 *-----------------------------------------------------------------------
 */

#include <stdio.h>
#include "y.tab.h"
#include "config.h"

/*
 * build the ioconf.c file
 */
char	*qu();
char    *qumscp();
char	*intv();

/*
 * no driver routines for these devices
 */
char *tbl_nodriver[] = { "dssc", "hsc", "mscp", 0 } ; 

dec_ioconf()
{
	register struct device *dp, *mp, *np;
	int i;
	register int uba_n, slave;
	int fatal_error = 0;
	int directconn;
	FILE *fp,*fpb;
	extern int isconfigured();

	fp = fopen(path("ioconf.c"), "w");
	if (fp == 0) {
		perror(path("ioconf.c"));
		exit(1);
	}
	fpb = fopen(path("iobus.c"), "w");
	if (fpb == 0) {
		perror(path("iobus.c"));
		exit(1);
	}
	fprintf(fp, "#include \"../machine/pte.h\"\n");
	fprintf(fp, "#include \"../h/param.h\"\n");
	fprintf(fp, "#include \"../h/buf.h\"\n");
	fprintf(fp, "#include \"../h/map.h\"\n");
	fprintf(fp, "#include \"../h/vm.h\"\n");
	fprintf(fp, "#include \"../h/config.h\"\n");
	fprintf(fp, "\n");

	switch (machine) {
	case MACHINE_VAX:
		fprintf(fp, "#include \"../io/mba/vax/mbavar.h\"\n");
		break;
	case MACHINE_MIPS:
		break;
	}

        fprintf(fp, "#include \"../io/uba/ubavar.h\"\n\n");
	fprintf(fp, "\n");
	fprintf(fp, "#define C (caddr_t)\n\n");
	fprintf(fp, "int nulldev();\n");

	/* 
	 * set up null routines for devices that do not have a
	 * driver associated with them.
	 */

	{
	register char **ptr = tbl_nodriver;

	while(*ptr) { 
		if (!isconfigured(*ptr)) {
			ptr++;
			continue;
		}
    		for (dp = dtab; dp != 0; dp = dp->d_next) 
		    if(eq(dp->d_name, *ptr)) {
			fprintf(fp, "struct uba_driver %sdriver; ", *ptr);
			fprintf(fp, "int (*%sint%d[])() = { nulldev, 0 }; ", 
				*ptr, dp->d_unit);
			fprintf(fp, " /* no %sdriver */\n", *ptr);
		    }
		ptr++;
	    }
	}
	fprintf(fp, "\n");
	/*
	 * First print the mba initialization structures
	 */
	if (seen_mba) {
		for (dp = dtab; dp != 0; dp = dp->d_next) {
			mp = dp->d_conn;
			if(mp == 0 || mp == TO_NEXUS || !eq(mp->d_name, "mba"))
				continue;
			fprintf(fp, "extern struct mba_driver %sdriver;\n",
			    dp->d_name);
		}
		fprintf(fp, "\nstruct mba_device mbdinit[] = {\n");
		fprintf(fp, "\t/* Device,  Unit, Mba, Drive, Dk */\n");
		for (dp = dtab; dp != 0; dp = dp->d_next) {
			mp = dp->d_conn;
			if (dp->d_unit == QUES || mp == 0 ||
			    mp == TO_NEXUS || !eq(mp->d_name, "mba"))
				continue;
			if (dp->d_addr) {
				printf("can't specify csr address on mba for %s%d\n",
				    dp->d_name, dp->d_unit);
				fatal_error++;
				continue;
			}
			if (dp->d_vec != 0) {
				printf("can't specify vector for %s%d on mba\n",
				    dp->d_name, dp->d_unit);
				fatal_error++;
				continue;
			}
			if (dp->d_drive == UNKNOWN) {
				printf("drive not specified for %s%d\n",
				    dp->d_name, dp->d_unit);
				fatal_error++;
				continue;
			}
			if (dp->d_slave != UNKNOWN) {
				printf("can't specify slave number for %s%d\n", 
				    dp->d_name, dp->d_unit);
				fatal_error++;
				continue;
			}
			fprintf(fp, "\t{ &%sdriver, %d,   %s,",
				dp->d_name, dp->d_unit, qu(mp->d_unit));
			fprintf(fp, "  %s,  %d },\n",
				qu(dp->d_drive), dp->d_dk);
		}
		fprintf(fp, "\t0\n};\n\n");
		/*
		 * Print the mbsinit structure
		 * Driver Controller Unit Slave
		 */
		fprintf(fp, "struct mba_slave mbsinit [] = {\n");
		fprintf(fp, "\t/* Driver,  Ctlr, Unit, Slave */\n");
		for (dp = dtab; dp != 0; dp = dp->d_next) {
			/*
			 * All slaves are connected to something which
			 * is connected to the massbus.
			 */
			if ((mp = dp->d_conn) == 0 || mp == TO_NEXUS)
				continue;
			np = mp->d_conn;
			if (np == 0 || np == TO_NEXUS ||!eq(np->d_name, "mba"))
				continue;
			fprintf(fp, "\t{ &%sdriver, %s",
			    mp->d_name, qu(mp->d_unit));
			fprintf(fp, ",  %2d,    %s },\n",
			    dp->d_unit, qu(dp->d_slave));
		}
		fprintf(fp, "\t0\n};\n\n");
	}
	/*
	 * Now generate interrupt vectors for the unibus
	 */
	for (dp = dtab; dp != 0; dp = dp->d_next) {
		if (dp->d_vec != 0) {
			struct idlst *ip;
			mp = dp->d_conn;
			if (mp == 0 || mp == TO_NEXUS||can_connect(dp->d_name))
				continue;
			if( (mp->d_conn != 0 && mp->d_conn != TO_NEXUS &&
				   eq(mp->d_conn->d_name,"mba")) )
			    continue;
			if(!can_connect(mp->d_name))
			    continue;
			fprintf(fp,
			    "extern struct uba_driver %sdriver;\n",dp->d_name);
			if(ip = dp->d_vec) {
				fprintf(fp, "extern ");
				
				for (;;) {
				    fprintf(fp,"X%s%d()", ip->id,  dp->d_unit);
				    ip = ip->id_next;
				    if (ip == 0)
					break;
				    fprintf(fp, ", ");
				}
				fprintf(fp, ";\n");
				fprintf(fp, "int\t (*%sint%d[])() = { ", dp->d_name,
					 dp->d_unit);
				ip = dp->d_vec;
				for (;;) {
				    fprintf(fp, "X%s%d", ip->id, dp->d_unit);
				    ip = ip->id_next;
				    if (ip == 0)
					break;
				    fprintf(fp, ", ");
				}
				fprintf(fp, ", 0 } ;\n");
			}
		}
	}

	fprintf(fp, "\nstruct uba_ctlr ubminit[] = {\n");
	fprintf(fp, "/*driver,\tctlrname,\tctlr,\tpar,\tadpt,\tnexus,\trcntl,\tubanum,\talive,\tintr,\taddr,\taddr2,\tbuspri,\tivnum*/\n\n");
	fprintf(fpb, "\nstruct config_adpt config_adpt[] = {\n");
	dump_adapt("vba", fpb,&vba_bus);
	dump_adapt("vaxbi", fpb,&vaxbi_bus);
	dump_adapt("ci", fpb,&ci_bus);
	dump_adapt("uba", fpb,&uba_bus);
	dump_adapt("xmi", fpb,&xmi_bus);
	dump_adapt("msi", fpb,&msi_bus);
	dump_adapt("ibus", fpb,&ibus_bus);
	
	for (dp = dtab; dp != 0; dp = dp->d_next) {
		mp = dp->d_conn;
		if (dp->d_type != CONTROLLER || mp == TO_NEXUS || mp == 0
			|| dp->d_unit == QUES)
			continue;
		if(can_connect(dp->d_name))
		    continue;
		if (needs_vector(mp->d_name)) {
			if (dp->d_vec == 0) {
				printf("must specify vector for %s%d\n",
				    dp->d_name, dp->d_unit);
				fatal_error++;
				continue;
			}
			if (dp->d_addr == 0 && needs_csr(mp->d_name)) {
				printf("must specify csr address for %s%d\n",
				    dp->d_name, dp->d_unit);
				fatal_error++;
				continue;
			}
		}
		if (dp->d_drive != UNKNOWN || dp->d_slave != UNKNOWN) {
			printf("drives need their own entries; dont ");
			printf("specify drive or slave for %s%d\n",
			    dp->d_name, dp->d_unit);
			fatal_error++;
			continue;
		}
		if (dp->d_flags) {
			printf("controllers (e.g. %s%d) ",
			    dp->d_name, dp->d_unit);
			printf("don't have flags, only devices do\n");
			fatal_error++;
			continue;
		}
		
		if ((mp->d_name) && needs_pseudo_uba(mp->d_name)||
		    (eq(mp->d_name,"vaxbi")&&needs_pseudo_uba(dp->d_name)) ||
		    (eq(mp->d_name,"vba")&&needs_pseudo_uba(dp->d_name)) ||
		    (eq(mp->d_name,"xmi")&&needs_pseudo_uba(dp->d_name))){
			if (dp->d_extranum == UNKNOWN) {
				uba_n = UNKNOWN;
			} else {
				uba_n = highuba + dp->d_extranum + 1;
			}
		} else {
			uba_n = uba_num(mp);
		}
		fprintf(fp, "{ &%sdriver,\t\"%s\",\t\t%d,\t0, \t%s,",
		    dp->d_name, dp->d_name, dp->d_unit, qu(dp->d_adaptor));

		fprintf(fp,"\t%s,", qu(dp->d_nexus));
		fprintf(fp,"\t%s,", qu(dp->d_rcntl));
		fprintf(fp,"\t%s,", qu(uba_n));
		fprintf(fp,
		    "\t0,\t%sint%d, C 0%o, C 0%o,",
		    dp->d_name, dp->d_unit, dp->d_addr,dp->d_addr2);

		fprintf(fp,"\t%d,\t0x%x",dp->d_pri,dp->d_ivnum);
		fprintf(fp, "},\n\n");
		if(can_connect(mp->d_name)) {
			/* dump out a bus structure for the unmentioned
			 * parent
			 */
		    print_tree(dp->d_conn,fpb);
		    fprintf(fpb,"\t{\"%s\", %s,C &%sdriver, %d, 'C', 0, -1, -1},\n",
			mp->d_name, qu(mp->d_unit), dp->d_name, dp->d_unit);
		    
		}
	}
	fprintf(fp, "\t0\n};\n");
/* unibus devices */
	fprintf(fp, "\nstruct uba_device ubdinit[] = {\n");
	fprintf(fp,
"/* driver,      name, unit, par, adpt, nex, rcntl, ubanum, ctlr, slave,  intr,  addr,  addr2, dk, flags, buspri, ivnum*/\n\n");
	for (dp = dtab; dp != 0; dp = dp->d_next) {
		mp = dp->d_conn;
		if (dp->d_unit == QUES || dp->d_type != DEVICE || mp == 0 ||
		    mp == TO_NEXUS ||
		    (mp->d_type == MASTER || eq(mp->d_name, "mba")))
			continue;
		np = mp->d_conn;
		if (np != 0 && np != TO_NEXUS && eq(np->d_name, "mba"))
		    continue;

		np = 0;
		directconn = (can_connect(mp->d_name));
		
		if (directconn) {

			if (dp->d_vec == 0) {
				printf("must specify vector for device %s%d\n",
				    dp->d_name, dp->d_unit);
				fatal_error++;
				continue;
			}
			if (needs_csr(mp->d_name) && dp->d_addr == 0) {
				printf("must specify csr for device %s%d\n",
				    dp->d_name, dp->d_unit);
				fatal_error++;
				continue;
			}
			if (dp->d_drive != UNKNOWN || dp->d_slave != UNKNOWN) {
				printf("drives/slaves can be specified ");
				printf("only for controllers, ");
				printf("not for device %s%d\n",
				    dp->d_name, dp->d_unit);
				fatal_error++;
				continue;
			}


			if ((dp->d_conn) && needs_pseudo_uba(dp->d_conn)) {

				if (dp->d_extranum == UNKNOWN) {
					uba_n = UNKNOWN;
				} else {
					uba_n = highuba + dp->d_extranum + 1; 
				}
			} else {
				if(eq("uba",mp->d_name))
				    uba_n = mp->d_unit;
				else
				    uba_n = UNKNOWN;
			}
			slave = QUES;
		} else {
			if ((np = mp->d_conn) == 0) {
				printf("%s%d isn't connected to anything ",
				    mp->d_name, mp->d_unit);
				printf(", so %s%d is unattached\n",
				    dp->d_name, dp->d_unit);
				fatal_error++;
				continue;
			}
			if (np != TO_NEXUS) {
			    if(needs_pseudo_uba(np->d_name) ||
			      ((mp) && needs_pseudo_uba(mp->d_name)
				 )) {
				if (mp->d_extranum == UNKNOWN) {
					uba_n = QUES;
				} else {
					uba_n = highuba + mp->d_extranum + 1; 
				}
			    } else {
		 		if(mp->d_unit == QUES)
				    uba_n = QUES;
				else
				    uba_n = uba_num(np);
			    }
			} else {
			    uba_n = QUES;
			}
			if (dp->d_drive == UNKNOWN) {
				printf("must specify ``drive number'' ");
				printf("for %s%d\n", dp->d_name, dp->d_unit);
				fatal_error++;
				continue;
			}
			
			/* NOTE THAT ON THE UNIBUS ``drive'' IS STORED IN */
			/* ``SLAVE'' AND WE DON'T WANT A SLAVE SPECIFIED */
			if (dp->d_slave != UNKNOWN) {
				printf("slave numbers should be given only ");
				printf("for massbus tapes, not for %s%d\n",
				    dp->d_name, dp->d_unit);
				fatal_error++;
				continue;
			}
			if (dp->d_vec != 0) {
				printf("interrupt vectors should not be ");
				printf("given for drive %s%d\n",
				    dp->d_name, dp->d_unit);
				fatal_error++;
				continue;
			}
			if (dp->d_addr != 0) {
				printf("csr addresses should be given only ");
				printf("on controllers, not on %s%d\n",
				    dp->d_name, dp->d_unit);
				fatal_error++;
				continue;
			}
			slave = dp->d_drive;
		}
		fprintf(fp, "{ &%sdriver,\t\"%s\",  %2d,  0, %s,",
		    directconn? dp->d_name : mp->d_name, 
		    dp->d_name, dp->d_unit,
		    qu(dp->d_adaptor));

		fprintf(fp, "  %s, ",  qu(dp->d_nexus)); 
		fprintf(fp, "  %s, ",  qu(dp->d_rcntl)); 
		fprintf(fp, "  %s, ", qu(uba_n));

		fprintf(fp, "  %s, ",directconn? " -1" : qu(mp->d_unit));
		if (dp->d_drive == QUES && can_wildcard(dp->d_name) ) { 
			if ((eq(dp->d_name, "ra")) || (eq(dp->d_name, "tms")))
			    fprintf(fp, "  %s,",qumscp(slave));
			else
			    fprintf(fp, "   %s,",qu(slave));
			fprintf(fp, "  %s,  C 0%o,  C 0%o,  %d,",
			    intv(dp), dp->d_addr,dp->d_addr2,dp->d_dk);
		    }
		    else
			fprintf(fp, "  %2d,  %s,  C 0%o,  C 0%o,  %d,",
		    	    slave, intv(dp), dp->d_addr, dp->d_addr2,dp->d_dk);
		fprintf(fp, "  0x%x,",
		    dp->d_flags);

		fprintf(fp,"  %d,  0x%x", dp->d_pri, dp->d_ivnum);
		fprintf(fp, " },\n\n");
		if(can_connect(mp->d_name)&& directconn) {
			/* dump out a bus structure for the unmentioned
			 * parent
			 */
		   print_tree(dp->d_conn,fpb);
		   fprintf(fpb,"\t{\"%s\", %s,C &%sdriver, %d, 'D', 0, -1, -1},\n",
			mp->d_name, qu(mp->d_unit), dp->d_name, dp->d_unit);
		}

	}


	fprintf(fpb, "\t0\n};\n");
	fprintf(fp, "\t0\n};\n\n#include \"iobus.c\"\n\n", extrauba);

	/* dump out the error dispatch tables for each bus */
	if (ci_bus.max_bus_num > 0) dump_dispatch("ci",fp);
	if (vaxbi_bus.max_bus_num > 0) dump_dispatch("vaxbi",fp);
	if (xmi_bus.max_bus_num > 0) dump_dispatch("xmi",fp);
	if (vba_bus.max_bus_num > 0) dump_dispatch("vba",fp);     /*PAUL*/

	(void) fclose(fp);

	if (fatal_error)
		exit(1);
}

#if MACHINE_SUN
sun_ioconf()
{
	register struct device *dp, *mp;
	register int slave;
	FILE *fp;

	fp = fopen(path("ioconf.c"), "w");
	if (fp == 0) {
		perror(path("ioconf.c"));
		exit(1);
	}
	fprintf(fp, "#include \"../h/param.h\"\n");
	fprintf(fp, "#include \"../h/buf.h\"\n");
	fprintf(fp, "#include \"../h/map.h\"\n");
	fprintf(fp, "#include \"../h/vm.h\"\n");
	fprintf(fp, "\n");
	fprintf(fp, "#include \"../sundev/mbvar.h\"\n");
	fprintf(fp, "\n");
	fprintf(fp, "#define C (caddr_t)\n\n");
	fprintf(fp, "\n");
	/*
	 * Now generate interrupt vectors for the Multibus
	 */
	for (dp = dtab; dp != 0; dp = dp->d_next) {
		if (dp->d_pri != 0) {
			mp = dp->d_conn;
			if (mp == 0 || mp == TO_NEXUS ||
			    !eq(mp->d_name, "mb"))
				continue;
			fprintf(fp, "extern struct mb_driver %sdriver;\n",
			    dp->d_name);
		}
	}
	/*
	 * Now spew forth the mb_cinfo structure
	 */
	fprintf(fp, "\nstruct mb_ctlr mbcinit[] = {\n");
	fprintf(fp, "/*\t driver,\tctlr,\talive,\taddr,\tintpri */\n");
	for (dp = dtab; dp != 0; dp = dp->d_next) {
		mp = dp->d_conn;
		if (dp->d_type != CONTROLLER || mp == TO_NEXUS || mp == 0 ||
		    !eq(mp->d_name, "mb"))
			continue;
		if (dp->d_pri == 0) {
			printf("must specify priority for %s%d\n",
			    dp->d_name, dp->d_unit);
			continue;
		}
		if (dp->d_addr == 0) {
			printf("must specify csr address for %s%d\n",
			    dp->d_name, dp->d_unit);
			continue;
		}
		if (dp->d_drive != UNKNOWN || dp->d_slave != UNKNOWN) {
			printf("drives need their own entries; ");
			printf("dont specify drive or slave for %s%d\n",
			    dp->d_name, dp->d_unit);
			continue;
		}
		if (dp->d_flags) {
			printf("controllers (e.g. %s%d) don't have flags, ");
			printf("only devices do\n",
			    dp->d_name, dp->d_unit);
			continue;
		}
		fprintf(fp, "\t{ &%sdriver,\t%d,\t0,\tC 0x%x,\t%d },\n",
		    dp->d_name, dp->d_unit, dp->d_addr, dp->d_pri);
	}
	fprintf(fp, "\t0\n};\n");
	/*
	 * Now we go for the mb_device stuff
	 */
	fprintf(fp, "\nstruct mb_device mbdinit[] = {\n");
	fprintf(fp,
"\t/* driver,  unit, ctlr,  slave,   addr,    pri,    dk, flags*/\n");
	for (dp = dtab; dp != 0; dp = dp->d_next) {
		mp = dp->d_conn;
		if (dp->d_unit == QUES || dp->d_type != DEVICE || mp == 0 ||
		    mp == TO_NEXUS || mp->d_type == MASTER ||
		    eq(mp->d_name, "mba"))
			continue;
		if (eq(mp->d_name, "mb")) {
			if (dp->d_pri == 0) {
				printf("must specify vector for device %s%d\n",
				    dp->d_name, dp->d_unit);
				continue;
			}
			if (dp->d_addr == 0) {
				printf("must specify csr for device %s%d\n",
				    dp->d_name, dp->d_unit);
				continue;
			}
			if (dp->d_drive != UNKNOWN || dp->d_slave != UNKNOWN) {
				printf("drives/slaves can be specified only ");
				printf("for controllers, not for device %s%d\n",
				    dp->d_name, dp->d_unit);
				continue;
			}
			slave = QUES;
		} else {
			if (mp->d_conn == 0) {
				printf("%s%d isn't connected to anything, ",
				    mp->d_name, mp->d_unit);
				printf("so %s%d is unattached\n",
				    dp->d_name, dp->d_unit);
				continue;
			}
			if (dp->d_drive == UNKNOWN) {
				printf("must specify ``drive number'' for %s%d\n",
				   dp->d_name, dp->d_unit);
				continue;
			}
			/* NOTE THAT ON THE UNIBUS ``drive'' IS STORED IN */
			/* ``SLAVE'' AND WE DON'T WANT A SLAVE SPECIFIED */
			if (dp->d_slave != UNKNOWN) {
				printf("slave numbers should be given only ");
				printf("for massbus tapes, not for %s%d\n",
				    dp->d_name, dp->d_unit);
				continue;
			}
			if (dp->d_pri != 0) {
				printf("interrupt priority should not be ");
				printf("given for drive %s%d\n",
				    dp->d_name, dp->d_unit);
				continue;
			}
			if (dp->d_addr != 0) {
				printf("csr addresses should be given only");
				printf("on controllers, not on %s%d\n",
				    dp->d_name, dp->d_unit);
				continue;
			}
			slave = dp->d_drive;
		}
		fprintf(fp,
"\t{ &%sdriver,  %2d,   %s,    %2d,   C 0x%x, %d,  %d,  0x%x },\n",
		    eq(mp->d_name, "mb") ? dp->d_name : mp->d_name, dp->d_unit,
		    eq(mp->d_name, "mb") ? " -1" : qu(mp->d_unit),
		    slave, dp->d_addr, dp->d_pri, dp->d_dk, dp->d_flags);
	}
	fprintf(fp, "\t0\n};\n");
	(void) fclose(fp);
}
#endif

char *intv(dev)
	register struct device *dev;
{
	static char buf[20];
	register struct device *mp = dev->d_conn;

	if (dev->d_vec == 0)
		return ("     0");
	return (sprintf(buf, "%sint%d", dev->d_name, dev->d_unit));
}

char *
qu(num)
{

	if (num == QUES)
		return ("'?'");
	if (num == UNKNOWN)
		return (" -1");
	return (sprintf(errbuf, "%3d", num));
}

/*
 * MSCP devices represent wildcards in the kernel via " -1" instead of '?'.
 * This is necessary because the '?' is actually a 63 which conflicts with
 * a valid unit number of 63.
 */
char *
qumscp(num)
{

	if ((num == QUES) || (num == UNKNOWN))
		return ("-1");
	return (sprintf(errbuf, "%3d", num));
}

char  *tbl_can_connect[] = { "uba","mba","vaxbi","vba","kdb","klesib","klesiu",
			     "aio","aie", "uda", "ci", "msi","ibus", "xmi", 
			     "kdm", 0};

/*
 * can things connect to this name?
 */
can_connect(name)
register	char *name;
{
    register	char	**ptr = tbl_can_connect;
    
    while(*ptr)
	{
	    if(eq(*ptr,name))return(1);
	    ptr++;
	}
    return(0);
}


char  *tbl_can_wildcard[] = { "ra","tms" };

/*
 * does this device type allow wildcarded slaves
 */
can_wildcard(name)
register	char *name;
{
    register	char	**ptr = tbl_can_wildcard;
    
    while(*ptr)
	{
	    if(eq(*ptr,name))return(1);
	    ptr++;
	}
    return(0);
}

char *tbl_needs_vector[] = {  "uba", "vaxbi","vba","aio","aie", "kdb", "klesib",
			      "uda", "klesiu", "klesiq", "xmi", 
			      "kdm", 0  };
/*
 * needs_vector(name) things connect to this device need to supply a vector
 */
needs_vector(name)
register char *name;
{
    register char **ptr = tbl_needs_vector;
    
    while(*ptr)
	{
	    if(eq(*ptr,name))return(1);
	    ptr++;
	}
    return(0);
}

char *tbl_needs_csr[] = { "uba", "uda", 0};


/*
 * needs_csr(name)
 * things connect to this device need to specify a csr.
 */
needs_csr(name)
register char *name;
{
    register char **ptr = tbl_needs_csr;
    
    while(*ptr)
	{
	    if(eq(*ptr,name))return(1);
	    ptr++;
	}
    return(0);
}

/*
 * print_tree(dp, fpb)
 *
 */
print_tree(dp, fpb)
register  struct device *dp;
register  FILE *fpb;
{
    register	struct device *mp = dp->d_conn;
    
    if(mp == TO_NEXUS || eq(dp->d_name,"uba") || eq(dp->d_name,"ci") 
	|| eq(dp->d_name,"msi") || mp == 0)
	return;
    
    print_tree(dp->d_conn,fpb);
    
    fprintf(fpb,"\t{\"%s\", %s, C \"%s\", %d, 'A', 0, -1, -1},\n",
	 mp->d_name, qu(mp->d_unit), dp->d_name, dp->d_unit);
}

/*
 *dump out a adapter line for the named device.
 */
dump_adapt(str,fpb,bus_info)
register	char	*str;
register	FILE	*fpb;
struct bus_info *bus_info;
{
    register	struct device *dp, *mp;
   
    bus_info->max_bus_num = 0;
    bus_info->cnt = 0;

    for (dp = dtab; dp != 0; dp = dp->d_next) {
	if(eq(dp->d_name,str)) 
	    {
		print_tree(dp,fpb);
		
		if ((mp = dp->d_conn) == TO_NEXUS) 
		{
			fprintf(fpb,"\t{\"%s\", '%s', C \"%s\", %d, 'A', 0, -1, -1},\n",
				"nexus", "?", dp->d_name, dp->d_unit);

			if ((dp->d_unit+1) > bus_info->max_bus_num) {
				 bus_info->max_bus_num = dp->d_unit+1;
			}
			if (dp->d_unit != -1) bus_info->cnt++;
			
		} else {
		    fprintf(fpb,"\t{\"%s\", %s, C \"%s\", %d, 'A', 0, -1, -1},\n",
			  mp->d_name, qu(mp->d_unit), dp->d_name, dp->d_unit);
		}
		
			    
		
	    }
    }
 
}

/*
 *dump out vector for each name device "str" which is connected to nexus 
 */
dump_vec_adapt(str,fpb)
register	char	*str;
register	FILE	*fpb;
{
    register	struct device *dp, *mp;
   
    for (dp = dtab; dp != 0; dp = dp->d_next) {
	if(eq(dp->d_name,str))  {
	    if ((mp = dp->d_conn) == TO_NEXUS && (dp->d_unit != -1)) {

		switch (machine) {
		    case MACHINE_VAX:
			if(eq(dp->d_name,"ci")) {
				fprintf(fpb, "\t.globl\t_X%sint%d\n\t.align\t2\n_X%sint%d:\n\tpushr\t$0x3f\n", str,dp->d_unit, str,dp->d_unit);
	    			fprintf(fpb,"\tpushl\t_ci_isr+((%d*8)+4)\n",dp->d_unit);
				fprintf(fpb, "\tcalls\t$1,*_ci_isr+(%d*8)\n",dp->d_unit);
				fprintf(fpb, "#if defined(VAX750) || defined(VAX730) || defined(VAX8200) || defined(MVAX) || defined(VAX420) || defined(VAX3600)  || defined(VAX60) || defined(VAX8800) || defined(VAX6200) || defined(VAX6400)\n");
				fprintf(fpb,"\tmfpr\t$1,r0\n");
				fprintf(fpb,"\tincl\tCPU_INTR(r0)\n");
				fprintf(fpb,"#endif\n\tpopr\t$0x3f\n\trei\n\n");
			} else {
				fprintf(fpb, "\t.globl\t_X%serr%d\n\t.align\t2\n_X%serr%d:\n\tpushr\t$0x3f\n",
					    str,dp->d_unit, str,dp->d_unit);
	    			fprintf(fpb,"\tpushal\t6*4(sp)\n\tpushl\t$%d\n",dp->d_unit);
				fprintf(fpb, "\tcalls\t$2,_%serrors\n\tpopr\t$0x3f\n",str);
				fprintf(fpb, "\trei\n\n");			
			}
			break;
		    case MACHINE_MIPS:
			if(eq(dp->d_name,"ci")) {
			    if(dp->d_unit == 0) {
				fprintf(fpb, "#include \"../h/types.h\"\n");
				fprintf(fpb, "#include \"../io/ci/ciadapter.h\"\n");
			    }
			    fprintf(fpb,"X%sint%d(stray_arg)\n",str,dp->d_unit);
			    fprintf(fpb,"int stray_arg;\n");
			    fprintf(fpb,"{\n");
			    fprintf(fpb,"/* stray_arg is not used here but locore calls it with\n");
			    fprintf(fpb," * an argument that is the offset into the scb data structure;\n");
			    fprintf(fpb," * which the stray interrupt routine uses to find where the\n");
			    fprintf(fpb," * stray came from.  The unused arg keeps everything consistent.\n");
			    fprintf(fpb," */\n");
			    fprintf(fpb,"\textern CIISR ci_isr[];\n");
			    fprintf(fpb,"\t(*ci_isr[%d].isr)(ci_isr[%d].pccb);\n",dp->d_unit,dp->d_unit);
			    fprintf(fpb,"}\n");
			} else {
			    fprintf(fpb,"X%serr%d(stray_arg)\n",str,dp->d_unit);
			    fprintf(fpb,"int stray_arg;\n");
			    fprintf(fpb,"{\n");
			    fprintf(fpb,"/* stray_arg is not used here but locore calls it with\n");
			    fprintf(fpb," * an argument that is the offset into the scb data structure;\n");
			    fprintf(fpb," * which the stray interrupt routine uses to find where the\n");
			    fprintf(fpb," * stray came from.  The unused arg keeps everything consistent.\n");
			    fprintf(fpb," */\n");
			    fprintf(fpb,"\textern %serrors();\n",str);
			    fprintf(fpb,"\t%serrors(%d);\n",str,dp->d_unit);
			    fprintf(fpb,"}\n");
			}
			break;
		} /* End Switch */
	    }		
	}
    } /* End for loop */
 
}
dump_dispatch(str,fpb)
register	char	*str;
register	FILE	*fpb;
{
    register	struct device *dp, *mp;
   
    for (dp = dtab; dp != 0; dp = dp->d_next) {
	if(eq(dp->d_name,str))  {
	    if ((mp = dp->d_conn) == TO_NEXUS && (dp->d_unit != -1)) {
    		if( eq(str, "ci") ) {
		        fprintf(fpb,"extern int X%sint%d();\n",str,dp->d_unit);
    		} else {
			fprintf(fpb,"extern int X%serr%d();\n",str,dp->d_unit);
		}
	    }		
	}
    }
    if( eq(str, "ci") ) {
	fprintf(fpb,"int\t(*%sintv[])() = {\n",str );
    } else {
        fprintf(fpb,"\nstruct bus_dispatch %serr_dispatch[] = {\n",str);
    }
    for (dp = dtab; dp != 0; dp = dp->d_next) {
	if(eq(dp->d_name,str))  {
	    if ((mp = dp->d_conn) == TO_NEXUS && (dp->d_unit != -1)) {
    		if( eq(str, "ci") ) {
		    fprintf(fpb,"\t X%sint%d ,\n",str,dp->d_unit);
    		} else {
		    fprintf(fpb,"\t{0x%x,(int)X%serr%d},\n",
				dp->d_unit,str,dp->d_unit);
		}
	    }		
	}
    }
    if( eq(str, "ci") ) {
        fprintf(fpb,"\t 0 };\n\n");
    } else {
        fprintf(fpb,"\t{-1,-1} };\n");
    }
}

uba_num(dp)
struct device *dp;
{
	if(eq(dp->d_name ,"uba")) return(dp->d_unit);
	if(dp->d_conn == TO_NEXUS || dp->d_conn == 0) return(UNKNOWN);
	return(uba_num(dp->d_conn));
}




/* this code was used by the MIPS CO. machine and we do not support the VME
 * bus.  The code is here just in case we need to reff. it someday.
 */

#if MACHINE_MIPS

mips_ioconf()
{
	register struct device *dp, *mp, *np;
	register int slave;
	FILE *fp;
	char buf1[64], buf2[64];
	char *concat3();

	unlink(path("ioconf.c"));
	fp = fopen(path("ioconf.c"), "w");
	if (fp == 0) {
		perror(path("ioconf.c"));
		exit(1);
	}
	fprintf(fp, "#include \"../machine/pte.h\"\n");
	fprintf(fp, "#include \"../h/param.h\"\n");
	fprintf(fp, "#include \"../h/buf.h\"\n");
	fprintf(fp, "#include \"../h/map.h\"\n");
	fprintf(fp, "#include \"../h/vm.h\"\n");
        fprintf(fp, "#include \"../h/config.h\"\n");
        fprintf(fp, "\n");
        fprintf(fp, "#include \"../io/mba/vax/mbavar.h\"\n");
	fprintf(fp, "\n");
#ifdef notdef
	if (seen_mbii && seen_vme) {
		printf("can't have both vme and mbii devices\n");
		exit(1);
	}
	if (seen_mbii)
		fprintf(fp, "#include \"../mipsmbii/mbiivar.h\"\n");
	if (seen_vme)
		fprintf(fp, "#include \"../mipsvme/vmevar.h\"\n");
	fprintf(fp, "\n");
#endif notdef
	fprintf(fp, "#define C	(caddr_t)\n");
	fprintf(fp, "#define NULL	0\n\n");
	fprintf(fp, "int nulldev();\n");
#ifdef notdef
	if (!seen_mbii)
		goto checkvme;
	/*
	 * MBII stuff should go here
	 */

checkvme:
	if (!seen_vme)
		goto closefile;
	/*
	 * Now generate interrupt vectors for the vme bus
	 */
	for (dp = dtab; dp != 0; dp = dp->d_next) {
		if (dp->d_vec != 0) {
			struct idlst *ip;
			mp = dp->d_conn;
			if (mp == 0 || mp == TO_NEXUS || !eq(mp->d_name, "vme"))
				continue;
			if (is_declared(dp->d_name))
				continue;
			declare(dp->d_name);
			fprintf(fp, "extern struct vme_driver %sdriver;\n",
			    dp->d_name);
			fprintf(fp, "extern ");
			ip = dp->d_vec;
			for (;;) {
				fprintf(fp, "%s()", ip->id);
				ip = ip->id_next;
				if (ip == 0)
					break;
				fprintf(fp, ", ");
			}
			fprintf(fp, ";\n");
			fprintf(fp, "int (*_%sint[])() = { ", dp->d_name,
			    dp->d_unit);
			ip = dp->d_vec;
			for (;;) {
				fprintf(fp, "%s", ip->id);
				ip = ip->id_next;
				if (ip == 0)
					break;
				fprintf(fp, ", ");
			}
			fprintf(fp, ", 0 } ;\n\n");
		}
	}
	fprintf(fp, "\nstruct vme_ctlr vmminit[] = {\n");
	fprintf(fp,
"  /*          driver  ctlr alive        intr          addr    am */\n");
	for (dp = dtab; dp != 0; dp = dp->d_next) {
		mp = dp->d_conn;
		if (dp->d_type != CONTROLLER || mp == TO_NEXUS || mp == 0 ||
		    !eq(mp->d_name, "vme"))
			continue;
		if (dp->d_vec == 0) {
			printf("must specify vector for %s%d\n",
			    dp->d_name, dp->d_unit);
			continue;
		}
		if (dp->d_addr == 0) {
			printf("must specify csr address for %s%d\n",
			    dp->d_name, dp->d_unit);
			continue;
		}
		if (dp->d_addrmod == 0) {
			printf("must specify address modifier for %s%d\n",
			    dp->d_name, dp->d_unit);
			continue;
		}
		if (dp->d_drive != UNKNOWN || dp->d_slave != UNKNOWN) {
			printf("drives need their own entries; dont ");
			printf("specify drive or slave for %s%d\n",
			    dp->d_name, dp->d_unit);
			continue;
		}
		if (dp->d_flags) {
			printf("controllers (e.g. %s%d) ",
			    dp->d_name, dp->d_unit);
			printf("don't have flags, only devices do\n");
			continue;
		}
		fprintf(fp,
"  {   %14s, %3d,    0, %11s, C 0x%08x, 0x%02x },\n",
		     concat3(buf1, "&", dp->d_name, "driver"),
		     dp->d_unit,
		     concat3(buf2, "_", dp->d_name, "int"),
		     dp->d_addr,
		     dp->d_addrmod);
	}
	fprintf(fp, "  {             NULL }\n};\n");
	/*
	 * vme devices
	 */
	fprintf(fp, "\nstruct vme_device vmdinit[] = {\n");
	fprintf(fp,
"/*       driver  unit ctlr slave      intr          addr    am dk       flags */\n"
	);
	for (dp = dtab; dp != 0; dp = dp->d_next) {
		mp = dp->d_conn;
		if (dp->d_unit == QUES || dp->d_type != DEVICE || mp == 0 ||
		    mp == TO_NEXUS || mp->d_type == MASTER)
			continue;
		for (np = mp; np && np != TO_NEXUS; np = np->d_conn)
			if (eq(np->d_name, "vme"))
				break;
		if (np != 0 && np != TO_NEXUS && !eq(np->d_name, "vme"))
			continue;
		np = 0;
		if (eq(mp->d_name, "vme")) {
			if (dp->d_vec == 0) {
				printf("must specify vector for device %s%d\n",
				    dp->d_name, dp->d_unit);
				continue;
			}
			if (dp->d_addr == 0) {
				printf("must specify csr for device %s%d\n",
				    dp->d_name, dp->d_unit);
				continue;
			}
			if (dp->d_addrmod == 0) {
				printf(
			"must specify address modifier for device %s%d\n",
				    dp->d_name, dp->d_unit);
				continue;
			}
			if (dp->d_drive != UNKNOWN || dp->d_slave != UNKNOWN) {
				printf("drives/slaves can be specified ");
				printf("only for controllers, ");
				printf("not for device %s%d\n",
				    dp->d_name, dp->d_unit);
				continue;
			}
			slave = QUES;
		} else {
			if ((np = mp->d_conn) == 0) {
				printf("%s%d isn't connected to anything ",
				    mp->d_name, mp->d_unit);
				printf(", so %s%d is unattached\n",
				    dp->d_name, dp->d_unit);
				continue;
			}
			if (dp->d_drive == UNKNOWN) {
				printf("must specify ``drive number'' ");
				printf("for %s%d\n", dp->d_name, dp->d_unit);
				continue;
			}
			if (dp->d_slave != UNKNOWN) {
				printf("slave numbers should be given only ");
				printf("for massbus tapes, not for %s%d\n",
				    dp->d_name, dp->d_unit);
				continue;
			}
			if (dp->d_vec != 0) {
				printf("interrupt vectors should not be ");
				printf("given for drive %s%d\n",
				    dp->d_name, dp->d_unit);
				continue;
			}
			if (dp->d_addr != 0) {
				printf("csr addresses should be given only ");
				printf("on controllers, not on %s%d\n",
				    dp->d_name, dp->d_unit);
				continue;
			}
			if (dp->d_addrmod != 0) {
				printf("address modifiers should be given only ");
				printf("on controllers, not on %s%d\n",
				    dp->d_name, dp->d_unit);
				continue;
			}
			slave = dp->d_drive;
		}
		fprintf(fp,
"{%14s, %3d, %3s, %4d,%10s, C 0x%08x, 0x%02x, %1d, 0x%08x },\n",
		    concat3(buf1, "&",
		        eq(mp->d_name, "vme") ? dp->d_name : mp->d_name,
			"driver"),
		    dp->d_unit,
		    eq(mp->d_name, "vme") ? "-1" : qu(mp->d_unit),
		    slave,
		    intv2(dp),
		    dp->d_addr,
		    dp->d_addrmod,
		    dp->d_dk,
		    dp->d_flags);
	}
	fprintf(fp, "{          NULL }\n};\n");
#endif notdef
closefile:
	(void) fclose(fp);
	return;
}

char *
intv2(dev)
register struct device *dev;
{
	static char buf[20];

	if (dev->d_vec == 0)
		return ("NULL");
	return (sprintf(buf, "_%sint", dev->d_name));
}

char *
concat3(buf, p1, p2, p3)
char *p1, *p2, *p3;
{
	return(sprintf(buf, "%s%s%s", p1, p2, p3));
}

#define	MAXDEVS	100
#define	DEVLEN	10
char decl_devices[MAXDEVS][DEVLEN];

declare(cp)
register char *cp;
{
	register i;

	for (i = 0; i < MAXDEVS; i++)
		if (decl_devices[i][0] == 0) {
			strncpy(decl_devices, cp, DEVLEN);
			return;
		}
	printf("device table full, fix mkioconf.c\n");
	exit(1);
}

is_declared(cp)
register char *cp;
{
	register i;

	for (i = 0; i < MAXDEVS; i++) {
		if (decl_devices[i][0] == 0)
			return(0);
		if (strncmp(decl_devices[i], cp, DEVLEN) == 0)
			return(1);
	}
	return(0);
}
#endif MACHINE_MIPS

is_cpu_declared(cp)
register char *cp;
{
        struct cputype *cpup;

	for (cpup = cputype; cpup; cpup = cpup->cpu_next)
	        if (strcmp(cpup->cpu_name, cp) == 0)
		        return(1);
	return(0);
}
