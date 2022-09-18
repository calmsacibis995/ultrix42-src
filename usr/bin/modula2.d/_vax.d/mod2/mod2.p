(*#@(#)mod2.p	4.1	Ultrix	7/17/90 *)
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
$Header: mod2.p,v 1.5 84/05/19 11:42:25 powell Exp $
 ****************************************************************************)
program Modula (input, output);

#include "globals.h"
#include "scanner.h"
#include "iolib.h"
#include "keywdtab.h"
#include "decls.h"
#include "builtin.h"
#include "optim.h"
#include "ocount.h"
#include "alloc.h"
#include "genpcf.h"
#include "gentf.h"
#include "gencode.h"

var
    verboseFlag : boolean;

procedure yyparse; external;
var
    yydebug : integer;	{ shared with yacc parser }

procedure ScanArgs;
var
    i, j, k : integer;
    gotName : boolean;
    argName : FileName;
begin {Modula}

    debugSet := [];
    upperCaseFlag := false;
    standardKeywordFlag := false;
    standardStringFlag := false;
    standardCardinalFlag := false;
    genDebugInfoFlag := false;
    genCheckFlag := false;
    optimFlag := false;
    OptNloop := false;
    OptNreg := false;
    OptNind := false;
    OptNtail := false;
    OptNcall := true;
    genIndirectStab := false;
    gprofFlag := false;
    verboseFlag := false;
    codeFileName := CODEFILENAME;
    target := TARGETVAX;
    i := 1;
    gotName := false;
    while (argc>i) and not gotName do begin
	argv(i, argName);
	if (argName[0]='-') then begin
	    if not (argName[1] in ['d','g','o','p','s','t','u','v','C','I','N','O','T'])
	    then begin
		message('mod2.0: Unknown option: ',argName);
		exit(101);
	    end else begin
		case (argName[1]) of
		    'd' : begin
			j := 2;
			while ( argName[j] in ['a'..'z'] ) do begin
			    debugSet := debugSet + [argName[j]];
			    j := j + 1;
			end; {while}
		    end;
		    'N' : begin
			if argName = '-Nreg' then begin
			    OptNreg := true;
			end else if argName = '-Nloop' then begin
			    OptNloop := true;
			end else if argName = '-Nind' then begin
			    OptNind := true;
			end else if argName = '-Ntail' then begin
			    OptNtail := true;
			end else if argName = '-Ncall' then begin
			    OptNcall := true;
			end;
		    end;
		    'T' : begin
			if argName = '-Tcexpr' then begin
			    TraceCexpr := true;
			end else if argName = '-Tdecls' then begin
			    TraceDecls := true;
			end else if argName = '-Tnexpr' then begin
			    TraceNexpr := true;
			end else if argName = '-Tnstmt' then begin
			    TraceNstmt := true;
			end else if argName = '-Tsymtab' then begin
			    TraceSymtab := true;
			end else if argName = '-Tgent' then begin
			    TraceGent := true;
			end else if argName = '-Tstab' then begin
			    TraceStab := true;
			end else if argName = '-Tactions' then begin
			    TraceActions := true;
			end else if argName = '-Toptim' then begin
			    TraceOptim := true;
			end else if argName = '-Topt' then begin
			    TraceOptim := true;
			    TraceOpt := true;
			    TraceActions := true;
			end else if argName = '-Tmark' then begin
			    TraceMark := true;
			    TraceOptim := true;
			    TraceOpt := true;
			end else begin
			    message('mod2.0: Unknown option: ',argName);
			    exit(101);
			end;
		    end;
		    't' : begin
			target := TARGETTITAN;
		    end;
		    'u' : begin
			upperCaseFlag := true;
		    end;
		    'v' : begin
			verboseFlag := true;
		    end;
		    's' : begin
			if argName[2] = ' ' then begin
			    standardStringFlag := true;
			    standardKeywordFlag := true;
			    standardCardinalFlag := true;
			end else if argName[2] = 'c' then begin
			    standardCardinalFlag := true;
			end else if argName[2] = 'k' then begin
			    standardKeywordFlag := true;
			end else if argName[2] = 's' then begin
			    standardStringFlag := true;
			end else begin
			    message('mod2.0: Unknown option: ',argName);
			    exit(101);
			end;
		    end;
		    'g' : begin
			genDebugInfoFlag := true;
		    end;
		    'o' : begin
			i := i + 1;
			if i >= argc then begin
			    message('No file name after -o');
			    exit(101);
			end;
			codeFileName := '  ';
			argv(i,codeFileName);
		    end;
		    'C' : begin
			genCheckFlag := true;
		    end;
		    'O' : begin
			optimFlag := true;
			OptNcall := false;
		    end;
		    'p' : begin
			gprofFlag := true;
		    end;
		    'I' : begin
			genIndirectStab := true;
		    end;
		end;
	    end;
	    i := i + 1;
	end else begin
	    gotName := true;
	end;
    end; {while}
    if not gotName then begin
	InitScanner(nil);
    end else begin
	j := 0;
	while argName[j] <> ' ' do begin
	    AddChar(argName[j]);
	    j := j + 1;
	end;
	j := j - 1;
	InitScanner(NewString);
	if codeFileName = CODEFILENAME then begin
	    if j > 5 then begin
		if (codeFileName[j-4] = '.') and (codeFileName[j-3] = 'm')
		    and (codeFileName[j-2] = 'o') and (codeFileName[j-1] = 'd')
		then begin
		    codeFileName := '  ';
		    for i := 1 to j-4 do begin
			codeFileName[i] := argName[i];
		    end;
		    case target of
			TARGETVAX : begin
			    codeFileName[j-3] := 'p';
			    codeFileName[j-2] := 'c';
			    codeFileName[j-1] := 'd';
			end;
			TARGETTITAN : begin
			    codeFileName[j-3] := 'i';
			end;
		    end;
		end;
	    end;
	end;
    end;
end;

begin
    
    InitFiles;
    InitStringTable;
    ScanArgs;
    if 'p' in debugSet then begin
	yydebug := 1;	{turn on parser debug}
    end;
    { initialization section }

    numberOfErrors := 0;
    InitKeywords;
    InitError;
    InitAlloc;
    InitSymTab;

    { These three must be in this order and after the above }
    InitStandardTypes; InitGlobalModule; InitBuiltin;

    yyparse;

    CheckModule(globalModule);

    if 's' in debugSet then begin
	writeln(output,'String table');
	DumpStringTab;
    end;
    if 'r' in debugSet then begin
	writeln(output,'Symbol table');
	DumpSymTab;
    end;

    if numberOfErrors = 0 then begin
	if verboseFlag then begin
	    writeln(output,'No parsing errors');
	end;
    end else if numberOfErrors = 1 then begin
	writeln(output,'1 parsing error');
	exit(numberOfErrors);
    end else begin
	writeln(output,numberOfErrors:1,' parsing errors');
	exit(numberOfErrors);
    end;

    if optimFlag then begin
	if verboseFlag then begin
	    writeln(output,'Start optimization');
	end;
	Optimize;
	if verboseFlag then begin
	    writeln(output,'End optimization');
	end;
    end;

    numberOfErrors := 0;
    GenCode;
    if numberOfErrors = 0 then begin
	if verboseFlag then begin
	    writeln(output,'No intermediate code generation errors');
	end;
    end else if numberOfErrors = 1 then begin
	writeln(output,'1 intermediate code generation error');
	exit(numberOfErrors);
    end else begin
	writeln(output,numberOfErrors:1,' intermediate code generation errors');
	exit(numberOfErrors);
    end;
    
end.
