/*
 * netload.c
 */
#ifndef lint
static char *sccsid = "@(#)netload.c	4.2	(ULTRIX)	10/9/90";
#endif lint
/************************************************************************
 *									*
 *			Copyright (c) 1986 by				*
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
 *   This software is  derived  from  software  received  from  the	*
 *   University    of   California,   Berkeley,   and   from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is  subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/
#include "../h/param.h"
#include "mop.h"

#ifdef vax
#include "vax/vmb.h"
#endif vax

/*
 * Maintenance History
 *
 * 09-Oct-90 J. Szczypek
 *	Added TURBOchannel ROM support.  Code has been added to build
 *	a complete ethernet packet (header added).
 *
 * 10-Nov-89 T.N. Cherng
 *	Get the netboot device from the prom_getenv("boot").
 *
 * 8-Feb-88 tresvik
 *	swap the definition of TERTIARY and SECONDARY so that Mop can
 *	be brought into spec.  Now TERTIARY = vmunix and SECONDARY =
 *	client specific paramater file.  Associated registration changes
 *	made to /etc/dms and /etc/ris in conjuction with this change.
 *	This turned out to more than anticipated.  A relatively minor
 * 	change to the mop_dlload code causes a significant restructuring 
 *	of this code.  It, in fact, represents the removal a fair amount
 *	of code which is no longer required.  Now that mop_dlload does the
 *	right things, it is easier to communicate with.
 */

struct netblk *netblk_ptr;

#ifdef vax
extern	struct	vmb_info *vmbinfo;
#endif vax

#ifdef mips
#define stop _prom_restart
#define printf _prom_printf
#define getenv prom_getenv
#define open _prom_open
#define read _prom_read
#define write _prom_write
#define close _prom_close

int	io;	/* I/O channel for mips */
extern int rex_base;
extern int rex_magicid;
#define LEN 2

#endif mips

#ifdef vax
/*
 * Allocate and Initialize param_buf as follows
 *
 *	param_buf.pad = 1;
 *	param_buf.prot = 0x160;
 *
 * Start with a network broadcast address 
 *	0x0000010000ab 
 * 
 *	param_buf.dest[0] = 0xab; 
 *	param_buf.dest[1] = 0x00; 
 *	param_buf.dest[2] = 0x00; 
 *	param_buf.dest[3] = 0x01; 
 *	param_buf.dest[4] = 0x00; 
 *	param_buf.dest[5] = 0x00;
 */
struct {
	u_short	pad;
	u_short	prot;
	u_char	dest[6];
} param_buf = {1, 0x160, 0xab, 0, 0, 1, 0, 0};

#endif vax

#ifdef mips

/*
 * Mop routines and data.
 */

char mop_destination[6];
char broadcast[6]={0xff,0xff,0xff,0xff,0xff,0xff};
char mopdl_multicast[6]={0xab, 0,0,1,0,0};
#endif mips

#define BAD_LOAD 1

int	DEBUG=0;

#ifdef mips
main (argc,argv,envp,vector)
int argc;
char **argv,**envp, **vector;
#endif mips
#ifdef vax
main ()
#endif vax
{
	int (*start)();
	int i;
	int k;
	char *j;
	extern char *version;

#ifdef mips
	int	console_magic;

	console_magic = (int)envp;
	if (console_magic == 0x30464354) {
		rex_base = (int)vector;
		rex_magicid = console_magic;
	}
#endif mips

	printf("\nUltrixload - %s\n\n", version);
/*	printf("\nconsole id = %x\n",(int)envp); */ 
/*	printf("\nrex_base = %x\n",(int)vector); */
#ifdef vax
	netblk_ptr = (struct netblk *)&vmbinfo->netblk;
#endif vax
#ifdef mips
	netblk_ptr = (struct netblk *)NETBLK_LDADDR;
#endif mips

	/*
	 * Clear out the netblk in case there is something there
	 * and we don't get a successful load.
	 */
	j = (char *)netblk_ptr;
	for (i = 0; i < sizeof (struct netblk); i++) 
		j[i] = (char) 0;

	if (DEBUG) {
		printf("DEBUG is enabled.\n");
		printf("`*' means that 32K bytes have been loaded\n");
		printf("`R' means that a read error occurred\n");
		printf("`W' means that a write error occurred\n");
		printf("`S' means that the packet rcvd was not the one asked for\n\n");
		printf("All errors are retried\n\n");
	}
#ifdef mips
	if (rex_base) {
		bcopy(mopdl_multicast,mop_destination,6);
		if (rex_bootinit()) {
			printf("bootinit of network driver failed\n");
		        rex_rex('h');
		}
	}
	else {
		io = open(getenv("boot"), 2);
		if (io < 1) {
			printf("Cannot open the boot device.");
		        stop();
		}
	}
#endif mips
#ifdef vax
	if (!(drvinit() & 1)) {		/* re-init the VMB boot driver */
		printf("driver initialization failed\n");
		        stop();
	}
#endif vax
	for (i = 0; i < 2000000; i++);	/* Give the host a breather */
	i = upload(PGMTYP_SECONDARY, netblk_ptr, sizeof (struct netblk));
	if (i == BAD_LOAD) {
		printf("Network parameter file load failed.\n");
		printf("Continuing without network information.\n");
	} else
		printf ("Host server is '%s'\n", netblk_ptr->srvname);
	printf("Loading operating system image ...\n");
	for (i = 0; i < 2000000; i++); 	/* Give the host a breather */
	start = (int(*)()) upload(PGMTYP_TERTIARY, 0, RCV_BUF_SZ);
	printf("\n");
	if ((int)start == BAD_LOAD) {
		printf("Unrecoverable network failure\n");
#ifdef mips
		if(rex_base)
		        rex_rex('h');
		else
#endif mips
		        stop();
	}
#ifdef vax
	disconnect();		/* shutdown the link */
	(*start)(vmbinfo);
#endif vax
#ifdef mips
	if (!rex_base)
		close(io);
	(*start)(argc,argv,envp,vector);
	if (rex_base)
   	        rex_rex('h');
	else
#endif mips
	        stop();
}

upload (prog, addr, bufsz)
int	prog, addr, bufsz;
{
	union mop_packets mop_output;
	union mop_packets mop_input;
	union mop_packets *mop_i = &mop_input;
	union mop_packets *mop_o = &mop_output;
	int wrt_cnt, wrt_retry=5, rd_retry=5, seq_errs=0;
	int j=0, ldnum=1;
	int i;
	
	/*
	 * The following setup of ...code is needed to get things 
	 * looping properly below.
	 */
	mop_i->memload.code = NETLOAD_REQUEST; 
	for (;;) {
		int tmp;

		switch (mop_i->memload.code) {
		/*
		 * This is local variable use to kick start the network
		 * boot sequence.  It initiates program requests by
		 * falling out to write after creating the desired
		 * program request packet.
		 */
		case NETLOAD_REQUEST:
			mop_o->req_pgm.code = REQ_PROG_CODE;
			mop_o->req_pgm.devtype = NET_QNA;
			mop_o->req_pgm.mopver = MOP_VERSION;
			mop_o->req_pgm.pgmtyp = prog;
			mop_o->req_pgm.swid_form = -1; /* force sys load */
			mop_o->req_pgm.proc = SYSTEMPROC;
			mop_o->req_pgm.rbufsz_param = XTRA_BUFSZ;
			mop_o->req_pgm.sz_field = 2;
			tmp = sizeof mop_i->memload;
			bcopy((char *)&tmp, mop_o->req_pgm.rcvbufsz,
				sizeof mop_o->req_pgm.rcvbufsz);
			wrt_cnt = sizeof mop_o->req_pgm;
			break;
		/*
		 * In response to a request for a multisegment tertiary
		 * load from the network  (except for the last segment)
		 */
		case VOLASS_CODE:
		/*
		 * Send the same packet out again, which is the original
		 * request
		 */
			mop_i->memload.code = NETLOAD_REQUEST;
			continue;
		case MEMLD_CODE:
			/*
			 * The load number of the packet received must
			 * equal the number requested.  If it doesn't, the
			 * host is again asked for the same packet by
			 * load sequence number
			 */
			if (mop_i->memload.loadnum != 
				(u_char)((ldnum - 1) & 0xff)) {
				if (DEBUG) printf("S");
				if (++seq_errs == 5) {
					printf("\n\
Wrong packet number received from server - retries exceeded\n");
					goto error;
				}
				break;		/* fall out to ask again */
			}
			seq_errs=0;
			bcopy(mop_i->memload.loadaddr, (char *)&tmp,
				sizeof mop_i->memload.loadaddr);
			bcopy(mop_i->memload.data, addr + tmp,
				bufsz);
			/* 
			 * Display a progress indicator about every 
			 * 32k bytes
			 */
			if (j++ == 23){
				printf("*");
				j=0;
			}
			/*
			 * Now, prepare the next request packet before 
			 * falling out to send it.
			 */
			mop_o->req_memload.code = REQ_LOAD_CODE;
			mop_o->req_memload.loadnum = ldnum++;
			if (ldnum > 255) ldnum =0;
			mop_o->req_memload.error = 0;
			wrt_cnt = sizeof mop_o->req_memload;
			break;
		/* 
	 	 * In response to SECONDARY load request and
	 	 * the last packet on a multisegment tertiary load
	 	 */
		case MEMLD_XFR_CODE:
			/*
			 * For SECONDARY requests the rcvmsg contains
			 * real data and we don't care about any other
			 * part of the packet.  There is only one program
			 * segment allowed.  This is the netblk piece of
			 * vmbinfo.
			 *
			 * If this code is received to indicate the end of
			 * a multisegment load (our request for TERTIARY),
			 * then there is no data and all we care about
			 * is returning the transfer address of (presumably) 
			 * the vmunix that was just loaded.
			 */
			if (prog == PGMTYP_SECONDARY) {
				bcopy(mop_i->memload_xfr.loadaddr,
					(char *)&tmp, 
					sizeof mop_i->memload_xfr.loadaddr);
				bcopy(mop_i->memload_xfr.type.data,
				    addr + tmp, bufsz);
				return(0);
			} else {
				bcopy(mop_i->memload_xfr.type.xfr_addr,
				      (char *)&tmp,
				       sizeof mop_i->memload_xfr.type.xfr_addr);
				return (tmp);
			}
			break;
		/*
		 * Other codes are unexpected and considered fatal.
		 */
		default:
			printf("Unexpected MOP response code of %d\n",
				mop_i->memload.code);
			goto error;
		}
		/*
		 * At least one write and one read occurs now before
		 * looping back up to evaluate the packet received.  Of
		 * course, too many read or write errors will cause
		 * failures to occur.
		 */
		while (wrt_retry--) {
			if (write_net(wrt_cnt,&mop_o->req_pgm.code))
				break;
			else {
				if (DEBUG) printf("W");
				continue;
			}
		}
		if (wrt_retry <= 0 ) { 	/* if we ran out of retries */
			printf("write I/O error: retries exceeded\n");
			goto error;
		}
		wrt_retry=5;

		if ((read_net((sizeof mop_i->memload),&mop_i->memload.code)) == 0) {
			if (DEBUG) printf("R");
			if (rd_retry--)
				continue;	/* retry */
			printf("read I/O error: retries exceeded\n");
			goto error;
		}
		rd_retry=5;
	}
error:
	return (BAD_LOAD);
}

write_net(size, addr)
int	size;
char    *addr;

{
	int status;
#ifdef vax
	status = qio(PHYSMODE,IO$_WRITELBLK,&param_buf,size,addr);
	return(status & 1);
#endif vax
#ifdef mips
	if (rex_base) {
		unsigned char buffer[1600];

		/*
		 * We are being called with only the data portion of
		 * the mop packet.  We must stick the ethernet header
		 * on and set the destination, the protocol and the
		 * length field.
		 */
		bcopy(addr,(void *)&buffer[16],size);
		bcopy(mop_destination,(void *)buffer,6);
		buffer[12]= 0x60;
		buffer[13]= 0x01;
		buffer[14]= size &0xff;
		buffer[15]= (size>>8) && 0xff;
		status = rex_bootwrite(0,buffer,size+16);
	}
	else
		status = write(io, addr, size);
	return((status < 0) ? 0 : 1);
#endif mips
}

read_net(size, addr)
int	size;
char    *addr;

{
	int status;

#ifdef vax
	status = qio(PHYSMODE,IO$_READLBLK,&param_buf,size,addr);
#endif vax

#ifdef mips

	if (rex_base) {
	  while(1) {
		u_char buffer[1600];
		int len;

		while ((status = rex_bootread(0,buffer,1600)) == 0); 
		if (status <= 0)
			return(0);
		/*
		 * If we got a message, check that it is not broadcast,
		 * that it is a mop message, and if we have "bound",
		 * that is from right source.  If it's ok, copy it to
		 * the callers buffer and return the proper length.
		 */
		if ((buffer[12]==0x60 && buffer[13]==0x1) &&
			!eaddr_match((char*)buffer,broadcast)) {
			len = buffer[14] | (buffer[15]<<8);
			if (mop_destination[0] & 1){
				bcopy((void *)&buffer[6],mop_destination,6);
				bcopy((void *)&buffer[16],addr,len);
				return(1);
			      }
		  	else {
			    if (eaddr_match(mop_destination,(char*)&buffer[6])){
				bcopy((void *)&buffer[16],addr,len);
				return(1);
			      }
			} 
		      } /* END IF */
	      } /* END WHILE */
	}
	else {
		status = read(io, addr, size);
		return((status < 0) ? 0 : 1);
	}
#endif mips

}

#ifdef mips
/*
 * eaddr_match
 */
int
eaddr_match(ea1, ea2)
char *ea1, *ea2;
{
  int i;
  for (i=0;i<6;i++)
    if( *ea1++ != *ea2++)
      return(0);
  return(1);
}
#endif mips
