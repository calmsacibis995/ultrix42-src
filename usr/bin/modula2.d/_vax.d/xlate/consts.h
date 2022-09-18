(*#@(#)consts.h	4.1	Ultrix	7/17/90 *)
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
const

    BUFFSIZE = 512;         { size of buffer read from the input}

    MAXDISPLEVEL = 32;	{ maximum display level }

{ Names of built-in routines }
    IRTDISPOSE = 'runtime__deallocate '; { name of dispose external procedure }
    IRTNEW = 'runtime__allocate ';      { name of new external procedure }
    IRTSYSTEM = 'runtime__demcall ';    { name of system external procedure }
    IRTMAKESET = 'runtime__makeset ';
    IRTSMALLEST = 'runtime__smallest ';
    IRTLONGDIV = 'runtime__udiv ';
    IRTLONGMOD = 'runtime__umod ';
    IRTERRORASSERT = 'runtime__errorassert ';

    WORDSIZE = 32;
    HALFSIZE = 16;
    BYTESIZE = 8;
    ADDRSIZE = 32;      { target machine address size }
    BOOLSIZE = 8;	{ target machine boolean size }
    CHARSIZE = 8;   	{ target machine character size }
    REALSIZE = 32;
    INTSIZE = 32;

    MAXSETSIZE = 1024;		{ maximum set size (for constant generation)}
    MAXSETWORDS = 64;		{ number of 16-bit words in the largest set }

    maxoperands = 8;        { maximum number of operands on instruction }
    operandsize = 5000;     { maximum number of chars in operand or label  }
    opcodefieldsize = 4;    { number of chars in p-code opcode + 1 }
    opdsep = ',';           { separator char for p-code operands }
    TAB = '	';              { a tab character (not a space) }

    NUMFILES = 100;         { number of different files (for nam) }
    FNAMESIZE = 50;         { maximum file name size (for nam) }


    NUMOPCODES = 100;       { maximum number of pcode opcodes }
    NUMSTDPROCS = 50;       { maximum number of standard procedures }
    MAXTABLESIZE = 210;     { the size of the opcode hash table  }  
    NUMCOMBLOCKS = 1000;    { max number of common blocks }
