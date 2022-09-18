#ifndef lint
static char *sccsid = "@(#)misc.c	4.1      (ULTRIX)        7/17/90";
#endif

/************************************************************************
 *									*
 *			Copyright (c) 1988-1989 by			*
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
/*
 * History:
 *
 * 02-Mar-90  Janet Schank
 *      siisc is no longer used.  Both vax and mips now use sz_softc.
 *      Changed Nscsi to Nscsibus.
 *
 */

#include	<sys/types.h>
#define KERNEL
#include <sys/time.h>
#undef KERNEL
#include	"crash.h"
#include	<sys/gnode.h>
#include	<sys/proc.h>
#include	<sys/smp_lock.h>
#define KERNEL
#include	<sys/file.h>
#include	<sys/text.h>
#include	<sys/buf.h>
#include	<sys/callout.h>
#include	<sys/mount.h>
#undef KERNEL
#include	<strings.h>
#include	<sys/cmap.h>
#include	<sys/socket.h>
#include	<sys/socketvar.h>
#include	<net/if.h>
#include	<netinet/in.h>
#include	<arpa/inet.h>
#include	<netinet/if_ether.h>
#include	<nfs/dnlc.h>

#define	SEGBYTES	0x40000000L	/* power of 2 to the 30th */
#define	CORECLKS	512		/* Core Clicks */
#define	BITMASK		0777		/* Remove low order bits */

extern	struct	dstk	dmpstack;
extern	struct	glop	dumpglop;
extern	struct	uarea	x;

fatal(str)
	char	*str;
{
	printf("error: %s\n", str);
	(void)exit(1);
}

error(str)
	char	*str;
{
	printf("error: %s\n", str);
}

#ifdef notdef
int
atoi(s)
	char *s;
{
	int i;
	if (s == NULL) error ("illegal integer");
	if(s[0] != '0') {
		if (!sscanf(s,"%d",&i)) {
			error ("illegal integer");
			return(-1);
		}
		return(i);
	}
	if (s[1] == '\0') return(0);
	if ((s[1] == 'x') || (s[1] == 'X')) {
		if (s[2] == '\0') {
			error ("illegal integer");
			return(-1);
		}
		if (!sscanf(&s[2],"%x",&i)) {
			error ("illegal integer");
			return(-1);
		}
		return(i);
	}
	if (!sscanf(s,"%o",&i)) {
		error ("illegal integer");
		return(-1);
	}
	return(i);
}
#endif

unsigned int
scan_vaddr(s)
	char *s;
{
	unsigned i;
	if (s == NULL) error ("bad address format");
	if (s[0] == '0') {
		if (s[1] == '\0') return(0);
		if (((s[1] != 'x') && (s[1] != 'X')) || 
		    (s[2] == '\0') || !sscanf(&s[2],"%x",&i))
			error ("bad address format");
	}
	else if (!sscanf(s,"%x",&i)) error ("bad address format");
	return(i);
}	
	
struct tabsum tab[TABSUM_MAX];
struct mapsum map[MAPSUM_MAX];
unsigned int slr, sbr;
struct buckets *gnodebuckets[GNODEBUCKETS];
struct buckets *filebuckets[FILEBUCKETS];
struct buckets *mountbuckets[MOUNTBUCKETS];
struct buckets *bufbuckets[BUFBUCKETS];
struct buckets *procbuckets[PROCBUCKETS];

init()
{
	extern	char	*dumpfile;
	extern	struct	dstk	dmpstack;
	extern	int	Kfp;
	extern	char	*namelist;
	extern	struct	Symbol	*stbl;
	extern	void	sigint();
	extern  struct user USER;
	extern  struct buf bfreelist[];
	struct	sigvec	vec;
	int	i;
	struct	Symbol *mapsym;
	char *malloc();
	struct	Symbol	*symsrch(), *tmp;
	

	/*
	 * Initialize the Symbol structures for devices which
	 * might NOT exist.
	 */
	bzero(&Mscp_classb, sizeof(struct Symbol));
	bzero(&Tmscp_classb, sizeof(struct Symbol));
	bzero(&Mscp_utable, sizeof(struct Symbol));
	bzero(&Tmscp_utable, sizeof(struct Symbol));
	bzero(&Sz_softc, sizeof(struct Symbol));
	bzero(&Nscsibus, sizeof(struct Symbol));
	bzero(&Scs_config_db, sizeof(struct Symbol));
	bzero(&Ports, sizeof(struct Symbol));
	
	if ((mem = open (dumpfile,O_RDONLY,0)) < 0)
		fatal("cannot open dump file");

	rdsymtab();
	
	/*
	 * Find the symbol table entries for the symbols we need.
	 */
#ifdef vax
	Usrptma = *symsrch("_Usrptma"); 
#endif vax
#ifdef mips
	Usrptma = *symsrch("_Usrptmap");
#endif mips

	Arptab = *symsrch("_arptab");
	Text = *symsrch("_text");
	Swap = *symsrch("_swapmap");
	Sptbase = *symsrch("_Sysmap");
	Sys = *symsrch("_utsname");
	Lbolt = *symsrch("_lbolt");
	Panic = *symsrch("_panicstr");
	Callout = *symsrch("_callout");
	Usrpt = *symsrch("_usrpt");
	Cmap = *symsrch("_cmap");
	Dnlcache = *symsrch("_ncache");
	Buffree = *symsrch("_bfreelist");

	/* Now we initialize values which are, currently,
		configuration dependent */

	if ((tmp = symsrch("_mscp_classb")) != NULL)
		Mscp_classb = *tmp;
	if ((tmp = symsrch("_tmscp_classb")) != NULL)
		Tmscp_classb = *tmp;
	if ((tmp = symsrch("_mscp_unit_tbl")) != NULL)
		Mscp_utable = *tmp;
	if ((tmp = symsrch("_tmscp_unit_tbl")) != NULL)
		Tmscp_utable = *tmp;
	if ((tmp = symsrch("_scs_config_db")) != NULL) 
		Scs_config_db = *tmp;
	if ((tmp = symsrch("_scs_lport_db")) != NULL) 
		Ports = *tmp;
	if ((tmp = symsrch("_sz_softc")) != NULL)
		Sz_softc = *tmp;
	if ((tmp = symsrch("_nNSCSIBUS")) != NULL)
		Nscsibus = *tmp;
	
	vaddrinit();

	/*
	 * Collect some information about kernel tables and store it
	 * in the tabsum array for later use.
	 */
	
	
	initfile();
	initgnode();
	initmount();
	initbuf();
	initproc();
	
	tab[ARP_T].first = Arptab.s_value;
	readsym (symsrch("_arptab_size"), (char *)&tab[ARP_T].ents,
		 sizeof(int));
	tab[ARP_T].name = "arptab";
	tab[ARP_T].size = sizeof(struct arptab);
	
	readsym (&Text, (char *)&tab[TEXT_T].first,sizeof(unsigned));
	readsym (symsrch("_ntext"), (char *)&tab[TEXT_T].ents,sizeof(int));
	tab[TEXT_T].name = "text";
	tab[TEXT_T].size = sizeof(struct text);

	readsym (symsrch("_firstfree"),&firstfree,sizeof(int));
	readsym (&Cmap, (char *)&tab[CMAP_T].first, sizeof(unsigned));
	readsym (symsrch("_ncmap"), (char *)&tab[CMAP_T].ents,sizeof(int));
	tab[CMAP_T].name = "cmap";
	tab[CMAP_T].size = sizeof(struct cmap);

	readsym (&Callout, (char *)&tab[CALLOUT_T].first, sizeof(unsigned));
	readsym (symsrch("_ncallout"), (char *)&tab[CALLOUT_T].ents,
	    sizeof(int));
	tab[CALLOUT_T].name = "callout";
	tab[CALLOUT_T].size = sizeof(struct callout);

/*	tab[DNLC_T].first = Dnlcache.s_value; */
	readsym (&Dnlcache, (char *)&tab[DNLC_T].first, sizeof(unsigned));
	readsym (symsrch("_ncsize"), &tab[DNLC_T].ents, sizeof(int));
	tab[DNLC_T].name = "dnlcache";
	tab[DNLC_T].size = sizeof(struct ncache);
	for (i = 0; i < TABSUM_MAX; i++)
		tab[i].last = tab[i].first + (tab[i].ents * tab[i].size);

	tab[PROC_T].last = (unsigned) 0;
	tab[FILE_T].last = (unsigned) 0;
	tab[MOUNT_T].last = (unsigned) 0;
	tab[GNODE_T].last = (unsigned) 0;
	tab[BUF_T].last = (unsigned) 0;
	tab[BUFH_T].last = (unsigned) 0;
	
	/*
	 * Set up the page table map summaries.
	 * WARNING: this code is dependent on the ordering of page
	 * table allocations in spt.s.
	 */
#ifdef vax
	mapsym = symsrch("_mbutl");
	map[MBUF_SPT].first = mapsym->s_value;
	map[MBUF_SPT].name = "Mbmap";
	map[MBUF_SPT].descrip = "mbuf pool"; 

	mapsym = symsrch("_dmempt");
	map[MBUF_SPT].last = mapsym->s_value - 1;
	map[DMEM_SPT].first = mapsym->s_value;
	map[DMEM_SPT].name = "dmemptmap";
	map[DMEM_SPT].descrip = "km_alloc pool";
#endif	
	tab[CMHASH_T].first = Cmhash.s_value;
	tab[CMHASH_T].ents = CMHSIZ;
	tab[CMHASH_T].name = "cmhash";
	tab[CMHASH_T].size = sizeof(int);
#ifdef vax
	mapsym = symsrch("_vmb_info");
	map[DMEM_SPT].last = mapsym->s_value - 1;
	readsym (symsrch("_Syssize"), (char *)&slr, sizeof(int));
	sbr = Sys.s_value;
#endif


	/*
	 * Set up signal handler so we can trap ctrl-c.
	 */

	vec.sv_handler = sigint;
	vec.sv_mask = 0;
	vec.sv_onstack = 0;
	if (sigvec(SIGINT, &vec, NULL) < 0)
		error ("Trouble with sigvec call.");	


}

prod(addr, units, style)
	unsigned	addr;
	int	units;
	char	*style;
{
	register  int  i;
	register  struct  prmode  *pp;
	int	word;
	long	lword;
	char	ch;
	extern	struct	prmode	prm[];

	if(units == -1)
		return;
	for(pp = prm; pp->pr_sw != 0; pp++) {
		if(strcmp(pp->pr_name, style) == 0)
			break;
	}
	if (addr > (unsigned)VIRT_MEM)
		addr = sysvad(addr);
	if(lseek(mem, (int)(addr & VIRT_MEM), 0) == -1) {
		error("bad seek of addr");
	}
	switch(pp->pr_sw) {
	default:
	case NULL:
		error("invalid mode");
		break;

	case OCTAL:
	case DECIMAL:
		if(addr & 03) {
			printf("warning: address not word aligned\n");
		}
		for(i = 0; i < units; i++) {
			if(i % 8 == 0) {
				if(i != 0)
					printf("\n");
				printf(FMT, (unsigned)addr + i * NBPW);
				printf(":");
			}
			if(read(mem, (char *)&word, NBPW) != NBPW) {
				printf("  read error");
				break;
			}
			printf(pp->pr_sw == OCTAL ? " %7.7o" :
				"  %5u", word);
		}
		break;

	case LOCT:
	case LDEC:
		if(addr & 03) {
			printf("warning: address not word aligned\n");
		}
		for(i = 0; i < units; i++) {
			if(i % 4 == 0) {
				if(i != 0)
					printf("\n");
				printf(FMT, (int)addr + i * NBPW);
				printf(":");
			}
			if(read(mem, (char *)&lword, sizeof (long))
			    != sizeof (long)) {
				printf("  read error");
				break;
			}
			printf(pp->pr_sw == LOCT ? " %12.12lo" :
				"  %10lu", lword);
		}
		break;

	case CHAR:
	case BYTE:
		for(i = 0; i < units; i++) {
			if(i % (pp->pr_sw == CHAR ? 16 : 8) == 0) {
				if(i != 0)
					printf("\n");
				printf(FMT, (int)addr + i * sizeof (char));
				printf(":");
			}
			if(read(mem, (char *)&ch, sizeof (char)) !=
			    sizeof (char)) {
				printf("  read error");
				break;
			}
			if(pp->pr_sw == CHAR)
				putchar(ch);
			else
				printf(" %4.4o", ch & 0377);
		}
		break;
	case HEX:
		if(addr & 03) {
			printf("warning: address not word aligned\n");
		}
		for(i = 0; i < units; i++) {
			if(i % 4 == 0) {
				if(i != 0)
					printf("\n");
				printf(FMT, (int)addr + i * NBPW);
				printf(":");
			}
			if(read(mem, (char *)&lword, sizeof (long)) !=
			    sizeof (long)) {
				printf("  read error");
				break;
			}
			printf(" %08x", lword);
		}
		break;

	case STRING: {
		char string[1024];
		if(read(mem, string, sizeof (string)) != sizeof (string)) {
			printf("  read error");
			break;
		}
		if(units == 0)
			printf(" %s", string);
		else {
			char format[10];
			sprintf(format, " %%%ds", units);
			printf(format, string);
		}
		break;
	}
	}
	putc('\n', stdout);
}

struct gnode *gnodetab=NULL;

initgnode() {
	struct buckets 	*bucketaddr=NULL;
	struct Symbol Glist, Gnode;
	struct Symbol *symsrch();
	
	Gnode = *symsrch("_gnode");
	Gfree = *symsrch("_gfreeh");
	readsym (&Gnode, (char *)&tab[GNODE_T].first, sizeof(unsigned));
	readsym (symsrch("_ngnode"), (char *)&tab[GNODE_T].ents, sizeof(int));
	tab[GNODE_T].name = "gnode";
	tab[GNODE_T].size = sizeof(struct gnode);
	tab[GNODE_T].last = (unsigned) 0;
	
	if (gnodetab == NULL)
	gnodetab = (struct gnode *) malloc((unsigned)(sizeof(struct gnode) *
	    tab[GNODE_T].ents));

	if (bucketaddr == NULL)
	bucketaddr = (struct buckets *) malloc((unsigned)
	    (sizeof(struct buckets) * tab[GNODE_T].ents));

	if((bucketaddr == NULL) || (gnodetab == NULL)) 
		fatal("cannot allocate space for gnode temp tables");
	else {	/* get us the gnodes */
		register int i;
		register int max = tab[GNODE_T].ents;
		char *gnodeaddr;
		
		bzero((char *)gnodetab, sizeof(struct gnode) * max);
		bzero((char *)bucketaddr, sizeof(struct buckets) * max);
		bzero((char *)gnodebuckets, sizeof gnodebuckets);
		Glist = *symsrch("_gnode");		
		readmem((char *)&gnodeaddr, (int)Glist.s_value,
		    sizeof(struct gnode *));
		
		for(i = 0; i < max; i++)  {
			readmem((char *)&gnodetab[i], (int)gnodeaddr,
			    sizeof(struct gnode));
			bucketaddr->index = i;
			bucketaddr->addr = (char *) gnodeaddr;
			bucketaddr->next = gnodebuckets[HASH(gnodeaddr,
			    GNODEBUCKETS)];
			gnodebuckets[HASH(gnodeaddr, GNODEBUCKETS)] =
			    bucketaddr++;
			gnodeaddr = (char *) ((int)gnodeaddr + sizeof(struct
			    gnode));
		}
	}
}

#ifdef notdef
/*MJK*/printgnodecache(hash)
	int hash;
{
	struct buckets *bp;
	int i;
	for(i = 0; i < GNODEBUCKETS; i++) {
		if(hash != -1) i = hash;
		printf("bucket %d 0x%x\n", i, &gnodebuckets[i]);
		bp = gnodebuckets[i];
		do {
			printf("bucketaddr 0x%x addr 0x%x, index %d next 0x%x\n",
			bp, bp->addr, bp->index, bp->next);
			bp = bp->next;
		} while(bp->next);
		if(hash != -1) break;
	}
}
#endif

struct file *filetab=NULL;

initfile() {
	struct buckets 	*bucketaddr=NULL;
	struct Symbol Flist;
	struct Symbol *symsrch();

	readsym (symsrch("_nfile"), (char *)&tab[FILE_T].ents, sizeof(int));
	tab[FILE_T].name = "file";
	tab[FILE_T].size = sizeof(struct file);
	tab[FILE_T].last = (unsigned) 0;
	if (filetab == NULL)
	filetab = (struct file *) malloc((unsigned)(sizeof(struct file) *
	    tab[FILE_T].ents));
	if (bucketaddr == NULL)
	bucketaddr = (struct buckets *) malloc((unsigned)
	    (sizeof(struct buckets) * tab[FILE_T].ents));
	if((bucketaddr == NULL) || (filetab == NULL)) 
		fatal("cannot allocate space for file temp tables");
	else {	/* get us the file */
		register int i;
		register int max = tab[FILE_T].ents;
		char *fileaddr;
		
		bzero((char *)filetab, sizeof(struct file) * max);
		bzero((char *)bucketaddr, sizeof(struct buckets) * max);
		bzero((char *)filebuckets, sizeof filebuckets);
		Flist = *symsrch("_file");		
		readmem((char *)&fileaddr, Flist.s_value, sizeof(int));
		
		for(i = 0; i < max; i++)  {
			readmem((char *)&filetab[i], (int)fileaddr,
			    sizeof(struct file));
			bucketaddr->index = i;
			bucketaddr->addr = (char *) fileaddr;
			bucketaddr->next = filebuckets[HASH(fileaddr,
			    FILEBUCKETS)];
			filebuckets[HASH(fileaddr, FILEBUCKETS)] =
			    bucketaddr++;
			fileaddr = (char *) ((int)fileaddr + sizeof(struct
			    file));
		}
	}
}

struct mount *mounttab=NULL;

initmount() {
	struct buckets 	*bucketaddr=NULL;
	struct Symbol Mlist;
	struct Symbol *symsrch();
	
	readsym (symsrch("_nmount"), (char *)&tab[MOUNT_T].ents, sizeof(int));
	tab[MOUNT_T].name = "mount";
	tab[MOUNT_T].size = sizeof(struct mount);
	tab[MOUNT_T].last = (unsigned) 0;
	
	if (mounttab == NULL)
	mounttab = (struct mount *) malloc((unsigned)(sizeof(struct mount) *
	    tab[MOUNT_T].ents));
	if (bucketaddr == NULL)
	bucketaddr = (struct buckets *) malloc((unsigned)
	    (sizeof(struct buckets) * tab[MOUNT_T].ents));

	if((bucketaddr == NULL) || (mounttab == NULL)) 
		fatal("cannot allocate space for mount temp tables");
	else {	/* get us the file */
		register int i;
		register int max = tab[MOUNT_T].ents;
		char *mountaddr;
		
		bzero((char *)mounttab, sizeof(struct mount) * max);
		bzero((char *)bucketaddr, sizeof(struct buckets) * max);
		bzero((char *)mountbuckets, sizeof mountbuckets);
		Mlist = *symsrch("_mount");		
		mountaddr = (char *) Mlist.s_value;

		for(i = 0; i < max; i++)  {
			readmem((char *)&mounttab[i], (int)mountaddr,
			    sizeof(struct mount));
			bucketaddr->index = i;
			bucketaddr->addr = (char *) mountaddr;
			bucketaddr->next = mountbuckets[HASH(mountaddr,
			    MOUNTBUCKETS)];
			mountbuckets[HASH(mountaddr, MOUNTBUCKETS)] =
			    bucketaddr++;
			mountaddr = (char *) ((int)mountaddr + sizeof(struct
			    mount));
		}
	}
}

#ifdef notdef
/*MJK*/printmountcache(hash)
	int hash;
{
	struct buckets *bp;
	int i;
	for(i = 0; i < MOUNTBUCKETS; i++) {
		if(hash != -1) i = hash;
		printf("printmountcache: bucket %d 0x%x\n", i, &mountbuckets[i]);
		fflush(stdout);
		bp = mountbuckets[i];
		while(bp) {
			printf("bucketaddr 0x%x addr 0x%x, index %d next 0x%x\n",
			bp, bp->addr, bp->index, bp->next);
			fflush(stdout);			
			bp = bp->next;
		}
		if(hash != -1) break;
	}
}
#endif

struct buf *buftab=NULL;

initbuf() {
	struct buckets 	*bucketaddr=NULL;
	struct Symbol *symsrch();
	struct Symbol Buf;
	int buf_headers;
	struct buf *buf;
	
	readsym (symsrch("_nbuf"), (char *)&tab[BUF_T].ents,sizeof(int));
	buf_headers = tab[BUF_T].ents;
	Buf = *symsrch("_buf");
	readsym (&Buf, (char *)&buf, sizeof(unsigned));
	tab[BUF_T].name = "buf";
	tab[BUF_T].size = sizeof(struct buf);
	tab[BUFH_T].first = Buffree.s_value;
	tab[BUFH_T].ents = BQUEUES;
	tab[BUFH_T].name = "bfreelist";
	tab[BUFH_T].size = sizeof(struct buf);
	tab[BUFH_T].last = (unsigned) 0;
	
	readmem((char *)bfreelist, symsrch("_bfreelist")->s_value,
	    sizeof(struct buf) * BQUEUES);
	if (buftab == NULL)
	buftab = (struct buf *) malloc((unsigned)
	    (sizeof(struct buf) * buf_headers));
	if (bucketaddr == NULL)
	bucketaddr = (struct buckets *) malloc((unsigned)
	    (sizeof(struct buckets) * buf_headers));
	if((bucketaddr == NULL) || (buftab == NULL)) {
		fatal("cannot allocate space for buf temp tables");
	} else {	/* get us the buffer headers */
		register int i;
		struct buf *bufaddr;
		
		bzero((char *)buftab, sizeof(struct buf) * buf_headers);
		bzero((char *)bucketaddr, sizeof(struct buckets) * buf_headers);
		bzero((char *)bufbuckets, sizeof bufbuckets);
		bufaddr = buf;		
#ifdef DEBUG
		printf("first buf at 0x%x buf_headers %d\n", buf, buf_headers);
#endif
		for(i = 0; i < buf_headers; i++) {		
			readmem((char *)&buftab[i], (int)bufaddr,
			    sizeof(struct buf));
			if(i < buf_headers) {
				bucketaddr->index = i;
				bucketaddr->addr = (char *) bufaddr;
				bucketaddr->next =
				    bufbuckets[HASH(bufaddr, BUFBUCKETS)];
				bufbuckets[HASH(bufaddr, BUFBUCKETS)] =
				    bucketaddr++;
			} else {
				prbadbuf(bufaddr, (struct buf *)
				    ((u_int) tab[BUFH_T].first + i));
				break;
			}
			bufaddr = (struct buf *)((int)bufaddr + sizeof(struct
				buf));
#ifdef DEBUG
			printf("buf %d at 0x%x\n", i + 1, bufaddr);
#endif
		}
	}
}

#ifdef notdef
/*MJK*/printbufcache(hash)
	int hash;
{
	struct buckets *bp;
	int i;
	for(i = 0; i < BUFBUCKETS; i++) {
		if(hash != -1) i = hash;
		bp = bufbuckets[i];
		printf("printbufcache: bucket %d 0x%x\n", i, &bufbuckets[i]);
		fflush(stdout);
		while(bp) {
			printf("bucketaddr 0x%x addr 0x%x, index %d next 0x%x\n",
			bp, bp->addr, bp->index, bp->next);
			fflush(stdout);			
			bp = bp->next;
		}
		if(hash != -1) break;
	}
}
#endif

prbadbuf(bp, stop)
	struct buf *bp;
	char *stop;
{
	while(bp != (struct buf *)stop) {
		printf("bad (non allocated buffer) at 0x%x\n", bp);
	}
}


struct proc *proctab=NULL;

initproc() {
	struct buckets 	*bucketaddr=NULL;
	struct Symbol Plist;
	struct Symbol *symsrch();
	
	Plist = *symsrch("_proc");
	tab[PROC_T].first = Plist.s_value;
	readsym (symsrch("_nproc"), (char *)&tab[PROC_T].ents, sizeof(int));
	tab[PROC_T].name = "proc";
	tab[PROC_T].size = sizeof(struct proc);
	tab[PROC_T].last = (unsigned) 0;
	
	if (proctab == NULL)
	proctab = (struct proc *) malloc((unsigned)(sizeof(struct proc) *
	    tab[PROC_T].ents));
	if (bucketaddr == NULL)	
	bucketaddr = (struct buckets *) malloc((unsigned)
	    (sizeof(struct buckets) * tab[PROC_T].ents));
	if((bucketaddr == NULL) || (proctab == NULL)) 
		fatal("cannot allocate space for proc temp tables");
	else {	/* get us the file */
		register int i;
		register int max = tab[PROC_T].ents;
		int procaddr;
		
		bzero((char *)proctab, sizeof(struct proc) * max);
		bzero((char *)bucketaddr, sizeof(struct buckets) * max);
		bzero((char *)procbuckets, sizeof procbuckets);
		readmem((char *)&procaddr, Plist.s_value, sizeof(int));

		for(i = 0; i < max; i++)  {
			readmem((char *)&proctab[i], procaddr,
			    sizeof(struct proc));
			bucketaddr->index = i;
			bucketaddr->addr = (char *) procaddr;
			bucketaddr->next = procbuckets[HASH(procaddr,
			    PROCBUCKETS)];
			procbuckets[HASH(procaddr, PROCBUCKETS)] =
			    bucketaddr++;
			procaddr = (procaddr + sizeof(struct proc));
		}
	}
}


getindex(addr, buckethead, bucket)
	char *addr;
	struct buckets *buckethead[];
	int bucket;
{
	struct buckets *bp = buckethead[HASH(addr, bucket)];
	while((bp != NULL) && (bp->addr != addr)) {
		bp = bp->next;
	}
	return(bp ? bp->index : -1);
}


resync()
{

	initfile();
	initgnode();
	initmount();
	initbuf();
	initproc();
	

}
