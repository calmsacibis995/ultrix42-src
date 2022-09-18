(*#@(#)globals.h	4.1	Ultrix	7/17/90 *)
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
$Header: globals.h,v 1.3 84/05/19 11:42:23 powell Exp $
 ****************************************************************************)
{ Definitions that are included everywhere }

{ debugging flags }
type
    DebugFlags = 'a'..'z';
var
    debugSet : set of DebugFlags;
    standardFlag : boolean;

{ printing indentation per level }
const
    INDENT = 4;
var
    TAB : char;

type
    ConstNode = ^ConstNodeRec;
    DataType = (DTNULL, DTPOINTER, DTRECORD, DTARRAY, DTINTEGER, DTBOOLEAN,
		DTCHAR, DTRENAME, DTSTRING, DTREAL, DTSET, DTCARDINAL,
		DTSUBRANGE, DTENUMERATION, DTPROC, DTWORD, DTANY);
    ConstNodeRec = record
	next : ConstNode;
	case kind : DataType of
	    DTSTRING : (strVal : String);
	    DTCHAR : (charVal : char);
	    DTINTEGER,
	    DTCARDINAL : (cardVal : String);
	    DTBOOLEAN : (boolVal : boolean);
	    DTREAL : (realVal : String);
	end;

procedure exit(status : integer); external;

var
    codeFileName : FileName;
    mainFileName : String;

    currLine : integer;
    currFile : String;

    sourceFileType: (UNKNOWNFILE, IMPLFILE, DEFNFILE, PROGFILE);
    moduleName: String;
    
    

