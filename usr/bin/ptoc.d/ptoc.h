
/*	Modification History: ptoc.h
 *
 * 21-July-87 afd
 *	Added type STRINGTY to base types for char strings
 *	Added st_string to const class for string constant decls.
 *
 */

#define INDENT 4
#define SYMBOLMAX 12
#define MAXLEV 10
#define NRESERVES 72
#define LINELENGTH 133
#define WORDLENGTH 20
#define EXPRLEN 500
#define MAXINT 32767

#define SINGLESTMT 0
#define FUNCLEVEL 1
#define BEGINLEVEL 2

#define WITHPTR 1
#define WITHREC 2
#define WITHNEST 5

typedef char symbol[SYMBOLMAX+1];
typedef char line[LINELENGTH+1];

/*
 * Declaration Classes, Data structures, & Base Types
 */
enum classes {INCLUDEC, CONSTC, TYPEC, VARC, FIELDC, DEFINEC,
		PROCC, FUNCC, PROGC, COMTC, NOCLASS};
enum dstructs {ARRS, UDEFS, SUBRANGES, RECORDS, SETS, PTRS, FILESTR, NOSTRUCT};
enum types {BOOLTY, CHARTY, STRINGTY, INTTY, OCTALTY, HEXTY, REALTY,
	    UNSIGNEDTY, DOUBLETY, ENUMTY, UNIONTY, USERTYPE, NOTYPE};

/*
 * Tree node types, and block types
 */

enum node_types {ASSIGNNODE, BEGINNODE, CASENODE, COMMENTNODE,
     EXPRESSIONNODE, FORNODE, FDECLNODE, FCALLNODE, GOTONODE, IFNODE,
     LABELNODE, LISTHEADNODE, PDECLNODE, PCALLNODE, PROGNODE, REPEATNODE,
     UNTILNODE, WHILENODE, WITHNODE, WRITENODE, WRITELNNODE, READNODE,
     READLNNODE, SEMINODE};
    
enum block_types {SEMIBLOCK, THENBLOCK, ELSEBLOCK, SUBRBLOCK, NOBLOCK,
     PARAMBLOCK, DECLBLOCK, WITHBLOCK};

/*
 * Comment structure
 */
struct cmtinfo {
    char *cmt;			/* one line of the comment */
    struct cmtinfo *next;	/* ptr to next comment struct (next line) */
    };

/*
 * Scanner record
 */
struct scaninfo {
    char si_name[LINELENGTH];	/* reserved words,idents,literals;chars in [0]*/
    enum reltype {eqrel, nerel, gtrel, gerel, ltrel, lerel}
    	 si_rel;		/* type of relation found */
    int si_idlen;		/* # of char's in ident */
    char si_dflag;		/* 1 = int const;  2 = float const */
    float si_cvalue;		/* numeric const */
    struct cmtinfo *si_cmtptr;	/* ptr to cmtinfo struct */
    };

/*
 * For storing array bounds & subranges
 */
struct pairs {
    struct pairs *pr_next;
    int pr_lower, pr_upper;	/* All types stored as int, converted to
    				   other types upon retrieval */
    struct stentry *pr_luser,	/* ptr to lower user const/subrange/enum */
		   *pr_uuser;	/* prt to upper, if needed */
    enum types pr_bound;	/* type of the bounds (esp. for arrays) */
    };

/*
 * A symbol table entry
 */

struct stentry {
    struct stentry *st_link;		/* ptr to next stentry */
    char *st_name;			/* the symbol name */
    int st_lexlev;			/* lexic level */
    enum dstructs st_dstruct;		/* the data structure */
    enum types st_tipe;			/* the base type */
    enum classes st_class;		/* the symbol class */
    struct stentry *st_uptr;		/* user defined name */
    struct cmtinfo *st_cmt;		/* ptr to decl comment */
    char st_emit;			/* == 0 if decl came from include file */
    char st_funcpar;			/* == 1 if param is a func name */
    					/* == 2 if param is a proc name */
    union {				/* Union is based on the class */
	struct {			/* CONST class */
	    int St_cval;		/* BOOL=0/1, CHAR/INT = actual value */
	    float St_fcval;		/* real/float const */
	    char *St_string;		/* string constant */
	    struct stentry *St_enumptr;	/* ptr from const name back to enum id */
	    }constant;
	struct {			/* TYPE, VAR, FIELD class */
	    int St_numdims;
	    /* St_lower[5], St_upper[5]; */
	    struct pairs *St_bounds;	/* array & subrange bounds */
	    char St_byref;		/* true indicates by-ref param */
	    struct stentry *St_next;	/* ptr to next field of record */
	    struct stentry *St_dupvar;	/* ptr to duplicate var (v1,v2: type) */
	    char St_funcvar;		/* true if dummy for func value */
	    char *St_value;		/* used for var's init value */
	    }tvf;
	struct {			/* PROC, FUNC, PROG class */
	    int St_nparams;
	    struct stentry *St_fparam, *St_lparam;/* ptr to first & last param */
	    }func;
	} st_un;
    };
/*
 * defines for the union
 */
#define st_cval st_un.constant.St_cval
#define st_fcval st_un.constant.St_fcval
#define st_string st_un.constant.St_string
#define st_enumptr st_un.constant.St_enumptr
#define st_numdims st_un.tvf.St_numdims
#define st_bounds st_un.tvf.St_bounds
#define st_byref st_un.tvf.St_byref
#define st_next st_un.tvf.St_next
#define st_dupvar st_un.tvf.St_dupvar
#define st_funcvar st_un.tvf.St_funcvar
#define st_value st_un.tvf.St_value
#define st_nparams st_un.func.St_nparams
#define st_fparam st_un.func.St_fparam
#define st_lparam st_un.func.St_lparam

struct treenode {
    struct stentry *firstlocal;
    struct treenode *parent;
    struct treenode *prev;
    struct treenode *next;
    struct treenode *firstc;
    struct treenode *lastc;
    struct stentry *stdecl;		/* ptr to st entry */
    char *expression;			/* used by most */
    enum node_types type;

    union {
	struct {
	    char *Storewhere, *Storewhat;
	    } assign;
	struct {
	    char *Variable, *Initvalue, *Finalvalue, To;
	    } afor;
	struct {
	    enum block_types Blktype;
	    struct stentry *Firstsym;	/* first stentry under this block */
	    struct stentry *Lastsym;	/* last stentry underck */
	    } begin;
	struct {
	    struct stentry *Ftype;	/* Function type */
	    } func;
	struct {
	    struct cmtinfo *Blkcmt;	/* Block comment */
	    } comment;
	} tree_un;
    };
/*
 * defines for the union
 */
#define storewhere tree_un.assign.Storewhere
#define storewhat tree_un.assign.Storewhat
#define variable tree_un.afor.Variable
#define initvalue tree_un.afor.Initvalue
#define finalvalue tree_un.afor.Finalvalue
#define to tree_un.afor.To
#define blktype tree_un.begin.Blktype
#define firstsym tree_un.begin.Firstsym
#define lastsym tree_un.begin.Lastsym
#define ftype tree_un.func.Ftype
#define blkcmt tree_un.comment.Blkcmt


struct fwdstmt
    {
    struct fwdstmt *next;
    struct treenode *tree;
    };

/*
 * Pascal Tokens
 */

enum token {
    ILLEGALTOKE	,  ANDT		,  ARRAYT	,  ASSIGNOP	,
    BEGINT,
    BOOLEANT	,  CASET	,
    CHART	,  COMMA	,  COMMENT	,  CONSTT	,
    CHARCONST	,  COLON 	,  DIVT		,  DOT		,
    DOTDOT	,  DOUBLE	,  DOWNTOT	,
    ELSET	,  ENDT		,  EXTERNALT	,
    FALSET	,  FORT		,
    LOOPDO	,  FORWARDT	,  FUNCTIONT	,  FILET	,
    GOTOT	,  IDENT	,  IFT		,  IN		,
    INCLUDET	,  INTEGERT	,  INTDIVT	,  LABELT	,
    LEFTPAREN	,  LEFTBRACKET	,  MECHT	,
    MINUS	,  MODT		,  MODULET	,
    MULT	,  NOTT		,  NUMCONST	,  OFT		,
    ORT		,  OTHERWISE	,  PACKED	,  PERCENT	,
    PLUS	,  POUND	,  PROCEDURET	,  PROGRAMT	,
    QUOTE	,  READT	,  READLNT	,  REALT	,
    RECORDT	,  RELATIONAL	,  REPEATT	,
    RIGHTPAREN	,  RIGHTBRACKET	,  SEMICOLON	,  SETT		,  
    THENT	,  TRUET	,  TOT		,  TYPET	,
    UNSIGNT	,
    UNTILT	,  UPARROW	,  VART		,  VARYING	,
    WHILET	,  WITHT	,  WRITET	,  WRITELNT	,  
    };
