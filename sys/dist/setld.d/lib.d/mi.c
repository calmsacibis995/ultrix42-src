/*
 *			Copyright (c) 1989 by
 *		Digital Equipment Corporation, Maynard, MA
 *			All rights reserved.
 *								
 *	This software is furnished under a license and may be used and
 *	copied  only  in accordance with the terms of such license and
 *	with the  inclusion  of  the  above  copyright  notice.   This
 *	software  or  any  other copies thereof may not be provided or
 *	otherwise made available to any other person.  No title to and
 *	ownership of the software is hereby transferred.		
 *								
 *	The information in this software is subject to change  without
 *	notice  and should not be construed as a commitment by Digital
 *	Equipment Corporation.					
 *								
 *	Digital assumes no responsibility for the use  or  reliability
 *	of its software on equipment which is not supplied by Digital.
 *
 *	000	07-mar-1989	ccb
 *	New.
 *	001	14-jun-1989	ccb
 *		Type clean-up wrt setld.h
*/

/*!	Contructors, Recognizers, Operators
*/

#ifndef lint
static	char *sccsid = "@(#)mi.c	4.1	(ULTRIX)	7/2/90";
#endif lint

#include	<sys/param.h>
#include	<sys/dir.h>
#include	<stdio.h>
#include	"setld.h"

/*LINTLIBRARY*/

#define	MIRECMAX	(sizeof(PathT)+(sizeof(NameT)*2)+2)

static FILE	*MiTToFILE();

/*	MiT	*MiInit() -
 *		convert FILE to MiT
 *
 *	given:	FILE * p - a FILE pointer
 *	does:	converts it to an MiT
 *	return:	a pointer to the MiT, NULL if none available.
 *
 *	note:	currently does nothing but cast
*/

MiT *MiInit(p)
FILE *p;
{
	return( (MiT *) p );
}



/*	MiRecT	*MiRead() -
 *		read a master inventory record
 *
 *	given:	MiT *p - a pointer to a master inventory
 *	does:	read one record from the inventory
 *	return:	NULL on EOF, else a (static) MiRecT containing the information
 *		from the record.
*/

MiRecT *MiRead(p)
MiT *p;
{
	static MiRecT	r;			/* static record */
	static char	buf[MIRECMAX+1];	/* input buffer */
	static NameT	flgtmp;			/* tmp flag buffer */

	if( fgets( buf, sizeof(buf), MiTToFILE(p) ) == NULL )
		return(NULL);

	(void) sscanf( buf, "%s\t%s\t%s\n", flgtmp, r.mi_path, r.mi_subset );

	/* attempt to extract a hex number from the flgtmp string. this
	 *  is done in two phases so the the first sscanf doesn't choke
	 *  on old style inventories.
	*/
	(void) sscanf( flgtmp, "%x", &r.mi_flags );
	return( &r );
}


/*	FILE	*MiTToFILE() -
 *		xlate MiT * to FILE *
*/

static FILE *MiTToFILE(p)
FILE *p;
{
	return( (MiT *) p );
}

