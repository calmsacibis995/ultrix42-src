(*#@(#)imc.mod	4.1	Ultrix	7/17/90 *)
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
$Header: imc.mod,v 1.5 84/06/06 12:22:05 powell Exp $
 ****************************************************************************)
module imc;
import unix;
from io import output, writef, sreadf, swritef;
from system import size, tsize, address, BITSPERWORD, BYTESPERWORD, MAXINT;
from memory import Allocate, Deallocate;
from parameters import NumParameters, GetParameter;
from strings import Compare, Assign, Append;
from stringtab import AddChar, NewString, String, CopyString, WriteString;
from list import List, AddToList;
from numtab import Number, LookUpNumber, DefineNumber,
    MAXBUILTINTYPES, DumpNumTab, traceNumtab;
from symtab import Symbol, TypeNode, EnumNode, LookUpSymbol, DefineSymbol,
	DataType, DumpSymTab, currScope, FieldNode, ParamNode,
	ModuleNode, PortNode, ConstNode, SetValue, ParamKind, SymbolKind,
	traceSymtab;
from porttab import DefinePort, DefineModule, DumpExports, CheckExports,
	currModule, tracePorttab, errorCount, autoMakeFlag, WatchModule,
	moduleList, logErrorsFlag, IgnoreModule, libraryCheckFlag,
	fatalErrorFlag;
from execute import AddArg, AddString, Execute, MAXARGS, ArgString;
from libfile import LIBFILENAME;

const
    BYTESIZE = BITSPERWORD div BYTESPERWORD;

    STABNMOD2 = 80;		(* = N_MOD2 in /usr/include/stab.h *)
    EXPORTSYMBOL = STABNMOD2;	(* nlist type for export symbol *)
    LOCALSYMBOL = 128;		(* nlist type for local symbol *)

    OBJECTMAGICNUMBER = 407B;	(* marker for first word of object *)
    ARCHIVEMAGICSTRING = "!<arch>\n";	(* marker for archive file *)
    ARCHIVEMAGICSTRINGSIZE = 8;
    ARCHIVEDIRECTORYSIZE = 60;

    OBJECTHEADERSIZE = 32;	(* size of a.out header *)
type
    ObjectHeader = record
	case boolean of
	TRUE : 		(* header for an object file *)
	    magic : integer;
	    textSize, dataSize, bssSize, symSize : cardinal;
	    entryPoint : cardinal;
	    textRelocSize, dataRelocSize : cardinal;
	|
	FALSE : 	(* header for an archive file *)
	    armagic : array [1..ARCHIVEMAGICSTRINGSIZE] of char;
	end;
    end;
    ArchiveDirectory = record
	name : array [1..16] of char;
	date : array [1..12] of char;
	usergroup : array [1..12] of char;	(* beware alignment! *)
	mode : array [1..8] of char;
	size : array [1..12] of char;		(* last 2 chars are `\n *)
    end;

    CharPtr = pointer @nocheck to array [0..MAXINT] of char;

    CharSet = set of char;
var
    moduleName : ArgString;
    errorMsg : array [1..100] of char;
    echoFlag, executeFlag : boolean;
    makeFlags : ArgString;
    inArchive : boolean;
    fileListIndex : cardinal;
    fileList : array [1..MAXARGS] of String;

procedure Panic(name, msg : array of char);
begin
    writef(output,"%s : %s\n",name,msg);
    halt;
end Panic;

procedure SaveArg(arg : array of char);
var
    i : cardinal;
begin
    inc(fileListIndex);
    i := 0;
    while (i < number(arg)) and (arg[i] # 0C) do
	AddChar(arg[i]);
	inc(i);
    end;
    fileList[fileListIndex] := NewString();
end SaveArg;

procedure BuildProc(retType : TypeNode) : TypeNode;
var
    tn : TypeNode;
    pn : ParamNode;
begin
    new(tn);
    tn^.kind := DTPROC;
    tn^.retType := retType;
    tn^.numParams := GetNumber();
    tn^.paramList := nil;
    SkipOver(';');
    while currChar # ';' do
	new(pn);
	if currChar in CharSet{'0'..'9'} then
	    pn^.name := nil;
	else
	    pn^.name := GetString();
	    SkipOver(':');
	end;
	pn^.paramType := GetType();
	SkipOver(',');
	pn^.kind := val(ParamKind,cardinal(GetNumber()));
	tn^.paramList := AddToList(tn^.paramList,pn);
	SkipOver(';');
    end;
    return tn;
end BuildProc;

procedure BuildRecord(tn : TypeNode);
var
    fn : FieldNode;
begin
    tn^.kind := DTRECORD;
    tn^.size := GetNumber();
    tn^.fieldList := nil;
    repeat
	new(fn);
	fn^.name := GetString();
	SkipOver(':');
	fn^.fieldType := GetType();
	SkipOver(',');
	fn^.offset := GetNumber();
	SkipOver(',');
	fn^.size := GetNumber();
	tn^.fieldList := AddToList(tn^.fieldList,fn);
	SkipOver(';');
    until currChar = ';';
    NextChar();
end BuildRecord;

procedure BuildEnum(tn : TypeNode);
var
    en : EnumNode;
begin
    tn^.kind := DTENUMERATION;
    tn^.enumCount := 0;
    tn^.enumList := nil;
    repeat
	inc(tn^.enumCount);
	new(en);
	en^.name := GetString();
	SkipOver(':');
	en^.enumOrd := GetNumber();
	en^.enumType := tn;
	tn^.enumList := AddToList(tn^.enumList,en);
	SkipOver(',');
    until currChar = ';';
    NextChar();
end BuildEnum;

(* GetType: Get a type pointer *)
procedure GetType():TypeNode;
var
    tn, retType, ignoreTn : TypeNode;
    indirectType : boolean;
begin
    if currChar = 'i' then
	repeat
	    NextChar();
	until currChar = ',';
	NextChar();
	indirectType := true;
    else
	indirectType := false;
    end;
    case currChar of
    | '0'..'9':
	tn := GetTypeNumber();
    | 'r':
	NextChar;
	new(tn);
	tn^.kind := DTSUBRANGE;
	tn^.baseType := GetType();
	SkipOver(';');
	tn^.subMinOrd := GetNumber();
	SkipOver(';');
	tn^.subMaxOrd := GetNumber();
    | 'a':
	NextChar;
	new(tn);
	tn^.kind := DTARRAY;
	tn^.indexType := GetType();
	SkipOver(';');
	tn^.elementType := GetType();
    | 'A':
	NextChar;
	new(tn);
	tn^.kind := DTARRAY;
	tn^.indexType := nil;
	tn^.elementType := GetType();
    | 'S':
	NextChar;
	new(tn);
	tn^.kind := DTSET;
	tn^.setRange := GetType();
    | 'o':
	NextChar;
	new(tn);
	tn^.kind := DTOPAQUE;
	tn^.opaqueName := GetString();
	if currChar = ',' then
	    NextChar();
	    ignoreTn := GetType();
	end;
	SkipOver(';');
    | 's':
	NextChar;
	new(tn);
	BuildRecord(tn);
    | 'f':
	NextChar;
	retType := GetType();
	SkipOver(',');
	tn := BuildProc(retType);
    | 'p':
	NextChar;
	tn := BuildProc(nil);
    | 'e':
	NextChar;
	new(tn);
	BuildEnum(tn);
    | '*':
	NextChar;
	new(tn);
	tn^.kind := DTPOINTER;
	tn^.toType := GetType();
    else
	DataError('Unknown type');
	tn := nil;
    end;
    if indirectType then
	SkipOver(';');
    end;
    return tn;
end GetType;

procedure BuildConst():ConstNode;
var
    cn : ConstNode;
    sv : SetValue;
    en : EnumNode;
begin
    new(cn);
    case currChar of
    | 'r':
	NextChar;
	cn^.kind := DTREAL;
	cn^.realVal := GetValue();
    | 'i':
	NextChar;
	cn^.kind := DTCARDINAL;
	cn^.cardVal := GetNumber();
    | 'b':
	NextChar;
	cn^.kind := DTBOOLEAN;
	cn^.boolVal := GetNumber();
    | 'c':
	NextChar;
	cn^.kind := DTCHAR;
	cn^.charVal := GetNumber();
    | 's':
	NextChar;
	cn^.kind := DTSTRING;
	SkipOver('''');
	loop
	    if currChar = '''' then
		NextChar;
		if currChar # '''' then
		    exit;
		end;
		AddChar('''');
	    else
		AddChar(currChar);
	    end;
	    NextChar;
	end;
	cn^.strVal := NewString();
    | 'S':
	NextChar;
	cn^.kind := DTSET;
	new(sv);
	sv^.setType := GetType();
	SkipOver(',');
	sv^.size := GetNumber();
	SkipOver(',');
	sv^.value := GetValue();
	cn^.setVal := sv;
    | 'e':
	NextChar;
	cn^.kind := DTENUMERATION;
	new(en);
	en^.enumType := GetType();
	SkipOver(',');
	en^.enumOrd := GetNumber();
	cn^.enumVal := en;
    else
	DataError('Unknown constant');
	cn := nil;
    end;
    return cn;
end BuildConst;

procedure DefineTypeNumber(number : cardinal) : Number;
var
    num : Number;
    tn : TypeNode;
begin
    tn := GetType();
    if not DefineNumber(num,number,0) then
	if num^.numType^.kind # DTOPAQUE then
	    swritef(errorMsg,'Type %d already defined',number);
	    DataError(errorMsg);
	else
	    (* update the type, but don't affect pointers to it *)
	    num^.numType^ := tn^;
	end;
    else
	num^.numType := tn;
    end;
    return num;
end DefineTypeNumber;

(* GetTypeNumber: Get the type corresponding to the type number *)
procedure GetTypeNumber():TypeNode;
var
    number : integer;
    num : Number;
    tn : TypeNode;
begin
    number := GetNumber();
    if currChar = '=' then
	NextChar();
	num := DefineTypeNumber(number);
    else
	num := LookUpNumber(number,0);
	if num = nil then
	    new(tn);
	    tn^.kind := DTOPAQUE;
	    if not DefineNumber(num,number,0) then
		swritef(errorMsg,'Missing type %d already defined',number);
		DataError(errorMsg);
	    else
		num^.numType := tn;
	    end;
	end;
    end;
    return num^.numType;
end GetTypeNumber;

var
    currEntry : CharPtr;
    currCharIndex : cardinal;
    currChar : char;

procedure NextChar();
begin
    currCharIndex := currCharIndex + 1;
    currChar := currEntry^[currCharIndex];
end NextChar;

procedure SkipOver(c : char);
var
    eligible : boolean;
    entry : CharPtr;
begin
    if currChar # c then
	swritef(errorMsg,'Expected "%c", found "%c"',c,currChar);
	DataError(errorMsg);
    else
	eligible := currChar in CharSet{',', ';'};
	NextChar;
	if eligible and (currChar = '?') then
	    (* continuation: go to the next entry *)
	    entry := CharPtr(stringTab+symTab^[symTabIndex].name-4);
	    inc(symTabIndex);
	    InitChar(entry);
	end;
    end;
end SkipOver;

const
    MAXDATAERRORS = 100;
var
    dataErrorCount : cardinal;
procedure DataError(msg : array of char);
begin
    writef(output,'Error in symbol table information: %s: char %d "%s"\n',
	msg,currCharIndex,currEntry^);
    inc(dataErrorCount);
    if dataErrorCount > MAXDATAERRORS then
	Panic('Too many errors','files are messed up');
    end;
end DataError;

procedure GetNumber() : integer;
var
    n : integer;
    negative : boolean;
begin
    n := 0;
    if not (currChar in CharSet{'0'..'9','-'}) then
	DataError('Number expected');
	return n;
    end;
    if currChar = '-' then
	negative := true;
	NextChar;
    else
	negative := false;
    end;
    while currChar in CharSet{'0'..'9'} do
	n := n * 10 + integer(ord(currChar) - ord('0'));
	NextChar;
    end;
    if negative then
	n := -n;
    end;
    return n;
end GetNumber;

procedure GetString() : String;
begin
    if not (currChar in CharSet{'a'..'z','A'..'Z'}) then
	DataError('Identifier expected');
	return nil;
    end;
    while currChar in CharSet{'a'..'z','A'..'Z','0'..'9','_'} do
	AddChar(currChar);
	NextChar;
    end;
    return NewString();
end GetString;

procedure GetModule() : String;
begin
    if not (currChar in CharSet{'a'..'z','A'..'Z'}) then
	DataError('Identifier expected');
	return nil;
    end;
    while currChar in CharSet{'a'..'z','A'..'Z','0'..'9'} do
	AddChar(currChar);
	NextChar;
    end;
    return NewString();
end GetModule;

procedure GetValue() : String;
begin
    while currChar # ';' do
	AddChar(currChar);
	NextChar;
    end;
    return NewString();
end GetValue;

(* InitChar: Initialize NextChar routine *)
procedure InitChar(entry : CharPtr);
begin
    currEntry := entry;
    currCharIndex := 0;
    currChar := currEntry^[currCharIndex];
end InitChar;

(* ProcessLocalEntry: Process the next local entry *)
procedure ProcessLocalEntry();
var
    number : integer;
    tn : TypeNode;
    num : Number;
    entry : CharPtr;
begin
    entry := CharPtr(stringTab+symTab^[symTabIndex].name-4);
    inc(symTabIndex);
    InitChar(entry);
    if echoFlag then
	writef(output,"l %s\n",entry^);
    end;
    while currChar # ':' do
	if currChar = 0C then
	    return;
	end;
	NextChar;
    end;
    NextChar;
    if currChar # 't' then
	return;
    end;
    NextChar;
    number := GetNumber();
    if (number <= MAXBUILTINTYPES) or (currChar # '=') then
	return;
    end;
    NextChar;
    num := DefineTypeNumber(number);
end ProcessLocalEntry;

(* ProcessExportEntry: Process the next export entry *)
procedure ProcessExportEntry() : boolean;
var
    name, modname : String;
    unqual, imported, extern : boolean;
    pn : PortNode;
    tn : TypeNode;
    entryType : char;
    mn : ModuleNode;
    entry : CharPtr;
begin
    entry := CharPtr(stringTab+symTab^[symTabIndex].name-4);
    inc(symTabIndex);
    InitChar(entry);
    if echoFlag then
	writef(output,"X %s\n",entry^);
    end;
    modname := GetModule();
    if currChar = '_' then
	NextChar;
	name := GetString();
    else
	name := nil;
    end;
    SkipOver(':');
    SkipOver('X');
    unqual := currChar = '0';
    NextChar;
    imported := currChar = '0';
    NextChar;
    extern := currChar = '0';
    NextChar;
    entryType := currChar;
    NextChar;
    case entryType of
    | 'v','p','f','t','c':
	new(pn);
	case entryType of
	| 'v':
	    pn^.kind := SYMVAR;
	    pn^.symVar := GetType();
	| 'p':
	    pn^.kind := SYMPROC;
	    pn^.symProc := BuildProc(nil);
	| 'f':
	    pn^.kind := SYMPROC;
	    tn := GetType();
	    SkipOver(',');
	    pn^.symProc := BuildProc(tn);
	| 't':
	    pn^.kind := SYMTYPE;
	    pn^.symType := GetType();
	| 'c':
	    pn^.kind := SYMCONST;
	    SkipOver('=');
	    pn^.symConst := BuildConst();
	end;
	DefinePort(modname,name,pn,imported,unqual,extern,currModule);
    | 'm':
	(* new module *)
	if inArchive then
	    mn := DefineModule(modname,0);
	else
	    mn := DefineModule(modname,fileListIndex);
	end;
	if mn^.defined then
	    writef(output,"Duplicate module %s(",fileName);
	    WriteString(output,modname);
	    writef(output,") ignored\n");
	    return(false);
	end;
	mn^.defined := true;
	currScope := mn^.scope;
	if logErrorsFlag then
	    writef(output,"Module ");
	    WriteString(output,modname);
	    writef(output,'\n');
	end;
	if inArchive then
	    mn^.watchErrors := TRUE;
	end;
	mn^.named := not inArchive;
	currModule := mn;
    | 'z':
	currModule := nil;
	(* end of external information *)
	return(false);
    else
	swritef(errorMsg,'Unknown symbol type %c',currChar);
	DataError(errorMsg);
    end;
    return(true);
end ProcessExportEntry;

type
    NameListRec = record
	name : cardinal;
	flags : cardinal;
	value : cardinal;
    end;
var
    stringTab : address;
    symTab : pointer to array [1..MAXINT] of NameListRec;
    symTabIndex : integer;

(* ProcessFile:  Process the file at the current seek position *)
procedure ProcessFile(obj : integer; header : ObjectHeader);
var
    readLen : integer;
    stringTabSize, stringTabOffset : cardinal;
    numSymbols, symTabOffset : integer;
    ignore : integer;
begin
    if header.magic # OBJECTMAGICNUMBER then
	return;
    end;
    (* read in symbol table *)
    numSymbols := header.symSize div (tsize(NameListRec) div BYTESIZE);
    if numSymbols <= 0 then
	Panic(fileName,"bad object file");
    end;
    symTabOffset := header.textSize+header.dataSize +
		header.textRelocSize + header.dataRelocSize;
    ignore := unix.lseek(obj,symTabOffset,1);
    Allocate(symTab,header.symSize*BYTESIZE);
    readLen := unix.read(obj,symTab^,header.symSize);

    (* get size of string table *)
    readLen := unix.read(obj,stringTabSize,BYTESPERWORD);
    stringTabSize := stringTabSize - BYTESPERWORD;
    if stringTabSize <= 0 then
	Panic(fileName,"bad object file");
    end;

    (* read in string table *)
    Allocate(stringTab,stringTabSize*BYTESIZE);
    readLen := unix.read(obj,stringTab^,stringTabSize);

    symTabIndex := 1;
    loop
	if symTabIndex > numSymbols then
	    exit;
	end;
	if symTab^[symTabIndex].flags mod 256 = EXPORTSYMBOL then
	    if not ProcessExportEntry() then
		exit;
	    end;
	elsif (currModule # nil) and
		(symTab^[symTabIndex].flags mod 256 = LOCALSYMBOL)
	then
	    ProcessLocalEntry();
	else
	    inc(symTabIndex);
	end;
    end;

    Deallocate(stringTab,stringTabSize*BYTESIZE);
    Deallocate(symTab,header.symSize*BYTESIZE);
end ProcessFile;

procedure ProcessArchive(obj : integer);
var
    arOffset, moduleSize : cardinal;
    dir : array [1..ARCHIVEDIRECTORYSIZE] of char;
    readLen : integer;
    header : ObjectHeader;
begin
    inArchive := TRUE;
    (* start after magic string *)
    arOffset := ARCHIVEMAGICSTRINGSIZE;
    loop
	(* position to next module *)
	ignore := unix.lseek(obj,arOffset,0);

	(* get directory entry *)
	readLen := unix.read(obj,dir,ARCHIVEDIRECTORYSIZE);
	if readLen # ARCHIVEDIRECTORYSIZE then
	    exit;
	end;
	readLen := sreadf(dir,"%s %*d %*d %*d %*d %d",moduleName,moduleSize);

	(* read in object file header *)
	readLen := unix.read(obj,header,OBJECTHEADERSIZE);
	if readLen # OBJECTHEADERSIZE then
	    exit;
	end;

	ProcessFile(obj,header);

	arOffset := (arOffset + ARCHIVEDIRECTORYSIZE + moduleSize+1) div 2 * 2;
    end;
    inArchive := FALSE;
end ProcessArchive;

var
    fileName, arg : ArgString;
    i, j, readLen, fileNameLength, ignore : integer;
    objFile : integer;
    header : ObjectHeader;
    mn : ModuleNode;
begin
    currModule := nil;
    dataErrorCount := 0;
    fileListIndex := 0;
    echoFlag := FALSE;
    traceNumtab := FALSE;
    traceSymtab := FALSE;
    tracePorttab := FALSE;
    autoMakeFlag := FALSE;
    logErrorsFlag := FALSE;
    libraryCheckFlag := FALSE;
    makeFlags := "";
    inArchive := FALSE;
    if NumParameters <= 1 then
	Panic("usage","mod2.2 files.o");
    end;
    if NumParameters > MAXARGS div 2 then
	Panic("usage","Cannot handle that many files");
    end;
    for i := 1 to NumParameters do
	if cardinal(i) = NumParameters then
	    Assign(fileName,LIBFILENAME);
	else
	    GetParameter(i,fileName,fileNameLength);
	end;
	if fileName[0] = '-' then
	    case fileName[1] of
	    | 'e':
		echoFlag := TRUE;
	    | 'v':
		logErrorsFlag := TRUE;
	    | 'm':
		autoMakeFlag := TRUE;
		Assign(makeFlags,fileName);
	    | 'L':
		libraryCheckFlag := TRUE;
	    | 'W':
		WatchModule(fileName);
	    | 'N':
		IgnoreModule(fileName);
	    | 'T':
		if Compare(fileName,'=',"-Tnumtab") then
		    traceNumtab := TRUE;
		elsif Compare(fileName,'=',"-Tsymtab") then
		    traceSymtab := TRUE;
		elsif Compare(fileName,'=',"-Tporttab") then
		    tracePorttab := TRUE;
		end;
	    | 'l':
		SaveArg(fileName);
		(* get end of library name *)
		j := 2;
		while fileName[j] # 0C do
		    arg[j-2] := fileName[j];
		    inc(j);
		end;
		arg[j-2] := 0C;
		Assign(fileName,"/lib/lib");
		Append(fileName,arg);
		Append(fileName,".a");
		objFile := unix.open(fileName,0);
		if objFile >= 0 then
		    if echoFlag then
			writef(output,"File %s\n",fileName);
		    end;
		    ProcessArchive(objFile);
		    ignore := unix.close(objFile);
		else
		    Assign(fileName,"/usr/lib/lib");
		    Append(fileName,arg);
		    Append(fileName,".a");
		    objFile := unix.open(fileName,0);
		    if objFile >= 0 then
			ProcessArchive(objFile);
			ignore := unix.close(objFile);
		    else
			Assign(fileName,"/usr/local/lib/lib");
			Append(fileName,arg);
			Append(fileName,".a");
			objFile := unix.open(fileName,0);
			if objFile >= 0 then
			    ProcessArchive(objFile);
			    ignore := unix.close(objFile);
			else
			    Panic(arg,"Cannot open library file");
			end;
		    end;
		end;
	    else
		Panic("Unknown option",fileName);
	    end;
	else
	    if cardinal(i) # NumParameters then
		SaveArg(fileName);
	    end;
	    objFile := unix.open(fileName,0);
	    if objFile < 0 then
		Panic(fileName,"Cannot open file");
	    else
		if echoFlag then
		    writef(output,"File %s\n",fileName);
		end;
		readLen := unix.read(objFile,header,OBJECTHEADERSIZE);
		if readLen # OBJECTHEADERSIZE then
		    Panic(fileName,"Error reading file header");
		end;
		if Compare(header.armagic,'=',ARCHIVEMAGICSTRING) then
		    ProcessArchive(objFile);
		elsif header.magic = OBJECTMAGICNUMBER then
		    ProcessFile(objFile, header);
		else
		    Panic(fileName,"Not an object file or library");
		end;
		ignore := unix.close(objFile);
	    end;
	end;
    end;
    CheckExports();
    if (errorCount > 0) and autoMakeFlag and not fatalErrorFlag then
	AddArg("mod");
	AddArg("-c");
	AddArg(makeFlags);
	if libraryCheckFlag then
	    AddArg("-L");
	end;
	if logErrorsFlag then
	    AddArg("-v");
	end;
	i := 2;	(* skip -m *)
	while makeFlags[i] # 0C do
	    j := 0;
	    while (makeFlags[i] # ',') and (makeFlags[i] # ' ') and
		    (makeFlags[i] # 0C)
	    do
		fileName[j] := makeFlags[i];
		inc(i);
		inc(j);
	    end;
	    fileName[j] := 0C;
	    AddArg(fileName);
	    if makeFlags[i] # 0C then
		inc(i);
	    end;
	end;
	executeFlag := FALSE;
	mn := moduleList^.first;
	loop
	    if mn = nil then
		exit;
	    end;
	    if mn^.outOfDate then
		if mn^.named and (mn^.fileName # 0) then
		    CopyString(mn^.name,fileName);
		    Assign(arg,"-W");
		    Append(arg,fileName);
		    AddArg(arg);
		    CopyString(fileList[mn^.fileName],fileName);
		    i := 0;
		    while fileName[i] # 0C do
			inc(i);
		    end;
		    if (fileName[i-2] = '.') and (fileName[i-1] = 'o') then
			fileName[i-1] := 0C;
			Append(fileName,"mod");
			AddArg(fileName);
			fileList[mn^.fileName] := nil;
		    else
			fatalErrorFlag := TRUE;
			writef(output,"Cannot determine source file from %s\n",
				fileName);
		    end;
		    executeFlag := TRUE;
		else
		    fatalErrorFlag := TRUE;
		    writef(output,"Cannot find file for module %s\n",fileName);
		end;
	    elsif not mn^.builtin and mn^.ignoreErrors then
		CopyString(mn^.name,fileName);
		Assign(arg,"-N");
		Append(arg,fileName);
		AddArg(arg);
	    end;
	    mn := mn^.next;
	end;
	if executeFlag then
	    for i := 1 to fileListIndex do
		if fileList[i] # nil then
		    AddString(fileList[i]);
		end;
	    end;
	    Execute("mod");
	end;
    end;
    if errorCount > 100 then
	errorCount := 101;
    end;
    unix.uexit(errorCount);
end imc.
