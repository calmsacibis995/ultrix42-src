(*#@(#)scanner.h	4.1	Ultrix	7/17/90 *)
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
$Header: scanner.h,v 1.4 84/05/19 11:44:28 powell Exp $
 ****************************************************************************)
const
	LINESIZE = 1000;
type
    FileNumber = integer;
    Line = array [1..LINESIZE] of -128..127; { fool Pascal into using bytes }
    FilePtr = ^FileRec;
    FileRec = record
	next : FilePtr;
	name : String;
	number : FileNumber;
	lineNumber : integer;
	ptr, size : -1..LINESIZE;
	line : Line;
    end;



    TokenRec = record
	case kind : Token of
	    TKIDENT : (str : String);
	    TKSTRCONST, TKCHARCONST, TKCARDCONST, TKBOOLCONST, TKREALCONST
	   		 : (con : ConstNode);
    end;
    yylvalue = record
	case Token of
	    TKIDENT : (str : String);
	    TKSTRCONST, TKCHARCONST, TKCARDCONST, TKBOOLCONST, TKREALCONST
	   		 : (con : ConstNode);
    end;

var
    inFile : FilePtr;
    currToken : TokenRec;	{ current scanned token }
    upperCaseFlag : boolean;	{ convert all to upper case }
    readingFile : boolean;
    yylval : yylvalue;		{ place to save value of token }

procedure NextChar; external;

procedure NewLine; external;

procedure InitScanner(fn : String); external;

function InitFile(fn:String) : boolean; external;

procedure LineWrite; external;

function atof (var c : char) : real; external;
