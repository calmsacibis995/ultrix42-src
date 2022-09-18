/*
 * static	char	*sccsid = "@(#)fg_data.c	4.1	(ULTRIX)	7/2/90";
 */

/************************************************************************
 *									*
 *			Copyright (c) 1985, 1986 by			*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any	other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or	reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/*
 * fg_data.c
 *
 * Modification history:
 *
 *	Changed #include of cpuconf.h to find it in it's new home --
 *	sys/machine/common/cpuconf.h
 *
 * 23-Sep-88 -- carito (Allen Carito)
 *      Added cvax.h header file inclusion and rearragend cpu.h header file
 *      to preceed cvax.h
 *
 * xx-xxx-87 -- 
 *      created.
 */

#include "fg.h"

#include "../machine/pte.h"	/* page table values */
#include "../machine/mtpr.h"	/* VAX register access stuff */

#include "../h/param.h" 	/* general system params & macros */
#include "../h/conf.h"		/* "linesw" tty driver dispatch */
#include "../h/dir.h"		/* for directory handling */
#include "../h/user.h"		/* user structure (what else?) */
#include "../io/uba/fgioctl.h"	/* ioctl call values */
#include "../h/tty.h"
#include "../h/map.h"		/* resource allocation map struct */
#include "../h/buf.h"		/* buf structs */
#include "../h/vm.h"		/* includes 'vm' header files */
#include "../h/bk.h"		/* BKINPUT macro for line stuff */
#include "../h/clist.h" 	/* char list handling structs */
#include "../h/file.h"		/* file I/O definitions */
#include "../h/uio.h"		/* write/read call structs */
#include "../h/kernel.h"	/* clock handling structs */
#include "../../machine/common/cpuconf.h"
#include "../h/proc.h"
#include "../h/ipc.h"
#include "../h/shm.h"
#include "../h/devio.h"
#include "../machine/ka60.h"
#include "../h/exec.h"
#include "../io/uba/pdma.h"
#include "../io/uba/ubareg.h"	/* uba & 'qba' register structs */
#include "../io/uba/ubavar.h"	/* uba structs & uba map externs */
#include "../machine/cpu.h" 	/* per cpu (pcpu) struct */
#include "../machine/cvax.h"
#include "../io/uba/fguser.h"	/* definitions shared with my client */
#include "../io/uba/fgreg.h"	/* VAXstar(color)  device register structures */



/*
 * VAXstar(color) driver status flags for tracking operational state
 */

	struct fgflags {

	    u_int inuse;	    /* which minor dev's are in use now */
	    u_int config;	    /* I/O page register content */
	    u_int mapped;	    /* user mapping status word */
	    u_int kernel_loop;	    /* if kernel console is redirected */
	    u_int user_fifo;	    /* FIFO from user space in progress */
	    u_short pntr_id;	    /* type code of pointing device */
	    u_short duart_imask;    /* shadowing for duart intrpt mask reg */
	    u_short adder_ie;	    /* shadowing for adder intrpt enbl reg */
	    u_short curs_acc;	    /* cursor acceleration factor */
	    u_short curs_thr;	    /* cursor acceleration threshold level */
	    u_short tab_res;	    /* tablet resolution factor */
	    u_short selmask;	    /* mask for active fg select entries */
	};

/* bit definitions for "inuse" entry  */

#define CONS_DEV	0x01
#define ALTCONS_DEV	0x02
#define GRAPHIC_DEV	0x04

/* bit definitions for 'mapped' member of flag structure */

#define MAPEQ		0x04		/* event queue buffer mapped */

/* bit definitions for 'selmask' member of fgflag structure */

#define SEL_READ	0x01		/* read select is active */
#define SEL_WRITE	0x02		/* write select is active */

/*
 * constants used in shared memory operations
 */

#define EVENT_BUFSIZE  1024	/* # of bytes per device's event buffer */

#define MAXEVENTS  ( (EVENT_BUFSIZE - sizeof(struct fginput))	 \
		     / sizeof(struct _vs_event) )

#define	COMM_SIZE	70 * 1024	/* Size of common area in bytes */

/*******************************************************************/

#ifdef BINARY

        extern struct tty fg_tty[];
	extern struct fgflags fgflags;  /* Firefox device status flags */
        extern struct fgmap fgmap;        /* Firefox register map structure */
        extern caddr_t fgbase;          /* base address of Firefox legss */

/*
 * shared memory allocation
 */

	extern char event_shared[];		 /* reserve event buf space */
	extern struct fginput *eq_header;	 /* event buf header ptrs */

        extern char comm_shared[];           /* reserve space: common area */

/*
 * console cursor structure
 */

	struct _vs_cursor cursor;



/*********************************************************************/

#else

        struct tty fg_tty[NFG*4];       /* teletype structures for each.. */
                                        /* ..possible minor device */
	struct fgflags fgflags;		/* Firefox device status flags */
        struct fgmap fgmap;     /* Firefox legss register map structure */
        caddr_t fgbase;                 /* base address of Firefox legss */

/*
 * the array "event_shared[]" is made up of a number of event queue buffers
 * equal to the number of Firefox units configured into the running
 * kernel (NFG).
 * Each event queue buffer begins with an event queue header (struct sginput)
 * followed by a group of event queue entries (struct _vs_event).  The array
 * "*eq_header[]" is an array of pointers to the start of each event queue
 * buffer in "event_shared[]".
 */

#define EQSIZE ((EVENT_BUFSIZE * NFG) + 512)

	char event_shared[EQSIZE];	    /* reserve space for event bufs */
	struct fginput *eq_header;     /* event queue header pntrs */

/* Common area (shared memory) */

        char comm_shared[COMM_SIZE];      /* reserve space: common area */


/*
 * console cursor structure
 */

	struct _vs_cursor cursor;




/************************************************************************/

	int nNFG = NFG;

#endif

