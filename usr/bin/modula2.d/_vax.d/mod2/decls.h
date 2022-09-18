(*#@(#)decls.h	4.1	Ultrix	7/17/90 *)
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
$Header: decls.h,v 1.6 84/06/06 12:55:41 powell Exp $
 ****************************************************************************)
var
    badExprNode : ExprNode;

procedure InitGlobalModule; external;

procedure DefineConst(name : String; value : ConstNode); external;

procedure DefineType(name : String; value : TypeNode); external;

function DefineVar(name : String; varType : TypeNode; mt : MemoryType;
	global : boolean): VarNode; external;

procedure DefineVarList(idList : IdentList; varType : TypeNode;
	global : Token); external;

function DefineModule(name : String; kind : Token) : ModuleNode; external;

procedure EndModule(mn : ModuleNode; body : StmtList; name : String); external;

function DefineProc(name : String; global : Token): ProcNode; external;

function AddTypeToProc(proc : ProcNode; procType : TypeNode): ProcNode;
	external;

procedure EndProc(proc : ProcNode; body : StmtList; name : String); external;

function ProcType(paramList : ParamList; funcType : TypeNode) : TypeNode;
	external;

function MakeParamList(kindToken : Token; idents : IdentList;
	paramType : TypeNode) : ParamList; external;

function AppendParamList(some, more : ParamList) : ParamList; external;

function AppendIdentList(some, more : IdentList) : IdentList; external;

function PointerType (toType : TypeNode; option : Token) : TypeNode; external;

function SetType (setRange : TypeNode) : TypeNode; external;

function ArrayType (indexType : TypeNode; elementType : TypeNode;
	kind, option : Token) : TypeNode; external;

function AddToFieldList (list : FieldList; newOne : FieldNode) : FieldList;
	external;

function AddToVariantList (list : VariantList; newOne : VariantNode)
	: VariantList; external;

function MakeFieldList(idents : IdentList; fieldType : TypeNode) : FieldList;
	external;

function AppendFieldList(some, more : FieldList) : FieldList; external;

function MakeVariant (tag : ConstSetList; fieldList : FieldList) : VariantNode;
	external;

function MakeVariantField (ident : String; fieldType : TypeNode;
	variantList : VariantList; elseVariant : VariantNode)
	: FieldList; external;

procedure DefineFields(fieldList : FieldList; scope : Scope;
	an : AllocationNode; recType : TypeNode;
	containingVariant : VariantNode; var alignment : cardinal); external;

function RecordType (fieldList : FieldList) : TypeNode; external;

function MakeSubrange (low, up : cardinal; baseType : TypeNode)
	: TypeNode; external;

function SubrangeType (lower, upper : ConstNode) : TypeNode; external;

function AddToVarList (list : VarList; newOne : VarNode) : VarList;
	external;

function AddToEnumList (list : EnumList; newOne : EnumNode) : EnumList;
	external;

function EnumerationType(idList : IdentList) : TypeNode; external;

function AddToIdentList (list : IdentList; newOne : IdentNode) : IdentList;
	external;

function MakeIdent (name : String) : IdentNode; external;

procedure PrintType(tn:TypeNode;indent:integer); external;

function AddToImportList (list : ImportList; newOne : ImportNode) : ImportList;
	external;

function Import(fromIdent : String; idents : IdentList) : ImportNode; external;

function ReadImport(imp : ImportNode) : ImportNode; external;

procedure ProcessImport(imp : ImportNode; complain : boolean);
	external;

procedure Export(idents : IdentList; qualToken : Token); external;

function MakeConstSet (lower, upper : ConstNode) : ConstSetNode; external;

function AddToConstSetList (list : ConstSetList; newOne : ConstSetNode)
	: ConstSetList; external;

function AddToProcList (list : ProcList; newOne : ProcNode) : ProcList;
	external;

function AddToModuleList (list : ModuleList; newOne : ModuleNode) : ModuleList;
	external;

function BaseType (tn : TypeNode) : TypeNode; external;

function ActualType (tn : TypeNode) : TypeNode; external;

function NumberOf (tn : TypeNode) : cardinal; external;

function LowerBoundOf (tn : TypeNode) : cardinal; external;

function UpperBoundOf (tn : TypeNode) : cardinal; external;

function SizeOf (tn : TypeNode) : cardinal; external;

function AlignmentOf (tn : TypeNode) : cardinal; external;

function WordSizeOf (tn : TypeNode) : cardinal; external;

procedure CheckEqualType(sym : Symbol; tn : TypeNode); external;

procedure CheckEqualVar(sym : Symbol; tn : TypeNode); external;

procedure CheckEqualProc(proc : ProcNode; procType : TypeNode); external;

procedure CheckProc(pn : ProcNode); external;

procedure CheckModule(mn : ModuleNode); external;

function NewTypeNode (kind : DataType) : TypeNode; external;

function SameTypeParam(dst, src : TypeNode) : boolean; external;

function AddToPortList(list : PortList; newOne : PortNode): PortList; external;

const
    MODULEINITNAME = '_init';
    MODULEINITFLAG = '_initflag';
var
    compileModuleName : String;
