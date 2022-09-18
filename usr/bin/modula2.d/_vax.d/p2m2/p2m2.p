(*#@(#)p2m2.p	4.1	Ultrix	7/17/90 *)
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
$Header: p2m2.p,v 1.3 84/05/19 11:42:45 powell Exp $
 ****************************************************************************)
program p2m2 (input, output);

#include "tokens.h"
#include "stringtab.h"
#include "globals.h"
#include "error.h"
#include "scanner.h"
#include "io.h"
#include "keywords.h"
#include "util.h"

procedure yyparse; external;
var
    yydebug : integer;	{ shared with yacc parser }
    sourceFile : String;
    hFile : String;
    hFlag : boolean;
    argName : FileName;

    
procedure ScanArgs;
var
    i, j, k : integer;
    gotName : boolean;
    done: boolean;
begin

    debugSet := [];
    lowerCaseFlag := false;
    standardFlag := false;
    hFlag := false;
    i := 1;
    gotName := false;
    while (argc>i) and not gotName do begin
	argv(i, argName);
	if (argName[0]='-') then begin
	    case (argName[1]) of
		'd' : begin
		    j := 2;
		    while ( argName[j] in ['a'..'z'] ) do begin
			debugSet := debugSet + [argName[j]];
			j := j + 1;
		    end; {while}
		end;
		'l' : begin
		    lowerCaseFlag := true;
		end;
		'h' : begin
		    hFlag := true;
		end;
		's' : begin
		    standardFlag := true;
		end;
	    end; {case}
	    i := i + 1;
	end else begin
	    gotName := true;
	end;
    end; {while}
    if i > argc then begin
	message('p2m2: missing source file');
	halt;
    end else begin
	j := 0;
	while argName[j] <> ' ' do begin
	    AddChar(argName[j]);
	    j := j + 1;
	end;
	sourceFile := NewString;
        if argName[j-2] <> '.' then begin
            sourceFileType := UNKNOWNFILE;
        end else if argName[j-1] = 'p' then begin
            sourceFileType := IMPLFILE;
        end else if argName[j-1] = 'h' then begin
            sourceFileType := DEFNFILE;
        end else begin
            sourceFileType := UNKNOWNFILE;
        end;
	if sourceFileType = UNKNOWNFILE then begin
	    message('p2m2: source file must end in .h or .p');
	    halt;
	end;
	if sourceFileType <> IMPLFILE then begin
	    hFlag := false;
	end;
	for k := 0 to j-3 do begin
	    AddChar(argName[k]);
	end;
        moduleName := NewString;

	if hFlag then begin
	    argName[j-1] := 'h';
	    for k := 0 to j-1 do begin
		AddChar(argName[k]);
	    end;
	    hFile := NewString;
	end;
    end;
end;

begin
    InitFiles;
    InitStringTable;
    ScanArgs;
    { initialization section }
    numberOfErrors := 0;
    TAB := chr(9);
    InitKeywords;

    if hFlag then begin
	{ parse .h file first }
	InitScanner(hFile);
	InitPass0;
	repeat
	    yyparse;
	until (currToken.kind = TKENDOFFILE);

	InitScanner(sourceFile);
	InitPass1;
	repeat
	    yyparse;
	until (currToken.kind = TKENDOFFILE);
    end else begin
	{ just do .p or .h file }
	InitScanner(sourceFile);
	InitPass0;
	InitPass1;
	repeat
	    yyparse;
	until (currToken.kind = TKENDOFFILE);
    end;

    Pass2;

    if numberOfErrors = 0 then begin
	message('No parsing errors');
    end else if numberOfErrors = 1 then begin
	message('1 parsing error');
	exit(numberOfErrors);
    end else begin
	message(numberOfErrors:1,' parsing errors');
	exit(numberOfErrors);
    end;
end {p2m2}.
