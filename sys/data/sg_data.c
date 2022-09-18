/*
 * static	char	*sccsid = "@(#)sg_data.c	4.1	(ULTRIX)	7/2/90";
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
 * sg_data.c
 *
 * Modification history:
 *
 * 24-May-89	darrell
 *	Changed the #include for cpuconf.h to find it in it's new home --
 *	sys/machine/common/cpuconf.h
 *
 *   8-Sep-86  -- rafiey (Ali Rafieymehr)
 *	Created this data file for VAXstar color.
 *	Derived from qd_data.c.
 *
 * 11-Aug-87 - Tim Burke
 *
 *	Added exec.h to list of include files for compatibility mode check 
 *	stored in the upper 4 bits of the magic number.
 *
 */

#include "sg.h"

#include "../machine/pte.h"	/* page table values */
#include "../machine/mtpr.h"	/* VAX register access stuff */

#include "../h/param.h" 	/* general system params & macros */
#include "../h/conf.h"		/* "linesw" tty driver dispatch */
#include "../h/dir.h"		/* for directory handling */
#include "../h/user.h"		/* user structure (what else?) */
#include "../io/uba/sgioctl.h"	/* ioctl call values */
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
#include "../machine/cpu.h" 	/* per cpu (pcpu) struct */
#include "../h/exec.h"

#include "../io/uba/pdma.h"
#include "../io/uba/ubareg.h"	/* uba & 'qba' register structs */
#include "../io/uba/ubavar.h"	/* uba structs & uba map externs */

#include "../io/uba/sguser.h"	/* definitions shared with my client */
#include "../io/uba/sgreg.h"	/* VAXstar(color)  device register structures */

/*
 * VAXstar(color) driver status flags for tracking operational state
 */

	struct sgflags {

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
	    u_short selmask;	    /* mask for active sg select entries */
	};

/* bit definitions for "inuse" entry  */

#define CONS_DEV	0x01
#define ALTCONS_DEV	0x02
#define GRAPHIC_DEV	0x04

/* bit definitions for 'mapped' member of flag structure */

#define MAPDEV		0x01		/* hardware is mapped */
#define MAPDMA		0x02		/* DMA buffer mapped */
#define MAPEQ		0x04		/* event queue buffer mapped */
#define MAPSCR		0x08		/* scroll param area mapped */
#define MAPCOLOR	0x10		/* color map writing buffer mapped */
#define MAPFIFO		0x20		/* FIFO buffer mapped */

/* bit definitions for 'selmask' member of sgflag structure */

#define SEL_READ	0x01		/* read select is active */
#define SEL_WRITE	0x02		/* write select is active */

/*
 * constants used in shared memory operations
 */

#define EVENT_BUFSIZE  1024	/* # of bytes per device's event buffer */

#define MAXEVENTS  ( (EVENT_BUFSIZE - sizeof(struct sginput))	 \
		     / sizeof(struct _vs_event) )

#define SG_DMA_BUFSIZ	(1024 * 3)
#define FIFO_BUFSIZ	(1024 * 3)

#define COLOR_BUFSIZ  ((sizeof(struct color_buf) + 512) & ~0x01FF)

/*******************************************************************/

#ifdef BINARY

	extern struct uba_device *sginfo[];  
					     /* uba structure  */
	extern struct tty sg_tty[];

/*	extern struct sg_softc sg_softc[];*/

	extern struct	sg_fifo_space {
		char	sg_pad[128*1024];
	};
/*
 * static storage used by multiple functions in this code
 */

	extern int Qbus_unmap[];	  /* Qbus mapper release key */
	extern struct sgflags sgflags;  /* VAXstar(color) device status flags */
	extern struct sgmap sgmap;	  /* VAXstar(color) register map structure */
	extern caddr_t sgbase;	  	/* base address of VAXstar(color) */
	extern struct buf sgbuf[];	  /* buf structs used by strategy */
	extern char one_only[]; 	  /* lock for single process access */

/*
 * shared memory allocation
 */

	extern char event_shared[];		 /* reserve event buf space */
	extern struct sginput *eq_header;	 /* event buf header ptrs */

	extern char FIFO_shared[];		  /* reserve FIFO buf space */
	extern struct FIFOreq_header *FIFOheader; /* FIFO buf header ptrs */

	extern char DMA_shared[];		  /* reserve DMA buf space */
	extern struct DMAreq_header *DMAheader; /* DMA buf header ptrs */

	extern char scroll_shared[];	/* reserve space for scroll structs */
	extern struct scroll *scroll; /* pointers to scroll structures */

	extern char color_shared[];	      /* reserve space: color bufs */
	extern struct color_buf *color_buf; /* pointers to color bufs */

/*
 * input event "select" use
 */

	extern struct proc *rsel;	/* process waiting for select */

	extern int FIFObuf_size;
	extern int DMAbuf_size;

/*
 * console cursor structure
 */

	struct _vs_cursor cursor;


/*********************************************************************/

#else

/*
 * reference to an array of "uba_device" structures built by the auto
 * configuration program.  The uba_device structure decribes the device
 * sufficiently for the driver to talk to it.  The auto configuration code
 * fills in the uba_device structures (located in ioconf.c) from user
 * maintained info.
 */

	struct uba_device *sginfo[NSG];  /* array of pntrs to each VAXstar's (color) */
					 /* uba structures  */
	struct tty sg_tty[NSG*4];	/* teletype structures for each.. */
					/* ..possible minor device */

	struct	sg_fifo_space {
		char	sg_pad[32867];
	};

/*	struct sg_softc sg_softc[NSG];*/

/*
 * static storage used by multiple functions in this code
 */

	int Qbus_unmap[NSG];		/* Qbus mapper release code */
	struct sgflags sgflags;		/* VAXstar(color) device status flags */
	struct sgmap sgmap; 	/* VAXstar(color) register map structure */
	caddr_t sgbase;			/* base address of VAXstar(color) */
	struct buf sgbuf[NSG];		/* buf structs used by strategy */
	char one_only[NSG];		/* lock for single process access */

/*
 * the array "event_shared[]" is made up of a number of event queue buffers
 * equal to the number of VAXstar color units configured into the running
 * kernel (NSG).
 * Each event queue buffer begins with an event queue header (struct sginput)
 * followed by a group of event queue entries (struct _vs_event).  The array
 * "*eq_header[]" is an array of pointers to the start of each event queue
 * buffer in "event_shared[]".
 */

#define EQSIZE ((EVENT_BUFSIZE * NSG) + 512)

	char event_shared[EQSIZE];	    /* reserve space for event bufs */
	struct sginput *eq_header;     /* event queue header pntrs */

/*
 * This allocation method reserves enough memory pages for NSG shared DMA I/O
 * buffers.  Each buffer must consume an integral number of memory pages to
 * guarantee that a following buffer will begin on a page boundary.  Also,
 * enough space is allocated so that the FIRST I/O buffer can start at the
 * 1st page boundary after "&DMA_shared".  Page boundaries are used so that
 * memory protections can be turned on/off for individual buffers.
 */

#define IOBUFSIZE  ((FIFO_BUFSIZ * NSG) + 512)

	char FIFO_shared[IOBUFSIZE];	    /* reserve I/O buffer space */
	struct FIFOreq_header *FIFOheader;  /* FIFO buffer header pntrs */

	char DMA_shared[IOBUFSIZE];	    /* reserve I/O buffer space */
	struct DMAreq_header *DMAheader;  /* DMA buffer header pntrs */

/*
 * The driver assists a client in scroll operations by loading dragon
 * registers from an interrupt service routine.	The loading is done using
 * parameters found in memory shrade between the driver and it's client.
 * The scroll parameter structures are ALL loacted in the same memory page
 * for reasons of memory economy.
 */

	char scroll_shared[2 * 512];	/* reserve space for scroll structs */
	struct scroll *scroll;		/* pointers to scroll structures */

/*
 * the driver is programmable to provide the user with color map write
 * services at VSYNC interrupt time.  At interrupt time the driver loads
 * the color map with any user-requested load data found in shared memory
 */

#define COLOR_SHARED  ((COLOR_BUFSIZ * NSG) + 512)

	char color_shared[COLOR_SHARED];      /* reserve space: color bufs */
	struct color_buf *color_buf;     /* pointers to color bufs */

/*
 * input event "select" use
 */

	struct proc *rsel; 	/* process waiting for select */

/*
 * console cursor structure
 */

	struct _vs_cursor cursor;


/************************************************************************/

	int nNSG = NSG;

	int FIFObuf_size = FIFO_BUFSIZ;

	int SG_DMAbuf_size = SG_DMA_BUFSIZ;

#endif

