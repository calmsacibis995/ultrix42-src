(*#@(#)alloc.h	4.1	Ultrix	7/17/90 *)
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
$Header: alloc.h,v 1.4 84/05/19 11:34:50 powell Exp $
 ****************************************************************************)
var
    globalVarList : GlobalVarList;
    globalVarNumber : integer;

function RoundUp(value, multiple : cardinal) : cardinal; external;

procedure AllocateMemory(ar : AllocationNode; mt : MemoryType;
	size, alignment : MemoryOffset; proc : ProcNode;
	var address : Address); external;

procedure AllocateGlobal(name : String; size : MemoryOffset;
	var address : Address); external;

function InitAllocationNode : AllocationNode; external;

function SaveAllocationNode (an : AllocationNode) : AllocationNode; external;

procedure RestoreAllocationNode (an, san : AllocationNode); external;

procedure UpdateAllocationNode (an : AllocationNode); external;

procedure InitAlloc; external;

function AddToGlobalVarList (list : GlobalVarList; newOne : GlobalVarNode)
	: GlobalVarList; external;
