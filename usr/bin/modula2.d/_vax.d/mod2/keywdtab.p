(*#@(#)keywdtab.p	4.1	Ultrix	7/17/90 *)
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
$Header: keywdtab.p,v 1.6 84/06/06 12:56:56 powell Exp $
 ****************************************************************************)
#include "globals.h"
#include "keywdtab.h"

procedure InstallKey(keyword : ShortString; token : Token);
var
    i : integer;
    str : String;
    key : KeywordRecPtr;
begin
    i := 1;
    while (i < SHORTSTRINGSIZE) and (keyword[i] <> ' ') do begin
	AddChar(keyword[i]);
	i := i + 1;
    end;
    str := NewString;
    if token in [TKAND..TKINCLUDE] then begin
	tokenKeywordTab[token] := str;
    end;
    new(key);
    key^.str := str;
    key^.token := token;
    key^.next := keywordTab[str^.hash mod (KEYHASHSIZE+1)];
    keywordTab[str^.hash mod (KEYHASHSIZE+1)] := key;
end;

procedure InitKeywords;
var
    i : integer;
begin {InitKeys}
	for i:=0 to KEYHASHSIZE do begin	{initialize key hash array}
	    keywordTab[i] := nil;
	end;
	InstallKey('AND             ', TKAND);	{put keywords in strTable}
	InstallKey('ARRAY           ', TKARRAY);
	InstallKey('BEGIN           ', TKBEGIN);
	InstallKey('BY              ', TKBY);
	InstallKey('CASE            ', TKCASE);
	InstallKey('CONST           ', TKCONST);
	InstallKey('DEFINITION      ', TKDEFINITION);
	InstallKey('DIV             ', TKDIV);
	InstallKey('DO              ', TKDO);
	InstallKey('ELSE            ', TKELSE);
	InstallKey('ELSIF           ', TKELSIF);
	InstallKey('END             ', TKEND);
	InstallKey('EXIT            ', TKEXIT);
	InstallKey('EXPORT          ', TKEXPORT);
	InstallKey('FOR             ', TKFOR);
	InstallKey('FROM            ', TKFROM);
	InstallKey('IF              ', TKIF);
	InstallKey('IMPLEMENTATION  ', TKIMPLEMENTATION);
	InstallKey('IMPORT          ', TKIMPORT);
	InstallKey('IN              ', TKIN);
	InstallKey('LOOP            ', TKLOOP);
	InstallKey('MOD             ', TKMOD);
	InstallKey('MODULE          ', TKMODULE);
	InstallKey('NOT             ', TKNOT);
	InstallKey('OF              ', TKOF);
	InstallKey('OR              ', TKOR);
	InstallKey('POINTER         ', TKPOINTER);
	InstallKey('PROCEDURE       ', TKPROCEDURE);
	InstallKey('QUALIFIED       ', TKQUALIFIED);
	InstallKey('RECORD          ', TKRECORD);
	InstallKey('REPEAT          ', TKREPEAT);
	InstallKey('RETURN          ', TKRETURN);
	InstallKey('SET             ', TKSET);
	InstallKey('THEN            ', TKTHEN);
	InstallKey('TO              ', TKTO);
	InstallKey('TYPE            ', TKTYPE);
	InstallKey('UNTIL           ', TKUNTIL);
	InstallKey('VAR             ', TKVAR);
	InstallKey('WHILE           ', TKWHILE);
	InstallKey('WITH            ', TKWITH);
	InstallKey('@SIZE           ', TKATSIZE);
	InstallKey('@ALIGN          ', TKATALIGN);
	InstallKey('@PASCAL         ', TKATPASCAL);
	InstallKey('@C              ', TKATC);
	InstallKey('@NOCHECK        ', TKATNONE);
	InstallKey('@NILCHECK       ', TKATNIL);
	InstallKey('@INCLUDE        ', TKINCLUDE);
	InstallKey('@NOCOUNT        ', TKNOCOUNT);
	InstallKey('@EXTERNAL       ', TKEXTERNAL);
	InstallKey('@UNQUALIFIED    ', TKUNQUALIFIED);
	InstallKey('@DYNARRAY       ', TKDYNARRAY);
	InstallKey('@SUBARRAY       ', TKSUBARRAY);
	if 'k' in debugSet then begin
	    DumpKeywords;
	end;
end; {InitKeys}

function KeyLookUp{(ident : String): Token};
var
	hash : integer;
	found : boolean;
	key : KeywordRecPtr;
	keyStr : String;
	i : integer;
	ci, ck : char;

begin {KeyLookUp}
    { keyword lookup ignores case.  Hash values are case-independent }
    found := false;
    hash := ident^.hash mod (KEYHASHSIZE+1);;
    key := keywordTab[hash];
    while (key <> nil) and not found do begin
	keyStr := key^.str;
	if (ident^.hash = keyStr^.hash) and (ident^.length = keyStr^.length)
	then begin
	    found := true;
	    i := 0;
	    while (i < ident^.length) and found do begin
		ci := GetChar(ident,i);
		ck := GetChar(keyStr,i);
		if standardKeywordFlag then begin
		    { keywords must be upper case only }
		    found := ci = ck;
		end else begin
		    if ci in ['a'..'z'] then begin
			ci := chr(ord(ci)-ord('a')+ord('A'));
		    end;
		    found := ci = ck;
		end;
		i := i + 1;
	    end;
	end;
	if not found then begin
	    key := key^.next;
	end;
    end;
    if key <> nil then begin
	KeyLookUp := key^.token;
    end else begin
	KeyLookUp := TKIDENT;
    end;
end; {KeyLookUp}

procedure DumpKeywords;
var
    i : integer;
    key : KeywordRecPtr;
begin
    for i := 0 to KEYHASHSIZE do begin
	key := keywordTab[i];
	while key <> nil do begin
	    write(output,i:0,' ',key^.token:0,' ');
	    WriteString(output,key^.str);
	    writeln(output);
	    key := key^.next;
	end;
    end;
end;
