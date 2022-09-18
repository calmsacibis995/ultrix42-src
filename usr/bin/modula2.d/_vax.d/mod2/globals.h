(*#@(#)globals.h	4.1	Ultrix	7/17/90 *)
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
$Header: globals.h,v 1.5 84/05/19 11:41:01 powell Exp $
 ****************************************************************************)
{ Definitions that are included everywhere }

{ debugging flags }
type
    DebugFlags = 'a'..'z';
    TargetMachine = (TARGETVAX,TARGETTITAN);
var
    debugSet : set of DebugFlags;
    genDebugInfoFlag : boolean;
    genCheckFlag : boolean;
    standardKeywordFlag, standardCardinalFlag, standardStringFlag : boolean;
    TraceCexpr, TraceDecls, TraceGenpc, TraceNexpr, TraceNstmt, TraceSymtab,
	TraceOpt, TraceOptim : boolean;
    target : TargetMachine;

{ printing indentation per level }
const
    INDENT = 4;

{ file names }
const
    FILENAMESIZE = 100;
    CODEFILENAME = 'mod.pcd';
type
    FileName = array [0..FILENAMESIZE] of char;

procedure exit(status : integer); external;

#include "machine.h"
#include "tokens.h"
#include "stringtab.h"
#include "symtab.h"
#include "error.h"
var
    codeFileName : FileName;
    mainFileName : String;
