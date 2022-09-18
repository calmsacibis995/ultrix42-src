#ifdef	line
static char *sccsid = "@(#)tc.c	4.6      (ULTRIX)  1/22/91";
#endif	lint

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
 * Modification History: tc.c
 *
 * 21-Jan-91	Randall Brown
 *	Changed interface to routine tc_isolate_memerr(), now has
 *	a tc_memerr_status struct passed to it.
 *
 * 06-Dec-90	Randall Brown
 *	Added the interface routine tc_isolate_memerr().
 *
 * 15-Oct-90	Randall Brown
 *	Changed the tc_probe() routine so that it looks at the ROM header
 *	correctly for 2-byte and 4-byte wide ROMs.
 *
 *  6-Sep-90	Randall Brown
 *	Changed slot_order to be config_order. 
 *
 *  5-Jul-90	Randall Brown
 *	Added the routine tc_module_name() for drivers to determine
 *	the name of the specific option module.
 *
 * 13-Mar-90	Randall Brown
 *	Changed the probe routine to look for the ROM at both 
 *	offset 0 and 0x3c0000.
 *
 * 22-Jan-90	Randall Brown
 *	Created this file for TURBOchannel support
 *
 */
#include "../machine/pte.h"
#include "../machine/cpu.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/map.h"
#include "../h/buf.h"
#include "../h/dk.h"
#include "../h/user.h"			/* gets time.h and debug.h too */
#include "../h/proc.h"
#include "../h/vm.h"
#include "../h/conf.h"
#include "../h/errlog.h"
#include "../h/cmap.h"
#include "../h/config.h"

#include "../io/uba/ubavar.h"
#include "../io/uba/ubareg.h"
#include "../io/tc/tc.h"

#include "../../machine/common/cpuconf.h"

struct tc_slot 	tc_slot[TC_IOSLOTS];	/* table with IO device info */
u_int	tc_slotaddr[TC_IOSLOTS];/* table with slot address info */

/* offsets to look for rom in address space */
u_int	tc_romoffset[2] = { 0x003c0000, 0 };

struct tc_sw tc_sw;

extern struct uba_ctlr ubminit[];
extern struct uba_device ubdinit[];
extern struct config_adpt config_adpt[];

tc_init()
{
    int i;
    /*
     * Clear the tc_slot table to "safe" initialized values
     */
    for (i = 0; i < TC_IOSLOTS; i++) {
	strcpy(tc_slot[i].modulename,"");
	strcpy(tc_slot[i].devname,"");
	tc_slot[i].slot = i;
	tc_slot[i].module_width = 0;
	tc_slot[i].intr = 0;
	tc_slot[i].unit = -1;
	tc_slot[i].physaddr = 0;
	tc_slot[i].class = 0;
	tc_slot[i].intr_b4_probe = 0;
	tc_slot[i].intr_aft_attach = 0;
	tc_slot[i].adpt_config = 0;
    }
}

/*
 * tc_probe()
 *
 * Determine what is really on the system by doing a badaddr on
 * each of the variable IO option slots and reading the module ROM.
 */
tc_probe(verbose)
    int verbose;			/* work silently if not set */
{
    register int i, j, k, l;
    int s;
    int found;
    int rom_width;
    int rom_stride;
    int module_width;
    int romoffset;
    int good_rom, bad_format;
    char curr_module_version[TC_ROMNAMLEN + 1];	/* module version from the ROM */
    char curr_module_vendor[TC_ROMNAMLEN + 1];	/* module vendor from the ROM */
    char curr_module_name[TC_ROMNAMLEN + 1];	/* module name from the ROM */
    u_char *romptr;
    char testp;			/* test pattern character */
    u_long curr_module_id;		/* module id from the ROM */
    extern struct tc_option tc_option[];
    
    /*
     * Clear the optional tc_slot table entries to "safe"
     * initialized values.
     */
    for (i = TC_OPTION_SLOTS-1; i >= 0; i--) {
	strcpy(tc_slot[i].modulename,"");
	strcpy(tc_slot[i].devname,"");
	tc_slot[i].slot = i;
	tc_slot[i].module_width = 0;
	tc_slot[i].intr = 0;
	tc_slot[i].unit = -1;
	tc_slot[i].physaddr = 0;
	tc_slot[i].class = 0;
	tc_slot[i].intr_b4_probe = 0;
	tc_slot[i].intr_aft_attach = 0;
	tc_slot[i].adpt_config = 0;
    }
    
    /*
     * Start at the TOP of the variable IO option slots and work down.
     * This is done for the sake of options that can take up more than
     * 1 slot (the ROM space will be at the highest slot's address space).
     */
    for (i = TC_OPTION_SLOTS-1; i >= 0; i = i-module_width) {

	good_rom = 0;
	bad_format = 0;

	for (j = 0; (j < 2) && (good_rom != 1); j++) {

	    /*
	     * Do badaddr probe on addr tc_slotaddr[i] to see if the
	     * slot has a module in it (try to read 1 byte).
	     */
	    if (BADADDR(PHYS_TO_K1(tc_slotaddr[i]+tc_romoffset[j]), 1)) {
		module_width = 1;
		(*tc_sw.clear_errors)();
		wbflush();
		continue;
	    }
	    
	    /*
	     * Found a module, now read the ROM:
	     * First read the ROM width, stride and module width out
	     * of the "first layer".  Also sanity check the ROM by reading
	     * the test patterns.  If its ok, then from the ROM width &
	     * stride we know how to read the "second" layer", with the
	     * module name and module id.
	     */
	    rom_width = *(u_char *)PHYS_TO_K1(tc_slotaddr[i] + tc_romoffset[j] + 0x3e0);
	    rom_stride = *(u_char *)PHYS_TO_K1(tc_slotaddr[i] + tc_romoffset[j] + 0x3e4);
	    module_width = *(u_char *)PHYS_TO_K1(tc_slotaddr[i] + tc_romoffset[j] + 0x3ec);
	    testp = *(u_char *)PHYS_TO_K1(tc_slotaddr[i] + tc_romoffset[j] + 0x3f0);
	    if ((((int)testp) & 0xff) != 0x55) {
		module_width = 1;
		bad_format = 1;
		continue;
	    }
	    testp = *(u_char *)PHYS_TO_K1(tc_slotaddr[i] + tc_romoffset[j] + 0x3f4);
	    if ((((int)testp) & 0xff) != 0x00) {
		module_width = 1;
		bad_format = 1;
		continue;
	    }
	    testp = *(u_char *)PHYS_TO_K1(tc_slotaddr[i] + tc_romoffset[j] + 0x3f8);
	    if ((((int)testp) & 0xff) != 0xaa ) {
		module_width = 1;
		bad_format = 1;
		continue;
	    }
	    testp = *(u_char *)PHYS_TO_K1(tc_slotaddr[i] + tc_romoffset[j] + 0x3fc);
	    if ((((int)testp) & 0xff) != 0xff) {
		module_width = 1;
		bad_format = 1;
		continue;
	    }

	    /* Since we have made it through all of the tests, we are  	*/
	    /* dealing with a good rom.					*/
	    good_rom = 1;
	    romoffset = tc_romoffset[j];
	    
	}

	if (good_rom == 0) {
	    if (verbose && bad_format)
		printf ("Module in slot %d has bad ROM format, can't configure it\n", i);
	    continue;
	}

	    
	romptr = (u_char *)(PHYS_TO_K1(tc_slotaddr[i] + romoffset));

	/* Read in the version from the ROM */
	for (k = 0, l = TC_VERSION_OFFSET; k < TC_ROMNAMLEN; k++, l += rom_stride)
	    curr_module_version[k] = romptr[l];
	curr_module_version[TC_ROMNAMLEN] = NULL;

	/* Read in the vendor from the ROM */
	for (k = 0, l = TC_VENDOR_OFFSET; k < TC_ROMNAMLEN; k++, l += rom_stride)
	    curr_module_vendor[k] = romptr[l];
	curr_module_vendor[TC_ROMNAMLEN] = NULL;

	/* Read in the name from the ROM */
	for (k = 0, l = TC_NAME_OFFSET; k < TC_ROMNAMLEN; k++, l += rom_stride)
	    curr_module_name[k] = romptr[l];
	curr_module_name[TC_ROMNAMLEN] = NULL;

	/*
	 * Look for the module in the tc_option data table to get
	 * the config file name for the controller/device.
	 */
	found = 0;
	for (j = 0; tc_option[j].modname[0] != NULL; j++) {
	    if (!(strcmp (curr_module_name, tc_option[j].modname))) {
		/*
		 * Found it, fill in the tc_slot table
		 */
		strcpy(tc_slot[i].version, curr_module_version);
		strcpy(tc_slot[i].vendor, curr_module_vendor);
		strcpy(tc_slot[i].modulename, curr_module_name);
		strcpy(tc_slot[i].devname, tc_option[j].confname);
		tc_slot[i].slot = i;
		tc_slot[i].module_width = module_width;
		tc_slot[i].physaddr = tc_slotaddr[i];
		tc_slot[i].intr_b4_probe = tc_option[j].intr_b4_probe;
		tc_slot[i].intr_aft_attach = tc_option[j].intr_aft_attach;
		if (tc_option[j].type == 'A')
		    tc_slot[i].adpt_config = tc_option[j].adpt_config;
		found = 1;
		break;
	    }
	}
	if ((found == 0) && verbose) {
	    printf ("Module %s not in tc_option data table, can't configure it\n", curr_module_name);
	}
    }
}

/*
 *	tc_enable_option()
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
tc_enable_option(ui)
    struct uba_device *ui;
{
    register int index = ui->ui_nexus;

    (*tc_sw.enable_option)(index);
}

/*
 *	tc_disable_option()
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
tc_disable_option(ui)
    struct uba_device *ui;
{
    register int index = ui->ui_nexus;

    (*tc_sw.disable_option)(index);
}

/*
 *	tc_module_name(ui, cp)
 *		struct uba_device *ui;
 *		char cp[TC_ROMNAMLEN];
 *
 *	Takes a pointer to a uba_device struct or a uba_controller struct,
 *  	since the beginning of these structs are identical it will take
 *	a pointer to either one.
 *
 *	This function will fill in the character array 'cp' with the ascii
 *	string of the TURBOchannel option's module name refered to by the
 *	 'ui' pointer.
 *
 *	The function will return a (-1) if it was unable to use the 'cp'
 *	pointer that it was given.
 */

tc_module_name(ui, cp)
	struct uba_device *ui;
	char *cp;
{
    register int index = ui->ui_nexus;
    register int i;
    register char *tcp;

    /* sanity check the buffer cp[] to verify that it 	*/
    /* can be written into.				*/
    for (i = 0, tcp = cp; i < TC_ROMNAMLEN + 1; i++, tcp++) 
	if (BADADDR(tcp, 1))
	    return (-1);

    bcopy(tc_slot[index].modulename, cp, TC_ROMNAMLEN + 1);
    return (0);
}

tc_addr_to_name(addr, cp)
	u_int addr;
	char *cp;
{
    register int i;
    register char *tcp;

    /* sanity check the buffer cp[] to verify that it 	*/
    /* can be written into.				*/
    for (i = 0, tcp = cp; i < TC_ROMNAMLEN + 1; i++, tcp++) 
	if (BADADDR(tcp, 1))
	    return (-1);

    for (i = 0; i < TC_IOSLOTS; i++) {
	if (svtophy(addr) == tc_slotaddr[i]) {
	    bcopy(tc_slot[i].modulename, cp, TC_ROMNAMLEN + 1);
	    return (0);
	}
    }

    return (-1);
}

/*
 * Look for a specific option on the system, if its found return its
 *     physical address, else return 0.
 * In particular, this routine is used at "consinit" time, to determine
 *     if there is a graphics module in the system and if so, where it is.
 */
tc_where_option(str)
	char *str;			/* the device name to look for */
{
	register int i;
	register int index;

	extern int cold;

	cold = 1;
	tc_probe(0);
	cold = 0;

	/*
	 * Go thru the tc_slot table and look for device in "str".
	 * If found, return its address, else return 0.
	 */
	for (i = 0; tc_sw.config_order[i] != -1; i++) {
	    index = tc_sw.config_order[i];
	    if (!(strcmp (str, tc_slot[index].devname)))
			return (tc_slot[index].physaddr);
	}

	return(0);
}

/*	tc_find()
 *
 *	This routine probes the option slots and fills in the tc_slot
 *	table with the devices that are found on the system.  It then
 *	turns on interrupts and calls the probe routine for each device.
 *
 *	NOTE: 	Interrupts are enabled after return from this routine.
 */
tc_find()
{
    int found;
    register int i, j;
    register int index;		/* current index into tc_slot to config */

    /* Probe the option slots with printing turned on. */
    tc_probe(1);

    /*
     * For each device/controller found on the system, look for it in
     * the ubminit or ubdinit data structs (built from the config file).
     * If it is found, then fill in the tc_slot table.
     *
     * If in the config file a dev or ctlr was assigned to a specific
     * ibus (slot #), then we will only match if we find that piece
     * of hardware in the corresponding slot on the system.
     *
     * The default case is that we will assign unit numbers to like devices
     * starting with unit 0 and going thru the devices according to the
     * "config_order" list.
     */
    for (i = 0; tc_sw.config_order[i] != -1; i++) {
	index = tc_sw.config_order[i];
	found = 0;
	for (j = 0; ubminit[j].um_driver != 0; j++) 
	    if ((!(strcmp (tc_slot[index].devname, ubminit[j].um_ctlrname))) &&
		((ubminit[j].um_adpt == tc_slot[index].slot) ||
		 ((ubminit[j].um_adpt == '?') && (!(tc_func_used(*ubminit[j].um_intr)))))){
		tc_slot[index].intr = *ubminit[j].um_intr;
		tc_slot[index].unit = ubminit[j].um_ctlr;
		ubminit[j].um_nexus = index;
		tc_slot[index].class = TC_CTLR;
		if (tc_slot[index].intr_b4_probe)
		    (*tc_sw.enable_option)(index);
		found = 1;
		break;
	    }
	if (!found)
	    for (j = 0; ubdinit[j].ui_driver != 0; j++) {
		if ((!(strcmp (tc_slot[index].devname, ubdinit[j].ui_devname))) && 
		    ((ubdinit[j].ui_adpt == tc_slot[index].slot) ||
		     ((ubdinit[j].ui_adpt == '?') && (!(tc_func_used(*ubdinit[j].ui_intr)))))){
		    tc_slot[index].intr = *ubdinit[j].ui_intr;
		    tc_slot[index].unit = ubdinit[j].ui_unit;
		    ubdinit[j].ui_nexus = index;
		    tc_slot[index].class = TC_DEV;
		    if (tc_slot[index].intr_b4_probe)
			(*tc_sw.enable_option)(index);
		    found = 1;
		    break;
		}
	    }
	if (!found)
	    for (j = 0; config_adpt[j].p_name != NULL; j++) {
		if ((!(strcmp (tc_slot[index].devname, config_adpt[j].c_name))) &&
		    (config_adpt[j].c_type == 'A')) {
		    /* 
		     * Note: probe routine fills in the
		     * 	interrupt routine
		     */
		    tc_slot[index].unit = config_adpt[j].c_num;
		    tc_slot[index].class = TC_ADPT;
		    if (tc_slot[index].intr_b4_probe)
			(*tc_sw.enable_option)(index);
		    found = 1;
		    break;
		}
	    }
    }
    
    /*
     * Safe to take interrupts now (first clear any old bits in ERR reg).
     */
    (*tc_sw.clear_errors)();
    splnone();
    config_delay();
    
    /*
     * Mark each "ibus" alive so installation sizer will see it.
     */
    for (i = 0; tc_sw.config_order[i] != -1; i++) {
	index = tc_sw.config_order[i];
	config_set_alive("ibus", tc_slot[index].slot, 0, 0);
    }
    
    /*
     * Go thru the tc_slot table and call ib_config_dev for each
     *   device and ib_config_cont for each controller.
     * The ib_config_* routines will in turn call the probe and attach
     *   routines for each device/controller.
     * If its an adapter, we get the name of the config routine from
     *   the maxoption data table.
     */
    for (i = 0; tc_sw.config_order[i] != -1; i++) {
	index = tc_sw.config_order[i];
	if (tc_slot[index].unit >= 0) {	/* have a valid table entry */
	    if (tc_slot[index].class == TC_CTLR) {
		if (!(ib_config_cont(PHYS_TO_K1(tc_slot[index].physaddr),
				     tc_slot[index].physaddr, tc_slot[index].slot, 
				     tc_slot[index].devname, 0))) {
		    printf("%s%d not probed\n",
			   tc_slot[index].devname, tc_slot[index].unit);
		    (*tc_sw.disable_option)(index);
		} else {
		    if (tc_slot[index].intr_aft_attach) {
			(*tc_sw.enable_option)(index);
		    } else {
			(*tc_sw.disable_option)(index);
		    }
		}
	    } else if (tc_slot[index].class == TC_DEV) {
		if (!(ib_config_dev(PHYS_TO_K1(tc_slot[index].physaddr),
				    tc_slot[index].physaddr, tc_slot[index].slot,
				    tc_slot[index].devname, 0))) {
		    printf("%s%d not probed\n",
			   tc_slot[index].devname, tc_slot[index].unit);
		    (*tc_sw.disable_option)(index);
		} else {
		    if (tc_slot[index].intr_aft_attach) {
			(*tc_sw.enable_option)(index);
		    } else {
			(*tc_sw.disable_option)(index);
		    }
		}
	    } else if (tc_slot[index].class == TC_ADPT) {
		if (!(*(tc_slot[index].adpt_config))(PHYS_TO_K1(tc_slot[index].physaddr),
						    tc_slot[index].physaddr,
						    tc_slot[index].slot,
						    tc_slot[index].unit,
						    &tc_slot[index].intr)) {
		    printf("%s%d not probed\n",
			   tc_slot[index].devname, tc_slot[index].unit);
		    (*tc_sw.disable_option)(index);
		} else {
		    if (tc_slot[index].intr_aft_attach) {
			(*tc_sw.enable_option)(index);
		    } else {
			(*tc_sw.disable_option)(index);
		    }
		}
	    }
	}
    }
}

/*
 * Search thru the tc_slot table to see if the given func address has
 *   already been used (already in the table).  Return 1 if used, 0 if not.
 * This is for devices & controllers that have the ibus number wildcarded.
 */
tc_func_used(intr)
	int (*intr)();		/* the interrupt routine for this device */
{
	register int i;

	for (i = 0; i < TC_IOSLOTS; i++) {
		if (tc_slot[i].intr == intr)
			return(1);
	}
	return (0);
}

tc_isolate_memerr(memerr_status)
	struct tc_memerr_status *memerr_status;
{
    if (memerr_status->va == 0) 
	memerr_status->va = (caddr_t)PHYS_TO_K1(memerr_status->pa);

    if (tc_sw.isolate_memerr)
	return ((*tc_sw.isolate_memerr)(memerr_status));
    else
	return (-1);
}
