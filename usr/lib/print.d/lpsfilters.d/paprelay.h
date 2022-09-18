/* @(#)paprelay.h	4.1      ULTRIX 	11/15/90 */
/*
 * file:		paprelay.h
 *
 * History:
 *  4.0-04  27-Jul-90	jdj - Change RELAY_MAX_FRAME back to 1024;  Added
 *			DECNET_MAX_FRAME to support the clients smaller frame.
 *	    13-Jul-90	mva - Change RELAY_MAX_FRAME t0 960 (was 1024)
 *  4.0-03  11-Jul-90	jdj - Moved RelayMsg to psds.h
 *  4.0-02  26-Mar-90	jdj - Add connect type
 *  4.0-02  01-Mar-90	jdj - Change to PAP nomenclature
 *  4.0-01  16-Feb-90	jdj - Add RelayMsg datatype
 *  1.0-00  06-Jan-88	Christopher A. Kent - Originally called relay.h
 *
 *	 This  document contains proprietary information  of Digital
 *	 Equipment Corporation (DIGITAL). This information shall not
 *	 be disclosed  to  persons outside  the  employ  of  DIGITAL
 *	 except  by  DIGITAL personnel so authorized  by DIGITAL and
 *	 only  for  use  by  such  other  persons   in  the  design,
 *	 production, or manufacture of products for DIGITAL.
 *
 *	  Copyright (c) 1988 - 1990 by Digital Equipment Corporation
 *
 *		   PostScript is by Adobe Systems, Inc.
 */

#ifndef	RELAY_H

typedef struct _RelayLine{
    char	*opCode;
    char	*ID;
    char	*length;
    char	*data;
} RelayLine, *RelayPtr;

typedef struct _RelayRec{
    char	*opCode;
    int		ID;
    int		length;
    char	*data;
} RelayRec, *RelayRecPtr;

#define	RELAY_MAX_FRAME	1024		/* biggest valid frame */
#define DECNET_MAX_FRAME 960		/* frame size used by DECnet client */
#define	RELAY_ARG_SEP	'\001'		/* internal data separator */
#define	RELAY_SOM	'\002'		/* start of frame */

/* connect types */

#define TCP_CONNECT	1
#define DECNET_CONNECT  2
#define LAPS_CONNECT	3

/* opCode enumeration */

#define	OP_NULL		"0"		/* do nothing */
#define	OP_SSN		"1"		/* start of session */
#define	OP_EOJ		"2"		/* end of job */
#define	OP_SOD		"3"		/* start new document */
#define	OP_EOD		"4"		/* end of document */
#define	OP_DATA		"5"		/* PDL data */
#define	OP_KILL		"6"		/* kill current job and session */
#define	OP_SOJ		"7"		/* start of job */
#define	OP_EOF		"8"		/* advise of imminent close */
#define	OP_FLUSH	"9"		/* performance hack */
#define OP_SHOW		"A"		/* show printer status */

#define	OP_MSSN		"41"		/* start management session */
#define	OP_TIME		"42"		/* set time and date */
#define	OP_ACCT		"43"		/* accounting record */
#define	OP_EMSG		"44"		/* error message */

#define	OP_CSSN		"45"		/* start console session */

#define	OP_OPEN		"50"		/* open remote file */
#define	OP_READ		"51"		/* read remote file */
#define	OP_WRITE	"52"		/* write remote file */
#define	OP_CLOSE	"53"		/* close remote file */

#define OP_FINDFONT	"56"		/* return path to named font */

#define	OP_REPLY	"101"		/* reply */
#define	OP_PREPLY	"102"		/* preliminary reply */
#define	OP_NAK		"103"		/* service not available */
#define OP_STATUS	"104"		/* returnstatus message */
#define OP_MSG		"105"		/* status message */

#define	RELAY_H
#endif	RELAY_H
