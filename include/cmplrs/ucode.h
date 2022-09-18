/* --------------------------------------------------------- */
/* | Copyright (c) 1986, 1989 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                                  | */
/* --------------------------------------------------------- */
/* $Header: ucode.h,v 2010.2.1.5 89/11/29 22:39:12 bettina Exp $ */

/* ref.		date		description				    */
/* !01		5jun85		adding symbol table modifications	    */
/* !02		30july85	0.11 changes				    */

/* LOD, ILOD, STR, ISTR flags (lexlev field) */
#include <ansi_compat.h>
#define VOLATILE_ATTR 1         /* data is volatile */

/* ABS, ADD, SQR, CVT, CVTL, DEC, INC, NEG, DIV, MOD, MPY, SUB and
   REM flags (lexlev field) */
#define OVERFLOW_ATTR 2		/* overflow checking required */

/* Data area designation (lexlev field)
 * Four bits are reserved to designate the data area.
 * Default means the data area is selected based upon size and initialization
 * This field occupies the high order 4 bits of the lexlevel field
 */
#define DATA_AREA_MASK 240      /* 0xF0 mask to isolate this field          */
#define DATA_AREA_BIT_OFS 4     /* number of bits to shift to get this field*/
#define DEFAULT_DATA_AREA 0     /* data defaults into appropriate data area */
#define READONLY_DATA_AREA 1    /* data put in readonly data area (rdata)   */
#define SMALL_DATA_AREA 2       /* data put in small data area (sdata,sbss) */
#define LARGE_DATA_AREA 3       /* data put in large data area (data,bss)   */
#define TEXT_AREA 4 	        /* data put in text area */

/* ENT flag attributes (extrnal field) */
#define EXTERNAL_ATTR       1	/* external entry point */
#define FRAMEPTR_ATTR       2	/* manifest real frame pointer */
#define PRESERVE_STACK_ATTR 4	/* prevent cutting back stack on exit */
#define STACK_OVERFLOW_ATTR  8	/* check for stack overflow */
#define LOAD_STACKLIMIT_ON_ENTRY_ATTR 16 /* load stack limit register */

/* CUP flag attributes (extrnal field) */
#define NOSIDEEFFECT_ATTR 1		/* indicates call has no side effect */
#define RETURN_ATTR 2			/* indicates call will not return */
/* 4 used in 1.31 for reloading stack limit for ada, but then removed in 2.0 */
#define REALLOC_ARG_ATTR 8              /* reallocate the arg build area */
#define GOTO_ATTR 16			/* indicates call is the Pascal GOOB*/
#define COMPOSITE_CALL_ATTR 32 		/* call returns a composite object */
/* CUP flag attributes (push field) */
#define STDARGS_NUM_MASK 224	/* 0xE0 mask to isolate this field */
#define STDARGS_NUM_BIT_OFS 5     /* number of bits to shift to get this field*/
#define STDARGS_NUM_MAX	4  /* max vararg starting position encodable */
			   /* beyond STDARGS_NUM_MAX, use STDARGS_NUM_MAX */
			   /* since there is no effect > 4 anyways (for now */

/* LAB flag attributes (lexlev field) */
#define GOOB_TARGET    1	/* label is target of GOOB (non-local goto) */
#define EXCEPTION_ATTR 2	/* label is jumped to due to an exception */
#define EXTERN_LAB_ATTR 4	/* label is referenced both externally and 
				   internally (for PL/1 only) */
#define IJP_ATTR 8		/* label is target of IJP (for f77 and PL/1) */

/* PDEF flag attributes (lexlev field) */
#define IN_MODE     1  		/* parameter passing modes */
#define OUT_MODE    2
#define INOUT_MODE  3

#ifdef LANGUAGE_PASCAL
(*****************************************************************************)
(* This file contains all types that define U-code			     *)
(*****************************************************************************)

/* macros to test and set attributes */
#define IS_VOLATILE_ATTR(x) (bitand(x, VOLATILE_ATTR) <> 0)
#define SET_VOLATILE_ATTR(x) x := bitor(x, VOLATILE_ATTR)
#define RESET_VOLATILE_ATTR(x) x := bitand(x, bitnot(VOLATILE_ATTR))
#define IS_FRAMEPTR_ATTR(x) (bitand(x, FRAMEPTR_ATTR) <> 0)
#define SET_FRAMEPTR_ATTR(x) x := bitor(x, FRAMEPTR_ATTR)
#define IS_EXTERNAL_ATTR(x) (bitand(x, EXTERNAL_ATTR) <> 0)
#define IS_OVERFLOW_ATTR(x) (bitand(x, OVERFLOW_ATTR) <> 0)
#define SET_OVERFLOW_ATTR(x) x := bitor(x, OVERFLOW_ATTR)
#define IS_PRESERVE_STACK_ATTR(x) (bitand(x, PRESERVE_STACK_ATTR) <> 0)
#define SET_PRESERVE_STACK_ATTR(x) x := bitor(x, PRESERVE_STACK_ATTR)
#define IS_EXCEPTION_ATTR(x) (bitand(x, EXCEPTION_ATTR) <> 0)
#define SET_EXCEPTION_ATTR(x) x := bitor(x, EXCEPTION_ATTR)
#define IS_EXTERN_LAB_ATTR(x) (bitand(x, EXTERN_LAB_ATTR) <> 0)
#define SET_EXTERN_LAB_ATTR(x) x := bitor(x, EXTERN_LAB_ATTR)
#define IS_IJP_ATTR(x) (bitand(x, IJP_ATTR) <> 0)
#define SET_IJP_ATTR(x) x := bitor(x, IJP_ATTR)
#define IS_STACK_OVERFLOW_ATTR(x) (bitand(x, STACK_OVERFLOW_ATTR) <> 0)
#define SET_STACK_OVERFLOW_ATTR(x)  x := bitor(x, STACK_OVERFLOW_ATTR)
#define	IS_LOAD_STACKLIMIT_ON_ENTRY_ATTR(x) (bitand(x, LOAD_STACKLIMIT_ON_ENTRY_ATTR) <> 0)
#define SET_LOAD_STACKLIMIT_ON_ENTRY_ATTR(x) x := bitor(x, LOAD_STACKLIMIT_ON_ENTRY_ATTR)
#define IS_REALLOC_ARG_ATTR(x) (bitand(x, REALLOC_ARG_ATTR) <> 0)
#define SET_REALLOC_ARG_ATTR(x) x := bitor(x, REALLOC_ARG_ATTR)

#define SET_DATA_AREA(x,v) x := bitor(bitand(x,bitnot(DATA_AREA_MASK)),lshift(v,DATA_AREA_BIT_OFS))
#define GET_DATA_AREA(x) rshift(bitand(x,DATA_AREA_MASK),DATA_AREA_BIT_OFS)
#define IS_RETURN_ATTR(x) (bitand(x, RETURN_ATTR) <> 0)
#define SET_RETURN_ATTR(x) x := bitor(x, RETURN_ATTR)
#define IS_NOSIDEEFFECT_ATTR(x) (bitand(x, NOSIDEEFFECT_ATTR) <> 0)
#define IS_GOTO_ATTR(x) (bitand(x, GOTO_ATTR) <> 0)
#define SET_GOTO_ATTR(x) x := bitor(x, GOTO_ATTR)
#define IS_COMPOSITE_CALL_ATTR(x) (bitand(x, COMPOSITE_CALL_ATTR) <> 0)
#define SET_COMPOSITE_CALL_ATTR(x) x := bitor(x, COMPOSITE_CALL_ATTR)
#define SET_STDARGS_NUM(x,v) x := bitor(bitand(x,bitnot(STDARGS_NUM_MASK)),lshift(v,STDARGS_NUM_BIT_OFS))
#define GET_STDARGS_NUM(x) rshift(bitand(x,STDARGS_NUM_MASK),STDARGS_NUM_BIT_OFS)

const
  (* set constant representation in Ucode				     *)
  Maxoperands = 10;			(* maximum number of operands in     *)
					(* u-code instruction + 1	     *)
  Maxinstlength = 8;			(* maximum size of a b-code	     *)
					(* instruction, in host words, = max *)
					(* (size of largest set constant (in *)
					(* bits), size of largest string     *)
					(* constant (in bits)) div wordsize+ *)
					(* 2;				     *)
type
  Datatype = (Adt,			(* address (pointer)		     *)
      Udt,				(* pointer to unaligned data         *)
      Fdt,				(* pointer to procedure		     *)
      Hdt,				(* address that only points to heap  *)
      Jdt,				(* integer, single word 	     *)
      Ldt,				(* non-negative integer, single word *)
      Mdt,				(* array or record		     *)
      Pdt,				(* procedure, untyped		     *)
      Qdt,				(* real, double word		     *)
      Rdt,				(* real, single word		     *)
      Sdt,				(* set				     *)
      Xdt,				(* extended precision		 !02 *)
      Gdt,				(* address of a label		     *)
      Ndt,				(* non-local label 	             *)
      Idt,				(* integer, double word		     *)
      Kdt,				(* unsigned integer, double word     *)
      XX3,			 	(* reserved			     *)
      Zdt); 				(* undefined			     *)


  Memtype  = (Zmt, 			(* undefined			     *)
      Mmt,				(* complex variables		     *)
      Pmt, 				(* parameters			     *)
      Rmt,				(* register			     *)
      Smt,				(* statically allocated memory	     *)
      Tmt,				(* return			     *)
      Amt				(* Parameter build area		     *)
      );

  (***************************************************************************)
  (* constants								     *)
  (***************************************************************************)
  Valuptr = ^Valu;
  Stringtext = record
		case boolean of
		false: (ss: packed array[1..Strglgth] of char);
		true:  (ssarray: array[1..
			(Strglgth+HostCharsPerWord-1) div HostCharsPerWord] of
			integer);
		end;
  Stringtextptr = ^Stringtext;
  Strg =
#ifdef PASTEL
    packed record
#else
    record
#endif
      Len      : 0..65535;
      Chars    : Stringtextptr;
    end {record};
  Valu =
    record				(* describes a constant value	     *)
      Ival: integer;
      case Datatype of
	Adt, Hdt, Ldt, Jdt, Fdt, Gdt, Ndt: ();
	Mdt, Qdt, Rdt, Sdt, Xdt: 					(*!02*)
	(* Ival gives the length of the string in Chars *)
	  (Chars    : Stringtextptr);
    end {record};

  (***************************************************************************)
  (* ucode instructions 						     *)
  (***************************************************************************)

  Uopcode  = (
	Uabs,	Uadd,	Uadj,	Uand,	Ubgn,	Uchkh,	Uchkl,	
	Uchkn,	Uchkt, 	Uclab,	Ucomm,	Ucsym,	Ucup,	Ucvt,	
	Ucvtl,  Ucia,	Udec,	Udef,	Udif, 	Udiv,	Udup,	
	Uend,	Uent,	Ueof, 	Uequ,	Uesym,	Ufjp,	Ugeq, 	
	Ufill,	Ugrt,	Ugsym,	Uicuf,	Uidx,	Uiequ,	Uigeq,	
	Uigrt, 	Uilda,  Uileq,	Uiles,	Uildv,  Uilod,	Uinc,	
	Uineq,	Uinit,  Uinn,	Uint, 	Uior,	Uisld,	Uisst,	
	Uistr,	Uistv,  Uixa,   Ulab,	Ulca,	Ulda, 	Uldc,	
	Uop56,	Uleq,   Ules,   Ulex,	Ulnot,	Uloc,	Ulod, 	
	Ulsym,	Umax,   Umin,   Umod,	Umov,	Umpy,	Umst,	
	Umus,	Uneg,   Uneq,   Unot,	Uop74,	Uodd,	Uoptn,	
	Upar,	Upmov,  Upop,   Uregs,	Uret,	Urlod,	Urnd,
	Urstr,	Usdef,  Usgs,   Ushl,	Ushr,	Usqr,   Ustp,
	Ustr,	Usub,   Uswp,	Utjp,	Uujp, 	Uuni,   Uop97,	
	Uop98,	Uop99,  Uop100,	Uop101,	Uop102,	Uop103, Uop104,
	Umovv,	Uvreg,  Uxjp,	Uxor,	Ubgnb,  Uendb,  Updef,  	(*!02*)
	Urem,   Usqrt,  Urldc,  Urlda,  Uijp,   Ucg1,   Ucg2, 
	Uaent,  Urpar,  Umpmv,  Uaos,   Uldsp,  Ustsp,  Uasym, 
	Uldap,  Uldef,  Ufsym,  Uunal,  Utpeq,  Utpne,  Utplt,
	Utpgt,  Utple,  Utpge,  Utyp,   Unop);				(*!01*)
/* Warning: Add ucodes at the end only, Unop must be last */


#if defined(PASTEL) || defined(PP)
  Bcrec = packed record
      case boolean of
	false : (
	  Opc	   : Uopcode options(align: 8, size: 8);
	  Dtype    : Datatype options(align: 8, size: 8);
	  Mtype    : Memtype options(align: 8, size: 8);
	  Lexlev   : 0..255 options(align: 8, size: 8);
	  I1	   : integer options(align: 32);
	  case Uopcode of
	    Ucvt : (
	      Dtype2   : Datatype options(align: 8, size: 8));
#else
  Bcrec = record
      case boolean of
	false : (
	  Opc	   : Uopcode;		{ 7 bits }
	  Dtype    : Datatype;		{ 4 bits }
	  Mtype    : Memtype;		{ 3 bits }
	  Lexlev   : 0..255;		{ 8 bits }
	  I1	   : integer;
	  case Uopcode of
	    Ucvt : (
	      Dtype2 : Datatype);
#endif
	    Uent, Ucup: (
		  Pop, Push, Extrnal : 0..255);				/*!01*/
	    Uiequ : (
	      Length : integer;
	      case Uopcode of
		Uldc : (
		  Constval : Valu);
		Uiequ : (
		  Offset : integer;
		  case Uopcode of
		    Uinit : (
		      Offset2  : integer;
		      aryoff   : integer;
		      Initval  : Valu);
		    Uxjp : (
		      Label2   : cardinal )
		)
	    )
	);
	true : (
	  Intarray : array[1..Maxinstlength] of integer);
    end {record};

  (***************************************************************************)
  (* source line buffer 						     *)
  (***************************************************************************)
  Sourceline = packed array[1..Strglgth] of char;

  Opcstring = packed array[1..4] of char; (* string representation of an     *)
					(* Uopcode			     *)
  (* different types of operands in a u-code inustrtion 		     *)

  Uoperand = (Sdtype, Smtype, Slexlev, Slabel0, Slabel1, Sblockno, Sdtype2,
	  Spname0, Spname1, Spop, Spush, Sexternal, Scheckval, Slength, 
	  Sconstval, Scomment, Sareaname, Soffset, Svname0, Ssomenumber,
	  Soffset2, Sinitval, Slabel2, SarrayOffset, Sdtypenum, Smtypenum, Send);
  (* describes the order and type of operands in a u-code inustrtion	     *)
  Uops	   = array[1..Maxoperands] of Uoperand;

  utabrec  =
    record
      Format   : Uops;			(* operands			     *)
      Opcname  : Opcstring;		(* opcode name table		     *)
      Hasattr,				(* true if using lexlev field for \v
					   and \o attributes *)
      Hasconst : boolean;		(* true if instruction requirs	     *)
					(* constant			     *)
      Instlength : 1..Maxinstlength;	(* length of instruction	     *)
      stack_pop: 0..3;			{whether leaf, unary or binary op}
      stack_push: 0..1;			{whether statement or expression}
    end {record};
#endif

#if defined (__LANGUAGE_C)
/* This file MUST correspond to the Pascal definitions found above.
   This represents a fast transliteration of part of it to C (by
   no means is it complete, although the structure was successfully
   used to write binary UCODES and read them back by Pascal programs
   that read the binary form of UCODE).			*/

/*****************************************************************************/
/* This file contains all types that define U-code			     */
/*****************************************************************************/
  /* set constant representation in Ucode				     */
#define  Maxoperands  10		/* maximum number of operands in     */
					/* u-code instruction + 1	     */
#define  Maxinstlength  8		/* maximum size of a b-code	     */
					/* instruction, in host words, = max */
					/* (size of largest set constant (in */
					/* bits), size of largest string     */
					/* constant (in bits)) div wordsize+ */
					/* 2;				     */

/* macros to test and set attributes */
#define IS_VOLATILE_ATTR(x) (x & VOLATILE_ATTR) 
#define SET_VOLATILE_ATTR(x) x = (x | VOLATILE_ATTR)
#define IS_FRAMEPTR_ATTR(x) (x & FRAMEPTR_ATTR) 
#define SET_FRAMEPTR_ATTR(x) x = (x | FRAMEPTR_ATTR)
#define IS_EXTERNAL_ATTR(x) (x & EXTERNAL_ATTR) 
#define IS_OVERFLOW_ATTR(x) (x & OVERFLOW_ATTR) 
#define SET_OVERFLOW_ATTR(x) x = (x | OVERFLOW_ATTR)
#define IS_PRESERVE_STACK_ATTR(x) (x & PRESERVE_STACK_ATTR)
#define SET_PRESERVE_STACK_ATTR(x) x = (x | PRESERVE_STACK_ATTR)
#define IS_EXCEPTION_ATTR(x) (x & EXCEPTION_ATTR)
#define SET_EXCEPTION_ATTR(x) x = (x | EXCEPTION_ATTR)
#define IS_EXTERN_LAB_ATTR(x) (x & EXTERN_LAB_ATTR)
#define SET_EXTERN_LAB_ATTR(x) x = (x | EXTERN_LAB_ATTR)
#define IS_STACK_OVERFLOW_ATTR(x) (x & STACK_OVERFLOW_ATTR)
#define SET_STACK_OVERFLOW_ATTR(x)  x = (x |  STACK_OVERFLOW_ATTR)
#define	IS_LOAD_STACKLIMIT_ON_ENTRY_ATTR(x) (x & LOAD_STACKLIMIT_ON_ENTRY_ATTR)
#define SET_LOAD_STACKLIMIT_ON_ENTRY_ATTR(x) x = (x | LOAD_STACKLIMIT_ON_ENTRY_ATTR)
#define IS_REALLOC_ARG_ATTR(x) (x & REALLOC_ARG_ATTR)
#define SET_REALLOC_ARG_ATTR(x) x = (x | REALLOC_ARG_ATTR)

#define SET_DATA_AREA(x,v) x = ((x & ~DATA_AREA_MASK)|(v << DATA_AREA_BIT_OFS))
#define GET_DATA_AREA(x) ((x & DATA_AREA_MASK) >> DATA_AREA_BIT_OFS)
#define IS_RETURN_ATTR(x) (x & RETURN_ATTR)
#define SET_RETURN_ATTR(x) x = (x | RETURN_ATTR)
#define IS_NOSIDEEFFECT_ATTR(x) (x & NOSIDEEFFECT_ATTR)
#define IS_GOTO_ATTR(x) (x & GOTO_ATTR)
#define SET_GOTO_ATTR(x) x = (x | GOTO_ATTR)
#define IS_COMPOSITE_CALL_ATTR(x) (x & COMPOSITE_CALL_ATTR)
#define SET_COMPOSITE_CALL_ATTR(x) x = (x | COMPOSITE_CALL_ATTR)
#define SET_STDARGS_NUM(x,v) x = ((x & ~STDARGS_NUM_MASK)|(v << STDARGS_NUM_BIT_OFS))
#define GET_STDARGS_NUM(x) ((x & STDARGS_NUM_MASK) >> STDARGS_NUM_BIT_OFS)

  enum
  Datatype  {Adt,			/* address (pointer)		     */
      Udt,				/* procedure entry point	     */
      Fdt,				/* C pointer to function	     */
      Hdt,				/* address that only points to heap  */
      Jdt,				/* integer, single word 	     */
      Ldt,				/* non-negative integer, single word */
      Mdt,				/* array or record		     */
      Pdt,				/* procedure, untyped		     */
      Qdt,				/* real, double word		     */
      Rdt,				/* real, single word		     */
      Sdt,				/* set				     */
      Xdt,				/* extended precision		 !02 */
      Gdt,				/* address of label		    */
      Ndt,				/* non-local labels		     */
      Idt,				/* integer, double word		     */
      Kdt,				/* unsigned integer, double word     */
      XX3,			 	/* reserved			     */
      Zdt}; 				/* undefined			     */


enum
Memtype
 {    Zmt, 				/*				     */
      Mmt,				/* complex variables		     */
      Pmt, 				/* parameters			     */
      Rmt,				/* register			     */
      Smt,				/* statically allocated memory	     */
      Tmt,				/* temporaries			     */
      Amt				/* Parameter build area	   	     */
 } ;



  /***************************************************************************/
  /* constants								     */
  /***************************************************************************/
typedef char Stringtext[Strglgth];

typedef struct Valu {
  int Ival;
  char  *Chars;
};

  /***************************************************************************/
  /* ucode instructions 						     */
  /***************************************************************************/

enum 
  Uopcode   {
	Uabs,	Uadd,	Uadj,	Uand,	Ubgn,	Uchkh,	Uchkl,	
	Uchkn,	Uchkt, 	Uclab,	Ucomm,	Ucsym,	Ucup,	Ucvt,	
	Ucvtl,  Ucia,	Udec,	Udef,	Udif, 	Udiv,	Udup,	
	Uend,	Uent,	Ueof, 	Uequ,	Uesym,	Ufjp,	Ugeq, 	
	Ufill,	Ugrt,	Ugsym,	Uicuf,	Uidx,	Uiequ,	Uigeq,	
	Uigrt, 	Uilda,  Uileq,	Uiles,	Uildv,  Uilod,	Uinc,	
	Uineq,	Uinit,  Uinn,	Uint, 	Uior,	Uisld,	Uisst,	
	Uistr,	Uistv,  Uixa,   Ulab,	Ulca,	Ulda, 	Uldc,	
	Uop56,	Uleq,   Ules,   Ulex,	Ulnot,	Uloc,	Ulod, 	
	Ulsym,	Umax,   Umin,   Umod,	Umov,	Umpy,	Umst,	
	Umus,	Uneg,   Uneq,   Unot,	Uop74,	Uodd,	Uoptn,	
	Upar,	Upmov,  Upop,   Uregs,	Uret,	Urlod,	Urnd,
	Urstr,	Usdef,  Usgs,   Ushl,	Ushr,	Usqr,   Ustp,
	Ustr,	Usub,   Uswp,	Utjp,	Uujp, 	Uuni,   Uop97,	
	Uop98,	Uop99,  Uop100,	Uop101,	Uop102,	Uop103, Uop104,
	Umovv,	Uvreg,  Uxjp,	Uxor,	Ubgnb,  Uendb,  Updef,  	
	Urem,   Usqrt,  Urldc,  Urlda,  Uijp,   Ucg1,   Ucg2, 
	Uaent,  Urpar,  Umpmv,  Uaos,   Uldsp,  Ustsp,  Uasym, 
	Uldap,  Uldef,  Ufsym,  Uunal,  Utpeq,  Utpne,  Utplt,
	Utpgt,  Utple,  Utpge,	Utyp,   Unop};			/*!01*/
/* Warning: Add ucodes at the end only, Unop must be last */

struct Bcrec   { 
	  unsigned int  Opc :8;	
	  unsigned int  Dtype :8;	
	  unsigned int  Mtype :8;	
	  unsigned int  Lexlev :8;  		
	  int  I1;         
	  union {
	    struct {
	      enum Datatype Dtype2:8; unsigned :24;
	    }secondty;
	    struct {
	       unsigned int Pop :8, Push :8, Extrnal :8;
	    }uent;
            struct {
	      int Length;
	      union {
		struct Valu Constval;
		struct {
		  int offset;
		  union {
		  struct {
		      int offset2;
		      int aryoff;
		      struct Valu initval;
		    }uinit ;
		    unsigned Label2;
		 }uop3;  
		}uiequ2;
             }uop2;
	    }uiequ1;
	   }Uopcde;
	   };

union Bcode {
  struct Bcrec Ucode;
  int intarray[Maxinstlength];
}Uc;

/* access paths to members of the U_code structure */
#define UCVT    Uopcde
#define UENT    Uopcde.uent
#define UCHKL   Uopcde
#define UIEQU   Uopcde.uiequ1
#define UIEQU2  Uopcde.uiequ1.uop2.uiequ2
#define OPC     Uc.Ucode.Opc
#define DTYPE   Uc.Ucode.Dtype
#define MTYPE   Uc.Ucode.Mtype
#define LEXLEV  Uc.Ucode.Lexlev
#define IONE    Uc.Ucode.I1
#define DTYPE2  Uc.Ucode.Uopcde.secondty.Dtype2
#define POP     Uc.Ucode.Uopcde.uent.Pop
#define PUSH    Uc.Ucode.Uopcde.uent.Push
#define EXTRNAL Uc.Ucode.Uopcde.uent.Extrnal
#define CHECKVAL Uc.Ucode.Uopcde.Checkval
#define LENGTH  Uc.Ucode.Uopcde.uiequ1.Length
#define CONSTVAL Uc.Ucode.Uopcde.uiequ1.uop2.Constval
#define OFFSET   Uc.Ucode.Uopcde.uiequ1.uop2.uiequ2.offset
#define OFFSET2  Uc.Ucode.Uopcde.uiequ1.uop2.uiequ2.uop3.uinit.offset2
#define ARYOFF   Uc.Ucode.Uopcde.uiequ1.uop2.uiequ2.uop3.uinit.aryoff
#define INITVAL  Uc.Ucode.Uopcde.uiequ1.uop2.uiequ2.uop3.uinit.initval
#define LABEL2   Uc.Ucode.Uopcde.uiequ1.uop2.uiequ2.uop3.Label2

enum uoperand {
	Sdtype, Smtype, Slexlev, Slabel0, Slabel1, Sblockno, Sdtype2,
	Spname0, Spname1, Spop, Spush, Sexternal, Scheckval, Slength, 
	Sconstval, Scomment, Sareaname, Soffset, Svname0, Ssomenumber,
	Soffset2, Sinitval, Slabel2, SarrayOffset, Sdtypenum, Smtypenum, Send };


#endif /* (C) */
