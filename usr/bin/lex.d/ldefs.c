/*	ldefs.c	4.1	83/08/11	*/

# include <stdio.h>
# include <ctype.h>
# define PP 1

# ifdef ultrix
# define CWIDTH 8
# define CMASK  0377
# define ASCII  1
# define NCH    256
# define alpha  isalpha
# define printable isprint
# else

# ifdef unix
#  define CWIDTH 7
#  define CMASK 0177
#  define ASCII 1
#  define NCH   128
#  endif
#endif

# ifdef gcos
# define CWIDTH 9
# define CMASK 0777
# define ASCII 1
# define NCH   128
# endif

# ifdef ibm
# define CWIDTH 8
# define CMASK 0377
# define EBCDIC 1
# define NCH    256
# endif


# define TOKENSIZE 1000
# define DEFSIZE 40
# define DEFCHAR 1000
# define STARTCHAR 100
# define STARTSIZE 256
# define CCLSIZE 1000
# ifdef SMALL
# define TREESIZE 600
# define NTRANS 1500
# define NSTATES 300
# define MAXPOS 1500
# define NOUTPUT 1628    /* was 1500, need to increase for 8 bit clean */
# endif

# ifndef SMALL
# define TREESIZE 1000
# define NSTATES 500
# define MAXPOS 2500
# define NTRANS 2000
# define NOUTPUT 3128   /* was 3000, need to increase for 8 bit clean */
# endif
# define NACTIONS 100
# define ALITTLEEXTRA 30

# define RCCL NCH+90
# define RNCCL NCH+91
# define RSTR NCH+92
# define RSCON NCH+93
# define RNEWE NCH+94
# define FINAL NCH+95
# define RNULLS NCH+96
# define RCAT NCH+97
# define STAR NCH+98
# define PLUS NCH+99
# define QUEST NCH+100
# define DIV NCH+101
# define BAR NCH+102
# define CARAT NCH+103
# define S1FINAL NCH+104
# define S2FINAL NCH+105

# define DEFSECTION 1
# define RULESECTION 2
# define ENDSECTION 5
# define TRUE 1
# define FALSE 0

# define PC 1
# define PS 1

# ifdef DEBUG
# define LINESIZE 110
extern int yydebug;
extern int debug;		/* 1 = on */
extern int charc;
# endif

# ifndef DEBUG
# define freturn(s) s
# endif

extern int sargc;
extern char **sargv;
extern unsigned char buf[520];
extern int ratfor;		/* 1 = ratfor, 0 = C */
extern int yyline;		/* line number of file */
extern int sect;
extern int eof;
extern int lgatflg;
extern int divflg;
extern int funcflag;
extern int pflag;
extern int casecount;
extern int chset;	/* 1 = char set modified */
extern FILE *fin, *fout, *fother, *errorf;
extern int fptr;
extern unsigned char *ratname, *cname;
extern int prev;	/* previous input character */
extern int pres;	/* present input character */
extern int peek;	/* next input character */
extern int *name;
extern int *left;
extern int *right;
extern int *parent;
extern unsigned char *nullstr;
extern int tptr;
extern unsigned char pushc[TOKENSIZE];
extern unsigned char *pushptr;
extern unsigned char slist[STARTSIZE];
extern unsigned char *slptr;
extern unsigned char **def, **subs, *dchar;
extern unsigned char **sname, *schar;
extern unsigned char *ccl;
extern unsigned char *ccptr;
extern unsigned char *dp, *sp;
extern int dptr, sptr;
extern unsigned char *bptr;		/* store input position */
extern unsigned char *tmpstat;
extern int count;
extern int **foll;
extern int *nxtpos;
extern int *positions;
extern int *gotof;
extern int *nexts;
extern unsigned char *nchar;
extern int **state;
extern int *sfall;		/* fallback state num */
extern unsigned char *cpackflg;		/* true if state has been character packed */
extern int *atable, aptr;
extern int nptr;
extern unsigned char symbol[NCH];
extern unsigned char cindex[NCH];
extern int xstate;
extern int stnum;
extern int ctable[];
extern int ZCH;
extern int ZCHHALF;  /* need for 8 bit clean */
extern int ccount;
extern unsigned char match[NCH];
extern unsigned char extra[NACTIONS];
extern unsigned char *pcptr, *pchar;
extern int pchlen;
extern int nstates, maxpos;
extern int yytop;
extern int report;
extern int ntrans, treesize, outsize;
extern long rcount;
extern int optim;
extern int *verify, *advance, *stoff;
extern int scon;
extern unsigned char *psave;
extern unsigned char *calloc(), *myalloc();
extern int buserr(), segviol();
