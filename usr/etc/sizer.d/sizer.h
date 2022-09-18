/*	@(#)sizer.h	4.5	(ULTRIX)	8/13/90 */
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
 * Name: sizer.h
 *
 * Modification History
 *
 * Aug 10, 1990 - Randall Brown
 *	Added include file <sys/devio.h>
 *
 * Dec 12, 1989 - Alan Frechette
 *	Added include file <sys/sysinfo.h>. Added new field
 *	"alive_unit" to alive device structure.
 *
 * July 14, 1989 - Alan Frechette
 *	Added define for XMINODE.
 *
 * Jun 19, 1989 - Darrell Dunnuck (darrell)
 *	Fixed include of cpuconf.h -- had given Alan wrong info.
 * 
 * May 10, 1989 - Alan Frechette
 *	Added a new name list element "NL_roottype" for determining 
 *	network boots.
 *
 * May 02, 1989 - Alan Frechette
 *	Changes to deal with new unique "cpu" handling for both
 *	vax and mips architectures. 
 *
 * Feb 12, 1989 - Alan Frechette
 *	New sizer code which supports multiple architectures.
 *      This file is a complete redesign and rewrite of the 
 *	original V3.0 sizer code by Tungning Cherng.
 *
 ***********************************************************************/

#include <stdio.h>
#include <signal.h>
#include <nlist.h>
#include <time.h>
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <sys/param.h>
#include <sys/vm.h>
#include <sys/dir.h>
#include <sys/stat.h>
#include <sys/buf.h>
#include <sys/config.h>
#include <sys/devio.h>
#include <io/uba/ubavar.h>	
#include <io/mba/vax/mbavar.h>
#include <machine/cpuconf.h>
#include <machine/param.h>
#ifdef vax
#include <machine/rpb.h>
#include <machine/sas/vmb.h>
#endif vax

/************************************************************************/
/*									*/
/* The following declarations define the CONFIGURATION FILE DEFINITION  */
/* TYPES, the CONFIGURATION FILE TABLE STRUCTURES, an the ALIVE DEVICE  */
/* LIST STRUCTURE.  						        */
/*									*/
/************************************************************************/

/* The CONFIGURATION FILE device definition types */
#define UNKNOWN	       -1
#define	ADAPTER		0
#define	CONTROLLER	1
#define	MASTER		2
#define	DISK		3
#define	TAPE		4
#define	DEVICE		5

/* The CONFIGURATION FILE node definition types */
#define VAXBINODE	0
#define CINODE		1
#define MSINODE		2
#define XMINODE		3

/* The CONFIGURARTION FILE bus definition types */
#define UNIBUS		0
#define MASSBUS		1


/* Structure for configuring the NODE information. */
struct config_node {
    char *conn_name;		/* The connection point name */
    char *nodename;		/* The associated node name  */
    int nodetype;		/* The associated node type  */
};

/* Structure for configuring the CPU information. */
struct config_cpu {
    int cputype;		/* The type of the system CPU	*/
    char *cpuname;		/* The name of the system CPU  	*/
    int maxusers;		/* Maximum # of users allowed	*/
    char *bootdev;		/* The name of the boot device  */
};

/* Structure for configuring the DEVICE information. */
struct config_device {
    char *devname;		/* The name of the I/O device	*/
    int devtype;		/* The type of the I/O device	*/
    int makedevflag;		/* Make device special file flag*/
				/* (1-MAKEDEV)	(0-NO MAKEDEV)	*/
    int supportedflag;		/* Supported I/O device flag	*/
				/* (1-SUPPORTED) (0-UNSUPPORTED)*/
    char *ivectors;		/* The interrupt vectors 	*/
    int flags;			/* Flags field of I/O device	*/
};

/* Structure for defining the configuration file hardware name. */
struct hardware {
	char	*typename;	/* The hardware device name */
};

/* Structure for defining the FLOATING ADDRESS DEVICES information. */
struct float_devices {
	char 	*name;		/* Floating address device name */
	int 	gap;		/* Floating address device gap 	*/
};

/*
 * Structure used for AUTO-CONFIGURATION of the system. As sizer
 * figures out the configuration for the system it places each 
 * alive adapter, controller, and device it finds in this structure.
 */
struct alive_device_list {
    short bus_type;		/* The bus type (MASSBUS, UNIBUS, ...) 	*/
    short device_index;		/* Index of device in "devtbl" table 	*/
    short device_unit;  	/* The unit # of the device		*/
    short device_drive;		/* The drive # or slave # of the device	*/
    char  unsupp_devname[10];	/* The unsupported device name          */
    char  conn_name[10];	/* The name of the connection point     */
    short conn_number;		/* The connection number		*/
    short node_number;		/* The node number      		*/
    short node_index;		/* Index of node in "nodetbl" table 	*/
    short rctlr;		/* The remote controller number	        */
    long  csr;			/* The csr addr of device in I/O space  */
    short alive_unit;		/* The device is alive		        */
};


/************************************************************************/
/*									*/
/* The following declarations define the GLOBAL INFORMATION used by the	*/
/* "sizer" program.							*/
/*									*/
/************************************************************************/

/* The NAMELIST elements to lookup in the kernel image file */
#define	NL_cpu			0
#define	NL_tz			1
#define	NL_rootdev		2
#define	NL_swdevt		3
#define	NL_dumpdev		4
#define	NL_ws_display_type	5
#define	NL_Physmem		6
#define	NL_rpb			7
#define	NL_cpu_avail		8
#define	NL_umem			9
#define	NL_cpu_subtype		10
#define	NL_qmem			11
#define	NL_ci_first_port	12
#define	NL_dmb_lines		13
#define	NL_dhu_lines		14
#define	NL_cpu_sub_subtype	15
#define NL_config_adpt		16
#define NL_ubdinit		17
#define NL_ubminit		18
#define NL_mbdinit		19
#define NL_mbsinit		20
#define NL_numubas		21
#define NL_nummbas		22
#define NL_mbhead		23
#define NL_roottype		24
#define NL_prsize		25

#define MAXDEVICES		400
#define PATHSIZE		255
#define DEVNAMESIZE		20
#define MAXUPRC			50
#define NODISPLAY		0
#define DISPLAY			1

char	BOOT[20];
int	kmem, CPU, CPUSUB, ADLINDEX; 
int	scs_sysidflag, float_flag;
FILE 	*fp, *fpdevs;
struct 	alive_device_list adltbl[MAXDEVICES];
long 	reset_anythg();

/* 
 * The following global variables holds the name information from
 * the specific fields in the "config_adpt", "ubdinit", "mbdinit",
 * and "mbsinit" device structures.
 */
char	cname[DEVNAMESIZE], pname[DEVNAMESIZE];
char	ubcname[DEVNAMESIZE], ubdname[DEVNAMESIZE];
char	mbdname[DEVNAMESIZE], mbsname[DEVNAMESIZE];

/*
 * Holds the device offset and device count values when reading 
 * device information from the kernel image file. The offsets
 * are for the "config_adpt", "ubdinit", "mbdinit", and "mbsinit"
 * device structures.
 */
long 	adapter_offset;		/* Used for reading "config_adpt" */
int 	adapter_cnt;
long	udevice_offset;		/* Used for reading "ubdinit" */
int	udevice_cnt;
long 	mslave_offset;		/* Used for reading "mbsinit" */
int 	mslave_cnt;
long	mdevice_offset;		/* Used for reading "mbdinit" */
int	mdevice_cnt;
struct 	uba_ctlr ubctlr;	/* Holds a UNIBUS controller structure */
struct 	uba_device ubdevice;	/* Holds a UNIBUS device structure */
struct 	mba_driver mbdriver;	/* Holds a MASSBUS driver structure */

/* External declarations for the CONFIGURATION FILE TABLES */
extern 	struct config_node		nodetbl[];
extern 	struct config_cpu		cputbl[];
extern 	struct config_device		devtbl[];
extern	struct nlist			nl[];
extern 	struct hardware			hardtbl[];
extern 	struct float_devices		floattbl[];

/* External declarations for the CONFIGURATION FILE TABLES SIZES */
extern 	int NODETBLSIZE;
extern 	int CPUTBLSIZE;
extern 	int DEVTBLSIZE;
extern 	int NLTBLSIZE;
extern 	int HARDTBLSIZE;
extern 	int FLTTBLSIZE;

