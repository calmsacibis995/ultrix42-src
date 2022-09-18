

%union {
	char	*str;
	int	val;
	struct	file_list *file;
	struct	idlst *lst;
	double	fval;
        struct  shad_device *shad_list        
}

%token	AND
%token	ANY
%token	ARGS
%token	AT
%token	BUS
%token	COMMA
%token	CONFIG
%token	CONTROLLER
%token	CPU
%token	CSR
%token	DEVICE
%token	DISK
%token  DMMIN
%token  DMMAX
%token	DRIVE
%token	DST
%token	DUMPS
%token	EQUALS
%token	FLAGS
%token	HZ
%token	IDENT
%token	MACHINE
%token	MAJOR
%token	MAKEOPTIONS
%token	MASTER
%token  MAXUPRC
%token	MAXUSERS
%token	MINOR
%token	MINUS
%token	MSCP
%token	NEXUS
%token	ON
%token	OPTIONS
%token	PHYSMEM
%token	PRIORITY
%token	PROCESSORS
%token	PSEUDO_DEVICE
%token	RELEASE
%token	ROOT
%token	REMOTE_CONTROLLER
%token	SCSID
%token	SEMICOLON
%token	SIZE
%token	SLAVE
%token	SMMAX
%token	SMMIN
%token	SMBRK
%token	SMSEG
%token	SMSMAT
%token	MAXTSEG
%token	MAXUVA
%token	SWAP
%token	SHADOW
%token	TIMEZONE
%token	TRACE
%token	VECTOR
%token	VERSION
%token	BUFCACHE
%token	MAXDSIZE
%token	MAXSSIZE
%token	SWAPFRAG
%token	VASSLOP
%token	MAXRETRY

%token	<str>	INTERCONNECT
%token	<str>	ID
%token	<val>	NUMBER
%token	<fval>	FPNUMBER

%type	<str>	Save_id
%type	<str>	Opt_value
%type	<str>	Dev
%type	<lst>	Id_list
%type	<val>	optional_size
%type	<str>	device_name
%type	<val>	major_minor
%type	<val>	root_device_spec
%type	<val>	dump_device_spec
%type	<file>	swap_device_spec
%type	<shad_list>	Shad_dev_name
%type	<shad_list>	Shad_const

%{

#ifndef lint
static	char	*sccsid = "@(#)yconfig.y	4.4	(ULTRIX)	9/10/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1985,86,87,88 by			*
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
 *
 *   Modification history:
 *
 * 30-Sep-90    skc
 *      Added support for shadow device.
 * 
 * 03-Aug-90	rafiey (Ali Rafieymehr)
 *	Added VAX9000 to CPUs needing emulation code.
 *
 * 01-May-90	Joe Szczypek
 *	Added equiv for DS65x0 ==> DS64x0
 *
 * 20-Dec-89    Paul Grist
 *      Added VMEbus support - vba adapters. The syntax changes involve
 *      allowing a second CSR and a NUMBER after the vector list 
 *      (Id_list) to be allowed. The PRIORITY syntax will also be
 *      used by vba support. Currently no error checking will be made
 *      to ensure the second CSR is "2" or that the PRIORITY is in the
 *      VMEbus range of 1-7. This was done to leave things flexible
 *      and generic for future support.
 *
 * 17-Oct-89	Randall Brown
 *	Added equiv for DS2100 ==> DS3100
 *
 * 12-Jun-89	Gopal
 *	Added support for virtual memeoy configurable parameters.
 *	(like vasslop, maxretry, maxdsiz, maxssiz and swapfrag).
 *	Also added a case "swap on none".
 *
 * 06-Sep-88	robin
 *	Removed "equiv"s for Mayfair: 3550, 3652, 3650 => 3600
 *
 * 02-Oct-88	woodward
 *	Fix for 6220 to include emulation code properly.
 *
 * 13-Jun-88	chet
 *	Added config file option BUFCACHE (percent of memory to allocate
 *	for the buffer cache)
 *
 * 14-Apr-88	Robin
 *	Added code to allow the ibus adapter
 *
 * 25-Mar-88 -- darrell
 *	Added VAX60 (Firefox) to CPUs needing emulation code.
 *
 * 15-Feb-88	Fred Canter
 *	Add VAX420 (CVAXstar/PVAX) or CPUs needing emulation code.
 *
 *  1-25-88	Ricky Palmer
 *	Added MSI support for Mayfair II.
 *
 * 30-Nov-87 -- robin
 *	Added "equiv"s for Mayfair: 3300, 3400, 3550, 3652, 3650 => 3600
 *
 * 12-11-87	Robin L. and Larry C.
 *	Added portclass support to the system.
 *
 * 24-Aug-87 -- afd
 *	Added "equiv"s for Mayfair: 3200, 3500, 3602 => 3600
 *
 * 19-May-87 -- afd
 *	equiv VAX3000 to VAX3600
 *
 * 20-Apr-87 -- afd
 *	Changed name CVAXQ to VAX3600
 *
 * 09-Mar-87 -- afd
 *	If cpu CVAXQ is defined set "emulation_instr" flag
 *	(as is done for MVAX).
 *
 * 15-Apr-86 -- afd
 *	If machine type is 'mvax' call it vax and give a warning.
 *	If cpu MVAX is defined set "emulation_instr" flag.
 *
 * 8 Apr 86  -- lp
 * 	Added bvp support
 *
 * 01 Apr 86 -- depp
 *	Added in shared memory configurable items
 *
 * 05-Mar-86 -- jrs
 *	Added code to allow configuring of devices on the bi.
 *
 * 25-Feb-86 -- jrs
 *	Changed to allow multiple "needs" files per files.* line
 *	Changed to support "swap on boot" similar to "swap on generic"
 *
 * 05-Feb-86 -- jrs
 *	Move cpu aliasing to parsing as we need info earlier than thought
 *
 * 22-Sep-85 -- jrs
 *	Fix bug in timezone vs. dst specifications
 *
 *  4 June 85 -- depp
 *	Added new config specs "release" and "version".
 *
 */

/*	config.y	1.18	83/05/18	*/

/* NOTE: the lambda comments appear to represent the null
 *       case in the config syntax that follows. I was
 *       going to change the comments, but did not want
 *       to disturb the convention used by the original
 *       code.
 */

#include "config.h"
#include <ctype.h>
#include <stdio.h>

struct cpequiv {
	char *cp_alias;
	char *cp_equiv;
} cpequiv[] = {
	{ "VAX725", "VAX730" },
	{ "VAX785", "VAX780" },
	{ "VAX8650", "VAX8600" },
	{ "VAX8350", "VAX8200" },
	{ "VAX8300", "VAX8200" },
	{ "VAX8250", "VAX8200" },
	{ "VAX8000", "VAX8200" },
	{ "VAX8840", "VAX8800" },
	{ "VAX8830", "VAX8800" },
	{ "VAX8820", "VAX8800" },
	{ "VAX8810", "VAX8800" },
	{ "VAX8700", "VAX8800" },
	{ "VAX8550", "VAX8800" },
	{ "VAX8530", "VAX8800" },
	{ "VAX8500", "VAX8800" },
	{ "VAX3000", "VAX3600" },
	{ "VAX3200", "VAX3600" },
	{ "VAX3300", "VAX3600" },
	{ "VAX3400", "VAX3600" },
	{ "VAX3500", "VAX3600" },
	{ "VAX3602", "VAX3600" },
	{ "VAX3800", "VAX3600" },
	{ "VAX3900", "VAX3600" },
	{ "VAX6210", "VAX6200" },
	{ "VAX6220", "VAX6200" },
	{ "VAX6230", "VAX6200" },
	{ "VAX6240", "VAX6200" },
	{ "VAX6250", "VAX6200" },
	{ "VAX6260", "VAX6200" },
	{ "VAX6270", "VAX6200" },
	{ "VAX6280", "VAX6200" },
	{ "VAX6300", "VAX6200" },
	{ "VAX6310", "VAX6200" },
	{ "VAX6320", "VAX6200" },
	{ "VAX6330", "VAX6200" },
	{ "VAX6340", "VAX6200" },
	{ "VAX6350", "VAX6200" },
	{ "VAX6360", "VAX6200" },
	{ "VAX6370", "VAX6200" },
	{ "VAX6380", "VAX6200" },
	{ "VAX6400", "VAX6400" },
	{ "VAX6410", "VAX6400" },
	{ "VAX6420", "VAX6400" },
	{ "VAX6430", "VAX6400" },
	{ "VAX6440", "VAX6400" },
	{ "VAX6450", "VAX6400" },
	{ "VAX6460", "VAX6400" },
	{ "VAX6470", "VAX6400" },
	{ "VAX6480", "VAX6400" },
	{ "VAX6500", "VAX6400" },
	{ "VAX6510", "VAX6400" },
	{ "VAX6520", "VAX6400" },
	{ "VAX6530", "VAX6400" },
	{ "VAX6540", "VAX6400" },
	{ "VAX6550", "VAX6400" },
	{ "VAX6560", "VAX6400" },
	{ "VAX6570", "VAX6400" },
	{ "VAX6580", "VAX6400" },
	{ "DS2100",  "DS3100"  },
	{ NULL, NULL }
	};

struct	device cur;
struct	device *curp = 0;
struct  shad_device *cur_shadp = 0;
char	*temp_id;
char	*val_id;
char	*malloc();

int first_mscp = 1;
struct device psuedo_controller;

%}
%%
Configuration:
	Many_specs
		= {
		   verifysystemspecs();
		  }
		;

Many_specs:
	Many_specs Spec
		|
	/* lambda */
		;

Spec:
	Device_spec SEMICOLON
	      = { newdev(&cur); } |
	Config_spec SEMICOLON
		|
	TRACE SEMICOLON
	      = { do_trace = !do_trace; } |
	SEMICOLON
		|
	error SEMICOLON
		;

Config_spec:
	MACHINE Save_id
	    = {
		if (!strcmp($2, "vax")) {
			machine = MACHINE_VAX;
			machinename = "vax";
			upmachinename = "VAX";
		} else if (!strcmp($2, "mips")) {
			machine = MACHINE_MIPS;
			machinename = "mips";
			upmachinename = "MIPS";
		} else if (!strcmp($2, "sun")) {
			machine = MACHINE_SUN;
			machinename = "sun";
			upmachinename = "SUN";
		} else if (!strcmp($2, "mvax")) {
			machine = MACHINE_VAX;
			machinename = "vax";
			upmachinename = "VAX";
			yyerror("Machine type 'mvax' obsolete; using 'vax'");
		} else
			yyerror("Unknown machine type");
	      } |
	CPU Save_id
	      = {
		struct cpequiv *cpe;
		struct cputype *cp =
		    (struct cputype *)malloc(sizeof (struct cputype));
		cp->cpu_name = ns($2);
		cp->cpu_next = cputype;
		cputype = cp;
		free(temp_id);
		init_dev(&cur);
		cur.d_name=ns($2);
		cur.d_type=CPU;
		for (cpe = cpequiv; cpe->cp_alias != NULL; cpe++) {
			if (strcmp(cp->cpu_name, cpe->cp_alias) == 0) {
				cp = (struct cputype *)malloc(sizeof (struct cputype));
				cp->cpu_name = cpe->cp_equiv;
				cp->cpu_next = cputype;
				cputype = cp;
				init_dev(&cur);
				newdev(&cur);
				cur.d_name = cpe->cp_equiv;
				cur.d_type = CPU;
				break;
			}
		}
		newdev(&cur);
		if (	(eq(cp->cpu_name, "MVAX")) ||
			(eq(cp->cpu_name, "VAX60")) ||
			(eq(cp->cpu_name, "VAX420")) ||
			(eq(cp->cpu_name, "VAX3600")) ||
			(eq(cp->cpu_name, "VAX6200")) ||
			(eq(cp->cpu_name, "VAX6400")) ||
			(eq(cp->cpu_name, "VAX9000")))
		{
			emulation_instr = 1;
		}
	      } |
	VERSION NUMBER
	      = {
		version = $2;
		} |
	RELEASE FPNUMBER
	      = {
		release = $2;
		} |
	OPTIONS Opt_list
		|
	MAKEOPTIONS Mkopt_list
		|
	IDENT ID
	      = { ident = ns($2); } |
	System_spec
		|
	HZ NUMBER
	      = { yyerror("HZ specification obsolete; delete"); } |
	TIMEZONE NUMBER
	      = { timezone = 60 * $2; check_tz(); } |
	TIMEZONE NUMBER DST NUMBER
	      = { timezone = 60 * $2; dst = $4; check_tz(); } |
	TIMEZONE NUMBER DST
	      = { timezone = 60 * $2; dst = 1; check_tz(); } |
	TIMEZONE FPNUMBER
	      = { timezone = (int) (60 * $2 + 0.5); check_tz(); } |
	TIMEZONE FPNUMBER DST NUMBER
	      = { timezone = (int) (60 * $2 + 0.5); dst = $4; 
		  check_tz(); } |
	TIMEZONE FPNUMBER DST
	      = { timezone = (int) (60 * $2 + 0.5); dst = 1; 
		  check_tz(); } |
	TIMEZONE MINUS NUMBER
	      = { timezone = -60 * $3; check_tz(); } |
	TIMEZONE MINUS NUMBER DST NUMBER
	      = { timezone = -60 * $3; dst = $5; check_tz(); } |
	TIMEZONE MINUS NUMBER DST
	      = { timezone = -60 * $3; dst = 1; check_tz(); } |
	TIMEZONE MINUS FPNUMBER
	      = { timezone = -((int) (60 * $3 + 0.5)); check_tz(); } |
	TIMEZONE MINUS FPNUMBER DST NUMBER
	      = { timezone = -((int) (60 * $3 + 0.5)); dst = $5; 
		  check_tz(); } |
	TIMEZONE MINUS FPNUMBER DST
	      = { timezone = -((int) (60 * $3 + 0.5)); dst = 1; 
		  check_tz(); } |
	MAXUSERS NUMBER
	      = { maxusers = $2; } |
	MAXUPRC NUMBER
	      = { maxuprc = $2;} |
	DMMIN NUMBER
	      = { dmmin = $2;} |
	DMMAX NUMBER
	      = { dmmax = $2;} |
	PHYSMEM NUMBER
	      = { physmem = $2;} |
	BUFCACHE NUMBER
	      = { bufcache = $2;} |
	SMMAX NUMBER
	      = { smmax = $2;} |
	SMMIN NUMBER
	      = { smmin = $2;} |
	SMBRK NUMBER
	      = { smbrk = $2;} |
	SMSEG NUMBER
	      = { smseg = $2;} |
	SMSMAT NUMBER
	      = { smsmat = $2;} |
	MAXTSEG NUMBER
	      = { maxtsiz = $2;} |
	MAXUVA NUMBER
	      = { maxuva = $2;} |
	MAXDSIZE NUMBER
	      = { maxdsiz = $2;} |
	MAXSSIZE NUMBER
	      = { maxssiz = $2;} |
	SWAPFRAG NUMBER
	      = { swapfrag = $2;} |
	VASSLOP NUMBER
	      = { vasslop = $2;} |
	MAXRETRY NUMBER
	      = { maxretry = $2;} |
	SCSID NUMBER NUMBER
	      = { scs_system_id.hos = $2; scs_system_id.lol = $3;} |
	SCSID NUMBER 
	      = { scs_system_id.hos = 0; scs_system_id.lol = $2;} |
	PROCESSORS NUMBER
	      = { processors = $2;};

System_spec:
	  System_id System_parameter_list
		= { checksystemspec(*confp); }
	;
		
System_id:
	  CONFIG Save_id
		= { mkconf($2); }
	;

System_parameter_list:
	  System_parameter_list System_parameter
	| System_parameter
	;

System_parameter:
	  swap_spec
	| root_spec
	| dump_spec
	;
	
swap_spec:
	  SWAP optional_on swap_device_list
	;
	
swap_device_list:
	  swap_device_list AND swap_device
	| swap_device
	;
	
swap_device:
	  swap_device_spec optional_size
	      = { mkswap(*confp, $1, $2); }
	;

swap_device_spec:
	  device_name
		= {
			struct file_list *fl = newswap();

			if (eq($1, "generic") || eq($1, "boot") || eq($1, "none"))
				fl->f_fn = $1;
			else {
				fl->f_swapdev = nametodev($1, 0, 'b');
				fl->f_fn = devtoname(fl->f_swapdev);
			}
			$$ = fl;
		}
	| major_minor
		= {
			struct file_list *fl = newswap();

			fl->f_swapdev = $1;
			fl->f_fn = devtoname($1);
			$$ = fl;
		}
	;

root_spec:
	  ROOT optional_on root_device_spec
		= {
			struct file_list *fl = *confp;

			if (fl && fl->f_rootdev != NODEV)
				yyerror("extraneous root device specification");
			else
				fl->f_rootdev = $3;
		}
	;

root_device_spec:
	  device_name
		= { $$ = nametodev($1, 0, 'a'); }
	| major_minor
	;

dump_spec:
	  DUMPS optional_on dump_device_spec
		= {
			struct file_list *fl = *confp;

			if (fl && fl->f_dumpdev != NODEV)
				yyerror("extraneous dump device specification");
			else
				fl->f_dumpdev = $3;
		}

	;

dump_device_spec:
	  device_name
		= { $$ = nametodev($1, 0, 'b'); }
	| major_minor
	;

major_minor:
	  MAJOR NUMBER MINOR NUMBER
		= { $$ = makedev($2, $4); }
	;

optional_on:
	  ON
	| /* empty */
	;

optional_size:
	  SIZE NUMBER
	      = { $$ = $2; }
	| /* empty */
	      = { $$ = 0; }
	;

device_name:
	  Save_id
		= { $$ = $1; }
	| Save_id NUMBER
		= {
			char buf[80];

			(void) sprintf(buf, "%s%d", $1, $2);
			$$ = ns(buf); free($1);
		}
	| Save_id NUMBER ID
		= {
			char buf[80];

			(void) sprintf(buf, "%s%d%s", $1, $2, $3);
			$$ = ns(buf); free($1);
		}
	;

Opt_list:
	Opt_list COMMA Option
		|
	Option
		;

Option:
	Save_id
	      = {
		struct opt *op = (struct opt *)malloc(sizeof (struct opt));
		op->op_name = ns($1);
		op->op_next = opt;
		op->op_value = 0;
		opt = op;
		free(temp_id);
	      } |
	Save_id EQUALS Opt_value
	      = {
		struct opt *op = (struct opt *)malloc(sizeof (struct opt));
		op->op_name = ns($1);
		op->op_next = opt;
		op->op_value = ns($3);
		opt = op;
		free(temp_id);
		free(val_id);
	      } ;

Opt_value:
	ID
	      = { $$ = val_id = ns($1); } |
	NUMBER
	      = { char nb[16]; $$ = val_id = ns(sprintf(nb, "%d", $1)); };


Save_id:
	ID
	      = { $$ = temp_id = ns($1); }
	;

Mkopt_list:
	Mkopt_list COMMA Mkoption
		|
	Mkoption
		;

Mkoption:
	Save_id EQUALS Opt_value
	      = {
		struct opt *op = (struct opt *)malloc(sizeof (struct opt));
		op->op_name = ns($1);
		op->op_next = mkopt;
		op->op_value = ns($3);
		mkopt = op;
		free(temp_id);
		if (val_id)
			free(val_id);
	      } ;

Dev:
	INTERCONNECT
	      = { $$ = ns($1); } |
	ID
	      = { $$ = ns($1); }
	;

Device_spec:
	DEVICE Dev_name Dev_info Int_spec
	      = { cur.d_type = DEVICE; } |
	MASTER Dev_name Dev_info Int_spec
	      = { cur.d_type = MASTER; } |
	DISK Dev_name Dev_info Int_spec
	      = { cur.d_dk = 1; cur.d_type = DEVICE; } |
	CONTROLLER Dev_name Dev_info Int_spec
	      = { cur.d_type = CONTROLLER; } |
	PSEUDO_DEVICE Init_dev Dev
	      = {
		cur.d_name = $3;
		cur.d_type = PSEUDO_DEVICE;
		} |
	PSEUDO_DEVICE Init_dev Dev NUMBER
	      = {
		cur.d_name = $3;
		cur.d_type = PSEUDO_DEVICE;
		cur.d_slave = $4;
		} |
	BUS Init_dev Dev
	      = {
		cur.d_name = $3;
		cur.d_type = BUS;
		} |
	BUS Init_dev Dev NUMBER
	      = {
		cur.d_name = $3;
		cur.d_type = BUS;
		cur.d_slave = $4;
		} |
        SHADOW Shad_dev_name ON Shad_const_list  
        ;

Dev_name:
	Init_dev Dev NUMBER
	      = {
		cur.d_name = $2;
		if (eq($2, "mba"))
			seen_mba = 1;
		else if (eq($2, "uba")) {
			seen_uba = 1;
			if ($3 > highuba) {
				highuba = $3;
			}
		}
		cur.d_unit = $3;
		};

Init_dev:
	/* lambda */
	      = { init_dev(&cur); };

Dev_info:
	Con_info Info_list
		|
	/* lambda */
		;

Con_info:
	AT Dev NUMBER
	      = {
		if (must_nexus(cur.d_name))
			yyerror(sprintf(errbuf,
			    "You can not connect a %s to a %s", cur.d_name,$2));
		if ((cur.d_conn = connect($2, $3)) != 0) {
			cur.d_adaptor = cur.d_conn ->d_adaptor;
			cur.d_nexus = cur.d_conn ->d_nexus;
			cur.d_extranum = cur.d_conn->d_extranum;
			if(needs_csr(cur.d_conn->d_name)) {
			    cur.d_addr = cur.d_conn->d_addr;
			    cur.d_conn->d_addr = 0;
			}
			if(needs_vector(cur.d_conn->d_name)){
			    cur.d_vec = cur.d_conn->d_vec;
			    cur.d_conn->d_vec = 0;
			}
		}
		} |
		AT MSCP = {
			/* create a mscp pseudo if it doesn't exist */
			/* and link this one to it */
		    if (first_mscp) {
			struct device tempdev;

		    	init_dev(&tempdev);
	       		tempdev.d_type = CONTROLLER;
			tempdev.d_name = ns("mscp");
			tempdev.d_unit = 0;
			psuedo_controller.d_name = ns("mscp");
		        tempdev.d_conn = &psuedo_controller;
		    	newdev(&tempdev);
			first_mscp = 0;

		    }
		    cur.d_conn = connect("mscp", 0);
		    cur.d_drive = QUES;
		}|
		
	AT NEXUS NUMBER
	      = { check_nexus(&cur, $3); cur.d_conn = TO_NEXUS;
		cur.d_adaptor = cur.d_unit;
		} |
	AT Dev NUMBER NEXUS NUMBER
	      = { if(!(eq($2, "vaxbi") || eq($2, "xmi")||eq($2,"vba")|| eq($2, "ci") || eq($2, "msi")))
			yyerror(sprintf(errbuf,
			 "You can't specify a %s at a(n) %s with a node or nexus number",
			 cur.d_name,$2));
/* hack */
			cur.d_conn = connect($2,$3);
			cur.d_adaptor = $3;
			cur.d_nexus = $5;
			if(needs_pseudo_uba(cur.d_name)) {
			    cur.d_extranum = extrauba++;
			}
		} |
	AT Dev NUMBER REMOTE_CONTROLLER NUMBER
	     =	{
			cur.d_conn = connect($2,$3);
			cur.d_adaptor = $3;
			cur.d_rcntl = $5;
	      	};
		
Info_list:
	Info_list Info
		|
	/* lambda */
		;

Info:
	CSR NUMBER
	      = {     cur.d_addr = $2; } | 
	CSR NUMBER CSR NUMBER NUMBER
	      = { cur.d_addr = $2;
		  cur.d_addr2 = $5;    } |
	DRIVE NUMBER
	      = { cur.d_drive = $2;    } |
	SLAVE NUMBER
	      = {
		if (cur.d_conn != 0 && cur.d_conn != TO_NEXUS &&
		     cur.d_conn->d_type == MASTER)
			cur.d_slave = $2;
		else
			yyerror("can't specify slave--not to master");
		} |
	FLAGS NUMBER
	      = { cur.d_flags = $2; };

Int_spec:
	VECTOR Id_list
	      = { cur.d_vec = $2;   } |
	VECTOR Id_list NUMBER
	      = { cur.d_vec = $2;
		  cur.d_ivnum = $3; } |
	PRIORITY NUMBER 
	      = { cur.d_pri = $2;   } |
	PRIORITY NUMBER VECTOR Id_list
	      = { cur.d_pri = $2;
		  cur.d_vec = $4;   } |
	PRIORITY NUMBER VECTOR Id_list NUMBER
	      = { cur.d_pri = $2;
		  cur.d_vec = $4;
		  cur.d_ivnum = $5; } |
	/* lambda */
		;

Id_list:
	Save_id
	      = {
		struct idlst *a = (struct idlst *)malloc(sizeof(struct idlst));
		a->id = $1; a->id_next = 0; $$ = a;
		} |
	Save_id Id_list =
		{
		struct idlst *a = (struct idlst *)malloc(sizeof(struct idlst));
	        a->id = $1; a->id_next = $2; $$ = a;
		};

Shad_dev_name:
	device_name
              = {
                mk_shaddev($1);
                }
        ;

Shad_const_list:
	Shad_const_list AND Shad_const
      | Shad_const 
        ;

Shad_const:
	device_name
              = { 
                add_shaddev($1); 
                }
	;   

%%

yyerror(s)
	char *s;
{

	fprintf(stderr, "config: line %d: %s\n", yyline, s);
}

/*
 * return the passed string in a new space
 */
char *
ns(str)
	register char *str;
{
	register char *cp;

	cp = malloc((unsigned)(strlen(str)+1));
	(void) strcpy(cp, str);
	return (cp);
}

/*
 * add a device to the list of devices
 */
newdev(dp)
	register struct device *dp;
{
	register struct device *np;

	np = (struct device *) malloc(sizeof *np);
	*np = *dp;
	if (curp == 0)
		dtab = np;
	else
		curp->d_next = np;
	curp = np;
}

/*
 * note that a configuration should be made
 */
mkconf(sysname)
	char *sysname;
{
	register struct file_list *fl, **flp;
	int i;

	fl = (struct file_list *) malloc(sizeof *fl);
	fl->f_type = SYSTEMSPEC;
	fl->f_needs[0] = sysname;
	for (i = 1; i < NNEEDS; i++) {
		fl->f_needs[i] = 0;
	}
	fl->f_rootdev = NODEV;
	fl->f_dumpdev = NODEV;
	fl->f_fn = 0;
	fl->f_next = 0;
	for (flp = confp; *flp; flp = &(*flp)->f_next)
		;
	*flp = fl;
	confp = flp;
}

struct file_list *
newswap()
{
	struct file_list *fl = (struct file_list *)malloc(sizeof (*fl));
	int i;

	fl->f_type = SWAPSPEC;
	fl->f_next = 0;
	fl->f_swapdev = NODEV;
	fl->f_swapsize = 0;
	for (i = 0; i < NNEEDS; i++) {
		fl->f_needs[i] = 0;
	}
	fl->f_fn = 0;
	return (fl);
}

/*
 * Add a swap device to the system's configuration
 */
mkswap(system, fl, size)
	struct file_list *system, *fl;
	int size;
{
	register struct file_list **flp;
	char *cp, name[80];

	if (system == 0 || system->f_type != SYSTEMSPEC) {
		yyerror("\"swap\" spec precedes \"config\" specification");
		return;
	}
	if (size < 0) {
		yyerror("illegal swap partition size");
		return;
	}
	/*
	 * Append swap description to the end of the list.
	 */
	flp = &system->f_next;
	for (; *flp && (*flp)->f_type == SWAPSPEC; flp = &(*flp)->f_next)
		;
	fl->f_next = *flp;
	*flp = fl;
	/*
	 * If first swap device for this system,
	 * set up f_fn field to insure swap
	 * files are created with unique names.
	 */
	if (system->f_fn)
		return;
	if (eq(fl->f_fn, "generic") || eq(fl->f_fn, "boot") || eq(fl->f_fn, "none"))
		system->f_fn = ns(fl->f_fn);
	else
		system->f_fn = ns(system->f_needs[0]);
}

/*
 * find the pointer to connect to the given device and number.
 * returns 0 if no such device and prints an error message
 */
struct device *
connect(dev, num)
	register char *dev;
	register int num;
{
	register struct device *dp;
	struct device *huhcon();

	if (num == QUES)
		return (huhcon(dev));
	for (dp = dtab; dp != 0; dp = dp->d_next) {
		if ((num != dp->d_unit) || !eq(dev, dp->d_name))
			continue;
		if (dp->d_type != CONTROLLER && dp->d_type != MASTER){
			yyerror(sprintf(errbuf,
			    "%s connected to non-controller", dev));
			return (0);
		}
		return (dp);
	}
	yyerror(sprintf(errbuf, "%s %d not defined", dev, num));
	return (0);
}

/*
 * connect to an unspecific thing
 */
struct device *
huhcon(dev)
	register char *dev;
{
	register struct device *dp, *dcp;
	struct device rdev;
	int oldtype;

	/*
	 * First make certain that there are some of these to wildcard on
	 */
	for (dp = dtab; dp != 0; dp = dp->d_next)
		if (eq(dp->d_name, dev))
			break;
	if (dp == 0) {
		yyerror(sprintf(errbuf, "no %s's to wildcard", dev));
		return (0);
	}
	oldtype = dp->d_type;
	dcp = dp->d_conn;
	/*
	 * Now see if there is already a wildcard entry for this device
	 * (e.g. Search for a "uba ?")
	 */
	if(dp->d_unit != -1)
	for (; dp != 0; dp = dp->d_next)
		if (eq(dev, dp->d_name) && dp->d_unit == -1)
			break;
	/*
	 * If there isn't, make one because everything needs to be connected
	 * to something.
	 */
	if (dp == 0) {
		dp = &rdev;
		init_dev(dp);
		dp->d_unit = QUES;
		dp->d_adaptor = dp->d_nexus = QUES;
		dp->d_name = ns(dev);
		dp->d_type = oldtype;
		dp->d_next = 0;
		newdev(dp);
		dp = curp;
		/*
		 * Connect it to the same thing that other similar things are
		 * connected to, but make sure it is a wildcard unit
		 * (e.g. up connected to sc ?, here we make connect sc? to a
		 * uba?).  If other things like this are on the NEXUS or
		 * if they aren't connected to anything, then make the same
		 * connection, else call ourself to connect to another
		 * unspecific device.
		 */
		if (dcp == TO_NEXUS || dcp == 0)
			dp->d_conn = dcp;
		else
			dp->d_conn = connect(dcp->d_name, QUES);
	}
	return (dp);
}

init_dev(dp)
	register struct device *dp;
{

	dp->d_name = "OHNO!!!";
	dp->d_type = DEVICE;
	dp->d_conn = 0;
	dp->d_vec = 0;
	dp->d_addr = dp->d_addr2 = dp->d_ivnum = 0;
        dp->d_pri = dp->d_flags = dp->d_dk = 0;
	dp->d_slave = dp->d_drive = dp->d_unit = UNKNOWN;
	dp->d_adaptor = dp->d_nexus = dp->d_extranum = UNKNOWN;
}

/*
 * make certain that this is a reasonable type of thing to connect to a nexus
 */
check_nexus(dev, num)
	register struct device *dev;
	int num;
{

	switch (machine) {

	case MACHINE_VAX:
	    	if(!can_nexus(dev->d_name))
			yyerror("only vba's, uba's, mba's, xmi's, and vaxbi's should be connected to the nexus");
		if (num != QUES)
			yyerror("can't give specific nexus numbers");
		break;

	case MACHINE_SUN:
		if (!eq(dev->d_name, "mb"))
			yyerror("only mb's should be connected to the nexus");
		break;
	}
}

/*
 * Check the timezone to make certain it is sensible
 */

check_tz()
{
	if (abs(timezone) > 12 * 60)
		yyerror("timezone is unreasonable");
	else
		hadtz = 1;
}

/*
 * Check system specification and apply defaulting
 * rules on root, dump, and swap devices.
 */
checksystemspec(fl)
	register struct file_list *fl;
{
	char buf[BUFSIZ];
	register struct file_list *swap;
	int generic;

	if (fl == 0 || fl->f_type != SYSTEMSPEC) {
		yyerror("internal error, bad system specification");
		exit(1);
	}
	swap = fl->f_next;
	generic = swap && swap->f_type == SWAPSPEC &&
			(eq(swap->f_fn, "generic") || eq(swap->f_fn, "boot"));
	if (fl->f_rootdev == NODEV && !generic) {
		yyerror("no root device specified");
		exit(1);
	}
	/*
	 * Default swap area to be in 'b' partition of root's
	 * device.  If root specified to be other than on 'a'
	 * partition, give warning, something probably amiss.
	 */
	if (swap == 0 || swap->f_type != SWAPSPEC) {
		dev_t dev;

		swap = newswap();
		dev = fl->f_rootdev;
		if (minor(dev) & 07) {
			sprintf(buf, 
"Warning, swap defaulted to 'b' partition with root on '%c' partition",
				(minor(dev) & 07) + 'a');
			yyerror(buf);
		}
		swap->f_swapdev =
		   makedev(major(dev), (minor(dev) &~ 07) | ('b' - 'a'));
		swap->f_fn = devtoname(swap->f_swapdev);
		mkswap(fl, swap, 0);
	}
	/*
	 * Make sure a generic swap isn't specified, along with
	 * other stuff (user must really be confused).
	 */
	if (generic) {
		if (fl->f_rootdev != NODEV) {
			sprintf(buf, "root device specified with %s swap",
					fl->f_fn);
			yyerror(buf);
		}
		if (fl->f_dumpdev != NODEV) {
			sprintf(buf, "dump device specified with %s swap",
					fl->f_fn);
			yyerror(buf);
		}
		return;
	}
	/*
	 * Default dump device and warn if place is not a
	 * swap area
	 */
	if (fl->f_dumpdev == NODEV)
		fl->f_dumpdev = swap->f_swapdev;
	if (fl->f_dumpdev != swap->f_swapdev) {
		struct file_list *p = swap->f_next;

		for (; p && p->f_type == SWAPSPEC; p = p->f_next)
			if (fl->f_dumpdev == p->f_swapdev)
				return;
		sprintf(buf, "Warning, orphaned dump device, %s",
			"do you know what you're doing");
		yyerror(buf);
	}
}

/*
 * Verify all devices specified in the system specification
 * are present in the device specifications.
 */
verifysystemspecs()
{
	register struct file_list *fl;
	dev_t checked[50], *verifyswap();
	register dev_t *pchecked = checked;

	for (fl = conf_list; fl; fl = fl->f_next) {
		if (fl->f_type != SYSTEMSPEC)
			continue;
		if (!finddev(fl->f_rootdev))
			deverror(fl->f_needs[0], "root");
		*pchecked++ = fl->f_rootdev;
		pchecked = verifyswap(fl->f_next, checked, pchecked);
#define	samedev(dev1, dev2) \
	((minor(dev1) &~ 07) != (minor(dev2) &~ 07))
		if (!alreadychecked(fl->f_dumpdev, checked, pchecked)) {
			if (!finddev(fl->f_dumpdev))
				deverror(fl->f_needs[0], "dump");
			*pchecked++ = fl->f_dumpdev;
		}
	}
}

/*
 * Do as above, but for swap devices.
 */
dev_t *
verifyswap(fl, checked, pchecked)
	register struct file_list *fl;
	dev_t checked[];
	register dev_t *pchecked;
{

	for (;fl && fl->f_type == SWAPSPEC; fl = fl->f_next) {
		if (eq(fl->f_fn, "generic") || eq(fl->f_fn, "boot")||
			eq(fl->f_fn, "none"))
			continue;
		if (alreadychecked(fl->f_swapdev, checked, pchecked))
			continue;
		if (!finddev(fl->f_swapdev))
			fprintf(stderr,
			   "config: swap device %s not configured", fl->f_fn);
		*pchecked++ = fl->f_swapdev;
	}
	return (pchecked);
}

/*
 * Has a device already been checked
 * for it's existence in the configuration?
 */
alreadychecked(dev, list, last)
	dev_t dev, list[];
	register dev_t *last;
{
	register dev_t *p;

	for (p = list; p < last; p++)
		if (samedev(*p, dev))
			return (1);
	return (0);
}

deverror(systemname, devtype)
	char *systemname, *devtype;
{

	fprintf(stderr, "config: %s: %s device not configured\n",
		systemname, devtype);
}

/*
 * Look for the device in the list of
 * configured hardware devices.  Must
 * take into account stuff wildcarded.
 */
finddev(dev)
	dev_t dev;
{

	/* punt on this right now */
	return (1);
}

char *tbl_pseudo_uba[] = { "kdb", "kdm", "klesib", 0 } ;
 
/*
 * look up this device in a table to see if it needs a pseudo_uba.
 */
needs_pseudo_uba(str)
register	char	*str;
{
	register	char	**ptr = tbl_pseudo_uba;

	while(*ptr) if(!strcmp(str,*ptr++)) return(1);
	
	return(0);
}

char *tbl_must_nexus[] = { "mba", "vaxbi", "ci",  "xmi", "ibus", "vba", 0 };

/*
 * See if the device must be connected to a nexus
 */
must_nexus(str)
register	char	*str;
{
	register	char	**ptr = tbl_must_nexus;

	while(*ptr) if(!strcmp(str,*ptr++)) return(1);
	
	return(0);
}

char *tbl_can_nexus[] = { "uba", "mba", "vaxbi", "ci", "xmi", "msi","ibus", "vba", 0 };


/*
 * See if the device can be connected to a nexus
 */
can_nexus(str)
register	char	*str;
{
	register	char	**ptr = tbl_can_nexus;

	while(*ptr) if(!strcmp(str,*ptr++)) return(1);
	
	return(0);
}


/*
 * If a shadow device is used for storing the root file system, we
 * create a configuration data structure that will indicate the shadow
 * device and the associated physical devices.
 */
mk_shaddev(name)
register char *name;
{
    register struct shad_device *sp;

    sp = (struct shad_device *) malloc(sizeof *sp);

    sp->next_shad = NULL;
    sp->shad_devt = shadnametodev(name, 1, 'a'); 
    sp->num_of_const = 0;

    if ( cur_shadp == 0 ) {
        shad_tabp = sp;        
    }
    else {
        cur_shadp->next_shad = sp;
    }

    cur_shadp = sp;
}

/*
 * This function records the additional constituents of the shadow set.
 */
add_shaddev(name)
register char *name;
{
    register int i;

    i = cur_shadp->num_of_const;
    cur_shadp->constituents[i] = nametodev(name, 1, 'a');
    cur_shadp->num_of_const++;

}







