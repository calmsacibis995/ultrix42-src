(*#@(#)stringtab.h	4.1	Ultrix	7/17/90 *)
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
$Header: stringtab.h,v 1.3 84/05/19 11:43:20 powell Exp $
 ****************************************************************************)
{ string table stuff }
const
    STRINGBLOCKSIZE = 10000;
    MAXSTRINGSIZE = 1000;
    STRINGHASHSIZE = 1357;
    SHORTSTRINGSIZE = 40;
    FILENAMESIZE = 100;

type
    StringIndex = 0..STRINGBLOCKSIZE;
    StringLength = 0..MAXSTRINGSIZE;
    StringBlock = ^ StringBlockRec;
    StringBlockRec = record;
	next : StringBlock;
	s : array [StringIndex] of char;
    end;
    StringEntry = ^ StringEntryRec;
    StringEntryRec = record
	block : StringBlock;
	index : StringIndex;
	length : StringLength;
	hash : integer;			{ hash value for quick comparisons }
	next : StringEntry;
    end;
    String = StringEntry;
    ShortString = array [1..SHORTSTRINGSIZE] of char;
    FileName = array [0..FILENAMESIZE] of char;

procedure InitStringTable; external;

function NewString : String; external;

procedure AddChar(c : char); external;

function GetChar(s : String; charNum : StringLength) : char; external;

procedure AddText(s : ShortString); external;

procedure AddString(str : String); external;

procedure WriteString(var f : text; s : String); external;

procedure WriteStringConst(var f : text; s : String); external;

procedure OutString(s : String); external;

procedure OutStringConst(s : String); external;

procedure StringToFileName(s : String; var fn : FileName); external;

procedure DumpStringTab; external;

function EqualAnyCase(a,b : String) : boolean; external;
