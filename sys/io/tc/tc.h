/*
 * @(#)tc.h	4.6	(ULTRIX)	1/22/91
 */
/************************************************************************
 *									*
 *			Copyright (c) 1990 by				*
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

/*
 * Modification History: tc.h
 *
 * 21-Jan-91	Randall Brown
 *	Added tc_memerr_status struct for use in tc_isolate_memerr()
 *	routine.
 *
 * 15-Oct-90	Randall Brown
 *	Added defines for ROM header offsets.
 *
 *  6-Sep-90	Randall Brown
 *	Changed slot_order to be config_order. 
 *
 *  5-Jul-90	Randall Brown
 *	Added the routine tc_ui_to_name() for drivers to determine
 *	the name of the specific option module.
 *
 * 27-Mar-90	Randall Brown
 *	Added #defines for TC_OPTION_SLOT_*
 *
 * 22-Jan-90	Randall Brown
 *	Created this file for TURBOchannel support
 *
 */

#include "../h/types.h"

#define TC_IOSLOTS 8			/* number of TURBOchannel slots */
#define TC_OPTION_SLOTS 3		/* number of optional tc slots */

#define TC_OPTION_SLOT_0	0
#define TC_OPTION_SLOT_1	1
#define TC_OPTION_SLOT_2	2

#define TC_ROMOFFSET	0x003c0000	/* offset in slot to IO ROM */

#define TC_VERSION_OFFSET	0x400
#define TC_VENDOR_OFFSET	0x420
#define TC_NAME_OFFSET		0x440

#define TC_ROMNAMLEN 8			/* length of module name in rom */

struct tc_option {
	char modname[TC_ROMNAMLEN+1];	/* the module name */
	char confname[TC_ROMNAMLEN+1];	/* device or ctlr name (config file) */
	int  intr_b4_probe;		/* enable intr before probe routine */
	int  intr_aft_attach;		/* enable intr after attach routine */
	char type;			/* D = dev, C = ctrlr, A = adpt */
	int (*adpt_config)();		/* adpater config routine to call */
};

/*
 *	The following are used to describe the class attribute of the
 *	module.  These will be filled in during auto-configuration.
 */
#define TC_CTLR 	1
#define TC_DEV 		2
#define TC_ADPT 	3

struct tc_slot {
	char version[TC_ROMNAMLEN+1];	/* the version in the ROM */
	char vendor[TC_ROMNAMLEN+1];	/* the vendor in the ROM */
	char modulename[TC_ROMNAMLEN+1];	/* the module name in the ROM */
	char devname[TC_ROMNAMLEN+1];	/* the controller or device name */
	int slot;		/* the TURBOchannel IO slot number */
	int module_width;	/* how many slots the IO module takes */
	int (*intr)();		/* intr routine for device (from config file)*/
	int unit;		/* device unit number (from config file) */
	int physaddr;		/* the physical addr of the device */
	int class;		/* ctlr or dev: to call right config routine */
	int intr_b4_probe;	/* enable intr before probe routine */
	int intr_aft_attach;	/* enable intr after attach routine */
	int (*adpt_config)();	/* config routine for adapters
				   (from tc_option_data table) */
};

extern struct tc_slot tc_slot[];	/* table with IO device info */
extern u_int   tc_slotaddr[];		/* table with slot address info */

struct tc_sw {
        int *config_order;	/* order to probe slots of TURBOchannel */
        int (*enable_option)();	/* routine to enable interrupt */
	int (*disable_option)();/* routine to disable interrupt */
	int (*clear_errors)();	/* routine to clear errors caused by probe */
	int (*isolate_memerr)();/* routine to isolate memory errors */
};

extern struct tc_sw tc_sw;
/*
 *	tc_enable_option(ui)
 *		struct uba_device *ui;
 *
 *	Takes a pointer to a uba_device struct or a uba_controller struct,
 *  	since the beginning of these structs are identical it will take
 *	a pointer to either one.
 *
 *	This function enables an option's interrupt on the TURBOchannel
 *	to interrupt the system at the I/O interrupt level.  
 *	This is done calling the system specific routine to allow the
 *	option's slot to interrupt the system.
 */

int tc_enable_option();

/*
 *	tc_disable_option(ui)
 *		struct uba_device *ui;
 *
 *	Takes a pointer to a uba_device struct or a uba_controller struct,
 *  	since the beginning of these structs are identical it will take
 *	a pointer to either one.
 *
 *	This function disables an option's interrupt on the TURBOchannel from
 *	interrupting the system at the I/O interrupt level.  
 *	This is done by calling the system specific routine to reset the 
 *	option's slot from interrupting the system.
 */	

int tc_disable_option();

/*
 *	tc_ui_to_name(ui, cp)
 *		struct uba_device *ui;
 *		char *cp; ( cp[TC_ROMNAMLEN + 1] )
 *
 *	Takes a pointer to a uba_device struct or a uba_controller struct,
 *  	since the beginning of these structs are identical it will take
 *	a pointer to either one.
 *
 *	This function will fill in the character array 'cp' with the ascii
 *	string of the TURBOchannel option's module name refered to by the
 *	 'ui' pointer.  The array 'cp' must be declared by the caller
 *	of this routine to be the size defined above.
 *
 *	The function will return a (-1) if it was unable to use the 'cp'
 *	pointer that it was given.
 */

int	tc_ui_to_name();

/*
 *	tc_addr_to_name(addr, cp)
 *		u_int	addr;
 *		char	*cp; ( cp[TC_ROMNAMLEN + 1] )
 *
 *	Takes the address passed to the device driver's probe routine
 *	which is the base address of the device.
 *
 *	This function will fill in the character array 'cp' with the ascii
 *	string of the TURBOchannel option's module name refered to by the
 *	base address 'addr'.  The array 'cp' must be declared by the caller
 *	of the routine to be the size defined above.
 *
 *	This function would be used in a driver's probe routine, since the
 *	'ui' pointer required by the above routine 'tc_ui_to_name' is
 *	not valid in the probe routine.
 *
 *	The function will return a (-1) if it was unable to use the 'cp'
 *	pointer that it was given.
 */

int	tc_addr_to_name();

struct tc_memerr_status {
    caddr_t	pa;		/* physical address of error */
    caddr_t	va;		/* virtual address, 0 if not know */
    int		log;		/* flag whether to log error */
    int		blocksize;	/* size of DMA block */
    u_int	errtype;	/* error type status */
};

/*
 *	tc_isolate_memerr(memerr_status)
 *	    struct tc_memerr_status *memerr_status;
 *
 *	Takes a pointer to a tc_memerr_status struct.
 *
 *	Takes the physical address (pa) of the error, the virtual
 *	address of the error (va), flag for logging, and a pointer
 *	to a status int.  If the va is 0, a K1 address is formed from
 *	the physical address and is used as the virtual address.
 *	
 *	This function will fill in the u_int 'errtype' with
 *	the information about the memory error of 'pa' as defined below.
 *	This is done by calling a system specific routine to determine the 
 *	exact error based on the physical address and virtual address.  If
 *	the parameter 'log' is set to TC_LOG_MEMERR, the system 
 *	specific routine will log the error in the same manner a memory
 *	error is logged as if it came directly into the CPU.
 *
 *	The function will return a (-1) if it the physical address supplied
 *	is bad, or if the system specific routine does not exist.
 */

/*
 * 	The following defines are used for the 'log' parameter in the
 * 	tc_memerr_status struct passed to tc_isolate_memerr().
 */
#define	TC_NOLOG_MEMERR		0	/* do not log error info */
#define	TC_LOG_MEMERR		1	/* log error information */

/* 
 *	The following defines are used for the 'errtype' return status
 *	in the tc_memerr_status struct passed to tc_isolate_memerr().
 *
 *	NOTE: As other systems are produced that have different
 *	      memory subsystems, additional defines will be added to the
 *	      list below.
 */
#define TC_MEMERR_NOERROR	0	/* no error found */
#define TC_MEMERR_TRANS		1	/* transient parity error */
#define TC_MEMERR_SOFT		2	/* soft parity error */
#define TC_MEMERR_HARD		3	/* hard parity error */

int	tc_isolate_memerr();

