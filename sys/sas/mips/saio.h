#ident "$Header: saio.h,v 1.1 87/08/18 16:24:23 mdove Exp $"
/*		4.1	saio.h	*/

/*
 * Copyright 1985 by MIPS Computer Systems, Inc.
 */

/*
 * saio.h -- Header file for standalone package
 */

/*
 * EXSTKSZ -- sizeof stack needed by saio exception handling code
 * address of top of stack assumed to be in _fault_sp
 */
#define	EXSTKSZ	1024

#ifdef LANGUAGE_C
/*
 * device table is interface between monitor and device drivers
 */
struct device_table {
	char *dt_string;	/* device name */
	int (*dt_init)();	/* device init routine */
	int (*dt_open)();	/* device open routine */
	int (*dt_strategy)();	/* device strategy routine, returns cnt */
	int (*dt_close)();	/* device close routine */
	int (*dt_ioctl)();	/* device ioctl routine */
	int dt_type;		/* device "type" */
	int dt_fs;		/* file system type */
	char *dt_desc;		/* device description */
};
#endif LANGUAGE_C

/*
 * device types
 */
#define	DTTYPE_CHAR	0x1	/* character device */
#define	DTTYPE_CONS	0x2	/* can be console */
#define	DTTYPE_BLOCK	0x4	/* block device */
#define DTTYPE_RAW	0x8	/* raw device that uses fs switch */

/*
 * File structure types
 * NOTE: these are indices into fs_table in io.c and changes here
 * must be reflected in fs_table
 */
#define	DTFS_NONE	0	/* no file structure on device */
#define	DTFS_BFS	1	/* bfs protocol */
#define	DTFS_DVH	2	/* disk volume header */
#define	DTFS_TPD	3	/* boot tape directory */
#define DTFS_NCP	4	/* Network console protocol */
#define	DTFS_BSD42	5	/* 4.2 BSD file system */
#define	DTFS_SYSV	6	/* System V file system */
#define	DTFS_AUTO	-1	/* determined from partition table */

#ifdef LANGUAGE_C
struct fs_table {
	int (*fs_init)();	/* fs init routine */
	int (*fs_open)();	/* fs open routine */
	int (*fs_read)();	/* fs read routine, returns count */
	int (*fs_write)();	/* fs write routine, return count */
	int (*fs_ioctl)();	/* fs ioctl routine */
	int (*fs_close)();	/* fs close routine */
};
#endif LANGUAGE_C


/*
 * character device flags
 */
#define	DB_RAW		0x1	/* don't interpret special chars */
#define	DB_STOPPED	0x2	/* stop output */

/*
 * character device buffer
 * (prom equivalent of c-list)
 */
#define	CBUFSIZE	1024

#ifdef LANGUAGE_C
struct device_buf {
	int db_flags;		/* character device flags */
	char *db_in;		/* pts at next free char */
	char *db_out;		/* pts at next filled char */
	char db_buf[CBUFSIZE];	/* circular buffer for input */
};
#endif LANGUAGE_C

/*
 * Simple circular buffer functions
 */
#define	CIRC_EMPTY(x)	((x)->db_in == (x)->db_out)
#define	CIRC_FLUSH(x)	((x)->db_in = (x)->db_out = (x)->db_buf)
#define	CIRC_STOPPED(x)	((x)->db_flags & DB_STOPPED)

/*
 * iob.i_fs-tape and iob.i_ino-dir should be cast to the opproiate
 * structure tape in each file system or driver routine.
 * The size of these arrays a define below and should checked in the
 * init routine to make sure there is enough space available.
 */
#define IOB_INODE	300
#ifdef PROM
#define IOB_FS		512
#else
#define IOB_FS		8196
#endif PROM

/*
 * Io block: includes an
 * inode, cells for the use of seek, etc,
 * and a buffer.
 */
#ifdef LANGUAGE_C
struct	iob {
	int	i_flgs;		/* see F_ below */
	int	i_ctlr;		/* controller board */
	int	i_unit;		/* pseudo device unit */
	int	i_part;		/* disk partition */
	char	*i_ma;		/* memory address of i/o buffer */
	int	i_cc;		/* character count of transfer */
	off_t	i_offset;	/* seek offset in file */
	daddr_t	i_bn;		/* 1st block # of next read */
	int	i_fstype;	/* file system type */
	int	i_errno;	/* error # return */
	unsigned int	i_devaddr;	/* csr address */
	struct device_table *i_dp;	/* pointer into device_table */
	char	*i_buf;			/* i/o buffer for blk devs */
	char	i_ino_dir[IOB_INODE];	/* inode or disk/tape directory */
	char	i_fs_tape[IOB_FS];	/* file system or tape header */
};
#endif LANGUAGE_C

#ifndef NULL
#define NULL 0
#endif

/*
 * file flags
 */
#define F_READ		0x0001		/* file opened for reading */
#define F_WRITE		0x0002		/* file opened for writing */
#define	F_NBLOCK	0x0004		/* non-blocking io */
#define	F_SCAN		0x0008		/* device should be scanned */
#define	F_QIC11		0x0010		/* qic 11 device */
#define	F_QIC24		0x0020		/* qic 24 device */

/*
 * Request codes
 */
#define	READ	1
#define	WRITE	2

#define	DEVIOCTL(io, cmd, arg)	(*(io)->i_dp->dt_ioctl)(io, cmd, arg)
#define	DEVREAD(io)		(*(io)->i_dp->dt_strategy)(io, READ)
#define	DEVWRITE(io)		(*(io)->i_dp->dt_strategy)(io, WRITE)
