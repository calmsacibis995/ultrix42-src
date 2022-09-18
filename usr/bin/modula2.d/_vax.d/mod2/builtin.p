(*#@(#)builtin.p	4.1	Ultrix	7/17/90 *)
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
$Header: builtin.p,v 1.6 84/06/06 12:51:01 powell Exp $
 ****************************************************************************)
#include "globals.h"
#include "const.h"
#include "decls.h"
#include "bexpr.h"
#include "cexpr.h"
#include "builtin.h"
#include "optim.h"
#include "otree.h"
#include "ocount.h"

var
    allocateString, deallocateString : String;

procedure InitUnixProcNames;
begin
    unixProcNames[1] := 'exit';
    unixProcNames[2] := 'fork';
    unixProcNames[3] := 'read';
    unixProcNames[4] := 'write';
    unixProcNames[5] := 'open';
    unixProcNames[6] := 'close';
    unixProcNames[7] := 'wait';
    unixProcNames[8] := 'creat';
    unixProcNames[9] := 'link';
    unixProcNames[10] := 'unlink';
    unixProcNames[11] := 'exec';
    unixProcNames[12] := 'chdir';
    unixProcNames[13] := 'time';
    unixProcNames[14] := 'mknod';
    unixProcNames[15] := 'chmod';
    unixProcNames[16] := 'chown';
    unixProcNames[17] := 'break';
    unixProcNames[18] := 'stat';
    unixProcNames[19] := 'lseek';
    unixProcNames[20] := 'getpid';
    unixProcNames[21] := 'mount';
    unixProcNames[22] := 'umount';
    unixProcNames[23] := 'setuid';
    unixProcNames[24] := 'getuid';
    unixProcNames[25] := 'stime';
    unixProcNames[26] := 'ptrace';
    unixProcNames[27] := 'alarm';
    unixProcNames[28] := 'fstat';
    unixProcNames[29] := 'pause';
    unixProcNames[30] := 'utime';
    unixProcNames[31] := 'stty';
    unixProcNames[32] := 'gtty';
    unixProcNames[33] := 'access';
    unixProcNames[34] := 'nice';
    unixProcNames[35] := 'ftime';
    unixProcNames[36] := 'sync';
    unixProcNames[37] := 'kill';
    unixProcNames[38] := 'switch';
    unixProcNames[39] := 'setpgrp';
    unixProcNames[40] := 'tell';
    unixProcNames[41] := 'dup';
    unixProcNames[42] := 'pipe';
    unixProcNames[43] := 'times';
    unixProcNames[44] := 'profil';
    unixProcNames[45] := '???';
    unixProcNames[46] := 'setgid';
    unixProcNames[47] := 'getgid';
    unixProcNames[48] := 'signal';
    unixProcNames[49] := '???';
    unixProcNames[50] := '???';
    unixProcNames[51] := 'acct';
    unixProcNames[52] := 'setphys';
    unixProcNames[53] := 'syslock';
    unixProcNames[54] := 'ioctl';
    unixProcNames[55] := 'reboot';
    unixProcNames[56] := 'mpx';
    unixProcNames[57] := '???';
    unixProcNames[58] := '???';
    unixProcNames[59] := 'exece';
    unixProcNames[60] := 'umask';
    unixProcNames[61] := 'chroot';
    unixProcNames[62] := '???';
    unixProcNames[63] := '???';
    unixProcNames[64] := '???';
    unixProcNames[65] := '???';
    unixProcNames[66] := 'vfork';
    unixProcNames[67] := 'vread';
    unixProcNames[68] := 'vwrite';
    unixProcNames[69] := 'smdate';
    unixProcNames[70] := 'brk';
    unixProcNames[71] := 'sbrk';
end;

procedure InitBuiltin;
var
    name : String;
    sym : Symbol;
    mn : ModuleNode;
    pn : ProcNode;
    paramList : ParamList;
    exports : IdentList;
    cn : ConstNode;
    vn : VarNode;
    error : boolean;
    saveScope : Scope;
begin
    error := false;
    { Enter system types }
    { DEFINITION MODULE SYSTEM }
    AddText('SYSTEM');
    name := NewString;
    mn := DefineModule(name,TKBEGIN);
    exports := AddToIdentList(nil,nil);

    AddText('WORD');
    name := NewString;
    exports := AddToIdentList(exports,MakeIdent(name));
    DefineType(name,wordTypeNode);

    AddText('BYTE');
    name := NewString;
    exports := AddToIdentList(exports,MakeIdent(name));
    DefineType(name,byteTypeNode);

    AddText('ADDRESS');
    name := NewString;
    exports := AddToIdentList(exports,MakeIdent(name));
    DefineType(name,addressTypeNode);

    AddText('ADR');
    name := NewString;
    exports := AddToIdentList(exports,MakeIdent(name));
    pn := DefineProc(name,TKPROCEDURE);
    pn^.builtin := BIPADR;
    pn^.procType := builtinProcTypeNode;
    EndProc(pn,nil,nil);

    AddText('SIZE');
    name := NewString;
    exports := AddToIdentList(exports,MakeIdent(name));
    pn := DefineProc(name,TKPROCEDURE);
    pn^.builtin := BIPSIZE;
    pn^.procType := builtinProcTypeNode;
    EndProc(pn,nil,nil);

    AddText('TSIZE');
    name := NewString;
    exports := AddToIdentList(exports,MakeIdent(name));
    pn := DefineProc(name,TKPROCEDURE);
    pn^.builtin := BIPTSIZE;
    pn^.procType := builtinProcTypeNode;
    EndProc(pn,nil,nil);

    AddText('BYTESIZE');
    name := NewString;
    exports := AddToIdentList(exports,MakeIdent(name));
    pn := DefineProc(name,TKPROCEDURE);
    pn^.builtin := BIPBYTESIZE;
    pn^.procType := builtinProcTypeNode;
    EndProc(pn,nil,nil);

    AddText('TBYTESIZE');
    name := NewString;
    exports := AddToIdentList(exports,MakeIdent(name));
    pn := DefineProc(name,TKPROCEDURE);
    pn^.builtin := BIPTBYTESIZE;
    pn^.procType := builtinProcTypeNode;
    EndProc(pn,nil,nil);

    AddText('CPUTIME');
    name := NewString;
    exports := AddToIdentList(exports,MakeIdent(name));
    pn := DefineProc(name,TKPROCEDURE);
    pn^.builtin := BIPCPUTIME;
    pn^.procType := builtinProcTypeNode;
    EndProc(pn,nil,nil);

    AddText('UNIXCALL');
    name := NewString;
    exports := AddToIdentList(exports,MakeIdent(name));
    pn := DefineProc(name,TKPROCEDURE);
    pn^.builtin := BIPUNIXCALL;
    pn^.procType := builtinProcTypeNode;
    EndProc(pn,nil,nil);

    AddText('BYTESPERWORD');
    name := NewString;
    exports := AddToIdentList(exports,MakeIdent(name));
    new(cn);
    cn^.kind := DTCARDINAL;
    cn^.cardVal := WORDSIZE div BYTESIZE;
    DefineConst(name,cn);

    AddText('BITSPERWORD');
    name := NewString;
    exports := AddToIdentList(exports,MakeIdent(name));
    new(cn);
    cn^.kind := DTCARDINAL;
    cn^.cardVal := WORDSIZE;
    DefineConst(name,cn);

    AddText('BITSPERBYTE');
    name := NewString;
    exports := AddToIdentList(exports,MakeIdent(name));
    new(cn);
    cn^.kind := DTCARDINAL;
    cn^.cardVal := BYTESIZE;
    DefineConst(name,cn);

    AddText('MOSTSIGBIT');
    name := NewString;
    exports := AddToIdentList(exports,MakeIdent(name));
    new(cn);
    cn^.kind := DTCARDINAL;
    cn^.cardVal := WORDSIZE-1;
    DefineConst(name,cn);

    AddText('LEASTSIGBIT');
    name := NewString;
    exports := AddToIdentList(exports,MakeIdent(name));
    new(cn);
    cn^.kind := DTCARDINAL;
    cn^.cardVal := 0;
    DefineConst(name,cn);

    AddText('MAXUNSIGNED');
    name := NewString;
    exports := AddToIdentList(exports,MakeIdent(name));
    new(cn);
    cn^.kind := DTCARDINAL;
    cn^.cardVal := MAXCARD;
    DefineConst(name,cn);

    AddText('MAXCARD');
    name := NewString;
    exports := AddToIdentList(exports,MakeIdent(name));
    new(cn);
    cn^.kind := DTCARDINAL;
    if standardCardinalFlag then begin
	cn^.cardVal := MAXCARD;
    end else begin
	cn^.cardVal := MAXINT;
    end;
    DefineConst(name,cn);

    AddText('MAXINT');
    name := NewString;
    exports := AddToIdentList(exports,MakeIdent(name));
    new(cn);
    cn^.kind := DTCARDINAL;
    cn^.cardVal := MAXINT;
    DefineConst(name,cn);

    AddText('PROCESS');
    name := NewString;
    exports := AddToIdentList(exports,MakeIdent(name));
    DefineType(name,processTypeNode);

    AddText('NEWPROCESS');
    name := NewString;
    exports := AddToIdentList(exports,MakeIdent(name));
    pn := DefineProc(name,TKPROCEDURE);
    pn^.builtin := BIPNEWPROCESS;
    pn^.procType := builtinProcTypeNode;
    EndProc(pn,nil,nil);

    AddText('TRANSFER');
    name := NewString;
    exports := AddToIdentList(exports,MakeIdent(name));
    pn := DefineProc(name,TKPROCEDURE);
    pn^.builtin := BIPTRANSFER;
    pn^.procType := builtinProcTypeNode;
    EndProc(pn,nil,nil);

    Export(exports,TKQUALIFIED);

    EndModule(mn,nil,nil);

    AddText('BITOPERATIONS');
    name := NewString;
    mn := DefineModule(name,TKBEGIN);
    exports := AddToIdentList(nil,nil);

    AddText('BITAND');
    name := NewString;
    exports := AddToIdentList(exports,MakeIdent(name));
    pn := DefineProc(name,TKPROCEDURE);
    pn^.builtin := BIPBITAND;
    pn^.procType := builtinProcTypeNode;
    EndProc(pn,nil,nil);

    AddText('BITOR');
    name := NewString;
    exports := AddToIdentList(exports,MakeIdent(name));
    pn := DefineProc(name,TKPROCEDURE);
    pn^.builtin := BIPBITOR;
    pn^.procType := builtinProcTypeNode;
    EndProc(pn,nil,nil);

    AddText('BITNOT');
    name := NewString;
    exports := AddToIdentList(exports,MakeIdent(name));
    pn := DefineProc(name,TKPROCEDURE);
    pn^.builtin := BIPBITNOT;
    pn^.procType := builtinProcTypeNode;
    EndProc(pn,nil,nil);

    AddText('BITXOR');
    name := NewString;
    exports := AddToIdentList(exports,MakeIdent(name));
    pn := DefineProc(name,TKPROCEDURE);
    pn^.builtin := BIPBITXOR;
    pn^.procType := builtinProcTypeNode;
    EndProc(pn,nil,nil);

    AddText('BITSHIFTLEFT');
    name := NewString;
    exports := AddToIdentList(exports,MakeIdent(name));
    pn := DefineProc(name,TKPROCEDURE);
    pn^.builtin := BIPBITSHIFTLEFT;
    pn^.procType := builtinProcTypeNode;
    EndProc(pn,nil,nil);

    AddText('BITSHIFTRIGHT');
    name := NewString;
    exports := AddToIdentList(exports,MakeIdent(name));
    pn := DefineProc(name,TKPROCEDURE);
    pn^.builtin := BIPBITSHIFTRIGHT;
    pn^.procType := builtinProcTypeNode;
    EndProc(pn,nil,nil);

    AddText('BITINSERT');
    name := NewString;
    exports := AddToIdentList(exports,MakeIdent(name));
    pn := DefineProc(name,TKPROCEDURE);
    pn^.builtin := BIPBITINSERT;
    pn^.procType := builtinProcTypeNode;
    EndProc(pn,nil,nil);

    AddText('BITEXTRACT');
    name := NewString;
    exports := AddToIdentList(exports,MakeIdent(name));
    pn := DefineProc(name,TKPROCEDURE);
    pn^.builtin := BIPBITEXTRACT;
    pn^.procType := builtinProcTypeNode;
    EndProc(pn,nil,nil);

    Export(exports,TKQUALIFIED);

    EndModule(mn,nil,nil);

    { get strings for allocate and deallocate }
    { as you define memory module }

    AddText('MEMORY');
    name := NewString;
    mn := DefineModule(name,TKBEGIN);
    exports := AddToIdentList(nil,nil);

    AddText('ALLOCATE');
    name := NewString;
    allocateString := name;
    exports := AddToIdentList(exports,MakeIdent(name));
    pn := DefineProc(name,TKPROCEDURE);
    pn^.builtin := BIPALLOCATE;
    pn^.procType := builtinProcTypeNode;
    EndProc(pn,nil,nil);

    AddText('DEALLOCATE');
    name := NewString;
    deallocateString := name;
    exports := AddToIdentList(exports,MakeIdent(name));
    pn := DefineProc(name,TKPROCEDURE);
    pn^.builtin := BIPDEALLOCATE;
    pn^.procType := builtinProcTypeNode;
    EndProc(pn,nil,nil);

    Export(exports,TKQUALIFIED);

    EndModule(mn,nil,nil);


    { DEFINITION MODULE IO }
    AddText('IO');
    name := NewString;
    mn := DefineModule(name,TKBEGIN);

    exports := AddToIdentList(nil,nil);

    AddText('FILE');
    name := NewString;
    exports := AddToIdentList(exports,MakeIdent(name));
    DefineType(name,fileTypeNode);

    AddText('WRITEF');
    name := NewString;
    exports := AddToIdentList(exports,MakeIdent(name));
    pn := DefineProc(name,TKPROCEDURE);
    pn^.builtin := BIPWRITEF;
    pn^.procType := builtinProcTypeNode;
    EndProc(pn,nil,nil);

    AddText('READF');
    name := NewString;
    exports := AddToIdentList(exports,MakeIdent(name));
    pn := DefineProc(name,TKPROCEDURE);
    pn^.builtin := BIPREADF;
    pn^.procType := builtinProcTypeNode;
    EndProc(pn,nil,nil);

    AddText('SWRITEF');
    name := NewString;
    exports := AddToIdentList(exports,MakeIdent(name));
    pn := DefineProc(name,TKPROCEDURE);
    pn^.builtin := BIPWRITES;
    pn^.procType := builtinProcTypeNode;
    EndProc(pn,nil,nil);

    AddText('SREADF');
    name := NewString;
    exports := AddToIdentList(exports,MakeIdent(name));
    pn := DefineProc(name,TKPROCEDURE);
    pn^.builtin := BIPREADS;
    pn^.procType := builtinProcTypeNode;
    EndProc(pn,nil,nil);

    AddText('WRITEB');
    name := NewString;
    exports := AddToIdentList(exports,MakeIdent(name));
    pn := DefineProc(name,TKPROCEDURE);
    pn^.builtin := BIPWRITEB;
    pn^.procType := builtinProcTypeNode;
    EndProc(pn,nil,nil);

    AddText('READB');
    name := NewString;
    exports := AddToIdentList(exports,MakeIdent(name));
    pn := DefineProc(name,TKPROCEDURE);
    pn^.builtin := BIPREADB;
    pn^.procType := builtinProcTypeNode;
    EndProc(pn,nil,nil);

    AddText('WRITEC');
    name := NewString;
    exports := AddToIdentList(exports,MakeIdent(name));
    pn := DefineProc(name,TKPROCEDURE);
    pn^.builtin := BIPWRITEC;
    pn^.procType := builtinProcTypeNode;
    EndProc(pn,nil,nil);

    AddText('READC');
    name := NewString;
    exports := AddToIdentList(exports,MakeIdent(name));
    pn := DefineProc(name,TKPROCEDURE);
    pn^.builtin := BIPREADC;
    pn^.procType := builtinProcTypeNode;
    EndProc(pn,nil,nil);

    AddText('OPEN');
    name := NewString;
    exports := AddToIdentList(exports,MakeIdent(name));
    pn := DefineProc(name,TKPROCEDURE);
    pn^.builtin := BIPOPENF;
    pn^.procType := builtinProcTypeNode;
    EndProc(pn,nil,nil);

    AddText('CLOSE');
    name := NewString;
    exports := AddToIdentList(exports,MakeIdent(name));
    pn := DefineProc(name,TKPROCEDURE);
    pn^.builtin := BIPCLOSEF;
    pn^.procType := builtinProcTypeNode;
    EndProc(pn,nil,nil);

    AddText('EOF');
    name := NewString;
    exports := AddToIdentList(exports,MakeIdent(name));
    pn := DefineProc(name,TKPROCEDURE);
    pn^.builtin := BIPEOFF;
    pn^.procType := builtinProcTypeNode;
    EndProc(pn,nil,nil);

    AddText('INPUT');
    name := NewString;
    exports := AddToIdentList(exports,MakeIdent(name));
    vn := DefineVar(name,fileTypeNode,MEMNORMAL,false);

    AddText('OUTPUT');
    name := NewString;
    exports := AddToIdentList(exports,MakeIdent(name));
    vn := DefineVar(name,fileTypeNode,MEMNORMAL,false);

    AddText('TERMINAL');
    name := NewString;
    exports := AddToIdentList(exports,MakeIdent(name));
    vn := DefineVar(name,fileTypeNode,MEMNORMAL,false);

    Export(exports,TKQUALIFIED);

    EndModule(mn,nil,nil);

    saveScope := currScope;
    currScope := builtinScope;

    AddText('ABS');
    name := NewString;
    pn := DefineProc(name,TKPROCEDURE);
    pn^.builtin := BIPABS;
    pn^.procType := builtinProcTypeNode;
    EndProc(pn,nil,nil);

    AddText('ASSERT');
    name := NewString;
    pn := DefineProc(name,TKPROCEDURE);
    pn^.builtin := BIPASSERT;
    pn^.procType := builtinProcTypeNode;
    EndProc(pn,nil,nil);

    AddText('CAP');
    name := NewString;
    pn := DefineProc(name,TKPROCEDURE);
    pn^.builtin := BIPCAP;
    pn^.procType := builtinProcTypeNode;
    EndProc(pn,nil,nil);

    AddText('CHR');
    name := NewString;
    pn := DefineProc(name,TKPROCEDURE);
    pn^.builtin := BIPCHR;
    pn^.procType := builtinProcTypeNode;
    EndProc(pn,nil,nil);

    AddText('DEC');
    name := NewString;
    pn := DefineProc(name,TKPROCEDURE);
    pn^.builtin := BIPDEC;
    pn^.procType := builtinProcTypeNode;
    EndProc(pn,nil,nil);

    AddText('DISPOSE');
    name := NewString;
    pn := DefineProc(name,TKPROCEDURE);
    pn^.builtin := BIPDISPOSE;
    pn^.procType := builtinProcTypeNode;
    EndProc(pn,nil,nil);

    AddText('EXCL');
    name := NewString;
    pn := DefineProc(name,TKPROCEDURE);
    pn^.builtin := BIPEXCL;
    pn^.procType := builtinProcTypeNode;
    EndProc(pn,nil,nil);

    AddText('FLOAT');
    name := NewString;
    pn := DefineProc(name,TKPROCEDURE);
    pn^.builtin := BIPFLOAT;
    pn^.procType := builtinProcTypeNode;
    EndProc(pn,nil,nil);

    AddText('LONGFLOAT');
    name := NewString;
    pn := DefineProc(name,TKPROCEDURE);
    pn^.builtin := BIPLONGFLOAT;
    pn^.procType := builtinProcTypeNode;
    EndProc(pn,nil,nil);

    AddText('HALT');
    name := NewString;
    pn := DefineProc(name,TKPROCEDURE);
    pn^.builtin := BIPHALT;
    pn^.procType := builtinProcTypeNode;
    EndProc(pn,nil,nil);

    AddText('HIGH');
    name := NewString;
    pn := DefineProc(name,TKPROCEDURE);
    pn^.builtin := BIPHIGH;
    pn^.procType := builtinProcTypeNode;
    EndProc(pn,nil,nil);

    AddText('NUMBER');
    name := NewString;
    pn := DefineProc(name,TKPROCEDURE);
    pn^.builtin := BIPNUMBER;
    pn^.procType := builtinProcTypeNode;
    EndProc(pn,nil,nil);

    AddText('INC');
    name := NewString;
    pn := DefineProc(name,TKPROCEDURE);
    pn^.builtin := BIPINC;
    pn^.procType := builtinProcTypeNode;
    EndProc(pn,nil,nil);

    AddText('INCL');
    name := NewString;
    pn := DefineProc(name,TKPROCEDURE);
    pn^.builtin := BIPINCL;
    pn^.procType := builtinProcTypeNode;
    EndProc(pn,nil,nil);

    AddText('NEW');
    name := NewString;
    pn := DefineProc(name,TKPROCEDURE);
    pn^.builtin := BIPNEW;
    pn^.procType := builtinProcTypeNode;
    EndProc(pn,nil,nil);

    AddText('MAX');
    name := NewString;
    pn := DefineProc(name,TKPROCEDURE);
    pn^.builtin := BIPMAX;
    pn^.procType := builtinProcTypeNode;
    EndProc(pn,nil,nil);

    AddText('MIN');
    name := NewString;
    pn := DefineProc(name,TKPROCEDURE);
    pn^.builtin := BIPMIN;
    pn^.procType := builtinProcTypeNode;
    EndProc(pn,nil,nil);

    AddText('ODD');
    name := NewString;
    pn := DefineProc(name,TKPROCEDURE);
    pn^.builtin := BIPODD;
    pn^.procType := builtinProcTypeNode;
    EndProc(pn,nil,nil);

    AddText('ORD');
    name := NewString;
    pn := DefineProc(name,TKPROCEDURE);
    pn^.builtin := BIPORD;
    pn^.procType := builtinProcTypeNode;
    EndProc(pn,nil,nil);

    AddText('TRUNC');
    name := NewString;
    pn := DefineProc(name,TKPROCEDURE);
    pn^.builtin := BIPTRUNC;
    pn^.procType := builtinProcTypeNode;
    EndProc(pn,nil,nil);

    AddText('VAL');
    name := NewString;
    pn := DefineProc(name,TKPROCEDURE);
    pn^.builtin := BIPVAL;
    pn^.procType := builtinProcTypeNode;
    EndProc(pn,nil,nil);

    AddText('FIRST');
    name := NewString;
    pn := DefineProc(name,TKPROCEDURE);
    pn^.builtin := BIPFIRST;
    pn^.procType := builtinProcTypeNode;
    EndProc(pn,nil,nil);

    AddText('LAST');
    name := NewString;
    pn := DefineProc(name,TKPROCEDURE);
    pn^.builtin := BIPLAST;
    pn^.procType := builtinProcTypeNode;
    EndProc(pn,nil,nil);

    currScope := saveScope;

    if generateBlockNumber > MAXBUILTINSCOPES then begin
	Error('Compiler error: too many builtin scopes');
	exit(99);
    end;
    generateBlockNumber := MAXBUILTINSCOPES+1;

    InitUnixProcNames;

end;

procedure InitStandardTypes;
var
    sym : Symbol;
    cn : ConstNode;
    error : boolean;
begin
    error := false;
    integerTypeNode := NewTypeNode(DTINTEGER);
    integerTypeNode^.size := WORDSIZE;
    AddText('INTEGER');
    error := error or not DefineSymbol(sym,NewString,builtinScope,ANYCASE);
    sym^.kind := SYMTYPE;
    sym^.symType := integerTypeNode;

    cardinalTypeNode := NewTypeNode(DTCARDINAL);
    cardinalTypeNode^.size := WORDSIZE;

    cardIntTypeNode := NewTypeNode(DTINTEGER);
    cardIntTypeNode^.size := WORDSIZE;

    AddText('CARDINAL');
    error := error or not DefineSymbol(sym,NewString,builtinScope,ANYCASE);
    sym^.kind := SYMTYPE;
    if standardCardinalFlag then begin
	sym^.symType := cardinalTypeNode;
    end else begin
	sym^.symType := cardIntTypeNode;
    end;

    AddText('UNSIGNED');
    error := error or not DefineSymbol(sym,NewString,builtinScope,ANYCASE);
    sym^.kind := SYMTYPE;
    sym^.symType := cardinalTypeNode;

    charTypeNode := NewTypeNode(DTCHAR);
    case target of
	TARGETVAX : begin
	    charTypeNode^.size := CHARSIZE;
	end;
	TARGETTITAN : begin
	    charTypeNode^.size := WORDSIZE;
	end;
    end;
    AddText('CHAR');
    error := error or not DefineSymbol(sym,NewString,builtinScope,ANYCASE);
    sym^.kind := SYMTYPE;
    sym^.symType := charTypeNode;

    charConstTypeNode := NewTypeNode(DTCHAR);
    case target of
	TARGETVAX : begin
	    charConstTypeNode^.size := CHARSIZE;
	end;
	TARGETTITAN : begin
	    charConstTypeNode^.size := WORDSIZE;
	end;
    end;

    realTypeNode := NewTypeNode(DTREAL);
    realTypeNode^.size := WORDSIZE;
    AddText('REAL');
    error := error or not DefineSymbol(sym,NewString,builtinScope,ANYCASE);
    sym^.kind := SYMTYPE;
    sym^.symType := realTypeNode;

    longrealTypeNode := NewTypeNode(DTLONGREAL);
    longrealTypeNode^.size := 2*WORDSIZE;
    AddText('LONGREAL');
    error := error or not DefineSymbol(sym,NewString,builtinScope,ANYCASE);
    sym^.kind := SYMTYPE;
    sym^.symType := longrealTypeNode;

    realConstTypeNode := NewTypeNode(DTLONGREAL);
    realConstTypeNode^.size := 2*WORDSIZE;

    stringTypeNode := NewTypeNode(DTSTRING);
    stringTypeNode^.stringLength := 0;
    stringTypeNode^.size := 0;

    bitsetTypeNode := NewTypeNode(DTSET);
    bitsetTypeNode^.size := WORDSIZE;
    bitsetTypeNode^.setRange := MakeSubrange(0,WORDSIZE-1,cardIntTypeNode);
    AddText('BITSET');
    error := error or not DefineSymbol(sym,NewString,builtinScope,ANYCASE);
    sym^.kind := SYMTYPE;
    sym^.symType := bitsetTypeNode;

    booleanTypeNode := NewTypeNode(DTBOOLEAN);
    case target of
	TARGETVAX : begin
	    booleanTypeNode^.size := BOOLEANSIZE;
	end;
	TARGETTITAN : begin
	    booleanTypeNode^.size := WORDSIZE;
	end;
    end;
    AddText('BOOLEAN');
    error := error or not DefineSymbol(sym,NewString,builtinScope,ANYCASE);
    sym^.kind := SYMTYPE;
    sym^.symType := booleanTypeNode;

    new(cn);
    cn^.kind := DTBOOLEAN;
    cn^.boolVal := true;
    AddText('TRUE');
    error := error or not DefineSymbol(sym,NewString,builtinScope,ANYCASE);
    sym^.kind := SYMCONST;
    sym^.symConst := cn;

    new(cn);
    cn^.kind := DTBOOLEAN;
    cn^.boolVal := false;
    AddText('FALSE');
    error := error or not DefineSymbol(sym,NewString,builtinScope,ANYCASE);
    sym^.kind := SYMCONST;
    sym^.symConst := cn;

    procTypeNode := NewTypeNode(DTPROC);
    procTypeNode^.size := WORDSIZE;
    procTypeNode^.paramList := nil;
    procTypeNode^.funcType := nil;
    AddText('PROC');
    error := error or not DefineSymbol(sym,NewString,builtinScope,ANYCASE);
    sym^.kind := SYMTYPE;
    sym^.symType := procTypeNode;

    builtinProcTypeNode := NewTypeNode(DTPROC);
    builtinProcTypeNode^.size := WORDSIZE;
    builtinProcTypeNode^.paramList := nil;
    builtinProcTypeNode^.funcType := nil;

    new(cn);
    cn^.kind := DTPOINTER;
    AddText('NIL');
    error := error or not DefineSymbol(sym,NewString,builtinScope,ANYCASE);
    sym^.kind := SYMCONST;
    sym^.symConst := cn;

    opaqueTypeNode := NewTypeNode(DTOPAQUE);
    AddText('_opaque_');
    opaqueTypeNode^.opaqueName := NewString;
    opaqueTypeNode^.size := WORDSIZE;

    wordTypeNode := NewTypeNode(DTWORD);
    wordTypeNode^.size := WORDSIZE;

    byteTypeNode := NewTypeNode(DTBYTE);
    byteTypeNode^.size := BYTESIZE;

    addressTypeNode := PointerType(wordTypeNode,TKATNONE);

    anyTypeNode := NewTypeNode(DTANY);
    anyTypeNode^.size := WORDSIZE;

    nullTypeNode := NewTypeNode(DTNULL);
    nullTypeNode^.size := 0;

    fileTypeNode := PointerType(opaqueTypeNode,TKATNONE);

    processTypeNode := NewTypeNode(DTOPAQUE);
    processTypeNode^.size := WORDSIZE;
    AddText('SYSTEM_PROCESS');
    processTypeNode^.opaqueName := NewString;

    arrayOfCharTypeNode := ArrayType(nil, charTypeNode, TKARRAY, TKNULL);

    indexableTypes := [DTINTEGER, DTCARDINAL, DTCHAR, DTBOOLEAN, DTENUMERATION,
				DTSUBRANGE];
    if error then begin
	Error('Compiler error:  Cannot initialize builtin types');
	exit(99);
    end;
end;

function CheckWritef(en : ExprNode; format : String; pl : ExprNode;
	    var newFormat : String) : boolean;
const
    FMTCHAR = '%';
var
    ok, lasttime, long : boolean;
    pen : ExprNode;
    pentn : TypeNode;
    currPos, formatSize : integer;
    currChar : char;
    currCharacter : character;

    procedure FmtErr(en : ExprNode; msg : ErrorString);
    var
	i, j : integer;
	tmp : ErrorString;
    begin
	tmp := ', character #'; { 13 chars, see below }
	i := ERRORSTRINGSIZE;
	while (i > 1) and (msg[i] = ' ') do begin
	    i := i - 1;
	end;
	for j := 1 to 13 do begin
	    msg[i+j] := tmp[j];
	end;
	ExprErrorNumber(en,msg,currPos);
	ok := false;
    end;

    procedure Advance;
    begin
	if currPos >= formatSize then begin
	    if currChar <> chr(0) then begin
		FmtErr(en,'Premature end of format');
	    end;
	    currChar := chr(0);
	end else begin
	    if currPos > 0 then begin
		AddCharX(currCharacter);
	    end;
	    currCharacter := GetCharX(format,currPos);
	    if currCharacter in [MINCHAR..MAXCHAR] then begin
		currChar := chr(currCharacter);
	    end else begin
		currChar := '?';
	    end;
	    currPos := currPos + 1;
	end;
    end;

    procedure CheckWidth;
    var
	tn : TypeNode;
    begin
	if currChar = '-' then begin
	    Advance;
	end;
	lasttime := true;
	repeat
	    if currChar = '*' then begin
		Advance;
		if pen = nil then begin
		    ok := false;
		    FmtErr(en,'No parameter for field width');
		end else begin
		    tn := BaseType(CheckExpr(pen,EVALGET));
		    if IsBadExpr(pen) then begin
			ok := false;	{ error already printed }
		    end else if not (tn^.kind in [DTCARDINAL,DTINTEGER])
		    then begin
			FmtErr(en, 'Field width not integer or cardinal');
		    end;
		    pen := pen^.next;
		end;
	    end else begin
		while currChar in ['0'..'9'] do begin
		    Advance;
		end;
	    end;
	    if lasttime and (currChar = '.') then begin
		Advance;
		lasttime := false;
	    end else begin
		lasttime := true;
	    end;
	until lasttime;
    end;
begin
    ok := true;
    pen := pl;
    currPos := 0;
    formatSize := format^.length;
    while ok and (currPos < formatSize) do begin
	Advance;
	if currChar <> FMTCHAR then begin
	    { do nothing }
	end else begin
	    Advance;
	    if currChar = FMTCHAR then begin
		{ do nothing }
	    end else begin
		CheckWidth;
		if not ok then begin
		    { do nothing }
		end else if pen = nil then begin
		    FmtErr(en,'No parameter for format');
		end else begin
		    long := false;
		    if currChar = 'l' then begin
			Advance;
			long := true;
		    end;
		    if currChar in ['d','o','x','u'] then begin
			{ need an integer or cardinal }
			pentn := CheckExpr(pen,EVALGET);
			if IsBadExpr(pen) then begin
			    ok := false;
			end else if not Passable(integerTypeNode,PARAMVALUE,pentn,pen)
			then begin
			    FmtErr(en,'Format requires integer or cardinal');
			end;
			pen := pen^.next;
		    end else if (not long) and (currChar in ['f','e','g'])
		    then begin
			{ need a real }
			pentn := CheckExpr(pen,EVALGET);
			if IsBadExpr(pen) then begin
			    ok := false;
			end else if not Passable(realTypeNode,PARAMVALUE,pentn,pen)
			then begin
			    FmtErr(en,'Format requires real');
			end;
			pen := pen^.next;
		    end else if (currChar in ['F','E','G']) or
				(long and (currChar in ['f','e','g']))
		    then begin
			if currChar in ['F','E','G'] then begin
			    currCharacter := ord(currChar)-ord('A')+ord('a');
			end;
			{ need a real }
			pentn := CheckExpr(pen,EVALGET);
			if IsBadExpr(pen) then begin
			    ok := false;
			end else if not Passable(longrealTypeNode,PARAMVALUE,pentn,pen)
			then begin
			    FmtErr(en,'Format requires longreal');
			end;
			pen := pen^.next;
		    end else if currChar = 'c' then begin
			{ need a char }
			pentn := CheckExpr(pen,EVALGET);
			if IsBadExpr(pen) then begin
			    ok := false;
			end else if not Passable(charTypeNode,PARAMVALUE,pentn,pen)
			then begin
			    FmtErr(en,'Format requires char');
			end;
			pen := pen^.next;
		    end else if currChar = 's' then begin
			{ need a string }
			pentn := CheckExpr(pen,EVALPOINT);
			if IsBadExpr(pen) then begin
			    ok := false;
			end else if not Passable(arrayOfCharTypeNode,PARAMARRAYVALUE,pentn,pen)
			then begin
			    FmtErr(en,'Format requires string');
			end else begin
			    RefOpenArray(pen,pentn);
			end;
			pen := pen^.next;
		    end else begin
			FmtErr(en,'Invalid format character');
		    end;
		end;
	    end;
	end;
    end;
    if ok and (pen <> nil) then begin
	ok := false;
	ExprError(en,'Too many parameters for format string');
    end;
    AddChar(currChar);
    newFormat := NewString;
    CheckWritef := ok;
end;

function CheckReadf(en : ExprNode; format : String; pl : ExprNode) : boolean;
const
    FMTCHAR = '%';
    SUPPRESSCHAR = '*';
    LBRACK = '[';
    RBRACK = ']';
var
    ok, suppress, long : boolean;
    pen : ExprNode;
    pentn : TypeNode;
    currPos, formatSize : integer;
    currChar : char;
    currCharacter : character;

    procedure FmtErr(en : ExprNode; msg : ErrorString);
    var
	i, j : integer;
	tmp : ErrorString;
    begin
	tmp := ', character #'; { 13 chars, see below }
	i := ERRORSTRINGSIZE;
	while (i > 1) and (msg[i] = ' ') do begin
	    i := i - 1;
	end;
	for j := 1 to 13 do begin
	    msg[i+j] := tmp[j];
	end;
	ExprErrorNumber(en,msg,currPos);
	ok := false;
    end;

    procedure Advance;
    begin
	if currPos >= formatSize then begin
	    if currChar <> chr(0) then begin
		FmtErr(en,'Premature end of format');
	    end;
	    currChar := chr(0);
	end else begin
	    currCharacter := GetCharX(format,currPos);
	    if currCharacter in [MINCHAR..MAXCHAR] then begin
		currChar := chr(currCharacter);
	    end else begin
		currChar := '?';
	    end;
	    currPos := currPos + 1;
	end;
    end;


    procedure CheckWidth;
    begin
	while currChar in ['0'..'9'] do begin
	    Advance;
	end;
    end;

    procedure CheckVarParam(en : ExprNode);
    begin
	if not IsAddressableExpr(en) then begin
	    FmtErr(en,'Format requires var parameter');
	end;
    end;
begin
    ok := true;
    pen := pl;
    formatSize := format^.length;
    currPos := 0;
    while ok and (currPos < formatSize) do begin
	Advance;
	if currChar <> FMTCHAR then begin
	    { do nothing }
	end else begin
	    Advance;
	    if currChar = FMTCHAR then begin
		{ do nothing }
	    end else begin
		if currChar = SUPPRESSCHAR then begin
		    suppress := true;
		    Advance;
		end else begin
		    suppress := false;
		end;
		CheckWidth;
		if not ok then begin
		    { do nothing }
		end else if not suppress and (pen = nil) then begin
		    FmtErr(en,'No parameter for format');
		end else begin
		    long := false;
		    if currChar = 'l' then begin
			Advance;
			long := true;
		    end;
		    if currChar in ['d','o','x','u'] then begin
			if not suppress then begin
			    { need an integer or cardinal }
			    pentn := CheckExpr(pen,EVALPUT);
			    if IsBadExpr(pen) then begin
				ok := false;
			    end else if Assignable(integerTypeNode,pentn,pen) = nil
			    then begin
				FmtErr(en,'Format requires integer or cardinal');
			    end;
			    CheckVarParam(pen);
			    pen := pen^.next;
			end;
		    end else if (not long) and (currChar in ['f','e'])
		    then begin
			if not suppress then begin
			    { need a real }
			    pentn := CheckExpr(pen,EVALPUT);
			    if IsBadExpr(pen) then begin
				ok := false;
			    end else if not Passable(realTypeNode,PARAMVAR,pentn,pen)
			    then begin
				FmtErr(en,'Format requires real');
			    end;
			    CheckVarParam(pen);
			    pen := pen^.next;
			end;
		    end else if (currChar in ['F','E']) or
				(long and (currChar in ['f','e']))
		    then begin
			if not suppress then begin
			    { need a longreal }
			    pentn := CheckExpr(pen,EVALPUT);
			    if IsBadExpr(pen) then begin
				ok := false;
			    end else if not Passable(longrealTypeNode,PARAMVAR,pentn,pen)
			    then begin
				FmtErr(en,'Format requires longreal');
			    end;
			    CheckVarParam(pen);
			    pen := pen^.next;
			end;
		    end else if currChar = 'c' then begin
			if not suppress then begin
			    { need a char }
			    pentn := CheckExpr(pen,EVALPUT);
			    if IsBadExpr(pen) then begin
				ok := false;
			    end else if not Passable(charTypeNode,PARAMVAR,pentn,pen)
			    then begin
				FmtErr(en,'Format requires char');
			    end;
			    CheckVarParam(pen);
			    pen := pen^.next;
			end;
		    end else if currChar = 's' then begin
			if not suppress then begin
			    { need a string }
			    pentn := CheckExpr(pen,EVALPUT);
			    if IsBadExpr(pen) then begin
				ok := false;
			    end else if not Passable(arrayOfCharTypeNode,PARAMARRAYVAR,pentn,pen)
			    then begin
				FmtErr(en,'Format requires string');
			    end else begin
				RefOpenArray(pen,pentn);
				CheckVarParam(pen);
			    end;
			    pen := pen^.next;
			end;
		    end else if currChar = LBRACK then begin
			while (currChar <> chr(0)) and (currChar <> RBRACK) do begin
			    Advance;
			end;
			if currChar <> RBRACK then begin
			    FmtErr(en,'Missing ]');
			end;
			if not suppress then begin
			    { need a string }
			    pentn := CheckExpr(pen,EVALPUT);
			    if IsBadExpr(pen) then begin
				ok := false;
			    end else if not Passable(arrayOfCharTypeNode,PARAMARRAYVAR,pentn,pen)
			    then begin
				FmtErr(en,'Format requires string');
			    end else begin
				RefOpenArray(pen,pentn);
				CheckVarParam(pen);
			    end;
			    pen := pen^.next;
			end;
		    end else begin
			FmtErr(en,'Invalid format character');
		    end;
		end;
	    end;
	end;
    end;
    if ok and (pen <> nil) then begin
	ok := false;
	ExprError(en,'Too many parameters for format string');
    end;
    CheckReadf := ok;
end;

procedure PointerOrAddress(en : ExprNode; tn : TypeNode);
begin
    if tn^.kind = DTPOINTER then begin
	ValueOrAddr(en,addressTypeNode,EVALGET);
    end else begin
	RefOpenArray(en,tn);
    end;
end;

function CheckBuiltin{(isFunc : boolean; procExpr : ExprNode;
	var proc : ProcNode; params : ExprList; var retType : TypeNode)
	: boolean};
type
    ErrorKind = (ERRNONE, ERROTHER, ERRPROC, ERRFUNC, ERRNUMP, ERRTYPE1,
	ERRTYPE2, ERRTYPE3, ERRTYPE4, ERRMETHOD1, ERRMETHOD2, ERRMETHOD4);
var
    p, p1, p2, p3, p4, nen : ExprNode;
    ptn, pt1, pt2, pt3, pt4, pt1b : TypeNode;
    nump : integer;
    error : ErrorKind;
    value, lowerBound : cardinal;
    sym : Symbol;
    newProc : ProcNode;
    pn1, pn2 : ParamNode;
    cn : ConstNode;
begin
    error := ERRNONE;
    nump := 0;
    p1 := nil;
    p2 := nil;
    pt1 := nil;
    pt2 := nil;
    if params = nil then begin
	{ do nothing }
    end else if params^.first = nil then begin
	{ do nothing }
    end else begin
	p1 := params^.first;
	if p1^.next = nil then begin
	    nump := 1;
	end else begin
	    p2 := p1^.next;
	    if p2^.next = nil then begin
		nump := 2;
	    end else begin
		p := params^.first;
		nump := 0;
		while p <> nil do begin
		    nump := nump + 1;
		    p := p^.next;
		end;
	    end;
	end;
    end;
    case proc^.builtin of
	BIPABS : begin
	    if not isFunc then begin
		error := ERRPROC;
	    end else if nump <> 1 then begin
		error := ERRNUMP;
	    end else begin
		pt1 := BaseType(CheckExpr(p1,EVALGET));
		if IsBadExpr(p1) then begin
		    error := ERROTHER;
		end else if not (pt1^.kind in [DTINTEGER, DTREAL, DTLONGREAL]) then begin
		    error := ERRTYPE1;
		end else begin
		    retType := pt1;
		end;
	    end;
	end;
	BIPMIN, BIPMAX : begin
	    if not isFunc then begin
		error := ERRPROC;
	    end else if nump <> 2 then begin
		error := ERRNUMP;
	    end else begin
		pt1 := BaseType(CheckExpr(p1,EVALGET));
		pt2 := BaseType(CheckExpr(p2,EVALGET));
		if IsBadExpr(p1) or IsBadExpr(p2) then begin
		    error := ERROTHER;
		end else if pt1^.kind <> pt2^.kind then begin
		    ErrorName(proc^.name,'Both parameters must be same type');
		    error := ERROTHER;
		end else if not (pt1^.kind in [DTCARDINAL, DTINTEGER, DTREAL, DTLONGREAL]) then begin
		    error := ERRTYPE1;
		end else begin
		    retType := pt1;
		end;
	    end;
	end;
	BIPASSERT : begin
	    if isFunc then begin
		error := ERRFUNC;
	    end else if nump in [1,2] then begin
		pt1 := BaseType(CheckExpr(p1,EVALGET));
		if IsBadExpr(p1) then begin
		    error := ERROTHER;
		end else if pt1^.kind <> DTBOOLEAN then begin
		    error := ERRTYPE1;
		end else if nump = 1 then begin
		    new(cn);
		    cn^.kind := DTSTRING;
		    cn^.strVal := NewString;
		    p2 := ConstExprNode(cn);
		    p2^.exprType := ConstType(cn);
		    params := AddToExprList(params,p2);
		end else begin
		    pt2 := CheckExpr(p2,EVALPOINT);
		    if IsBadExpr(p2) then begin
			error := ERROTHER;
		    end else if not Passable(arrayOfCharTypeNode,PARAMARRAYVALUE,pt2,p2)
		    then begin
			error := ERRTYPE2;
		    end else begin
			RefOpenArray(p2,pt2);
		    end;
		end;
	    end else begin
		error := ERRNUMP;
	    end;
	    retType := nil;
	end;
	BIPCAP : begin
	    if not isFunc then begin
		error := ERRPROC;
	    end else if nump <> 1 then begin
		error := ERRNUMP;
	    end else begin
		pt1 := BaseType(CheckExpr(p1,EVALGET));
		if IsBadExpr(p1) then begin
		    error := ERROTHER;
		end else if pt1^.kind <> DTCHAR then begin
		    error := ERRTYPE1;
		end else begin
		    retType := charTypeNode;
		end;
	    end;
	end;
	BIPCHR : begin
	    if not isFunc then begin
		error := ERRPROC;
	    end else if nump <> 1 then begin
		error := ERRNUMP;
	    end else begin
		pt1 := BaseType(CheckExpr(p1,EVALGET));
		if IsBadExpr(p1) then begin
		    error := ERROTHER;
		end else if not (pt1^.kind in [DTCARDINAL,DTINTEGER,DTWORD,DTBYTE]) then begin
		    error := ERRTYPE1;
		end else begin
		    retType := charTypeNode;
		end;
	    end;
	end;
	BIPDEC, BIPINC : begin
	    if isFunc then begin
		error := ERRFUNC;
	    end else if nump in [1,2] then begin
		pt1 := BaseType(CheckExpr(p1,EVALPOINT));
		if IsBadExpr(p1) then begin
		    error := ERROTHER;
		end else if not (pt1^.kind in indexableTypes) then begin
		    error := ERRTYPE1;
		end else if not IsAddressableExpr(p1) then begin
		    error := ERRMETHOD1;
		end else if nump = 1 then begin
		    p2 := ConstExprNode(CardinalConst(1));
		    p2^.exprType := cardIntTypeNode;
		    params := AddToExprList(params,p2);
		end else begin
		    pt2 := BaseType(CheckExpr(p2,EVALGET));
		    if IsBadExpr(p2) then begin
			error := ERROTHER;
		    end else if not (pt2^.kind in [DTCARDINAL, DTINTEGER]) then begin
			error := ERRTYPE2;
		    end;
		end;
	    end else begin
		error := ERRNUMP;
	    end;
	    if error = ERRNONE then begin
		p3 := ConstExprNode(CardinalConst(SizeOf(pt1)));
		p3^.exprType := cardIntTypeNode;
		params := AddToExprList(params,p3);
		retType := pt1;
	    end;
	end;
	BIPDISPOSE, BIPNEW : begin
	    if isFunc then begin
		error := ERRFUNC;
	    end else if nump < 1 then begin
		error := ERRNUMP;
	    end else begin
		pt1 := BaseType(CheckExpr(p1,EVALPOINT));
		if IsBadExpr(p1) then begin
		    error := ERROTHER;
		end else if pt1^.kind <> DTPOINTER then begin
		    error := ERRTYPE1;
		end else if not IsAddressableExpr(p1) then begin
		    error := ERRMETHOD1;
		end else begin
		    value := WordSizeOf(pt1^.toType);
		    if pt1^.ptrCheck = CHECKPTRMODULA then begin
			{ extra word needed for modula pointer check }
			value := value + WORDSIZE;
		    end else if pt1^.ptrCheck in [CHECKPTRPASCAL,CHECKPTRC]
		    then begin
			{ Pascal allocator works in bytes }
			value := CardDiv(value,BYTESIZE);
		    end;
		    { ignore any parameters after the first }
		    p1^.next := nil;
		    params^.last := p1;
		    { add length as second parameter }
		    p2 := ConstExprNode(CardinalConst(value));
		    p2^.exprType := cardIntTypeNode;
		    params := AddToExprList(params,p2);
		    p3 := ConstExprNode(CardinalConst(ord(pt1^.ptrCheck)));
		    p3^.exprType := cardIntTypeNode;
		    params := AddToExprList(params,p3);
		    retType := nil;
		    if proc^.builtin = BIPNEW then begin
			sym := LookUpSymbol(allocateString,nil,ANYCASE);
		    end else begin
			sym := LookUpSymbol(deallocateString,nil,ANYCASE);
		    end;
		    if sym = nil then begin
			{ just use the default }
		    end else if sym^.kind <> SYMPROC then begin
			error := ERROTHER;
		    end else if sym^.symProc^.builtin = BIPALLOCATE then begin
			{ just use the default }
		    end else if sym^.symProc^.builtin = BIPDEALLOCATE then begin
			{ just use the default }
		    end else begin
			newProc := sym^.symProc;
			if newProc^.procType^.paramList = nil then begin
			    error := ERROTHER;
			end else begin
			    pn1 := newProc^.procType^.paramList^.first;
			    if pn1 = nil then begin
				error := ERROTHER;
			    end else begin
				pn2 := pn1^.next;
				if pn2 = nil then begin
				    error := ERROTHER;
				end else begin
				    if (pn1^.paramType <> addressTypeNode) or
					not (pn2^.paramType^.kind in
						    [DTCARDINAL,DTINTEGER]) or
					(pn2^.next <> nil)
				    then begin
					error := ERROTHER;
				    end else begin
					{ use local allocate/deallocate }
					{ instead of new/dispose }
					new(cn);
					cn^.kind := DTPROC;
					cn^.procVal := sym^.symProc;
					p4 := ConstExprNode(cn);
					pt4 := CheckExpr(p4,EVALPOINT);
					params := AddToExprList(params,p4);
				    end;
				end;
			    end;
			end;
		    end;
		    if error = ERROTHER then begin
			ExprErrorName(p1,sym^.name,
			    'wrong procedure type for new/dispose substitution');
		    end;
		end;
	    end;
	end;
	BIPALLOCATE, BIPDEALLOCATE: begin
	    if isFunc then begin
		error := ERRFUNC;
	    end else if nump <> 2 then begin
		error := ERRNUMP;
	    end else begin
		pt1 := BaseType(CheckExpr(p1,EVALPUT));
		pt2 := BaseType(CheckExpr(p2,EVALGET));
		if IsBadExpr(p1) or IsBadExpr(p2) then begin
		    error := ERROTHER;
		end else if pt1^.kind <> DTPOINTER then begin
		    error := ERRTYPE1;
		end else if not IsAddressableExpr(p1) then begin
		    error := ERRMETHOD1;
		end else if not Passable(cardinalTypeNode,PARAMVALUE,pt2,p2)
		then begin
		    error := ERRTYPE2;
		end else begin
		    retType := nil;
		end;
	    end;
	end;
	BIPEXCL, BIPINCL : begin
	    if isFunc then begin
		error := ERRFUNC;
	    end else if nump <> 2 then begin
		error := ERRNUMP;
	    end else begin
		pt1 := CheckExpr(p1,EVALPOINT);
		pt1b := BaseType(pt1);
		pt2 := BaseType(CheckExpr(p2,EVALGET));
		if IsBadExpr(p1) or IsBadExpr(p2) then begin
		    error := ERROTHER;
		end else if pt1b^.kind <> DTSET then begin
		    error := ERRTYPE1;
		end else if not IsAddressableExpr(p1) then begin
		    error := ERRMETHOD1;
		end else if not Passable(pt1b^.setRange,PARAMVALUE,pt2,p2)
		then begin
		    error := ERRTYPE2;
		end else begin
		    lowerBound := LowerBoundOf(pt1b^.setRange);
		    if lowerBound <> 0 then begin
			{ replace p2 with (p2 - lowerBound) }
			nen := NewExprNode(p2^.kind);
			nen^ := p2^;
			p2^.kind := EXPRBINOP;
			p2^.exprBinOp := TKMINUS;
			p2^.opnd1 := nen;
			p2^.opnd2 := ConstExprNode(CardinalConst(lowerBound));
			p2^.opnd2^.exprType := nen^.exprType;
			p2^.opnd2^.operType := nen^.exprType;
		    end;
		    p3 := ConstExprNode(CardinalConst(SizeOf(pt1)));
		    p3^.exprType := cardIntTypeNode;
		    params := AddToExprList(params,p3);
		    retType := nil;
		end;
	    end;
	end;
	BIPFLOAT, BIPLONGFLOAT : begin
	    if not isFunc then begin
		error := ERRPROC;
	    end else if nump <> 1 then begin
		error := ERRNUMP;
	    end else begin
		pt1 := BaseType(CheckExpr(p1,EVALGET));
		if IsBadExpr(p1) then begin
		    error := ERROTHER;
		end else if not (pt1^.kind in [DTINTEGER, DTCARDINAL, DTREAL,
			DTLONGREAL])
		then begin
		    error := ERRTYPE1;
		end else if proc^.builtin = BIPFLOAT then begin
		    retType := realTypeNode;
		end else begin
		    retType := longrealTypeNode;
		end;
	    end;
	end;
	BIPHALT : begin
	    if isFunc then begin
		error := ERRFUNC;
	    end else if nump <> 0 then begin
		error := ERRNUMP;
	    end else begin
		retType := nil;
	    end;
	end;
	BIPHIGH, BIPNUMBER : begin
	    if not isFunc then begin
		error := ERRPROC;
	    end else if nump <> 1 then begin
		error := ERRNUMP;
	    end else begin
		pt1 := BaseType(CheckExpr(p1,EVALPOINT));
		if IsBadExpr(p1) then begin
		    error := ERROTHER;
		end else if pt1^.kind <> DTARRAY then begin
		    error := ERRTYPE1;
		end else if pt1^.indexType <> nil then begin
		    { constant: evaluate now }
		    if proc^.builtin = BIPHIGH then begin
			value := UpperBoundOf(pt1^.indexType);
		    end else begin
			value := NumberOf(pt1^.indexType);
		    end;
		    procExpr^.kind := EXPRCONST;
		    procExpr^.exprConst := CardinalConst(value);
		    if value >= 0 then begin
			procExpr^.exprType := cardIntTypeNode;
		    end else begin
			procExpr^.exprType := integerTypeNode;
		    end;
		    retType := procExpr^.exprType;	
		end else if pt1^.nocount then begin
		    ExprError(procExpr,'Cannot take high of NOCOUNT array');
		    error := ERROTHER;
		end else begin
		    retType := cardIntTypeNode;	
		end;
	    end;
	end;
	BIPODD : begin
	    if not isFunc then begin
		error := ERRPROC;
	    end else if nump <> 1 then begin
		error := ERRNUMP;
	    end else begin
		pt1 := BaseType(CheckExpr(p1,EVALGET));
		if IsBadExpr(p1) then begin
		    error := ERROTHER;
		end else if not (pt1^.kind in [DTCARDINAL, DTINTEGER]) then begin
		    error := ERRTYPE1;
		end else begin
		    retType := booleanTypeNode;
		end;
	    end;
	end;
	BIPORD : begin
	    if not isFunc then begin
		error := ERRPROC;
	    end else if nump <> 1 then begin
		error := ERRNUMP;
	    end else begin
		pt1 := BaseType(CheckExpr(p1,EVALGET));
		if IsBadExpr(p1) then begin
		    error := ERROTHER;
		end else if not (pt1^.kind in indexableTypes) then begin
		    error := ERRTYPE1;
		end else begin
		    retType := cardIntTypeNode;
		end;
	    end;
	end;
	BIPTRUNC : begin
	    if not isFunc then begin
		error := ERRPROC;
	    end else if nump <> 1 then begin
		error := ERRNUMP;
	    end else begin
		pt1 := BaseType(CheckExpr(p1,EVALGET));
		if IsBadExpr(p1) then begin
		    error := ERROTHER;
		end else if not (pt1^.kind in [DTREAL,DTLONGREAL]) then begin
		    error := ERRTYPE1;
		end else begin
		    retType := integerTypeNode;
		end;
	    end;
	end;
	BIPVAL : begin
	    if not isFunc then begin
		error := ERRPROC;
	    end else if nump <> 2 then begin
		error := ERRNUMP;
	    end else begin
		pt1 := BaseType(CheckExprType(p1,EVALGET));
		pt2 := BaseType(CheckExpr(p2,EVALGET));
		if IsBadExpr(p1) or IsBadExpr(p2) then begin
		    error := ERROTHER;
		end else if p1^.kind <> EXPRSYM then begin
		    error := ERRTYPE1;
		end else if p1^.exprSym^.kind <> SYMTYPE then begin
		    error := ERRTYPE1;
		end else if not (pt1^.kind in indexableTypes) then begin
		    error := ERRTYPE1;
		end else if not (pt2^.kind in [DTCARDINAL,DTINTEGER]) then begin
		    error := ERRTYPE2;
		end else begin
		    retType := pt1;
		end;
	    end;
	end;
	BIPADR : begin
	    if not isFunc then begin
		error := ERRPROC;
	    end else if nump <> 1 then begin
		error := ERRNUMP;
	    end else begin
		pt1 := BaseType(CheckExpr(p1,EVALPOINT));
		if IsBadExpr(p1) then begin
		    error := ERROTHER;
		end else if not IsAddressableExpr(p1) then begin
		    error := ERRMETHOD1;
		end else begin
		    RefOpenArray(p1,pt1);
		    retType := addressTypeNode;
		end;
	    end;
	end;
	BIPSIZE, BIPBYTESIZE : begin
	    if not isFunc then begin
		error := ERRPROC;
	    end else if nump <> 1 then begin
		error := ERRNUMP;
	    end else begin
		pt1 := CheckExpr(p1,EVALGET);
		if IsBadExpr(p1) then begin
		    error := ERROTHER;
		end else begin
		    { constant: evaluate now }
		    value := SizeOf(pt1);
		    if proc^.builtin = BIPBYTESIZE then begin
			value := CardDiv(value+BYTESIZE-1,BYTESIZE);
		    end;
		    procExpr^.kind := EXPRCONST;
		    procExpr^.exprConst := CardinalConst(value);
		    procExpr^.exprType := cardIntTypeNode;
		    retType := cardIntTypeNode;
		end;
	    end;
	end;
	BIPFIRST, BIPLAST : begin
	    if not isFunc then begin
		error := ERRPROC;
	    end else if nump <> 1 then begin
		error := ERRNUMP;
	    end else begin
		pt1 := CheckExprType(p1,EVALGET);
		if IsBadExpr(p1) then begin
		    error := ERROTHER;
		end else if p1^.kind <> EXPRSYM then begin
		    error := ERRTYPE1;
		end else if p1^.exprSym^.kind <> SYMTYPE then begin
		    error := ERRTYPE1;
		end else begin
		    { constant: evaluate now }
		    if proc^.builtin = BIPFIRST then begin
			value := LowerBoundOf(p1^.exprSym^.symType);
		    end else begin
			value := UpperBoundOf(p1^.exprSym^.symType);
		    end;
		    retType := p1^.exprSym^.symType;
		    new(cn);
		    if retType^.kind = DTCHAR then begin
			cn^.kind := DTCHAR;
			cn^.charVal := trunc(value);
		    end else if retType^.kind = DTBOOLEAN then begin
			cn^.kind := DTBOOLEAN;
			cn^.boolVal := value = 1.0;
		    end else begin
			{ enumerations and integers }
			cn^.kind := DTINTEGER;
			cn^.cardVal := trunc(value);
		    end;
		    procExpr^.kind := EXPRCONST;
		    procExpr^.exprConst := cn;
		    procExpr^.exprType := retType;
		end;
	    end;
	end;
	BIPTSIZE, BIPTBYTESIZE : begin
	    if not isFunc then begin
		error := ERRPROC;
	    end else if nump <> 1 then begin
		error := ERRNUMP;
	    end else begin
		pt1 := CheckExprType(p1,EVALGET);
		if IsBadExpr(p1) then begin
		    error := ERROTHER;
		end else if p1^.kind <> EXPRSYM then begin
		    error := ERRTYPE1;
		end else if p1^.exprSym^.kind <> SYMTYPE then begin
		    error := ERRTYPE1;
		end else begin
		    { constant: evaluate now }
		    value := SizeOf(p1^.exprSym^.symType);
		    if proc^.builtin = BIPTBYTESIZE then begin
			value := CardDiv(value+BYTESIZE-1,BYTESIZE);
		    end;
		    procExpr^.kind := EXPRCONST;
		    procExpr^.exprConst := CardinalConst(value);
		    procExpr^.exprType := cardIntTypeNode;
		    retType := cardIntTypeNode;
		end;
	    end;
	end;
	BIPUNIXCALL : begin
	    if not isFunc then begin
		error := ERRPROC;
	    end else if nump < 1 then begin
		error := ERRNUMP;
	    end else begin
		pt1 := BaseType(CheckExpr(p1,EVALGET));
		if IsBadExpr(p1) then begin
		    error := ERROTHER;
		end else if p1^.kind <> EXPRCONST then begin
		    error := ERRTYPE1;
		end else if not (pt1^.kind in [DTINTEGER, DTCARDINAL])
		then begin
		    error := ERRTYPE1;
		end else begin
		    if p2 <> nil then begin
			p := p2;
			while p <> nil do begin
			    ptn := CheckExpr(p,EVALGET);
			    if IsBadExpr(p1) then begin
				error := ERROTHER;
			    end;
			    p := p^.next;
			end;
		    end;
		    retType := integerTypeNode;
		end;
	    end;
	end;
	BIPCPUTIME : begin
	    if not isFunc then begin
		error := ERRPROC;
	    end else if nump <> 0 then begin
		error := ERRNUMP;
	    end else begin
		retType := cardIntTypeNode;
	    end;
	end;
	BIPWRITEF : begin
	    if isFunc then begin
		error := ERRFUNC;
	    end else if nump < 2 then begin
		error := ERRNUMP;
	    end else begin
		pt1 := BaseType(CheckExpr(p1,EVALGET));
		pt2 := CheckExpr(p2,EVALPOINT);
		if IsBadExpr(p1) or IsBadExpr(p2) then begin
		    error := ERROTHER;
		end else if pt1 <> fileTypeNode then begin
		    error := ERRTYPE1;
		end else if p2^.kind <> EXPRCONST then begin
		    error := ERRTYPE2;
		end else if not Passable(arrayOfCharTypeNode,PARAMARRAYVALUE,pt2,p2)
		then begin
		    error := ERRTYPE2;
		end else if p2^.exprConst^.kind = DTCHAR then begin
		    { one character string is OK }
		end else if p2^.exprConst^.kind <> DTSTRING then begin
		    error := ERRTYPE2;
		end else begin
		    { need to update format string }
		    new(cn);
		    cn^.kind := DTSTRING;
		    if not CheckWritef(p2,p2^.exprConst^.strVal,p2^.next,
						cn^.strVal)
		    then begin
			error := ERROTHER;
		    end else begin
			p2^.exprConst := cn;
		    end;
		end;
	    end;
	    retType := nil;
	end;
	BIPREADF : begin
	    if not isFunc then begin
		error := ERRPROC;
	    end else if nump < 2 then begin
		error := ERRNUMP;
	    end else begin
		pt1 := BaseType(CheckExpr(p1,EVALGET));
		pt2 := CheckExpr(p2,EVALPOINT);
		if IsBadExpr(p1) or IsBadExpr(p2) then begin
		    error := ERROTHER;
		end else if pt1 <> fileTypeNode then begin
		    error := ERRTYPE1;
		end else if not Passable(arrayOfCharTypeNode,PARAMARRAYVALUE,pt2,p2)
		then begin
		    error := ERRTYPE2;
		end else if p2^.kind <> EXPRCONST then begin
		    error := ERRTYPE2;
		end else if p2^.exprConst^.kind = DTCHAR then begin
		    { one character string is OK }
		end else if p2^.exprConst^.kind <> DTSTRING then begin
		    error := ERRTYPE2;
		end else if p2^.kind <> EXPRCONST then begin
		    error := ERRTYPE2;
		end else begin
		    if not CheckReadf(p2,p2^.exprConst^.strVal,p2^.next)
		    then begin
			error := ERROTHER;
		    end;
		    retType := integerTypeNode;
		end;
	    end;
	end;
	BIPWRITES : begin
	    if isFunc then begin
		error := ERRFUNC;
	    end else if nump < 2 then begin
		error := ERRNUMP;
	    end else begin
		pt1 := CheckExpr(p1,EVALPUT);
		pt2 := CheckExpr(p2,EVALPOINT);
		if IsBadExpr(p1) or IsBadExpr(p2) then begin
		    error := ERROTHER;
		end else if not Passable(arrayOfCharTypeNode,PARAMARRAYVAR,pt1,p1)
		then begin
		    error := ERRTYPE1;
		end else if p2^.kind <> EXPRCONST then begin
		    error := ERRTYPE2;
		end else if not Passable(arrayOfCharTypeNode,PARAMARRAYVALUE,pt2,p2)
		then begin
		    error := ERRTYPE2;
		end else if p2^.exprConst^.kind = DTCHAR then begin
		    { one character string is OK }
		end else if p2^.exprConst^.kind <> DTSTRING then begin
		    error := ERRTYPE2;
		end else begin
		    RefOpenArray(p1,pt1);
		    { need to update format string }
		    new(cn);
		    cn^.kind := DTSTRING;
		    if not CheckWritef(p2,p2^.exprConst^.strVal,p2^.next,
						cn^.strVal)
		    then begin
			error := ERROTHER;
		    end else begin
			p2^.exprConst := cn;
		    end;
		end;
	    end;
	    retType := nil;
	end;
	BIPREADS : begin
	    if not isFunc then begin
		error := ERRPROC;
	    end else if nump < 2 then begin
		error := ERRNUMP;
	    end else begin
		pt1 := CheckExpr(p1,EVALPOINT);
		pt2 := CheckExpr(p2,EVALPOINT);
		if IsBadExpr(p1) or IsBadExpr(p2) then begin
		    error := ERROTHER;
		end else if not Passable(arrayOfCharTypeNode,PARAMARRAYVALUE,pt1,p1)
		then begin
		    error := ERRTYPE1;
		end else if p2^.kind <> EXPRCONST then begin
		    error := ERRTYPE2;
		end else if not Passable(arrayOfCharTypeNode,PARAMARRAYVALUE,pt2,p2)
		then begin
		    error := ERRTYPE2;
		end else if p2^.exprConst^.kind = DTCHAR then begin
		    { one character string is OK }
		end else if p2^.exprConst^.kind <> DTSTRING then begin
		    error := ERRTYPE2;
		end else begin
		    RefOpenArray(p1,pt1);
		    if not CheckReadf(p2,p2^.exprConst^.strVal,p2^.next)
		    then begin
			error := ERROTHER;
		    end;
		    retType := integerTypeNode;
		end;
	    end;
	end;
	BIPWRITEC : begin
	    if isFunc then begin
		error := ERRFUNC;
	    end else if nump <> 2 then begin
		error := ERRNUMP;
	    end else begin
		pt1 := CheckExpr(p1,EVALGET);
		pt2 := BaseType(CheckExpr(p2,EVALGET));
		if IsBadExpr(p1) or IsBadExpr(p2) then begin
		    error := ERROTHER;
		end else if pt1 <> fileTypeNode then begin
		    error := ERRTYPE1;
		end else if pt2^.kind <> DTCHAR then begin
		    error := ERRTYPE2;
		end else begin
		    retType := nil;
		end;
	    end;
	end;
	BIPREADC : begin
	    if not isFunc then begin
		error := ERRPROC;
	    end else if nump <> 2 then begin
		error := ERRNUMP;
	    end else begin
		pt1 := CheckExpr(p1,EVALGET);
		pt2 := BaseType(CheckExpr(p2,EVALPUT));
		if IsBadExpr(p1) or IsBadExpr(p2) then begin
		    error := ERROTHER;
		end else if pt1 <> fileTypeNode then begin
		    error := ERRTYPE1;
		end else if pt2^.kind <> DTCHAR then begin
		    error := ERRTYPE2;
		end else if not IsAddressableExpr(p2) then begin
		    error := ERRMETHOD2;
		end else begin
		    retType := integerTypeNode;
		end;
	    end;
	end;
	BIPWRITEB : begin
	    if isFunc then begin
		error := ERRFUNC;
	    end else if nump <> 3 then begin
		error := ERRNUMP;
	    end else begin
		pt1 := CheckExpr(p1,EVALGET);
		pt2 := BaseType(CheckExpr(p2,EVALPOINT));
		p3 := p2^.next;
		pt3 := BaseType(CheckExpr(p3,EVALGET));
		if IsBadExpr(p1) or IsBadExpr(p2) or IsBadExpr(p3) then begin
		    error := ERROTHER;
		end else if pt1 <> fileTypeNode then begin
		    error := ERRTYPE1;
		end else if not (pt3^.kind in [DTINTEGER,DTCARDINAL]) then begin
		    error := ERRTYPE3;
		end else begin
		    PointerOrAddress(p2,pt2);
		    retType := nil;
		end;
	    end;
	end;
	BIPREADB : begin
	    if not isFunc then begin
		error := ERRPROC;
	    end else if nump <> 3 then begin
		error := ERRNUMP;
	    end else begin
		pt1 := CheckExpr(p1,EVALGET);
		pt2 := BaseType(CheckExpr(p2,EVALPUT));
		p3 := p2^.next;
		pt3 := BaseType(CheckExpr(p3,EVALGET));
		if IsBadExpr(p1) or IsBadExpr(p2) or IsBadExpr(p3) then begin
		    error := ERROTHER;
		end else if pt1 <> fileTypeNode then begin
		    error := ERRTYPE1;
		end else if not (pt3^.kind in [DTINTEGER,DTCARDINAL]) then begin
		    error := ERRTYPE3;
		end else begin
		    PointerOrAddress(p2,pt2);
		    retType := integerTypeNode;
		end;
	    end;
	end;
	BIPOPENF : begin
	    if not isFunc then begin
		error := ERRPROC;
	    end else if nump <> 2 then begin
		error := ERRNUMP;
	    end else begin
		pt1 := CheckExpr(p1,EVALPOINT);
		pt2 := CheckExpr(p2,EVALPOINT);
		if IsBadExpr(p1) or IsBadExpr(p2) then begin
		    error := ERROTHER;
		end else if not Passable(arrayOfCharTypeNode,PARAMARRAYVALUE,pt1,p1)
		then begin
		    error := ERRTYPE1;
		end else if not Passable(arrayOfCharTypeNode,PARAMARRAYVALUE,pt2,p2)
		then begin
		    error := ERRTYPE2;
		end else begin
		    RefOpenArray(p1,pt1);
		    RefOpenArray(p2,pt2);
		end;
		retType := fileTypeNode;
	    end;
	end;
	BIPCLOSEF : begin
	    if isFunc then begin
		error := ERRFUNC;
	    end else if nump <> 1 then begin
		error := ERRNUMP;
	    end else begin
		pt1 := CheckExpr(p1,EVALGET);
		if IsBadExpr(p1) then begin
		    error := ERROTHER;
		end else if not Passable(fileTypeNode,PARAMVALUE,pt1,p1)
		then begin
		    error := ERRTYPE1;
		end;
	    end;
	    retType := nil;
	end;
	BIPNEWPROCESS: begin
	    if isFunc then begin
		error := ERRFUNC;
	    end else if nump <> 4 then begin
		error := ERRNUMP;
	    end else begin
		p3 := p2^.next;
		p4 := p3^.next;
		pt1 := BaseType(CheckExpr(p1,EVALGET));
		pt2 := BaseType(CheckExpr(p2,EVALGET));
		pt3 := BaseType(CheckExpr(p3,EVALGET));
		pt4 := CheckExpr(p4,EVALPUT);
		if IsBadExpr(p1) or IsBadExpr(p2) or IsBadExpr(p3) or
			IsBadExpr(p4)
		then begin
		    error := ERROTHER;
		end else if pt1^.kind <> DTPROC then begin
		    error := ERRTYPE1;
		end else if not Passable(addressTypeNode,PARAMVALUE,pt2,p2)
		then begin
		    error := ERRTYPE2;
		end else if not Passable(cardinalTypeNode,PARAMVALUE,pt3,p3)
		then begin
		    error := ERRTYPE3;
		end else if not Passable(processTypeNode,PARAMVAR,pt4,p4)
		then begin
		    error := ERRTYPE4;
		end else if not IsAddressableExpr(p4) then begin
		    error := ERRMETHOD4;
		end else begin
		    retType := processTypeNode;
		end;
	    end;
	end;
	BIPTRANSFER: begin
	    if isFunc then begin
		error := ERRFUNC;
	    end else if nump <> 2 then begin
		error := ERRNUMP;
	    end else begin
		pt1 := CheckExpr(p1,EVALPUT);
		pt2 := CheckExpr(p2,EVALPUT);
		if IsBadExpr(p1) or IsBadExpr(p2) then begin
		    error := ERROTHER;
		end else if not Passable(processTypeNode,PARAMVAR,pt1,p1)
		then begin
		    error := ERRTYPE1;
		end else if not Passable(processTypeNode,PARAMVAR,pt2,p2)
		then begin
		    error := ERRTYPE2;
		end else if not IsAddressableExpr(p1) then begin
		    error := ERRMETHOD1;
		end else if not IsAddressableExpr(p2) then begin
		    error := ERRMETHOD2;
		end else begin
		    retType := nil;
		end;
	    end;
	end;
	BIPBITAND, BIPBITOR, BIPBITXOR, BIPBITSHIFTLEFT,
	BIPBITSHIFTRIGHT : begin
	    if not isFunc then begin
		error := ERRPROC;
	    end else if nump <> 2 then begin
		error := ERRNUMP;
	    end else begin
		pt1 := BaseType(CheckExpr(p1,EVALGET));
		pt2 := BaseType(CheckExpr(p2,EVALGET));
		if IsBadExpr(p1) or IsBadExpr(p2) then begin
		    error := ERROTHER;
		end else if not (pt1^.kind in [DTCARDINAL, DTINTEGER]) then begin
		    error := ERRTYPE1;
		end else if not (pt2^.kind in [DTCARDINAL, DTINTEGER]) then begin
		    error := ERRTYPE2;
		end else begin
		    retType := cardIntTypeNode;
		end;
	    end;
	end;
	BIPBITNOT : begin
	    if not isFunc then begin
		error := ERRPROC;
	    end else if nump <> 1 then begin
		error := ERRNUMP;
	    end else begin
		pt1 := BaseType(CheckExpr(p1,EVALGET));
		if IsBadExpr(p1) then begin
		    error := ERROTHER;
		end else if not (pt1^.kind in [DTCARDINAL, DTINTEGER]) then begin
		    error := ERRTYPE1;
		end else begin
		    retType := cardIntTypeNode;
		end;
	    end;
	end;
	BIPBITEXTRACT : begin
	    if not isFunc then begin
		error := ERRPROC;
	    end else if nump <> 3 then begin
		error := ERRNUMP;
	    end else begin
		p3 := p2^.next;
		pt1 := BaseType(CheckExpr(p1,EVALGET));
		pt2 := BaseType(CheckExpr(p2,EVALGET));
		pt3 := BaseType(CheckExpr(p3,EVALGET));
		if IsBadExpr(p1) or IsBadExpr(p2) or IsBadExpr(p3) then begin
		    error := ERROTHER;
		end else if not (pt1^.kind in [DTCARDINAL, DTINTEGER]) then begin
		    error := ERRTYPE1;
		end else if not (pt2^.kind in [DTCARDINAL, DTINTEGER]) then begin
		    error := ERRTYPE2;
		end else if not (pt3^.kind in [DTCARDINAL, DTINTEGER]) then begin
		    error := ERRTYPE3;
		end else begin
		    retType := cardIntTypeNode;
		end;
	    end;
	end;
	BIPBITINSERT : begin
	    if not isFunc then begin
		error := ERRPROC;
	    end else if nump <> 4 then begin
		error := ERRNUMP;
	    end else begin
		p3 := p2^.next;
		p4 := p3^.next;
		pt1 := BaseType(CheckExpr(p1,EVALGET));
		pt2 := BaseType(CheckExpr(p2,EVALGET));
		pt3 := BaseType(CheckExpr(p3,EVALGET));
		pt4 := BaseType(CheckExpr(p4,EVALGET));
		if IsBadExpr(p1) or IsBadExpr(p2) or IsBadExpr(p3)
			or IsBadExpr(p4)
		then begin
		    error := ERROTHER;
		end else if not (pt1^.kind in [DTCARDINAL, DTINTEGER]) then begin
		    error := ERRTYPE1;
		end else if not (pt2^.kind in [DTCARDINAL, DTINTEGER]) then begin
		    error := ERRTYPE2;
		end else if not (pt3^.kind in [DTCARDINAL, DTINTEGER]) then begin
		    error := ERRTYPE3;
		end else if not (pt4^.kind in [DTCARDINAL, DTINTEGER]) then begin
		    error := ERRTYPE4;
		end else begin
		    retType := cardIntTypeNode;
		end;
	    end;
	end;
    end;
    case error of
	ERRNONE : ;
	ERROTHER : ;
	ERRPROC : ErrorName(proc^.name,'Function used as a procedure');
	ERRFUNC : ErrorName(proc^.name,'Procedure used as a function');
	ERRNUMP : ErrorName(proc^.name,'Incorrect number of parameters');
	ERRTYPE1 : ErrorName(proc^.name,'Wrong type for parameter #1');
	ERRTYPE2 : ErrorName(proc^.name,'Wrong type for parameter #2');
	ERRTYPE3 : ErrorName(proc^.name,'Wrong type for parameter #3');
	ERRTYPE4 : ErrorName(proc^.name,'Wrong type for parameter #4');
	ERRMETHOD1 : ErrorName(proc^.name,
		    'Variable required for VAR parameter #1');
	ERRMETHOD2 : ErrorName(proc^.name,
		    'Variable required for VAR parameter #2');
	ERRMETHOD4 : ErrorName(proc^.name,
		    'Variable required for VAR parameter #4');
    end;
    CheckBuiltin := error = ERRNONE;
end;


function OptBuiltin{(procExpr : ExprNode; proc : ProcNode; params : ExprList):OptTime};
var
    p, p1, p2, p3, p4 : ExprNode;
    nump : integer;
    time, timep : OptTime;

    procedure OptRestOfParams(mode : EvalMode);
    var
	p : ExprNode;
    begin
	if p2 <> nil then begin
	    p := p2^.next;
	    while p <> nil do begin
		timep := OptExpr(p,procExpr,mode);
		time := Latest(time,timep);
		if mode = EVALPUT then begin
		    MarkOptExpr(p);
		end;
		p := p^.next;
	    end;
	end;
    end;

begin
    nump := 0;
    p1 := nil;
    p2 := nil;
    if params = nil then begin
	{ do nothing }
    end else if params^.first = nil then begin
	{ do nothing }
    end else begin
	p1 := params^.first;
	if p1^.next = nil then begin
	    nump := 1;
	end else begin
	    p2 := p1^.next;
	    if p2^.next = nil then begin
		nump := 2;
	    end else begin
		p := params^.first;
		nump := 0;
		while p <> nil do begin
		    nump := nump + 1;
		    p := p^.next;
		end;
	    end;
	end;
    end;
    case proc^.builtin of
	BIPHALT, BIPCPUTIME, BIPSIZE, BIPTSIZE, BIPBYTESIZE, BIPTBYTESIZE,
	BIPFIRST, BIPLAST : begin { either constants or no parameters }
	    time := 0;
	end;
	BIPABS, BIPCAP, BIPCHR, BIPFLOAT, BIPLONGFLOAT, BIPHIGH, BIPNUMBER,
	BIPODD, BIPORD, BIPTRUNC, BIPBITNOT : begin { all one value parameter }
	    time := OptExpr(p1,procExpr,EVALGET);
	end;
	BIPASSERT, BIPMAX, BIPMIN, BIPBITAND, BIPBITOR, BIPBITXOR,
	BIPBITSHIFTLEFT, BIPBITSHIFTRIGHT : begin { all two value parameters }
	    time := OptExpr(p1,procExpr,EVALGET);
	    timep := OptExpr(p2,procExpr,EVALGET);
	    time := Latest(time,timep);
	end;
	BIPBITEXTRACT : begin { three value parameters }
	    time := OptExpr(p1,procExpr,EVALGET);
	    timep := OptExpr(p2,procExpr,EVALGET);
	    time := Latest(time,timep);
	    p3 := p2^.next;
	    timep := OptExpr(p3,procExpr,EVALGET);
	    time := Latest(time,timep);
	end;
	BIPBITINSERT : begin { three value parameters }
	    time := OptExpr(p1,procExpr,EVALGET);
	    timep := OptExpr(p2,procExpr,EVALGET);
	    time := Latest(time,timep);
	    p3 := p2^.next;
	    timep := OptExpr(p3,procExpr,EVALGET);
	    time := Latest(time,timep);
	    p4 := p3^.next;
	    timep := OptExpr(p4,procExpr,EVALGET);
	    time := Latest(time,timep);
	end;
	BIPDEC, BIPINC, BIPINCL, BIPEXCL : begin
	    time := OptExpr(p1,procExpr,EVALPOINT);
	    timep := OptExpr(p2,procExpr,EVALGET);
	    time := Latest(time,timep);
	    p3 := p2^.next;
	    timep := OptExpr(p3,procExpr,EVALGET);
	    time := Latest(time,timep);
	    MarkOptExpr(p1);
	end;
	BIPDISPOSE, BIPNEW : begin
	    time := OptExpr(p1,procExpr,EVALPOINT);
	    timep := OptExpr(p2,procExpr,EVALGET);
	    time := Latest(time,timep);
	    p3 := p2^.next;
	    timep := OptExpr(p3,procExpr,EVALGET);
	    time := Latest(time,timep);
	    p4 := p3^.next;
	    if p4 <> nil then begin
		timep := OptExpr(p4,procExpr,EVALGET);
		time := Latest(time,timep);
	    end;
	    MarkOptExpr(p1);
	end;
	BIPALLOCATE, BIPDEALLOCATE : begin
	    time := OptExpr(p1,procExpr,EVALPUT);
	    timep := OptExpr(p2,procExpr,EVALGET);
	    time := Latest(time,timep);
	    MarkOptExpr(p1);
	end;
	BIPVAL : begin
	    time := OptExpr(p2,procExpr,EVALGET);
	end;
	BIPADR : begin
	    time := OptExpr(p1,procExpr,EVALPOINT);
	end;
	BIPUNIXCALL : begin
	    time := OptExpr(p2,procExpr,EVALGET);
	    OptRestOfParams(EVALGET);
	    MarkOptAll;
	    time := -optTime;
	end;
	BIPOPENF : begin
	    time := OptExpr(p1,procExpr,EVALGET);
	    timep := OptExpr(p2,procExpr,EVALGET);
	    time := Latest(time,timep);
	end;
	BIPCLOSEF : begin
	    time := OptExpr(p1,procExpr,EVALGET);
	end;
	BIPWRITEF : begin
	    time := OptExpr(p1,procExpr,EVALGET);
	    timep := OptExpr(p2,procExpr,EVALGET);
	    time := Latest(time,timep);
	    OptRestOfParams(EVALGET);
	end;
	BIPREADF : begin
	    time := OptExpr(p1,procExpr,EVALGET);
	    timep := OptExpr(p2,procExpr,EVALGET);
	    time := Latest(time,timep);
	    OptRestOfParams(EVALPUT);
	end;
	BIPWRITES : begin
	    time := OptExpr(p1,procExpr,EVALPUT);
	    timep := OptExpr(p2,procExpr,EVALGET);
	    time := Latest(time,timep);
	    OptRestOfParams(EVALGET);
	    MarkOptExpr(p1);
	end;
	BIPREADS : begin
	    time := OptExpr(p1,procExpr,EVALGET);
	    timep := OptExpr(p2,procExpr,EVALGET);
	    time := Latest(time,timep);
	    OptRestOfParams(EVALPUT);
	end;
	BIPWRITEC : begin
	    time := OptExpr(p1,procExpr,EVALGET);
	    timep := OptExpr(p2,procExpr,EVALGET);
	    time := Latest(time,timep);
	end;
	BIPREADC : begin
	    time := OptExpr(p1,procExpr,EVALGET);
	    timep := OptExpr(p2,procExpr,EVALPUT);
	    time := Latest(time,timep);
	    MarkOptExpr(p2);
	end;
	BIPWRITEB : begin
	    p3 := p2^.next;
	    time := OptExpr(p2,procExpr,EVALGET);
	    timep := OptExpr(p3,procExpr,EVALGET);
	    time := Latest(time,timep);
	    timep := OptExpr(p1,procExpr,EVALGET);
	    time := Latest(time,timep);
	end;
	BIPREADB : begin
	    p3 := p2^.next;
	    timep := OptExpr(p2,procExpr,EVALPUT);
	    time := OptExpr(p3,procExpr,EVALGET);
	    time := Latest(time,timep);
	    timep := OptExpr(p1,procExpr,EVALGET);
	    time := Latest(time,timep);
	    MarkOptExpr(p2);
	end;
	BIPNEWPROCESS : begin
	    time := OptExpr(p1,procExpr,EVALGET);
	    timep := OptExpr(p2,procExpr,EVALGET);
	    time := Latest(time,timep);
	    p3 := p2^.next;
	    timep := OptExpr(p3,procExpr,EVALGET);
	    time := Latest(time,timep);
	    p4 := p3^.next;
	    timep := OptExpr(p4,procExpr,EVALPUT);
	    time := Latest(time,timep);
	    MarkOptExpr(p4);
	end;
	BIPTRANSFER : begin
	    time := OptExpr(p1,procExpr,EVALPUT);
	    timep := OptExpr(p2,procExpr,EVALPUT);
	    time := Latest(time,timep);
	    MarkOptAll;
	end;
    end;
    OptBuiltin := time;
end;

procedure CountBuiltin{(proc : ProcNode; params : ExprList)};
var
    p, p1, p2, p3, p4 : ExprNode;
    minElement : cardinal;
    procnum : cardinal;

    procedure CountRestOfParams(mode : EvalMode);
    begin
	if p2 <> nil then begin
	    p := p2^.next;
	    while p <> nil do begin
		if (mode = EVALGET) and (p^.exprType <> addressTypeNode)
		then begin
		    CountExpr(p,EVALGET);
		end else begin
		    CountExpr(p,EVALPOINT);
		end;
		p := p^.next;
	    end;
	end;
    end;

begin
    p1 := nil;
    p2 := nil;
    if params = nil then begin
	{ do nothing }
    end else if params^.first = nil then begin
	{ do nothing }
    end else begin
	p1 := params^.first;
	if p1^.next <> nil then begin
	    p2 := p1^.next;
	end;
    end;

    case proc^.builtin of
	BIPABS, BIPCAP, BIPCHR, BIPFLOAT, BIPLONGFLOAT,
	BIPODD, BIPORD, BIPTRUNC, BIPBITNOT : begin
	    CountExpr(p1,EVALGET);
	end;
	BIPASSERT, BIPMAX, BIPMIN, BIPBITAND, BIPBITOR, BIPBITXOR,
	BIPBITSHIFTLEFT, BIPBITSHIFTRIGHT : begin
	    CountExpr(p1,EVALGET);
	    CountExpr(p2,EVALGET);
	end;
	BIPBITEXTRACT : begin
	    CountExpr(p1,EVALGET);
	    CountExpr(p2,EVALGET);
	    p3 := p2^.next;
	    CountExpr(p3,EVALGET);
	end;
	BIPBITINSERT : begin
	    CountExpr(p1,EVALGET);
	    CountExpr(p2,EVALGET);
	    p3 := p2^.next;
	    CountExpr(p3,EVALGET);
	    p4 := p3^.next;
	    CountExpr(p4,EVALGET);
	end;
	BIPDEC, BIPINC : begin
	    CountExpr(p1,EVALPOINT);
	    CountExpr(p2,EVALGET);
	    p3 := p2^.next;
	    CountExpr(p3,EVALGET);
	end;
	BIPDISPOSE : begin
	    CountExpr(p1,EVALPOINT);
	    CountExpr(p2,EVALGET);
	    p3 := p2^.next;
	    CountExpr(p3,EVALGET);
	    p4 := p3^.next;
	    if p4 <> nil then begin
		CountExpr(p4,EVALGET);
	    end;
	end;
	BIPEXCL, BIPINCL : begin
	    CountExpr(p1,EVALPOINT);
	    CountExpr(p2,EVALGET);
	    p3 := p2^.next;
	    CountExpr(p3,EVALGET);
	end;
	BIPHIGH, BIPNUMBER : begin
	    if p1^.exprType^.indexType = nil then begin
		CountExpr(p1,EVALPOINT);
	    end;
	end;
	BIPNEW : begin
	    CountExpr(p1,EVALPOINT);
	    CountExpr(p2,EVALGET);
	    p3 := p2^.next;
	    CountExpr(p3,EVALGET);
	    p4 := p3^.next;
	    if p4 <> nil then begin
		CountExpr(p4,EVALGET);
	    end;
	end;
	BIPALLOCATE, BIPDEALLOCATE : begin
	    CountExpr(p1,EVALPUT);
	    CountExpr(p2,EVALGET);
	end;
	BIPVAL : begin
	    CountExpr(p2,EVALGET);
	end;
	BIPADR : begin
	    CountExpr(p1,EVALPOINT);
	end;
	BIPHALT, BIPSIZE, BIPTSIZE, BIPBYTESIZE, BIPTBYTESIZE, BIPCPUTIME,
	BIPFIRST, BIPLAST : begin
	end;
	BIPUNIXCALL : begin
	    p := p2;
	    while p <> nil do begin
		CountExpr(p,EVALGET);
		p := p^.next;
	    end;
	end;
	BIPWRITEF : begin
	    CountExpr(p1,EVALGET);
	    CountExpr(p2,EVALPOINT);
	    CountRestOfParams(EVALGET);
	end;
	BIPWRITES : begin
	    CountExpr(p1,EVALPOINT);
	    CountExpr(p2,EVALPOINT);
	    CountRestOfParams(EVALGET);
	end;
	BIPREADF : begin
	    CountExpr(p1,EVALGET);
	    CountExpr(p2,EVALPOINT);
	    CountRestOfParams(EVALPUT);
	end;
	BIPREADS : begin
	    CountExpr(p1,EVALPOINT);
	    CountExpr(p2,EVALPOINT);
	    CountRestOfParams(EVALPUT);
	end;
	BIPWRITEC : begin
	    CountExpr(p1,EVALGET);
	    CountExpr(p2,EVALGET);
	end;
	BIPREADC : begin
	    CountExpr(p1,EVALGET);
	    CountExpr(p2,EVALPUT);
	end;
	BIPWRITEB : begin
	    CountExpr(p1,EVALGET);
	    CountExpr(p2,EVALPOINT);
	    p3 := p2^.next;
	    CountExpr(p3,EVALGET);
	end;
	BIPREADB : begin
	    CountExpr(p1,EVALGET);
	    CountExpr(p2,EVALPOINT);
	    p3 := p2^.next;
	    CountExpr(p3,EVALGET);
	end;
	BIPOPENF : begin
	    CountExpr(p1,EVALPOINT);
	    CountExpr(p2,EVALPOINT);
	end;
	BIPCLOSEF : begin
	    CountExpr(p1,EVALGET);
	end;
    end;
end;
