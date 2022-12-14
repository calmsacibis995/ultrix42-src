/*static char sccsid[] = "@(#)as.h	4.3 ULTRIX 9/4/90";*/

/************************************************************************
 *									*
 *			Copyright (c) 1985 by				*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any  other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   This software is  derived  from  software  received  from  the	*
 *   University    of   California,   Berkeley,   and   from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is  subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/************************************************************************
 *									*
 *			Modification History				*
 *
 * 003  Tanya Klinchina, 20-Nov-1989
 *      Added support for vector instructions.
 *
 * 002	David L Ballenger, 24-Feb-1986					*
 *	Add definitions for module and label alignment.			*
 *									*
 *	001 - Added new stab-type FORWARDSTAB, for those stabs		*
 *	      that address forward references.				*
 *	      (Victoria Holt, May 7, 1985)				*
 *									*
 ************************************************************************/

/*
 *	Copyright (c) 1982 Regents of the University of California
 *	@(#)as.h 4.19 8/11/83
 */
#define	reg	register

#include <sys/types.h>
#include <a.out.h>
#include <stab.h>

#define readonly
#define	NINST		300

#define	NEXP		20	/* max number of expr. terms per instruction */
#define	NARG		6	/* max number of args per instruction */
#define	NHASH		1103	/* hash table is dynamically extended */
#define	TNAMESIZE	32	/* maximum length of temporary file names */
#define	NLOC		4	/* number of location ctrs */
/*
 *	Sizes for character buffers.
 *	what			size #define name	comments
 *
 *	name assembly		NCPName	
 *	name save		STRPOOLDALLOP	
 *
 *	-name saving is a simple first fit
 */
#ifndef STRPOOLDALLOP
#	define STRPOOLDALLOP	8192
#endif /* not STRPOOLDALLOP */

#define	NCPName	NCPS
#ifndef NCPS
#	undef	NCPName
#	define	NCPName	4096
#endif /* not NCPS */
/*
 *	Check sizes, and compiler error if sizes botch
 */
#if STRPOOLDALLOP < NCPName
	$$$botch with definition sizes
#endif /* test botches */
/*
 * Symbol types
 */
#define	XUNDEF	0x0
#define	XABS	0x2
#define	XTEXT	0x4
#define	XDATA	0x6
#define	XBSS	0x8

#define	XXTRN	0x1
#define	XTYPE	0x1E

#define	XFORW	0x20	/* Was forward-referenced when undefined */

#define	ERR	(-1)
#define	NBPW	32	/* Bits per word */

#define	AMASK	017

/*
 * Actual argument syntax types
 */
#define ACTRL   0       /* vector ctrl word */
#define	AREG	1	/* %r */
#define	ABASE	2	/* (%r) */
#define	ADECR	3	/* -(%r) */
#define	AINCR	4	/* (%r)+ */
#define	ADISP	5	/* expr(%r) */
#define	AEXP	6	/* expr */
#define	AIMM	7	/* $ expr */
#define	ASTAR	8	/* * */
#define	AINDX	16	/* [%r] */
#define AVECT   32      /* vector instruction argument */
#define AVREG   33      /* vector reg %v */
#define AVIMM   39      /* vector $ expr */
/*
 *	Definitions for the things found in ``instrs''
 */
#define	INSTTAB 1
#include "instrs.h"

/*
 *	Tells outrel what it is relocating
 *	RELOC_PCREL is an implicit argument to outrel; it is or'ed in
 *	with a TYPX
 */
#define	RELOC_PCREL	(1<<TYPLG)
/*
 *	reference types for loader
 */
#define	PCREL	1
#define	LEN1	2
#define	LEN2	4
#define	LEN4	6
#define	LEN8	8
#define	LEN16	10

extern	int	reflen[];	/* {LEN*+PCREL} ==> number of bytes */
extern	int	lgreflen[];	/* {LEN*+PCREL} ==> lg number of bytes */
extern	int	len124[];	/* {1,2,4,8,16} ==> {LEN1, LEN2, LEN4, LEN8} */
extern	char	mod124[];	/* {1,2,4,8,16} ==> {bits to construct operands */
extern	int	type_124[];	/* {1,2,4,8,16} ==> {TYPB,TYPW,TYPL,TYPQ,TYPO} */
extern	int	ty_NORELOC[];	/* {TYPB..TYPH} ==> {1 if relocation not OK */
extern	int	ty_float[];	/* {TYPB..TYPH} ==> {1 if floating number */
extern	int	ty_LEN[];	/* {TYPB..TYPH} ==> {LEN1..LEN16} */
extern	int	ty_nbyte[];	/* {TYPB..TYPH} ==> {1,2,4,8,16} */
extern	int	ty_nlg[];	/* {TYPB..TYPH} ==> lg{1,2,4,8,16} */
extern	char	*ty_string[];	/* {TYPB..TYPH} ==> printable */

#define	TMPC	7	
#define	HW	0x1
#define	FW	0x3
#define	DW	0x7
#define	OW	0xF

#define	round(x,y)	(((x)+(y)) & ~(y))

#define	STABTYPS	0340
#define	STABFLAG	0200

/*
 *	Follows are the definitions for the symbol table tags, which are
 *	all unsigned characters..
 *	High value tags are generated by the asembler for internal
 *	use.
 *	Low valued tags are the parser coded tokens the scanner returns.
 *	There are several pertinant bounds in this ordering:
 *		a)	Symbols greater than JXQUESTIONABLE
 *			are used by the jxxx bumper, indicating that
 *			the symbol table entry is a jxxx entry
 *			that has yet to be bumped.
 *		b)	Symbols greater than IGNOREBOUND are not
 *			bequeathed to the loader; they are truly
 *			for assembler internal use only.
 *		c)	Symbols greater than OKTOBUMP represent
 *			indices into the program text that should
 *			be changed in preceeding jumps or aligns
 *			must get turned into their long form.
 */

#define	TAGMASK		0xFF

#	define	JXACTIVE	0xFF	/*jxxx size unknown*/
#	define	JXNOTYET	0xFE	/*jxxx size known, but not yet expanded*/
#	define	JXALIGN		0xFD	/*align jxxx entry*/
#	define	JXINACTIVE	0xFC	/*jxxx size known and expanded*/

#define	JXQUESTIONABLE		0xFB

#	define	JXTUNNEL	0xFA	/*jxxx that jumps to another*/
#	define	OBSOLETE	0xF9	/*erroneously entered symbol*/

#define	IGNOREBOUND	0xF8		/*symbols greater than this are ignored*/
#	define	FLOATINGSTAB	0xF7
#	define	LABELID		0xF6

#define	OKTOBUMP	0xF5
#	define  FORWARDSTAB	0xF3
#	define	FIXEDSTAB	0xF4

/* Definitions pertaining to alignment of modules and labels.  These are
 * expressed as powers of 2.
 *
 * MODULE_ALIGNMENT	This is the maximum alignment factor guaranteed by
 *			ld(1) for a text, data, or bss segment from a module.
 *			This header file is included by ld.c to get this
 *			value.
 *
 * LABEL_ALIGNMENT	This is the alignment guaranteed by as(1) for labels
 *			following jbr instructions or jxxx instructions which
 *			are converted to a bxxx followed by a brw.  See
 *			asjxxx.c for a description.
 *
 * ALIGN_GUARANTEE	The maximumn value for an alignment expression on a
 *			.align statement that as(1) will accept without
 *			issuing a warning message.  This is dependent on
 *			the maximum module alignment guaranteed by ld(1).
 *
 * MAX_ALIGN_EXPR	The maximum value that as will except for an 
 *			alignment expression without issuing an error.
 *
 * LEGAL_ALIGN_VAL(n)	Macro to see if 'n' is in the legal range for an
 *			alignment expression.
 */
#define MODULE_ALIGNMENT	(3)	/* quadword alignment */
#define LABEL_ALIGNMENT		(2)	/* longword alignment */
#define ALIGN_GUARANTEE		MODULE_ALIGNMENT
#define MAX_ALIGN_EXPR		(16)
#define LEGAL_ALIGN_VAL(n)	(((n) >= 0) && ((n) <= MAX_ALIGN_EXPR))

/*
 *	astoks.h contains reserved word codings the parser should
 *	know about
 */
#include "astoks.h"

/*
 *	The structure for one symbol table entry.
 *	Symbol table entries are used for both user defined symbols,
 *	and symbol slots generated to create the jxxx jump from
 *	slots.
 *	Caution: the instructions are stored in a shorter version
 *	of the struct symtab, using all fields in sym_nm and
 *	tag.  The fields used in sym_nm are carefully redeclared
 *	in struct Instab and struct instab (see below).
 *	If struct nlist gets changed, then Instab and instab may
 *	have to be changed.
 */

struct symtab{
		struct	nlist	s_nm;
		u_char	s_tag;		/* assembler tag */
		u_char	s_ptype;	/* if tag == NAME */
		u_char	s_jxneedalign;	/* if a jbr/jxxx needs alignment */
		short	s_index;	/* which segment */
		struct	symtab *s_dest;	/* if JXXX, where going to */
#ifdef DJXXX
		short	s_jxline;	/* source line of the jump from */
#endif
};
/*
 *	Redefinitions of the fields in symtab for
 *	use when the symbol table entry marks a jxxx instruction.
 */
#define	s_jxbump	s_ptype		/* tag == JX..., how far to expand */
#define	s_jxfear	s_desc		/* how far needs to be bumped */
/*
 *	Redefinitions of fields in the struct nlist for symbols so that
 *	one saves typing, and so that they conform 
 *	with the old naming conventions.
 */
#define	s_name	s_nm.n_un.n_name
#define	i_name	s_name
#define	FETCHNAME(sp)	(((struct strdesc *)(sp)->s_name)->sd_string)
#define	STRLEN(sp)	(((struct strdesc *)(sp)->s_name)->sd_strlen)
#define	STROFF(sp)	(((struct strdesc *)(sp)->s_name)->sd_stroff)
#define	STRPLACE(sp)	(((struct strdesc *)(sp)->s_name)->sd_place)
#define	s_nmx	s_nm.n_un.n_strx	/* string table index */
#define	s_type	s_nm.n_type		/* type of the symbol */
#define	s_other	s_nm.n_other		/* other information for sdb */
#define	s_desc	s_nm.n_desc		/* type descriptor */
#define	s_value	s_nm.n_value		/* value of the symbol, or sdb delta */

struct	instab{
	struct	nlist	s_nm;		/* instruction name, type (opcode) */
	u_char	s_tag;			
	u_char	s_eopcode;
	char    s_format;               /* vector instruction format */
	u_char	s_ctrlcode;    		/* vector instruction CMP/CVT code */
};
typedef	struct	instab	*Iptr;
/*
 *	The fields nm.n_desc and nm.n_value total 6 bytes; this is
 *	just enough for the 6 bytes describing the argument types.
 *	We use a macro to define access to these 6 bytes, assuming that
 *	they are allocated adjacently.
 *	IF THE FORMAT OF STRUCT nlist CHANGES, THESE MAY HAVE TO BE CHANGED.
 *
 *	Instab is cleverly declared to look very much like the combination of
 *	a struct symtab and a struct nlist.
 */
/*
 *	With the 1981 VAX architecture reference manual,
 *	DEC defined and named two byte opcodes. 
 *	In addition, DEC defined four new one byte instructions for
 *	queue manipulation.
 *	The assembler was patched in 1982 to reflect this change.
 *
 *	The two byte opcodes are preceded with an escape byte
 *	(usually an ESCD) and an opcode byte.
 *	For one byte opcodes, the opcode is called the primary opcode.
 *	For two byte opcodes, the second opcode is called the primary opcode.
 *
 *	We store the primary opcode in I_popcode,
 *	and the escape opcode in I_eopcode.
 *
 *	For one byte opcodes in the basic arhitecture,
 *		I_eopcode is CORE
 *	For one byte opcodes in the new architecture definition,
 *		I_eopcode is NEW
 *	For the two byte opcodes, I_eopcode is the escape byte.
 *
 *	The assembler checks if a NEW or two byte opcode is used,
 *	and issues a warning diagnostic.
 */
/*
 *	For upward compatability reasons, we can't have the two opcodes
 *	forming an operator specifier byte(s) be physically adjacent
 *	in the instruction table.
 *	We define a structure and a constructor that is used in
 *	the instruction generator.
 */
struct Opcode{
	u_char	Op_eopcode;
	u_char	Op_popcode;
};

#define	BADPOINT	0xAAAAAAAA
/*
 *	See if a structured opcode is bad
 */
#define	ITABCHECK(o)	((itab[o.Op_eopcode] != (Iptr*)BADPOINT) && (itab[o.Op_eopcode][o.Op_popcode] != (Iptr)BADPOINT))
/*
 *	Index the itab by a structured opcode
 */
#define	ITABFETCH(o)	itab[o.Op_eopcode][o.Op_popcode]

struct	Instab{
	char	*I_name;
	u_char	I_popcode;		/* basic op code */
	char	I_nargs;
	char	I_args[6];
	u_char	I_s_tag;
	u_char	I_eopcode;
	char    I_format;               /* vector instruction format number */
	u_char	I_ctrlcode;    		/* vector instruction CMP/CVT code */
};
/*
 *	Redefinitions of fields in the struct nlist for instructions so that
 *	one saves typing, and conforms to the old naming conventions
 */
#define	i_popcode	s_nm.n_type	/* use the same field as symtab.type */
#define	i_eopcode	s_eopcode
#define	i_ctrlcode	s_ctrlcode
#define	i_nargs		s_nm.n_other	/* number of arguments */
#define	fetcharg(ptr, n) ((struct Instab *)ptr)->I_args[n]

struct	arg {				/*one argument to an instruction*/
	char	a_atype;
	char	a_areg1;
	char	a_areg2;
	char	a_dispsize;		/*usually d124, unless have B^, etc*/
	struct	exp *a_xp;
};
/*
 *	Definitions for numbers and expressions.
 */
#include "asnumber.h"
struct	exp {
	Bignum	e_number;	/* 128 bits of #, plus tag */
	char	e_xtype;
	char	e_xloc;
	struct	symtab		*e_xname;
};
#define	e_xvalue	e_number.num_num.numIl_int.Il_long
#define	e_xwvalue	e_number.num_num.numIl_int.Iw_short

#define		MINLIT		0
#define		MAXLIT		63

#define		MINBYTE		-128
#define		MAXBYTE		127
#define		MINUBYTE	0
#define		MAXUBYTE	255

#define		MINWORD		-32768
#define		MAXWORD		32767
#define		MINUWORD	0
#define		MAXUWORD	65535

#define		ISLIT(x)	(((x) >= MINLIT) && ((x) <= MAXLIT))
#define		ISBYTE(x)	(((x) >= MINBYTE) && ((x) <= MAXBYTE))
#define		ISUBYTE(x)	(((x) >= MINUBYTE) && ((x) <= MAXUBYTE))
#define		ISWORD(x)	(((x) >= MINWORD) && ((x) <= MAXWORD))
#define		ISUWORD(x)	(((x) >= MINUWORD) && ((x) <= MAXUWORD))
/*
 *	Definitions for strings.
 *
 *	Strings are stored in the string pool; see strsave(str, length)
 *	Strings are known by their length and values.
 *	A string pointer points to the beginning of the value bytes;
 *
 *	If this structure is changed, change insts also.
 */
struct	strdesc{
	int	sd_stroff;	/* offset into string file */
	short	sd_place;	/* where string is */
	u_short	sd_strlen;	/* string length */
	char	sd_string[1];	/* the string itself, flexible length */
};
/*
 *	Where a string can be.  If these are changed, also change instrs.
 */
#define	STR_FILE	0x1
#define	STR_CORE	0x2
#define	STR_BOTH	0x3

struct strdesc *savestr();

/*
 *	Global variables
 */
	extern	struct	arg	arglist[NARG];	/*building operands in instructions*/
	extern	struct	arg	varglist[NARG];	/*building operands in vector instructions*/
	extern	struct	exp	explist[NEXP];	/*building up a list of expressions*/
	extern	struct	exp	*xp;		/*current free expression*/
	/*
	 *	Communication between the scanner and the jxxx handlers.
	 *	lastnam:	the last name seen on the input
	 *	lastjxxx:	pointer to the last symbol table entry for
	 *			a jump from
	 */
	extern	struct	symtab	*lastnam;
	extern	struct	symtab	*lastjxxx;	
	/*
	 *	Lgensym is used to make up funny names for local labels.
	 *	lgensym[i] is the current funny number to put after
	 *	references to if, lgensym[i]-1 is for ib.
	 *	genref[i] is set when the label is referenced before
	 *	it is defined (i.e. 2f) so that we can be sure these
	 *	labels are always defined to avoid weird diagnostics
	 *	from the loader later.
	 */
	extern	int	lgensym[10];
	extern	char	genref[10];

	extern	struct	exp	*dotp;		/* the current dot location */
	extern	int	loctr;

	extern	struct	exec	hdr;		/* a.out header */
	extern	u_long	tsize;			/* total text size */
	extern	u_long	dsize;			/* total data size */
	extern	u_long	trsize;			/* total text relocation size */
	extern	u_long	drsize;			/* total data relocation size */
	extern	u_long	datbase;		/* base of the data segment */
	/*
	 *	Bitoff and bitfield keep track of the packing into 
	 *	bytes mandated by the expression syntax <expr> ':' <expr>
	 */
	extern	int	bitoff;	
	extern	long	bitfield;
	
	/*
	 *	The lexical analyzer builds up symbols in yytext.  Lookup
	 *	expects its argument in this buffer
	 */
	extern	char	yytext[NCPName+2];	/* text buffer for lexical */
	/*
	 *	Variables to manage the input assembler source file
	 */
	extern	int	lineno;			/*the line number*/
	extern	char	*dotsname;		/*the name of the as source*/

	extern	FILE	*tokfile;		/* temp token communication*/
	extern	FILE	*strfile;		/* temp string file*/
	extern	char	tokfilename[TNAMESIZE];	/* token file name */
	extern	char	strfilename[TNAMESIZE];	/* string file name */
	extern	int	strfilepos;		/* position in string file */

	extern	int	passno;			/* 1 or 2 */

	extern	int	anyerrs;		/*errors as'ing arguments*/
	extern	int	anywarnings;		/*warnings as'ing arguments*/
	extern	int	silent;			/*don't mention the errors*/
	extern	int	savelabels;		/*save labels in a.out*/
	extern	int	orgwarn;		/* questionable origin ? */
	extern	int	useVM;			/*use virtual memory temp file*/
	extern	int	jxxxJUMP;		/*use jmp instead of brw for jxxx */
	extern	int	readonlydata;		/*initialized data into text space*/
	extern	int	nGHnumbers;		/* GH numbers used */
	extern	int	nGHopcodes;		/* GH opcodes used */
	extern	int	nnewopcodes;		/* new opcodes used */
#ifdef DEBUG
	extern	int	debug;
	extern	int	toktrace;
#endif
	/*
	 *	Information about the instructions
	 */
	extern	struct	instab	**itab[NINST];	/*maps opcodes to instructions*/
	extern  readonly struct Instab instab[];

	extern	int	curlen;			/*current literal storage size*/
	extern	int	d124;			/*current pointer storage size*/
	
	struct	symtab	**lookup();		/*argument in yytext*/
	struct 	symtab	*symalloc();

	char	*Calloc();
	char	*ClearCalloc();

#define outb(val) {dotp->e_xvalue++; if (passno==2) bputc((val), (txtfil));}

#define outs(cp, lg) dotp->e_xvalue += (lg); if (passno == 2) bwrite((cp), (lg), (txtfil))

#define	Outb(o)	outb(o)
/*
 *	Most of the time, the argument to flushfield is a power of two constant,
 *	the calculations involving it can be optimized to shifts.
 */
#define flushfield(n) if (bitoff != 0)  Flushfield( ( (bitoff+n-1) /n ) * n)

/*
 * The biobuf structure and associated routines are used to write
 * into one file at several places concurrently.  Calling bopen
 * with a biobuf structure sets it up to write ``biofd'' starting
 * at the specified offset.  You can then use ``bwrite'' and/or ``bputc''
 * to stuff characters in the stream, much like ``fwrite'' and ``fputc''.
 * Calling bflush drains all the buffers and MUST be done before exit.
 */
struct	biobuf {
	short	b_nleft;		/* Number free spaces left in b_buf */
/* Initialize to be less than BUFSIZ initially, to boundary align in file */
	char	*b_ptr;			/* Next place to stuff characters */
	char	*b_buf;			/* Pointer to the buffer */
	off_t	b_off;			/* Current file offset */
	struct	biobuf *b_link;		/* Link in chain for bflush() */
};
#define	bputc(c,b) ((b)->b_nleft ? (--(b)->b_nleft, *(b)->b_ptr++ = (c)) \
		       : bflushc(b, c))
#define BFILE	struct biobuf

	extern	BFILE	*biobufs;	/* head of the block I/O buffer chain */
	extern	int	biofd;		/* file descriptor for block I/O file */
	extern	off_t	boffset;	/* physical position in logical file */
	extern	int	biobufsize;	/* optimal block size for I/O */
#define min(a,b) ((a) < (b) ? (a):(b))
#define movestr(to,from,len)	{register char *too = to, *fromm = from;\
				 register int length = len;\
				 if (length>0) bcopy(from,to,length);}

	/*
	 *	For each of the named .text .data segments
	 *	(introduced by .text <expr>), we maintain
	 *	the current value of the dot, and the BFILE where
	 *	the information for each of the segments is placed
	 *	during the second pass.
	 */
	extern	struct	exp	usedot[NLOC + NLOC];
	extern		BFILE	*usefile[NLOC + NLOC];
	extern		BFILE	*txtfil;/* file for text and data: into usefile */
	/*
	 *	Relocation information for each segment is accumulated
	 *	seperately from the others.  Writing the relocation
	 *	information is logically viewed as writing to one
	 *	relocation saving file for  each segment; physically
	 *	we have a bunch of buffers allocated internally that
	 *	contain the relocation information.
	 */
	struct	relbufdesc	*rusefile[NLOC + NLOC];
	struct	relbufdesc	*relfil;

struct Ctrl_word{
  unsigned vc_bits:4;             /* bits 0-3 */
  unsigned vb_bits:4;             /* bits 4-7 */
  unsigned va_bits:4;             /* bits 8-11 */
  unsigned unused_bit:1;          /* bit  12 */
  unsigned mod_bits:3  ;          /* bit  13-15 */
};

struct Vinst_arg {
  int               va_tag;       /* VCTRL, VOPND, VLTRL */
  char              va_args[3];   /* if va_tag = VCTRL: va,vb,vc */
                                  /* if va_tag = VOPND: src opnd # */
};

struct Vinst_fmt {
  u_short           vi_mod_mask;  /* modifier mask */
  u_short           vi_mod_dflt;  /* default modifiers */
  char              vi_nargs;     /* number of vector inst operands */
  struct Vinst_arg  vi_args[6];   /* up to 6 arguments per vinst */
};

union Vinst_mod {
  struct {
    short   mod_length;
    char    mod_ch[2];
  }mod_value;
  int     imod;
};

/* Definitions pertaining to vector instruction format.  */
#define         VCTRL 1
#define         VOPND 2
#define         VLTRL 3

#define         VARG1 1
#define         VARG2 (VARG1 + 1)
#define         VARG3 (VARG2 + 1)
#define         VARG4 (VARG3 + 1)
#define         VARG5 (VARG4 + 1)
#define         VARG6 (VARG5 + 1)
#define         VCODE (VARG6 + 1)

#define         VA    0
#define         VB    1
#define         VC    2

#define         mi_bit    (1)
#define         exc_bit   (1)
#define         mtf_bit   (1<<1)
#define         moe_bit   (1<<2)

#define         MOE 0x01
#define         MTF 0x02
#define         MI  0x04
#define         EXC 0x08
