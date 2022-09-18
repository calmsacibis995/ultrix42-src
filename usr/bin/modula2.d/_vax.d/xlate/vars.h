(*#@(#)vars.h	4.1	Ultrix	7/17/90 *)
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
 *									    *
 ****************************************************************************)
var

{ current pcode line }
    pclabel :       operandstring;
    pclbsize :      integer;
    opcode :        opcodes;
    opcodestr :     opcodestring;
    opd :           opdptr;
    opdsizes :      opdsizeptr;
    opdcount :      integer;
    opd11 : 	    char; { opd[1][1] }

{ next pcode line }
    npclabel :       operandstring;
    npclbsize :      integer;
    nopcode :        opcodes;
    nopcodestr :     opcodestring;
    nopd :           opdptr;
    nopdsizes :      opdsizeptr;
    nopdcount :      integer;

{ current block number, display level and program name }
    progname :  operandstring;
    prognmsize :integer;
    curlev :	integer;
    curblockid:	integer;
    nodisplay : boolean;
    internal : boolean;
    pmemoff, tmemoff, mmemoff, smemoff, arsize, regmask : integer;
    mainprogblockid : integer;


{ counter to generate intruction labels }
    currentLabel :  LabelNumber;

{ counter to generate constant labels (block numbers) }
    currentConstant : integer;

{ opcode tables }
    hashTable : HashTable;         
    numopsused : integer;

{ map from characters to pcode types }
    associatedType :array [char] of pcodetype;

{ filename table for nam opcode }
    fnamtbl : array [0..NUMFILES] of record
	name : operandstring;
	size : integer;
    end;
    fntptr : integer;


{ options }
    checkrequested :boolean;
    ECHOPCODE, RUNIDS, VERBOSE, PRINTNAMES:  boolean;

{ deferred jump tables }
    jumpopposite :  array [jumpcond] of jumpcond;
    jumpnames :     array [jumpcond] of ShortString;
    nextjump : jumpcond;    { next jump to be done }

    errorcount :    integer;

{ current pcode line number }
    line :          integer;
		    
{ define common blocks (used by com opcode and WriteName }
    comtable  :  array [ 0 .. NUMCOMBLOCKS ] of Comtable;
    numcomblocks : 0 .. NUMCOMBLOCKS;

{ variables for builtin procedures }
    RTDISPOSE,
    RTNEW,
    RTSYSTEM,
    RTMAKESET,
    RTSMALLEST,
    RTLONGDIV,
    RTLONGMOD,
    RTERRORASSERT : operandstring;

