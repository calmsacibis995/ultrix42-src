#ifndef	lint 
static char *sccsid = "@(#)ports.c	4.2      (ULTRIX)        8/7/90";
#endif	lint

/************************************************************************
 *                                                                      *
 *                      Copyright (c) 1989,1990 by                           *
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
 *   Facility:	PCCB data structure formatter for the crash utility.
 *
 *   Abstract:	This module contains the routines used to display the
 *		contents of PCCB data structures.  Routines are also
 *		provided to traverse data structure lists.
 *
 *   Author:	Matthew Sacks December 4, 1989
 *
 *   History:
 *
 *   June 1990	Matthew Sacks
 *		Cleaned up code.  Added comments.
 *
 */

#include	"crash.h"
#include 	<sys/types.h>
#include 	<sys/buf.h>
#include 	<sys/devio.h>
#include 	<sys/param.h>
#include	<sys/time.h>
#include	<sys/kmalloc.h>
#include	<sys/ksched.h>
#include	<sys/errlog.h>
#include	<fs/ufs/fs.h>
#include	<io/scs/sca.h>
#include	<io/scs/scaparam.h>
#include	<io/ci/cippdsysap.h>
#include	<io/ci/cisysap.h>
#include	<io/msi/msisysap.h>
#include	<io/uba/uqsysap.h>
#include	<io/bi/bvpsysap.h>
#include	<io/gvp/gvpsysap.h>
#include	<io/sysap/sysap.h>
#include	<io/ci/cippdscs.h>
#include	<io/ci/ciscs.h>
#include	<io/msi/msiscs.h>
#include	<io/bi/bvpscs.h>
#include	<io/gvp/gvpscs.h>
#include	<io/uba/uqscs.h>
#include	<io/scs/scs.h>
#include	<io/gvp/gvp.h>
#include	<io/uba/uqppd.h>
#include	<io/uba/uqport.h>
#include	<io/ci/cippd.h>
#include	<io/ci/ciport.h>
#include	<io/ci/ciadapter.h> 
#include	<stdio.h>

char * token();
/* top level command interpreter for port displays */	
c_port(c)
	char *c;
{
	char *arg;
	int index;
	unsigned int addr;
	int	do_buffers, do_bvp, do_ci, do_msi, do_ssp, do_one, do_brief;
	PCCB	*P_addr;

	do_one = do_buffers = do_brief = 0;
	do_ssp = do_msi = do_bvp = do_ci = 0;	
	while ((arg = token()) != NULL) {
		if (strcmp(arg, "-ssp") == 0)
			{do_ssp = -1; continue;}
		if (strcmp(arg, "-msi") == 0) 
			{do_msi = -1; continue;}
		if (strcmp(arg, "-bvp") == 0) 
			{do_bvp = 1; continue;}
		if (strcmp(arg, "-ci") == 0) 
			{do_ci =  -1; continue;}
		if (strcmp(arg, "-buffers") == 0) 
			{do_buffers = -1; continue;}
		if (strcmp(arg, "-brief") == 0) 
			{do_brief = -1; continue;}
		if (strcmp(arg, "-A") == 0) {
			if ((arg = token ()) == NULL) {
				printf
					("-A option requires an address\n");
				break;
			}
			P_addr = (PCCB *)scan_vaddr(arg);
			do_one = -1;
		}	
	}
	/* the following line says that the default
	   action is to do the SSP port */
	if (! (do_ssp | do_msi | do_bvp | do_ci))
		do_ssp = -1;
	print_ports(do_brief,
		    do_buffers,
		    do_bvp,
		    do_ci,
		    do_msi,
		    do_one,
		    do_ssp,
		    P_addr);
}

/*
 *
 *   Name:	print_ports 	- Traverse and display data structs
 *
 *   Abstract:	This routine will dump the system (local) port structures.
 *	        traverse the data structures printing results.
 *
 *   Return
 *   Values:	NONE
 */

int 
print_ports (do_brief, do_buffers, do_bvp, do_ci, do_msi, do_one,
do_ssp, P_addr)
int	do_brief,	/* on means do brief display */
	do_buffers,	/* on means include application buffers */
	do_bvp,		/* on means do the bvp port */
	do_ci,		/* on means do the ci port */
	do_msi,		/* on means do the msi (=DSSI) port */
	do_one,		/* on means do the port at address "P_addr" */
	do_ssp;		/* on means do the ssp port */
PCCB	*P_addr;

{
pccbq	*pccbs,*port_list;
PCCB	*the_port,*pccb_flink;
PCCB	current_port;
pccbq	list_header;


port_list = (pccbq *) Ports.s_value;

if (port_list == 0) {
	printf ("scs local port database is null\n");
	return (0);
	}

if (readmem((char *)&list_header,(int)port_list,sizeof(pccbq))
	!= sizeof(pccbq))
	{
	printf ("could not read local port database header\n");
	return (0);
	}

pccbs = &list_header;
pccbs = pccbs->flink;

if (pccbs == port_list) {
	printf ("scs local port database has no entries\n");
	return (0);
	}

while (pccbs != port_list) {

pccb_flink = Pos_to_pccb(pccbs,flink);
pccbs = (pccbq *) pccb_flink;

if ((readmem(&current_port, pccbs, sizeof(PCCB)))
	 != sizeof(PCCB))
	{
	printf ("could not read the local port database\n");
	return (0);
	}
	
	switch (current_port.lpinfo.type.hwtype) {
		case HPT_UQSSP:
		case HPT_KFQSA:
		case HPT_KFXSA:
			if (((do_one && 
				((PCCB *)pccbs == P_addr))) ||
				  		((!do_one) && (do_ssp)))
				print_ssp (&current_port, pccbs,
						do_brief, do_buffers);
				break;

		case HPT_BVPSSP:
			if (((do_one && 
				((PCCB *)pccbs == P_addr))) ||
				  		((!do_one) && (do_bvp)))
				if (do_bvp) print_bvp (&current_port, pccbs);
				break;

		case HPT_CI780:
		case HPT_CI750:
		case HPT_SHAC:
		case HPT_CIBCI:
		case HPT_CIXCD:
		case HPT_CIBCA_BA:
		case HPT_CIBCA_AA:
			if (((do_one && 
				((PCCB *)pccbs == P_addr))) ||
				  		((!do_one) && (do_ci)))
				if (do_ci)  print_ci (&current_port, pccbs);
				break;

		case HPT_SII:
		case HPT_RF30:
		case HPT_RF31:
		case HPT_RF71:
		case HPT_TF70:
		case HPT_TF85:
			if (((do_one && 
				((PCCB *)pccbs == P_addr))) ||
				  		((!do_one) && (do_msi)))
				if (do_msi) print_msi (&current_port, pccbs);
				break;

		default:	printf ("unknown hardware port type: %d\n",
					 current_port.lpinfo.type.hwtype);
		} /* end switch */

	pccbs = current_port.flink;

	} /* end while */
} /* print_ports */


/*
 *
 *   Name:	print_ssp 	-  display an ssp PCCB structure.
 *
 *   Abstract:	display an ssp ring structure.  with the -buffers option
 *		it will include the application buffers.  It knows the
 *		difference between an XMI, BI, and Q-BUS ssp port and
 *		varies the display accordingly.
 *
 *		port is a pointer to a local copy of the PCCB.
 *		kport is the actual kernal address of it.
 *		do_brief is non-zero if only a brief display should
 *		be done.
 *		do_buffers is non-zero if the aplication buffer should
 *		be included with each corresponding ring descriptor.
 *
 *   Return
 *   Values:	NONE
 */

print_ssp (port, kport, do_brief, do_buffers)
PCCB	*port, *kport;
int	do_brief,
	do_buffers;
{
UQCA	*comm_area_ptr = (UQCA *)&((port->pd.uq.uq)->uqca);
UQCA	comm_area;	/* user mode copy of a (kernel) port comm area */
short	ring_index;	/* a helper variable */
u_char	*ring_adr,*buff_adr,
	*buff_end,*buff_index; /* helper variables */
UQBUF	the_buffer;
int	item_count;

if (readmem(&comm_area,comm_area_ptr,sizeof(UQCA)) != sizeof(UQCA)) {
	printf ("could not read port communications area\n");
	return (0);
	}

printf ("PORT COMMAND CONTROL BLOCK - Storage Systems Port\n");
printf ("-------------------------------------------------\n");
printf ("SSP Port Command Control Block Address %17X\n", kport);
printf ("flink %50X\n", port->flink);
printf ("blink %50X\n", port->blink);
printf ("SSP Communications Area Address %24X\n",
							port->pd.uq.uq);
printf ("SSP Communications Area Address (Mapped Mode) %10X\n",
							port->pd.uq.uqptr);
   /* The following routine will print the name of the controller */
(void) print_uq_cnt_type (port->lpinfo.pd.uq.uq_type);
printf ("Controller Number %38d (decimal)\n", port->pd.uq.uq_ctlr);
printf ("Class Driver Connections Count %25d (decimal)\n", port->pd.uq.ncon);
printf ("IP register address %36X\n",port->pd.uq.uqregptrs.uqip);
printf ("SA register address %36X\n",port->pd.uq.uqregptrs.uqsa);
printf ("UQ port credits %40X\n", port->lpinfo.pd.uq.uq_credits[0]);
printf ("UQ port state %42X\n", port->lpinfo.pd.uq.uq_state);
printf ("UQ port flag word %38X\n", port->lpinfo.pd.uq.uq_flags);

if (port->lpinfo.pd.uq.uq_type == KDM_TYPE) {
	printf ("\nXMI Specifics\n");
	printf ("-------------\n");
	printf ("\tPDR register address %27X\n",
					port->pd.uq.uqregptrs.uqpd);
	printf ("\tScratch pad address %28.8X\n", comm_area.ca_scp_add);
	printf ("\tScratch pad size %31.4X\n", comm_area.ca_scp_size);
	printf ("\tPage Frame Number Mask %25.4X\n",comm_area.ca_xmi.pfn);
	printf 
	  ("\tHexaword Write Indicator (HW) %18.3s\n",
		(((comm_area.ca_xmi.flags) & 1) ? "ON" : "OFF"));
	printf 
	  ("\tTransient Error Reporting (ET) %17.3s\n",
		(((comm_area.ca_xmi.flags) & 2) ? "ON" : "OFF"));
	printf 
	  ("\tPage Size Indicator (PSI) %22.1X\n",comm_area.ca_xmi.psi);
	}
	else
	printf 
	  ("Bus-purge Buffered Data Path %27.2X\n",comm_area.ca_bdp);

if (port->lpinfo.pd.uq.uq_type == BDA_TYPE) printf
	  ("SAW register (VAXBI only) address %22X\n",
					port->pd.uq.uqregptrs.uqsaw);
printf ("\n");

if (! do_brief) 
{
printf ("SSP Response Ring\n");
printf ("-----------------\n");
if (! do_buffers) printf ("\tDescriptor Address		\tContents\n");
for (ring_index=0, ring_adr=(u_char *) comm_area_ptr->ca_rspdsc;
				ring_index<NRSP;
				ring_adr += sizeof(u_long), ++ring_index)
{
	if (do_buffers) 
		printf ("\n\n\tDescriptor Address		\tContents\n");
	if (ring_index == port->pd.uq.uq_lastrsp)
		printf ("\n%68s\n", "Most Recent Response");
	printf ("\t%8.8X\t\t\t\t%8.8X  %s\n",
	ring_adr, comm_area.ca_rspdsc[ring_index],
	comm_area.ca_rspdsc[ring_index] & (1 << 31) ?
	"Controller Owned" : "Host Owned");

	/* now print the application buffer */
	if (do_buffers)
	{
	printf ("\tApplication Buffer\n");
	buff_adr = (u_char *) (port->pd.uq.rspbtab[ring_index]);
	if (readmem(&the_buffer, buff_adr, sizeof(UQBUF)) != sizeof(UQBUF))
	printf ("\t\tcould not read the application buffer\n");
	else
	{
	buff_adr = (u_char *)&the_buffer;
	printf ("\tflink\t\t%8.8X\n", ((UQH *)buff_adr)->flink);
	printf ("\tblink\t\t%8.8X\n", ((UQH *)buff_adr)->blink);
	printf ("\t\t\t%8.8X\n", ((UQH *)buff_adr)->ua);
	buff_adr += sizeof(UQH);
	printf ("\tScs Header\t%8.4X\n",((SCSH *)buff_adr)->mtype);
	printf ("\t\t\t%8.4X\n", ((SCSH *)buff_adr)->credit);
	printf ("\t\t\t%8.8X\n", ((SCSH *)buff_adr)->rconnid);
	printf ("\t\t\t%8.8X\n", ((SCSH *)buff_adr)->sconnid);
	buff_adr += sizeof(SCSH);
	printf ("\tBuffer Contents\n");
	for (buff_end = (u_char *) (buff_adr + sizeof(APP_BUF)), item_count=1;
				buff_adr < buff_end;
				item_count++, buff_adr += sizeof(u_long))
		if (item_count % 4) printf ("\t%8.8X", *(u_long *)buff_adr);
		else printf ("\t\t%8.8X\n", *(u_long *)buff_adr);
	} /* end else */
	} /* end if */
} /* end for */




printf ("\n\nSSP Command Ring\n");
printf ("-----------------\n");
if (! do_buffers) printf ("\tDescriptor Address		\tContents\n");
for (ring_index=0, ring_adr=(u_char *)comm_area_ptr->ca_cmddsc;
				ring_index<NCMD;
				ring_adr += sizeof(u_long), ++ring_index)
{
	if (do_buffers) printf
			("\n\n\tDescriptor Address		\tContents\n");
	if (ring_index == port->pd.uq.uq_lastcmd)
		printf ("\n%67s\n", "Most Recent Command");
	printf ("\t%8.8X\t\t\t\t%8.8X  %s\n",
	ring_adr, comm_area.ca_cmddsc[ring_index],
	comm_area.ca_cmddsc[ring_index] & (1 << 31) ?
		"Controller Owned" : "Host Owned");

if (do_buffers) {
	/* now print the buffer */
	buff_adr = (u_char *) (port->pd.uq.cmdbtab[ring_index]);
	printf ("\tApplication Buffer\n");
	if (readmem(&the_buffer, buff_adr, sizeof(UQBUF)) != sizeof(UQBUF))
	printf ("\t\tcould not read the application buffer\n");
	else
	{
	buff_adr = (u_char *)&the_buffer;
	printf ("\tflink\t\t%8.8X\n", ((UQH *)buff_adr)->blink);
	printf ("\tblink\t\t%8.8X\n", ((UQH *)buff_adr)->blink);
	printf ("\t\t\t%8.8X\n", ((UQH *)buff_adr)->ua);
	buff_adr += sizeof(UQH);
	printf ("\tScs Header\t%8.4hX\n",(u_short *)((SCSH *)buff_adr)->mtype);
	printf ("\t\t\t%8.4hX\n", (u_short *)((SCSH *)buff_adr)->credit);
	printf ("\t\t\t%8.8lX\n", ((SCSH *)buff_adr)->rconnid);
	printf ("\t\t\t%8.8lX\n", ((SCSH *)buff_adr)->sconnid);
	buff_adr += sizeof(SCSH);
	printf ("\tBuffer Contents\n");
	for (buff_end = (u_char *) (buff_adr + sizeof(APP_BUF)), item_count=1;
				buff_adr < buff_end;
				item_count++, buff_adr += sizeof(u_long))
	if (item_count % 4) printf ("\t%8.8X", *(u_long *)buff_adr);
		else printf ("\t\t%8.8X\n", *(u_long *)buff_adr);
	printf ("\n");
	} /* end else */
	} /* end if */
} /* end for */
} /* end if */
printf ("\n");

printf ("\nPort Dump\n");
printf ("---------\n");
for
(buff_adr = (u_char *) port, item_count = 0;
	buff_adr < ((u_char *)port+sizeof(PCCB));
	buff_adr += sizeof(u_long))
	{
	printf ("%8.8X ", (u_long) *((u_long *)buff_adr));
	if (((++item_count) % 8) == 0)
		{
		item_count = 0;
		printf ("\n");
		}
	}

if (item_count)
	printf ("\n\n");
else 	printf ("\n");

} /* print_ssp */


/*
 *
 *   Name:	print_bvp 	-  display an bvp PCCB structure.
 *
 *   Abstract:	
 *		port is a pointer to a local copy of a bvp PCCB.
 *		kport is the actual kernal address of it.
 *
 *   Return
 *   Values:	NONE
 */

print_bvp (port, kport)
PCCB	*port, *kport;
{
u_char	*buff_adr;
int	item_count;

printf ("PORT COMMAND CONTROL BLOCK - Generic Vax Port\n");
printf ("---------------------------------------------\n");
printf ("GVP Port Command Control Block Address %17X\n", kport);
printf ("flink %50X\n", port->flink);
printf ("blink %50X\n", port->blink);

printf ("\nPort Dump\n");
printf ("---------\n");
for
(buff_adr = (u_char *) port, item_count = 0;
	buff_adr < ((u_char *)port+sizeof(PCCB));
	buff_adr += sizeof(u_long))
	{
	printf ("%8.8X ", (u_long) *((u_long *)buff_adr));
	if (((++item_count) % 8) == 0)
		{
		item_count = 0;
		printf ("\n");
		}
	}

if (item_count)
	printf ("\n\n");
else 	printf ("\n");

} /* print_bvp */



/*
 *
 *   Name:	print_msi 	-  display an msi PCCB structure.
 *
 *   Abstract:	
 *		port is a pointer to a local copy of a msi PCCB.
 *		kport is the actual kernal address of it.
 *
 *   Return
 *   Values:	NONE
 */

print_msi (port, kport)
PCCB	*port, *kport;
{
u_char	*buff_adr;
int	item_count;

printf ("PORT COMMAND CONTROL BLOCK - Mass Storage Interconnect\n");
printf ("------------------------------------------------------\n");
printf ("MSI Port Command Control Block Address %17X\n", kport);
printf ("flink %50X\n", port->flink);
printf ("blink %50X\n", port->blink);

printf ("\nMSI High Priority Command Queue\n");
printf ("\tflink %42X\n", port->pd.msi.comqh.flink);
printf ("\tblink %42X\n", port->pd.msi.comqh.blink);
printf ("MSI Low Priority Command Queue\n");
printf ("\tflink %42X\n", port->pd.msi.comql.flink);
printf ("\tblink %42X\n", port->pd.msi.comql.blink);
printf ("MSI Free Sequential Message Queue\n");
printf ("\tflink %42X\n", port->pd.msi.mfreeq.flink);
printf ("\tblink %42X\n", port->pd.msi.mfreeq.blink);
printf ("MSI Free Datagram Queue\n");
printf ("\tflink %42X\n", port->pd.msi.dfreeq.flink);
printf ("\tblink %42X\n", port->pd.msi.dfreeq.blink);

printf ("\nApplication Datagram Size %30X\n",
				port->lpinfo.pd.msi.dg_size);
printf ("Application Sequential Message Size %20X\n",
				port->lpinfo.pd.msi.msg_size);
printf ("Port Driver And Port To Port Datagram Overhead %9X\n",
				port->lpinfo.pd.msi.pd_ovhd);
printf ("Port To Port Datagram Overhead %25X\n",
				port->lpinfo.pd.msi.pd_ovhd);

printf ("\nDSSI REGISTER ADDRESSES\n");
printf ("------------------------\n");
printf ("Control Status CSR %37.X\n",
				port->pd.msi.siiregptrs.msicsr);
printf ("DSSI Control Register %34.X\n",
				port->pd.msi.siiregptrs.msidscr);
printf ("DSSI Status Register %35.4X\n",
				port->pd.msi.siiregptrs.msidssr);
printf ("ID Register %44.4X\n",
				port->pd.msi.siiregptrs.msiidr);
printf ("Timeout Register %39.4X\n",
				port->pd.msi.siiregptrs.msitr);
printf ("Target List Pointer Register %27X\n",
				port->pd.msi.siiregptrs.msitlp);
printf ("Initiator List Pointer Register %24X\n",
				port->pd.msi.siiregptrs.msiilp);
printf ("Diagnostic Control Register %28X\n",
				port->pd.msi.siiregptrs.msidcr);
printf ("SII Command Register %35.4X\n",
				port->pd.msi.siiregptrs.msicomm);
printf ("Data Transfer Register %33X\n",
				port->pd.msi.siiregptrs.msidstat);
printf ("Main Control Diagnostic Register  %22X\n",
				port->pd.msi.siiregptrs.msiisr3);


printf ("\nMSI Local Port Flags\n");
printf ("--------------------\n");
printf ("\tFirst Time Initialization %22s\n",
		port->pd.msi.lpstatus.init ? "ON" : "OFF");
printf ("\tPort Active %36s\n",
		port->pd.msi.lpstatus.active ? "ON" : "OFF");
printf ("\tRetry Delay Timer Active %23s\n",
		port->pd.msi.lpstatus.timer ? "ON" : "OFF");
printf ("\tTransmit Fork Process Scheduled %16s\n",
		port->pd.msi.lpstatus.xfork ? "ON" : "OFF");
printf ("\tReceive Fork Process Scheduled %17s\n",
		port->pd.msi.lpstatus.rfork ? "ON" : "OFF");

/* do hex dump of whole port structure */
printf ("\nPort Dump\n");
printf ("---------\n");
for
(buff_adr = (u_char *) port, item_count = 0;
	buff_adr < ((u_char *)port+sizeof(PCCB));
	buff_adr += sizeof(u_long))
	{
	printf ("%8.8X ", (u_long) *((u_long *)buff_adr));
	if (((++item_count) % 8) == 0)
		{
		item_count = 0;
		printf ("\n");
		}
	}

if (item_count)
	printf ("\n\n");
else 	printf ("\n");

printf ("\n\n");
} /* print_msi */

/*
 *
 *   Name:	print_ci 	-  display an ci PCCB structure.
 *
 *   Abstract:	
 *		port is a pointer to a local copy of a ci PCCB.
 *		kport is the actual kernal address of it.
 *
 *   Return
 *   Values:	NONE
 */

print_ci (port, kport)
PCCB	*port, *kport;
{
u_char	*buff_adr;
int	item_count;

printf ("PORT COMMAND CONTROL BLOCK - Computer Interconnect\n");
printf ("--------------------------------------------------\n");
printf ("CI Port Command Control Block Address %18X\n", kport);
printf ("flink %50X\n", port->flink);
printf ("blink %50X\n", port->blink);
printf ("\n");

printf ("\nPort Dump\n");
printf ("---------\n");
for
(buff_adr = (u_char *) port, item_count = 0;
	buff_adr < ((u_char *)port+sizeof(PCCB));
	buff_adr += sizeof(u_long))
	{
	printf ("%8.8X ", (u_long) *((u_long *)buff_adr));
	if (((++item_count) % 8) == 0)
		{
		item_count = 0;
		printf ("\n");
		}
	}

if (item_count)
	printf ("\n\n");
else 	printf ("\n");
} /* print_ci */


/*
 *
 *   Name:	print_uq_cnt_type
 *
 *   Abstract:	use the type field to print the ascii string of the name
 *		of an SSP controller.  These values are defined in 
 *		appendix A-3 of the SSP manual.  This is a subroutine of
 *		print_ssp.
 *
 *
 *   Return
 *   Values:	NONE
 */

print_uq_cnt_type (type)
int	type;
{
printf	("Controller Type");
switch (type)	{
	case UDA_TYPE:	printf ("%41.8s\n", "uda50");
			break;
	case RC25_TYPE:	printf ("%41.8s\n", "klesi");
			break;
	case RUX_TYPE:	printf ("%41.8s\n", "rux50");
			break;
	case MAYA_TYPE: printf ("%41.11s\n", "tqk50/tuk50");
			break;
	case TU81_TYPE:	printf 
			("%41.28s\n", "(integrated controller) tu81");
			break;
	case UDA50A_TYPE: printf ("%41.8s\n", "uda50a");
			break;
	case RQDX_TYPE:	printf ("%41.11s\n", "rqdx1/rqdx2");
			break;
	case KDA50A_TYPE: printf ("%41.8s\n", "kda50-q");
			break;
	case TK70_TYPE: printf ("%41.8s\n", "tqk70");
			break;
	case RV20_TYPE:	printf ("%41.8s\n", "rv20");
			break;
	case KRQ50_TYPE:printf ("%41.8s\n", "krq50");
			break;
	case BDA_TYPE:	printf ("%41.8s\n", "kdb50");
			break;
	case RQDX3_TYPE:printf ("%41.8s\n", "rqdx3");
			break;
	case KFQSA_D_TYPE: printf ("%41.10s\n", "kfqsa_disk");
			break;
	case KFQSA_T_TYPE: printf ("%41.10s\n", "kfqsa_tape");
			break;
	case KFQSA_DT_TYPE: printf ("%41.19s\n", "kfqsa_disk_and_tape");
			break;
	case KFQSA_OTHER: printf ("%41.8s\n", "kfqsa");
			break;
	case KRU50_TYPE:printf ("%41.8s\n", "kru50");
			break;
	case KDM_TYPE:	printf ("%41.8s\n", "kdm70");
			break;
	case TQK7L_TYPE:printf ("%41.8s\n", "tqk7l");
			break;
	case TM32_TYPE: printf ("%41.8s\n", "tm32\n");
			break;
	default:	
			printf ("%37.12s\n", "undetermined\n");
	}
} /* print_cnt_type */
