/*	@(#)ipc.h	4.4	ULTRIX	9/18/90	*/

/*
 *
 *   Modification history:
 *
 * 13-Aug-90 -- sekhar
 *	defined IPC_MMAP for both vax and mips
 *
 * 15-Jun-90 -- sekhar
 * 	Added IPC_MMAP for mapping device memory.
 *
 * 22 Dec 89 -- scott
 *	X/Open compliance changes
 *
 * 08 Nov 89 -- fran
 *	Change types in ipc_perm to reflect X/Open (XPG3).
 *
 * 15 Dec 88 -- jmartin
 *	SMP locking for shared memory data structures (cf. h/shm.h)
 *	Integration of v3.0 shared memory changes, i.e. KM_ALLOC of
 *	per-process shared memory structures.
 *
 * 01 Mar 85 -- depp
 *	New file derived from System V IPC
 *
 */

/* Common IPC Access Structure */
struct ipc_perm {
	uid_t	uid;	/* owner's user id */
	gid_t	gid;	/* owner's group id */
	uid_t	cuid;	/* creator's user id */
	gid_t	cgid;	/* creator's group id */
	mode_t	mode;	/* access modes */
	unsigned short seq; /* slot usage sequence number */
	long	key;	/* key */
};

/* Common IPC Definitions. */
/* Mode bits. */
#define	IPC_CREAT	0001000		/* create entry if key doesn't exist */
#define	IPC_EXCL	0002000		/* fail if key exists */
#define	IPC_NOWAIT	0004000		/* error if request must wait */
#if !defined(_POSIX_SOURCE)
#define	IPC_MMAP	0020000		/* entry used for memory mapping */
					/* device memory */
#define IPC_SYSTEM      0040000         /* share memory between kern/user */
#define	IPC_ALLOC	0100000		/* entry currently allocated */
#endif /* !defined(_POSIX_SOURCE) */

/* Keys. */
#define	IPC_PRIVATE	(long)0	/* private key */

/* Control Commands. */
#define	IPC_RMID	0	/* remove identifier */
#define	IPC_SET		1	/* set options */
#define	IPC_STAT	2	/* get options */

#if !defined(_POSIX_SOURCE)
#define ipcfree(perm) ((perm).mode = 0)
#endif /* !defined(_POSIX_SOURCE) */
