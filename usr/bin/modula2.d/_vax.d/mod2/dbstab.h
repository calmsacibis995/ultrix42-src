(*#@(#)dbstab.h	4.1	Ultrix	7/17/90 *)
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
$Header: dbstab.h,v 1.4 84/05/19 11:37:53 powell Exp $
 ****************************************************************************)
procedure InitStab; external;
procedure StabGlobalPort; external;
procedure StabTypeDef(tn : TypeNode); external;
procedure StabTypeNumber(tn : TypeNode); external;
procedure StabProc(proc : ProcNode); external;
procedure StabModule(module : ModuleNode); external;
procedure StabScope(scope : Scope); external;
procedure StabLine(fileName:String; lineNumber : integer); external;
procedure StabNamedType(name : String; tn : TypeNode); external;
