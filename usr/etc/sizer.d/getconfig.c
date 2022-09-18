#ifndef lint
static char *sccsid = "@(#)getconfig.c	4.6      (ULTRIX)  12/6/90";
#endif lint
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


/************************************************************************
 *
 * Name: getconfig.c
 *
 * Modification History
 *
 * Nov 28, 1990 - Darrell Dunnuck
 *	Changed sizer so that it will not add a line containing
 *	"pseudo-device scsnet" if /install.tmp/.config exists and
 *	contains any pseudo-devices.  This is to keep from getting
 *	two "pseudo-device scsnet" lines in the config file.
 *
 * Oct 15, 1990 - Paul Grist
 *      Added additional support for Prestoserve at installation to
 *      make pr0 at istallation time if nvram is present.
 *
 * Sept 4, 1990 - Vince Wallace
 *	Added option & pseudo-device netman to the default config file.
 * 
 * Aug 10, 1990 - Randall Brown
 *	Added support for xcons and ws pseudo-devices. Only used on mips side
 *
 * May 21, 1990 - Robin
 *	Added presto NVRAM suport.
 *
 * Dec 12, 1989 - Alan Frechette
 *	For all SCSI controllers allow full blown configuration of all
 *	possible scsi devices. Also only make the MAKEDEV entries for
 *	the scsi devices which are alive.
 *
 * Nov 3, 1989 - Alan Frechette
 *	Added config file lines "maxdsiz 64" and "smmax 1024" for 3MAX.
 *
 * Oct 11, 1989 - Alan Frechette
 *	Fixed sizer to no longer get the CSR for a UNIBUS controller
 *	from a UNIBUS device structure. Allow DECNET option for VAX
 *	as well as for MIPS.
 *
 * July 14, 1989 - Alan Frechette
 *	Added (XMINODE) check and support for RIGEL (VAX_6400).
 *
 * July 10, 1989 - Alan Frechette
 *	Make sure we always check for SMP option all the time.
 *
 * July 6, 1989 - Alan Frechette
 *	Added check for a VAXSTAR cputype.
 *
 * May 10, 1989 - Alan Frechette
 *	Added option SMP.
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

/****************************************************************
* Name:		getconfig					*
*    								*
* Abstract:	Create the configuration file for the system.	*
* 		Create the makedevices file for the system.	*
*								*
* Inputs:							*
* sysname	The configuration file name for the system.	*
* tzone		The timezone to be use if specified.		*
*								*
* Outputs:	None.						*
*								*
* Return Values: None.						*
*								*
* Side Effects:	The configuration file and the makedevices file	*
*		are  created in  the /tmp directory under the	*
*		following pathnames:				*
*		1) Configuration File is under /tmp/sysname.	*
*		2) Makedevices File is under /tmp/sysname.devs.	*
*								*
*		Many other routines are called to handle and to	*
*		get the configuration file information.		*
****************************************************************/
getconfig(sysname, tzone)
char *sysname;
char *tzone;
{
	int index, physmem, maxcpu, tz, dst, xosflg, scs_sysid, defopts;
	int nvram_size, ws = 0;
	char devpath[PATHSIZE], path[PATHSIZE];
	char command[80];

	scs_sysidflag = float_flag = defopts = 0;
	/* Check for a valid configuration file name */
	checksysname(sysname);

	/* Create the makedevices file */
	sprintf(devpath, "/tmp/%s.devs", sysname);
   	if((fpdevs = fopen(devpath, "w")) == NULL) {
		fprintf(stderr, "Cannot open (%s).\n", devpath);
		quitonerror(-10);
	}

	/* Create the configuration file */
	sprintf(path, "/tmp/%s", sysname);
	if((fp = fopen(path, "w")) == NULL) {
		fprintf(stderr, "Cannot open (%s).\n",path);
		quitonerror(-11);
	}

	/* Get the "cpu" information and "physical memory" size */
	index = getcpu(NODISPLAY);
	maxcpu = getmaxcpu();
	physmem = getphysmem();
	fprintf(fp, "ident\t\t\"%s\"\n", sysname);
#ifdef vax
	fprintf(fp, "machine\t\tvax\n");
#endif vax
#ifdef mips
	fprintf(fp, "machine\t\tmips\n");
#endif mips
	fprintf(fp, "cpu\t\t\"%s\"\n", cputbl[index].cpuname);
	fprintf(fp, "maxusers\t%d\n", cputbl[index].maxusers);
	fprintf(fp, "processors\t%d\n", maxcpu);
	fprintf(fp, "maxuprc\t\t%d\n", MAXUPRC);
	fprintf(fp, "physmem\t\t%d\n", physmem);

	/* Get the "timezone" information */
	if(tzone != NULL && strlen(tzone))
		fprintf(fp, "timezone\t%s\n", tzone);
	else {
		gettimezone(&tz, &dst);
		fprintf(fp, "timezone\t%d dst %d\n", tz, dst);
	}

	/* Special default entries for 3MAX and 3MIN */
	if ((CPU == DS_5000) || (CPU == DS_5000_100)) {
		fprintf(fp, "maxdsiz\t\t64\n");
		fprintf(fp, "smmax\t\t1024\n");
	}
	fprintf(fp, "\n");

	/* Get any "options" from the /install.tmp/.config file */
	if(!getconfig_string("options")) {
		fprintf(fp, "options\t\tLAT\n");
		fprintf(fp, "options\t\tQUOTA\n");
		fprintf(fp, "options\t\tINET\n");
		fprintf(fp, "options\t\tEMULFLT\n");
		fprintf(fp, "options\t\tNFS\n");
		fprintf(fp, "options\t\tRPC\n");
		fprintf(fp, "options\t\tDLI\n");
		fprintf(fp, "options\t\tNETMAN\n");
#ifdef vax
		fprintf(fp, "options\t\tBSC\n");
#endif vax
		fprintf(fp, "options\t\tUFS\n");
		fprintf(fp, "options\t\tDECNET\n");
		defopts = 1;
	}
	if(maxcpu > 1)
		fprintf(fp, "options\t\tSMP\n");

	/* Get the workstation display type "options" */
	xosflg=get_X();
	if(xosflg==1 || xosflg==2)
		fprintf(fp, "options\t\tUWS\n");
	if(xosflg==2)
		fprintf(fp, "options\t\tXOS\n");
	fprintf(fp, "\n");

#ifdef mips
	/* Get default "makeoptions" that are needed */
	fprintf(fp, "makeoptions\tENDIAN=\"-EL\"\n");
	fprintf(fp, "\n");
#endif mips

	/* Get the "root", "swap", and "dump" devices */
	getroot(NODISPLAY);

	/* Get all "UNIBUS/QBUS", "MASSBUS" and "FLOATING" devices */

	fprintf(fpdevs, "MAKEDEV  %s ",BOOT);
	getdevices();

	/* Get any "hardware" from the /install.tmp/.config file */
	getconfig_string("hardware");

	/* Get the "scs_sysid" information */
	if(scs_sysidflag) 
		fprintf(fp, "\nscs_sysid\t%d\n",getsysid());
	else
		fprintf(fp, "\nscs_sysid\t 1\n");
	fprintf(fp, "\n");

	/* Get any "pseudo-devices" from the /install.tmp/.config file */
	if(!getconfig_string("pseudo-device")) {
		fprintf(fp, "pseudo-device\tpty\n");
		fprintf(fp, "pseudo-device\tloop\n");
		fprintf(fp, "pseudo-device\tinet\n");
		fprintf(fp, "pseudo-device\tether\n");
		fprintf(fp, "pseudo-device\tlat\n");
		fprintf(fp, "pseudo-device\tlta\n");
		fprintf(fp, "pseudo-device\trpc\n");
		fprintf(fp, "pseudo-device\tnfs\n");
		fprintf(fp, "pseudo-device\tdli\n");
		fprintf(fp, "pseudo-device\tnetman\n");
#ifdef vax
		fprintf(fp, "pseudo-device\tbsc\n");
#endif vax
		fprintf(fp, "pseudo-device\tufs\n");
		fprintf(fp, "pseudo-device\tdecnet\n");
	if(scs_sysidflag)
		fprintf(fp, "pseudo-device\tscsnet\n");
	}
	nvram_size = getnvram();
	if(nvram_size != 0) {
		fprintf(fp, "pseudo-device\tpresto\n");
		fprintf(fpdevs, "pr0 ");
	}
	if(xosflg==2) {
		fprintf(fp, "pseudo-device\txos\n");
		fprintf(fpdevs, "xos ");
	}

#ifdef mips
	getsysinfo(GSI_WSD_TYPE, &ws, sizeof(ws));
	if (ws > 0) {
	    fprintf(fp, "pseudo-device\txcons\n");
	    if (ws == WS_DTYPE)
		fprintf(fp, "pseudo-device\tws\n");
	    fprintf(fpdevs, "xcons ");
	}
#endif mips

	/* Search the /install.tmp/.config file for any "user options" */
	if(search_config("options", "LAT") || defopts)
		fprintf(fpdevs, "lta0 ");
	if(search_config("options", "PACKETFILTER"))
		fprintf(fpdevs, "pfilt ");
	if(search_config("options", "AUDIT"))
		fprintf(fpdevs, "audit ");
	fprintf(fpdevs, "\n");
	fclose(fpdevs);
	fclose(fp);
	fprintf(stdout, "\nConfiguration file complete.\n");
}

/****************************************************************
* Name:		getdevices					*
*    								*
* Abstract:	Get the device information for the given system.* 
*		This routine will find all the alive ADAPTERS, 	*
*		CONTROLLERS, and DEVICES for the given system 	*
*		and place them into the alive device list table *
*		"adltbl[]". This is accomplished by reading the	*
*		device information from the appropriate kernel 	*
*		data structures in kernel memory. The I/O space	*
*		for the system is determined at system bootup	*
*		time during kernel autoconfiguration. The kernel*
*		autconfiguration code probes the I/O space to 	*
*		find all the alive devices in the system and it	*
*		updates its data structures accordingly.	*
*								*
* Inputs:	None.						*
*								*
* Outputs:	None.						*
*								*
* Return Values: None.						*
*								*
* Side Effects:	Many other routines are called to handle the	*
*		sizing of the system I/O space in order to     	*
*		find all the alive devices in the system. The 	*
*		following routines are called:			*
*								*
* get_config_adpt()	Gets ADAPTER, CONTROLLER, DEVICE info.	*
* get_uba_device()	Gets UNIBUS device info.		*
* get_nummbas()		Gets number of MASSBUS ADAPTERS.	*
* get_mba_device()	Gets MASSBUS device info.		*
* get_mba_slave()	Gets MASSBUS slave device info.		*
* getfloat()		Gets UNIBUS FLOATING device info.	*
* add_alive_device()	Adds entry to alive device table.	*
* printdevice()		Prints device info to the config file.	*
****************************************************************/
getdevices()
{

    struct config_adpt adapter, *c_adpt;
    struct uba_device ubadevice, *ui;
    struct uba_ctlr   *um;
    struct mba_device mbadevice, *mi;
    struct mba_slave mbaslave, *ms;
    int i, saveindex;
    int numubas, nummbas;

    /* Initialize */
    c_adpt = &adapter;
    ui = &ubadevice;
    mi = &mbadevice;
    ms = &mbaslave;
    numubas = nummbas = ADLINDEX = 0;
    adapter_offset = reset_anythg(NL_config_adpt);
    adapter_cnt = 0;

    /*
     * Loop through the "config_adpt" structure and find all
     * the alive adapters, controllers, and devices on the
     * UNIBUS.
     */
    while(get_config_adpt(c_adpt)) {
	if(c_adpt->p_num == (int) '?' && strcmp(c_adpt->p_name,"nexus") != 0)
	    continue;
	if(c_adpt->c_num == -1 || c_adpt->c_ptr == NULL)
	    continue;

	/* Found an ADAPTER type reference */
        if(c_adpt->c_type == 'A') {
		if(strcmp(c_adpt->c_name, "uba") == 0)
		    numubas++;
	    	add_alive_device(UNIBUS, ADAPTER, c_adpt, 0, 0, 0, 0);
        }

	/* Found a CONTROLLER type reference */
        else if(c_adpt->c_type == 'C') {
	    um = (struct uba_ctlr *)c_adpt->c_ptr;
	    if(um == NULL || um->um_alive == 0)
	        continue;
	    saveindex = ADLINDEX;
	    add_alive_device(UNIBUS, CONTROLLER, c_adpt, um, 0, 0, 0);
	    
    	    /*
     	     * Now loop through the "ubdinit" structure and find all
     	     * the alive devices which are attached to this CONTROLLER.
     	     */
    	    udevice_offset = reset_anythg(NL_ubdinit);
    	    udevice_cnt = 0;
	    while(get_uba_device(ui)) {
	        if(ui->ui_alive == 0) {
		    switch(CPU) {
		    case DS_3100:
		    case DS_5100:
		    case DS_5000:
		    case DS_5000_100:
		    case C_VAXSTAR:
		    case VAX_60:
	        	if(strcmp(ui->ui_devname,"rz") != 0 && 
				strcmp(ui->ui_devname, "tz") != 0)
			    continue;
			break;

		    default:
			continue;
			break;
		    }
		}

		/* Check if device belongs to this controller */
		if((um->um_ctlr == ui->ui_ctlr) &&
		    		(um->um_driver == ui->ui_driver))
		    add_alive_device(UNIBUS, DEVICE, 0, um, ui, 0, 0);
    	    }
        }

	/* Found a DEVICE type reference */
        else if(c_adpt->c_type == 'D') {
	    ui = (struct uba_device *)c_adpt->c_ptr;
	    if(ui == NULL || ui->ui_alive == 0)
	        continue;
	    add_alive_device(UNIBUS, DEVICE, c_adpt, 0, ui, 0, 0);
        }
    }

    /*
     * Now loop through the "mbdinit" and "mbsinit" structures 
     * and find all the alive controllers and devices on the 
     * MASSBUS. This is done only if the system has any MASSBUS
     * ADAPTERS. The variable "nummbas" is set to the number of
     * MASSBUS ADAPTERS the system has.
     */
    if(nl[NL_mbhead].n_type != N_UNDF)
        nummbas = get_nummbas();	
    adapter.c_name = "mba";
    adapter.p_name = "nexus";
    adapter.p_num = (int) '?';
    for(i=0; i<nummbas; i++) {
	adapter.c_num = i;
	add_alive_device(MASSBUS, ADAPTER, &adapter, 0, 0, 0, 0);
    }

    /* Find number of MASSBUS DEVICE CONTROLLERS */
    mdevice_offset = reset_anythg(NL_mbdinit);
    mdevice_cnt = 0;
    while(nl[NL_mbdinit].n_type != N_UNDF && nummbas && get_mba_device(mi)) {
    	if(mi->mi_alive == 0)
    	    continue;
        add_alive_device(MASSBUS, CONTROLLER, 0, 0, 0, mi, 0);
    }

    /* Find number of MASSBUS SLAVE DEVICES */
    mslave_offset = reset_anythg(NL_mbsinit);
    mslave_cnt = 0;
    while(nl[NL_mbsinit].n_type != N_UNDF && nummbas && get_mba_slave(ms)) {
	if(ms->ms_alive == 0)
	    continue;
	add_alive_device(MASSBUS, DEVICE, 0, 0, 0, 0, ms);
    }

    /* Now find devices in FLOATING ADDRESS SPACE */
    getfloat(numubas);

    /* Print out configuration information for all device types */
    printdevice();
}

/****************************************************************
* Name:		printdevice					*
*    								*
* Abstract:	Print out the alive device information to the 	*
*		configuration file. This routine loops through	*
*		the alive device list table "adltbl[]" and 	*
*		prints out the alive ADAPTERS, CONTROLLERS, and *
*		DEVICES information to the configuration file.	*
*								*
* Inputs:	None.						*
*								*
* Outputs:	None.						*
*								*
* Return Values: None.						*
****************************************************************/
printdevice()
{

	struct alive_device_list *adl;
	int index, nodeindex, i;
	char tdevname[DEVNAMESIZE], tconname[DEVNAMESIZE];
	char *unsupported = "#UNSUPPORTED";
	char *harddevstr;

	/* Loop through the alive device linked list */
	for(i=0; i<ADLINDEX; i++) {
		/* Get device index and node index */
		index = adltbl[i].device_index;
		nodeindex = adltbl[i].node_index;

		/* Print out for an unsupported device */
		if(index == -1) {
			sprintf(tdevname, "%s", adltbl[i].unsupp_devname);
			if(adltbl[i].conn_number == (int) '?')
			    sprintf(tconname, "%s?", adltbl[i].conn_name);
			else
			    sprintf(tconname, "%s%d",
		    	    	adltbl[i].conn_name, adltbl[i].conn_number);
			fprintf(fp, "%-16s%-10s at %-10s", unsupported,
					tdevname, tconname);
			if(nodeindex != -1) {
		    	    if(nodetbl[nodeindex].nodetype == VAXBINODE)
	        		fprintf(fp, "%s%d  ", 
					nodetbl[nodeindex].nodename,
						adltbl[i].node_number);
		    	    else if(nodetbl[nodeindex].nodetype == XMINODE)
	        		fprintf(fp, "%s%d  ", 
					nodetbl[nodeindex].nodename,
						adltbl[i].node_number);
		    	    else if(nodetbl[nodeindex].nodetype == CINODE)
	        		fprintf(fp, "%s %d  ", 
					nodetbl[nodeindex].nodename,
						adltbl[i].rctlr);
		    	    else if(nodetbl[nodeindex].nodetype == MSINODE)
	        		fprintf(fp, "%s %d  ", 
					nodetbl[nodeindex].nodename,
						adltbl[i].rctlr);
			}
			if(adltbl[i].csr != -1)
		    		printcsr(i);
			if(adltbl[i].device_drive != -1)
				fprintf(fp, "drive %d", adltbl[i].device_drive);
			fprintf(fp, "\n");
			continue;
		}

		/* Print out the device type */
		harddevstr = hardtbl[devtbl[index].devtype].typename;
		if(devtbl[index].supportedflag)
		    	fprintf(fp, "%-16s", harddevstr);
		else
		    	fprintf(fp, "#%-16s", harddevstr);

		/* Print out the device name and the connection name */
		sprintf(tdevname, "%s%d",
				devtbl[index].devname, adltbl[i].device_unit);
		if(devtbl[index].devtype == ADAPTER &&
				adltbl[i].conn_number == (int) '?')
			sprintf(tconname, "%s?", adltbl[i].conn_name);
		else
			sprintf(tconname, "%s%d",
		    	    	adltbl[i].conn_name, adltbl[i].conn_number);
		if(strncmp(tconname,"ibus",4) == 0)
			strcpy(tconname, "ibus?");
		fprintf(fp, "%-10s at %-10s", tdevname, tconname);

		/* If device is a "ci" adapter then set "scs_sysidflag" */
		if(devtbl[index].devtype == ADAPTER &&
				strcmp(devtbl[index].devname,"ci") == 0)
			scs_sysidflag++;

		/* Print out the node type if one exists */
		if(nodeindex != -1) {
		    if(nodetbl[nodeindex].nodetype == VAXBINODE)
	        	fprintf(fp, "%s%d  ", nodetbl[nodeindex].nodename,
					adltbl[i].node_number);
		    else if(nodetbl[nodeindex].nodetype == XMINODE)
	        	fprintf(fp, "%s%d  ", nodetbl[nodeindex].nodename,
					adltbl[i].node_number);
		    else if(nodetbl[nodeindex].nodetype == CINODE)
	        	fprintf(fp, "%s %d  ", nodetbl[nodeindex].nodename,
					adltbl[i].rctlr);
		    else if(nodetbl[nodeindex].nodetype == MSINODE)
	        	fprintf(fp, "%s %d  ", nodetbl[nodeindex].nodename,
					adltbl[i].rctlr);
		}

		/* Print out the csr if one exists */
		if(adltbl[i].csr != -1)
		    	printcsr(i);

		/* Print out the flags if any exists */
	    	if(devtbl[index].flags)
			fprintf(fp, "flags 0x%x  ", devtbl[index].flags);

		/* Print out the interrupt vectors if any exists */
	    	if(strlen(devtbl[index].ivectors) != 0)
			fprintf(fp, "vector %s ", devtbl[index].ivectors);

		/* Print out the drive or slave number if one exists */
		if(adltbl[i].device_drive != -1) {
		    if((adltbl[i].bus_type == MASSBUS) && 
			(devtbl[index].devtype == TAPE))
			fprintf(fp, "slave %d", adltbl[i].device_drive);
		    else
			fprintf(fp, "drive %d", adltbl[i].device_drive);
		}
		fprintf(fp, "\n");

		/* Make special file for this device */
		if(devtbl[index].makedevflag && adltbl[i].alive_unit)
			printmakedev(i);
	}
}

/****************************************************************
* Name:		printcsr					*
*    								*
* Abstract:	Print out the CSR address of a device to the	*
*		configuration file.				*
*								*
* Inputs:							*
* i		The current index in the alive device table.	*
*								*
* Outputs:	None.						*
*								*
* Return Values: None.						*
****************************************************************/
printcsr(i)
int i;
{

	long csr;
	int index, j;
	char tdevname[DEVNAMESIZE], tconname[DEVNAMESIZE];

	index = adltbl[i].device_index;
	if(index != -1 && strcmp(devtbl[index].devname,"idc") == 0)
		adltbl[i].csr = 0175606;

	/* Print out the CSR address for this device */
	switch(CPU) {
	case MVAX_I:
	case MVAX_II:
	case VAXSTAR:
	case VAX_3400:
	case VAX_3600:
	case VAX_3900:
	case C_VAXSTAR:
	case VAX_60:
	case DS_5400:
	case DS_5500:
	case DS_5800:
		if(CPUSUB == ST_VAXSTAR) {
			csr = adltbl[i].csr;
			fprintf(fp, "csr 0x%8x  ", csr);
		}
		else {
			csr = adltbl[i].csr;
			csr |= 0160000;
			fprintf(fp, "csr 0%6o  ", csr);
		}
		break;
	default:
		csr = adltbl[i].csr;
		fprintf(fp, "csr 0%6o  ", csr);
		break;
	}

	/* Check for a floating address device */
	if(csr > 0160000 && csr < 0170000) {
		if(!float_flag) {
			fprintf(stdout, "The installation software found ");
			fprintf(stdout, "these devices in the floating\n");
			fprintf(stdout, "address space:\n\n");
			float_flag = 1;
		}
		if(index != -1) {
			if(devtbl[index].devtype == CONTROLLER)
				j = i - 1;
			else
				j = i;
			index = adltbl[j].device_index;
			sprintf(tdevname, "%s%d",
				devtbl[index].devname, adltbl[j].device_unit);
		}
		else {
			j = i;
			sprintf(tdevname, "%s", adltbl[j].unsupp_devname);
		}
		sprintf(tconname, "%s%d",
		    		adltbl[j].conn_name, adltbl[j].conn_number);
		fprintf(stdout, "\t%-10s\t", tdevname);
		fprintf(stdout, "on %-10s\t", tconname);
		fprintf(stdout, "at 0%o\n", csr);
	}
}

/****************************************************************
* Name:		printmakedev					*
*    								*
* Abstract:	Print out the MAKEDEVICE information to the 	*
*		makedevices file.				*
*								*
* Inputs:							*
* i		The current index in the alive device table.	*
*								*
* Outputs:	None.						*
*								*
* Return Values: None.						*
****************************************************************/
printmakedev(i)
int i;
{

	int index, number;
	long offset;
	char devname[20];

	/* Print the the MAKEDEV entry for this device */
	index = adltbl[i].device_index;
	sprintf(devname, "%s%d", devtbl[index].devname,
			adltbl[i].device_unit);
	switch(CPU) {
	case MVAX_I:
	case MVAX_II:
	case VAXSTAR:
	case VAX_3400:
	case VAX_3600:
	case VAX_3900:
	case C_VAXSTAR:
	case VAX_60:
	case DS_5400:
	case DS_5500:
	case DS_5800:
		/* Special cases for "dz" and "dhu" devices */
		if(strcmp(devtbl[index].devname, "dz") == 0)
		    sprintf(devname, "dzv%d", adltbl[i].device_unit);
		else if(strcmp(devtbl[index].devname, "dhu") == 0)
		    sprintf(devname, "dhv%d", adltbl[i].device_unit);
		break;
	default:
		break;
	}

	/* Special cases for "dmb" devices */
	if(strcmp(devtbl[index].devname, "dmb") == 0) {
		number = adltbl[i].device_unit;
		offset = reset_anythg(NL_dmb_lines);
		offset = lseek(kmem,offset+number*4, 0);
		read(kmem, &number, sizeof(number));
		if(number == 16)
			sprintf(devname,"dhb%d",adltbl[i].device_unit);
		else
			sprintf(devname,"dmb%d",adltbl[i].device_unit);
	}
	fprintf(fpdevs, "%s  ", devname);
}
