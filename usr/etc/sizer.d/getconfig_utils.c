#ifndef lint
static char *sccsid = "@(#)getconfig_utils.c	4.1    ULTRIX  7/2/90";
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
 * Name: getconfig_utils.c
 *
 * Modification History
 * 
 * Jan 12, 1990 - Alan Frechette
 *	Initialize "alive_unit" field in init_adl_entry().
 *
 * Dec 12, 1989 - Alan Frechette
 *	Set the "alive_unit" field in the alive device structure.
 *
 * Oct 11, 1989 - Alan Frechette
 *	Fixed sizer to get the CSR for a UNIBUS controller from the
 *	UNIBUS controller structure instead of from the UNIBUS device
 *	structure.
 *
 * July 14, 1989 - Alan Frechette
 *	Fixed handling of unsupported devices not found in the
 *	configuration device table "config_device[]". Add check
 *	for (XMINODE) in "check_connection()". Null out "pname"
 *	and "cname" in "get_config_adpt()".
 *
 * July 6, 1989 - Alan Frechette
 *	Added call to "check_connection()" for a "ci" adapter
 *	attached to a "vaxbi" or "xmi".
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
* Name:		add_alive_device				*
*    								*
* Abstract:	Add an alive device to the alive device list	*
*		table "adltbl[]".				*
*								*
* Inputs:							*
* bustype	The bus type of the device (UNIBUS, MASSBUS).	*
* devtype	The device type (ADAPTER, CONTROLLER, DEVICE).	*
* c_adpt	The "config_adpt" adapter structure.		*
* um		The "uba_ctlr" controller structure.		*
* ui		The "uba_device" device structure.		*
* mi		The "mba_device" device structure.		*
* ms		The "mba_slave" slave device structure.		*
*								*
* Outputs:	None.						*
*								*
* Return Values: None.						*
*								*
* Side Effects:	If the alive device is not configured in the	*
*		configuration device table "devtbl[]" then a	*
*		message will be printed out to the user that	*
*		this device will be ignored. This following	*
*		routines are also called:			*
* 								*
* init_adl_entry()	Initialize an alive device table entry.	*
* match_device_name()	Search device name in device table.	*
* match_node_name()	Search connection name in node table.	*
* check_connection()	Check for a VAXBI/XMI connection point.	*
****************************************************************/
add_alive_device(bustype, devtype, c_adpt, um, ui, mi, ms)
int bustype;
int devtype;
struct config_adpt *c_adpt;
struct uba_ctlr *um;
struct uba_device *ui;
struct mba_device *mi;
struct mba_slave *ms;
{

    struct mba_driver *md;
    int index, nodeindex;

    /* Initialize an allocate an alive device list entry */
    init_adl_entry(ADLINDEX);
    index = -1;

    switch(devtype) {
    case ADAPTER:	/* Store information about an ADAPTER */
	index = match_device_name(c_adpt->c_name);
	if(index == -1)
	    sprintf(adltbl[ADLINDEX].unsupp_devname,
			"%s%d", c_adpt->c_name, c_adpt->c_num);
	strcat(adltbl[ADLINDEX].conn_name,c_adpt->p_name);
	adltbl[ADLINDEX].bus_type = bustype;
	adltbl[ADLINDEX].conn_number = c_adpt->p_num;
	adltbl[ADLINDEX].device_index = index;
	adltbl[ADLINDEX].device_unit = c_adpt->c_num;
	adltbl[ADLINDEX].alive_unit = 1;
        check_connection(c_adpt->p_name, c_adpt->p_num, 
				c_adpt->c_nexus_num, ADLINDEX);
	break;
    
    case CONTROLLER:	/* Store information about a CONTROLLER */
	if(bustype == MASSBUS) {
	    md = mi->mi_driver;
	    index = match_device_name(md->md_dname);
	    if(index == -1)
		sprintf(adltbl[ADLINDEX].unsupp_devname,
				"%s%d", md->md_dname, mi->mi_unit);
	    adltbl[ADLINDEX].bus_type = bustype;
	    strcat(adltbl[ADLINDEX].conn_name,"mba");
	    adltbl[ADLINDEX].conn_number = mi->mi_mbanum;
	    adltbl[ADLINDEX].device_index = index;
	    adltbl[ADLINDEX].device_unit = mi->mi_unit;
	    adltbl[ADLINDEX].device_drive = mi->mi_drive;
	    adltbl[ADLINDEX].alive_unit = mi->mi_alive;
	}
	else if(bustype == UNIBUS) {
	    index = match_device_name(um->um_ctlrname);
	    if(index == -1)
		sprintf(adltbl[ADLINDEX].unsupp_devname,
				"%s%d", um->um_ctlrname, um->um_ctlr);
	    nodeindex = match_node_name(c_adpt->p_name);
	    adltbl[ADLINDEX].bus_type = bustype;
	    strcat(adltbl[ADLINDEX].conn_name,c_adpt->p_name);
	    adltbl[ADLINDEX].conn_number = c_adpt->p_num;
	    adltbl[ADLINDEX].device_index = index;
	    adltbl[ADLINDEX].device_unit = um->um_ctlr;
	    adltbl[ADLINDEX].alive_unit = um->um_alive;
	    if(nodeindex >= 0) {
	    	adltbl[ADLINDEX].rctlr = um->um_rctlr;
		adltbl[ADLINDEX].node_index = nodeindex;
	    	adltbl[ADLINDEX].node_number = um->um_nexus;
	    }

	    /* Get the CSR address for this controller */
	    if(CPUSUB == ST_VAXSTAR)
		adltbl[ADLINDEX].csr = (u_long) um->um_physaddr;
	    else
		adltbl[ADLINDEX].csr = (u_short) um->um_physaddr;

            check_connection(c_adpt->p_name, c_adpt->p_num, 
					um->um_nexus, ADLINDEX);
	}
	break;
    
    case DEVICE:	/* Store information about a DEVICE */
	if(bustype == MASSBUS) {
	    md = ms->ms_driver;
	    index = match_device_name(md->md_sname);
	    if(index == -1)
		sprintf(adltbl[ADLINDEX].unsupp_devname,
				"%s%d", md->md_sname, ms->ms_unit);
	    adltbl[ADLINDEX].bus_type = bustype;
	    strcat(adltbl[ADLINDEX].conn_name,md->md_dname);
	    adltbl[ADLINDEX].conn_number = ms->ms_ctlr;
	    adltbl[ADLINDEX].device_index = index;
	    adltbl[ADLINDEX].device_unit = ms->ms_unit;
	    adltbl[ADLINDEX].device_drive = ms->ms_slave;
	    adltbl[ADLINDEX].alive_unit = ms->ms_alive;
	}
	else if(bustype == UNIBUS) {
	    index = match_device_name(ui->ui_devname);
	    if(index == -1)
		sprintf(adltbl[ADLINDEX].unsupp_devname,
				"%s%d", ui->ui_devname, ui->ui_unit);
	    adltbl[ADLINDEX].bus_type = bustype;
	    if(c_adpt != NULL) {
	        strcat(adltbl[ADLINDEX].conn_name,c_adpt->p_name);
	        adltbl[ADLINDEX].conn_number = c_adpt->p_num;
	    }
	    if(um != NULL) {
	        strcat(adltbl[ADLINDEX].conn_name,um->um_ctlrname);
	        adltbl[ADLINDEX].conn_number = um->um_ctlr;
	    }
	    nodeindex = match_node_name(adltbl[ADLINDEX].conn_name);
	    adltbl[ADLINDEX].device_index = index;
	    adltbl[ADLINDEX].device_unit = ui->ui_unit;
	    adltbl[ADLINDEX].device_drive = ui->ui_slave;
	    adltbl[ADLINDEX].alive_unit = ui->ui_alive;
	    if(nodeindex >= 0) {
	        adltbl[ADLINDEX].node_number = ui->ui_nexus;
		adltbl[ADLINDEX].node_index = nodeindex;
	    }
	    if(strcmp(adltbl[ADLINDEX].conn_name, "uba") == 0) {
		/* Get the CSR address for this device */
		if(CPUSUB == ST_VAXSTAR)
		    adltbl[ADLINDEX].csr = (u_long) ui->ui_addr;
		else
		    adltbl[ADLINDEX].csr = (u_short) ui->ui_physaddr;
	    }
	    if(c_adpt != NULL)
	        check_connection(c_adpt->p_name, c_adpt->p_num, 
					ui->ui_nexus, ADLINDEX);
	}
	break;

    default:
        ADLINDEX--;
	break;
    }
    ADLINDEX++;
}

/****************************************************************
*  Name:	match_device_name				*
*    								*
*  Abstract:	Search for the given device name in the 	*
*		configuration device table "devtbl[]".		*
*								*
*  Inputs:							*
*  devname	The device name to search for.			*
*								*
*  Outputs:	None.						*
*								*
*  Return Values: 						*
*  i		The index into the device table "devtbl[]".	*
****************************************************************/
match_device_name(devname)
char *devname;
{

    int i;

    /* Search for device name in "devtbl" table */
    for(i=0; i<DEVTBLSIZE; i++) {
	if(strcmp(devname, devtbl[i].devname) == 0)
	    return(i);
    }
    return(-1);
}

/****************************************************************
*  Name:	match_node_name					*
*    								*
*  Abstract:	Search for the given connection name in the 	*
*		configuration node table "nodetbl[]".		*
*								*
*  Inputs:							*
*  conn_name	The connection name to search for.		*
*								*
*  Outputs:	None.						*
*								*
*  Return Values: 						*
*  i		The index into the node table "nodetbl[]".	*
****************************************************************/
match_node_name(conn_name)
char *conn_name;
{

    int i;

    /* Search for connection name in "nodetbl" table */
    for(i=0; i<NODETBLSIZE; i++) {
	if(strcmp(conn_name, nodetbl[i].conn_name) == 0)
	    return(i);
    }
    return(-1);
}

/****************************************************************
*  Name:	check_connection				*
*    								*
*  Abstract:	Search for the final connection point of a 	*
*		particular device. If the final connection 	*
*		point is itself at a VAXBI or XMI connection  	*
*		then we set the node number for this VAXBI or	*
*		XMI connection. Also if the final connection 	*
*		point is not at a UNIBUS ADAPTER then we set 	*
*		the CSR equal to -1.				*
*								*
*    	(EX)	adapter	     vaxbi2  	at  nexus?		*
*		controller   kdb0  	at  vaxbi2	node?	*
*		controller   uq4  	at  kdb0		*
*								*
*    		In this example the device is "uq4" and its 	*
*		final connection point is at "vaxbi2", so we 	*
*		need to patch the node number for this VAXBI 	*
*		connection.					*
*								*
*  Inputs:							*
*  pname	The connection point of a given device.		*
*  pnumber	The connection number of a given device.	*
*  nexus	The nexus number to set for the VAXBI/XMI node.	*
*  index	The index of the alive device list entry.	*
*								*
*  Outputs:	None.						*
*								*
*  Return Values: None.						*
****************************************************************/
check_connection(pname, pnumber, nexus, index)
char *pname;
int pnumber;
int nexus;
int index;
{

    int i, nodeindex;

    /*
     * Search through the alive device linked list looking
     * for the connection point of the given DEVICE passed
     * in as "pname" and "pnumber". If the connection point 
     * is itself at a VAXBI or XMI connection then we need 
     * to set the node number for this VAXBI or XMI connection.
     */
    for(i = 0; i < ADLINDEX; i++) {
	/* Check if this is the DEVICE'S connection point */
	if((strcmp(devtbl[adltbl[i].device_index].devname, pname) == 0) && 
	    			(adltbl[i].device_unit == pnumber)) {
	    /*
	     * If the DEVICE's final connection point is itself
	     * at a VAXBI or XMI connection then patch the node 
	     * number. 
	     */
    	    nodeindex = match_node_name(adltbl[i].conn_name);
	    if(nodeindex != -1 && 
		(nodetbl[nodeindex].nodetype == VAXBINODE) ||
			(nodetbl[nodeindex].nodetype == XMINODE)) {
		adltbl[i].node_index = nodeindex;
	    	adltbl[i].node_number = nexus;
	    }
	    else if(nodeindex == -1 && 
		strcmp(adltbl[i].conn_name, "nexus") == 0) {
    	    	nodeindex = match_node_name(pname);
	    	if(nodeindex != -1 && 
		    (nodetbl[nodeindex].nodetype == VAXBINODE) ||
			(nodetbl[nodeindex].nodetype == XMINODE)) {
		    adltbl[index].node_index = nodeindex;
	    	    adltbl[index].node_number = nexus;
		}
	    }
	    /*
	     * If the DEVICE's final connection point is not
	     * attached to the UNIBUS ADAPTER then make sure 
	     * the CSR is set equal to -1.
	     */
	    if(strcmp(adltbl[i].conn_name, "uba") != 0 &&
			strcmp(adltbl[index].conn_name, "uba") != 0)
		adltbl[index].csr = -1;
	    break;
	}
    }
}

/****************************************************************
*  Name:	get_config_adpt					*
*    								*
*  Abstract:	Get entry from "config_adpt" structure.	This	*
*    		routine reads a CONFIG ADAPTER structure entry	*
*		from kernel memory.				*
*								*
*  Inputs:	None.						*
*								*
*  Outputs:							*
*  adpt		The "config_adpt" structure entry.		*
*								*
*  Return Values: 						*
*  1		More "config_adpt" structures to read.		*
*  0		No more "config_adpt" structures to read.	*
****************************************************************/
get_config_adpt(adpt)
struct config_adpt *adpt;
{

	/* Read "config_adpt" structure entry from kernel memory */
	adapter_offset = lseek(kmem,adapter_offset + adapter_cnt,0);
	adapter_cnt = read(kmem,adpt,sizeof(*adpt));

	/* Read the pname field from the "config_adpt" structure */
	lseek(kmem,adpt->p_name,0);
	pname[0] = NULL;
	adpt->p_name = &pname[0];
	read(kmem,adpt->p_name,DEVNAMESIZE);

	/* Read the cname field from the "config_adpt" structure */
	lseek(kmem,adpt->c_name,0);
	cname[0] = NULL;
	adpt->c_name = &cname[0];
	read(kmem,adpt->c_name,DEVNAMESIZE);

	/* Read in UNIBUS CONTROLLER information */
	if(adpt->c_type == 'C'  && adpt->c_ptr != NULL) {
		lseek(kmem,adpt->c_ptr,0);
		read(kmem,&ubctlr,sizeof(ubctlr));
		adpt->c_ptr = (char *)&ubctlr;
		/* Read the UNIBUS CONTROLLER name */
		lseek(kmem,ubctlr.um_ctlrname,0);
		ubctlr.um_ctlrname = &ubcname[0];
		read(kmem,ubctlr.um_ctlrname,DEVNAMESIZE);
	}
	/* Read in UNIBUS DEVICE information */
	else if(adpt->c_type == 'D' && adpt->c_ptr != NULL) {
		lseek(kmem,adpt->c_ptr,0);
		read(kmem,&ubdevice,sizeof(ubdevice));
		adpt->c_ptr = (char *)&ubdevice;
		/* Read the UNIBUS DEVICE name */
		lseek(kmem,ubdevice.ui_devname,0);
		ubdevice.ui_devname = &ubdname[0];
		read(kmem,ubdevice.ui_devname, DEVNAMESIZE);
	}

	/* Have we finished reading the "config_adpt" list */
	return((adpt->p_name[0] != NULL) ? 1 : 0);
}

/****************************************************************
*  Name:	get_uba_device					*
*    								*
*  Abstract:	Get "uba_device" entry from "ubdinit" structure.*
*    		This routine reads a UNIBUS device structure	*
*		from kernel memory.				*
*								*
*  Inputs:	None.						*
*								*
*  Outputs:							*
*  device	The "uba_device" device structure entry.	*
*								*
*  Return Values: 						*
*  1		More "uba_device" structures to read.		*
*  0		No more "uba_device" structures to read.	*
****************************************************************/
get_uba_device(device)
struct uba_device *device;
{

	/* Read "uba_device" structure entry from kernel memory */
	udevice_offset = lseek(kmem, udevice_offset + udevice_cnt, 0);
	udevice_cnt = read(kmem, device, sizeof(*device));

	/* Read the device name from the "uba_device" structure */
	lseek(kmem, device->ui_devname, 0);
	device->ui_devname = &ubdname[0];
	read(kmem, device->ui_devname, DEVNAMESIZE);

	/* Have we finished reading the "uba_device" list */
	return((device->ui_driver != NULL) ? 1 : 0);
}

/****************************************************************
*  Name:	get_nummbas					*
*    								*
*  Abstract:	Get the number of MASSBUS ADAPTERS in the 	*
*		system.						*
*								*
*  Inputs:	None.						*
*								*
*  Outputs:	None.						*
*								*
*  Return Values: 						*
*  count	Returns the number of MASSBUS ADAPTERS used.	*
****************************************************************/
get_nummbas()
{

	long offset;
	int number, i, count;
	struct mba_hd mbhead;

	/* Read the number of MASSBUS ADAPTERS configured */
	offset = reset_anythg(NL_nummbas);
	lseek(kmem, offset, 0);
	read(kmem, &number, sizeof(number));

	/* Now figure out how many MASSBUS ADAPTERS are used */
	offset = reset_anythg(NL_mbhead);
	for(i=0, count=0; i<number; i++) {
		/* Read a "mba_hd" structure from kernel memory */
		lseek(kmem, offset, 0);
		read(kmem, &mbhead, sizeof(mbhead));
		offset += sizeof(mbhead);

		/* Is this MASSBUS ADAPTER being used */
		if(mbhead.mh_ndrive && mbhead.mh_mba && mbhead.mh_physmba)
			count++;
	}
	return(count);
}

/****************************************************************
*  Name:	get_mba_slave					*
*    								*
*  Abstract:	Get "mba_slave" entry from "mbsinit" structure.	*
*    		This routine reads a MASSBUS slave device	*
*		structure from kernel memory.			*
*								*
*  Inputs:	None.						*
*								*
*  Outputs:							*
*  sdevice	The "mba_slave" slave device structure entry.	*
*								*
*  Return Values: 						*
*  1		More "mba_slave" structures to read.		*
*  0		No more "mba_slave" structures to read.		*
****************************************************************/
get_mba_slave(sdevice)
struct mba_slave *sdevice;
{

	/* Read "mba_slave" structure entry from kernel memory */
	mslave_offset = lseek(kmem, mslave_offset + mslave_cnt, 0);
	mslave_cnt = read(kmem, sdevice, sizeof(*sdevice));

	/* Get the "mba_driver" structure that this is pointed to */
	lseek(kmem, sdevice->ms_driver, 0);
	read(kmem, &mbdriver, sizeof(mbdriver));

	/* Read the device name from the "mba_driver" structure */
	lseek(kmem, mbdriver.md_dname, 0);
	mbdriver.md_dname = &mbdname[0];
	read(kmem, mbdriver.md_dname, DEVNAMESIZE);

	/* Read the slave name from the "mba_driver" structure */
	lseek(kmem, mbdriver.md_sname, 0);
	mbdriver.md_sname = &mbsname[0];
	read(kmem, mbdriver.md_sname, DEVNAMESIZE);

	/* Have we finished reading the "mba_slave" list */
	if(sdevice->ms_driver == NULL)
		return(0);
	else {
		sdevice->ms_driver = &mbdriver;
		return(1);
	}
}

/****************************************************************
*  Name:	get_mba_device					*
*    								*
*  Abstract:	Get "mba_device" entry from "mbdinit" structure.*
*    		This routine reads a MASSBUS device structure	*
*		from kernel memory.				*
*								*
*  Inputs:	None.						*
*								*
*  Outputs:							*
*  mdevice	The "mba_device" device structure entry.	*
*								*
*  Return Values: 						*
*  1		More "mba_device" structures to read.		*
*  0		No more "mba_device" structures to read.	*
****************************************************************/
get_mba_device(mdevice)
struct mba_device *mdevice;
{

	/* Read "mba_device" structure entry from kernel memory */
	mdevice_offset = lseek(kmem, mdevice_offset + mdevice_cnt, 0);
	mdevice_cnt = read(kmem, mdevice, sizeof(*mdevice));

	/* Get the "mba_driver" structure that this is pointed to */
	lseek(kmem, mdevice->mi_driver, 0);
	read(kmem, &mbdriver, sizeof(mbdriver));

	/* Read the device name from the "mba_driver" structure */
	lseek(kmem, mbdriver.md_dname, 0);
	mbdriver.md_dname = &mbdname[0];
	read(kmem, mbdriver.md_dname, DEVNAMESIZE);

	/* Read the slave name from the "mba_driver" structure */
	lseek(kmem, mbdriver.md_sname, 0);
	mbdriver.md_sname = &mbsname[0];
	read(kmem, mbdriver.md_sname, DEVNAMESIZE);

	/* Have we finished reading the "mba_device" list */
	if(mdevice->mi_driver == NULL)
		return(0);
	else {
		mdevice->mi_driver = &mbdriver;
		return(1);
	}
}

/****************************************************************
*  Name:	init_adl_entry					*
*    								*
*  Abstract:	Initialize an alive device table entry to its 	*
*		default settings.				*
*								*
*  Inputs:							*
*  i		The current index in the alive device table.	*
*								*
*  Outputs:	None.						*
*								*
*  Return Values: None.						*
****************************************************************/
init_adl_entry(i)
int i;
{
    	adltbl[i].bus_type = -1;
    	adltbl[i].device_index = -1;
    	adltbl[i].device_unit = -1;
    	adltbl[i].device_drive = -1;
    	adltbl[i].conn_name[0] = NULL;
    	adltbl[i].conn_number = -1;
    	adltbl[i].node_number = -1;
    	adltbl[i].node_index = -1;
    	adltbl[i].rctlr = -1;
    	adltbl[i].csr = -1;
    	adltbl[i].alive_unit = 1;
}
