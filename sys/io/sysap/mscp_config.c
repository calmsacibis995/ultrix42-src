#ifndef	lint
static char *sccsid = "@(#)mscp_config.c	4.3	(ULTRIX)	12/6/90";
#endif	lint
/************************************************************************
 *                                                                      *
 *                      Copyright (c) 1985 - 1989 by		        *
 *              Digital Equipment Corporation, Maynard, MA              *
 *                      All rights reserved.                            *
 *                                                                      *
 *   This software is furnished under a license and may be used and     *
 *   copied  only  in accordance with the terms of such license and     *
 *   with the  inclusion  of  the  above  copyright  notice.   This     *
 *   software  or  any  other copies thereof may not be provided or     *
 *   otherwise made available to any other person.  No title to and     *
 *   ownership of the software is hereby transferred.                   *
 *                                                                      *
 *   The information in this software is subject to change  without     *
 *   notice  and should not be construed as a commitment by Digital     *
 *   Equipment Corporation.                                             *
 *                                                                      *
 *   Digital assumes no responsibility for the use  or  reliability     *
 *   of its software on equipment which is not supplied by Digital.     *
 *                                                                      *
 ************************************************************************
 *
 *
 *   Facility:	Systems Communication Architecture
 *		Disk and Tape Class Driver common code.
 *
 *   Abstract:	This module contains functions which locate and fill in
 *		various static ULTRIX data structures with information
 *		obtained dynamically from the class drivers and underlying
 *		SCA layers.
 *
 *   mscp_find_controller	Find controller structure for connection
 *   mscp_find_device		Find device structure for unit
 *   mscp_find_model		Get controller model information
 *   mscp_strcmp		Utility string compare function
 *   mscp_adapt_check		Check whether this controller attaches to this
 *				adapter as specified by the configuration.
 *
 *   Author:	David E. Eiche	Creation Date:	September 30, 1985
 *
 *   History:
 *
 *   04-Dec-1990	Pete Keilty
 *	Added new routine mscp_adapt_check() for static load balancing.
 *	Checks whether this controller attaches to this adapter as 
 *	specified by the configuration.
 *
 *   05-Jul-1990	Pete Keilty
 *	Changed mscp_find_controller bus_type for CI is now DEV_BICI,
 *	DEV_XMICI and DEV_CI depending on interconnect.
 *
 *   06-Jun-1990	Pete Keilty
 *	Changed mscp_find_controller to find wildcarded at ci? for HSC's.
 *
 *   27-Dec-1989	David E. Eiche		DEE0081
 *	Change mscp_find_model() to eliminate the setting of host
 *	timeout.
 *
 *   09-Nov-1989	David E. Eiche		DEE0080
 *	Change mscp_find_model() to use the new software port and
 *	interconnect type fields in the local port information block
 *	to fill in the controller name and bus type fields of the
 *	connection block.
 *
 *   26-Sep-1989	Tim Burke
 *	Test to make sure that dkn < DK_NDRIVE before assigning the dk slot for
 *	disk statistics.
 *
 *   18-May-1989	Tim Burke
 *	Changed mscp_find_device to wildcard slave numbers with a -1 instead of
 *	using '?'.  This is necessary because the '?' character conflicts with
 *	unit number 63.
 *
 *   17-Jan-1989	Todd M. Katz		TMK0002
 *	1. The macro Scaaddr_lol() has been renamed to Scaaddr_low().  It now
 *	   accesses only the low order word( instead of low order longword ) of
 *	   a SCA system address.
 *	2. Include header file ../vaxmsi/msisysap.h.
 *	3. Use the ../machine link to refer to machine specific header files.
 *
 *   05-Aug-1988	David E. Eiche		DEE0049
 *	Rearrange the code added in DEE0048 to provide defaults	for
 *	bus type and controller name, improve code style, and add a 
 *	case for the BVPSSP hardware port type.
 *
 *   27-Jul-1988	David E. Eiche		DEE0048
 *	Change mscp_find_model to use information acquired from the
 *	port driver to determine the bus type and config name of the
 *	controller when that information cannot be derived directly
 *	from the model code.
 *
 *   17-Jul-1988	David E. Eiche		DEE0047
 *	Change mscp_find_controller to store the ubminit structure
 *	pointer in the connection block, rather than return it as
 *	the function value.
 *
 *   17-Jul-1988	David E. Eiche		DEE0046
 *	Move mscp_find_model routine to this module and change it
 *	to correspond to the new model table format and to store
 *	various fields in the connection block.
 *
 *   02-Jun-1988     Ricky S. Palmer
 *      Removed inclusion of header file ../vaxmsi/msisysap.h
 *	Added new code to correctly configure "dssc".
 *
 *   29-Apr-1988	David E. Eiche		DEE0033
 *	Add code to fill hardcoded "ra" and "hsc" strings into the
 *	hscdriver driver table entry.  Also correct header comment
 *	for 25-Apr-1988.
 *
 *   25-Apr-1988	Robin
 *	Added code to enable disk I/O statistics gathering for MSCP
 *	disks.
 *
 *   07-Feb-1988	David E. Eiche		DEE0026
 *	Change mscp_find_controller to print the controller name and number
 *	from the ubminit entry, add the remote controller number to the
 *	printout, and provide an additional message to clarify status when a
 *	HSC controller cannot be found in the configuration file.
 *
 *   07-Feb-1988	David E. Eiche		DEE0025
 *	Change mscp_find_controller to propogate adaptor number and
 *	nexus number from the adaptor entry to the controller entry
 *	in the config_adpt table.  This allows the change made in
 *	DEE0012 to work.  Also change mscp_find_device to propogate
 *	the remote controller number from ubminit to ubdinit entries.
 *
 *   02-Feb-1988	David E. Eiche		DEE0012
 *	Change mscp_find_controller to get adaptor number and nexus
 *	out of new fields in the adaptor configuration table rather
 *	than the CI adaptor table.
 *
 *   15-Jan-1988	Todd M. Katz		TMK0001
 *	Include new header file ../io/msi/msisysap.h.
 */

/**/

/* Libraries and Include Files.
 */
#include	"../h/types.h"
#include	"../h/time.h"
#include	"../h/param.h"
#include	"../h/kmalloc.h"
#include	"../h/buf.h"
#include	"../h/errno.h"
#include	"../h/ioctl.h"
#include	"../h/devio.h"
#include	"../h/dk.h"
#include	"../h/file.h"
#include	"../fs/ufs/fs.h"
#include	"../h/errlog.h"
#include	"../machine/pte.h"
#include	"../h/vmmac.h"
#include	"../h/config.h"
#include	"../io/scs/sca.h"
#include	"../io/ci/cippdsysap.h"
#include	"../io/ci/cisysap.h"
#include	"../io/ci/ciadapter.h"
#include	"../io/bi/bvpsysap.h"
#include	"../io/gvp/gvpsysap.h"
#include	"../io/msi/msisysap.h"
#include	"../io/uba/uqsysap.h"
#include	"../io/sysap/sysap.h"
#include	"../io/uba/ubavar.h"
#include	"../io/sysap/mscp_msg.h"
#include	"../io/sysap/mscp_defs.h"

/**/

/* External Variables and Routines.
 */
extern	struct uba_ctlr		ubminit[];
extern	struct uba_device	ubdinit[];
extern	struct uba_driver	mscpdriver;
extern	struct config_adpt	config_adpt[];
extern	MODEL			model_tbl[];
extern	int			model_ct;
extern  int                     dkn;

void				mscp_find_controller();
void				mscp_find_device();
void				mscp_find_model();



/**/

/*
 *
 *   Name:	mscp_find_controller - Find controller in Unibus database.
 *
 *   Abstract:	This function attempts to find a "Unibus" controller table
 *		entry corresponding to the supplied connection.  No wildcard
 *		card matching is provided for any field.
 *
 *   Inputs:	IPL_SCS
 *		cp			Connection block pointer
 *		    cnt_name		config name for controller
 *		    cnt_number		remote controller number if HSC;
 *					otherwise logical controller index.
 *   Implicit
 *   Inputs:	ubminit			Unibus controller table
 *		config_adpt		config'ed adapter table
 *
 *   Outputs:
 *
 *
 *   Return	
 *   Values:	NONE
 */

void
mscp_find_controller( cp )
    register CONNB		*cp;
{
    register struct uba_ctlr	*um;
    register struct config_adpt *ap, *ap1;
    register u_long		found = 0;
    register u_long		adapt_num = Ctrl_from_name( cp->lport_name );

    /* If the connection is to an HSC, search for a ubminit entry that
     * matches on controller name and remote controller number.  If
     * a match is found: 
     *
     *	1. Find the corresponding controller entry in the config_adpt table.
     *	2. Find the parent adaptor entry in the config_adpt table.
     *	3. Propogate the bus number and nexus number from the adaptor entry
     *	   to the controller entry in the config_adpt table, and then into
     *	   the ubminit entry.
     *	4. Link the config_adpt entry to the ubminit entry.
     *	5. Hard wire "ra" and "hsc" into the corresponding driver
     *     table entry.
     *	6. If the unit had not been previously marked alive, mark it so
     *     and print a message identifying the controller.
     *	7. Return a pointer to the ubminit entry to the caller.
     *
     */
    if( mscp_strcmp( "hsc", cp->cnt_name ) == 0 ||
	mscp_strcmp( "dssc", cp->cnt_name ) == 0) {
	for( um = ubminit; um->um_driver; um++ )
	    if(( mscp_strcmp( um->um_ctlrname, cp->cnt_name ) == 0 )
		&& um->um_rctlr == cp->cnt_number )
		break;

	if( um->um_driver ) {
	    for( ap = config_adpt; ap->p_name; ap++ )
		if(( ap->c_name == ( char * )um->um_driver ) && 
		    ( ap->c_num == um->um_ctlr ) &&
		    ( ap->c_type == 'C' ))
		    break;

	    if( ap->p_name ) {
		for( ap1 = config_adpt; ap1->p_name; ap1++ )
		    if(( mscp_strcmp( ap1->c_name, ap->p_name ) == 0 ) && 
			( ap1->c_num == adapt_num ) &&
			( ap1->c_type == 'A' ) &&
			( ap1->c_ptr ))
			break;

		if( ap1->p_name ) {
		    found++;
		    ap->c_bus_num = ap1->c_bus_num;
		    ap->c_nexus_num = ap1->c_nexus_num;
		    um->um_adpt = ap1->c_bus_num;
		    um->um_nexus = ap1->c_nexus_num;
		    um->um_ubanum = 0;

		    ap->c_ptr = ( caddr_t )um;
		    ap->p_num = adapt_num;

		    um->um_driver->ud_dname = "ra";
		    if( mscp_strcmp( "dssc", cp->cnt_name ) == 0) {
			um->um_driver->ud_mname = "dssc";
		    } else {
		    	um->um_driver->ud_mname = "hsc";
		    }

		    if( um->um_alive == 0 ) {
			um->um_alive = 1;
			printf("%s%d at %s%d node %d\n",
			   um->um_ctlrname,
			   um->um_ctlr,
			   ap->p_name,
			   ap->p_num,
			   um->um_rctlr );
		    }
		}
	    }
	}

    /* If the connection is to another controller type, search for a ubminit
     * entry that matches on controller name and controller number.  If a
     * match is found, return a pointer to the ubminit entry to the caller.
     */
    } else {
	for( um = ubminit; um->um_driver; um++ )
	    if(( mscp_strcmp( um->um_ctlrname, cp->cnt_name ) == 0 )
		&& um->um_ctlr == cp->cnt_number )
		break;
	if( um->um_driver )
	    found++;
    }

    /* If the controller cannot be found in the ubminit table, print
     * a diagnostic and set the um pointer to NULL.
     */
    if( found == 0 ) {
	if( mscp_strcmp( "hsc", cp->cnt_name ) == 0 ||
	    mscp_strcmp( "dssc", cp->cnt_name ) == 0 )
	    printf( "mscp_find_controller: %s at node %d not config'ed\n",
		     cp->cnt_name,
		     cp->cnt_number );
	else
	    printf( "mscp_find_controller: %s%d not config'ed\n",
		     cp->cnt_name,
		     cp->cnt_number );
	um = NULL;
    }

    cp->ubctlr = um;
    return;
}

/**/

#define PTS_SAME_CTLR		20
#define PTS_SAME_SLAVE		18
#define PTS_WILD_CTLR		17
#define PTS_WILD_SLAVE		16
#define PTS_SAME_DRIVER		3
#define PTS_EXACT_MATCH		PTS_SAME_DRIVER+PTS_SAME_CTLR+PTS_SAME_SLAVE

/*
 *
 *   Name:	mscp_finddevice - Find unit in Unibus database.
 *
 *   Abstract:	
 *
 *   Inputs:
 *
 *   Outputs:
 *
 *
 *   Return	NONE
 *   Values:
 */

void
mscp_find_device( up )
    register UNITB		*up;

{
    register struct uba_ctlr	*um = up->connb->ubctlr;
    register char		*devname = up->connb->classb->dev_name;
    register struct uba_device	*ui;
    u_long			bestpoints = 0;
    struct uba_device		*bestui= NULL;
    char			name_string[10];

    /* Scan the "Unibus" device data base, looking for the best
     * match on device name, slave unit number, controller
     * number and controller name.
     */
    for( ui = ubdinit; ui->ui_driver; ui++ ) {
	register u_long		points = 0;
	if ( !ui->ui_alive  && !mscp_strcmp( ui->ui_devname, devname )) {
	    if( ui->ui_driver == um->um_driver ) {
		points += PTS_SAME_DRIVER;
		if( ui->ui_ctlr == um->um_ctlr ) points += PTS_SAME_CTLR;
		else if( ui->ui_ctlr == '?' ) points += PTS_WILD_CTLR;
		if( ui->ui_slave == up->unit ) points += PTS_SAME_SLAVE;
		else if( ui->ui_slave == -1 ) points += PTS_WILD_SLAVE;
	    } else if( ui->ui_driver == ( struct uba_driver * )&mscpdriver ) {
		points += PTS_WILD_CTLR;
		if( ui->ui_slave == up->unit ) points += PTS_SAME_SLAVE;
		else if( ui->ui_slave == -1 ) points += PTS_WILD_SLAVE;
	    }
	    if( points > bestpoints ) {
		bestpoints = points;
		bestui = ui;
	    }
	    if( points == PTS_EXACT_MATCH )
		break;
	}
    }
    if( bestpoints >= ( PTS_WILD_CTLR + PTS_WILD_SLAVE )) {
	register UNITB		**unit_tbl = up->connb->classb->unit_tbl;
	ui = bestui;
	if( unit_tbl[ ui->ui_unit ] == NULL ) {
	    unit_tbl[ ui->ui_unit ] = up;
	    ui->ui_alive = 1;
	    ui->ui_driver = um->um_driver;
	    ui->ui_ctlr = um->um_ctlr;
	    ui->ui_slave = up->unit;
	    ui->ui_ubanum = um->um_ubanum;
	    ui->ui_adpt = um->um_adpt;
	    ui->ui_nexus = um->um_nexus;
	    ui->ui_rctlr = um->um_rctlr;
	    ui->ui_hd = um->um_hd;
	    ui->ui_addr = um->um_addr;
	    if (ui->ui_addr != 0) {
		ui->ui_physaddr = ( caddr_t )svtophy( ui->ui_addr );
	    }
	    ui->ui_mi = um;
	    if((up->connb->classb->flags.disk) && (dkn < DK_NDRIVE)) {
		ui->ui_dk = dkn++;
	    }
	    else {
		ui->ui_dk = -1;
	    }
	    up->ubdev = ui;
	    mscp_media_to_ascii( up->media_id, name_string );
	    printf( "%s%d at %s%d slave %d (%s)\n",
		ui->ui_devname, ui->ui_unit, um->um_ctlrname,
		um->um_ctlr, ui->ui_slave, name_string );
	}
    }
    return;
}
/**/

/*
 *
 *   Name:	mscp_find_model - Find controller model information
 *
 *   Abstract:	This routine fills in the connection block with
 *		model dependent information.
 *
 *   Inputs:	cp			Connection block pointer
 *		    cnt_id.model	Controller model number
 *		    rport_addr		Remote port address
 *   		model_tbl		Controller model table
 *		    bus_type		I/O bus type
 *		    config_name		Config's controller name 
 *		    host_timeout	Host timeout period (s.)
 *		    model		Controller model number
 *		    name		Controller model name
 *		model_ct		Controller model table entry count
 *
 *   Outputs:	cp			Connection block pointer
 *		    bus_type		I/O bus type
 *		    cnt_name		Config's controller name
 *		    cnt_number		Config's controller number
 *		    model_name		Controller model name
 *
 *   Return	NONE
 *   Values:
 */

void
mscp_find_model( cp )
    register CONNB	*cp;
{
    register MODEL	*modp;
    ISB			isb;
    LPIB		lpib;

    /* Search the model table for an entry that matches the controller
     * model number in the connection block.  If there is no matching
     * entry, use the permanently reserved entry at the front of the model
     * table.
     * Fill in the connection block from the model table and extract
     * the controller number from the remote port address.  If the bus
     * type is unknown, get local port information from SCS and use
     * the port type to determine the bus type and the controller name
     * used by config.
     */
    for( modp = model_tbl;
	 modp < model_tbl + model_ct &&
	 cp->cnt_id.model > modp->model;
	 modp++ ) {}
    if( cp->cnt_id.model != modp->model ) {
	modp = model_tbl;
    }

    cp->model_name = ( u_char * )modp->name;
    cp->cnt_number =  Scaaddr_low( cp->rport_addr );
    cp->cnt_name = "ERROR";
    cp->bus_type = DEV_UNKBUS;

    isb.next_lport_name = cp->lport_name;
    if( scs_info_lport( &isb, &lpib ) == RET_SUCCESS ) {
	switch( lpib.type.swtype ) {
	case	SPT_UQSSP:
	    cp->cnt_name = "uq";
	    switch( lpib.type.ictype ) {
	    case	ICT_BI:
		cp->bus_type = DEV_BI;
		break;
	    case	ICT_XMI:
		cp->bus_type = DEV_XMI;
		break;
	    case	ICT_UB:
		cp->bus_type = DEV_UB;
		break;
	    case	ICT_QB:
		cp->bus_type = DEV_QB;
		break;
	    default:
		break;
	    }
	    break;
	case	SPT_CI:
	    cp->cnt_name = "hsc";
	    switch( lpib.type.ictype ) {
	    case	ICT_BI:
		cp->bus_type = DEV_BICI;
		break;
	    case	ICT_XMI:
		cp->bus_type = DEV_XMICI;
		break;
	    default:
	        cp->bus_type = DEV_CI;
	        break;
	    }
	    break;
	case SPT_BVPSSP:
	    cp->cnt_name = "bvpssp";
	    cp->bus_type = DEV_BI;
	    break;
	case	SPT_MSI:
	    cp->cnt_name = "dssc";
	    cp->bus_type = DEV_MSI;
	    break;
	default:
	    break;
	}
    }
}
/**/

/*
 *
 *   Name:	mscp_strcmp - Compare character strings for equality.
 *
 *   Abstract:	
 *
 *   Inputs:
 *
 *   Outputs:
 *
 *
 *   Return	NONE
 *   Values:
 */

int
mscp_strcmp( s, t )
    register u_char	*s;
    register u_char	*t;
{

    while( *s == *t++ ) {

	/* If end of string with all corresponding characters equal,
	 * return equality.
	 */
	if( *s++ == '\0' )
	    return( 0 );
    }

    /* Exit with inequality.
     */
    return( 1 );

}
/**/

/*
 *
 *   Name:	mscp_adapt_check - Check whether this controller attaches 
 *				   to this adapter as specified by the 
 *				   configuration.
 *
 *   Abstract:	Checks whether this controller attaches to this adapter
 *		as specified by the configuration.
 *
 *
 *   Inputs:  cmsb		Connection management service block
 *		rport_addr	Remote port address
 *		lport_name	Local port name and number
 *
 *   Outputs:
 *
 *
 *   Return
 *   Values: RET_SUCCESS = 1
 *	     FAILURE     = 0
 */
int
mscp_adapt_check( cmsb )
    register CMSB 	*cmsb;
{
    register struct uba_ctlr	*um;
    register struct config_adpt *ap;
    register char		*cp;
    register u_long		cnt_num = Scaaddr_low( cmsb->rport_addr );
    register u_long		adapt_num = Ctrl_from_name( cmsb->lport_name );
    register u_long		found = 0;

    cp = (char *)&cmsb->lport_name;
    if(( *cp == 'c' ) && ( *(cp+1) == 'i' )) {
	for( um = ubminit; um->um_driver; um++ )
	    if(( mscp_strcmp( um->um_ctlrname, "hsc" ) == 0 )
		&& um->um_rctlr == cnt_num )
		break;

	if( um->um_driver ) {
	    for( ap = config_adpt; ap->p_name; ap++ )
		if(( ap->c_name == ( char * )um->um_driver ) && 
		    ( ap->c_num == um->um_ctlr ) &&
		    ( ap->c_type == 'C' ) &&
		    ( mscp_strcmp( ap->p_name, "ci" ) == 0 ) &&
		    ( ap->p_num == adapt_num || ap->p_num == '?' )) {
   		    found = RET_SUCCESS;
		    break;
		}
	}
    } else {
   	found = RET_SUCCESS;
    }
    return( found );
}
