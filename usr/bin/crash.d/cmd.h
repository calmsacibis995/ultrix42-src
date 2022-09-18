
/*	@(#)cmd.h	@(#)cmd.h	4.3	(ULTRIX)	4/11/91	*/

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

int	c_arp(), c_block(), c_bufclean(), c_bufdirty(), c_bufempty();
int	c_buffer(), c_bufgp(), c_bufhdr(), c_bufbusy();
int	c_bufstats(), c_clntrpc(), c_cmap(), c_cpu(), c_crcheck();
int	c_cred(), c_crref(), c_dis(), c_dnlc();
int	c_ds(), c_dupreq(), c_export(), c_files(), c_fsdata(), c_ftext(), c_gnode(), c_gnofree();
int	c_gnolock(), c_gnorefs(), c_history(), c_inpcb();
int	c_kmalloc(), c_lock(), c_map(), c_mbuf(), c_mscp();
int	c_mntinfo(), c_mount(), c_namei(), c_nm(), c_od(), c_ofile(), c_pcblk(), c_ppte();
int	c_port(), c_proc(), c_proclock(), c_ps(), c_q(), c_quit();
int	c_resync(), c_rnode(), c_rpcxprt(), c_scs(), c_scsibus();
int	c_socket(), c_spt(), c_stack(), c_stat(), c_svcreq(), c_text();
int	c_tout(), c_trace(), c_ts(), c_tty(), c_gstats();
int	c_uarea(), c_udpdata(), c_ufile(), c_duphash(),c_bufhash(),c_sig();
int	c_presto();


struct	tsw	t[] = {
	"#",		c_q,		"	repeat last command",
	"#5",		c_q,		"	repeat command 5 (any number)",
	"#h",		c_q,		"	show history list",
	"!",		c_q,		"	escape to shell",
	"?",		c_q,		"	print this list of available commands",
	"arp",		c_arp,		"	arp table entries",
	"b",		c_buffer,	"	dump buffer contents",
	"block",	c_block,	NULL,
	"blocks",	c_block,	"	disk blocks of an inode",
	"buf",		c_bufhdr,	"	buffer headers",
	"buffer",	c_buffer,	"	buffer data",
	"bufgp",	c_bufgp,	NULL,
	"bufhdr",	c_bufhdr,	NULL,
	"bufbusy",	c_bufbusy, 	"	busy buffers",
	"bufdirty",	c_bufdirty, 	"	dirty buffers",
	"bufclean",	c_bufclean, 	"	clean cached buffers",
	"bufempty",	c_bufempty, 	"	empty buffers",
	"bufhash",	c_bufhash, 	"	buffer cache hash lists",
	"bufstats",	c_bufstats, 	"	buffer cache statistics",
	"c",		c_tout,		NULL,
	"cache",	c_bufgp,	"	buffers for a gnode",
	"call",		c_tout,		NULL,
	"callout",	c_tout,		"	callout table",
	"calls",	c_tout,		NULL,
	"client",	c_clntrpc,	"	dump rpc client table",
	"cmap",		c_cmap,		"	cmap entry dump - enter pfn",
	"cmap -i",	c_cmap,		"	cmap entry dump - enter cmap index",
	"cmap -a",	c_cmap,		"	cmap entry dump - enter cmap address",
	"cmap -h",	c_cmap,		"	cmap hash list dump - enter cmap index",
	"cmap -b",	c_cmap,		"	cmap hash list dump - enter blkno",
	"cpu",		c_cpu,		"	cpudata table",
	"crcheck",	c_crcheck,	"	check credential ref counts",
	"cred",		c_cred,		"	print contents of credential",
	"credref",	c_crref,	"	list references to a credential",
	"df",		c_fsdata, 	NULL,
	"dis",		c_dis,		"	disassemble object code",
	"dnlc",		c_dnlc,		"	display the dnlc cache contents",
	"ds",		c_ds,		"	data address namelist search",
	"dump",		c_od,		NULL,
	"dupreq",	c_dupreq,	"	display the rpc duplicate request list",
	"duphash",	c_duphash,	"	display the NFS dupreq hash list",
	"export",	c_export,	"	display the current nfs export list",
	"f",		c_files,	NULL,
	"file",		c_files,	"	file table",
	"files",	c_files,	NULL,
	"freet",	c_ftext,  	NULL,
	"fsdata",	c_fsdata, 	"	filesystem data",
	"g -",		c_gnode,	"	all gnodes",
	"g -",		c_gnode,	"	list of gnode slots (all): g - 1 4 8", 
	"g -amod",	c_gnode,	"	gnodes with exact mode: g -amod 20622",
	"g -fs",	c_gnode,	"	gnodes with file system:g -fs 2",
	"g -gid",	c_gnode,	"	gnodes with gid: g -gid 10",
	"g -gno",	c_gnode,	"	gnodes with g_number: g -gno 2",
	"g -hmod",	c_gnode,	"	gnodes with type mode: g -hmod 7000",
	"g -lmod",	c_gnode,	"	gnodes with permission mode: g -lmod 111",
	"g -maj",	c_gnode,	"	gnodes with major device: g -maj 9",
	"g -min",	c_gnode,	"	gnodes with minor device: g -min a",
	"g -uid",	c_gnode,	"	gnodes with uid: g -uid 412",
	"g",		c_gnode,	"	list of gnode slots (active only): g 1 5 7",
	"g",		c_gnode,	"	short form for active gnode table",
	"gfree",	c_gnofree,	"	free gnode slots",
	"glock",	c_gnolock,	"	locked gnode slots",
	"gref",		c_gnorefs,	"	find refs on a gnode slot",
	"gnode",	c_gnode,	"	active gnode table",
	"gstats",	c_gstats,	"	gnode cache statistics",
	"h",		c_q,		"	help on commands starting with letter: h g",
	"h",		c_q,		"	help",
	"history",	c_history,	"	get history",
	"hdr",		c_bufhdr,	NULL,
	"help",		c_q,		"	help",
	"inpcb",	c_inpcb,	"	display tcp or udp pcb chain",
	"k",		c_stack,	NULL,
	"kernel",	c_stack,	NULL,
	"kmalloc",	c_kmalloc,	"	display kmalloc pool stats",
	"lock",		c_lock,		"	display a smp lock trace",
	"m",		c_mount,	NULL,
	"map",		c_map,		"	resource maps - type map for list",
	"mbuf",		c_mbuf,		"	display mbufs on a socket",
	"mi",		c_mntinfo,	NULL,
	"mnt",		c_mount,	NULL,
	"mntinfo",	c_mntinfo,	"	remote mount information",
	"mount",	c_mount,	"	mount table (abbr: m) (-s option for short)",
	"mscp",		c_mscp,		"	mscp disk & tape subsystem traversal",
	"mscp -disk",	c_mscp,		"	mscp disk subsystem traversal",
	"mscp -tape",	c_mscp,		"	mscp tape subsystem traversal - tmscp",
	"mscp -config",	c_mscp,		"	mscp disk & tape subsystem configuration",
	"mscp -connb",	c_mscp,		"	display mscp connection block",
	"mscp -classb",	c_mscp,		"	display mscp class block",
	"mscp -unitb",	c_mscp,		"	display mscp unit block",
	"mscp -reqb",	c_mscp,		"	display mscp request block",
	"mscp -dtable", c_mscp,		"	display mscp disk unit table",
	"mscp -ttable", c_mscp,		"	display tmscp tape unit table",
	"mscp -devunit",c_mscp,		"	display unit block for major/minor",
	"nm",		c_nm,		"	name search",
	"namei",	c_namei,	"	display namei cache",
	"ofile",	c_ofile,	"	u.u_ofile dump",
	"od",		c_od,		"	dump symbol values",
	"p",		c_proc,		NULL,
	"pcb",		c_pcblk,	"	process control block",
	"ports",	c_port,		"	display port control blocks",
	"ports -ssp",	c_port,		"	display SSP port control blocks",
	"ports -msi",	c_port,		"	display MSI port control blocks",
	"ports -ci",	c_port,		"	display CI port control blocks",
	"ports -gvp",	c_port,		"	display GVP port control blocks",
	"ports -brief",	c_port,		"	display port blocks, in brief",
	"ppte",		c_ppte,		"	ptes associated with proc slot",
	"presto",	c_presto,	"	presto status",
	"proc -",	c_proc,		"	process table long listing",
	"proc -r",	c_proc,		"	runnable processes only",
	"proc",		c_proc,		"	process table",
	"proclock",	c_proclock,	"	show sleeplocks held by sleeping procs",
	"ps",		c_ps,		"	proc table summary with command lines",
	"q",		c_quit,		NULL,
	"quit",		c_quit,		"	exit",
	"r",		c_rnode,	NULL,
	"rd",		c_od,		NULL,
	"rnode",	c_rnode,	"	remote gnode fields",
	"s",		c_stack,	"	dump kernel stack for process",
	"scs",		c_scs,		"	System Communications Services (SCS)",
	"scs -cb",	c_scs,		"	SCS connection block structure",
	"scs -cib",	c_scs,		"	SCS connection info block structure",
	"scs -pb",	c_scs,		"	SCS path block structure",
	"scs -pib",	c_scs,		"	SCS path information block structure",
	"scs -sb",	c_scs,		"	SCS system block structure",
	"scs -sib",	c_scs,		"	SCS system information block structure",
	"SCSI",         c_scsibus, 	NULL,
	"scsi",         c_scsibus,	"	SCSI controller information",
	"scsi -target", c_scsibus,	"       SCSI target information",
	"scsi -devtab", c_scsibus,	"       SCSI devtab information",
	"scsi -trans",  c_scsibus,	"       SCSI transfer information",
	"scsi -cmd",    c_scsibus,	"       SCSI message/command data",
	"scsi -bbr",    c_scsibus,	"       SCSI Bad Block Replacement data",
	"scsi -error",  c_scsibus,	"       SCSI error information",
	"scsi -sii",    c_scsibus,	"       SCSI SII information",
	"scsi -dct",    c_scsibus,	"       SCSI DCT stats",
	"scsi -spin",   c_scsibus,	"       SCSI SPIN stats",
	"scsi -all",    c_scsibus,	"       all SCSIBUS informaton",
	"sock",		c_socket, 	NULL,
	"socket",	c_socket, 	"	sockets from file table",
        "spt",          c_spt,    	NULL, 	/*for debugging*/
	"stack",	c_stack,	NULL,
	"stat",		c_stat,		NULL,
	"stk",		c_stack,	NULL,
	"sum",		c_sig,		"	Quick summary of machine state",
	"sync",		c_resync,	"	resync internal tables with /dev/mem",
	"svcxprt",	c_rpcxprt,	"	print rpc server xprt structure",
	"svcreq",	c_svcreq,	"	kernel rpc request",
	"t",		c_trace,	NULL,
	"term",		c_tty,		NULL,
	"text",		c_text,		"	text table",
	"time",		c_tout,		NULL,
	"timeout",	c_tout,		NULL,
	"tout",		c_tout,		NULL,
	"trace -",	c_trace,	"	kernel trace with variables",
	"trace",	c_trace,	"	kernel trace of process",
	"ts",		c_ts,		"	text address namelist search",
	"tty -",	c_tty,		"	print tty structure with clists",
	"tty type",	c_tty,		NULL,
	"tty",		c_tty,		"	print tty structure",
	"txt",		c_text,		NULL,
	"u",		c_uarea,	"	user area",
	"u_area",	c_uarea,	NULL,
	"uarea",	c_uarea,	NULL,
	"ufile",	c_ufile,	"	user open file table",
	"user",		c_uarea,	"	user area",
	"udpdata",	c_udpdata,	"	kernel rpc udp input descriptor",
	"v",		c_rnode,	NULL,
	"vnode",	c_rnode,	NULL,
	"x",		c_text,		NULL,
	NULL,		NULL,	NULL
};

struct	prmode	prm[] = {
	"decimal",	DECIMAL,
	"dec",		DECIMAL,
	"d",		DECIMAL,
	"e",		DECIMAL,
	"octal",	OCTAL,
	"oct",		OCTAL,
	"o",		OCTAL,
	"directory",	DIRECT,
	"direct",	DIRECT,
	"dir",		DIRECT,
/*	"d",		DIRECT, */
	"write",	WRITE,
	"w",		WRITE,
	"character",	CHAR,
	"char",		CHAR,
	"c",		CHAR,
	"byte",		BYTE,
	"b",		BYTE,
	"longdec",	LDEC,
	"ld",		LDEC,
	"D",		LDEC,
	"longoct",	LOCT,
	"lo",		LOCT,
	"O",		LOCT,
	"hex",		HEX,
	"h",		HEX,
	"x",		HEX,
	"hexadec",	HEX,
	"hexadecimal",	HEX,
	"string", 	STRING,
	"s",		STRING,
	NULL,		NULL
} ;
