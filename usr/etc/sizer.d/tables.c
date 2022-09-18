#ifndef lint
static char *sccsid = "@(#)tables.c	4.4      (ULTRIX)  8/13/90";
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
 * Name: tables.c
 *
 * Modification History
 * 
 * 10-Aug-90	Randall Brown
 *	Added support for DS5000_100 (3MIN).
 *	Added supprot for devices scc and fb.
 *
 * 09-Aug-90	Fred L. Templin
 *	Added line for "FZA"
 *
 * 03-Aug-90	rafiey (Ali Rafieymehr)
 * 	Added support for VAX9000.
 * 
 * May 21, 1990 - Robin
 * 	Added presto NVRAM support
 *
 * Mar 2, 1990 - Randall Brown
 *	Added support for 2da and 3da graphics boards
 *
 * Dec 6, 1989 - Alan Frechette
 *	Added support for MIPSFAIR2.
 *
 * Nov 3, 1989 - Alan Frechette
 *	Added vmebus adapter type.
 *
 * Oct 11, 1989 - Alan Frechette
 *	Added support for 3MAX and PMAX.
 *
 * July 14, 1989 - Alan Frechette
 *	Added separate "config_device[]" table for mips. Mark which
 *	devices are supported and which devices are not supported.
 *	Added support for RIGEL (VAX_6400), added new (XMINODE),
 *	added new (KDM) controller.
 *
 * July 6, 1989 - Alan Frechette
 *	Added define for VAXSTAR cputype in cpu config table.
 *
 * May 10, 1989 - Alan Frechette
 *	Added an entry to the name list structure to get the 
 *	"roottype" for determining network boots.
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

#include "sizer.h"

/********************************************************************
 *
 * Table Structure for configuring the NODE information.
 *
 *******************************************************************/
struct config_node nodetbl[] = {
    { "vaxbi",		"node",		VAXBINODE	},
    { "xmi",		"node",		XMINODE		},
    { "ci",		"cinode",	CINODE		},
    { "msi",		"msinode",	MSINODE		},
    { "\0",		"\0",		UNKNOWN		}
};

/********************************************************************
 *
 * Table Structure for configuring the CPU information.
 *
 *******************************************************************/
struct config_cpu cputbl[] = {
    { VAX_780,	    "VAX780",	  	32,	"boot780"  },
    { VAX_750,	    "VAX750",	  	32,	"boot750"  },
    { VAX_730,	    "VAX730",	  	16,	"boot730"  },
    { VAX_8200,	    "VAX8200",	  	32,	"boot8200" },
    { VAX_8600,	    "VAX8600",	 	128,	"boot8600" },
    { VAX_8800,	    "VAX8800",	 	128,	"boot8800" },
    { VAX_8820,	    "VAX8820",	 	128,	"boot8800" },
    { VAX_6200,	    "VAX6200",	 	128,	"boot6200" },
    { VAX_6400,	    "VAX6400",	 	128,	"boot6400" },
    { MVAX_I,	    "MVAX",	   	8,	"bootI"    },
    { MVAX_II,	    "MVAX",	  	16,	"bootII"   },
    { VAXSTAR,	    "MVAX",	  	16,	"boot2000" },
    { VAX_3400,	    "VAX3400",	  	32,   	"boot3400" },
    { VAX_3600,	    "VAX3600",	  	32,   	"bootII"   },
    { VAX_3900,	    "VAX3900",	  	32,   	"bootII"   },
    { C_VAXSTAR,    "VAX420",	  	32,	"boot420"  },
    { VAX_60,	    "VAX60",	  	32,	"boot60"   }, 
    { VAX_9000,	    "VAX9000",	 	128,	"boot9000" },
    { DS_3100,      "DS3100",  		32,	 "\0"      }, 
    { DS_5000,      "DS5000",		32,	 "\0"      }, 
    { DS_5000_100,  "DS5000_100",	32,	 "\0"      }, 
    { DS_5100,      "DS5100",		32,	 "\0"      }, 
    { DS_5400,      "DS5400",		128,	 "\0"      }, 
    { DS_5500,      "DS5500",		128,	 "\0"      }, 
    { DS_5800,      "DS5800",		128,	 "\0"      }, 
    { UNKNOWN,	    "UNDEFINED_CPU",	0,	 "\0"  	   } 
};

/********************************************************************
 *
 * Table Structure for configuring the DEVICE information.
 *
 *******************************************************************/
struct config_device devtbl[] = {
#ifdef vax
    { "ci",	ADAPTER,	0,1,	"\0",		0 },
    { "ibus",	ADAPTER,	0,1,	"\0",		0 },
    { "mba",	ADAPTER,	0,1,	"\0",		0 },
    { "msi",	ADAPTER,	0,1,	"\0",		0 },
    { "uba",	ADAPTER,	0,1,	"\0",		0 },
    { "vaxbi",	ADAPTER,	0,1,	"\0",		0 },
    { "vba",	ADAPTER,	0,1,	"\0",		0 },
    { "xmi",	ADAPTER,	0,1,	"\0",		0 },
    { "aie",	CONTROLLER,	0,1,	"\0",		0 },
    { "aio",	CONTROLLER,	0,1,	"\0",		0 },
    { "asc",	CONTROLLER,	0,1,	"ascintr",	0 },
    { "bvpssp",	CONTROLLER,	0,1,	"bvpsspintr",	0 },
    { "dssc",	CONTROLLER,	0,1,	"\0",		0 },
    { "fx",	CONTROLLER,	0,0,	"rxintr",	0 },
    { "hk",	CONTROLLER,	0,1,	"rkintr",	0 },
    { "hl",	CONTROLLER,	0,1,	"rlintr",	0 },
    { "hsc",	CONTROLLER,	0,1,	"\0",		0 },
    { "idc",	CONTROLLER,	0,1,	"idcintr",	0 },
    { "kdb",	CONTROLLER,	0,1,	"\0",		0 },
    { "kdm",	CONTROLLER,	0,1,	"\0",		0 },
    { "klesib",	CONTROLLER,	0,1,	"\0",		0 },
    { "klesiu",	CONTROLLER,	0,1,	"\0",		0 },
    { "rqd",	CONTROLLER,	0,1,	"\0",		0 },
    { "sc",	CONTROLLER,	0,1,	"upintr",	0 },
    { "scsi",	CONTROLLER,	0,1,	"szintr",	0 },
    { "sdc",	CONTROLLER,	0,1,	"sdintr",	0 },
    { "sii",	CONTROLLER,	0,1,	"sii_intr",	0 },
    { "stc",	CONTROLLER,	0,1,	"stintr",	0 },
    { "tm",	CONTROLLER,	0,0,	"tmintr",	0 },
    { "uda",	CONTROLLER,	0,1,	"\0",		0 },
    { "uq",	CONTROLLER,	0,1,	"uqintr",	0 },
    { "ut",	CONTROLLER,	0,0,	"utintr",	0 },
    { "va",	CONTROLLER,	0,0,	"\0",		0 },
    { "zs",	CONTROLLER,	0,1,	"tsintr",	0 },
    { "ht",	MASTER,		0,1,	"\0",		0 },
    { "mt",	MASTER,		0,1,	"\0",		0 },
    { "hp",	DISK,		1,1,	"\0",		0 },
    { "ra",	DISK,		1,1,	"\0",		0 },
    { "rb",	DISK,		1,1,	"\0",		0 },
    { "rd",	DISK,		1,1,	"\0",		0 },
    { "rk",	DISK,		1,1,	"\0",		0 },
    { "rl",	DISK,		1,1,	"\0",		0 },
    { "rx",	DISK,		1,1,	"\0",		0 },
    { "rz",	DISK,		1,1,	"\0",		0 },
    { "up",	DISK,		1,0,	"\0",		0 },
    { "urx",	DISK,		1,0,	"\0",		0 },
    { "vz",	DISK,		1,0,	"\0",		0 },
    { "mu",	TAPE,		1,1,	"\0",		0 },
    { "st",	TAPE,		1,1,	"\0",		0 },
    { "te",	TAPE,		1,1,	"\0",		0 },
    { "tj",	TAPE,		1,1,	"\0",		0 },
    { "tms",	TAPE,		1,1,	"\0",		0 },
    { "ts",	TAPE,		1,1,	"\0",		0 },
    { "tu",	TAPE,		1,1,	"\0",		0 },
    { "tz",	TAPE,		1,1,	"\0",		0 },
    { "acc",	DEVICE,		0,0,	"accrint accxint",0 },
    { "ad",	DEVICE,		1,0,	"\0",		0 },
    { "bvpni", 	DEVICE,		0,1,	"bvpniintr",	0 },
    { "cfb",	DEVICE,		1,0,	"cfbvint", 	0 },
    { "css",	DEVICE,		0,0,	"cssrint cssxint",0xa },
    { "ct",	DEVICE,		1,0,	"\0", 		0 },
    { "dc",	DEVICE,		0,0,	"dcintr",	0 },
    { "de",	DEVICE,		0,1,	"deintr",	0 },
    { "dh",	DEVICE,		1,0,	"dhrint dhxint",0xffff },
    { "dhu",	DEVICE,		1,1,	"dhurint dhuxint",0xffff },
    { "dm",	DEVICE,		1,0,	"dmintr", 	0xffff },
    { "dmb",	DEVICE,		1,1,	"dmbsint dmbaint dmblint",0xff },
    { "dmc",	DEVICE,		0,1,	"dmcrint dmcxint",0 },
    { "dmf",	DEVICE,		1,1,
      "dmfsrint dmfsxint dmfdaint dmfdbint dmfrint dmfxint dmflint",0xff },
    { "dmv",	DEVICE,		0,1,	"dmvrint dmvxint",0 },
    { "dmz",	DEVICE,		1,1,
      "dmzrinta dmzxinta dmzrintb dmzxintb dmzrintc dmzxintc",0xffffff },
    { "dn",	DEVICE,		0,0,	"dnintr",	0 },
    { "dpv",	DEVICE,		0,1,	"dpvrint dpvxint",0 },
    { "dup",	DEVICE,		0,1,	"duprint dupxint",0 },
    { "dz",	DEVICE,		1,1,	"dzrint dzxint",0xff },
    { "ec",	DEVICE,		0,0,	"ecrint ecxint eccollide",0 },
    { "en",	DEVICE,		0,0,	"enrint enxint encollide", 0 },
    { "fc",	DEVICE,		1,1,	"fcxrint",	0xf },
    { "fg",	DEVICE,		1,1,	"fgvint",	0xf },
    { "fza",	DEVICE,		0,1,	"fzaintr",	0 },
    { "hy",	DEVICE,		0,0,	"hyint",	0 },
    { "ik",	DEVICE,		1,0,	"ikintr", 	0 },
    { "il",	DEVICE,		0,0,	"ilrint ilcint",0 },
    { "kg",	DEVICE,		1,0,	"\0", 		0 },
    { "ln",	DEVICE,		0,1,	"lnintr",	0 },
    { "lp",	DEVICE,		1,1,	"lpintr",	0 },
    { "lx",	DEVICE,		1,1, 	"lxbvpint",	0 },
    { "ne",	DEVICE,		0,1,	"neintr",	0 },
    { "pcl",	DEVICE,		0,0,	"pclxint pclrint",0 },
    { "pm",	DEVICE,		1,0,	"pmvint", 	0 },
    { "ps",	DEVICE,		1,0,	"\0", 		0 },
    { "qd",	DEVICE,		1,1,	"qddint qdaint qdiint",	0xf },
    { "qe",	DEVICE,		0,1,	"qeintr",	0 },
    { "qv",	DEVICE,		1,1,	"qvkint qvvint",0xf },
    { "se",	DEVICE,		0,1,	"seintr",	0 },
    { "sg",	DEVICE,		1,1,	"sgaint sgfint",0xf },
    { "sh",	DEVICE,		1,1,	"shrint shxint",0xff },
    { "sm",	DEVICE,		1,1,	"smvint",	0xf },
    { "ss",	DEVICE,		1,1,	"ssrint ssxint",0xf },
    { "sz",	DEVICE,		0,1,	"\0",		0 },
    { "un",	DEVICE,		0,0,	"unintr",	0 },
    { "uu",	DEVICE,		1,0,	"\0", 		0 },
    { "vp",	DEVICE,		1,0,	"vpintr",	0 },
    { "vv",	DEVICE,		1,0,	"vvrint vvxint", 0 },
    { "xna",	DEVICE,		0,1,	"xnaintr", 	0 },
    { "\0",	UNKNOWN,	0,0,	"\0", 		0 }
#endif vax
#ifdef mips
    { "ci",	ADAPTER,	0,1,	"\0",		0 },
    { "ibus",	ADAPTER,	0,1,	"\0",		0 },
    { "mba",	ADAPTER,	0,0,	"\0",		0 },
    { "msi",	ADAPTER,	0,1,	"\0",		0 },
    { "uba",	ADAPTER,	0,1,	"\0",		0 },
    { "vaxbi",	ADAPTER,	0,1,	"\0",		0 },
    { "vba",	ADAPTER,	0,1,	"\0",		0 },
    { "xmi",	ADAPTER,	0,1,	"\0",		0 },
    { "aie",	CONTROLLER,	0,1,	"\0",		0 },
    { "aio",	CONTROLLER,	0,1,	"\0",		0 },
    { "asc",	CONTROLLER,	0,1,	"ascintr",	0 },
    { "bvpssp",	CONTROLLER,	0,1,	"bvpsspintr",	0 },
    { "dssc",	CONTROLLER,	0,1,	"\0",		0 },
    { "fx",	CONTROLLER,	0,0,	"rxintr",	0 },
    { "hk",	CONTROLLER,	0,0,	"rkintr",	0 },
    { "hl",	CONTROLLER,	0,0,	"rlintr",	0 },
    { "hsc",	CONTROLLER,	0,1,	"\0",		0 },
    { "idc",	CONTROLLER,	0,0,	"idcintr",	0 },
    { "kdb",	CONTROLLER,	0,1,	"\0",		0 },
    { "kdm",	CONTROLLER,	0,1,	"\0",		0 },
    { "klesib",	CONTROLLER,	0,1,	"\0",		0 },
    { "klesiu",	CONTROLLER,	0,1,	"\0",		0 },
    { "rqd",	CONTROLLER,	0,0,	"\0",		0 },
    { "sc",	CONTROLLER,	0,1,	"upintr",	0 },
    { "scsi",	CONTROLLER,	0,1,	"szintr",	0 },
    { "sdc",	CONTROLLER,	0,1,	"sdintr",	0 },
    { "sii",	CONTROLLER,	0,1,	"sii_intr",	0 },
    { "stc",	CONTROLLER,	0,1,	"stintr",	0 },
    { "tm",	CONTROLLER,	0,0,	"tmintr",	0 },
    { "uda",	CONTROLLER,	0,1,	"\0",		0 },
    { "uq",	CONTROLLER,	0,1,	"uqintr",	0 },
    { "ut",	CONTROLLER,	0,0,	"utintr",	0 },
    { "va",	CONTROLLER,	0,0,	"\0",		0 },
    { "zs",	CONTROLLER,	0,0,	"tsintr",	0 },
    { "ht",	MASTER,		0,0,	"\0",		0 },
    { "mt",	MASTER,		0,0,	"\0",		0 },
    { "hp",	DISK,		1,0,	"\0",		0 },
    { "ra",	DISK,		1,1,	"\0",		0 },
    { "rb",	DISK,		1,0,	"\0",		0 },
    { "rd",	DISK,		1,0,	"\0",		0 },
    { "rk",	DISK,		1,0,	"\0",		0 },
    { "rl",	DISK,		1,0,	"\0",		0 },
    { "rx",	DISK,		1,1,	"\0",		0 },
    { "rz",	DISK,		1,1,	"\0",		0 },
    { "up",	DISK,		1,0,	"\0",		0 },
    { "urx",	DISK,		1,0,	"\0",		0 },
    { "vz",	DISK,		1,0,	"\0",		0 },
    { "mu",	TAPE,		1,0,	"\0",		0 },
    { "st",	TAPE,		1,0,	"\0",		0 },
    { "te",	TAPE,		1,0,	"\0",		0 },
    { "tj",	TAPE,		1,0,	"\0",		0 },
    { "tms",	TAPE,		1,1,	"\0",		0 },
    { "ts",	TAPE,		1,0,	"\0",		0 },
    { "tu",	TAPE,		1,0,	"\0",		0 },
    { "tz",	TAPE,		1,1,	"\0",		0 },
    { "acc",	DEVICE,		0,0,	"accrint accxint",0 },
    { "ad",	DEVICE,		1,0,	"\0",		0 },
    { "bvpni", 	DEVICE,		0,1,	"bvpniintr",	0 },
    { "cfb",	DEVICE,		0,1,	"cfbvint", 	0 },
    { "fb",	DEVICE,		0,1,	"fbint", 	0 },
    { "css",	DEVICE,		0,0,	"cssrint cssxint",0xa },
    { "ct",	DEVICE,		1,0,	"\0", 		0 },
    { "dc",	DEVICE,		1,1,	"dcintr",	0 },
    { "scc", 	DEVICE,		1,1,	"sccintr",	0 },
    { "de",	DEVICE,		0,1,	"deintr",	0 },
    { "dh",	DEVICE,		1,0,	"dhrint dhxint",0xffff },
    { "dhu",	DEVICE,		1,1,	"dhurint dhuxint",0xffff },
    { "dm",	DEVICE,		1,0,	"dmintr", 	0xffff },
    { "dmb",	DEVICE,		1,1,	"dmbsint dmbaint dmblint",0xff },
    { "dmc",	DEVICE,		0,0,	"dmcrint dmcxint",0 },
    { "dmf",	DEVICE,		1,0,
      "dmfsrint dmfsxint dmfdaint dmfdbint dmfrint dmfxint dmflint",0xff },
    { "dmv",	DEVICE,		0,0,	"dmvrint dmvxint",0 },
    { "dmz",	DEVICE,		1,0,
      "dmzrinta dmzxinta dmzrintb dmzxintb dmzrintc dmzxintc",0xffffff },
    { "dn",	DEVICE,		0,0,	"dnintr",	0 },
    { "dpv",	DEVICE,		0,0,	"dpvrint dpvxint",0 },
    { "dup",	DEVICE,		0,0,	"duprint dupxint",0 },
    { "dz",	DEVICE,		1,0,	"dzrint dzxint",0xff },
    { "ec",	DEVICE,		0,0,	"ecrint ecxint eccollide",0 },
    { "en",	DEVICE,		0,0,	"enrint enxint encollide", 0 },
    { "fc",	DEVICE,		1,0,	"fcxrint",	0xf },
    { "fg",	DEVICE,		1,0,	"fgvint",	0xf },
    { "fza",	DEVICE,		0,1,	"fzaintr",	0 },
    { "ga",	DEVICE,		0,1,	"gaintr", 	0 },
    { "gq",	DEVICE,		0,1,	"gqintr", 	0 },
    { "hy",	DEVICE,		0,0,	"hyint",	0 },
    { "ik",	DEVICE,		1,0,	"ikintr", 	0 },
    { "il",	DEVICE,		0,0,	"ilrint ilcint",0 },
    { "kg",	DEVICE,		1,0,	"\0", 		0 },
    { "ln",	DEVICE,		0,1,	"lnintr",	0 },
    { "lp",	DEVICE,		1,1,	"lpintr",	0 },
    { "lx",	DEVICE,		1,0, 	"lxbvpint",	0 },
    { "mdc",	DEVICE,		1,1, 	"mdcintr",	0 },
    { "ne",	DEVICE,		0,1,	"neintr",	0 },
    { "pcl",	DEVICE,		0,0,	"pclxint pclrint",0 },
    { "pm",	DEVICE,		0,1,	"pmvint", 	0 },
    { "ps",	DEVICE,		1,0,	"\0", 		0 },
    { "qd",	DEVICE,		1,0,	"qddint qdaint qdiint",	0xf },
    { "qe",	DEVICE,		0,1,	"qeintr",	0 },
    { "qv",	DEVICE,		1,0,	"qvkint qvvint",0xf },
    { "se",	DEVICE,		0,1,	"seintr",	0 },
    { "sg",	DEVICE,		1,0,	"sgaint sgfint",0xf },
    { "sh",	DEVICE,		1,0,	"shrint shxint",0xff },
    { "sm",	DEVICE,		1,0,	"smvint",	0xf },
    { "ss",	DEVICE,		1,0,	"ssrint ssxint",0xf },
    { "sz",	DEVICE,		0,1,	"\0",		0 },
    { "un",	DEVICE,		0,0,	"unintr",	0 },
    { "uu",	DEVICE,		1,0,	"\0", 		0 },
    { "vp",	DEVICE,		1,0,	"vpintr",	0 },
    { "vv",	DEVICE,		1,0,	"vvrint vvxint", 0 },
    { "xna",	DEVICE,		0,1,	"xnaintr", 	0 },
    { "\0",	UNKNOWN,	0,0,	"\0", 		0 }
#endif mips
};

/********************************************************************
 *
 * Table Structure for defining the configuration file HARDWARE
 * NAME types.
 *
 *******************************************************************/
struct hardware hardtbl[] = {
    { "adapter"		},
    { "controller"	},
    { "master"		},
    { "disk"		},
    { "tape"		},
    { "device"		},
    { "\0"		}
};

/********************************************************************
 *
 * Table Structure for defining the NAMELIST kernel elements we
 * need to search for.
 *
 ********************************************************************/
struct nlist nl[] = {
	{ /* 0 */	"_cpu" 			},
	{ /* 1 */	"_tz" 			},
	{ /* 2 */	"_rootdev" 		},
	{ /* 3 */	"_swdevt" 		},
	{ /* 4 */	"_dumpdev" 		},
	{ /* 5 */	"_ws_display_type"	},
	{ /* 6 */	"_Physmem" 		},
	{ /* 7 */	"_rpb" 			},
	{ /* 8 */	"_cpu_avail" 		},
	{ /* 9 */	"_umem"			},
	{ /* 10 */	"_cpu_subtype"		},
	{ /* 11 */	"_qmem"			},
	{ /* 12 */	"_ci_first_port"	},
	{ /* 13 */	"_dmb_lines"		},
	{ /* 14 */	"_dhu_lines"		},
	{ /* 15 */	"_cpu_sub_subtype"	},
	{ /* 16 */	"_config_adpt"		},
	{ /* 17 */	"_ubdinit"		},
	{ /* 18 */	"_ubminit"		},
	{ /* 19 */	"_mbdinit"		},
	{ /* 20 */	"_mbsinit"		},
	{ /* 21 */	"_nNUBA"		},
	{ /* 22 */	"_nNMBA"		},
	{ /* 23 */	"_mba_hd"		},
	{ /* 24 */	"_roottype"		},
	{ /* 25 */	"_prsize"		},
	{ /* end */	"\0"			} 
};

/********************************************************************
 *
 * Table Structure for defining the FLOATING ADDRESS DEVICES that
 * we need to search for.
 *
 *******************************************************************/
struct float_devices floattbl[] = {
	{ "dj11",	04 	},	
	{ "dh",		010 	},	
	{ "dq11",	04 	}, 
	{ "du11",	04 	}, 	
	{ "dup",	04 	},	
	{ "lk11",	04 	}, 
	{ "dmc", 	04 	},	
	{ "dz", 	04 	}, 	
	{ "kmc11",	04 	}, 
	{ "lpp11", 	04 	},	
	{ "vmv21",	04 	},	
	{ "vmv31",	010 	},
	{ "dwr70", 	04 	},	
	{ "hl",		04 	},	
	{ "lpa11",	010 	},
	{ "kw11c", 	04 	}, 	
	{ "rsv",	04 	},	
	{ "fx",   	04 	}, 
	{ "un",	 	04 	},	
	{ "hy",   	04 	}, 	
	{ "dmp11",	04 	}, 
	{ "dpv",  	04 	},	
	{ "isb11",	04 	},	
	{ "dmv", 	010 	},
	{ "de",	 	04 	},	
	{ "uda",	02 	},	
	{ "dmf",  	020 	},
	{ "kms11", 	010 	}, 	
	{ "vs100",	010 	},	
	{ "klesiu",	02 	},
	{ "kmv11", 	010 	},	
	{ "dhu",  	010 	}, 	
	{ "dmz", 	020 	},
	{ "cpi32",	020 	},	
	{ "qvss",	040 	},
	{ "vs31",	04 	}, 
	{ "dtqna", 	04 	},	
	{ "csam",	04 	},	
	{ "adv11c",	04 	},
	{ "aav11c",	04 	}, 	
	{ "axv11c",	04 	},	
	{ "kwv11c",	02 	},
	{ "adv11d", 	04 	},	
	{ "aav11d",	04 	}, 	
	{ "drq3p", 	04 	},
	{ "\0", 	0  	}
};

/* Define the sizes of all the above tables */
int NODETBLSIZE = (sizeof(nodetbl)/sizeof(nodetbl[0]));
int CPUTBLSIZE  = (sizeof(cputbl)/sizeof(cputbl[0]));
int DEVTBLSIZE  = (sizeof(devtbl)/sizeof(devtbl[0]));
int NLTBLSIZE  	= (sizeof(nl)/sizeof(nl[0]));
int HARDTBLSIZE = (sizeof(hardtbl)/sizeof(hardtbl[0]));
int FLTTBLSIZE  = (sizeof(floattbl)/sizeof(floattbl[0]));
