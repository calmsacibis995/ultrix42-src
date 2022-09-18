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
$Header: machine.h,v 1.4 84/05/19 11:33:50 powell Exp $
 ****************************************************************************)
function Open(var name:FileName) : integer; external;
function Read(fid:integer; var buff:Buff; size:integer) : integer; external;

procedure Op (v : ShortString); external;
procedure S(v : ShortString); external;

procedure RefillBuffer; external;

function Create(var name:FileName) : integer; external;

procedure ErrorI(value, width: integer); external;
procedure ErrorC(value:char; width: integer); external;
procedure ErrorStr(value:ErrorString; width: integer); external;
procedure ErrorEOL; external;

procedure WriteI(fid:integer; value, width: integer); external;
procedure WriteC(fid:integer; value:char; width: integer); external;
procedure WriteS(fid:integer;var value:ErrorString; width: integer); external;
procedure WriteEOL(fid:integer); external;

function Mult(i, j: integer) : integer; external;
function Add(i, j: integer) : integer; external;
function atoi(var opd : operandstring) : integer; external;
procedure exit(status:integer); external;

const
    STANDARDINPUT = 'SYS$INPUT';
    STANDARDOUTPUT = 'SYS$OUTPUT';
    STANDARDERROR = 'SYS$ERROR';

var
{ input buffer and variables (used by #include readch.i) }
    buff : Buff;
    nextchar : char;
    EOF, EOLN : boolean;
    dumch : char;
    inputfile, 
    errorfile,
    chptr,
    numchread  : integer;

procedure setoptions; external;
