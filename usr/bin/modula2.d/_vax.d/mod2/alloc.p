(*#@(#)alloc.p	4.1	Ultrix	7/17/90 *)
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
$Header: alloc.p,v 1.5 84/05/19 11:34:55 powell Exp $
 ****************************************************************************)
#include "globals.h"
#include "alloc.h"

procedure InitAlloc;
begin
    globalVarList := AddToGlobalVarList(nil,nil);
    globalVarNumber := 1;
end;

function RoundUp{(value, multiple : cardinal) : cardinal};
begin
    RoundUp := CardDiv(value+multiple-1,multiple) * multiple;
end;

procedure AllocateMemory{(ar : AllocationNode; mt : MemoryType;
	size, alignment : cardinal; proc : ProcNode; var address : Address)};
var
    offset : cardinal;
begin
    assert(mt <> MEMGLOBAL);
    if (mt = MEMFAST) and (ar^.current[MEMFAST] >= MAXTSTORAGE*WORDSIZE)
    then begin
	mt := MEMNORMAL;
    end;
    offset := ar^.current[mt];
    if mt in [MEMFAST,MEMPARAM] then begin
	size := RoundUp(size,WORDSIZE);
	offset := RoundUp(offset,WORDSIZE);
    end else begin
	offset := RoundUp(offset,alignment);
    end;
    address.kind := mt;
    address.proc := proc;
    if proc <> nil then begin
	address.level := proc^.displayLevel;
    end else begin
	address.level := 0;
    end;
    address.offset := offset;
    if TraceDecls then begin
	writeln(output,'AllocateMemory: size=',size:1:0,', align=',
		alignment:1:0,', offset=',offset:1:0,', ',mt);
    end;
    ar^.current[mt] := offset + size;
    if ar^.current[mt] > ar^.maximum[mt] then begin
	ar^.maximum[mt] := ar^.current[mt];
    end;
end;

procedure AllocateGlobal{(name : String; size : MemoryOffset;
	var address : Address)};
var
    gvn : GlobalVarNode;
begin
    size := RoundUp(size,WORDSIZE);

    new(gvn);
    gvn^.name := name;
    gvn^.number := globalVarNumber;
    globalVarNumber := globalVarNumber + 1;
    gvn^.size := size;
    gvn^.extern := false;
    address.kind := MEMGLOBAL;
    address.gvn := gvn;
    globalVarList := AddToGlobalVarList(globalVarList,gvn);
end;

function InitAllocationNode {: AllocationNode};
var
    an : AllocationNode;
begin
    new(an);
    an^.current[MEMNORMAL] := 0;
    an^.maximum[MEMNORMAL] := 0;
    an^.current[MEMFAST] := 0;
    an^.maximum[MEMFAST] := 0;
    an^.current[MEMPARAM] := 0;
    an^.maximum[MEMPARAM] := 0;
    InitAllocationNode := an;
end;

function SaveAllocationNode {(an : AllocationNode) : AllocationNode};
var
    san : AllocationNode;
begin
    new(san);
    san^.current[MEMNORMAL] := an^.current[MEMNORMAL];
    san^.maximum[MEMNORMAL] := an^.maximum[MEMNORMAL];
    san^.current[MEMFAST] := an^.current[MEMFAST];
    san^.maximum[MEMFAST] := an^.maximum[MEMFAST];
    san^.current[MEMPARAM] := an^.current[MEMPARAM];
    san^.maximum[MEMPARAM] := an^.maximum[MEMPARAM];
    SaveAllocationNode := san;
end;

procedure RestoreAllocationNode {(an, san : AllocationNode)};
begin
    an^.current[MEMNORMAL] := san^.current[MEMNORMAL];
    an^.current[MEMFAST] := san^.current[MEMFAST];
    an^.current[MEMPARAM] := san^.current[MEMPARAM];
    assert (san^.maximum[MEMNORMAL] <= an^.maximum[MEMNORMAL]);
    assert (san^.maximum[MEMFAST] <= an^.maximum[MEMFAST]);
    assert (san^.maximum[MEMPARAM] <= an^.maximum[MEMPARAM]);
    dispose(san);
end;

procedure UpdateAllocationNode {(an : AllocationNode)};
begin
    an^.current[MEMNORMAL] := an^.maximum[MEMNORMAL];
    an^.current[MEMFAST] := an^.maximum[MEMFAST];
end;

function AddToGlobalVarList {(list : GlobalVarList; newOne : GlobalVarNode)
	: GlobalVarList};
begin
    if list = nil then begin
	new(list);
	list^.first := nil;
	list^.last := nil;
    end;
    if newOne = nil then begin
	{ empty statement, do nothing }
    end else if list^.last = nil then begin
	newOne^.next := nil;
	list^.first := newOne;
	list^.last := newOne;
    end else begin
	newOne^.next := nil;
	list^.last^.next := newOne;
	list^.last:= newOne;
    end;
    AddToGlobalVarList := list;
end;

