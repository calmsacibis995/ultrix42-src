/*	@(#)msdup_msg.h	4.1	(ULTRIX)	2/19/91	*/

/************************************************************************
 *									*
 *			Copyright (c) 1989 by				*
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
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/*
 *
 *   Facility:	Systems Communication Architecture
 *		Diagnostic/Utilities Protocol (DUP) Class Driver
 *
 *   Abstract:	This module contains structure definitions for DUP
 *		command/response messages.
 *
 *   Author:	Andrew J. Moskal	Creation Date:	dd-mmn-yyyy
 *
 *   Modification History:
 *
 */

/*
 *   Conditional check to include this header file only once.
 */
#ifndef MSDUP_msg.h
#define MSDUP_msg.h	0		/* Disable subsequent includes       */

/*
 *  Include prerequisite definitions
 */
#ifdef	KERNEL
#include "../io/sysap/msdup_pgm.h"	/* DUP program file/header defs      */
#include "../io/sysap/msdup_ioctl.h"	/* DUP I/O control function defs     */
#else	KERNEL
#include <io/sysap/msdup_pgm.h>		/* DUP program file/header defs      */
#include <io/sysap/msdup_ioctl.h>	/* DUP I/O control function defs     */
#endif	KERNEL


/*
 *  Local constants
 */
#define	MAX_SDI_SIZE		512	/* Maximum transfer per n-SDIs      */
#define	SDI_DATA_SIZE		22	/* SDI data area size               */

/*
 *  DUP command message opcodes
 */
#define	MSDUP_OP_GETDUST	0x01	/* Get DUST Status                   */
#define	MSDUP_OP_EXESUP		0x02	/* Execute Supplied Program          */
#define	MSDUP_OP_EXELCL		0x03	/* Execute Local Program             */
#define	MSDUP_OP_SEND		0x04	/* Send Data                         */
#define	MSDUP_OP_RECEIVE	0x05	/* Receive Data                      */
#define	MSDUP_OP_ABORT		0x06	/* Abort program                     */
#define	MSDUP_OP_SDI		0x07	/* Send Data Immediate               */

#define	MSDUP_OP_CMD		0x7F	/* Command specifier                 */
#define	MSDUP_OP_END		0x80	/* End Message Indicator             */

/*
 *  DUP command modifiers
 */
#define MSDUP_MD_NULL		0x00	/* No modifiers                      */

/*
 *  Execute Supplied/Local Program command modifiers
 */
#define	MSDUP_MD_STANDALONE	0x01	/* Allow Standalone                  */

/*
 *  Send Data Immediate command modifiers
 */
#define	MSDUP_MD_TOBECONT	0x08	/* To be continued                   */

/*
 *  DUP end (response) message status codes
 */
#define	MSDUP_EM_SUCCESS	0x00	/* Success                           */
#define	MSDUP_EM_INVCMD		0x01	/* Invalid command                   */
#define	MSDUP_EM_NOREGAVL	0x02	/* No region available               */
#define	MSDUP_EM_NOREGS		0x03	/* No region suitable                */
#define	MSDUP_EM_NOPRGM		0x04	/* Program not known                 */
#define	MSDUP_EM_LOADFAIL	0x05	/* Load failure                      */
#define	MSDUP_EM_STANDALONE	0x06	/* Standalone                        */
#define MSDUP_EM_HSTBUFACC	0x09	/* Host buffer access error          */

/*
 *  DUP program flags
 */
#define	MSDUP_PF_STANDALONE	0x01	/* Standalone                        */
#define	MSDUP_PF_OVERLAY	0x02	/* Overlay segment                   */
#define	MSDUP_PF_OVRWRT		0x04	/* Read/Write overaly segment        */
#define	MSDUP_PF_STDDIALOGUE	0x08	/* Standard Dialogue                 */
#define MSDUP_PF_NOCRLF		0x10	/* Pass-thru                         */
					/*  (Do not append CR/LF)            */

/*
 *  Standard DUP command timeout parameters
 */
#define	TICKS_PER_SECOND     100	/* Clock ticks per second  */
/* The following timeouts are in seconds. */
#define	MSDUP_TMO_GETDUST     10 	/* Get DUST Status         */
#define	MSDUP_TMO_EXESUP      30 	/* Execute Supplied Program*/
#define	MSDUP_TMO_ABORT	      10 	/* Abort Program           */
#define MSDUP_TMO_BABBLE      30 	/* Babble Timeout          */

/* The following timeouts are in clock ticks. */
#define MSDUP_TMO_RSRC_WT    100	/* Wait for resources.     */
#define MSDUP_TMO_RSRC	     300	/* Number of clock ticks 
					   to wait before giving up*/

/*
 * Priority at which sleep should occur. 
 * This should be interruptible by signals. [MP]
 */
#define MSDUP_SLEEP_PRI		PZERO+1

/*
 *  DUP response identifier (command reference number)
 */
#define MSDUP_RSPID u_long

#define MakeRSPID(dev, seq) ((((dev) & 0xFFFF)<<16) | ((seq) & 0xFFFF))
#define DevFromRSPID(rspid) (((rspid)>>16) & 0xFFFF)
#define SeqFromRSPID(rspid) ((rspid) & 0xFFFF)
/*
 *  DUP command/end (response) message offsets
 */
struct _msdup_cmd_hdr {
  MSDUP_RSPID	cmd_ref;		/* Command reference number          */
  u_long	: 32;			/* Reserved                          */
  u_char	opcode : 7;		/* Opcode                            */
  u_char	em : 1;			/* End message?			     */
  u_char	:  8;			/* Reserved                          */
  u_short	modifier;		/* Command modifiers                 */
};
#define MSDUP_cmd_rspid		header.cmd_ref
#define MSDUP_cmd_opcode	header.opcode
#define MSDUP_cmd_em		header.em
#define MSDUP_cmd_modifier	header.modifier

typedef struct _msdup_getdust_cmd {
  struct _msdup_cmd_hdr	header;
} MSDUP_GETDUST_CMD;

typedef struct _msdup_exesup_cmd {
  struct _msdup_cmd_hdr	header;
  u_long		ini_size;	/* Header/Initial segment size       */
  BHANDLE		ini_lbhandle;	/* Initial segment buffer descriptor */
  BHANDLE		ovr_lbhandle;	/* Overlay segment buffer descriptor */
} MSDUP_EXESUP_CMD;

typedef struct _msdup_exelcl_cmd {
  struct _msdup_cmd_hdr	header;
  u_char		pgm_nam[ PGM_NAME_SIZE ];
} MSDUP_EXELCL_CMD;

typedef struct _msdup_srd_cmd {
  struct _msdup_cmd_hdr	header;
  u_long		byte_cnt;	/* Byte count                        */
  BHANDLE		dat_lbhandle;	/* Data buffer descriptor            */
} MSDUP_SRD_CMD;

typedef struct _msdup_abort_cmd {
  struct _msdup_cmd_hdr	header;
} MSDUP_ABORT_CMD;

typedef struct _msdup_sdi_cmd {
  struct _msdup_cmd_hdr	header;
  u_short		byte_cnt;	/* Byte count                        */
  u_char		data[ SDI_DATA_SIZE ];
					/* Data                              */
} MSDUP_SDI_CMD;


struct _msdup_em_hdr {
  MSDUP_RSPID	cmd_ref;		/* Command reference number           */
  u_long	: 32;	/* Reserved                           */
  u_char	endcode : 7;		/* Endcode (Opcode)                   */
  u_char	em : 1;			/* Set for an end message.	      */
  u_char	:  8;	/* Reserved                           */
  u_short	status;			/* Command status                     */
};
#define MSDUP_em_rspid		header.cmd_ref
#define MSDUP_em_em		header.em
#define MSDUP_em_endcode	header.endcode
#define MSDUP_em_status		header.status

typedef struct _msdup_getdust_em {
  struct _msdup_em_hdr	header;
  MSDUP_DUST		dust;
} MSDUP_GETDUST_EM;

typedef struct _msdup_exesup_em {
  struct _msdup_em_hdr	header;
} MSDUP_EXESUP_EM;

typedef struct _msdup_exelcl_em {
  struct _msdup_em_hdr	header;
  u_short		version;	/* Program version number            */
  u_char		xfer_tmo;	/* Send/Receive Data, SDI timeout    */
  MSDUP_PGMFLG		flags;		/* Program flags                     */
  u_char		abort_tmo;	/* Abort timeout                     */
} MSDUP_EXELCL_EM;

typedef struct _msdup_srd_em {
  struct _msdup_em_hdr	header;
  u_long	byte_cnt;	/* Byte count                        */
} MSDUP_SRD_EM;

typedef struct _msdup_abort_em {
  struct _msdup_em_hdr	header;
} MSDUP_ABORT_EM;

typedef struct _msdup_sdi_em {
  struct _msdup_em_hdr	header;
} MSDUP_SDI_EM;

typedef union {
  MSDUP_GETDUST_CMD	gd_cmd;
  MSDUP_EXESUP_CMD	exesup_cmd;
  MSDUP_EXELCL_CMD	exelcl_cmd;
  MSDUP_SRD_CMD		srd_cmd;
  MSDUP_ABORT_CMD	abt_cmd;
  MSDUP_SDI_CMD		sdi_cmd;
  MSDUP_GETDUST_EM	gd_em;
  MSDUP_EXESUP_EM	exesup_em;
  MSDUP_EXELCL_EM	exelcl_em;
  MSDUP_SRD_EM		srd_em;
  MSDUP_ABORT_EM	abt_em;
  MSDUP_SDI_EM		sdi_em;
} MSDUP_MSG;

#define MSDUP_cmd_ref		gd_cmd.header.cmd_ref
#define MSDUP_opcode		gd_cmd.header.opcode
#define MSDUP_modifier		gd_cmd.header.modifier
#define MSDUP_ini_lbhandle	exesup_cmd.ini_lbhandle
#define MSDUP_ovr_lbhandle	exesup_cmd.ovr_lbhandle
#define MSDUP_pgm_nam		exelcl_cmd.pgm_nam
#define MSDUP_srd_byt_cnt	srd_cmd.byte_cnt
#define MSDUP_srd_lbhandle	srd_cmd.dat_lbhandle
#define MSDUP_sdi_byt_cnt	sdi_cmd.byte_cnt
#define MSDUP_sdi_dat		sdi_cmd.data
#define MSDUP_em		gd_em.header.em
#define MSDUP_endcode		gd_em.header.endcode
#define MSDUP_status		gd_em.header.status
#define MSDUP_dust		gd_em.dust
#define MSDUP_pgm_ver		exelcl_em.version
#define MSDUP_data_tmo		exelcl_em.xfer_tmo
#define MSDUP_pgm_flgs		exelcl_em.flags
#define MSDUP_abort_tmo		exelcl_em.abort_tmo


#endif	MSDUP_msg.h
