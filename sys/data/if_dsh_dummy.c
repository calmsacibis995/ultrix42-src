/*
**	@(#)if_dsh_dummy.c	4.1	(ULTRIX)	10/10/90
*/
/************************************************************************
**                                                                      *
**        Copyright (c) Digital Equipment Corporation, 1990.            *
**        All Rights Reserved.  Unpublished rights reserved             *
**        under the copyright laws of the United States.                *
**                                                                      *
**        The software contained on this media is proprietary           *
**        to and embodies the confidential technology of                *
**        Digital Equipment Corporation.  Possession, use,              *
**        duplication or dissemination of the software and              *
**        media is authorized only pursuant to a valid written          *
**        license from Digital Equipment Corporation.                   *
**                                                                      *
**        RESTRICTED RIGHTS LEGEND   Use, duplication, or               *
**        disclosure by the U.S. Government is subject to               *
**        restrictions as set forth in Subparagraph (c)(1)(ii)          *
**        of DFARS 252.227-7013, or in FAR 52.227-19, as                *
**        applicable.                                                   *
**                                                                      *
*************************************************************************
**++
**  FACILITY:
**
**      Wide Area Network Device Drivers (WANDD) for ULTRIX
**
**  MODULE NAME:
**
**	if_dsh_dummy.c
**
**  ABSTRACT:
**
**      This module provides a 'dummy' DST32/DSH32 device driver to satisy
**      global references in ka630.c and ka420.c for uVAX-2000 and uVAX-3100
**      support.  When the WAN Device Drivers software is installed, this
**      module compiles down to a null object.  The routines provided are-
**
**	int    dsh_Probe              (reg);
**	int    dsh_Attach             (ui);
**
**  AUTHORS:
**
**      Tony Griffiths,  TaN Engineering (Australia)
**
**  CREATION DATE:
**
**	28-August-1990
**
**  MODIFICATION HISTORY:
**
**--
*/

# ifdef WDD
#       include "dsh.h"
#       if NDSH > 0
#               define DSH_DUMMY 0
#       else
#               define DSH_DUMMY 1
#       endif NDSH
# else
#       define DSH_DUMMY 1
# endif WDD

#if DSH_DUMMY > 0

/*
**
**  INCLUDE FILES
**
*/

#include "../h/types.h"
#include "../machine/pte.h"
#include "../h/map.h"
#include "../h/param.h"
#include "../h/kmalloc.h"
#include "../h/mbuf.h"
#include "../h/buf.h"
#include "../h/time.h"
#include "../h/proc.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/vmmac.h"
#include "../h/ioctl.h"
#include "../h/errno.h"
#include "../h/time.h"
#include "../h/ipc.h"
#include "../h/shm.h"

#include "../net/net/if.h"
#include "../net/net/netisr.h"
#include "../net/net/route.h"
#include "../net/netinet/in.h"
#include "../net/netinet/in_systm.h"
#include "../net/netinet/in_var.h"

#include "../machine/cpu.h"
#include "../machine/mtpr.h"
#include "../io/netif/if_uba.h"
#include "../io/uba/ubareg.h"
#include "../io/uba/ubavar.h"

/*
**
**  MACRO DEFINITIONS
**
*/

struct uba_device  *dsh_info_dummy[1];

#define FALSE   0
#define TRUE    1

/****************************
** Routines in this module **
****************************/

int     dsh_Probe_dummy(),  dsh_Attach_dummy();

/****************************
** Global data definitions **
****************************/

unsigned short dsh_std_dummy[] = { 0 };

struct  uba_driver dshdriver =
            { dsh_Probe_dummy, 0, dsh_Attach_dummy, 0, dsh_std_dummy,
             "dsh", dsh_info_dummy };


/*
**++
**  FUNCTION NAME:
**
**	int dsh_Probe_dummy (addr);
**
**  FUNCTIONAL DESCRIPTION:
**
**	This is the dummy probe() routine.  It ALWAYS return failure!
**
**  FORMAL PARAMETERS:
**
**      caddr_t addr        address of device registers in I/O page
**
**  IMPLICIT INPUTS:
**
**      cvec, numuba    global variables maintained by ULTRIX
**
**  IMPLICIT OUTPUTS:
**
**      This function returns the following values:
**
**      0                 device failed self-test/interrupt
**
**  SIDE EFFECTS:
**
**--
*/

int dsh_Probe_dummy (addr)

    caddr_t             addr;   /* address of device registers in I/O page */

{
    return (FALSE);             /* return failure ALWAYS */
}

/*
**++
**  FUNCTION NAME:
**
**	int dsh_Attach_dummy (ui);
**
**  FUNCTIONAL DESCRIPTION:
**
**	This routine should NEVER be called!!!
**
**  FORMAL PARAMETERS:
**
**      struct uba_device   *ui     pointer to Unit Information structure
**
**  IMPLICIT INPUTS:
**
**      none
**
**  IMPLICIT OUTPUTS:
**
**      none
**
**  FUNCTION VALUE:
**
**      This function ALWAYS returns 0 
**
**  SIDE EFFECTS:
**
**
**--
*/

int dsh_Attach_dummy (ui)

    struct uba_device  *ui;             /* pointer to dsh_info structure */
{
    return (FALSE);
}

#endif DSH_DUMMY

#undef DSH_DUMMY


