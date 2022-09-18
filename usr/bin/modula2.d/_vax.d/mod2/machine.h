(*#@(#)machine.h	4.1	Ultrix	7/17/90 *)
(****************************************************************************
 *									    *
 *  Copyright (c) 1984 by						    *
 *  DIGITAL EQUIPMENT CORPORATION, Maynard, Massachusetts.		    *
 *  All rights reserved.						    *
 * 									    *
 *  This software is furnished under a license and may be used and copied   *
 *  only in  accordance with  the  terms  of  such  license  and with the   *
 *  inclusion of the above copyright notice. This software or  any  other   *
 *  copies thereof may not be provided or otherwise made available to any   *
 *  other person.  No title to and ownership of  the  software is  hereby   *
 *  transferred.							    *
 * 									    *
 *  The information in this software is  subject to change without notice   *
 *  and  should  not  be  construed as  a commitment by DIGITAL EQUIPMENT   *
 *  CORPORATION.							    *
 * 									    *
 *  DIGITAL assumes no responsibility for the use  or  reliability of its   *
 *  software on equipment which is not supplied by DIGITAL.		    *
 * 									    *
$Header: machine.h,v 1.4 84/05/19 11:41:28 powell Exp $
 ****************************************************************************)
const
    WORDSIZE = 32;
    BYTESIZE = 8;
    CHARSIZE = 8;
    BOOLEANSIZE = 8;
    MAXINT = 2147483647;
    MAXCARD = 4294967295.0;
    MINCHAR = 0;
    MAXCHAR = 127;
    BACKSPACECHAR = 8;
    TABCHAR = 9;
    LINEFEEDCHAR = 10;
    FORMFEEDCHAR = 12;
    RETURNCHAR = 13;
type
    cardinal = real;	{ easy way to get 48-bit "integers" }
    character = 0..255;	{ allow 8-bit characters }
    rawcharacter = -128..127;	{ Pascal range for a byte }

function CardDiv(a,b : cardinal) : cardinal; external;
function CardMod(a,b : cardinal) : cardinal; external;
