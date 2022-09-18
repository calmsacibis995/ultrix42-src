#ifndef	lint 
static char *sccsid = "@(#)mscp.c	4.1	(ULTRIX)	7/17/90";
#endif	lint

/************************************************************************
 *                                                                      *
 *                      Copyright (c) 1989 by                           *
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
 *   Facility:	MSCP data structure formatter for the crash utility.
 *
 *   Abstract:	This module contains the routines used to display the
 *		contents of MSCP data structures.  Routines are also
 *		provided to traverse data structure lists.
 *
 *   Author:	Tim Burke	September 13, 1989
 *
 *   Modification History:
 *
 *	23-Oct-1989	Tim Burke
 *	Added the print_uniq_id routine to display the contents of a 
 *	UNIQ_ID data structure.
 *
 *	22-Nov-1989	Tim Burke
 *	Added the print_lbhandle routine so that the local buffer handle
 *	field is properly translated as well as the following field which
 *	is the request block state.
 */

#include 	<sys/types.h>
#include 	<sys/buf.h>
#include 	<sys/devio.h>
#include 	<sys/mtio.h>
#include 	<sys/param.h>
#include	<fs/ufs/fs.h>
#include	<io/scs/sca.h>
#include	<io/ci/cippdsysap.h>
#include	<io/ci/cisysap.h>
#include	<io/msi/msisysap.h>
#include	<io/uba/uqsysap.h>
#include	<io/bi/bvpsysap.h>
#include	<io/gvp/gvpsysap.h>
#include	<io/sysap/sysap.h>
#include	<io/sysap/mscp_msg.h>
#include	<io/sysap/mscp_defs.h>
#include	"crash.h"
#include	<stdio.h>

/*
 * Defines related to displaying MSCP data structures.
 */

/*
 * MSCP sysap types
 */
#define MSCP_TAPE	0x01		/* TMSCP - tape class subysstem */
#define MSCP_DISK	0x02		/* MSCP  - disk class subysstem */

/*
 * MSCP printing levels - controls ammount of output.
 */
#define MSCP_PRINTFULL	0x01		/* Verbose printing		*/
#define MSCP_PRINTBRIEF	0x02		/* Brief printing		*/

#define DEVNAME_SIZE	80	/* String length for device name */
char *token();

c_mscp(c)
	char *c;
{
	char *arg;
	int index, majno, minno;
	unsigned int addr;

	arg = token();
	if (arg == NULL) {
		(void)printmscp(MSCP_TAPE | MSCP_DISK, MSCP_PRINTFULL);
		return;
	}
	else if (strcmp(arg,"-disk") == 0) {
		(void)printmscp(MSCP_DISK, MSCP_PRINTFULL);
	}
	else if (strcmp(arg,"-tape") == 0) {
		(void)printmscp(MSCP_TAPE, MSCP_PRINTFULL);
	}
	else if (strcmp(arg,"-config") == 0) {
		(void)printmscp(MSCP_TAPE|MSCP_DISK, MSCP_PRINTBRIEF);
	}
	else if (strcmp(arg,"-dtable") == 0) {
		(void)print_unit_table(MSCP_DISK, MSCP_PRINTFULL);
	}
	else if (strcmp(arg,"-ttable") == 0) {
		(void)print_unit_table(MSCP_TAPE, MSCP_PRINTFULL);
	}
	else if (strcmp(arg,"-connb") == 0) {
		arg = token();
		if (arg == NULL) {
			printf("usage: -connb requires address\n");
			return;
		}
		addr = scan_vaddr(arg);
		(void)printmscp_connb(addr, MSCP_PRINTFULL);
	}
	else if (strcmp(arg,"-reqb") == 0) {
		arg = token();
		if (arg == NULL) {
			printf("usage: -reqb requires address\n");
			return;
		}
		addr = scan_vaddr(arg);
		(void)printmscp_reqb(addr, MSCP_PRINTFULL);
	}
	else if (strcmp(arg,"-classb") == 0) {
		arg = token();
		if (arg == NULL) {
			printf("usage: -classb requires address\n");
			return;
		}
		addr = scan_vaddr(arg);
		(void)printmscp_classb(addr, MSCP_PRINTFULL);
	}
	else if (strcmp(arg,"-unitb") == 0) {
		arg = token();
		if (arg == NULL) {
			printf("usage: -unitb requires address\n");
			return;
		}
		addr = scan_vaddr(arg);
		(void)printmscp_unitb(addr, MSCP_PRINTFULL);
	}
	else if (strcmp(arg,"-devunit") == 0) {
		arg = token();
		if (arg == NULL) {
			printf("usage: -devunit requires major/minor numbers\n");
			return;
		}
		majno = atoi(arg);
		arg = token();
		if (arg == NULL) {
			printf("usage: -devunit requires major/minor numbers\n");
			return;
		}
		minno = atoi(arg);
		(void)printmscp_devunit(majno, minno, MSCP_PRINTFULL);
	}
	else {
		printf("Invalid mscp parameter: %s\n",arg);
	}
	while(token()!=NULL);
}

/*
 *
 *   Name:	printmscp	- Traverse and display data structs
 *
 *   Abstract:	This routine will start at the connection block and
 *	        traverse the data structures printing results.
 *
 *   Inputs:    subsys_name 	- The name of the particular subsystem.
 *		printmode	- Controlls output level.
 *
 *   Return	NONE
 *   Values:
 */

int 
printmscp(subsys_name, printmode)
    int		subsys_name;
    int		printmode;

{
	CLASSB *mscp_classb;
	CLASSB *tmscp_classb;

	mscp_classb = (CLASSB *)Mscp_classb.s_value;
	tmscp_classb = (CLASSB *)Tmscp_classb.s_value;

	if (subsys_name & MSCP_DISK) {
		printf("\n");
		printf("MSCP SUBSYSTEM:\n");
		printmscp_classb(mscp_classb, printmode);
		mscp_follow_connections(mscp_classb, printmode);
	}
	if (subsys_name & MSCP_TAPE) {
		printf("\n");
		printf("TMSCP SUBSYSTEM:\n");
		printmscp_classb(tmscp_classb, printmode);
		mscp_follow_connections(tmscp_classb, printmode);
	}
}

/*
 *
 *   Name:	mscp_follow_connections	- Follow a list of connection blocks
 *			starting with a class block pointer.
 *
 *   Abstract:	This routine is used to traverse the mscp connections.
 *		call the appropriate routine to print out the contents of
 *		the connection blocks.
 *
 *   Inputs:    classp	 	- Pointer to a class block
 *		printmode	- Controlls output level.
 *
 *   Return	NONE
 *   Values:
 */
int 
mscp_follow_connections(classp, printmode)
    CLASSB	*classp;
    int		printmode;

{
	CLASSB class_st;
	CONNB  conn_st, *conn_pt;

	if (classp == 0) {
		printf("\tNULL class block pointer\n");
		return;
	}
	if(readmem((char *) &class_st, (int)classp, sizeof(CLASSB)) !=
	    sizeof(CLASSB)) {
		printf("could not read class block at 0x%x.\n",classp);
		return;
	}

	if ((class_st.flink == class_st.blink) &&
	    (class_st.flink == (CONNB *)classp)) {
		printf("\n\tThis class block does not have any connections.\n");
		return;
	}
	
	conn_pt = class_st.flink;
	do {
	    printmscp_connb(conn_pt, printmode);
	    mscp_follow_units(conn_pt, printmode);
	    if(readmem((char *) &conn_st, (int)conn_pt, sizeof(CONNB)) !=
	        sizeof(CONNB)) {
		    printf("could not read connection block at 0x%x\n",conn_pt);
		    return;
	    }
	    conn_pt = conn_st.flink;
	} while  (conn_pt != (CONNB *)classp);
}

/*
 *
 *   Name:	mscp_follow_units	- Follow a list of unit blocks
 *			starting with a connection block pointer.
 *
 *   Abstract:	This routine is used to traverse the mscp units.
 *		Call the appropriate routine to print out the contents of
 *		the unit blocks.
 *
 *   Inputs:    connp	 	- Pointer to a connection block
 *		printmode	- Controlls output level.
 *
 *   Return	NONE
 *   Values:
 */
int 
mscp_follow_units(connp, printmode)
    CONNB	*connp;
    int		printmode;

{
	CONNB  conn_st;
	UNITB  unit_st, *unit_pt;
	char   *no_units;
	int    offset;

	if (connp == 0) {
		printf("\tNULL connection block pointer\n");
		return;
	}
	if(readmem((char *) &conn_st, (int)connp, sizeof(CONNB)) !=
	    sizeof(CONNB)) {
		printf("could not read connection block at 0x%x.\n",connp);
		return;
	}

	/*
	 * If there are no units the unit.flink and unit.blink will both point
	 * to the address of the unit.flink which is the 4th element in the
 	 * connection block data struct.  Set no_units to this address.
	 */
	offset = (int)(&conn_st.unit.flink) - (int)(&conn_st.flink);
	no_units = (char *) ((char *)connp + offset);

	if ((conn_st.unit.flink == conn_st.unit.blink) && 
	    (conn_st.unit.flink == (UNITB *)no_units)) {
		printf("\n\tThis connection block does not have any units.\n");
		return;
	}
	
	unit_pt = conn_st.unit.flink;
	do {
	    printmscp_unitb(unit_pt, printmode);
	    mscp_follow_reqb(unit_pt, printmode);
	    if(readmem((char *) &unit_st, (int)unit_pt, sizeof(UNITB)) !=
	        sizeof(UNITB)) {
		    printf("could not read unit block at 0x%x\n",unit_pt);
		    return;
	    }
	    unit_pt = unit_st.flink;
	} while  (unit_pt != (UNITB *)no_units);
}

/*
 *
 *   Name:	mscp_follow_reqb	- Follow a list of request blocks
 *			starting with a unit block pointer.
 *
 *   Abstract:	This routine is used to traverse the mscp request queues.
 *		Call the appropriate routine to print out the contents of
 *		the request blocks.
 *
 *   Inputs:    unitp	 	- Pointer to a unit block
 *		printmode	- Controlls output level.
 *
 *   Return	NONE
 *   Values:
 */
int 
mscp_follow_reqb(unitp, printmode)
    UNITB	*unitp;
    int		printmode;

{
	UNITB  unit_st;
	REQB   reqb_st, *reqb_pt;
	char   *no_req;
	int    offset;

	if (unitp == 0) {
		printf("\tNULL unit block pointer\n");
		return;
	}
	if(readmem((char *) &unit_st, (int)unitp, sizeof(UNITB)) !=
	    sizeof(UNITB)) {
		printf("could not read unit block at 0x%x.\n",unitp);
		return;
	}

	/*
	 * If there are no requests the request flink and request blink 
	 * will both point
	 * to the address of the request queue which is the 5th element in the
 	 * unit block data struct.  Set no_req to this address.
	 */
	offset = (int)(&unit_st.request.flink) - (int)(&unit_st.flink);
	no_req = (char *) ((char *)unitp + offset);

	if ((unit_st.request.flink == unit_st.request.blink) && 
	    (unit_st.request.flink == (REQB *)no_req)) {
		if (printmode & MSCP_PRINTFULL)
		printf("\n\tThere are no requests stalled in resource wait.\n");
		return;
	}
	
	reqb_pt = unit_st.request.flink;
	do {
	    printmscp_reqb(reqb_pt, printmode);
	    if(readmem((char *) &reqb_st, (int)reqb_pt, sizeof(REQB)) !=
	        sizeof(REQB)) {
		    printf("could not read request block at 0x%x\n",reqb_pt);
		    return;
	    }
	    reqb_pt = reqb_st.flink;
	} while  (reqb_pt != (REQB *)no_req);
}

/*
 *
 *   Name:	printmscp_connb	- Print contents of a connection block
 *
 *   Abstract:	Disect and display a connection block.
 *
 *   Inputs:    connp	 	- Pointer to a connection block
 *		printmode	- Controlls output level.
 *
 *   Return	NONE
 *   Values:
 */
int 
printmscp_connb(connp, printmode)
    CONNB	*connp;
    int		printmode;

{
	CONNB conn_st;
	char device_name[DEVNAME_SIZE];
	int i;
	char *reqb_addr;
	int offset;

	printf("\n\t-----------------  CONNECTION BLOCK  ------------------\n");
	if (connp == 0) {
		printf("\tNULL connection block pointer\n");
		return;
	}
	if(readmem((char *) &conn_st, (int)connp, sizeof(CONNB)) !=
	    sizeof(CONNB)) {
		printf("could not read connection block at 0x%x.\n",connp);
		return;
	}

    if (printmode & MSCP_PRINTFULL) {
	printf("\tConnection block address is 0x%x\n",connp);
	printf("\tConnection list: \(flink\) 0x%x, \(blink\) 0x%x\n",
		conn_st.flink, conn_st.blink);
	printf("\tClass block: \(0x%x\) ",conn_st.classb);
	praddr(conn_st.classb);
	printf("\n");
	printf("\tUnit list: \(unit.flink\) 0x%x, \(unit.blink\) 0x%x\n",
		conn_st.unit.flink, conn_st.unit.blink);
	printf("\tBBR block address: 0x%x, state: 0x%x\n",conn_st.bbrb,
		conn_st.state);
	printf("\tflags: ");
		if (conn_st.flags.restart)
			printf("restart ");
		if (conn_st.flags.sngl_strm)
			printf("sngl_strm ");
		if (conn_st.flags.path_fail)
			printf("path_fail ");
		if (conn_st.flags.need_upoll)
			printf("need_upoll ");
		if (conn_st.flags.upoll_busy)
			printf("upoll_busy ");
		if (conn_st.flags.need_cr)
			printf("need_cr ");
	printf("\n");
	printf("\tRequest Queues\tflink\t\tblink\t\tstatus\n");
	printf("\t    active\t0x%x\t0x%x",conn_st.active.flink,
			conn_st.active.blink);
	offset = (int)(&conn_st.active.flink) - (int)(&conn_st.flink);
	reqb_addr = (char *) ((char *)connp + offset);
	if ((conn_st.active.flink == conn_st.active.blink) &&
	    ((char *)conn_st.active.flink == reqb_addr))
		printf("\tempty\n");
	else
		printf("\tnon-empty\n");
	printf("\t    restart\t0x%x\t0x%x",conn_st.restart.flink,
			conn_st.restart.blink);
	offset = (int)(&conn_st.restart.flink) - (int)(&conn_st.flink);
	reqb_addr = (char *) ((char *)connp + offset);
	if ((conn_st.restart.flink == conn_st.restart.blink) &&
	    ((char *)conn_st.restart.flink == reqb_addr))
		printf("\tempty\n");
	else
		printf("\tnon-empty\n");
	printf("\t    credit_wq\t0x%x\t0x%x",conn_st.credit_wq.flink,
			conn_st.credit_wq.blink);
	offset = (int)(&conn_st.credit_wq.flink) - (int)(&conn_st.flink);
	reqb_addr = (char *) ((char *)connp + offset);
	if ((conn_st.credit_wq.flink == conn_st.credit_wq.blink) &&
	    ((char *)conn_st.credit_wq.flink == reqb_addr))
		printf("\tempty\n");
	else
		printf("\tnon-empty\n");
	printf("\t    buffer_wq\t0x%x\t0x%x",conn_st.buffer_wq.flink,
			conn_st.buffer_wq.blink);
	offset = (int)(&conn_st.buffer_wq.flink) - (int)(&conn_st.flink);
	reqb_addr = (char *) ((char *)connp + offset);
	if ((conn_st.buffer_wq.flink == conn_st.buffer_wq.blink) &&
	    ((char *)conn_st.buffer_wq.flink == reqb_addr))
		printf("\tempty\n");
	else
		printf("\tnon-empty\n");
	printf("\t    map_wq\t0x%x\t0x%x",conn_st.map_wq.flink,
			conn_st.map_wq.blink);
	offset = (int)(&conn_st.map_wq.flink) - (int)(&conn_st.flink);
	reqb_addr = (char *) ((char *)connp + offset);
	if ((conn_st.map_wq.flink == conn_st.map_wq.blink) &&
	    ((char *)conn_st.map_wq.flink == reqb_addr))
		printf("\tempty\n");
	else
		printf("\tnon-empty\n");
	printf("\tCommand timeout interval: %d, Resource wait timeout: %d\n",
		conn_st.cmdtmo_intvl, conn_st.rsrctmo_intvl);
	printf("\tRetry count: %d, Current unit number: %d\n",
		conn_st.retry_count, conn_st.cur_unit);
	printf("\tRestart count: %d, Host timeout period: %d\n",
		conn_st.restart_count, conn_st.hst_tmo);
	printf("\tSystem ID: 0x%x, Remote port address: 0x%x\n",
		conn_st.sysid, conn_st.rport_addr);
	printf("\tLocal port name: \(0x%x\) ",conn_st.lport_name);
	/*
	 * The loacal port name is a long made up of 4 characters which 
	 * contains the port name in ASCII.
	 */
	for (i = 0; i < 4 ; i++) {
		device_name[0] = (unsigned char) ((conn_st.lport_name >> 8*i) 
			& 0xff);
		if (isalnum(device_name[0]))
			printf("%c",device_name[0]);
	}
	printf("\n");
	printf("\tMSCP version: %d, Controller flags 0x%x\n",
		conn_st.version, conn_st.cnt_flgs);
	printf("\tController timeout: %d, Controller software version: %d\n",
		conn_st.cnt_tmo, conn_st.cnt_svr);
	printf("\tController hardware version: %d\n",conn_st.cnt_hvr);
	print_uniq_id(&conn_st.cnt_id);
	printf("\tController maximum byte count: 0x%x\n",conn_st.max_bcnt);
	printf("\tOldest rspid: 0x%x, oldest rspid status: 0x%x\n",
		conn_st.old_rspid, conn_st.old_cmd_sts);
	printf("\tRestart reqb: 0x%x\n",conn_st.restart_reqb);
 	readmem((char *)device_name, (unsigned int)conn_st.model_name, DEVNAME_SIZE);
	printf("\tController model name: %s\n",device_name);
 	readmem((char *)device_name, (int)conn_st.cnt_name, DEVNAME_SIZE);
	printf("\tConfig controller name: %s, config controller number: %d\n",
		device_name, conn_st.cnt_number);
	printf("\tBus type: %d, ubminit entry: 0x%x\n",
		conn_st.bus_type, conn_st.ubctlr);
	printf("\tConnection ID: 0x%x\n", conn_st.connid);
	printf("\tTimeout request block: 0x%x, Polling request block: 0x%x\n",
		conn_st.timeout_reqb, conn_st.polling_reqb);
	printf("\tContents of the command timeout request block:\n");
	offset = (int)(&conn_st.timeout_reqb) - (int)(&conn_st.flink);
	reqb_addr = (char *) ((char *)connp + offset);
	printmscp_reqb((REQB *) reqb_addr, printmode);
	printf("\tContents of the polling / DAP request block:\n");
	offset = (int)(&conn_st.polling_reqb) - (int)(&conn_st.flink);
	reqb_addr = (char *) ((char *)connp + offset);
	printmscp_reqb((REQB *) reqb_addr, printmode);
    }
    else if (printmode & MSCP_PRINTBRIEF) {
	printf("\tConnection block address is 0x%x\n",connp);
	printf("\tLocal port name:  ");
	/*
	 * The loacal port name is a long made up of 4 characters which 
	 * contains the port name in ASCII.
	 */
	for (i = 0; i < 4 ; i++) {
		device_name[0] = (unsigned char) ((conn_st.lport_name >> 8*i) 
			& 0xff);
		if (isalnum(device_name[0]))
			printf("%c",device_name[0]);
	}
	printf(", ");
 	readmem((char *)device_name, (unsigned int)conn_st.model_name, DEVNAME_SIZE);
	printf("\tController model name: %s\n",device_name);
 	readmem((char *)device_name, (int)conn_st.cnt_name, DEVNAME_SIZE);
	printf("\tConfig controller name: %s, config controller number: %d\n",
		device_name, conn_st.cnt_number);
    }
	
}

/*
 *
 *   Name:	printmscp_reqb		- Print contents of a request block
 *
 *   Abstract:	Disect and display a request block.
 *
 *   Inputs:    reqp	 	- Pointer to a request block
 *		printmode	- Controlls output level.
 *
 *   Return	NONE
 *   Values:
 */
int 
printmscp_reqb(reqp, printmode)
    REQB	*reqp;
    int		printmode;

{
	REQB reqb_st;

	printf("\n\t-----------------  REQUEST BLOCK  ---------------------\n");
	if (reqp == 0) {
		printf("\tNULL request block pointer\n");
		return;
	}
	if(readmem((char *) &reqb_st, (int)reqp, sizeof(REQB)) !=
	    sizeof(REQB)) {
		printf("could not read requets block at 0x%x.\n",reqp);
		return;
	}

    if (printmode & MSCP_PRINTFULL) {
	printf("\tRequest block address is 0x%x\n",reqp);
	printf("\tRequest block list: \(flink\) 0x%x, \(blink\) 0x%x\n",
		reqb_st.flink, reqb_st.blink);
	if ((reqb_st.flink == reqb_st.blink) && (reqb_st.blink == reqp)) {
		printf("\n\tWarning: this is a non-active request packet!\n");
		printf("\tThe following values may therefore be invalid.\n\n");
	}
	printf("\tClass block: \(0x%x\) ",reqb_st.classb);
	praddr(reqb_st.classb);
	printf("\n");
	printf("\tConnection block: 0x%x, unit block: 0x%x\n",
		reqb_st.connb, reqb_st.unitb);
	printf("\tBuf pointer: 0x%x, Message pointer: 0x%x\n",
		reqb_st.bufptr, reqb_st.msgptr);
	printf("\tMessage size: %d, p1: %d, p2: %d\n",
		reqb_st.msgsize, reqb_st.p1, reqb_st.p2);
	printf("\tAuxiliary ptr: 0x%x, rspid: 0x%x\n",
		reqb_st.aux, reqb_st.rspid);
	printf("\tOperation sequence: %d, Resource wait pointer: 0x%x\n",
		reqb_st.op_seq_num, reqb_st.rwaitptr);
	print_lbhandle(&reqb_st.lbhandle);
	printf("\tState: %d \n",reqb_st.state);
	printf("\tState table: \(0x%x\) ",reqb_st.state_tbl);
	praddr(reqb_st.state_tbl);
	printf("\n");
	printf("\tflags: ");
		if (reqb_st.flags.perm_reqb)
			printf("perm_reqb ");
		if (reqb_st.flags.nocreditw)
			printf("nocreditw ");
		if (reqb_st.flags.online)
			printf("online ");
		if (reqb_st.flags.force)
			printf("force ");
	printf("\n");
    }
    else {
	printf("\tNO printing requested.\n");
    }
}

/*
 *
 *   Name:	printmscp_classb	- Print contents of a class block
 *
 *   Abstract:	Disect and display a class block.
 *
 *   Inputs:    classp	 	- Pointer to a class block
 *		printmode	- Controlls output level.
 *
 *   Return	NONE
 *   Values:
 */
int 
printmscp_classb(classp, printmode)
    CLASSB	*classp;
    int		printmode;

{
	CLASSB class_st;
	char device_name[DEVNAME_SIZE];

	printf("\n\t-----------------  CLASS BLOCK  -----------------------\n");
	if (classp == 0) {
		printf("\tNULL class block pointer\n");
		return;
	}
	if(readmem((char *) &class_st, (int)classp, sizeof(CLASSB)) !=
	    sizeof(CLASSB)) {
		printf("could not read class block at 0x%x.\n",classp);
		return;
	}

    if (printmode & MSCP_PRINTFULL) {
	printf("\tClass block address is 0x%x\n",classp);
	printf("\tConnection list: \(flink\) 0x%x, \(blink\) 0x%x\n",
			class_st.flink, class_st.blink);
	printf("\trspid wait queue: \(flink\) 0x%x, \(blink\) 0x%x\n",
			class_st.rspid_wq.flink, class_st.rspid_wq.blink);
	printf("\toperation count: %d, system count: %d\n",
			class_st.operation_ct, class_st.system_ct);
 	readmem((char *)device_name, (int)class_st.dev_name, DEVNAME_SIZE);
	printf("\tDevice name: %s \(address 0x%x\)\n",
			device_name, class_st.dev_name);
	printf("\tUnit table: \(0x%x\) ",class_st.unit_tbl);
	praddr(class_st.unit_tbl);
	printf("\n");
	printf("\tRecovery state table: \(0x%x\) ",class_st.recov_states);
	praddr(class_st.recov_states);
	printf("\n");
	printf("\tServices blocks: \(connection mgt\) 0x%x, \(maint\) 0x%x\n",
			class_st.cmsb, class_st.msb);
	printf("\tflags: ");
		if (class_st.flags.disk)
			printf("disk ");
		if (class_st.flags.init_done)
			printf("init_done ");
		if (class_st.flags.init_ip)
			printf("init_ip ");
		if (class_st.flags.need_poll)
			printf("need_poll ");
		if (class_st.flags.listen)
			printf("listen ");
	printf("\n");
    }
    else if (printmode & MSCP_PRINTBRIEF) {
	printf("\tClass block address is 0x%x\n",classp);
 	readmem((char *)device_name, (int)class_st.dev_name, DEVNAME_SIZE);
	printf("\tDevice name: %s \n", device_name);
    }
}

/*
 *
 *   Name:	printmscp_unitb	- Print contents of a unit block
 *
 *   Abstract:	Disect and display a unit block.
 *
 *   Inputs:    unitp	 	- Pointer to a unit block
 *		printmode	- Controlls output level.
 *
 *   Return	NONE
 *   Values:
 */

int 
printmscp_unitb(unitp, printmode)
    UNITB	*unitp;
    int		printmode;

{
	UNITB unit_st;
	char device_name[DEVNAME_SIZE];

	printf("\n\t-----------------  UNIT BLOCK  ------------------------\n");
	if (unitp == 0) {
		printf("\tNULL unit block pointer\n");
		return;
	}
	if(readmem((char *) &unit_st, (int)unitp, sizeof(UNITB)) !=
	    sizeof(UNITB)) {
		printf("could not read unit block at 0x%x.\n",unitp);
		return;
	}

    if (printmode & MSCP_PRINTFULL) {
	printf("\tUnit block address is 0x%x\n",unitp);
	printf("\tUnit block list: \(flink\) 0x%x, \(blink\) 0x%x\n",
		unit_st.flink, unit_st.blink);
	printf("\tConnection block: 0x%x, ubadev: 0x%x\n",
		unit_st.connb, unit_st.ubdev);
	printf("\tRequest block list: \(flink\) 0x%x, \(blink\) 0x%x\n",
		unit_st.request.flink, unit_st.request.blink);
	printf("\tState: 0x%x\n", unit_st.state);
	printf("\tFlags: ");
		if (unit_st.flags.alonl)
			printf("alonl ");
		if (unit_st.flags.busy)
			printf("busy ");
		if (unit_st.flags.online)
			printf("online ");
		if (unit_st.flags.online_ip)
			printf("online_ip ");
		if (unit_st.flags.close_ip)
			printf("close_ip ");
		if (unit_st.flags.rct_pres)
			printf("rct_pres ");
		if (unit_st.flags.wrtp)
			printf("wrtp ");
		if (unit_st.flags.force_scan_ip)
			printf("force_scan_ip ");
		if (unit_st.flags.wait_bump)
			printf("wait_bump ");
		if (unit_st.flags.excl_acc)
			printf("excl_acc ");
		if (unit_st.flags.mscp_wait)
			printf("mscp_wait ");
	printf("\n");
	printf("\tUnit number: %d\n", unit_st.unit);
	printf("\tResource wait counter: %d, partition mask: %d, dev: 0x%x\n",
		unit_st.rwaitct, unit_st.part_mask, unit_st.dev);
	printf("\tMulti-unit code: %d, Unit flags: 0x%x:\n",
		unit_st.mult_unt, unit_st.unt_flgs);
	print_uniq_id(&unit_st.unit_id);
	mscp_media_to_ascii( unit_st.media_id, device_name );
	printf("\tMedia ID: \(0x%x\) %s\n", unit_st.media_id, device_name);
	printf("\tShadow unit: %d, shadow status: 0x%x\n",
		unit_st.shdw_unt, unit_st.shdw_sts);
	printf("\tTrack size: %d, group size: %d, cylinder size: %d\n",
		unit_st.track, unit_st.group, unit_st.cylinder);
	printf("\tUnit SW version: %d, unit HW version: %d, RCT size: %d\n",
		unit_st.unit_svr, unit_st.unit_hvr, unit_st.rct_size);
	printf("\tRBNs per track: %d, RCT copies: %d, unit size: %d\n",
		unit_st.rbns, unit_st.rct_cpys, unit_st.unt_size);
	printf("\tVolume serial: %d, total size: %d, 1st bad lbn: %d\n",
		unit_st.vol_ser, unit_st.tot_size, unit_st.acc_badlbn);
	printf("\tAccess byte ct: %d, access status: 0x%x, access flags: 0x%x\n"
		,unit_st.acc_bytecnt, unit_st.acc_status, unit_st.acc_flags);
	printf("\tTms soft errors: %d, tms hard errors: %d\n",
		unit_st.tms_softcnt, unit_st.tms_hardcnt);
	printf("\tTms category: 0x%x, tms position: 0x%x, tms max xfer 0x%x\n",
		unit_st.tms_category_flags, unit_st.tms_position, 
		unit_st.tms_bcount);
	printf("\tTms format: 0x%x, tms speed: 0x%x\n",
		unit_st.tms_format, unit_st.tms_speed);
	printf("\tTms noise: 0x%x, tms format menu: 0x%x\n",
		unit_st.tms_noise, unit_st.format_menu);
	printf("\tTms state flags: ");
		if (unit_st.state_flags.Sflags.tms_serex)
			printf("tms_serex ");
		if (unit_st.state_flags.Sflags.tms_clserex)
			printf("tms_clserex ");
		if (unit_st.state_flags.Sflags.tms_eom)
			printf("tms_eom ");
		if (unit_st.state_flags.Sflags.tms_tm)
			printf("tms_tm ");
		if (unit_st.state_flags.Sflags.tms_write)
			printf("tms_write ");
		if (unit_st.state_flags.Sflags.tms_lost)
			printf("tms_lost ");
		if (unit_st.state_flags.Sflags.tms_bufmark)
			printf("tms_bufmark ");
		if (unit_st.state_flags.Sflags.tms_cach)
			printf("tms_cach ");
		if (unit_st.state_flags.Sflags.tms_cach_on)
			printf("tms_cach_on ");
		if (unit_st.state_flags.Sflags.tms_cache_lost)
			printf("tms_cache_lost ");
		if (unit_st.state_flags.Sflags.tms_inuse)
			printf("tms_inuse ");
		if (unit_st.state_flags.Sflags.tms_wait)
			printf("tms_wait ");
		if (unit_st.state_flags.Sflags.tms_cach_write)
			printf("tms_cach_write ");
	printf("\n");
	printf("\tSel: 0x%x, tms endcode: 0x%x, tms status: 0x%x\n",
		unit_st.sel, unit_st.tms_endcode, unit_st.tms_status);
	printf("\tTms flags: 0x%x, tms resid: %d, command reference: 0x%x\n",
		unit_st.tms_flags, unit_st.tms_resid, unit_st.cmd_ref);
	printf("\tRecovery location: 0x%x\n",unit_st.tms_recovery_location);
	printf("\tMedia type: ");
	if (unit_st.mscp_device)
		printf("%s\n", unit_st.mscp_device);
	else
		printf("NULL\n");
	printf("\tPartition info struct: 0x%x\n",unit_st.part_info);
	printf("\tEmbedded raw buf struct: 0x%x\n",unit_st.rawbuf);
	printf("\tEmbedded ioctl buf struct: 0x%x\n",unit_st.ioctlbuf);
    }
    else if (printmode & MSCP_PRINTBRIEF) {
	printf("\tUnit block address is 0x%x\n",unitp);
	printf("\tUnit number: %d, ", unit_st.unit);
	mscp_media_to_ascii( unit_st.media_id, device_name );
	printf("\tMedia ID: %s\n", device_name);
    }
}

/*
 *
 *   Name:	mscp_media_to_ascii - Convert MSCP media code to ASCII.
 *
 *   Abstract:	Convert the mscp media identifier to an ascii string.
 *		Don't ask me how this works!
 *
 *   Inputs:    media	The mscp media id number.
 *		ascii	A character array which will contain the ascii string
 *			corresponding to the media id number.
 *
 *   Outputs:	NONE
 *
 *
 *   Return	NONE
 *   Values:
 */

mscp_media_to_ascii( media, ascii )
    register u_long		media;
    register u_char		*ascii;

{
    register u_long		temp;

    *ascii++ = (( media >> 17 ) & 0x1f ) + 'A' - 1;
    if( temp = (( media >> 12 ) & 0x1f )) {
	*ascii++ = ( temp + 'A' - 1 );
	if( temp = (( media >> 7 ) & 0x1f ))
	    *ascii++ = ( temp + 'A' - 1 );
    }
    *ascii++ = (( media & 0x7f ) / 10 ) + '0';
    *ascii++ = (( media & 0x7f ) % 10 ) + '0';
    *ascii = '\0';
}

/*
 *
 *   Name:      print_unit_table        - Display unit table.
 *
 *   Abstract:  This routine will print out the unit blocks in the unit
 *              table.  It uses global symbols (mscp_unit_tbl and
 *              tmscp_unit_tbl) to determine the starting address of the
 *              tables.
 *
 *   Inputs:    subsys_name     - The name of the particular subsystem.
 *              printmode       - Controlls output level.
 *
 *   Return     NONE
 *   Values:
 */

int
print_unit_table(subsys_name, printmode)
    int         subsys_name;
    int         printmode;

{
        UNITB *unitb_addr;
	int   *unit_ptr;
        int   slot_number, numslots;
	char  title[80], tablename[80];


        if (subsys_name & MSCP_DISK) {
		strcpy(title,"MSCP DISK UNIT TABLE");
		strcpy(tablename,"mscp_unit_tbl");
		unit_ptr = (int *)Mscp_utable.s_value;
		numslots = NUNIT;
	}
        if (subsys_name & MSCP_TAPE) {
		strcpy(title,"TMSCP TAPE UNIT TABLE");
		strcpy(tablename,"tmscp_unit_tbl");
		unit_ptr = (int *)Tmscp_utable.s_value;
		numslots = NTUNIT;
	}

       printf("\n");
       printf("%s \(0x%x\):\n",title, unit_ptr);
       for (slot_number = 0; slot_number < numslots; slot_number++) {
		if (unit_ptr == 0) {
			printf("\tNULL unit block pointer\n");
			return;
		}
		if(readmem((char *) &unitb_addr, (int)unit_ptr, 
			sizeof(UNITB *)) != sizeof(UNITB *)) {
    		
			printf("could not read unit block pointer at 0x%x.\n",unit_ptr);
			return;
		}
		printf("%s[%d] : ",tablename, slot_number);
		if (unitb_addr != 0)
			printmscp_unitb(unitb_addr, printmode);
		else
			printf(" NULL\n");
                unit_ptr++;
       }
}

/*
 *
 *   Name:	printmscp_devunit	- Print unit block for a given dev_t.
 *
 *   Abstract:	Print out the contents of a unit block that corresponds to the
 *		major and minor number passed in as agruments.
 *
 *   Inputs:    majno		- Major number of device special file.
 *   		minno		- Minor number of device special file.
 *		printmode	- Controlls output level.
 *
 *   Return	NONE
 *   Values:
 */
int 
printmscp_devunit(majno, minno, printmode)
    int		majno;
    int		minno;
    int		printmode;

{
	dev_t devt;
	int   unit_number;
        UNITB *unitb_addr;
	int  *unit_ptr;
	
	/*
	 * Setup the dev_t with the low 8 bits as the minor number and the
 	 * high 8 bits as the major number.
	 */
	devt = minno & 0xFF;
	devt |= (majno & 0xFF) << 8;

	/*
	 * Make sure the major/minor pairs correspond to an MSCP disk or a 
	 * TMSCP tape.  MSCP disks are in a range of major numbers; the
	 * MSCP character special devs are checked by MSCP_C_DEV, the MSCP
	 * block devices are checked by MSCP_B_DEV.  For TMSCP devices the 
	 * character major number is 36 and the block special file is not
	 * supported.
	 */
	if ((MSCP_C_DEV(devt) == 0) && (MSCP_B_DEV(devt) == 0) &&
		(majno != 36) ) {
		printf("The dev_t 0x%x does not correspond to an MSCP device\n",
			devt);
		return;
	}
	/*
	 * Make sure the unit number looks reasonable.
	 * Setup unit_ptr to point to the base of the corresponding unit table.
	 */
	if (majno == 36) {	/* TMSCP */
		unit_number = UNIT(devt);
		if (unit_number > NUNIT) {
			printf("Specified unit %d is > max unit number %d\n", 
				unit_number, NUNIT);
			return;
		}
		unit_ptr = (int *)Tmscp_utable.s_value;
	}
	else {			/* MSCP */
		unit_number = Ux(devt);
		if (unit_number > NUNIT) {
			printf("Specified unit %d is > max unit number %d\n", 
				unit_number, NUNIT);
			return;
		}
		unit_ptr = (int *)Mscp_utable.s_value;
	}
	/*
	 * Offset within the unit table to the appropriate unit block and 
	 * print out the contents.
	 */
	unit_ptr += unit_number;
	if(readmem((char *) &unitb_addr, (int)unit_ptr, 
		sizeof(UNITB *)) != sizeof(UNITB *)) {
		printf("could not read unit block pointer at 0x%x.\n",unit_ptr);
		return;
	}
	printmscp_unitb(unitb_addr, printmode);
}

/*
 *
 *   Name:	print_uniq_id	- Print  out a UNIQ_ID structure
 *
 *   Abstract:	Print out the contents of a UNIQ_ID structure.  This involves
 *		translating the 3 elements of the structure.
 *
 *   Inputs:    uniq_id		- pointer to a UNIQ_ID data struct
 *
 *   Return	NONE
 *   Values:
 */
int 
print_uniq_id(uniq_id)
	UNIQ_ID *uniq_id;
{
	printf("\tDevice_no: 0x%x, ", uniq_id->device_no);
	printf("Model number: %d, Device class: %d \(",
		uniq_id->model, uniq_id->class);
	/*
	 * This should be done by reading in the "cu_class" table and
	 * looking for matches there.
	 */
	switch( uniq_id->class ) {
		case 0:	printf("Reserved"); break;
		case 1:	printf("Mass storage"); break;
		case 2:	printf("Disk"); break;
		case 3:	printf("Tape"); break;
		case 4:	printf("Disk"); break;
		case 5:	printf("Loader"); break;
		default: printf("Unknown");
	}
	printf("\)\n");
}
/*
 *
 *   Name:	print_lbhandle	- Local Buffer Handle
 *
 *   Abstract:	Print out the contents of a BHANDLE structure.  This involves
 *		translating the 3 elements of the structure.
 *
 *   Inputs:    lbhandle	- pointer to a BHANDLE data struct
 *
 *   Return	NONE
 *   Values:
 */
int 
print_lbhandle(lbhandle)
	BHANDLE *lbhandle;
{
	printf("\tLocal Buffer Handle: ");
	/*
	 * This should be enhanced to more fully display the pd element.
	 */
	printf("pd handle not translated, ");
	printf("scsid 0x%x",lbhandle->scsid);
	printf("\n");
}
