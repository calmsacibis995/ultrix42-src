/* --------------------------------------------------------- */
/* | Copyright (c) 1986, 1989 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                                  | */
/* --------------------------------------------------------- */
/* $Header: usys.h,v 2010.2.1.5 89/11/29 22:39:18 bettina Exp $ */

#include <ansi_compat.h>
#if defined(LANGUAGE_PASCAL) || defined(PASTEL)
#ifdef __mips

(* system-wide constants and types *)

const 
  Machine  = 2000;                      (* Machine ID.                       *)

  Identlength = 32;                     (* size of identifiers               *)
  Blankid  = '                                ';
#ifdef  BSD
  Filenamelen = 1024;             	(* maximum length file name in       *)
#endif /* BSD */
#ifdef  SYSV
  Filenamelen = 1024;               	(* maximum length file name in       *)
#endif /* SYSV */
					(* target operating system           *)
#if 0	/* Dec-1-87 */
  BlankFilename = '                                                                                    ';
#endif
  Strglgth = Filenamelen;               (* maximum size of a string constant *)
  UseBcode = true;                     (* use binary form of Ucode flag     *)
  HostCharsPerWord = 4;
  Maxsetsize = 512;                     (* maximum size of a set constant    *)

  (***************************************************************************)
  (* Uscan interface                                                         *)
  (***************************************************************************)
  Maxswitches = 15;                     (* maximum number of switches user   *)
					(* can set in command line           *)
  Maxfiles = 10;                        (* maximum number of files user can  *)
					(* specify in command line           *)

  Charlen  = 32;                        (* Length of component of text file  *)
  Tabsetting = 8;                       (* Spaces per tab                    *)

  (***************************************************************************)
  (* Character set. Must be same on host and target.                         *)
  (***************************************************************************)
  Eolchars = 1;                         (* number of end of line chars       *)
  Eopchar  = 12;                        (* end of page char, usually form    *)
					(* feed                              *)
  Tab      = 9;                         (* ord (tab)                         *)
  Ordlastchar = 127;                    (* Highest ORD of character in a     *)
					(* Text file                         *)


  Wordsize = 32;
  Doublewordsize = 64;
  Wordalign = 32;
  Doublewordalign = 64;
  Bytesize = 8;                         (* size of fixed-length byte         *)
  Setunitsize = 32;
  Setunitmax = 31;
  Defsetsize = 128;                     (* default size of a set             *)

#if 0
  Maxreg   = 63;                        (* >= number of registers on         *)
					(* machine; <= maximum set element   *)
					(* on host compiler.                 *)

  Labchars = 16;                        (* length of external names          *)
  Modchars = 12;                        (* number of significant characters  *)
					(* in a module name                  *)
#endif
  Localsbackwards = true;		(* assign locals in negative         *)
					(* direction?                        *)
#if 0
  Pmem     = true;                      (* use parameter memory?             *)
#endif

  Addrunit = 8;  	                (* size of addressable unit (e.g.    *)
					(* byte size on a byte-addressable   *)
					(* machine)                          *)

  Salign   = 32;                        (* simple types are guaranteed never *)
					(* to cross a boundary of this many  *)
					(* bits                              *)
#if 0
  Regsize  = Wordsize;                  (* size of a register variable       *)
#endif
  VarAlign = Wordsize;                  (* alignment of variables (in bits)  *)
  RecAlign = Addrunit;			(* alignment of fields of unpacked   *)
					(* records                           *)
  ArrAlign = Addrunit;                  (* alignment of elements of unpacked *)
					(* arrays                            *)
  Fpackunit = Addrunit;                 (* alignment of fields of packed     *)
					(* files                             *)
  Rpackunit = 1;			(* alignment of fields of packed     *)
					(* records                           *)
  Apackunit = 1;			(* alignment of elements of packed   *)
					(* arrays                            *)
  Apackeven = true;                     (* pack arrays evenly?               *)
  SpAlign  = Wordsize;                  (* DEFs will always be multiples of  *)
					(* this                              *)
  HeapAlign = Wordsize;                 (* Sizes of heap variables will      *)
					(* always be multiples of this       *)

  Realintsep = false;  (* whether separate static areas for reals and integers,
			   as in FOM *)

  (***************************************************************************)
  (* sizes of unpacked data types, in bits -- must always be a multiple of   *)
  (* Addrunit                                                                *)
  (***************************************************************************)
  Intsize  = Wordsize;                  (* size of integer                   *)
  Intalign = Wordalign;
  Realsize = Wordsize;                  (* size of real                      *)
  Realalign = Wordalign;
  Doublesize = Doublewordsize;		(* size of double                    *)
  Doublealign = Doublewordalign;
  Extendedsize = 3*Wordsize;		(* size of extended		     *)
  Extendedalign = Doublewordalign;
  Pointersize = Wordsize;		(* size of a pointer (address)       *)
  Pointeralign = Wordalign;
  Boolsize = Addrunit;                  (* size of a boolean                 *)
  Boolalign = Addrunit;
  Charsize = Addrunit;                  (* size of a character               *)
  Charalign = Addrunit;
  Pcharsize = 8;			(* minimum packing of characters     *)
  Entrysize = 2*Wordsize;		(* size of a procedure descriptor    *)
  Entryalign = Doublewordalign;		(* (type E)                          *)
  Psetsize = 1;                         (* minimum packing of sets           *)
  CharsperSalign = 4;                   (* packed chars per salign unit      *)

  Parthreshold = 2*Wordsize;		(* simple objects larger than this   *)
					(* will be passed indirectly         *)

#if 0
  DefLocalsinregs = 2;                  (* default number of locals in       *)
					(* registers                         *)
  Maxlocalsinregs = 4;                  (* maximum number of locals in       *)
					(* registers                         *)
#endif
  Uoptalloc = true;                     (* whether uopt should perform       *)
					(* register alloc.                   *)
#if 0
  Uoptregs = 8;                        (* number of registers reserved for  *)
					(* uopt                              *)
  Numregclass = 1;                      (* number of classes of registers    *)
					(* (either 1 or 2)                   *)
  Class2reg = 9;                       (* if no 2nd register class, =       *)
					(* uoptregs+1; oe, the first         *)
					(* register which is of the 2nd      *)
					(* class                             *)
  movcost  = 1.2;                       (* the execution cost of a           *)
					(* register-memory transfer          *)
  reglodsave = 1.0;                     (* the saving of loading from a reg  *)
					(* instead of from memory            *)
  regstrsave = 1.0;                     (* the saving of storing to a reg    *)
					(* instead of tomemory               *)

  ParmsinRegs = false;                  (* pass parameters in registers?     *)
  Calleesave = false;                   (* calleesave linkage convention?    *)
#endif
  (***************************************************************************)
  (* for definition of standard types on target machine                      *)
  (***************************************************************************)

  Tgtfirstchar = 0;                     (* lower bound of type char          *)
  Tgtlastchar = 127;                    (* upper bound of type char          *)
  Tgtmaxint = 2147483647;               (* largest integer                   *)
  Tgtminint = -2147483648;		(* smallest integer                  *)
#if 0
  Maxintdiv10 = 214748364;              (* for testing for overflow          *)
  Maxintmod10 = 7;                      (* for testing for overflow          *)
  Tgtmaxexp = 36;                       (* exponent of largest real          *)
  Tgtmaxman = 0.8507059173;             (* mantissa of largest real          *)
  Tgtminexp = -36;                      (* exponent of smallest pos real     *)
  Tgtminman = 0.14693680107;            (* mantissa of smallest pos real     *)
#endif
  Maxsetval = 1151;

  Lngrealsize = DoubleWordsize;
#if 0
  Lngint   = DoubleWordsize;
  Functhreshold = Wordsize;
#endif

  iob_size = 24*addrunit;		(* size of _iobuf struct *)

type
  Identname = packed array[1..Identlength] of char;
  Filename = packed array[1..Filenamelen] of char;

#endif /* defined(__mips) */
#endif /* defined(LANGUAGE_PASCAL) || define(PASTEL) */

#if defined(__LANGUAGE_C)
#ifdef __mips

/* system-wide constants and types */

#define  Machine   2001                 /* Machine ID.                       */
#define  Identlength  32                /* size of identifiers               */
#define  HostCharsPerWord 4
#define  bytes_per_target_word 4	/* bytes in the target word */
#define  Bytesize  8		        /* size of fixed-length byte         */
#ifdef  BSD
#define Filenamelen 1024
#endif /* BSD */
#ifdef  SYSV
#define Filenamelen 1024
#endif /* SYSV */
#define  Strglgth   Filenamelen         /* maximum size of a string constant */

typedef char Filename[Filenamelen];
typedef char Identname[Identlength];

#else /* !defined(mips) */

/* system-wide constants and types */
#define   Machine  11		/* Machine ID. */
#define   Identlength  32	/* size of identifiers */
#define   Strglgth  288		/* maximum size of a string constant */
#define   HostCharsPerWord 4

typedef char Identname[Identlength];

#endif /* defined(__mips)			   */
#endif /* define(__LANGUAGE_C) 			   */

