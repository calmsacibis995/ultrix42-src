/*
 *	@(#)config.h	4.4	(ULTRIX)	9/10/90
 */

/************************************************************************
 *									*
 *			Copyright (c) 1983,86,87,88 by			*
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
 *		Modification History
 *
 * 06-Sep-90    skc
 *      Added shadow data structure shad_device.
 *
 * 20-Aug-90	Matt Thomas
 *	Have config emit header files for psuedo-devices that are in
 *	the config file but had no file entries that matched them.  Also
 *	try to include pseudo-device specific "files.%s" file for each
 *	pseudo-device.  (this is for layered that don't want to have
 *	to edit the files file distributed by ULTRIX)
 *      
 * 20-Dec-89    Paul Grist
 *      Added support for the VMEbus - vba adapters. This included
 *      the addition of two new fields to the device structure, the
 *      second csr address, and the first interrupt vector number.
 *      Declared vba_bus a bus_info structure.
 * 
 * 12-Jun-89	G Gopal
 *	Added support for dynamic swap configurable parameters.
 *	(vasslop, maxdsiz, maxssiz, maxretry and swapfrag)
 *
 * 24-Feb-89	Mark Parenti
 *	Remove xos remnants.
 * 29-Mar-89    Tim Burke
 *      Allow for more than 32 "ra" type mscp disks.
 *
 * 24-Feb-89	Mark Parenti
 *	Remove xos remnants.
 *
 * 13-Jun-88	chet
 *	Added support for configurable buffer cache
 *
 * 04-14-88	Robin L.
 *	Added support for ibus
 *
 *  1-25-88	Ricky Palmer
 *	Added msi_bus struct declaration for MSI support. 
 *
 * 12-11-87	Robin L. and Larry C.
 *	Added portclass support to the system.
 *
 * 20-Apr-87 -- afd
 *	Changed name CVAXQ to VAX3600
 *
 * 16-Apr-87 -- fred (Fred Canter)
 *	Added xos variable (set if Xos configured).
 *
 * 14-Apr-87 -- prs
 *	Added the structure definition of file_sys. The new structure
 *	is a linked list of entries in the filesystems file.
 *
 * 09-Mar-87 -- afd
 *	Modified comment on "emulation_instr" variable to reflect that
 *	it gets set for CVAXQ (as well as MVAX).
 *	
 * 15-Apr-86 -- afd
 *	Took out define for MACHINE_MVAX (mvax is now considered 'vax').
 *	Declare "emulation_instr" for vaxen with subset instruction set.
 *
 * 02 Apr 86 -- depp
 *	Added shared memory configurable items 
 *
 * 05-Mar-86 -- jrs
 *	Added support for configuring bi devices.
 *
 * 25-Feb-86 -- jrs
 *	Changed to allow multiple "needs" files per files.* line
 *
 *  4 June 85 -- depp
 *	Added the variables version and release
 *
 ***********************************************************************/
/*
 * Config.
 */
/* @(#)config.h	1.5		8/14/84 */

#include <sys/types.h>

#define	NODEV	((dev_t)-1)
#define	NNEEDS	10

struct file_list {
	struct	file_list *f_next;	
	char	*f_fn;			/* the name */
	short	f_type;			/* see below */
	short	f_flags;		/* see below */
	short	f_special;		/* requires special make rule */
	char	*f_needs[NNEEDS];
	/*
	 * Random values:
	 *	swap space parameters for swap areas
	 *	root device, etc. for system specifications
	 */
	union {
		struct {		/* when swap specification */
			dev_t	fuw_swapdev;
			int	fuw_swapsize;
		} fuw;
		struct {		/* when system specification */
			dev_t	fus_rootdev;
			dev_t	fus_dumpdev;
		} fus;
	} fun;
#define	f_swapdev	fun.fuw.fuw_swapdev
#define	f_swapsize	fun.fuw.fuw_swapsize
#define	f_rootdev	fun.fus.fus_rootdev
#define	f_dumpdev	fun.fus.fus_dumpdev
};

/*
 * Types.
 */
#define DRIVER		1
#define NORMAL		2
#define	PROFILING	3
#define	SYSTEMSPEC	4
#define	SWAPSPEC	5

/*
 * MSCP disk specs.
 * There are 8 major numbers used to represent mscp disk devices.
 * Each major number represents 32 disk units.
 * The block device major munbers are in the range MSCP_MIN to MSCP_MAX.
 */
#define MSCP_MIN 	23
#define MSCP_MAX 	30
#define MSCP_MAJORS 8
#define UNITS_PER_MAJOR 32
#define MSCP_MAXDISK ((MSCP_MAJORS * UNITS_PER_MAJOR) - 1)
#define NONMSCP_MAXDISK 31

/*
 * Attributes (flags).
 */
#define	CONFIGDEP	1
#define	INVISIBLE	2
#define	NOTBINARY	8
#define	UNSUPPORTED	0x10
#define	OBJS_ONLY	0x20

struct	idlst {
	char	*id;
	struct	idlst *id_next;
};

struct device {
	int	d_type;			/* CONTROLLER, DEVICE, UBA or MBA */
	struct	device *d_conn;		/* what it is connected to */
	char	*d_name;		/* name of device (e.g. rk11) */
	struct	idlst *d_vec;		/* interrupt vectors */
	int	d_pri;			/* interrupt priority */
	int	d_addr;			/* address of csr */
	int     d_addr2;                /* address of csr2 */ 
	int     d_ivnum;                /* first interrupt vector */ 
	int	d_unit;			/* unit number */
	int	d_drive;		/* drive number */
	int	d_slave;		/* slave number */
#define QUES	-1	/* -1 means '?' */
#define	UNKNOWN -2	/* -2 means not set yet */
	int	d_rcntl;		/* remote controller number */
	int	d_dk;			/* if init 1 set to number for iostat */
	int	d_flags;		/* nlags for device init */
	int	d_adaptor;		/* which bus adaptor we are on */
	int	d_nexus;		/* which nexus on this adaptor */
	int	d_extranum;		/* which hidden uba we are on */
	int	d_counted;		/* has header been written? */
	struct	device *d_next;		/* Next one in list */
};
#define TO_NEXUS	(struct device *)-1

struct config {
	char	*c_dev;
	char	*s_sysname;
};

/*
 * Config has a global notion of which machine type is
 * being used.  It uses the name of the machine in choosing
 * files and directories.  Thus if the name of the machine is ``vax'',
 * it will build from ``makefile.vax'' and use ``../vax/asm.sed''
 * in the makerules, etc.
 */
int	machine;
char	*machinename;
char	*upmachinename;
#define	MACHINE_VAX	1
#define	MACHINE_SUN	2
#define	MACHINE_MIPS	3

/*
 * For each machine, a set of CPU's may be specified as supported.
 * These and the options (below) are put in the C flags in the makefile.
 */
struct cputype {
	char	*cpu_name;
	struct	cputype *cpu_next;
} *cputype;

/*
 * A set of options may also be specified which are like CPU types,
 * but which may also specify values for the options.
 */
struct opt {
	char	*op_name;
	char	*op_value;
	struct	opt *op_next;
} *opt, *mkopt;

/*
 * The file_sys structure is to verify that at least one of the file
 * systems listed in the filesystems file is specified.
 */
struct file_sys {
	char *fs_name;
	struct file_sys *fs_next;
} *file_sys;

char	*ident;
char	*ns();
char	*tc();
char	*qu();
char	*get_word();
char	*path();
char	*raise();

int	do_trace;

char	*index();
char	*rindex();
char	*malloc();
char	*strcpy();
char	*strcat();
char	*sprintf();

#if MACHINE_VAX
int	seen_mba, seen_uba;
#endif

struct	device *connect();
struct	device *dtab;
dev_t	nametodev();
char	*devtoname();

char	errbuf[80];
int	yyline;

struct	file_list *ftab, *conf_list, **confp;
char	*PREFIX;

int	timezone, hadtz;
int	dst;
int	profiling;

int	maxusers;
int	physmem; /* estimate of megabytes of physical memory */
int	bufcache; /* percent of memory to allocate for buffer cache */
int	maxuprc; /* max number of processes per user */
int	processors; /* max number of processors in the system */
int	dmmin;	 /* min chunk for virtual memory allocation */
int	dmmax;	 /* max chunk for virtual memory allocation */
double	release; /* Ultrix release number */
int	version; /* Version of that release */
int	highuba; /* highest uba number seen during config file parsing */
int	extrauba; /* number of extra unibuses we need to define */
int	smmin;	 /* minumum shared memory segment size */
int	smmax;	 /* maximum shared memory segment size */
int	smbrk;	 /* number of VAX pages between end of data segment and
		    beginning of first shared memory segment */
int	smseg;	/* number of shared memory segments per process */
int	smsmat; /* highest attachable shared memory address */
int	maxtsiz; /* max size of text segment */
int	maxdsiz; /* max size of data segment*/
int 	maxssiz; /* max size of stack segment */
int 	swapfrag; /* max swap fragment size (in clicks)*/
int	vasslop;  /* slop for recovering when we run out of swap space (in clicks*/
int	maxretry; /* maximum retry count to try KM_ALLOC in dynamic swap */
int	maxuva; /* max aggrate user page table size */
int	emulation_instr; /* true if cpu MVAX or VAX3600 defined in config file */
struct  _scs_system_id {        /* SCS system identification number          */
    u_long      lol;            /*  Low order long                           */
    u_short     hos;            /*  High order short                         */
    } scs_system_id; 
u_long	scsid_l; /* low order four bytes of scsid */
u_short	scsid_h; /* high order two bytes of scsid */

/*
 * Shadow device list
 */
#define SHAD_MAX_CONST 	8
struct shad_device {
    struct shad_device *next_shad;
    dev_t  shad_devt;
    int    num_of_const;
    dev_t  constituents[SHAD_MAX_CONST];
};    

struct shad_device *shad_tabp;

#ifdef vax
#define eq(a,b)	(!strcmp(a,b))
#endif vax
#ifdef mips
extern int eq();
#endif mips

extern	int	source;
extern	char	*tbl_pseudo_uba[];
extern	char	*tbl_must_nexus[];
extern	char	*tbl_is_uq[];

struct bus_info {
int max_bus_num;
int cnt;
};

struct bus_info vaxbi_bus;
struct bus_info xmi_bus;
struct bus_info ci_bus;
struct bus_info msi_bus;
struct bus_info uba_bus;
struct bus_info ibus_bus;
struct bus_info vba_bus;

extern int kdebug;
