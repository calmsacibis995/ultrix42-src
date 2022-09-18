/*	@(#)conf.h	4.2	(ULTRIX)	9/10/90	*/

/************************************************************************
 *									*
 *			Copyright (c) 1984 by				*
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
/************************************************************************
 *
 *			Modification History
 *
 *      30-Aug-90       skc
 *      Added shadow data structure shaddevt.
 *
 * 	09-Nov-89 jaw
 *	Make asymmetric driver macro's faster for single cpu.
 *
 *	16-Oct-89	bp
 *	Added field sw_nio to structure swdevt for collecting swap I/O.
 *
 *	02-Mar-87	Suzanne Logcher  -- logcher
 *	Merged in diskless changes, added swap device data
 *	structure, flags and macros.
 *
 *	23-Jul-86	Paul Shaughnessy -- prs
 *	Added the structure definition for genericconf.
 *
 *	11-Mar-86	Larry Palmer -- lp
 * 	Added strategy routine to cdevsw for n-buffered I/O. If
 *	the strategy routine is non-null this implies device can do
 *	multiple bufferring.
 *
 *	13-Sep-84	Stephen Reilly
 * 001- Add ioctl calls to the bdevsw
 *
 ***********************************************************************/
/*	conf.h	6.1	83/07/29	*/

/*
 * Declaration of block device
 * switch. Each entry (row) is
 * the only link between the
 * main unix code and the driver.
 * The initialization of the
 * device switches is in the
 * file conf.c.
 */
struct bdevsw
{
	int	(*d_open)();
	int	(*d_close)();
	int	(*d_strategy)();
	int	(*d_dump)();
	int	(*d_psize)();
	int	d_flags;
	int	(*d_ioctl)();				/* 001 */
	int	d_affinity;
};
#ifdef KERNEL
extern struct	bdevsw bdevsw[];
#endif /* KERNEL */

/*
 * Character device switch.
 */
struct cdevsw
{
	int	(*d_open)();
	int	(*d_close)();
	int	(*d_read)();
	int	(*d_write)();
	int	(*d_ioctl)();
	int	(*d_stop)();
	int	(*d_reset)();
	struct tty *d_ttys;
	int	(*d_select)();
	int	(*d_mmap)();
	int	(*d_strat)();
	int	d_affinity;
};

#define CALL_TO_NONSMP_DRIVER(pswitch,saveaffinity) {\
	if (smp) \
		if (pswitch.d_affinity == boot_cpu_mask) \
			saveaffinity = switch_affinity(boot_cpu_mask); \
}

#define RETURN_FROM_NONSMP_DRIVER(pswitch,saveaffinity) {\
	if (smp) \
		if (pswitch.d_affinity == boot_cpu_mask) \
			(void) switch_affinity(saveaffinity);\
}




#ifdef KERNEL
extern struct	cdevsw cdevsw[];
#define AUD_NO  69
#endif /* KERNEL */

/*
 * tty line control switch.
 */
struct linesw
{
	int	(*l_open)();
	int	(*l_close)();
	int	(*l_read)();
	int	(*l_write)();
	int	(*l_ioctl)();
	int	(*l_rint)();
	int	(*l_rend)();
	int	(*l_meta)();
	int	(*l_start)();
	int	(*l_modem)();
};
#ifdef KERNEL
extern struct	linesw linesw[];
#endif /* KERNEL */

/*
 * Swap device information
 */
struct swdevt
{
        dev_t   sw_dev;
        int     sw_freed;
        int     sw_nblks;
        int     sw_type;
        int *sw_gnodeptr;       /* gnode pointer */
        int (*sw_strat)( /* bp */ );
        int sw_priority;
	long	sw_nio;
 };
#define sw_gptr                 sw_gnodeptr
#define sw_pri                  sw_priority

/* SW_INIT initializes a given type of swap area */
/* #define SW_INIT(swp)            ((swp)->sw_dep->sw_init)()) */

/*
 * SW_IOSTRAT is the lowest level strategy routine.
 *      sw_type == SW_RAW       it's the block device strategy
 *      sw_type == SW_NFS       it's the NFS strategy routine
 *      sw_type == SW_UFS       WHO KNOWS at this time ?
 */
#define SW_IOSTRAT(swp,bp)      ((swp)->sw_strat)(bp)

/*
 *      Swap Types
 */
#define SW_RAW          0       /* Swap to local raw dev (original flavor)*/
#define SW_NFS          1       /* Swap to NFS remote file */
#define SW_UFS          2       /* Swap to UFS local file */

#ifdef KERNEL
extern struct	swdevt swdevt[];
#endif /* KERNEL */

/*
 * Generic configuration table
 */
struct genericconf
{
	caddr_t	gc_driver;
	char	*gc_name;
	dev_t	gc_root;
	int	gc_BTD_type;
};
#ifdef KERNEL
extern struct genericconf genericconf[];
#endif /* KERNEL */

/*
 * Shadow device information
 */
#define SHAD_CONST_MAX  8
struct shaddevt {
    dev_t   shad_devt;
    int     num_of_const;
    dev_t   constituents[SHAD_CONST_MAX];
}; 
