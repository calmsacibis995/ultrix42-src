/*  sccsid  =  @(#)dsd_switch.h	4.1   (ULTRIX)   7/2/90  */

/*	.title Compilation Control Switches
**	.ident / 1.01 /
**
**
**	  File:	dsd_switch.h
** Description:	Compilation Control Switches
**	Author:	Paul Baker
**	  Date:	21-Feb-1986
**
**
**	Copyright 1986, Digital Equipment Corporation
**
**
**++
**	The definitions in this file control the compilation variants
**	for the DSD Table Access and DSD Services modules.
**--
*/


/* Constant definitions */

#define SW_TRUE   (1 == 1)
#define SW_FALSE  (0 == 1)


/* Control switches for the DSD Table Access module */

#define DSD_ACCESS_DEBUG	SW_FALSE
#define DSD_DUMP_TABLES		SW_FALSE

/* Control switches for the DSD Services module */

#define STD_SERVICES_DEBUG	SW_FALSE


/* Control switches for the DSD Test modules */

#define STD_DATA_DEBUG		SW_FALSE
#define DSD_BTT_DEBUG		SW_FALSE
