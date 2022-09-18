(*#@(#)error.p	4.1	Ultrix	7/17/90 *)
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
$Header: error.p,v 1.5 84/05/19 11:38:35 powell Exp $
 ****************************************************************************)
#include "globals.h"
#include "scanner.h"

procedure MakeErrorString(var str : String; string : ShortString);
begin
    AddText(string);
    str := NewString;
end;

procedure InitError;
var
    dt : DataType;
    token : Token;
    sk : SymbolKind;
    ek : ExprKind;
    stk : StmtKind;
begin
    MakeErrorString(stringDataType[DTNULL],'null');
    MakeErrorString(stringDataType[DTPOINTER],'pointer');
    MakeErrorString(stringDataType[DTRECORD],'record');
    MakeErrorString(stringDataType[DTARRAY],'array');
    MakeErrorString(stringDataType[DTINTEGER],'integer');
    MakeErrorString(stringDataType[DTBOOLEAN],'boolean');
    MakeErrorString(stringDataType[DTCHAR],'char');
    MakeErrorString(stringDataType[DTRENAME],'rename');
    MakeErrorString(stringDataType[DTOPAQUE],'opaque');
    MakeErrorString(stringDataType[DTSTRING],'string');
    MakeErrorString(stringDataType[DTREAL],'real');
    MakeErrorString(stringDataType[DTLONGREAL],'longreal');
    MakeErrorString(stringDataType[DTSET],'set');
    MakeErrorString(stringDataType[DTCARDINAL],'cardinal');
    MakeErrorString(stringDataType[DTSUBRANGE],'subrange');
    MakeErrorString(stringDataType[DTENUMERATION],'enumeration');
    MakeErrorString(stringDataType[DTPROC],'proc');
    MakeErrorString(stringDataType[DTWORD],'word');
    MakeErrorString(stringDataType[DTBYTE],'byte');
    MakeErrorString(stringDataType[DTANY],'any');

    MakeErrorString(stringSymbolKind[SYMNULL],'null');
    MakeErrorString(stringSymbolKind[SYMVAR],'var');
    MakeErrorString(stringSymbolKind[SYMCONST],'const');
    MakeErrorString(stringSymbolKind[SYMPROC],'proc');
    MakeErrorString(stringSymbolKind[SYMTYPE],'type');
    MakeErrorString(stringSymbolKind[SYMMODULE],'module');
    MakeErrorString(stringSymbolKind[SYMFIELD],'field');
    MakeErrorString(stringSymbolKind[SYMENUM],'enum');

    MakeErrorString(stringToken[TKENDOFFILE],'end of file');
    MakeErrorString(stringToken[TKPLUS],'+ ');
    MakeErrorString(stringToken[TKMINUS],'- ');
    MakeErrorString(stringToken[TKASTERISK],'* ');
    MakeErrorString(stringToken[TKSLASH],'/ ');
    MakeErrorString(stringToken[TKASSIGN],':=');
    MakeErrorString(stringToken[TKAMPERSAND],'& ');
    MakeErrorString(stringToken[TKDOT],'. ');
    MakeErrorString(stringToken[TKCOMMA],', ');
    MakeErrorString(stringToken[TKSEMICOLON],'; ');
    MakeErrorString(stringToken[TKLPAREN],'( ');
    MakeErrorString(stringToken[TKLBRACKET],'[ ');
    MakeErrorString(stringToken[TKLBRACE],'{ ');
    MakeErrorString(stringToken[TKUPARROW],'^ ');
    MakeErrorString(stringToken[TKEQUALS],'= ');
    MakeErrorString(stringToken[TKSHARP],'# ');
    MakeErrorString(stringToken[TKLESS],'< ');
    MakeErrorString(stringToken[TKGREATER],'> ');
    MakeErrorString(stringToken[TKNOTEQUAL],'<>');
    MakeErrorString(stringToken[TKLSEQUAL],'<=');
    MakeErrorString(stringToken[TKGREQUAL],'>=');
    MakeErrorString(stringToken[TKDOTDOT],'..');
    MakeErrorString(stringToken[TKCOLON],': ');
    MakeErrorString(stringToken[TKRPAREN],') ');
    MakeErrorString(stringToken[TKRBRACKET],'] ');
    MakeErrorString(stringToken[TKRBRACE],'} ');
    MakeErrorString(stringToken[TKBAR],'| ');
    MakeErrorString(stringToken[TKIDENT],'identifier');
    MakeErrorString(stringToken[TKCARDCONST],'card const');
    MakeErrorString(stringToken[TKREALCONST],'real const');
    MakeErrorString(stringToken[TKCHARCONST],'char const');
    MakeErrorString(stringToken[TKSTRCONST],'string const');
    MakeErrorString(stringToken[TKBOOLCONST],'boolean const');
    MakeErrorString(stringToken[TKAND],'and');
    MakeErrorString(stringToken[TKARRAY],'array');
    MakeErrorString(stringToken[TKBEGIN],'begin');
    MakeErrorString(stringToken[TKBY],'by');
    MakeErrorString(stringToken[TKCASE],'case');
    MakeErrorString(stringToken[TKCONST],'const');
    MakeErrorString(stringToken[TKDEFINITION],'definition');
    MakeErrorString(stringToken[TKDIV],'div');
    MakeErrorString(stringToken[TKDO],'do');
    MakeErrorString(stringToken[TKELSE],'else');
    MakeErrorString(stringToken[TKELSIF],'elsif');
    MakeErrorString(stringToken[TKEND],'end');
    MakeErrorString(stringToken[TKEXIT],'exit');
    MakeErrorString(stringToken[TKEXPORT],'export');
    MakeErrorString(stringToken[TKFOR],'for');
    MakeErrorString(stringToken[TKFROM],'from');
    MakeErrorString(stringToken[TKIF],'if');
    MakeErrorString(stringToken[TKIMPLEMENTATION],'implementation');
    MakeErrorString(stringToken[TKIMPORT],'import');
    MakeErrorString(stringToken[TKIN],'in');
    MakeErrorString(stringToken[TKLOOP],'loop');
    MakeErrorString(stringToken[TKMOD],'mod');
    MakeErrorString(stringToken[TKMODULE],'module');
    MakeErrorString(stringToken[TKNOT],'not');
    MakeErrorString(stringToken[TKOF],'of');
    MakeErrorString(stringToken[TKOR],'or');
    MakeErrorString(stringToken[TKPOINTER],'pointer');
    MakeErrorString(stringToken[TKPROCEDURE],'procedure');
    MakeErrorString(stringToken[TKQUALIFIED],'qualified');
    MakeErrorString(stringToken[TKRECORD],'record');
    MakeErrorString(stringToken[TKREPEAT],'repeat');
    MakeErrorString(stringToken[TKRETURN],'return');
    MakeErrorString(stringToken[TKSET],'set');
    MakeErrorString(stringToken[TKTHEN],'then');
    MakeErrorString(stringToken[TKTO],'to');
    MakeErrorString(stringToken[TKTYPE],'type');
    MakeErrorString(stringToken[TKUNTIL],'until');
    MakeErrorString(stringToken[TKVAR],'var');
    MakeErrorString(stringToken[TKWHILE],'while');
    MakeErrorString(stringToken[TKWITH],'with');
    MakeErrorString(stringToken[TKINCLUDE],'include');
    MakeErrorString(stringToken[TKBAD],'bad');
    MakeErrorString(stringToken[TKNULL],'null');
end;

procedure ErrorAll(name : String; msg : ErrorString; number : integer;
	printNumber : boolean);
var
    i, j : integer;
begin
    write(output,'File ');
    if readingFile and (inFile <> nil) then begin
	if inFile^.name = nil then begin
	    write(output,'input');
	end else begin
	    WriteString(output,inFile^.name);
	end;
	if inFile^.lineNumber = -1 then begin
	    write(output,', line (end of file): ');
	end else begin
	    write(output,', line ',inFile^.lineNumber:1,': ');
	end;
    end else begin
	if currFile = nil then begin
	    write(output,'input');
	end else begin
	    WriteString(output,currFile);
	end;
	write(output, ', line ',currLine:1,': ');
    end;
    if name <> nil then begin
	WriteString(output,name);
	write(output,': ');
    end;
    i := ERRORSTRINGSIZE;
    while (i > 1) and (msg[i] = ' ') do begin
	i := i - 1;
    end;
    for j := 1 to i do begin
	write(output,msg[j]);
    end;
    if printNumber then begin
	write(output,number:1);
    end;
    writeln(output);
    if readingFile and (inFile <> nil) then begin
	LineWrite;
	for i := 1 to inFile^.ptr-1 do begin
	    if inFile^.line[i] = TABCHAR then begin
		write(output,chr(TABCHAR));
	    end else begin
		write(output,' ');
	    end;
	end;
	writeln(output,'^');
    end;
    numberOfErrors := numberOfErrors + 1;
end;

procedure ErrorName{(name : String; msg : ErrorString)};
begin
    ErrorAll(name,msg,0,false);
end;

procedure ErrorNumber{(msg : ErrorString; number : integer)};
begin
    ErrorAll(nil,msg,number,true);
end;

procedure Error{(msg : ErrorString)};
begin
    ErrorAll(nil,msg,0,false);
end;

procedure GeneralError(fileName : String; lineNumber : integer;
    name : String; msg : ErrorString; number : integer; printNumber : boolean);
var
    i, j : integer;
begin
    write(output,'File ');
    WriteString(output,fileName);
    if lineNumber = -1 then begin
	write(output,', line (end of file): ');
    end else begin
	write(output,', line ',lineNumber:1,': ');
    end;
    if name <> nil then begin
	WriteString(output,name);
	write(output,': ');
    end;
    i := ERRORSTRINGSIZE;
    while (i > 1) and (msg[i] = ' ') do begin
	i := i - 1;
    end;
    for j := 1 to i do begin
	write(output,msg[j]);
    end;
    if printNumber then begin
	write(output,number:1);
    end;
    writeln(output);
    numberOfErrors := numberOfErrors + 1;
end;

procedure ExprError{(expr : ExprNode; msg : ErrorString)};
begin
    GeneralError(expr^.fileName,expr^.lineNumber,nil,msg,0,false);
end;

procedure ExprErrorName{(expr : ExprNode; name : String; msg : ErrorString)};
begin
    GeneralError(expr^.fileName,expr^.lineNumber,name,msg,0,false);
end;

procedure ExprErrorNameNumber{(expr : ExprNode; name : String;
	msg : ErrorString; number : integer)};
begin
    GeneralError(expr^.fileName,expr^.lineNumber,name,msg,number,true);
end;

procedure ExprErrorNumber{(expr : ExprNode; msg : ErrorString;
	number : integer)};
begin
    GeneralError(expr^.fileName,expr^.lineNumber,nil,msg,number,true);
end;

procedure StmtError{(stmt : StmtNode; msg : ErrorString)};
begin
    GeneralError(stmt^.fileName,stmt^.lineNumber,nil,msg,0,false);
end;

procedure StmtErrorName{(stmt : StmtNode; name : String; msg : ErrorString)};
begin
    GeneralError(stmt^.fileName,stmt^.lineNumber,name,msg,0,false);
end;
