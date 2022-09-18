#ifndef lint
static char *sccsid = "@(#)cmd.c	4.4      (ULTRIX)        4/11/91";
#endif

/************************************************************************
 *									*
 *			Copyright (c) 1988 by				*
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

#include	"crash.h"
#include	<ctype.h>
#include	<sys/buf.h>
#include	<sys/ioctl.h>
#include	<sys/param.h>
#include	<sys/map.h>
#include	<sys/file.h>
#include	<sgtty.h>
#include	<signal.h>
#include	<sys/varargs.h>
#include	<sys/proc.h> 
#include	<machine/pte.h>
char *token();
extern struct tsw t[];
c_null(c) 
	char *c;
{
	printf("command '%s' not known, type ? for help\n", c);
	while(token() != NULL);
}

c_arp(c) 
	char *c;
{
	char *arg;
	int cnt;

	printf("     NAME  BUCK SLOT          IPADDR        ETHERADDR     MHOLD  TIMER  FLAGS\n");
	if((arg = token()) == NULL)
		for(cnt=0; cnt < tab[ARP_T].ents; cnt++)
			(void)prarp(cnt, 0);
	else if(*arg == '-')
		if((arg = token()) == NULL)
			for(cnt = 0; cnt < tab[ARP_T].ents; cnt++)
				(void)prarp(cnt, 1);
		else
			do
				(void)prarp(findarp(arg), 1);
	while((arg = token()) != NULL);
	else
		do 
			(void)prarp(findarp(arg), 0);
	while((arg = token()) != NULL);
}

#define	CURPROC	-1	/* Use the current or last process's data */

c_uarea(c)
	char *c;
{
	char *arg;
	int index;

	if((arg = token()) == NULL)
		pruarea(CURPROC);
	else do {
		if((index = gt_proc_slt(arg)) != -1)
			pruarea(index);
	} while((arg = token()) != NULL);
}

c_ofile(c)
	char *c;
{
	char *arg;
	int index;
   
	if((arg = token()) == NULL)
		profile(CURPROC);
	else do {
		if((index = gt_proc_slt(arg)) != -1)
			profile(index);
	} while((arg = token()) != NULL);
}

c_history(c)
	char *c;
{

	while(token());
	pr_history();
}

c_bufstats(c)
	char *c;
{
	while(token());
	pr_bufstats();
}

c_kmalloc(c)
	char *c;
{

	while(token());
	pr_kmalloc();
}

c_namei(c)
	char *c;
{
	char *arg;
	int index;

	if((arg = token()) == NULL)
		prnamei(-1);
	else do {
		if((index = atoi(arg)) != -1)
			prnamei(index);
	} while((arg = token()) != NULL);
}

c_dnlc(c)
	char *c;
{
	while(token());
	pr_dnlc();
}

c_ufile(c)
	char *c;
{
	char *arg;
	int index;

	if((arg = token()) == NULL)
		do_ufile(CURPROC);
	else do {
		if((index = gt_proc_slt(arg)) != -1)
			do_ufile(index);
	} while((arg = token()) != NULL);
}
	



c_block(c)
	char *c;
{
	char *arg;
	int index;

	if((arg = token()) != NULL) {
		do {
			index = get_gnode_slot(arg);
			if(index != -1)
				prblocks(index);
		} while((arg = token()) != NULL);
	}
}

c_ps(c)
	char *c;
{
	char *arg;
	int index, cnt, done=0;

	if((arg = token()) == NULL) {
		ps_hdr();
		for (cnt=0; cnt<tab[PROC_T].ents; cnt++)
			do_ps(cnt);
	}
	else {
		if (strcmp(arg,"-uid") == 0) {
			do {
				int uid;
				arg = token();
				sscanf(arg,"%d",&uid);
				for (cnt=0; cnt<tab[PROC_T].ents; cnt++) 
					if (proctab[cnt].p_uid == uid)
						do_ps(cnt);
			} while((arg = token()) != NULL);
			return;
		}
		do {
			if((index = gt_proc_slt(arg)) != -1) {
				if(done++ == 0)
					ps_hdr();
				do_ps(index);
			}
		}
		while((arg = token()) != NULL);
	}
}

c_pcblk(c)
	char *c;
{
	char *arg;
	int index;

	/* Uses the Process Table Slot */
	if((arg = token()) == NULL)
		prpcb(CURPROC);
	else do {
		if((index = gt_proc_slt(arg)) != -1)
			prpcb(index);
	} while((arg = token()) != NULL);
}

c_stack(c)
	char *c;
{
	char *arg;
	int index;

	if((arg = token()) == NULL)
		prstack(CURPROC);
	else do {
		if((index = gt_proc_slt(arg)) != -1)
			prstack(index);
	} while((arg = token()) != NULL);
}

c_trace(c)
	char *c;
{
	char *arg;
	int index, cpu, r;

	if((arg=token()) == NULL) {
		prtrace(0,CURPROC, 0);
		fflush(stdout);
		return;
	}
	if(arg[0] == '-') {
		if (strcmp(arg,"-sym") == 0) {
			r = -1;	/* wants vars */
		} else {
			r = 1;
		}
		arg = token();
	} else
		r = 0;
		
	if (strcmp(arg,"-all") == 0) {
		for (index=0; index<tab[PROC_T].ents; index++) {
			if(proctab[index].p_stat == SIDL)
				continue;
			printf("Trace - proc %d pid %d uid %d\n",
			       index, proctab[index].p_pid, 
			       proctab[index].p_uid);
			prtrace(r, index, 0); 
		}
		return;
	}


	if(arg[0] == '#') {
		if (arg[1] == '\0')
			arg = token();
		else
			arg++;

		cpu = atoi(arg);
		prtrace(r,CURPROC, cpu);
		return;
	}
		

	if(arg == NULL)
		prtrace(r,CURPROC, 0);
	else do {
		if((index = gt_proc_slt(arg)) != -1)
			prtrace(r, index, 0);
	} while((arg = token()) != NULL);
}

c_files(c)
	char *c;
{
	char *arg;
	int index, cnt, done=0;

	if((arg = token()) == NULL) {
		pr_filehdr();			
		for(cnt = 0; cnt < tab[FILE_T].ents; cnt++)
			prfile(cnt, 0);
	} else {
		do {
			index = get_file_slot(arg);
			if(index != -1) {
				if(done++ == 0)
					pr_filehdr();
				prfile(index, 1);
			}
		} while((arg = token()) != NULL);
	}
}

c_clntrpc(c)
	char *c;
{
	char *arg;
	int index, cnt, done=0;

	if((arg = token()) == NULL) {
		prclienthd();
		for(cnt = 0; cnt < MAXCLIENTS; cnt++)
			prclient(cnt, 0);
	} else {
		done=0;
		do {
			index = get_file_slot(arg);
			if(index != -1) {
				if(done++ == 0)
					prclienthd();
				prclient(index, 1);
			}
		} while((arg = token()) != NULL);
	}
}

c_rpcxprt(c)
	char *c;
{
	char *arg;
	int index;
	unsigned int addr;

	if ((arg= token()) == NULL) {
		printf("usage: svcxprt addr\n");
		return;
	}
	addr = scan_vaddr(arg);
	pr_svcxprt(addr);
}

c_svcreq(c)
	char *c;
{
	char *arg;
	int index;
	unsigned int addr;

	if ((arg= token()) == NULL) {
		printf("usage: svcreq addr\n");
		return;
	}
	addr = scan_vaddr(arg);
	pr_svcreq(addr);
}

c_udpdata(c)
	char *c;
{
	char *arg;
	int index;
	unsigned int addr;


	if ((arg= token()) == NULL) {
		printf("usage: udpdata addr\n");
		return;
	}
	addr = scan_vaddr(arg);
	pr_udpdata(addr);
}

c_socket(c)
	char *c;
{
	char *arg;
	int index, cnt, r, done=0;
	unsigned int addr;

	if((arg = token()) == NULL) {
		pr_sockhdr();			
		for(cnt = 0; cnt < tab[FILE_T].ents; cnt++)
			prsock(cnt, 0, 0);
			return;
	}
	if(arg[0] == '-') {
		r = 1;	/* wants details */
	} else
		r = 0;

	if((arg = token()) == NULL) {
		pr_sockhdr();
		for(cnt = 0; cnt < tab[FILE_T].ents; cnt++)
			prsock(cnt, 0, r);
		return;
	} else {
		do {
			index = get_file_slot(arg);
			if(index != -1) {
				if(done++ == 0)
					pr_sockhdr();
				prsock(index, 1, r);
			}
		} while((arg = token()) != NULL);
	}
}

c_rnode(c)
	char *c;
{
	char *arg;
	int index, cnt, done=0;
	unsigned int addr;

	if((arg = token()) == NULL) {
		printrnodehd();
		for(cnt=0; cnt < tab[GNODE_T].ents; cnt++)
			prvnode(cnt, 0);
	} else if (*arg == '-')
		if ((arg = token()) == NULL) {
			printrnodehd();
			for(cnt=0;cnt<tab[GNODE_T].ents;cnt++)
				prvnode(cnt, 1);
		} else {
			do {
				index = get_gnode_slot(arg);
				if(index != -1) {
					if(done++ == 0)
						printrnodehd();
					prvnode(index, 1);
				}
			} while((arg=token()) != NULL);
		}
	else {
		do {
			index = get_gnode_slot(arg);
			if(index != -1) {
				if(done++ == 0)
					(void)printrnodehd();
				prvnode(index, 0);
			} 
		} while((arg = token()) != NULL);
	}
}

c_mntinfo(c)
	char *c;
{
	char *arg;
	int index, done=0;
	unsigned int addr;

	if((arg = token()) == NULL)
		printf("Usage: mi <mounttab slot>\n");
	else {
		done = 0;
		do {
			index = get_mount_slot(arg);
			if(index != -1) {
				if (done++ == 0)
					(void)printmntinfohd();
				prmntinfo(index);
			}
		} while((arg = token()) != NULL);
	}
}

c_bufgp(c)
	char *c;
{
	char *arg;
	int index, done=0;
	unsigned int addr;


	if((arg = token()) == NULL) {
		printf("Usage: bufgp <gno slot>\n");
		return;
	}
	do {
		index = get_buf_slot(arg);
		if(index != -1) {
			if(done++ == 0)
				(void)printbufhd();
			do_bufgp(index);
		}
	} while((arg = token()) != NULL);
}

c_gnode(c)
	char *c;
{
	char *arg;
	int index, detail, cnt, done=0;
	unsigned int addr;

	
	detail = 1;
	if((arg = token()) == NULL) {
		(void)printgnodehd();
		/* print all valid gnodes */
		for(cnt=0; cnt < tab[GNODE_T].ents; cnt++)
			prgnode(cnt, 0, -1, detail);
		return;
	}
	if (strcmp(arg,"-lock") == 0) {
		detail = 2;
		arg = token();
	}
	if (strcmp(arg,"-all") == 0) {
		detail = 3;
		arg = token();
	}
	
	if(*arg == '-') {
		if (strcmp(arg,"-") == 0) {
			if ((arg = token()) == NULL) {
				printgnodehd();
				/* print ALL gnodes */
				for(cnt = 0;
				    cnt < tab[GNODE_T].ents;
				    cnt++)
					prgnode(cnt, 1, -1, detail);
			}
			else {
				/* print ALL asked for slots */
				do {
					index = get_gnode_slot(arg);
					if(index != -1) {
						if(done++ == 0)
							printgnodehd();
						prgnode(index, 1, -1, 
							detail);
					}
					
				} while((arg = token()) != NULL);
				return;
			}
		}
		else if (strcmp(arg,"-maj") == 0) {
			int maj;
			if((arg = token()) == NULL) {
				printf("-maj requires dev\n");
				return;
			}
			printgnodehd();
			sscanf(arg,"%x",&maj);
			for(cnt = 0; cnt < tab[GNODE_T].ents;
			    cnt++)
				prgnode(cnt, 2, maj, detail);
			
		}
		else if (strcmp(arg,"-min") == 0) {
			int min;
			if((arg = token()) == NULL) {
				printf("-min require dev\n");
				return;
			}
			printgnodehd();
			sscanf(token(),"%x",&min);
			for(cnt = 0; cnt < tab[GNODE_T].ents;
			    cnt++)
				prgnode(cnt, 3, min, detail);
		}
		else if (strcmp(arg,"-fs") == 0) {
			int filesys;
			if((arg = token()) == NULL) {
				printf("-fs requires mount slot\n");
				return;
			}
			if(isdigit(*arg))
				filesys = atoi(arg);
			else {
				printf("%s is an invalid token\n",
				       arg);
				while(token());
				return;
			}
			printgnodehd();
			for(cnt = 0; cnt < tab[GNODE_T].ents;
			    cnt++)
				prgnode(cnt, 4, filesys, detail);
		}
		else if (strcmp(arg,"-gno") == 0) {
			int gno;
			if((arg = token()) == NULL) {
				printf("-gno requires gnode\n");
				return;
			}
			gno = get_gnode_slot(arg);
			if(gno == -1) {
				printf("'%s' bad\n", arg);
				while(token());
				return;
			}
			printgnodehd();
			for(cnt = 0; cnt < tab[GNODE_T].ents;
			    cnt++)
				prgnode(cnt, 5, gno, detail);
		}
		else if (strcmp(arg,"-uid") == 0) {
			int uid;
			if((arg = token()) == NULL) {
				printf("-uid requires uid\n");
				return;
			}
			if(isdigit(*arg))
				uid = atoi(arg);
			else {
				printf("%s is an invalid token\n",
				       arg);
				while(token());
				return;
			}
			printgnodehd();
			for(cnt = 0; cnt < tab[GNODE_T].ents;
			    cnt++)
				prgnode(cnt, 6, uid, detail);
		}
		else if (strcmp(arg,"-gid") == 0) {
			int gid;
			if((arg = token()) == NULL) {
				printf("-gid requires gid\n");
				return;
			}
			if(isdigit(*arg))
				gid = atoi(arg);
			else {
				printf("%s is an invalid token\n",
				       arg);
				while(token());
				return;
			}
			printgnodehd();
			for(cnt = 0; cnt < tab[GNODE_T].ents;
			    cnt++)
				prgnode(cnt, 7, gid, detail);
		}
		else if (strcmp(arg,"-lmod") == 0) {
			int mode;
			if((arg = token()) == NULL) {
				printf("-lmod requires lower modes\n");
				return;
			}
			printgnodehd();
			sscanf(arg,"%o",&mode);
			if (mode > 0777) {
				printf("bad mode\n");
				while(token()!=NULL);
				return;
			}
			for(cnt = 0; cnt < tab[GNODE_T].ents;
			    cnt++)
				prgnode(cnt, 8, mode, detail);
		}
		else if (strcmp(arg,"-hmod") == 0) {
			int mode;
			if((arg = token()) == NULL) {
				printf("-hmod requires high modes\n");
				return;
			}
			printgnodehd();
			sscanf(arg,"%o",&mode);
			if (mode & 0777) {
				printf("bad mode\n");
				while(token()!=NULL);
				return;
			}
			for(cnt = 0; cnt < tab[GNODE_T].ents;
			    cnt++)
				prgnode(cnt, 9, mode);
		}
		else if (strcmp(arg,"-amod") == 0) {
			int mode;
			if((arg = token()) == NULL) {
				printf("-amod requires absolute modes\n");
				return;
			}
			printgnodehd();
			sscanf(arg,"%o",&mode);
			for(cnt = 0; cnt < tab[GNODE_T].ents;
			    cnt++)
				prgnode(cnt, 10, mode, detail);
		}
		else printf("bad arg\n");
	}
	else {
		/* print gnode slots asked for */
		do {
			index = get_gnode_slot(arg);
			if(index != -1) {
				if(done++ == 0)
					printgnodehd();
				prgnode(index, 1, -1, detail);
			}
		} while((arg = token()) != NULL);
		return;
	}
	while(token()!=NULL);
}

c_gnofree(c)
	char *c;
{
	char *arg;
	int index, detail;
	unsigned int addr;


	printgnodehd();
	prgnodelist((struct gnode *)(Gfree.s_value), detail);
	while(token() != NULL);
}

c_gnolock(c)
	char *c;
{
	char *arg;
	int index, cnt, detail;
	unsigned int addr;


	printgnodehd();
	for(cnt = 0; cnt < tab[GNODE_T].ents; cnt++)
		prgnode(cnt, 11, 0, detail);
}

c_gnorefs(c)
	char *c;
{
	char *arg;
	int index;
	unsigned int addr;


	arg = token();
	if (arg==NULL)
		return;
	do {
		index = get_gnode_slot(arg);
		if(index != -1) {
			pr_gref(index);
			}
	} while((arg = token()) != NULL);
}

c_dupreq(c)
	char *c;
{
	char *arg;
	int index;
	unsigned int addr;


	prdupreq();
	while(token() != NULL);
}

c_mbuf(c)
	char *c;
{
	char *arg;
	int index;
	unsigned int addr;

	arg = token();
	if (arg == NULL)
		goto mbuf_usage;
	addr = scan_vaddr(arg);
	prmbuf_chain(addr);
	return;

mbuf_usage:
	printf("usage: mbuf addr\n");
}

c_inpcb(c)
	char *c;
{
	char *arg;
	int index;
	unsigned int addr;

	arg = token();
	if (arg == NULL) {
		printf("\nUDP:\n");
		readsym(symsrch("_udb"), &addr, sizeof(addr));
		prinpcb(addr);
		printf("\nTCP:\n");
		readsym(symsrch("_tcb"), &addr, sizeof(addr));
		prinpcb(addr);
		return;
	}
	if (strcmp(arg,"-udp") == 0) {	
		readsym(symsrch("_udb"), &addr, sizeof(addr));
		prinpcb(addr);
	}
	if (strcmp(arg,"-tcp") == 0) {
		readsym(symsrch("_tcb"), &addr, sizeof(addr));
		prinpcb(addr);
	}
}

c_cred(c)
	char *c;
{
	char *arg;
	int index, done=0;
	unsigned int addr;

	done = 0;
	arg = token();
	if(arg == NULL) {
		printf("cred needs addr\n");
		return;
	}
	do {
		if(done++ == 0)
			pr_credhdr();			
		addr = scan_vaddr(arg);
		(void)print_cred(addr);
	} while((arg = token()) != NULL);
}

c_crref(c)
	char *c;
{
	char *arg;
	int index;
	unsigned int addr;

	while((arg = token()) != NULL) {
		addr = scan_vaddr(arg);
		(void)cred_reflist(addr);
	}
}

c_crcheck(c)
	char *c;
{
	char *arg;
	int index;
	unsigned int addr;

	(void)crcheck();
	while((arg = token()) != NULL);
}

c_resync(c)
	char *c;
{
	char *arg;
	int index;
	unsigned int addr;

	(void)resync();
	while((arg = token()) != NULL);
}

c_export(c)
	char *c;
{
	char *arg;
	int index;
	unsigned int addr;

	prexport();
	while((arg = token()) != NULL);
}

c_proc(c)
	char *c;
{
	char *arg;
	int index, cnt, done=0, r;
	unsigned int addr;

	if((arg = token()) == NULL) {
		(void)printprochd();
		for(cnt = 0; cnt < tab[PROC_T].ents; cnt++)
			prproc(cnt, 0, 0);
	} else if(*arg == '-') {
		r = arg[1] == 'r' ? 0 : 1;
		if((arg = token()) == NULL) {
			(void)printprochd();
			for(cnt=0; cnt < tab[PROC_T].ents; cnt++)
				prproc(cnt, !r, 0);
		} else
			do {
				if((index =
				    gt_proc_slt(arg)) != -1) {
					if(done++ == 0)
						(void)printprochd();
					prproc(index, !r, 1);
				}
			}
		while((arg = token()) != NULL);
	} else
		do {
			if((index = gt_proc_slt(arg)) != -1) {
				if(done++ == 0)
					printprochd();
				prproc(index, 0, 1);
			}
		} while((arg = token()) != NULL);
}

c_proclock(c)
	char *c;
{
	char *arg;
	int index, cnt, r;
	unsigned int addr;

	if((arg = token()) == NULL) {
		for(cnt = 0; cnt < tab[PROC_T].ents; cnt++)
			prproclock(cnt, 0, 0);
	} else if(*arg == '-') {
		r = arg[1] == 'r' ? 0 : 1;
		if((arg = token()) == NULL) {
			for(cnt=0; cnt < tab[PROC_T].ents; cnt++)
				prproclock(cnt, !r, 0);
		} else 
			do {
				if((index =
				    gt_proc_slt(arg)) != -1) {
					prproclock(index, !r, 1);
				}
			}
		while((arg = token()) != NULL);
	} else
		do {
			if((index = gt_proc_slt(arg)) != -1) {
				prproclock(index, 0, 1);
			}
		} while((arg = token()) != NULL);
}

c_tty(c)
	char *c;
{
	char *arg;
	int index, done=0;
	unsigned int addr;

	if((arg = token()) == NULL) {
		printf("usage: tty [-] proc slot ...\n");
		return;
	}
	if(*arg == '-') {
		if((arg = token()) == NULL) {
			printf("usage: tty [-] proc slot ...\n");
			return;
		}
		done = 1;
	}
	do {
		if((index = gt_proc_slt(arg)) != -1) 
			prtty(done, index);
	} while ((arg = token()) != NULL);
}
		
c_fsdata(c)
	char *c;
{
	char *arg;
	int index, cnt, done=0;
	unsigned int addr;

	if((arg = token()) == NULL) {
		printfsdatahd();
		for(cnt=0; cnt < tab[MOUNT_T].ents; cnt++)
			prfsdata(cnt, 0);
	} else do {
		index = get_mount_slot(arg);
		if(index != -1) {
			if(done++ == 0)
				printfsdatahd();
			prfsdata(index, 1);
		}
	} while((arg = token()) != NULL);
}

c_text(c)
	char *c;
{
	char *arg;
	int index, cnt;
	unsigned int addr;

	printf("SLOT GNODE REF  LDREF ");
	printf("PROC ");
	printf("SIZE  RSS FLAGS\n");
	if((arg = token()) == NULL)
		for(cnt = 0; cnt < tab[TEXT_T].ents; cnt++)
			prtext(cnt, 0);
	else do {
		if(*arg == '-') {
			for(cnt = 0; cnt < tab[TEXT_T].ents; cnt++)
				prtext(cnt, 1);
			break; 
		}
		prtext(atoi(arg), 1);
	} while((arg = token()) != NULL);
}

c_ftext(c)
	char *c;
{
	char *arg;
	int index, cnt;
	unsigned int addr;

	printf("SLOT GNODE REF  LDREF ");
	printf("PROC SIZE  RSS  FORW/BACK FLAGS\n");
	if((arg = token()) == NULL)
		for(cnt = 0; cnt < tab[TEXT_T].ents; cnt++)
			(void)fprtext(cnt);
	else do {
		(void)fprtext(atoi(arg));
	} while((arg = token()) != NULL);
}

c_mount(c)
	char *c;
{
	char *arg;
	int index, cnt, done=0;
	unsigned int addr;

	if((arg = token()) == NULL) {
		(void)printmounthd(1);
		for(cnt = 0; cnt < tab[MOUNT_T].ents; cnt++)
			prmount(cnt, 0);	/*valid&long*/
	}
	else if (strcmp(arg,"-s") == 0) {
		if((arg = token()) == NULL) {
			(void)printmounthd(0);
			for(cnt=0; cnt < tab[MOUNT_T].ents;
			    cnt++)
				prmount(cnt, 2);
		} else {
			while(arg != NULL) {
				index = get_mount_slot(arg);
				if(index != -1) {
					if(done++ == 0)
						(void)printmounthd(0);
					prmount(index, 2);
				}
				arg = token();
			}
		}
	} else {
		do {
			index = get_mount_slot(arg);
			if(index != -1) {
				if(done++ == 0)
					(void)printmounthd(1);
				prmount(index, 0);
			}

		} while((arg = token()) != NULL);
	}
}

c_ts(c)
	char *c;
{
	char *arg;
	int index;
	unsigned int addr;
	struct	Symbol	*nmsrch(), *symgrep(), *search(), *sp;

	while((arg = token()) != NULL) {
		printf("%s:\t",arg);
		addr = scan_vaddr(arg);
		sp = search(addr);
		if (sp != NULL) 
		prsym(sp,addr);
		else
			printf("No Match.");
		printf("\n");
		addr = 0;
	}
}

c_ds(c)
	char *c;
{
	char *arg;
	int index;
	unsigned int addr;

	while((arg = token()) != NULL) {
		printf("%s:\t",arg);
		addr = scan_vaddr(arg);
		praddr(addr);
		printf("\n");
		addr = 0;
	}
}

int already_seen;

c_q(c)
	char *c;
{
	char *arg;
	int index;
	unsigned int addr;
	struct tsw *tp;

	if (!already_seen) {
		printf("ULTRIX 4.0 Crash Dump Analyzer\n");
		printf("Setting HISTSIZE in the environment tells 'crash' how many commands to save.\n");	
		printf("The default is 20.\n");
		printf("\n\nthe general format of commands is:\n");
		printf("<command> [flags] [specifier]\n");
		printf("\t<command> is the list of commands below\n");
		printf("\t[flags] is the optional list of flags seen below\n");
		printf("\t[specifier] describes the table slot\n");
		printf("\t    it may be an integer, # for the process id\n");
		printf("\t    or either @ or * for the hex table address\n");
		printf("\nusage: crash [dump] [namelist]\n");
		printf("available commands:\n");
		printf("\n");
		already_seen++;
	}
	arg = token();
	for(tp = t; tp->t_action != NULL; tp++) {
		if(tp->t_dsc != 0) {
			if ((arg == NULL) || (*arg == NULL))
				printf("%s\t%s\n",
				       tp->t_nm,tp->t_dsc);
			else {
				if ((arg != NULL) && *arg == *tp->t_nm)
					printf("%s\t%s\n",
					       tp->t_nm,tp->t_dsc);
			}
		}
	}
	fflush(stdout);
	if (arg != NULL) while(token() != NULL);
}

c_stat(c)
	char *c;
{
	char *arg;
	int index;
	unsigned int addr;

	prstat();
	while(token() != NULL);
}

c_bufhdr(c)
	char *c;
{
	char *arg;
	int index,cnt,done=0;
	unsigned int addr;

	if((arg = token()) == NULL) {
		printbufhd();
		for(cnt = 0; cnt < tab[BUF_T].ents; cnt++)
			prbufhdr(cnt, 0);
	} else if(*arg == '-') {
		if((arg = token()) == NULL) {
			printbufhd();
			for(cnt = 0; cnt < tab[BUF_T].ents; cnt++)
				prbufhdr(cnt, 1);
		} else {
			index = get_buf_slot(arg);
			do {
				if(index != -1) {
					if(done++ == 0)
						printbufhd();
					prbufhdr(index, 1);
				}
			} while((arg = token()) != NULL);
		}
	} else {
		do {
			index = get_buf_slot(arg);
			if(index != -1) {
				if(done++ == 0)
					(void)printbufhd();
				prbufhdr(index, 0);
			}
		} while((arg = token()) != NULL);
	}
}
extern struct buf bfreelist[BQUEUES];
c_bufbusy(c)
	char *c;
{
	char *arg;
	int index;
	unsigned int addr;

	(void)printbufhd();
	prbufbusy();
	while((arg = token()) != NULL);
}

c_bufdirty(c)
	char *c;
{
	char *arg;
	int index;
	unsigned int addr;

	(void)printbufhd();
	prbuflist(bfreelist[BQ_DIRTY].av_forw);
	while((arg = token()) != NULL);
}

c_bufclean(c)
	char *c;
{
	char *arg;
	int index;
	unsigned int addr;

	(void)printbufhd();
	prbuflist(bfreelist[BQ_CLEAN].av_forw);
	while((arg = token()) != NULL);
}

c_bufempty(c)
	char *c;
{
	char *arg;
	int index;
	unsigned int addr;

	(void)printbufhd();
	prbuflist(bfreelist[BQ_EMPTY].av_forw);
	while((arg = token()) != NULL);
}
extern	struct	prmode	prm[];		
c_buffer(c)
	char *c;
{
	char *arg;
	int index, cnt, prdef=HEX;
	unsigned int addr;
	struct	prmode	*prptr;

	if((arg = token()) == NULL)
		for(cnt = 0; cnt < tab[BUF_T].ents; cnt++)
			prbuffer(cnt, prdef);
	else if((*arg >= '0' && *arg <= '9') || (*arg == '@') ||
		(*arg == '*')) {
		do {
			index = get_buf_slot(arg);
			if(index != -1)
				prbuffer(index, prdef);
		} while((arg = token()) != NULL);
	} else {
		for(prptr = prm; prptr->pr_sw != 0; prptr++)
			if(!strcmp(arg, prptr->pr_name))
				break;
		if(prptr->pr_sw == 0) {
			error("invalid mode");
			while(token() != NULL);
			return;
		}
		prdef = prptr->pr_sw;
		if((arg = token()) == NULL)
			for(cnt = 0; cnt < tab[BUF_T].ents; cnt++)
				prbuffer(cnt, prptr->pr_sw);
		else {
			do {
				index = get_buf_slot(arg);
				if(index != -1)
					prbuffer(index, prptr->pr_sw);
			} while((arg = token()) != NULL);
		}
	}
}

c_ppte(c)
	char *c;
{
	char *arg;
	int index, done=0;
	unsigned int addr;

	done = 0;
	while((arg = token()) != NULL) {
		index = gt_proc_slt(arg);
		if(index != -1) {
			if(done++ == 0)
				(void)printptehd();
			prpte(index);
		}
	}
}
c_spt(c)
	char *c;
{
	char *arg;
	int index;
	struct pte *pte;	
	
	printf("System Page Table - 0x%08x, %d entries\n",sptptr,sptlen);
	(void)printptehd();
	for (pte=(struct pte *)sptptr; 
	     pte< (struct pte *) (sptptr+sptlen*sizeof(struct pte)); 
	     pte++) {
		printkpte(pte,"Kernel");
	}
}
	
c_tout(c)
	char *c;
{
	char *arg;
	int index;
	unsigned int addr;

	printf("FUNCTION ARGUMENT  TIME\n");
	prcallout();
	while(token() != NULL);
}

c_nm(c)
	char *c;
{
	char *arg;
	int index;
	unsigned int addr;

	while((arg = token()) != NULL)
		prnm(arg);
	printf("\n");
}

c_od(c)
	char *c;
{
	char *arg;
	int index, units;
	unsigned int addr;
	struct	Symbol	*nmsrch(), *symgrep(), *search(), *sp;

	if((arg = token()) == NULL) {
		error("symbol expected");
		return;
	}

	if((sp = nmsrch(arg)) == NULL) {
	if(isdigit(arg[0])) {
		addr = scan_vaddr(arg);
		} else {
		printf("symbol not found\n");
		while(token() != NULL);
		return;
		}
	} else	addr = sp->s_value;

	if((arg = token()) == NULL) {
		units = 1;
		arg = NBPW==4 ? "hex" : "octal";
	} else {
		units = atoi(arg);
		if(units == -1) {
			while(token() != NULL);
			return;
		}
		if((arg = token()) == NULL)
			arg = NBPW==4 ? "hex" : "octal";
		else
			while(token() != NULL);
	}
	prod(addr, units, arg);
}

extern char *map_names[];
extern int num_maps, map_to_do;

c_map(c)
	char *c;
{
	char *arg;
	int index, i;
	unsigned int addr;

	arg = token();
maptop:
	if (arg) map_to_do = atoi(arg);
	if (arg == NULL || (map_to_do < 0 || map_to_do > num_maps-1)) {
		printf("Bad map number %s\n",arg);
		printf("Legal maps are 0 to %d\n",num_maps-1);
		for (i=0;i<num_maps;i++)
			printf("%s\n",map_names[i]);
		if (arg != NULL) while(token()!=NULL);
		return;
	}
	else domap();
	arg = token();
	if (arg != NULL) goto maptop;
}

c_dis(c)
	char *c;
{
	char *arg;
	int index;
	unsigned int addr;

	/* disassemble at address */
	if((arg = token()) == NULL) {
		printf("dis: need address\n");
		return;
	} else {
		char *arg2;
		if((arg2 = token()) == NULL)
			arg2 = arg;
		else
			while((token()) != NULL);
		dis(arg, arg2);
	}
	fflush(stdout);
}

c_cmap(c)
	char *c;
{
	char *arg;
	int index;
	unsigned int addr;
	
	printf("SLOT TYPE MDEV HLINK  NDX  BLKNO   PAGE       NEXT    PREV    ");
	printf("FLAGS\n");
	if((arg = token()) == NULL)
		prcmap(NULL, MFREE_CRASH);
	else if(*arg == '-') {
		switch(arg[1]) {
		      case 'i' :
			      if((arg = token()) == NULL) {
				      printf("This option requires input\n");
				      return;
			      } else do {
				      prcmap(atoi(arg),MONE);
			      } while((arg = token()) != NULL);
			      break;
			    case 'h' :
				    if((arg = token()) == NULL) {
					    printf("This option requires input\n");
					    return;
				    } else do {
					    prcmap(atoi(arg),MHASH);
				    } while((arg = token()) != NULL);
			      break;
			    case 'b' :
				    if((arg = token()) == NULL) {
					    printf("This option requires input\n");
					    return;
				    } else do {
					    prcmap(atoi(arg),MBLKNO|MHASH);
				    } while((arg = token()) != NULL);
			      break;
			    case 'a' :
				    if((arg = token()) == NULL) {
					    printf("This option requires input\n");
					    return;
				    } else do {
					    prcmap(atoi(arg),MADDR | MONE);
				    } while((arg = token()) != NULL);
			      break;
			      default :
				      printf("Invalid switch\n");
			      return;
		      }
	} else do {
		prcmap(atoi(arg),MPFN | MONE);
	} while((arg = token()) != NULL);
}
	
c_cpu(c)
	char *c;
{
	char *arg;
	int index;
	unsigned int addr;

	if ((arg = token()) == NULL) {
		prcpudata(0);
		return;
	} else if (strcmp(arg, "-l") ==0) {
		prcpudata(1);
		return;
	} else {
		printf("%s Invalid switch\n",arg);
		return;
	}
}
			

#ifdef vax
extern struct 	nlist *last_grep;
#endif vax
#ifdef mips
extern int	last_grep;
#endif mips


c_lock(c)
	char *c;
{
	char *arg;
	int index;
	unsigned int addr;
	struct	Symbol	*nmsrch(), *symgrep(), *search(), *sp;

	if((arg = token()) == NULL) {
		error("argument expected");
		return;
	}
	if (strcmp(arg, "-all") == 0) {
#ifdef vax
		last_grep = NULL;
#endif vax
#ifdef mips
		last_grep = 0;
#endif mips
			
		sp = NULL;
		prlockhead();
		while ((sp=symgrep("_lk_")) != NULL) 
			prlock(sp->s_value,1);
		return;
	} else {
		if(isdigit(arg[0])) 
			addr = scan_vaddr(arg);
		else if((sp = nmsrch(arg)) == NULL) {
			printf("symbol not found\n");
			while(token() != NULL);
			return;
		} else	
			addr = sp->s_value;
		prlockhead();
		prlock(addr,1);
		return;
	}
}			
	
c_duphash(c)
	char *c;
{
	char *arg;
	int index;

	if ((arg = token()) == NULL) {
		prduphash(0);
		return;
	} else if (strcmp(arg, "-l") ==0) {
		prduphash(1);
		return;
	} else {
		printf("%s Invalid switch\n",arg);
		return;
	}
}
	
c_gstats(c)
	char *c;
{
	char *arg;
	int index;

	if ((arg = token()) == NULL) {
		prgstats(0);
		return;
	} else {
		printf("%s Invalid argument\n",arg);
		return;
	}
}
/* the following array is derived from cpuconf.h */
char *names[]={
"UNKN_SYSTEM",
"VAX_780    ",
"VAX_750    ",
"VAX_730    ",
"VAX_8600   ",
"VAX_8200   ",
"VAX_8800   ",
"MVAX_I     ",
"MVAX_II    ",
"V_VAX      ",
"VAX_3600   ",
"VAX_6200   ",
"VAX_3400   ",
"C_VAXSTAR  ",
"VAX_60     ",
"VAX_3900   ",
"DS_3100    ",
"VAX_8820   ",
"DS_5400    ",
"DS_5800    ",
"DS_5000    ",
"DS_CMAX    ",
"VAX_6400   ",
"VAXSTAR    ",
"DS_5500    ",
"DS_5100    ",
"VAX_9000   ",
"DS_5000_100"};

extern 	char *dumpfile,*namelist;
c_sig(c)
	char *c;
{
	int	cpu_avail; /* number of processors that are in the system */
	int	cpu_systype;	/* the real sys type register */
	int	cpu;		/* CPU ID (defined above) not all match SID reg. */
	int	cpu_subtype;	/* CPU subtype (defined above) from SYS_TYPE reg. */
	int	cpu_archident;	/* saved ARCH_IDENT bits from SYS_TYPE register */
	int	vs_cfgtst;	/* VAXstar/CVAXstar configuration and test register */
	int	mb_slot;	/* M-Bus slot containing the Firefox I/O module */
	int 	arch;

	struct	Symbol	*nmsrch(), *symgrep(), *search(), *sp;
	char *ctime();
	struct timeval t,tboot;

	printf("Vmcore: %s Vmunix: %s Hostname: ",dumpfile,namelist);
	if ((sp = nmsrch("hostname")) == NULL) {
		printf("*Unknown*");
	} else {
		prod(sp->s_value, 1, "s");
		printf(" ");
	}
	readsym (symsrch("_cpu"), (char *)&cpu,sizeof(int));
	readsym (symsrch("_cpu_systype"), (char *)&cpu_systype,sizeof(int));
	readsym (symsrch("_cpu_avail"), (char *)&cpu_avail,sizeof(int));

	printf("cpu: %s subtype: %d avail: %d\n",
	       names[cpu],cpu_subtype,cpu_avail);

	readsym (symsrch("_boottime"), (char *)&tboot,sizeof(struct timeval));
	printf("Boot-time:\t%s",ctime(&tboot.tv_sec));

	readsym (symsrch("_atime"), (char *)&t,sizeof(struct timeval));
	printf("    Time:\t%s",ctime(&t.tv_sec));

	printf("Kernel version: \n");
	if ((sp = nmsrch("version")) == NULL) {
		printf("*Unknown*\n");
	} else {
		prod(sp->s_value, 1, "s");
		printf("\n");
	}
	
}			
