/*	@(#)scsi.h	4.1	(ULTRIX)	7/17/90	*/

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

#define FORMAT

#ifdef vax
#include        <io/scsi/vax/scsivar.h>
#include        <io/scsi/vax/scsireg.h>
#endif vax
#ifdef mips
#include        <io/scsi/mips/scsivar.h>
#include        <io/scsi/mips/scsireg.h>
#endif mips

#define EQUAL_MATCH 0
#define OR_MATCH 1

int
	scsi_read_softc();
char
	*scsi_decode();
void
	do_scsiprint_trans(),
	do_scsiprint_dctstats(),
	do_scsiprint_spinstats(),
	do_scsiprint_sii(),
	do_scsiprint_err(),
	do_scsiprint_bbr(),
	do_scsiprint_cmd(),
	do_scsiprint_devtab(),
	do_scsiprint_targ(),
	do_scsiprint_cntlr();

