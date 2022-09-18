(*#@(#)error.h	4.1	Ultrix	7/17/90 *)
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
$Header: error.h,v 1.3 84/05/19 11:38:32 powell Exp $
 ****************************************************************************)
const
    ERRORSTRINGSIZE = 100;
type
    ErrorString = array [1..ERRORSTRINGSIZE] of char;

var
    numberOfErrors : integer;
    stringDataType : array [DataType] of String;
    stringToken : array [Token] of String;
    stringSymbolKind : array [SymbolKind] of String;
    stringExprKind : array [SymbolKind] of String;
    stringStmtKind : array [SymbolKind] of String;

procedure InitError; external;

procedure Error(msg : ErrorString); external;

procedure ErrorName(name : String; msg : ErrorString); external;

procedure ErrorNumber(msg : ErrorString; number : integer); external;

procedure ExprError(expr : ExprNode; msg : ErrorString); external;

procedure ExprErrorName(expr : ExprNode; name : String; msg : ErrorString);
	external;

procedure ExprErrorNumber(expr : ExprNode; msg : ErrorString; number : integer);
	external;

procedure ExprErrorNameNumber(expr : ExprNode; name : String; msg : ErrorString;
	number : integer); external;

procedure StmtError(stmt : StmtNode; msg : ErrorString); external;

procedure StmtErrorName(stmt : StmtNode; name : String; msg : ErrorString);
	external;
