/*        @(#)portab_gen.h	4.1      7/2/90      */

/* File: portab_gen.h
 *
 *	COPYRIGHT  (c)  DIGITAL  EQUIPMENT CORPORATION 1987,
 *	1988, 1989.  ALL RIGHTS RESERVED.
 *
 *	THIS SOFTWARE IS FURNISHED UNDER A LICENSE AND MAY BE
 *	USED AND COPIED ONLY IN ACCORDANCE  WITH THE TERMS OF
 *	SUCH  LICENSE  AND  WITH  THE  INCLUSION OF THE ABOVE
 *	COPYRIGHT  NOTICE.  THIS SOFTWARE OR ANY OTHER COPIES
 *	THEREOF   MAY  NOT  BE  PROVIDED  OR  OTHERWISE  MADE
 *	AVAILABLE  TO  ANY  OTHER  PERSON.  NO  TITLE  TO AND
 *	OWNERSHIP  OF  THE  SOFTWARE  IS  HEREBY TRANSFERRED.
 *
 *	THE INFORMATION IN THIS SOFTWARE IS SUBJECT TO CHANGE
 *	WITHOUT  NOTICE  AND SHOULD  NOT BE  CONSTRUED  AS A
 *	COMMITMENT  BY  DIGITAL EQUIPMENT CORPORATION.
 *
 *	DIGITAL  ASSUMES  NO RESPONSIBILITY  FOR THE  USE  OR
 *	RELIABILITY  OF ITS SOFTWARE ON EQUIPMENT THAT IS NOT
 *	SUPPLIED BY DIGITAL.
 *
 */


/*
 *----------------------------------------------------------------------------
 *
 *  begin description
 *
 *	PORTAB_GEN.H - INCLUDE file to provide portable data declarations.
 *
 *	This file is called by one of four files:
 *	  portab_vax for VAXC implementations
 *	  portab_shar for VAXC sharable implementations
 *	  portab_latt for Lattice iapx 86 C implementations
 *        portab_ultrix for Ultrix C implementations
 *
 *  end description
 *
 *----------------------------------------------------------------------------
 */
 


/*
 *----------------------------------------------------------------------------
 *
 *  begin edit history
 *
 * Edit:	gh	 11-MAR-1986 10:39:51 Created 
 *
 *		gh	 10-APR-1986 13:07:00 Changed VOID declaration, O_RAW
 *
 *		jj	 16-APR-1986 08:25:00 Eliminated BYTE, ULONG and 
 *			NOSHARE, and changed NULLPTR for Lattice compatability.
 *
 *   001   30-OCT-1987 18:21:59 mhs
 *        Move general code above the if/then structures, and correct the 
 *        current if/then structures for the new LATT and SHAR flags.  
 *        Reinstate BYTE, ULONG & NOSHARE for backwards compatibility, add 
 *	  BOOL as shorter form of BOOLEAN, add EXT as shorter form of 
 *	  EXTERNAL, add DEF as shorter form of DEFAULT.
 *
 *   002   25-NOV-1987 15:05 mhs
 *        Change defines to typedefs.
 *			
 *   003   30-NOV-1987 11:01 bf
 *        Removed definitions of MIN, MAX, FOREVER and END_FOREVER.  These
 *        are to be placed in file specifically for C code macros. Place
 *        is TBD.  Also removed typedef for VOID, void to int for Lattice,
 *        as Lattice now supports void.
 *
 *   004   30-NOV-1987 11:53 bf
 *        Changed typedefs for REG, LOCAL, EXT, EXTERNAL and MLOCAL into 
 *        #define macros.  Can't typedef storage class specifiers.
 *
 *   005    4-DEC-1987 15:39 mhs
 *        Updated VAXC CONST to be defined as recommended const vs. readOBonly.
 *
 *   006   10-DEC-1987 14:26 bf
 *        VAX C doesn't like typedef of VOID to void.  Changed this
 *        back to #define.
 *
 *   007   17-DEC-1987 08:37 mhs
 *      Support CONST for Lattice using "initialized" construct.
 *
 *  008  24-DEC-1987 10:28 mhw
 *	changed VAX to CVAX to work with makehf
 *
 *  009   5-JAN-1988 15:08 mhw
 *      return CONST definition to readonly, const not supported
 *
 * 010    6-JAN-1988 10:32 mhw
 *	define READONLY for xlate files
 *
 * 011   12-JAN-1988 11:35 mhw
 *      created different definitions for NULLPTR for VAX and Lattice
 *
 * 012   21-JAN-1988 16:01 mhs
 *      Remove broken Lattice CONST defn and switch VAXC defn to const
 *
 * 013   26-JAN-1988 12:29 mhs
 *      Remove VAXC defn for CONST due to current compiler bug with const
 *
 * 014	11-FEB-1988 16:44 araj
 *	Remove CONST again
 *
 * 0015 23-FEB-1988 11:03 mhw
 *      Add defs for pointers to X
 *
 *   6-MAR-1988 11:32 bf
 *	Add macros for strcpy and strncpy routines, to avoid calling
 *	library routines in code.
 *
 *   7-MAR-1988 11:42 bf
 *	Make strXcpy macros only available for Lattice.
 *
 *   7-MAR-1988 13:27 bf
 *	Move strXcpy macros out of here into l4clibrary.
 *
 *  10-MAR-1988 21:55 araj
 *	Add pointer to Uword, Ubyte, Long, Ulong
 *
 *  20-MAY-1988 20:58 araj
 *	Added BOOLEAN so we can use the long form
 *
 *  27-FEB-1989 cp
 *     Added code to support Ultrix implementation
 *
 *  24-APR-1989 18:18	araj
 *	Added GET_LONG and PMAX portability
 *
 *  end edit history
 *
 *----------------------------------------------------------------------------
 */



#ifdef	CVAX				/* VAX version */
/*
 *----------------------------------------------------------------------------
 *	Definitions for VAXC compiler
 *----------------------------------------------------------------------------
 */

typedef char BYTE;
typedef BYTE * BYP;
typedef unsigned char UBYTE, BOOL, BOOLEAN;
typedef short WORD;
typedef unsigned short UWORD;
typedef long LONG, DWORD;		/* doublelongword = longword on VAX */
typedef unsigned long ULONG;
typedef int DEF, DEFAULT;		/* default integer size */

#define VOID	void			/*  VAX C won't typedef this. */

#define REG     register
#define LOCAL   auto
#define EXT     extern			/* external datatype */
#define EXTERNAL extern			/* superseded by EXT */
#define MLOCAL  static
#define GLOBAL	/**/			/* global is the default */
#define CONST	NOSHARE			/* for ansi compatability */
#define READONLY readonly		/* for use with xlate files */

#define O_RAW	(0)			/* for Lattice open() compatability */

#define NULLPTR ((BYP)0)		/* Value for a NULL pointer */
#define STRING_TERMINATOR ('\0')

#define GET_LONG(p) (*(LONG *)(p))

#ifdef SHAR
#define NOSHARE	/**/
#else
#define NOSHARE noshare			/* for multi-user applications */
#endif

#endif





#ifdef LATT				/* Lattice version */
/*
 *----------------------------------------------------------------------------
 *	Definitions for Lattice iapx 86 C compiler
 *----------------------------------------------------------------------------
 */

typedef char BYTE;
typedef BYTE * BYP;
typedef unsigned char UBYTE, BOOL, BOOLEAN;
typedef int WORD;
typedef unsigned int UWORD;
typedef long LONG, DWORD;
typedef unsigned long ULONG;
typedef int DEF, DEFAULT;		/* default integer size */
typedef void VOID;

#define REG      register
#define LOCAL    auto
#define EXT      extern
#define EXTERNAL extern
#define MLOCAL   static
#define GLOBAL	/**/
#define CONST	/**/			/* for ansi compatability */

#define O_RAW	/**/
#define NOSHARE	/**/


#define NULLPTR  ((BYP)0)		/* Value for a NULL pointer */
#define STRING_TERMINATOR ('\0')

#define GET_LONG(p) (*(LONG *)(p))


#endif

#ifdef ULTRIX				/* VAX Ultrix version */
/*
 *----------------------------------------------------------------------------
 *	Definitions for Ultrix C compiler
 *----------------------------------------------------------------------------
 */

typedef char BYTE;
typedef BYTE * BYP;
typedef unsigned char UBYTE, BOOL, BOOLEAN;
typedef short WORD;
typedef unsigned short UWORD;
typedef long LONG, DWORD;
typedef unsigned long ULONG;
typedef int DEF, DEFAULT;		/* default integer size */

/* Ultrix C won't typedef this, either */
#define VOID void 

#define REG      register
#define LOCAL    auto
#define EXT      extern
#define EXTERNAL extern
#define MLOCAL   static
#define GLOBAL  
#define CONST 

#define O_RAW	
#define NOSHARE 
#define READONLY

#define NULLPTR  ((BYP)0)		/* Value for a NULL pointer */
#define STRING_TERMINATOR ('\0')



#ifdef PMAX				/* PMAX version */

#define GET_LONG(p) ((*(UWORD *)(p)) | ((*(UWORD *)((p)+2))<<16)) 

#else

#define GET_LONG(p) (*(LONG *)(p))

#endif
#endif


/*
 *----------------------------------------------------------------------------
 *	Miscellaneous definitions to eliminate need for stdio.h
 *----------------------------------------------------------------------------
 */

#define FAILURE  (-1)
#define ERROR    (-1)

#ifndef EOF
#define EOF      (-1)
#endif

#define SUCCESS  (0)

#ifndef NULL
#define NULL     (0)
#endif

#define FALSE    (0)
#define TRUE     (1)




/*
 *----------------------------------------------------------------------------
 *	Global Definitions
 *----------------------------------------------------------------------------
 */




typedef DEF AD[1];		/* AD is an array of DEFault */
typedef AD *PAD;		/* PAD is ptr to array of DEFault */

typedef LONG AL[1];		/* AL is an array of LONG	  */
typedef AL *PAL;		/* PAL is ptr to array of LONG	  */

typedef WORD AW[1];		/* AW is an array of WORD */
typedef AW *PAW;		/* PAW is ptr to array of WORD */

typedef BYTE AB[1];		/* AB is an array of BYTE */
typedef AB *PAB;		/* PAB is ptr to array of BYTE */


typedef DEF (*PFD)();		/* PFD is ptr to a func that returns DEFault */
typedef WORD (*PFW)();		/* PFW is ptr to a func that returns WORD */
typedef BYTE (*PFB)();		/* PFB is ptr to a func that returns BYTE */
typedef VOID (*PFV)();		/* PFV is ptr to a func that returns VOID */
typedef int (*PFI)();		/* PFI is ptr to a func that returns int */


typedef UBYTE AUB[1];		/* AUB is an array of UBYTES */
typedef WORD *PW;		/* PW is ptr to WORD */
typedef BYTE *PB;		/* PB is ptr to BYTE */
typedef UBYTE *PUB;		/* PUB is ptr to UBYTE */
typedef UWORD *PUW;		/* PUW is ptr to UWORD */
typedef LONG *PL;		/* PL is ptr to LONG */
typedef ULONG *PUL;		/* PUL is ptr to ULONG */



