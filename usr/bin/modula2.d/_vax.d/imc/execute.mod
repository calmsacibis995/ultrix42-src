(*#@(#)execute.mod	4.1	Ultrix	7/17/90 *)
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
$Header: execute.mod,v 1.4 84/05/19 11:33:21 powell Exp $
 ****************************************************************************)
implementation module execute;
from io import writef, output, terminal;
from system import address, adr;
from unix import execve, environ, uexit, perror, File, fflush;
from strings import Assign, Append;
from parameters import GetEnvironment;
from stringtab import CopyString;
from porttab import logErrorsFlag;

var
    numArgs : cardinal;
    args : array [0..MAXARGS] of address;
    argStrings : array [0..MAXARGS] of ArgString;
    pathString : ArgString;
    programFile : ArgString;

procedure Execute(name : array of char);
var
    i, ignore : integer;
    j, k, nameLength : cardinal;
begin
    nameLength := 0;
    while (nameLength < number(name)-1) and (name[nameLength] # 0C) do
	inc(nameLength);
    end;
    args[numArgs] := nil;   
    if logErrorsFlag then
	writef(output,"%s:",name);
	i := 0;
	while args[i] # nil do
	    writef(output," %s",argStrings[i]);
	    inc(i);
	end;
	writef(output,"\n");
    end;
    GetEnvironment("PATH",pathString,i);
    if i < 0 then
	Assign(pathString,":/usr/ucb:/bin:/usr/bin");
    end;
    i := 0;
    while pathString[i] # 0C do
	j := 0;
	while (pathString[i] # 0C) and (pathString[i] # ':') do
	    programFile[j] := pathString[i];
	    inc(i);
	    inc(j);
	end;
	if (j > 0) and (programFile[j-1] # '/') then
	    programFile[j] := '/';
	    inc(j);
	end;
	programFile[j] := 0C;
	Append(programFile,name);
	fflush(File(output));
	fflush(File(terminal));
	ignore := execve(programFile,adr(args),environ);
	(* won't return, if successful *)
	if pathString[i] # 0C then
	    inc(i);
	end;
    end;
    writef(output,"%s: Command not found\n",name);
    uexit(99);
end Execute;

procedure AddArg(arg : array of char);
begin
     Assign(argStrings[numArgs],arg);
     args[numArgs] := adr(argStrings[numArgs]);
     inc(numArgs);
end AddArg;

procedure AddString(str : String);
begin
     CopyString(str,argStrings[numArgs]);
     args[numArgs] := adr(argStrings[numArgs]);
     inc(numArgs);
end AddString;

begin
    numArgs := 0;
end execute.
