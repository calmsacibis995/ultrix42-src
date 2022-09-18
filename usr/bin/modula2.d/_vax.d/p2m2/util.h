(*#@(#)util.h	4.1	Ultrix	7/17/90 *)
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
$Header: util.h,v 1.3 84/05/19 11:43:34 powell Exp $
 ****************************************************************************)
(* util.h -- output file handling definitions and misc. *)

const
    TEMPFILE = 'p2m2.temp';

var
    infile, outfile : text;
    spaceFlag, outString, outFile : boolean;

procedure InitPass0; external;

procedure InitPass1; external;

procedure Pass2; external;

procedure OutChar(c : char); external;

procedure EchoChar(c : char); external;

procedure SetOutput(toFile, toString : boolean); external;
