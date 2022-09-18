(*#@(#)symtab.h	4.1	Ultrix	7/17/90 *)
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
$Header: symtab.h,v 1.6 84/06/06 13:02:40 powell Exp $
 ****************************************************************************)
const
    MAXBLOCKNUMBER = 1000;
    MAXDISPLAYLEVEL = 15;
    MAXSETSIZE = 1024;
    NULLLABEL = 9999;
    MAXLABEL = 50000;

    MAXTSTORAGE = 20;	{ 0..MAXTSTORAGE only for T's }
    NUMTEMPS = 20;	{ 20 CSE temps (number MAXTSTORAGE+1..MAXTEMPSTORAGE }
    MAXTEMPSTORAGE = 40;	{ storage for T's and CSE's }

    MAXBUILTINSCOPES = 100;

type

    BlockNumber = -2..MAXBLOCKNUMBER;
    DisplayLevel = 0..MAXDISPLAYLEVEL;
    MemoryOffset = cardinal;
    MemoryType = (MEMNONE, MEMABSOLUTE, MEMGLOBAL, MEMNORMAL, MEMFAST,
		    MEMPARAM);
    LabelNumber = NULLLABEL..MAXLABEL;
    TypeNumber = integer;
    OptIndex = integer;  { temporary hack to avoid needless recompilations }
    OptTime = integer;
    SymbolCase = (ONECASE,ANYCASE,SCOPECASE);
    EvalMode = (EVALGET, EVALPUT, EVALPOINT);
    CheckKind = (CHECKPTRPASCAL, CHECKPTRMODULA, CHECKPTRNIL, CHECKPTRNONE,
	    CHECKPTRC, CHECKRANGE, CHECKVARIANT, CHECKSUBSCR, CHECKSUBSCROPEN,
	    CHECKCASE);

{ pointers to things.  Defining them here reduces ordering constraints }
type
    Scope = ^ScopeRec;
    Symbol = ^SymbolRec;
    SymbolList = ^SymbolListRec;
    ConstNode = ^ConstNodeRec;
    TypeNode = ^TypeNodeRec;
    VarNode = ^VarNodeRec;
    VarList = ^VarListRec;
    FieldNode = ^FieldNodeRec;
    FieldList = ^FieldListRec;
    VariantNode = ^VariantNodeRec;
    VariantList = ^VariantListRec;
    ModuleNode = ^ModuleNodeRec;
    ModuleList = ^ModuleListRec;
    ProcNode = ^ProcNodeRec;
    ProcList = ^ProcListRec;
    ExprNode = ^ExprNodeRec;
    ExprList = ^ExprListRec;
    StmtNode = ^StmtNodeRec;
    StmtList = ^StmtListRec;
    CaseNode = ^CaseNodeRec;
    CaseList = ^CaseListRec;
    CaseTreeNode = ^CaseTreeNodeRec;
    EnumNode = ^EnumNodeRec;
    EnumList = ^EnumListRec;
    ConstSetNode = ^ConstSetNodeRec;
    ConstSetList = ^ConstSetListRec;
    ExprSetNode = ^ExprSetNodeRec;
    ExprSetList = ^ExprSetListRec;
    ParamNode = ^ParamNodeRec;
    ParamList = ^ParamListRec;
    IdentNode = ^IdentNodeRec;
    IdentList = ^IdentListRec;
    ImportNode = ^ImportNodeRec;
    ImportList = ^ImportListRec;
    SetValue = ^SetValueRec;
    CodeNode = ^CodeNodeRec;
    CodeList = ^CodeListRec;
    GlobalVarNode = ^GlobalVarNodeRec;
    GlobalVarList = ^GlobalVarListRec;
    AllocationNode = ^AllocationNodeRec;
    WithQualNode = ^WithQualNodeRec;
    TempMapNode = ^TempMapNodeRec;
    PortNode = ^PortNodeRec;
    PortList = ^PortListRec;


    AllocationNodeRec = record
	current, maximum : array [MEMNORMAL..MEMPARAM] of MemoryOffset;
    end;

    Address = record
	case kind : MemoryType of
	    MEMGLOBAL : (gvn : GlobalVarNode);
	    MEMNORMAL, MEMFAST, MEMPARAM : (
		level : DisplayLevel;
		proc : ProcNode;
		offset : MemoryOffset;
	    );
	    MEMABSOLUTE : (absolute : cardinal);
    end;

    TempMapNodeRec = record
	map : array [0..MAXTEMPSTORAGE] of MemoryOffset;
	numReg : integer;
    end;

    OptMarkTime = record
	time : OptTime;
	proc : ProcNode;
    end;
    
    ScopeRec = record
	block : BlockNumber;
	open : boolean;
	enclosing : Scope;
	symbols : SymbolList;
    end;

    SymbolKind = (SYMNULL, SYMVAR, SYMCONST, SYMPROC, SYMTYPE,
		SYMMODULE, SYMFIELD, SYMENUM);
    SymbolRec = record
	name : String;
	block : BlockNumber;
	symCase : SymbolCase;
	next : Symbol;
	nextInTable : Symbol;
	case kind : SymbolKind of
	    SYMCONST : (symConst : ConstNode);
	    SYMTYPE : (symType : TypeNode);
	    SYMVAR : (symVar : VarNode);
	    SYMMODULE : (symModule : ModuleNode);
	    SYMPROC : (symProc : ProcNode);
	    SYMFIELD : (symField : FieldNode);
	    SYMENUM : (symEnum : EnumNode);
    end;
    SymbolNode = Symbol;
    SymbolListRec = record
	next : SymbolList;
	first, last : SymbolNode;
    end;

    PortNodeRec = record
	next : PortNode;
	module : ModuleNode;
	sym : Symbol;
	qualified : boolean;
	export : boolean;
	extern : boolean;
    end;
    PortListRec = record
	next : PortList;
	first, last : PortNode;
    end;

    DataType = (DTNULL, DTPOINTER, DTRECORD, DTARRAY, DTINTEGER, DTBOOLEAN,
		DTCHAR, DTRENAME, DTSTRING, DTREAL, DTSET, DTCARDINAL, DTBYTE,
		DTSUBRANGE, DTENUMERATION, DTPROC, DTWORD, DTANY, DTLONGREAL,
		DTOPAQUE, DTPROCESS);
    ConstNodeRec = record
	next : ConstNode;
	case kind : DataType of
	    DTSTRING : (strVal : String);
	    DTCHAR : (charVal : integer);
	    DTINTEGER,
	    DTCARDINAL : (cardVal : cardinal);
	    DTBOOLEAN : (boolVal : boolean);
	    DTREAL, DTLONGREAL : (realVal : real);
	    DTSET : (setVal : SetValue);
	    DTENUMERATION : (enumVal : EnumNode);
	    DTPROC : (procVal : ProcNode);
    end;

    ArrayKind = (ARRAYNORMAL, ARRAYOPEN, ARRAYSUBARRAY, ARRAYDYNAMIC);
    TypeNodeRec = record
	next : TypeNode;
	size, alignment : cardinal;
	number : TypeNumber;
	markTime : OptMarkTime;
	name : String;		{ local name for type }
	module : ModuleNode;	{ definition module in which type was defined }
	opaqueName : String;	{ name if now or ever opaque type }
	case kind : DataType of
	    DTPOINTER : (toType : TypeNode; ptrCheck : CheckKind);
	    DTRECORD : (recScope : Scope; fieldList : FieldList);
	    DTSET : (setRange : TypeNode);
	    DTARRAY : (indexType : TypeNode; elementType : TypeNode;
			    nocount : boolean; arrayKind : ArrayKind);
	    DTRENAME : (renameType : TypeNode);
	    DTSUBRANGE : (subMinOrd, subMaxOrd : cardinal; baseType : TypeNode);
	    DTENUMERATION : (enumCount : integer; enumList : EnumList);
	    DTSTRING : (stringLength : integer);
	    DTPROC : (numParams : integer; paramList : ParamList;
			    funcType : TypeNode);
    end;

    FieldKind = (FIELDNORMAL, FIELDVARIANT);
    FieldNodeRec = record
	next : FieldNode;
	name : String;
	recType : TypeNode;
	fieldType : TypeNode;
	offset : MemoryOffset;
	containingVariant : VariantNode;
	case kind : FieldKind of
	FIELDVARIANT: (variantList : VariantList);
    end;
    FieldListRec = record
	next : FieldList;
	first, last : FieldNode;
    end;

    VariantNodeRec = record
	next : VariantNode;
	tag : ConstSetList;
	tagField : FieldNode;
	fieldList : FieldList;
    end;
    VariantListRec = record
	next : VariantList;
	first, last : VariantNode;
    end;

    EnumNodeRec = record
	next : EnumNode;
	name : String;
	enumSym : Symbol;
	enumType: TypeNode;
	enumOrd : integer;
    end;
    EnumListRec = record
	next : EnumList;
	first, last : EnumNode;
    end;

    VarNodeRec = record
	next : VarNode;
	name : String;
	readonly : boolean;
	changed : boolean;
	indirect : boolean;
	varType : TypeNode;
	address : Address;
	markTime : OptMarkTime;
    end;
    VarListRec = record
	next : VarList;
	first, last : VarNode;
    end;

    ModuleKind = (MODGLOBAL, MODPROGRAM, MODDEFINITION, MODIMPLEMENTATION);
    ModuleNodeRec = record
	next : ModuleNode;
	name : String;
	enclosing : ModuleNode;
	enclosingProc : ProcNode;
	kind : ModuleKind;
	unqualExports, qualExports : IdentList;
	imports : ImportList;
	doingImport : boolean;
	exportScope : Scope;
	procs : ProcList;
	modules : ModuleList;
	scope : Scope;
	body : StmtList;
    end;
    ModuleListRec = record
	next : ModuleList;
	first, last : ModuleNode;
    end;
    BuiltinProcType = (BIPNOTBIP, BIPABS, BIPCAP, BIPCHR, BIPDEC, BIPDISPOSE,
	BIPEXCL, BIPFLOAT, BIPHALT, BIPHIGH, BIPINC, BIPINCL, BIPNEW, BIPODD,
	BIPORD, BIPTRUNC, BIPVAL,
	BIPADR, BIPSIZE, BIPTSIZE, BIPBYTESIZE, BIPTBYTESIZE, BIPUNIXCALL,
	BIPOPENF, BIPCLOSEF, BIPEOFF, BIPEOLF, BIPWRITEF, BIPREADF, 
	BIPWRITEC, BIPREADC, BIPREADB, BIPREADW, BIPWRITEB, BIPWRITEW,
	BIPREADS, BIPWRITES,
	BIPLONGFLOAT, BIPNEWPROCESS, BIPTRANSFER, BIPVLIST,
	BIPMIN, BIPMAX, BIPASSERT, BIPNUMBER, BIPFIRST, BIPLAST,
	BIPBITAND, BIPBITOR, BIPBITNOT, BIPBITXOR, BIPBITSHIFTLEFT,
	BIPBITSHIFTRIGHT, BIPBITINSERT, BIPBITEXTRACT, BIPCONVERT, BIPCOERCE,
	BIPCPUTIME, BIPALLOCATE, BIPDEALLOCATE);

    ProcNodeRec = record
	next : ProcNode;
	fileName : String;
	lineNumber : integer;
	name : String;
	globalName : String;
	builtin : BuiltinProcType;
	procType : TypeNode;
	block : BlockNumber;
	scope : Scope;
	displayLevel : DisplayLevel;
	enclosing : ProcNode;
	enclosingModule : ModuleNode;
	body : StmtList;
	code : CodeList;
	mem : AllocationNode;
	tempMap : TempMapNode;
	initFlagVar : VarNode;
	containsProcs : boolean;
	internalProc : boolean;
	extern : boolean;
	tailRecursion : boolean;
	tailRecursionEntry : LabelNumber;
	doesUpLevel, containsUpLevel : set of DisplayLevel;
	varList : VarList;
    end;
    ProcListRec = record
	next : ProcList;
	first, last : ProcNode;
    end;

    ExprKind = (EXPRBAD, EXPRNAME, EXPRSYM, EXPRCONST, EXPRUNOP, EXPRBINOP,
	EXPRSUBSCR, EXPRDOT, EXPRDEREF, EXPRFUNC, EXPRVAR, EXPRSET, EXPRVAL,
	EXPRCHECK);
    ExprNodeRec = record
	next : ExprNode;
	fileName : String;
	lineNumber : integer;
	exprType : TypeNode;
	baseVar : VarNode;
	basePtrType : TypeNode;
	opt : OptIndex;
	case kind : ExprKind of
	    EXPRNAME : (exprName : IdentList);
	    EXPRSYM : (exprSym : Symbol);
	    EXPRCONST : (exprConst : ConstNode; constType : TypeNode);
	    EXPRVAR : (exprVar : VarNode);
	    EXPRUNOP : (exprUnOp : Token; opnd : ExprNode;
			unOperType : TypeNode);
	    EXPRBINOP : (exprBinOp : Token; opnd1, opnd2 : ExprNode;
			operType : TypeNode);
	    EXPRSUBSCR : (arr : ExprNode; subsExpr : ExprNode;
		    subsOffset : cardinal; subscripts : ExprList);
	    EXPRDOT : (rec : ExprNode; fieldName : String; field : FieldNode);
	    EXPRDEREF : (realPtr : boolean; ptr : ExprNode);
	    EXPRFUNC : (func : ExprNode; params : ExprList);
	    EXPRSET : (setTypeName : IdentList; setValue : ExprSetList);
	    EXPRVAL : (exprVal : ExprNode; dependVar : VarNode;
		    dependPtrType : TypeNode);
	    EXPRCHECK : (exprCheck : CheckKind; checkVar : VarNode;
		    checkExpr : ExprNode; checkField : FieldNode;
		    checkType : TypeNode; checkLower, checkUpper : cardinal);
    end;
    ExprListRec = record
	next : ExprList;
	first, last : ExprNode;
    end;

    StmtKind = (STMTBAD, STMTASSIGN, STMTPROC, STMTIF, STMTCASE, STMTWHILE,
		STMTREPEAT, STMTLOOP, STMTFOR, STMTWITH, STMTEXIT, STMTRETURN);
    StmtNodeRec = record
	next : StmtNode;
	fileName : String;
	lineNumber : integer;
	bad : boolean;
	case kind : StmtKind of
	    STMTASSIGN : (lhs, rhs : ExprNode; lhsType : TypeNode;
				assignOp : Token);
	    STMTPROC : (proc : ExprNode; params : ExprList);
	    STMTIF : (ifCond : ExprNode; thenList, elseList : StmtList);
	    STMTWHILE : (whileCond : ExprNode; whileBody : StmtList;
		whilePreEval : ExprList);
	    STMTREPEAT : (repeatCond : ExprNode; repeatBody : StmtList;
	        repeatPreEval : ExprList);
	    STMTLOOP : (loopBody : StmtList; saveLoopActive : boolean;
		loopPreEval : ExprList);
	    STMTFOR : (forIndexName : String; forFrom, forTo, forBy : ExprNode;
		forBody : StmtList; forIndexVar, forLimitVar : VarNode;
		forByConst : ConstNode; forSaveAlloc : AllocationNode;
		forPreEval : ExprList; forIndexType : TypeNode);
	    STMTWITH : (withQual : ExprNode; withQualNode : WithQualNode;
		withBody : StmtList; withSaveAllocNode : AllocationNode);
	    STMTRETURN : (returnVal : ExprNode);
	    STMTCASE : (caseSel : ExprNode; cases : CaseList;
		caseTree : CaseTreeNode; caseElse : StmtList);
    end;
    StmtListRec = record
	next : StmtList;
	first, last : StmtNode;
    end;
    ParamKind = (PARAMVAR, PARAMVALUE, PARAMARRAYVAR, PARAMARRAYVALUE);
    ParamNodeRec = record
	next : ParamNode;
	name : String;
	paramType : TypeNode;
	paramVar : VarNode;
	docopy : boolean;
	reference : boolean;
	case kind : ParamKind of
	PARAMVALUE : (paramListVar : VarNode);
	PARAMARRAYVAR, PARAMARRAYVALUE : (numElements : VarNode);
    end;
    ParamListRec = record
	next : ParamList;
	first, last : ParamNode;
    end;

    IdentNodeRec = record
	next : IdentNode;
	name : String;
    end;
    IdentListRec = record
	next : IdentList;
	first, last : IdentNode;
    end;

    ImportNodeRec = record
	next : ImportNode;
	fileName : String;
	lineNumber : integer;
	saveModule : ModuleNode;
	saveScope : Scope;
	fromIdent : String;
	idents : IdentList;
	searchList : IdentList;
	currSearch : IdentNode;
    end;
    ImportListRec = record
	next : ImportList;
	first, last : ImportNode;
    end;

    ConstSetNodeRec = record
	next : ConstSetNode;
	lower, upper : ConstNode;
    end;
    ConstSetListRec = record
	next : ConstSetList;
	first, last : ConstSetNode;
    end;
    ExprSetNodeRec = record
	next : ExprSetNode;
	lower, upper : ExprNode;
    end;
    ExprSetListRec = record
	next : ExprSetList;
	first, last : ExprSetNode;
    end;
    CaseNodeRec = record
	next : CaseNode;
	pcodeLabel : LabelNumber;
	labelExprs : ExprSetList;
	labelConsts : ConstSetList;
	stmts : StmtList;
    end;
    CaseListRec = record
	next : CaseList;
	first, last : CaseNode;
    end;
    CaseTreeNodeRec = record
	lower, higher : CaseTreeNode;
	first, last : cardinal;
	caseNode : CaseNode;
    end;

    SetValueRec = record
	next : SetValue;
	setType : TypeNode;
	value : set of 0..MAXSETSIZE;
    end;

    CodeType = (CODEMODULE, CODEPROC);
    CodeNodeRec = record
	next : CodeNode;
	stmts : StmtList;
	case kind : CodeType of
	    CODEMODULE : (module : ModuleNode);
	    CODEPROC : (proc : ProcNode);
    end;
    CodeListRec = record
	next : CodeList;
	first, last : CodeNode;
    end;

    GlobalVarNodeRec = record
	next : GlobalVarNode;
	number : integer;
	size : cardinal;
	name : String;
	extern : boolean;
    end;
    GlobalVarListRec = record
	next : GlobalVarList;
	first, last : GlobalVarNode;
    end;

    WithQualNodeRec = record
	next : WithQualNode;
	implQual : VarNode;
	recType : TypeNode;
	baseVar : VarNode;
	basePtrType : TypeNode;
    end;

var
    integerTypeNode, realTypeNode, charTypeNode, cardinalTypeNode,
    bitsetTypeNode, booleanTypeNode, wordTypeNode, byteTypeNode,
    addressTypeNode,
    stringTypeNode, anyTypeNode, nullTypeNode, opaqueTypeNode,
    procTypeNode, cardIntTypeNode, builtinProcTypeNode,
    fileTypeNode, arrayOfCharTypeNode, longrealTypeNode, realConstTypeNode,
    charConstTypeNode, processTypeNode : TypeNode;

    indexableTypes : set of DataType;
    addressableExprs : set of ExprKind;

    generateBlockNumber : BlockNumber;
    globalScope, currScope : Scope;
    globalModule, currModule : ModuleNode;
    globalProc, currProc : ProcNode;
    builtinScope, programModuleScope : Scope;
    withQualList : WithQualNode;
    globalPortList : PortList;
    loopActive : boolean;

    currLine : integer;
    currFile : String;


procedure InitSymTab; external;

procedure DumpSymTab; external;

function StartScope (open : boolean) : Scope; external;

procedure OpenScope (scope : Scope; open : boolean); external;

procedure EndScope; external;

function DefineSymbol (var sym : Symbol; name : String; scope : Scope;
	symCase : SymbolCase) : boolean; external;

function LookUpSymbol (name : String; scope : Scope; symCase : SymbolCase)
	: Symbol; external;

function QualifiedName (names : IdentList) : Symbol; external;

function TypeOf (names : IdentList) : TypeNode; external;

function Compatible (var dtn : TypeNode; den : ExprNode; var stn : TypeNode;
	sen : ExprNode) : TypeNode; external;

function Assignable (dtn : TypeNode; var stn : TypeNode; sen : ExprNode)
	: TypeNode; external;

function Passable (dtn : TypeNode; kind : ParamKind; var stn : TypeNode;
	sen : ExprNode) : boolean; external;

function Port(sym : Symbol; scope : Scope) : Symbol; external;

function AddToCodeList (list : CodeList; newOne : CodeNode)
	: CodeList; external;

function AddToSymbolList (list : SymbolList; newOne : SymbolNode)
	: SymbolList; external;

procedure CheckEqualSym(sym1, sym2 : Symbol); external;
