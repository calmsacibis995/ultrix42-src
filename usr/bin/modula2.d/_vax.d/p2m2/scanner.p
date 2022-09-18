(*#@(#)scanner.p	4.1	Ultrix	7/17/90 *)
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
$Header: scanner.p,v 1.4 84/05/27 13:00:09 powell Exp $
 ****************************************************************************)
#include "tokens.h"
#include "stringtab.h"
#include "globals.h"
#include "error.h"
#include "scanner.h"
#include "io.h"
#include "keywords.h"
#include "util.h"

type
    YYlval = record case boolean of
	true : (str : String);
	false : (int : integer);
    end;
var yylval: YYlval;  { shared with yacc: really a pointer }

var
    ch : char;	{ current character }
    endOfFile : boolean;

function InitFile{(fn:String):boolean};
var
    f : FilePtr;
    fileName : FileName;
    number : FileNumber;
    success : boolean;
begin
    if fn = nil then begin
	number := GetInput;
    end else begin
	StringToFileName(fn,fileName);
	number := Open(fileName,fn^.length);
    end;
    if number > 0 then begin
	new(f);
	f^.name := fn;
	f^.number := number;
	f^.lineNumber := 0;
	f^.ptr := 0;
	f^.size := 0;
	f^.next := inFile;
	inFile := f;
	currFile := fn;
	success := true;
    end else begin
	success := false;
    end;
    InitFile := success;
end;

procedure InitScanner{(fn:String)};
begin
    inFile := nil;
    endOfFile := false;
    readingFile := true;
    mainFileName := fn;
    if not InitFile(fn) then begin
	ErrorName(fn,'Cannot find file');
	exit(999);
    end;
    NextChar;
end;

procedure NextChar;
begin
    if endOfFile then begin
	Error('Attempt to read past end of file');
	halt;
    end else if inFile = nil then begin
	ch := '?';
	endOfFile := true;
    end else begin
	inFile^.ptr := inFile^.ptr + 1;
	if inFile^.ptr > inFile^.size then begin
	    NewLine;
	end;
	if endOfFile then begin
	    ch := ' ';
	end else begin
	    ch := inFile^.line[inFile^.ptr];
	end;
    end;
end; {NextChar}

procedure PrevChar;
begin
	inFile^.ptr := inFile^.ptr - 1;
end; {PrevChar}

procedure NewLine;	{ procedure NewLine; external; }
var
	i : integer;
	oldf : FilePtr;
begin
	inFile^.lineNumber := inFile^.lineNumber + 1;
	currLine := inFile^.lineNumber;
	inFile^.size := ReadLine(inFile^.number,inFile^.line);
	if inFile^.size < 0 then begin
	    { end of file, pop stack }
	    Close(inFile^.number);
	    oldf := inFile;
	    inFile := inFile^.next;
	    dispose(oldf);
	    if inFile = nil then begin
		currFile := nil;
		currLine := -1;
	    end else begin
		currFile := inFile^.name;
		currLine := inFile^.lineNumber;
	    end;
	    NextChar;
	end else begin
	    inFile^.ptr := 1;
	    ch := inFile^.line[inFile^.ptr];
	end;
end; {NewLine}

procedure NextTok(var token : TokenRec);

var
    i : integer;
    lastCh : char;
    strDelim : char;
    hash : integer;
    number : array [0..79] of char;
    continue, instring : boolean;
    str : String;


procedure HandleNumbers(var token : TokenRec);
begin
    while ch in ['0'..'9'] do begin
	AddChar(ch);
	NextChar;
    end;
    if ch = '.' then begin
	NextChar;
	if ch = '.' then begin
	    PrevChar;
	end else begin
	    AddChar('.');
	    while ch in ['0'..'9'] do begin
		AddChar(ch);
		NextChar;
	    end;
	end;
    end;
    if ch in ['e','E'] then begin
	AddChar(ch);
	NextChar;
	if ch in ['+','-'] then begin
	    AddChar(ch);
	    NextChar;
	end;
	while ch in ['0'..'9'] do begin
	    AddChar(ch);
	    NextChar;
	end;
    end;
    token.kind := TKNUMBER;
    token.str := NewString;
end; {HandleNumbers}

begin {NextTok}
    continue := true;
    while continue do begin
	if endOfFile then begin
	    token.kind := TKENDOFFILE;
	    continue := false;
	    readingFile := false;
	end else if ch in [' ',chr(12),chr(10),TAB] then begin
	    OutChar(ch);
	    spaceFlag := true;
	    NextChar;
	end else if ch in ['a'..'z','A'..'Z'] then begin
	    if lowerCaseFlag then begin
		if ch in ['A'..'Z'] then begin
		    ch := chr(ord(ch)-ord('A')+ord('a'));
		end;
	    end;
	    repeat
		AddChar(ch);
		NextChar;
	    until not (ch in ['a'..'z', 'A'..'Z', '0'..'9']);
	    token.str := NewString;
	    token.kind := KeyLookUp(token.str);  {see if keyword or ident}
	    continue := false;		{exit procedure}
	end else if ch in ['0'..'9'] then begin
	    {handle numbers}
	    HandleNumbers(token);		{handles ints and reals}
	    continue := false;		{exit procedure}
	end else if (ch in [' '..'}']) then begin
	    case (ch) of
		'$','%','?','@','\','_','`' : begin
		    token.kind := TKBAD;
		    NextChar;
		    continue := false;
		end;
		'<' : begin
		    NextChar;
		    if (ch = '>') then begin
			token.kind := TKNOTEQUAL;
			NextChar;
		    end else if (ch = '=') then begin
			token.kind := TKLSEQUAL;
			NextChar;
		    end else begin
			token.kind := TKLESS;
		    end;
		    continue := false;		{exit procedure}
		end;
		'=' : begin
		    token.kind := TKEQUALS;
		    NextChar;
		    continue := false;		{exit procedure}
		end;
		'>' : begin
		    NextChar;
		    if (ch = '=') then begin
			token.kind := TKGREQUAL;
			NextChar;
		    end else begin
			token.kind := TKGREATER;
		    end;
		    continue := false;		{exit procedure}
		end;
		':' : begin
		    NextChar;
		    if (ch = '=') then begin
			token.kind := TKASSIGN;
			NextChar;
		    end else begin
			token.kind := TKCOLON;
		    end;
		    continue := false;		{exit procedure}
		end;
		'+' : begin
		    token.kind := TKPLUS;
		    NextChar;
		    continue := false;		{exit procedure}
		end;
		'-' : begin
		    token.kind := TKMINUS;
		    NextChar;
		    continue := false;		{exit procedure}
		end;
		'*' : begin
		    token.kind := TKASTERISK;
		    NextChar;
		    continue := false;		{exit procedure}
		end;
		';' : begin
		    token.kind := TKSEMICOLON;
		    NextChar;
		    continue := false;		{exit procedure}
		end;
		')' : begin
		    token.kind := TKRPAREN;
		    NextChar;
		    continue := false;		{exit procedure}
		end;
		'(' : begin
		    NextChar;
		    if (ch = '*') then begin		{start comment}
			instring := true;
			OutChar('(');
			OutChar('*');
			NextChar;
			repeat
			    if ch = '*' then begin
				NextChar;
				if ch = ')' then begin
				    NextChar;
				    instring := false;
				end else begin
				    OutChar('*');
				end;
			    end else begin
				OutChar(ch);
				NextChar;
			    end;
			until not instring;
			OutChar('*'); OutChar(')');
			spaceFlag := true;
		    end else begin
			token.kind := TKLPAREN;
			continue := false;		{exit procedure}
		    end;
		end;
		'{' : begin
		        NextChar;
			OutChar('('); OutChar('*');
			while ch <> '}' do begin
			    OutChar(ch);
			    NextChar;
			end;
			OutChar('*'); OutChar(')');
			spaceFlag := true;
			NextChar;
		end;
		'/' : begin
		    token.kind := TKSLASH;
		    NextChar;
		    continue := false;		{exit procedure}
		end;
		'|' : begin
		    token.kind := TKBAR;
		    NextChar;
		    continue := false;		{exit procedure}
		end;
		'^' : begin
		    token.kind := TKUPARROW;
		    NextChar;
		    continue := false;		{exit procedure}
		end;
		'.' : begin
		    NextChar;
		    if (ch = '.') then begin
			token.kind := TKDOTDOT;
			NextChar;
		    end else begin
			token.kind := TKDOT;
		    end;
		    continue := false;		{exit procedure}
		end;
		'[' : begin
		    token.kind := TKLBRACKET;
		    NextChar;
		    continue := false;		{exit procedure}
		end;
		']' : begin
		    token.kind := TKRBRACKET;
		    NextChar;
		    continue := false;		{exit procedure}
		end;
		'&' : begin
		    token.kind := TKAMPERSAND;
		    NextChar;
		    continue := false;		{exit procedure}
		end;
		',' : begin
		    token.kind := TKCOMMA;
		    NextChar;
		    continue := false;		{exit procedure}
		end;
		'#' : begin
			token.kind := TKSHARP;
			NextChar;
			continue := false;		{exit procedure}
		end;
		'''', '"' : begin		{string}
		    strDelim := ch;		{save apostr. or quotes}
		    NextChar;
		    instring := true;
		    repeat
			if ch = strDelim then begin
			    NextChar;
			    if ch = strDelim then begin
				{ doubled delimiter = delimiter character }
				AddChar(ch);
				NextChar;
			    end else begin
				{ must have been end of string }
				instring := false;
			    end;
			end else begin
			    { normal character }
			    AddChar(ch);
			    NextChar;
			end;
		    until not instring;
		    str := NewString;
		    if str^.length = 1 then begin
			token.kind := TKCHARCONST;
		    end else begin
			token.kind := TKSTRCONST;
		    end; {if}
		    token.str := str;
		    continue := false;		{exit procedure}
		end;
	    end; {case}
	end else begin
	    ErrorNumber('Invalid character, ord = ',ord(ch));
	    NextChar;
	    token.kind := TKBAD;
	    continue := false;
	end; {if}
    end; {while}
end; {NextTok}


function yylex : Token;
var
	i : integer;
begin
    NextTok(currToken);
    yylex := currToken.kind;
    if (currToken.kind in [TKIDENT, TKNUMBER, TKCHARCONST, TKSTRCONST])
    then begin
        yylval.str := currToken.str;
    end else begin
        yylval.int := 0;
    end;
end; {yylex}

procedure LineWrite;
var
	i : integer;
begin
    for i := 1 to inFile^.size do begin
	write(output,inFile^.line[i]);
    end;
    writeln(output);
end;

procedure yyerror(var msgx : ErrorString);
var
    i : integer;
    msg : ErrorString;
begin
    msg := '  ';
    i := 1;
    while msgx[i] <> chr(0) do begin
	msg[i] := msgx[i];
	i := i + 1;
    end;
    Error(msg);
end;

procedure EatSpace;
begin
    if ch = ' ' then begin
	NextChar;
    end;
end;

procedure AdvanceSpace;
begin
    while ch in [' ',chr(12),chr(10),TAB] do begin
	OutChar(ch);
	spaceFlag := true;
	NextChar;
    end;
end;
