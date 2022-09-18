#ifndef	lint 
static char *sccsid = "@(#)scs.c	4.1      (ULTRIX)        7/17/90";
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
 *   Facility:	SCS data structure formatter for the crash utility.
 *
 *   Abstract:	This module contains the routines used to display the
 *		contents of SCS data structures.  Routines are also
 *		provided to traverse data structure lists.
 *
 *   Author:	Tim Burke	November 22, 1989
 *
 *   Modification History:
 *
 */

#include 	<ctype.h>
#include 	<sys/types.h>
#include 	<sys/time.h>
#include 	<sys/param.h>
#include 	<sys/ksched.h>
#include 	<sys/dyntypes.h>
#include 	<sys/limits.h>
#include 	<sys/utsname.h>
#include 	<sys/kmalloc.h>
#include 	<sys/errlog.h>
#include 	<sys/smp_lock.h>
#include	<io/scs/sca.h>
#include	<io/scs/scaparam.h>
#include	<io/scs/scamachmac.h>
#include	<io/ci/cippdsysap.h>
#include	<io/ci/cisysap.h>
#include	<io/msi/msisysap.h>
#include	<io/bi/bvpsysap.h>
#include	<io/gvp/gvpsysap.h>
#include	<io/uba/uqsysap.h>
#include	<io/sysap/sysap.h>
#include	<io/ci/cippdscs.h>
#include	<io/ci/ciscs.h>
#include	<io/msi/msiscs.h>
#include	<io/bi/bvpscs.h>
#include	<io/gvp/gvpscs.h>
#include	<io/uba/uqscs.h>
#include	<io/scs/scs.h>
#include        "crash.h"
#include        <stdio.h>

/*
 * Defines local to this program.
 */

/*
 * MSCP printing levels - controls ammount of output.
 */
#define MSCP_PRINTFULL	0x01		/* Verbose printing		*/
#define MSCP_PRINTBRIEF	0x02		/* Brief printing		*/
/*
 * SCS printing levels - controls ammount of output.
 */
#define SCS_PRINTFULL   0x01            /* Verbose printing             */
#define SCS_PRINTBRIEF  0x02            /* Brief printing               */
/*
 * Types of queues pointed to by _pbq's and _cbq's.
 */
#define	X_PATHB		1	/* Path block queue			*/
#define	X_TIMEOUT	2	/* SCS protocol seq timeout pb queue	*/
#define	X_CBS		3	/* Connection block queue		*/
#define	X_SCS_CB	4	/* SCS waiting connection block queue	*/
char *token();
c_scs(c)
	char *c;
{
	char *arg;
	int index;
	unsigned int addr;
	
	arg = token();
	if (arg == NULL) {
		(void)printscs(0, SCS_PRINTFULL);
		return;
	}
	else if (strcmp(arg,"-sb") == 0) {
		arg = token();
		if (arg == NULL) {
			printf("usage: -sb requires address\n");
			return;
		}
		addr = scan_vaddr(arg);
		(void)printscs_system_block(addr, SCS_PRINTFULL);
	}
	else if (strcmp(arg,"-pb") == 0) {
		arg = token();
		if (arg == NULL) {
			printf("usage: -pb requires address\n");
			return;
		}
		addr = scan_vaddr(arg);
		(void)printscs_pb(addr, SCS_PRINTFULL);
	}
	else if (strcmp(arg,"-pib") == 0) {
		arg = token();
		if (arg == NULL) {
			printf("usage: -pib requires address\n");
			return;
		}
		addr = scan_vaddr(arg);
		(void)printscs_pib(addr, SCS_PRINTFULL);
	}
	else if (strcmp(arg,"-cb") == 0) {
		arg = token();
		if (arg == NULL) {
			printf("usage: -cb requires address\n");
			return;
		}
		addr = scan_vaddr(arg);
		(void)printscs_cb(addr, SCS_PRINTFULL);
	}
	else if (strcmp(arg,"-sib") == 0) {
		arg = token();
		if (arg == NULL) {
			printf("usage: -sib requires address\n");
			return;
		}
		addr = scan_vaddr(arg);
		(void)printscs_sib(addr, SCS_PRINTFULL);
	}
	else if (strcmp(arg,"-cib") == 0) {
		arg = token();
		if (arg == NULL) {
			printf("usage: -cib requires address\n");
			return;
		}
		addr = scan_vaddr(arg);
		(void)printscs_cib(addr, SCS_PRINTFULL);
	}
	else {
		printf("Invalid scs parameter: %s\n",arg);
	}
	while(token()!=NULL);
}

/*
 *
 *   Name:	printscs	- Traverse and display data structs
 *
 *   Abstract:	This routine will start at the globaly defined scs 
 *		scs configuration database pointer (scs_config_db) and
 *	        traverse the data structures printing results.
 *
 *   Inputs:    subsys_name 	- The name of the particular subsystem.
 *		printmode	- Controlls output level.
 *
 *   Return	NONE
 *   Values:
 */

int 
printscs(subsys_name, printmode)
    int		subsys_name;
    int		printmode;

{
        sbq *scs_config_db;

	printf("\n.................... SCS SUBSYSTEM  .....................\n");
	scs_config_db = (sbq *)Scs_config_db.s_value;
	printf("\tscs_config_db address is 0x%x\n",scs_config_db);
	scs_follow_systems(printmode);
}

/*
 *
 *   Name:	scs_follow_systems	- Follow a list of system blocks
 *			starting at the global address "_scs_config_db".
 *
 *   Abstract:	This routine is used to traverse the scs system blocks.
 *		call the appropriate routine to print out the contents of
 *		the system blocks.
 *
 *   Inputs:    
 *		printmode	- Controlls output level.
 *
 *   Return	NONE
 *   Values:
 */
int 
scs_follow_systems(printmode)
    int		printmode;

{
	sbq *scs_config_db;
	sbq config_db;
	sbq *sb_ptr;
	SB sb_st;

	scs_config_db = (sbq *)Scs_config_db.s_value;
	if (scs_config_db == 0) {
		printf("\tNULL scs_config_db\n");
		return;
	}
	if(readmem((char *) &config_db, (int)scs_config_db, sizeof(sbq)) !=
	    sizeof(sbq)) {
		printf("could not read scs_config_db at 0x%x.\n",scs_config_db);
		return;
	}

	if ((config_db.flink == config_db.blink) &&
	    (config_db.flink == (sbq *)scs_config_db)) {
		printf("\n\tThe forward and backward pointer of the\n");
		printf("\tscs_config_db are the same and point to the\n");
		printf("\ttscs_config_db itself.\n");
		return;
	}
	
	sb_ptr = config_db.flink;
	do {
	    printscs_system_block(sb_ptr, printmode);
	    if(readmem((char *) &sb_st, (int)sb_ptr, sizeof(SB)) !=
	        sizeof(SB)) {
		    printf("could not read system block at 0x%x\n",sb_ptr);
		    return;
	    }
	    sb_ptr = sb_st.flink;
	} while  (sb_ptr != (sbq *)scs_config_db);
}

/*
 *
 *   Name:	printscs_system_block	- Print contents of a system block
 *
 *   Abstract:	Disect and display a system block.
 *
 *   Inputs:    sb_ptr	 	- Pointer to a system block
 *		printmode	- Controlls output level.
 *
 *   Return	NONE
 *   Values:
 */
int 
printscs_system_block(sb_ptr, printmode)
    sbq		*sb_ptr;
    int		printmode;

{
	SB sb_st;
	int offset;

	printf("\n\t*****************  SYSTEM BLOCK  ******************\n");
	if (sb_ptr == 0) {
		printf("\tNULL system block pointer\n");
		return;
	}
	if(readmem((char *) &sb_st, (int)sb_ptr, sizeof(SB)) !=
	    sizeof(SB)) {
		printf("could not read system block at 0x%x.\n",sb_ptr);
		return;
	}

    if (printmode & SCS_PRINTFULL) {
	printf("\tSystem block address is 0x%x\n",sb_ptr);
	printf("\tflink: 0x%x, blink: 0x%x\n",sb_st.flink, sb_st.blink);
	printf("\tsize: %d, type: 0x%x\n",sb_st.size, sb_st.type);
	offset = (int)(&sb_st.pbs.flink) - (int)(&sb_st.flink);
	printscs_pbq((char *) ((char *)sb_ptr + offset), printmode, X_PATHB);
	offset = (int)(&sb_st.sinfo.sysid) - (int)(&sb_st.flink);
	printscs_sib((char *) ((char *)sb_ptr + offset), printmode);
    }
}

/*
 *
 *   Name:	printscs_pbq	- Print contents of a path block queue head.
 *
 *   Abstract:	Disect and display a path block queue head.
 *
 *   Inputs:    pbq_ptr	 	- Pointer to a path block queue head.
 *		printmode	- Controlls output level.
 *
 *   Return	NONE
 *   Values:
 */
int 
printscs_pbq(pbq_ptr, printmode, pbq_type)
    pbq		*pbq_ptr;
    int		printmode;
    int		pbq_type;

{
	pbq pbq_st;
	char *printstring;

	switch (pbq_type) {
		case X_PATHB: printstring = "path block"; break;
		case X_TIMEOUT: printstring = "timeout"; break;
		default: printstring = "UNKNOWN"; 
	}
        if (pbq_ptr == 0) {
	    printf("\tNULL %s queue head.\n",printstring);
	    return;
        }
	printf("\t%s queue: ", printstring);
	if(readmem((char *) &pbq_st, (int)pbq_ptr, sizeof(pbq)) !=
	    sizeof(pbq)) {
		printf("could not read %s queue head at 0x%x.\n", printstring,
			pbq_ptr);
		return;
	}

    if (printmode & SCS_PRINTFULL) {
	printf("flink 0x%x, blink 0x%x",pbq_st.flink, pbq_st.blink);
    }
    printf("\n");
    printscs_follow_pbq(pbq_ptr, printmode); 
}

/*
 *
 *   Name:	printscs_sib	- Print contents of a system information block.
 *
 *   Abstract:	Disect and display a system information block queue head.
 *
 *   Inputs:    sib_ptr	 	- Pointer to a system information block.
 *		printmode	- Controlls output level.
 *
 *   Return	NONE
 *   Values:
 */
int 
printscs_sib(sib_ptr, printmode)
    SIB		*sib_ptr;
    int		printmode;

{
	SIB sib_st;
	int i;
	char c;


	printf("\t------------- SYSTEM INFORMATION BLOCK ----------\n");
        if (sib_ptr == 0) {
	    printf("\tNULL system information block pointer.\n");
	    return;
        }
	if(readmem((char *) &sib_st, (int)sib_ptr, sizeof(SIB)) !=
	    sizeof(SIB)) {
		printf("could not read system information block at 0x%x.\n",
			sib_ptr);
		return;
	}

    if (printmode & SCS_PRINTFULL) {
	printf("\tSystem information block address is 0x%x\n",sib_ptr);
	printscs_scaddr((c_scaaddr *) sib_ptr, printmode); 
	printf("\tnpaths: %d, max_dg: %d, max_msg: %d\n", sib_st.npaths,
		sib_st.max_dg, sib_st.max_msg);
	printf("\tswtype: ");
		for (i=0; i < 4; i++) {
			c = (u_char)(sib_st.swtype >> 8*i);
			if (isprint(c))
				printf("%c",c);
		}
	printf(", swver: ");
		for (i=0; i < 4; i++) {
			c = (u_char)(sib_st.swver >> 8*i);
			if (isprint(c))
				printf("%c",c);
		}
	printf(", swincrn: 0x%x\n", sib_st.swincrn);
	printf("\thwtype: ");
		for (i=0; i < 4; i++) {
			c = (u_char)(sib_st.hwtype >> 8*i);
			if (isprint(c))
				printf("%c",c);
		}
	printf(", hwver: %d, %d, %d\n", 
		sib_st.hwver.val[0], sib_st.hwver.val[1], sib_st.hwver.val[2]);
	printf("\tnode_name: ");
	for (i=0; i < NODENAME_SIZE; i++) {
	    if (isprint(sib_st.node_name[i]))
		printf("%c",sib_st.node_name[i]);
	}
    }
    printf("\n");
}

/*
 *
 *   Name:	printscs_scaddr	- Print a system identification number.
 *
 *   Abstract:	Disect and display a system identification number.
 *
 *   Inputs:    scaddr_ptr	 	- Pointer to a system type c_scaaddr
 *		printmode		- Controlls output level.
 *
 *   Return	NONE
 *   Values:
 */
int 
printscs_scaddr(scaddr_ptr, printmode)
    c_scaaddr	*scaddr_ptr;
    int		printmode;

{
	c_scaaddr scaddr_st;
	int i;

	printf("\tSystem Identification Number: ");

        if (scaddr_ptr == 0) {
	    printf("\tNULL System ID pointer.\n");
	    return;
        }
	if(readmem((char *) &scaddr_st, (int)scaddr_ptr, sizeof(c_scaaddr)) !=
	    sizeof(c_scaaddr)) {
		printf("could not read system identification number at 0x%x.\n",
			scaddr_ptr);
		return;
	}

    if (printmode & SCS_PRINTFULL) {
	/*
	 * The type c_scaaddr is defined as "u_char val[ 6 ]" , the low 4
	 * bytes are an address and the upper 2 bytes is a port type.
	 */
	for (i=5; i >= 0; i--) {
		if (i%2) {
			printf("0x");
		}
		printf("%x",scaddr_st.val[i]);
		if (((i%2) == 0) && (i != 0)) {
			printf(", ");
		}
	}
    }
    printf("\n");
}

/*
 *
 *   Name:	printscs_follow_pbq	- Follow links in a path block queue.
 *
 *   Abstract:	Get to path blocks in a list.
 *
 *   Inputs:    pbq_ptr	 	- Pointer to a path block queue head.
 *		printmode	- Controlls output level.
 *
 *   Return	NONE
 *   Values:
 */
int 
printscs_follow_pbq(pbq_ptr, printmode)
    pbq		*pbq_ptr;
    int		printmode;

{
	pbq pbq_st;
	PB  pb_st;
	PB  *pb_ptr;

	if(readmem((char *) &pbq_st, (int)pbq_ptr, sizeof(pbq)) !=
	    sizeof(pbq)) {
		printf("could not path block queue head at 0x%x.\n",pbq_ptr);
		return;
	}

    if (pbq_ptr == 0) {
	printf("\tNULL path block queue head.\n");
	return;
    }
    if ((pbq_st.flink == pbq_st.blink) && (pbq_st.flink == pbq_ptr)) {
	printf("There are no allocated path blocks.\n");
	return;
    }
    pb_ptr = (PB *)pbq_st.flink;
    do {
	printscs_pb(pb_ptr, printmode);
	if(readmem((char *) &pb_st, (int)pb_ptr, sizeof(PB)) !=
	    sizeof(PB)) {
	    	printf("could not read path block at 0x%x\n",pb_ptr);
		return;
	}
	pb_ptr = (PB *)pb_st.flink;
    } while (pb_ptr != (PB *)pbq_ptr);
}

/*
 *
 *   Name:	printscs_pb	- Print contents of a path block structure.
 *
 *   Abstract:	Disect and display a path block. 
 *
 *   Inputs:    pb_ptr	 	- Pointer to a path block. 
 *		printmode	- Controlls output level.
 *
 *   Return	NONE
 *   Values:
 */
int 
printscs_pb(pb_ptr, printmode)
    PB		*pb_ptr;
    int		printmode;

{
	PB pb_st;
	int offset;

        if (pb_ptr == 0) {
	    printf("\tNULL path block pointer.\n");
	    return;
        }
	printf("\t\t---------------- PATH BLOCK ---------------------\n");
	if(readmem((char *) &pb_st, (int)pb_ptr, sizeof(PB)) !=
	    sizeof(PB)) {
		printf("could not read path block at 0x%x.\n",pb_ptr);
		return;
	}

    if (printmode & SCS_PRINTFULL) {
	printf("\t\tPath block address is 0x%x\n",pb_ptr);
	printf("\t\tflink: 0x%x, blink: 0x%x\n",pb_st.flink, pb_st.blink);
	printf("\t\tsize: %d, type: %d\n",pb_st.size, pb_st.type);
	printf("\t\ttimeout queue: flink 0x%x, blink 0x%x ",
		pb_st.timeout.flink, pb_st.timeout.blink);
	offset = (int)(&pb_st.timeout.flink) - (int)(&pb_st.flink);
	if (((char *)pb_ptr + offset) == (char *)pb_st.timeout.flink) {
		printf("\(EMPTY\) ");
	}
	printf("\n");
	printf("\t\tCB queue: flink 0x%x, blink 0x%x ",
		pb_st.cbs.flink, pb_st.cbs.blink);
	offset = (int)(&pb_st.cbs.flink) - (int)(&pb_st.flink);
	if (((char *)pb_ptr + offset) != (char *)pb_st.cbs.flink) {
		printf("\n");
		printscs_cbq((char *) ((char *)pb_ptr + offset), printmode,
			X_CBS);
	}
	else {
		printf("\(EMPTY\)\n");
	}
	printf("\t\tSCS waiting CB queue: flink 0x%x, blink 0x%x ",
		pb_st.scs_cb.flink, pb_st.scs_cb.blink);
	offset = (int)(&pb_st.scs_cb.flink) - (int)(&pb_st.flink);
	if (((char *)pb_ptr + offset) != (char *)pb_st.scs_cb.flink) {
		printf("\n");
		printscs_cbq((char *) ((char *)pb_ptr + offset), printmode,
			X_SCS_CB);
	}
	else {
		printf("\(EMPTY\)\n");
	}
	printf("\t\tsb: 0x%x, pccb: 0x%x, pdt: 0x%x\n", pb_st.sb, pb_st.pccb,
		pb_st.pdt);
	printf("\t\tSCS send message buffer pointer: 0x%x\n",pb_st.scs_msgbuf);
	offset = (int)(&pb_st.pb_lk) - (int)(&pb_st.flink);
	print_lockt((char *) ((char *)pb_ptr + offset), printmode);
	offset = (int)(&pb_st.pd) - (int)(&pb_st.flink);
	printscs_gvp((char *) ((char *)pb_ptr + offset), printmode);
	offset = (int)(&pb_st.ppd) - (int)(&pb_st.flink);
	printscs_cipppd((char *) ((char *)pb_ptr + offset), printmode);
	offset = (int)(&pb_st.pinfo) - (int)(&pb_st.flink);
	printscs_pib((char *) ((char *)pb_ptr + offset), printmode);
    }
}

/*
 *
 *   Name:	print_lockt	- Print contents of a lock structure
 *
 *   Abstract:	Disect and display a lock struct. 
 *
 *   Inputs:    lock_ptr	 	- Pointer to a path block. 
 *		printmode	- Controlls output level.
 *
 *   Return	NONE
 *   Values:
 */
int 
print_lockt(lock_ptr, printmode)
    struct lock_t	*lock_ptr;
    int		printmode;

{
	struct lock_t lock_st;

        if (lock_ptr == 0) {
	    printf("\tNULL lock structure pointer.\n");
	    return;
        }
	printf("\t\t\t---------------- LOCK STRUCTURE ------------\n");
	if(readmem((char *) &lock_st, (int)lock_ptr, sizeof(struct lock_t)) !=
	    sizeof(struct lock_t)) {
		printf("could not read lock structure at 0x%x.\n",lock_ptr);
		return;
	}

    if (printmode & SCS_PRINTFULL) {
	printf("\t\t\tl_hierpos: %d, l_type: %d, l_ipl: %d\n",
		lock_st.l_hierpos, lock_st.l_type, lock_st.l_ipl);
	printf("\t\t\tl_lock: %d, l_plock: 0x%x, l_pc: 0x%x\n",
		lock_st.l_lock, lock_st.l_plock, lock_st.l_pc);
	printf("\t\t\tl_lost: %d, l_spin: %d, l_won: %d, l_wanted: %d\n",
		lock_st.l_lost, lock_st.l_spin, lock_st.l_won,lock_st.l_wanted);
    }
}

/*
 *
 *   Name:	printscs_gvp	- Print contents of a _gvppb structure
 *
 *   Abstract:	This routine is called to print out the "pd" element of the
 *		path block data structure.  That field is defined as a union of
 *		only one element which is itself a union of type _gvppb which
 *		itself is a union of only one element which is a structure of
 *		type _cipb.  (You can call me Fred, or you can call me Sam, ...)
 *		So to make a long story short, this routine ammounts to only
 *		reading in a struct of type _cipb.
 *
 *   Inputs:    ci_ptr	 	- Pointer to a path block. 
 *		printmode	- Controlls output level.
 *
 *   Return	NONE
 *   Values:
 */
int 
printscs_gvp(ci_ptr, printmode)
    CIPB	*ci_ptr;
    int			printmode;

{
	CIPB ci_st;

        if (ci_ptr == 0) {
	    printf("\tNULL gvp structure pointer.\n");
	    return;
        }
	printf("\t\t\t---------------- GVP \(CIPB\) STRUCTURE -------\n");
	if(readmem((char *) &ci_st, (int)ci_ptr, sizeof(CIPB)) !=
	    sizeof(CIPB)) {
		printf("could not read gvp structure at 0x%x.\n",ci_ptr);
		return;
	}

    if (printmode & SCS_PRINTFULL) {
	printf("\t\t\tscpkt: 0x%x, invtcpkt: 0x%x\n",ci_st.scpkt, 
		ci_st.invtcpkt);
	printf("\t\t\tCable Status: Cable0 ");
		if (ci_st.pstatus.cable0)
			printf("Bad, ");
		else
			printf("Good, ");
		printf("Cable1 ");
		if (ci_st.pstatus.cable1)
			printf("Bad, ");
		else
			printf("Good, ");
		if (ci_st.pstatus.cables_crossed == 0)
			printf("not ");
		printf("crossed.\n");
    }
}

/*
 *
 *   Name:	printscs_cipppd	- Print contents of a _cippdpb structure
 *
 *   Abstract:	This routine is called to print out the "ppd" element of the
 *		path block data structure.  That field is defined as a union of
 *		only one element which is a structure of type _cippdpb.
 *
 *   Inputs:    cippd_ptr	 - Pointer to a CIPPDPB block. 
 *		printmode	- Controlls output level.
 *
 *   Return	NONE
 *   Values:
 */
int 
printscs_cipppd(cippd_ptr, printmode)
    CIPPDPB	*cippd_ptr;
    int		printmode;

{
	CIPPDPB cippd_st;
	int offset;

        if (cippd_ptr == 0) {
	    printf("\tNULL _cippdpb structure pointer.\n");
	    return;
        }
	printf("\t\t\t---------------- CIPPDPB STRUCTURE ------------\n");
	if(readmem((char *) &cippd_st, (int)cippd_ptr, sizeof(CIPPDPB)) !=
	    sizeof(CIPPDPB)) {
		printf("could not read cippdpb structure at 0x%x.\n",cippd_ptr);
		return;
	}

    if (printmode & SCS_PRINTFULL) {
	printf("\t\t\ttimer: %d, path_closed: %d, fkip: %d\n",
		cippd_st.fsmpstatus.timer, cippd_st.fsmpstatus.path_closed,
		cippd_st.fsmpstatus.fkip);
	printf("\t\t\tdbiip: 0x%x, due_time: %d, retry: %d\n",
		cippd_st.dbiip, cippd_st.due_time, cippd_st.retry);
	offset = (int)(&cippd_st.forkb) - (int)(&cippd_st.fsmpstatus);
	printscs_kschedblk((char *) ((char *)cippd_ptr + offset), printmode);
    }
}

/*
 *
 *   Name:	printscs_kschedblk	- Print contents of a kschedblk 
 *
 *   Abstract:	Print contents of a kschedblk.  This is otherwise refered to
 *		as a forkb.
 *
 *   Inputs:    kschedblk_ptr	- Pointer to a kschedblk. 
 *		printmode	- Controlls output level.
 *
 *   Return	NONE
 *   Values:
 */
int 
printscs_kschedblk(kschedblk_ptr, printmode)
    struct  kschedblk	*kschedblk_ptr;
    int			printmode;

{
	struct  kschedblk kschedblk_st;

        if (kschedblk_ptr == 0) {
	    printf("\tNULL kschedblk structure pointer.\n");
	    return;
        }
	printf("\t\t\t\t---------- FORKB STRUCTURE ------------\n");
	if(readmem((char *) &kschedblk_st, (int)kschedblk_ptr, 
	    sizeof(struct  kschedblk)) != sizeof(struct  kschedblk)) {
		printf("could not read kschedblk structure at 0x%x.\n",
			kschedblk_ptr);
		return;
	}

    if (printmode & SCS_PRINTFULL) {
	printf("\t\t\t\t");
	/*
	 * The following compilation error occurs when trying to display the
	 * ipl element: "warning: illegal member use: ipl".  I suspect that
	 * this is due to some form of define in a header file.  Skip the ipl
	 * field for now.
	 * printf("ipl: 0x%x, ", kschedblk_st.ipl);
	 */
	printf("arg: 0x%x, next: 0x%x\n", kschedblk_st.arg, kschedblk_st.next);
	printf("\t\t\t\t");
	printf("func: 0x%x", kschedblk_st.func);
	if (kschedblk_st.func != 0) {
		printf(" \(");
		praddr(kschedblk_st.func);
		printf("\)");
	}
	printf("\n");
    }
}

/*
 *
 *   Name:	printscs_pib	- Print contents of a path information block 
 *
 *   Abstract:	Print contents of a pib.  
 *
 *   Inputs:    pib_ptr	 	- Pointer to a pib. 
 *		printmode	- Controlls output level.
 *
 *   Return	NONE
 *   Values:
 */
int 
printscs_pib(pib_ptr, printmode)
    PIB	*pib_ptr;
    int			printmode;

{
	PIB pib_st;
	int i;
	int offset;
	u_char c;

        if (pib_ptr == 0) {
	    printf("\tNULL path information block structure pointer.\n");
	    return;
        }
	printf("\t\t\t---------------- PIB STRUCTURE ------------\n");
	if(readmem((char *) &pib_st, (int)pib_ptr, 
	    sizeof(PIB)) != sizeof(PIB)) {
		printf("could not read pib structure at 0x%x.\n",
			pib_ptr);
		return;
	}

    if (printmode & SCS_PRINTFULL) {
	printf("\t\t\tPath information block address is 0x%x\n",pib_ptr);
	printf("\t\t\tlport_name: ");
		for (i=0; i < 4; i++) {
			c = (u_char)(pib_st.lport_name >> 8*i);
			if (isprint(c))
				printf("%c",c);
		}
	printf("\n");
	printf("\t\t");
	offset = (int)(&pib_st.rport_addr) - (int)(&pib_st.lport_name);
	printscs_scaddr((char *) ((char *)pib_ptr + offset), printmode);
	printf("\t\t\tstate: 0x%x \(",pib_st.state);
		switch (pib_st.state) {
			case PS_CLOSED: printf("PS_CLOSED"); break;
			case PS_START_SNT: printf("PS_START_SNT"); break;
			case PS_START_REC: printf("PS_START_REC"); break;
			case PS_STACK_SNT: printf("PS_STACK_SNT"); break;
			case PS_OPEN: printf("PS_OPEN"); break;
			case PS_PATH_FAILURE: printf("PS_PATH_FAILURE"); break;
			default: printf("UNKNOWN"); 
		}
	printf("\), ");
	printf("hwtype: %d \(",pib_st.type.hwtype);
		switch (pib_st.type.hwtype) {
		    case HPT_UQSSP: printf("HPT_UQSSP"); break;
		    case HPT_CI780: printf("HPT_CI780"); break;
		    case HPT_CI750: printf("HPT_CI750"); break;
		    case HPT_HSC: printf("HPT_HSC"); break;
		    case HPT_CIBCI: printf("HPT_CIBCI"); break;
		    case HPT_KL10: printf("HPT_KL10"); break;
		    case HPT_CIBCA_BA: printf("HPT_CIBCA_BA"); break;
		    case HPT_CIBCA_AA: printf("HPT_CIBCA_AA"); break;
		    case HPT_BVPSSP: printf("HPT_BVPSSP"); break;
		    case HPT_CIXCD: printf("HPT_CIXCD"); break;
		    case HPT_SII: printf("HPT_SII"); break;
		    case HPT_KFQSA: printf("HPT_KFQSA"); break;
		    case HPT_SHAC: printf("HPT_SHAC"); break;
		    case HPT_KFXSA: printf("HPT_KFXSA"); break;
		    case HPT_RF71: printf("HPT_RF71"); break;
		    case HPT_RF30: printf("HPT_RF30"); break;
		    case HPT_RF31: printf("HPT_RF31"); break;
		    case HPT_TF70: printf("HPT_TF70"); break;
		    case HPT_TF85: printf("HPT_TF85"); break;
		    default: printf("UNKNOWN");
		}
	printf("\)\n");
	printf("\t\t\tdual_path: %d, reason 0x%x \n",pib_st.type.dual_path,
		pib_st.reason);
	printf("\t\t\tnconns: %d, duetime: %d, sanity: %d\n",pib_st.nconns,
		pib_st.duetime, pib_st.status.sanity);
	printf("\t\t\tprotocol: %d\n", pib_st.protocol);
	offset = (int)(&pib_st.pd) - (int)(&pib_st.lport_name);
	printscs_pd((char *) ((char *)pib_ptr + offset), printmode, 
		pib_st.type.hwtype);
    }
}

/*
 *
 *   Name:	printscs_cbq	- Print contents of a connection block queue 
 *				head.
 *
 *   Abstract:	Disect and display a connection block queue head.  Follow the 
 *		queue and print out the associated connection blocks.
 *
 *   Inputs:    cbq_ptr	 	- Pointer to a connection block queue head.
 *		printmode	- Controlls output level.
 *
 *   Return	NONE
 *   Values:
 */
int 
printscs_cbq(cbq_ptr, printmode, cbq_type)
    cbq		*cbq_ptr;
    int		printmode;
    int		cbq_type;

{
	cbq cbq_st;
	char *printstring;

	switch (cbq_type) {
		case X_CBS: printstring = "connection block"; break;
		case X_SCS_CB: printstring = "waiting connection block"; break;
		default: printstring = "UNKNOWN"; 
	}
        if (cbq_ptr == 0) {
	    printf("\tNULL %s queue head.\n",printstring);
	    return;
        }
	if(readmem((char *) &cbq_st, (int)cbq_ptr, sizeof(cbq)) !=
	    sizeof(cbq)) {
		printf("could not read %s queue head at 0x%x.\n", printstring,
			cbq_ptr);
		return;
	}

    printscs_follow_cbq(cbq_ptr, printmode); 
}

/*
 *
 *   Name:	printscs_follow_cbq	- Follow links in a connection block 
 *					 queue.
 *
 *   Abstract:	Get to connection blocks in a list.
 *
 *   Inputs:    cbq_ptr	 	- Pointer to a connection block queue head.
 *		printmode	- Controlls output level.
 *
 *   Return	NONE
 *   Values:
 */
int 
printscs_follow_cbq(cbq_ptr, printmode)
    cbq		*cbq_ptr;
    int		printmode;

{
	cbq cbq_st;
	CB  cb_st;
	CB  *cb_ptr;

	if(readmem((char *) &cbq_st, (int)cbq_ptr, sizeof(cbq)) !=
	    sizeof(cbq)) {
		printf("could not connection block queue head at 0x%x.\n",
			cbq_ptr);
		return;
	}

    if (cbq_ptr == 0) {
	printf("\tNULL connection block queue head.\n");
	return;
    }
    if ((cbq_st.flink == cbq_st.blink) && (cbq_st.flink == cbq_ptr)) {
	printf("EMPTY connection block queue.\n");
	return;
    }
    cb_ptr = (CB *)cbq_st.flink;
    do {
	printscs_cb(cb_ptr, printmode);
	if(readmem((char *) &cb_st, (int)cb_ptr, sizeof(CB)) !=
	    sizeof(CB)) {
	    	printf("could not read connection block at 0x%x\n",cb_ptr);
		return;
	}
	cb_ptr = (CB *)cb_st.flink;
    } while (cb_ptr != (CB *)cbq_ptr);
}

/*
 *
 *   Name:	printscs_cb	- Print contents of a connection block.
 *
 *   Abstract:	Disect and display a connection block. 
 *
 *   Inputs:    cb_ptr	 	- Pointer to a connection block. 
 *		printmode	- Controlls output level.
 *
 *   Return	NONE
 *   Values:
 */
int 
printscs_cb(cb_ptr, printmode)
    CB		*cb_ptr;
    int		printmode;

{
	CB cb_st;
	int offset;

        if (cb_ptr == 0) {
	    printf("\tNULL connection block pointer.\n");
	    return;
        }
	printf("\t\t\t------------- CONNECTION BLOCK ------------------\n");
	if(readmem((char *) &cb_st, (int)cb_ptr, sizeof(CB)) !=
	    sizeof(CB)) {
		printf("could not read connection block at 0x%x.\n",cb_ptr);
		return;
	}

    if (printmode & SCS_PRINTFULL) {
	printf("\t\t\tConnection block address is 0x%x\n",cb_ptr);
	printf("\t\t\tflink: 0x%x, blink: 0x%x\n",cb_st.flink, cb_st.blink);
	printf("\t\t\tsize: %d, type: %d\n",cb_st.size, cb_st.type);
	printf("\t\t\tSCS waiting CB queue: fiink 0x%x, blink 0x%x\n",
		cb_st.scs_cb.flink, cb_st.scs_cb.blink);
	printf("\t\t\tSYSAP control 0x%x ",cb_st.control);
	if (cb_st.control != 0) {
		printf(" \(");
		praddr(cb_st.control);
		printf("\)");
	}
	printf("\n");
	printf("\t\t\tMessage event 0x%x ",cb_st.msg_event);
	if (cb_st.msg_event != 0) {
		printf(" \(");
		praddr(cb_st.msg_event);
		printf("\)");
	}
	printf("\n");
	printf("\t\t\tDatagram event 0x%x ",cb_st.dg_event);
	if (cb_st.dg_event != 0) {
		printf(" \(");
		praddr(cb_st.dg_event);
		printf("\)");
	}
	printf("\n");
	printf("\t\t\tPB: 0x%x, PDT: 0x%x, PCCB: 0x%x\n", cb_st.pb, 
		cb_st.pdt, cb_st.pccb);
	printf("\t\t\tSYSAP auxiliary pointer: 0x%x\n",cb_st.aux);
	offset = (int)(&cb_st.forkb) - (int)(&cb_st.flink);
	printscs_kschedblk((char *) ((char *)cb_ptr + offset), printmode);
	printf("\t\t\tConnection rejection reason: 0x%x\n",
		cb_st.errlogopt.rreason);
	offset = (int)(&cb_st.cinfo) - (int)(&cb_st.flink);
	printscs_cib((char *) ((char *)cb_ptr + offset), printmode);
    }
}

/*
 *
 *   Name:	printscs_cib	- Print contents of a connection information 
 *				  block.
 *
 *   Abstract:	Disect and display a connection information block. 
 *
 *   Inputs:    cib_ptr	 	- Pointer to a connection information block. 
 *		printmode	- Controlls output level.
 *
 *   Return	NONE
 *   Values:
 */
int 
printscs_cib(cib_ptr, printmode)
    CIB		*cib_ptr;
    int		printmode;

{
	CIB cib_st;
	int offset;
	int i;
	u_char c;

        if (cib_ptr == 0) {
	    printf("\tNULL connection information block pointer.\n");
	    return;
        }
	printf("\t\t\t\t-------- CONNECTION INFORMATION BLOCK ------\n");
	if(readmem((char *) &cib_st, (int)cib_ptr, sizeof(CIB)) !=
	    sizeof(CIB)) {
		printf("could not read connection information block at 0x%x.\n",
			cib_ptr);
		return;
	}

    if (printmode & SCS_PRINTFULL) {
	printf("\t\t\t\tConnection information block address 0x%x\n",
		cib_ptr);
	printf("\t\t\t\tlconnid: index %d, seq_num %d\n",
		cib_st.lconnid.index, cib_st.lconnid.seq_num);
	printf("\t\t\t\trconnid: index %d, seq_num %d\n",
		cib_st.rconnid.index, cib_st.rconnid.seq_num);
	printf("\t\t\t\tcstate: %d, cbstate: %d\n",cib_st.cstate,
		cib_st.cbstate);
	printf("\t\t\t\tstatus: ");
		if (cib_st.status.cwait)
			printf("cwait ");
		if (cib_st.status.abort_fork)
			printf("abort_fork ");
		if (cib_st.status.disconnect)
			printf("disconnect ");
	printf("\n");
	printf("\t\t\t\tntransfers: %d, reason: %d\n",cib_st.ntransfers,
		cib_st.reason);
	printf("\t\t\t\trproc_name: ");
	for (i=0; i< NAME_SIZE; i++) {
		c = (u_char)cib_st.rproc_name[i];
		if (isprint(c))
			printf("%c",c);
	}
	printf("\n");
	printf("\t\t\t\tlproc_name: ");
	for (i=0; i< NAME_SIZE; i++) {
		c = (u_char)cib_st.lproc_name[i];
		if (isprint(c))
			printf("%c",c);
	}
	printf("\n");
	printf("\t\t\t\tlconn_data: ");
	for (i=0; i< NAME_SIZE; i++) {
		c = (u_char)cib_st.lconn_data[i];
		if (isprint(c))
			printf("%c",c);
	}
	printf("\n");
	printf("\t\t\t\trconn_data: ");
	for (i=0; i< NAME_SIZE; i++) {
		c = (u_char)cib_st.rconn_data[i];
		if (isprint(c))
			printf("%c",c);
	}
	printf("\n");
	printf("\t\t\t\treserved_credit: %d, snd_credit: %d\n",
		cib_st.reserved_credit, cib_st.snd_credit);
	printf("\t\t\t\tmin_snd_credit: %d, rec_credit: %d\n",
		cib_st.min_snd_credit, cib_st.rec_credit);
	printf("\t\t\t\tinit_rec_credit: %d, min_rec_credit: %d\n",
		cib_st.init_rec_credit, cib_st.min_rec_credit);
	printf("\t\t\t\tpend_rec_credit: %d, dg_credit: %d\n",
		cib_st.pend_rec_credit, cib_st.dg_credit);
	printf("\t\t\t\tdgs_snt: %d, dgs_rec: %d\n",
		cib_st.dgs_snt, cib_st.dgs_rec);
	printf("\t\t\t\tdgs_discard: %d, msgs_snt: %d\n",
		cib_st.dgs_discard, cib_st.msgs_snt);
	printf("\t\t\t\tmsgs_rec: %d, sdatas_snt: %d\n",
		cib_st.msgs_rec, cib_st.sdatas_snt);
	printf("\t\t\t\tbytes_snt: %d, rdatas_snt: %d\n",
		cib_st.bytes_snt, cib_st.rdatas_snt);
	printf("\t\t\t\tbytes_req: %d, bytes_mapped: %d\n",
		cib_st.bytes_req, cib_st.bytes_mapped);
    }
}

/*
 *
 *   Name:	printscs_pd	- Print contents of a port data block.
 *
 *   Abstract:	The "pd" field of the PIB data structure is a union of two
 *		possible types (gvp and msi).  For the case of CI the gvp
 *		field is interpreted.  For msi devices the msi structure is
 *		displayed.  For other hardware types (such as uq) this
 *		structure is not used and consequently is not interpreted.
 *
 *   Inputs:    pd_ptr	 	- Pointer to a port data block.
 *		printmode	- Controlls output level.
 *		hwtype		- Port hardware type.
 *
 *   Return	NONE
 *   Values:
 */
int 
printscs_pd(pd_ptr, printmode, hwtype)
    char	*pd_ptr;
    int		printmode;
    u_long	hwtype;

{
	switch (hwtype) {
		case HPT_SII: 	    printscs_msipib(pd_ptr, printmode); break;
		case HPT_CI780:
		case HPT_CI750:
		case HPT_CIBCI:
		case HPT_CIBCA_BA:
		case HPT_CIBCA_AA:
		case HPT_CIXCD:
		case HPT_HSC:
				    printscs_gvppib(pd_ptr, printmode); break;
		default: 	    printf("\t\t\tpd field not used.\n");
	}
}

/*
 *
 *   Name:	printscs_gvppib	- Print contents of a generic vaxport path
 *				  information block.
 *
 *   Abstract:	Disect and display a _gvppib. This structure is itself a 
 *		union of only 1 element (_cipib), so this routine reads in
 *		a struct of that type and displays the contents.
 *
 *   Inputs:    cipib_ptr	- Pointer to a CI path info block.
 *		printmode	- Controlls output level.
 *
 *   Return	NONE
 *   Values:
 */
int 
printscs_gvppib(cipib_ptr, printmode)
    CIPIB		*cipib_ptr;
    int		printmode;

{
	CIPIB cipib_st;
	int offset;

        if (cipib_ptr == 0) {
	    printf("\tNULL CI path information block pointer.\n");
	    return;
        }
	printf("\t\t\t\t------------ CIPIB BLOCK ----------\n");
	if(readmem((char *) &cipib_st, (int)cipib_ptr, sizeof(CIPIB)) !=
	    sizeof(CIPIB)) {
		printf("could not read CI path information block at 0x%x.\n",
			cipib_ptr);
		return;
	}

    if (printmode & SCS_PRINTFULL) {
	printf("\t\t\t\tCI path information block address 0x%x\n",
		cipib_ptr);
	printf("\t\t\t\trom_level: %d, fn_level: %d\n",
		cipib_st.ucode_rev.rom_level, cipib_st.ucode_rev.fn_level);
	printf("\t\t\t\tport_fcn: 0x%x, port_fcn_ext: 0x%x\n",
		cipib_st.port_fcn, cipib_st.port_fcn_ext);
	printf("\t\t\t\tport_fcn_ext2: 0x%x, reset_port: 0x%x\n",
		cipib_st.port_fcn_ext2, cipib_st.reset_port);
	printf("\t\t\t\trport_state: %d \(", cipib_st.rport_state);
		switch (cipib_st.rport_state) {
			case PS_UNINIT: printf("PS_UNINIT"); break;
			case PS_UNINIT_MAINT: printf("PS_UNINIT_MAINT"); break;
			case PS_DISAB: printf("PS_DISAB"); break;
			case PS_DISAB_MAINT: printf("PS_DISAB_MAINT"); break;
			case PS_ENAB: printf("PS_ENAB"); break;
			case PS_ENAB_MAINT: printf("PS_ENAB_MAINT"); break;
			default: printf("UNKNOWN");
		}
		printf("\)\n");
    }
}

/*
 *
 *   Name:	printscs_msipib	- Print contents of a MSI path
 *				  information block.
 *
 *   Abstract:	Disect and display a _msipib. This structure is itself a 
 *		union of only 1 element (_msirpi), so this routine reads in
 *		a struct of that type and displays the contents.
 *
 *   Inputs:    msirpi_ptr	- Pointer to a MSI path info block.
 *		printmode	- Controlls output level.
 *
 *   Return	NONE
 *   Values:
 */
int 
printscs_msipib(msirpi_ptr, printmode)
    MSIRPI	*msirpi_ptr;
    int		printmode;

{
	MSIRPI msirpi_st;
	int offset;

        if (msirpi_ptr == 0) {
	    printf("\tNULL MSI path information block pointer.\n");
	    return;
        }
	printf("\t\t\t\t------------ MSIRPI BLOCK ----------\n");
	if(readmem((char *) &msirpi_st, (int)msirpi_ptr, sizeof(MSIRPI)) !=
	    sizeof(MSIRPI)) {
		printf("could not read MSI path information block at 0x%x.\n",
			msirpi_ptr);
		return;
	}

    if (printmode & SCS_PRINTFULL) {
	printf("\t\t\t\tMSI path information block address 0x%x\n",
		msirpi_ptr);
	printf("\t\t\t\tst_level: %d, fn_level: %d\n",
		msirpi_st.ucode_rev.st_level, msirpi_st.ucode_rev.fn_level);
	printf("\t\t\t\tport_fcn: 0x%x, 0x%x\n",
		msirpi_st.port_fcn[0], msirpi_st.port_fcn[1]);
	printf("\t\t\t\treset_port: %d, port_state: %d \(",
		msirpi_st.sys_state.reset_port, msirpi_st.sys_state.port_state);
		switch (msirpi_st.sys_state.port_state) {
			case PS_UNINIT: printf("PS_UNINIT"); break;
			case PS_UNINIT_MAINT: printf("PS_UNINIT_MAINT"); break;
			case PS_DISAB: printf("PS_DISAB"); break;
			case PS_DISAB_MAINT: printf("PS_DISAB_MAINT"); break;
			case PS_ENAB: printf("PS_ENAB"); break;
			case PS_ENAB_MAINT: printf("PS_ENAB_MAINT"); break;
			default: printf("UNKNOWN");
		}
		printf("\)\n");
	printf("\t\t\t\tsys_state1: %d, sys_state2: %d \n",
		msirpi_st.sys_state.sys_state1, msirpi_st.sys_state.sys_state2);
	printf("\t\t\t\tport_fcn_ext.maxbodylen: %d\n",
		msirpi_st.port_fcn_ext.maxbodylen);
    }
}
