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
$Header: scanner.p,v 1.5 84/05/19 11:44:32 powell Exp $
 ****************************************************************************)
#include "globals.h"
#include "scanner.h"
#include "iolib.h"
#include "keywdtab.h"

var
    ch : char;	{ current character }
    rawch : rawcharacter; { possibly out-of-range character }
    endOfFile : boolean;
    previousToken : Token;
    inComment : boolean;
    eofOK : boolean;

procedure ScanEofOK;
begin
    eofOK := true;
end;

procedure EndFile;
var
    oldf : FilePtr;
begin
    if inComment then begin
	Error('Comment open at end of file');
    end;
    if not eofOK then begin
	ErrorName(currFile,'Unexpected end of file');
    end;
    if 'f' in debugSet then begin
	write(output,'EndFile: ');
	WriteString(output,currFile);
	writeln(output);
    end;
    if inFile <> nil then begin
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
    end;
    eofOK := false;
end;

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
	currLine := 0;
	success := true;
	eofOK := false;
	if 'f' in debugSet then begin
	    write(output,'InitFile: ');
	    WriteString(output,fn);
	    writeln(output);
	end;
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
    inComment := false;
    if not InitFile(fn) then begin
	ErrorName(fn,'Cannot find file');
	exit(999);
    end;
    NextChar;
    previousToken := TKBAD;
end;

procedure NextChar;
begin
    if endOfFile then begin
	Error('Attempt to read past end of file');
	exit(999);
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
	    rawch := inFile^.line[inFile^.ptr];
	    if (rawch > MAXCHAR) or (rawch < MINCHAR) then begin
		Error('Invalid character in input');
		ch := ' ';
	    end else begin
		ch := chr(rawch);
	    end;
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
begin
	inFile^.lineNumber := inFile^.lineNumber + 1;
	currLine := inFile^.lineNumber;
	inFile^.size := ReadLine(inFile^.number,inFile^.line);
	if inFile^.size < 0 then begin
	    EndFile;
	    NextChar;
	end else begin
	    inFile^.ptr := 1;
	    rawch := inFile^.line[inFile^.ptr];
	    if (rawch > MAXCHAR) or (rawch < MINCHAR) then begin
		Error('Invalid character in input');
		ch := ' ';
	    end else begin
		ch := chr(rawch);
	    end;
	end;
end; {NewLine}

procedure NextTok(var token : TokenRec);

var
    i, value : integer;
    lastCh : char;
    strDelim : char;
    hash : integer;
    number : array [0..79] of char;
    continue, instring : boolean;
    stringStartLine : integer;
    str : String;


procedure HandleNumbers(var token : TokenRec);
const
    MINEXP = -38;
    MAXEXP = 38;
    MAXREALDIG = 18;
    MAXREALSTR = '170174118346046923'; {maximum VAX d float}
    MAXCARDVALUE = 4294967295.0;
    MAXCHARVALUE = 255.0;
var
    i, j, exp, expsign, dotpos : integer;
    strnum : array [1..MAXREALDIG] of char;
    error : boolean;
procedure SetTokenInteger;
var
    maxdig, base : integer;
    value : real;	{ must be >= 32 bits of mantissa }
    legal : set of char;
begin
    token.kind := TKCARDCONST;
    if (strnum[i] = 'h') or (strnum[i] = 'H') then begin
	legal := ['0'..'9','a'..'f','A'..'F'];
	base := 16;
	i := i - 1;
    end else if (strnum[i] = 'b') or (strnum[i] = 'B') then begin
	legal := ['0'..'7'];
	base := 8;
	i := i - 1;
    end else if (strnum[i] = 'c') or (strnum[i] = 'C') then begin
	legal := ['0'..'7'];
	base := 8;
	i := i - 1;
	token.kind := TKCHARCONST;
    end else begin
	legal := ['0'..'9'];
	base := 10;
    end;
    if not error then begin
	value := 0.0;
	for i := 1 to i do begin
	    error := not (strnum[i] in legal);
	    if strnum[i] in ['0'..'9'] then begin
		value := value * base + ord(strnum[i]) - ord('0');
	    end else if strnum[i] in ['a'..'f'] then begin
		value := value * base + ord(strnum[i]) - ord('a') + 10;
	    end else if strnum[i] in ['A'..'F'] then begin
		value := value * base + ord(strnum[i]) - ord('A') + 10;
	    end;
	end;
    end;
    if not error then begin
	if ((token.kind = TKCHARCONST) and (value > MAXCHARVALUE))
	    or (value > MAXCARDVALUE)
	then begin
	    Error('Constant exceeds implementation limit');
	end;
    end;
    if error then begin
	token.kind := TKBAD;
    end else if token.kind = TKCHARCONST then begin
	new(token.con);
	token.con^.kind := DTCHAR;
	token.con^.charVal := trunc(value);
    end else begin
	new(token.con);
	token.con^.kind := DTCARDINAL;
	token.con^.cardVal := value;
    end;
end;
begin {HandleNumbers}
    error := false;
    strnum := '  ';
    i := 0;
    dotpos := 0;
    while not error and (ch in ['0'..'9','a'..'f','h','A'..'F','H']) do begin
	if i < MAXREALDIG then begin
	    i := i + 1;
	    strnum[i] := ch;
	    NextChar;
	end else begin
	    error := true;
	end;
    end;
    if error then begin
	{ do nothing }
    end else if ( ch <> '.' ) then begin	{ integer }
	SetTokenInteger;
    end else begin
	NextChar;
	if ( ch = '.' ) then begin	{ next token is dotdot }
	    PrevChar;
	    SetTokenInteger;	{ so it was really an integer }
	end else begin
	    dotpos := i;
	    for j := 1 to i do begin
		error :=  not (strnum[j] in ['0'..'9']);
	    end;
	    while not error and (ch in ['0'..'9']) do begin
		if i < MAXREALDIG then begin
		    i := i + 1;
		    strnum[i] := ch;
		    NextChar;
		end else begin
		    error := true;
		end;
	    end;
	    if error then begin
		{ do nothing }
	    end else if ( ch = 'e' ) or ( ch = 'E' ) then begin
		exp := 0;		{ get exponent }
		expsign := 1;
		NextChar;
		if ch = '-' then begin
		    expsign := -1;
		    NextChar;
		end else if ch = '+' then begin
		    NextChar;
		end;
		if ch in ['0'..'9'] then begin
		    exp := exp + ord(ch) - ord('0');
		    NextChar;
		    if ch in ['0'..'9'] then begin
			exp := exp * 10 + ord(ch) - ord('0');
			NextChar;
			if ch in ['0'..'9'] then begin
			    error := true;
			end;
		    end;
		end else begin
		    error := true;
		end;
	    end else begin
		exp := 0;
		expsign := 1;
	    end;
	    exp := expsign*exp + dotpos - i;
	    if (exp < MAXEXP) or
		    ((exp = MAXEXP) and (strnum <= MAXREALSTR))
	    then begin
		token.kind := TKREALCONST;
		new(token.con);
		token.con^.kind := DTREAL;
		i := i + 1;
		strnum[i] := 'e';
		if exp < 0 then begin
		    i := i + 1;
		    strnum[i] := '-';
		    exp := -exp;
		end;
		i := i + 1;
		strnum[i] := chr(ord('0') + exp div 10);
		i := i + 1;
		strnum[i] := chr(ord('0') + exp mod 10);
		token.con^.realVal := atof(strnum[1]);
	    end else begin
		error := true;
	    end;
	end;
    end;
    if error then token.kind := TKBAD;
end; {HandleNumbers}

procedure GetComment(comment : char);
var
    stop : boolean;
begin {GetComment}
    inComment := true;
    stop := false;
    repeat
	if ch = '*' then begin
	    NextChar;
	    if (ch = ')') then begin
		NextChar;
		stop := true;
	    end;
	end else if ch = '(' then begin
	    NextChar;
	    if (ch = '*') then begin
		NextChar;
		{ nested comment, recur }
		GetComment('(');
	    end;
	end else begin
	    NextChar;
	end;
    until stop;
    inComment := false;
end; {GetComment}

begin {NextTok}
    continue := true;
    while continue do begin
	if endOfFile then begin
	    token.kind := TKENDOFFILE;
	    continue := false;
	    readingFile := false;
	end else if ch in [' ',chr(TABCHAR),chr(FORMFEEDCHAR),chr(LINEFEEDCHAR),
			    chr(RETURNCHAR)]
	then begin
	    NextChar;
	end else if ch in ['a'..'z','A'..'Z','@'] then begin
	    repeat
		if upperCaseFlag then begin
		    if ch in ['a'..'z'] then begin
			ch := chr(ord(ch)-ord('a')+ord('A'));
		    end;
		end;
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
	end else if (ch in [' '..'~']) then begin
	    case (ch) of
		'$','%','?','\','_','`','!','~' : begin
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
		    if previousToken <> TKSEMICOLON then begin
			token.kind := TKSEMICOLON;
			NextChar;
			continue := false;	{exit procedure}
		    end else begin
			NextChar;		{ skip extra ;s }
		    end;
		end;
		')' : begin
		    token.kind := TKRPAREN;
		    NextChar;
		    continue := false;		{exit procedure}
		end;
		'(' : begin
		    NextChar;
		    if (ch = '*') then begin		{start comment}
			GetComment('(');
		    end else begin
			token.kind := TKLPAREN;
			continue := false;		{exit procedure}
		    end;
		end;
		'/' : begin
		    token.kind := TKSLASH;
		    NextChar;
		    continue := false;		{exit procedure}
		end;
		'{' : begin
		    token.kind := TKLBRACE;
		    NextChar;
		    continue := false;		{exit procedure}
		end;
		'}' : begin
		    token.kind := TKRBRACE;
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
		    stringStartLine := currLine;
		    repeat
			if ch = strDelim then begin
			    NextChar;
			    if not standardStringFlag and (ch = strDelim)
			    then begin
				{ doubled delimiter = delimiter character }
				AddChar(ch);
				NextChar;
			    end else begin
				{ must have been end of string }
				instring := false;
			    end;
			end else if not standardStringFlag and (ch = '\')
			then begin
			    NextChar;
			    if ch = 'n' then begin
				AddCharX(LINEFEEDCHAR);	{ newline }
			    end else if ch = 't' then begin
				AddCharX(TABCHAR);	{ tab }
			    end else if ch = 'r' then begin
				AddCharX(RETURNCHAR);	{ return }
			    end else if ch = 'f' then begin
				AddCharX(FORMFEEDCHAR);	{ form feed }
			    end else if ch = 'b' then begin
				AddCharX(BACKSPACECHAR);{ backspace }
			    end else if ch in ['0'..'9'] then begin
				i := 1;
				value := 0;
				while (i <= 3) and (ch in ['0'..'9']) do begin
				    value := value*8 + ord(ch)-ord('0');
				    NextChar;
				    i := i + 1;
				end;
				AddCharX(value);
				PrevChar;	{ don't consume extra char }
			    end else begin
				AddChar(ch);		{ etc. ', ", \ }
			    end;
			    NextChar;
			end else begin
			    if currLine <> stringStartLine then begin
				Error('String crosses line boundary');
				instring := false;
			    end;
			    { normal character }
			    AddChar(ch);
			    NextChar;
			end;
		    until not instring;
		    str := NewString;
		    if str^.length = 1 then begin
			token.kind := TKCHARCONST;
			new(token.con);
			token.con^.kind := DTCHAR;
			token.con^.charVal := GetCharX(str,0);
		    end else begin
			token.kind := TKSTRCONST;
			new(token.con);
			token.con^.kind := DTSTRING;
			token.con^.strVal := str;
		    end; {if}
		    continue := false;		{exit procedure}
		end;
	    end; {case}
	end else begin
	    AddChar(ch);
	    ErrorName(NewString,'Invalid character');
	    NextChar;
	    token.kind := TKBAD;
	    continue := false;
	end; {if}
    end; {while}
    previousToken := token.kind;
end; {NextTok}

function yylex : Token;
var
	i : integer;
begin
    NextTok(currToken);
    if currToken.kind = TKIDENT then begin
	yylval.str := currToken.str;
    end else if currToken.kind in [TKCARDCONST, TKREALCONST, TKBOOLCONST,
		TKCHARCONST, TKSTRCONST]
    then begin
	yylval.con := currToken.con;
    end;
    if 't' in debugSet then begin
	write(currToken.kind,' ');
	if (currToken.kind in [TKIDENT, TKCARDCONST, TKREALCONST, TKBOOLCONST,
			    TKCHARCONST, TKSTRCONST]) then begin
	    case (currToken.kind) of
		TKIDENT : begin
		    write('ident: ');
		    WriteString(output,currToken.str);
		    writeln;
		end;
		TKCARDCONST : begin
		    writeln('int: ',currToken.con^.cardVal:1:0);
		end;
		TKREALCONST : begin
		    writeln('real: ', currToken.con^.realVal);
		end;
		TKBOOLCONST : begin
		    writeln('bool: ', currToken.con^.boolVal);
		end;
		TKCHARCONST : begin
		    write('char: ',currToken.con^.charVal);
		    writeln;
		end;
		TKSTRCONST : begin
		    write('string: ');
		    WriteString(output,currToken.con^.strVal);
		    writeln;
		end;
	    end; {case}
	end else begin		{ must be token or keyword }
	    writeln('currToken: ',currToken.kind);
	end;
    end; {if}
    yylex := currToken.kind;
end; {yylex}

procedure LineWrite;
var
	i : integer;
begin
    for i := 1 to inFile^.size do begin
	if inFile^.line[i] in [MINCHAR..MAXCHAR] then begin
	    write(output,chr(inFile^.line[i]));
	end else begin
	    write(output,'?');
	end;
    end;
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
