#ifndef lint
static char sccsid[]  =  "@(#)select.c	4.2   (ULTRIX)   9/11/90";
#endif  lint

/* DEC/CMS REPLACEMENT HISTORY, Element SELECT.C*/
/* *4    10-APR-1990 09:40    Arce "es$parse move param for fldno out of string"*/
/* *3     8-AUG-1986 14:18:45 ZAREMBA "make local copy of fmt string"*/
/* *2     9-JUN-1986 09:32:38 ZAREMBA "V1_1 changes"*/
/* *1     6-JUN-1986 10:56:48 ZAREMBA "Select module"*/
/* DEC/CMS REPLACEMENT HISTORY, Element SELECT.C*/
/* DEC/CMS REPLACEMENT HISTORY, Element SELECT.C*/
/* *8    16-MAY-1986 14:13:26 ZAREMBA "removed dump routines"*/
/* *7     7-APR-1986 16:15:20 ZAREMBA "add comments to ES$NOT"*/
/* *6     7-APR-1986 16:04:20 ZAREMBA "added else"*/
/* *5    27-MAR-1986 10:42:27 ZAREMBA "Ultrix UERF bug fixes"*/
/* *4    19-MAR-1986 11:12:30 ZAREMBA "removed genmac.h"*/
/* *3    11-MAR-1986 12:15:10 ZAREMBA "added parsing and evalu functions"*/
/* *2    27-FEB-1986 17:53:18 ZAREMBA "Seperate paramters version"*/
/* *1    26-FEB-1986 10:53:07 ZAREMBA "Selection code"*/
/* DEC/CMS REPLACEMENT HISTORY, Element SELECT.C*/
/*
*	.TITLE	SELECT - ERMS SELECT MODULE
*	.IDENT	/1-001/
*
* COPYRIGHT (C) 1986 DIGITAL EQUIPMENT CORP.,
* CSSE SOFTWARE ENGINEERING
* MARLBOROUGH, MASSACHUSETTS
*
* THIS SOFTWARE IS FURNISHED UNDER A LICENSE FOR USE ONLY ON A 
* SINGLE COMPUTER SYSTEM AND MAY BE COPIED ONLY WITH THE INCLUSION
* OF THE ABOVE COPYRIGHT NOTICE.  THIS SOFTWARE,  OR ANY OTHER
* COPIES THEREOF, MAY NOT BE PROVIDED OR OTHERWISE MADE AVAILABLE
* TO ANY OTHER PERSON EXCEPT FOR USE ON SUCH SYSTEM AND TO ONE WHO
* AGREES TO THESE LICENSE TERMS.  TITLE TO AND OWNERSHIP OF THE
* SOFTWARE SHALL AT ALL TIMES REMAIN IN DEC.
*
* THE INFORMATION IN THIS SOFTWARE IS SUBJECT TO CHANGE WITHOUT
* NOTICE AND SHOULD NOT BE CONSTRUED AS A COMMITMENT BY DIGITAL
* EQUIPMENT CORPORATION.
*
* DEC ASSUMES NO RESPONSIBILITY FOR THE USE OR RELIABILITY OF
* ITS SOFTWARE ON EQUIPMENT WHICH IS NOT SUPPLIED BY DEC.
*
*++
*
* FACILITY:		[ ERMS - EIMS Record Management System ]
*
* ABSTRACT:
*
*	This module is the ERMS call interface to the SELECT function.
*	It can be called directly from a C program or as the
*	call from the pre-prosessor. 
*	
* ENVIRONMENT:	VAX/VMS C,  ULTRIX-32 C
*
* AUTHOR:  Don Zaremba,  CREATION DATE:  19-Nov-86
*
* MODIFIED BY:
*
* PROGRAM TABLE OF CONTENTS
*  Public
*   bstatus = es$eval(tree)
*   node   = es$parse(stat,fldno,selectformat [,argn...])
*   node   = es$mkselectnode(operand,left,right,arguments)
*   args   = es$mkarglist(fldno,fldtype,address)
*   status = es$apparglist(arglist,fldno,fldtype,address)
*
* Private
*   opcode = cvtopc(string)
*   VOID   =  strtoupper(str)
*   strptr = storestring(str)
*   longptr= storelong(long)
*   token  = nexttoken(str,delimiter,rem)
*   str    = trimspaces(str)
*   str    = trimleadingspaces(str)
*   address = cvtparms(ftype,ptype,spar,lpar)
*   VOID   = deletetree(tree)
*   node   = nextinorder(node)
*   node   = firstinorder(tree)
*--
*/

#include <stdio.h>
#include <ctype.h>
#include <varargs.h>
#include "eims.h"
#include "erms.h"
#include "select.h"
#include "generic_dsd.h"
#include "std_dsd.h"

/*
*++
*=
*=
*=MODULE FLOW - select.c
*=
*=  a - es$eval(tree)                               Evaluates selection
                                                    criteria.
*=          switch(tree->operator)
*=              {
*=              case ES$AND:
*=                  es$eval(tree->left)                     (* a)
*=                  return(ES$SUCC | ES$FAIL)
*=              case ES$OR:
*=                  es$eval(tree->left)                     (* a)
*=                  if (ES$SUCC)
*=                      return(ES$SUCC | ES$FAIL)
*=                  es$eval(tree->right)                    (* a)
*=                  return(ES$SUCC | ES$FAIL)
*=              case ES$NOT:
*=                  es$eval(tree->left)                     (* a)
*=                  return(ES$SUCC | ES$FAIL)
*=              case ES$EQUAL:
*=                  return(isequal(tree->operands)          (* d)
*=              case ES$LE | LT | GE | GT:
*=                  return(isinequal(tree->operands,oper)   (* c)
*=              case ES$RANGE:
*=                  return(isrange(tree->operands)          (* b)
*=
*=  b - isrange(args)           
*=          return(ES$SUCC | ES$FAIL)
*=
*=  c - isinequal(args,opcode)
*=          return(ES$SUCC | ES$FAIL)
*=
*=  d - isequal(args)
*=          return(ES$SUCC | ES$FAIL)
*=
*=  e - es$parse(arglist)                           Parse arglist & build node.
*=          find_std_item_dsd(fldno)                        (dsd_access.c)
*=          trimleadingspaces(string)                       (* m)
*=          nexttoken(string)                               (* k)
*=          strtoupper(opcode)                              (* j)
*=          cvtopc(opcode)                                  (* i)
*=          es$mkarglist(fldno,ftype,0)                     (* o)
*=          while(string != NULL)
*=              {
*=              nexttoken(string)                           (* k)
*=              trimspaces(token)                           (* l)
*=              cvtparms(stat,fldno,ftype,ptype,spar,lpar)  (* f)
*=              es$apparglist(args,fldno,ftype,node)        (* p)
*=              }
*=          es$mkselectnode(opcode,NULL,NULL,args)          (* n)
*=          return(node)
*=
*=  f - cvtparms(stat,fldno,ftype,ptype,spar,lpar)  Converts input to EIMS
*=                                                  field type.
*=          switch(ftype)
*=              {
*=              SHORT | LONG | REG | SHORT_REG | DATE:
*=                  if(ptype = STRING)
*=                      storelong(atol(spar))               (* h)
*=                  else
*=                      storelong(lpar)                     (* h)
*=              STRING:
*=                  storestring(spar)                       (* g)
*=              INDEX | SHORT_INDEX:
*=                  if(ptype = STRING)
*=                      get_code_std_item(fldno,spar)       (dsd_access.c)
*=                      storelong(lpar)                     (* h)
*=                  else
*=                      storelong(lpar)                     (* h)
*=              }
*=          return(string)
*=
*=  g - storestring(string)                         Allocates space for string.
*=          return(alloc_ptr)
*=
*=  h - storelong(long)                             Allocates space for long.
*=          return(alloc_ptr)
*=
*=  i - cvtopc(string)                              Converts op code to value.
*=          return(opcode_val)
*=
*=  j - strtoupper(string)                          converts string to up case.
*=          return(str)
*=
*=  k - nexttoken(str,delim,rem)                    Finds next token in string.
*=          return(token)
*=
*=  l - trimspaces(string)                          Changes multi spaces to 
*=                                                  single space.
*=          return(str)
*=
*=  m - trimleadingspaces(string)                   Removes leading spaces.
*=          return(str)
*=
*=  n - es$mkselectnode(opr,lnode,rnode,args)       Creates a selection node.
*=          malloc()
*=          return(node)
*=
*=  o - es$mkarglist(fldno,fldtype,varadr)          Allocates a structure for
*=                                                  1st element of arg_list. 
*=          malloc()
*=          return(str)
*=
*=  p - es$apparglist(args,fldno,fldtype,varadr)    Allocates a structure for
*=                                                  other elements in arg_list
*=                                                  and adds it to end of list.
*=          malloc()
*=          return(str)
*=
*=  q - deletetree(tree)                            Deletes a tree and frees
*=                                                  the space.
*=          if(tree->left)
*=              deletetree(left)                            (* q)
*=          if(tree->right)
*=              deletetree(right)                           (* q)
*=          free(tree)
*=
*=  r - nextinorder(last_node)                      Finds next_node in tree.
*=          firstinorder(last_node->right)                  (* s)
*=          return(node)
*=
*=  s - firstinorder(top_node)                      Finds first_node in order.
*=          return(node)
*=
*
*--
*/

/************************* function declarations *************************/

DD$STD_SEGS_DSD_PTR	find_std_segment_dsd();
DD$STD_ITEMS_DSD_PTR	find_std_item_dsd();
SELNODE			*es$mkselectnode();
SELNODE			*firstinorder();
ARGLIST			*es$mkarglist();
long			es$apparglist();
char			*storestring();
char			*storelong();
char			*nexttoken();
char			*trimspaces();
char			*trimleadingspaces();
char			*cvtparms();
char			*strchr();
char			*malloc();
void			strtoupper();
void			deletetree();

/*
*	.SBTTL	ERMS_EVALUATE - ERMS EVALUATE function
*++
* FUNCTIONAL DESCRIPTION:		
*
*	This function evaluates a selection criteria against a
*	standard record and returns ES$SUCC if the record
*	satisifies the criteria, ES$FAIL if not.
*	These evaluations are done using only the address and
*       datatype fields of an arglist. Because of this fact, this
*       function  can be called from a number of different
*       places. The address can point into a standard segment if
*       called from ERMS, or the address could point into a raw
*       buffer if called from ERIT.
*
* FUNCTION DECLARATION
*	long es$eval();
*	
* CALLING SEQUENCE:	status = es$eval(tree)
*
* FORMAL PARAMETERS:		
*
*	SELNODE *tree;	pointer to select tree
*
* IMPLICIT INPUTS: NONE
*
* IMPLICIT OUTPUTS: NONE
*
* FUNCTION VALUE:		integer value
*
*	ES$SUCC - record satisfies selection criteria
*	ES$FAIL - record does not satisfies selection criteria
*	
* SIDE EFFECTS:	NONE
*	
*--
*/
/*******************************  ES$EVAL  ****************************/

long es$eval(tree)
SELNODE *tree;
{

long	answer=0;
long	ans2;
short   i;
extern  ITEM_INFO        item_info[];

if (tree == NULL)
    return(answer);

if (tree->operands != NULL)
    {
    if (tree->operands->varaddress == NULL)
        {
        i = 0;
        while(item_info[i].id != 0)
            {
            if (tree->operands->fldno == item_info[i].id)
                {
                tree->operands->varaddress = (char *)item_info[i].item_ptr;
                break;
                }
            i++;
            }
        }
    }
switch (tree->operator)
    {
    case ES$AND :
	answer = es$eval(tree->left);
	if (answer == ES$FAIL)
	    return(ES$FAIL);		/* bail out at first fail of an AND */
	ans2 = es$eval(tree->right);
	answer = ((answer == ES$SUCC) && (ans2 == ES$SUCC)) ? ES$SUCC : ES$FAIL;
    break;
    case ES$OR :
	answer = es$eval(tree->left);
	if (answer == ES$SUCC)
	    return(ES$SUCC);		/* bail out at first success of an OR */
	ans2 = es$eval(tree->right);
	answer = ((answer == ES$SUCC) || (ans2 == ES$SUCC)) ? ES$SUCC : ES$FAIL;
    break;
    case ES$NOT:
	answer = es$eval(tree->left);
	answer = (answer == ES$SUCC) ? ES$FAIL : ES$SUCC;
    break;
    case ES$EQUAL:
	answer = isequal(tree->operands);
    break;
    case ES$LE:
    case ES$LT:
    case ES$GE:
    case ES$GT:
	answer = isinequal(tree->operands,tree->operator);
    break;
    case ES$RANGE:
	answer = isrange(tree->operands);
    break;
    }
return(answer);
}

/******************************  ISRANGE  ******************************/

long isrange(args)
ARGLIST *args;
{

long	ans;
char	*varadr;
long	vartype;
ARGLIST *carg;
ARGLIST	*carg2;

carg	= args;				/* point to first argument */
varadr	= args->varaddress;
vartype	= args->vartype;
ans	= ES$FAIL;
carg	= carg->next;
carg2	= carg->next;

switch (vartype)
    {
    case DT_STRING :
	ans = ((strcmp(*(char **)varadr,carg2->varaddress) <= 0) &&
		     (strcmp(*(char **)varadr,carg->varaddress) >= 0))
			    ? ES$SUCC : ES$FAIL;
    break;
    case DT_SHORT :
    case DT_SHORT_INDEX :
	ans = ((*(short*)varadr <= *(short*)carg2->varaddress) &&
		     (*(short*)varadr >= *(short*)carg->varaddress))
			    ? ES$SUCC : ES$FAIL;
    break;
    case DT_LONG :
    case DT_INDEXED :
    case DT_DATE:
	ans = ((*(long*)varadr <= *(long*)carg2->varaddress) &&
		     (*(long*)varadr >= *(long*)carg->varaddress))
			    ? ES$SUCC : ES$FAIL;
    break;
    }
return(ans);
}

/****************************** ISINEQUAL ********************************/

long isinequal(args,opcode)
ARGLIST *args;
short	opcode;
{

long	ans;
char	*varadr;
long	vartype;
ARGLIST	*carg;

carg	= args;		/* point to first argument */
varadr	= args->varaddress;
vartype	= args->vartype;
ans	= ES$FAIL;
carg	= carg->next;

switch (vartype)
    {
    case DT_STRING :
	ans = (strcmp(*(char **)varadr,carg->varaddress) < 0)
			? ES$SUCC : ES$FAIL;
    break;
    case DT_SHORT :
    case DT_SHORT_INDEX :
	switch (opcode)
	    {
	    case ES$LT:
		ans = (*(short*)varadr < *(short*)carg->varaddress) 
			? ES$SUCC : ES$FAIL;
	    break;
	    case ES$LE:
		ans = (*(short*)varadr <= *(short*)carg->varaddress) 
			? ES$SUCC : ES$FAIL;
	    break;
	    case ES$GT:
		ans = (*(short*)varadr > *(short*)carg->varaddress) 
			? ES$SUCC : ES$FAIL;
	    break;
	    case ES$GE:
		ans = (*(short*)varadr >= *(short*)carg->varaddress) 
			? ES$SUCC : ES$FAIL;
	    break;
	    }
    break;
    case DT_LONG :
    case DT_INDEXED:
    case DT_DATE:
	switch (opcode)
	    {
	    case ES$LT:
		ans = (*(long*)varadr < *(long*)carg->varaddress) 
			? ES$SUCC : ES$FAIL;
	    break;
	    case ES$LE:
		ans = (*(long*)varadr <= *(long*)carg->varaddress) 
			? ES$SUCC : ES$FAIL;
	    break;
	    case ES$GT:
		ans = (*(long*)varadr > *(long*)carg->varaddress) 
			? ES$SUCC : ES$FAIL;
	    break;
	    case ES$GE:
		ans = (*(long*)varadr >= *(long*)carg->varaddress) 
			? ES$SUCC : ES$FAIL;
	    break;
	    }			 /* end of opcode case */
    break;
    }
return(ans);
}

/****************************** ISEQUAL ******************************/

long isequal(args)
ARGLIST *args;
{

long	ans;
char	*varadr;
long	vartype;
ARGLIST	*carg;

carg	= args;		/* point to first argument */
varadr	= args->varaddress;
vartype	= args->vartype;
ans	= ES$FAIL;

while ((ans != ES$SUCC) && (carg->next != NULL))
    {
    carg = carg->next;
    switch (vartype)
	{
	case DT_STRING :
	    ans = (strcmp(*(char **)varadr,carg->varaddress) == 0)
			? ES$SUCC : ES$FAIL;
	break;
	case DT_SHORT :
	case DT_SHORT_INDEX :
	    ans = (*(short*)varadr == *(short*)carg->varaddress)
			? ES$SUCC : ES$FAIL;
	break;
	default:
	    ans = (*(long*)varadr == *(long*)carg->varaddress)
			? ES$SUCC : ES$FAIL;
	break;
	}
    }
return(ans);
}


/**********************************************************************
*
*	.SBTTL PARSE - Parse Selection Criteria
*++
* FUNCTIONAL DESCRIPTION:		
*
*	This function performs the following builds a selection
*	node. In the processes of doing this it will also do
*	the following:
*	    1. Parse the format string into types
*	    2. Perform type conversions when appropriate
*	    3. Validate selection criteria
*
* FUNCTION DECLARATION
*	SELNODE *es$parse();
*	
* CALLING SEQUENCE: node = es$parse(stat,fldno,format [,argn...]) 
*
* FORMAL PARAMETERS:		
*
*   long    stat;	    See notes below
*   long    fldno;	    See notes below
*   char    *format;	    See notes below
*  
* SIDE EFFECTS:	Space is allocated for a node and data is
*		stored in that node.
*
* NOTES:
*   The following error code are returned in stat.
*	    ES$SUCC - node created
*	    ES$NOFLD - bad EIMS field number
*	    ES$BADOPCODE  - bad opcode
*	    ES$NOINDEX - no index value found a coded field
*	    ES$BADFMT - bad format string
*
*   format  The format string is similiar to the printf format
*	    string. The only supported formats are %s for strings
*	    and %d for long integers.
*
*	    The select format is 
*		"Opcode arg1, arg2 [,argn...]"
*
*		where argn is either a text string or a placeholder
*		  of the form %d (for long) or %s (for string). If
*		  a placeholder is used, then there must be an
*		  additional argument for each placeholder.
* EXAMPLES:
*
*	node = es$parse(stat,fldno,"eq rp06, rp07");
*	node = es$parse(stat,fldno,"eq %s",p1);
*--
**********************************************************************/

SELNODE *es$parse(va_alist)
va_dcl
{

#define FMTBUFSIZE 128

va_list	incr;
long	*statptr;
long	retstat;
char	*opc;
char	*token;
short	fldno;
short	opcode;
char	*fmt;			/* pointer to local copy of fmt string */
char	*ufmt;			/* user format string */
static	char	fmtbuf[FMTBUFSIZE]; /* store local fmt string here */

short	ftype;

char	*spar;			/* string paramter */
long	lpar ;			/* long paramter */
short	ptype;			/* paramter type */
short	argcnt;			/* number in arglist */
char	*temp;

ARGLIST *args;				/* argument list */
SELNODE *snode;				/* store the return value here */

DD$STD_ITEMS_DSD_PTR item_ptr;

va_start(incr);
statptr = va_arg(incr,long *);	        /* address of the status array */
fldno = va_arg(incr,long);		/* pick up field number */
ufmt = va_arg(incr,char *);		/* address of selection format */

fmt = fmtbuf;
strcpy(fmt,ufmt);		    /* make a local copy */
fmt = trimleadingspaces(fmt);
 
if ((item_ptr = find_std_item_dsd(fldno)) == DD$UNKNOWN_ITEM)
    {
    *statptr = ES$NOFLD;
    return(NULL);
    }
ftype = item_ptr->TYPE;
					/* process opcode */
fmt = trimleadingspaces(fmt);
opc = nexttoken(fmt,' ',&fmt);  /* get opcode */
strtoupper(opc);
opcode = cvtopc(opc);
if (opcode == ES$FAIL)
    {
    *statptr = ES$BADOPCODE;
    return(NULL);
    }
				/* process the remaining paramters
				   there must be atleast one */
argcnt = 1;

args = es$mkarglist(fldno,ftype,0);
 
while (fmt != NULL) /* for each token */
    {
    token = nexttoken(fmt,',',&fmt); /* for each token */
    token = trimspaces(token);
    if (strlen(token) != 0)
        {
	ptype = DT_STRING; /* default to string */
	if (*token == '%')
	    {
	    token++;
	    switch(*token)	/* char after % defines the type */
	        {
		case 'd' :
		    lpar = va_arg(incr,long);
		    ptype = DT_LONG;
		break;
		case 's' :
		    spar = va_arg(incr,char *);
		    ptype = DT_STRING;
		break;
		default :
		    *statptr = ES$BADFMT;
		    return(NULL);
		break;
		}
	    }
	else
	    spar = token;
	temp = cvtparms(&retstat,fldno,ftype,ptype,spar,lpar);
	if (retstat != ES$SUCC)
	    {
	    *statptr = retstat;
	    return(NULL);
	    }
	argcnt++;
	retstat = es$apparglist(args,fldno,ftype,temp);
        }			 /* end of test for zero length tokens */
    }				 /* end of reading tokens */
    
snode = es$mkselectnode(opcode,NULL,NULL,args);
*statptr = ES$SUCC;
return(snode);
}

/****************************** CVTPARMS ******************************/
/*
*   This function converts the input paramater to the
*   type of the EIMS field.
*   The function returns the address of the stored paramter 
*/

char *cvtparms(stat,fldno,ftype,ptype,spar,lpar)
long	*stat;
short	fldno;
short	ftype;
short	ptype;
char	*spar;
long	lpar;
{

char	*ret;
long	llong;

*stat	= ES$SUCC;
ret	= NULL;

switch (ftype)
    {
    case DT_SHORT :  
    case DT_LONG  :
    case DT_SHORT_REGISTER :
    case DT_REGISTER :
    case DT_DATE  :
	if (ptype == DT_STRING)
	    ret = storelong(atol(spar));
	else
	    ret = storelong(lpar);
    break;
    case DT_STRING :
	ret = storestring(spar);
    break;
    case DT_SHORT_INDEX :
    case DT_INDEXED :
	if (ptype == DT_STRING)
	    {
	    if ((llong = get_code_std_item(fldno,spar)) == DD$UNKNOWN_CODE)
		{
		*stat = ES$TBLERR;
		return(NULL);
		}
	    *stat = ES$SUCC;
	    ret = storelong(llong);
	    }
	else
	    ret = storelong(lpar);
    break;
    default:
	*stat = ES$TPER;
	return(NULL);
    break;
    }
return(ret);
}


/****************************** STORESTRING ***************************/

char *storestring(s)
char *s;
{

char *ret;

ret = malloc(strlen(s)+1);
strcpy(ret,s);
return(ret);
}

/****************************** STORELONG ***************************/

char *storelong(l)
long l;
{

long *ret;

ret = (long *) malloc(sizeof(long));
*ret = l;
return((char *)ret);
}

/****************************** CVTOPC ***************************/

long cvtopc(s)
char *s;
{
static char *opcodes[] = { 
	    "",
	    "AND",
	    "OR",
	    "NOT",
	    "EQ",
	    "RANGE",
	    "LT",
	    "LE",
	    "GT",
	    "GE" };

short	 low = 1;
short	 high = 10;
short	 i;
    
for (i = low; i < high; i++)
    {
    if (strcmp(opcodes[i],s) == 0)
	return(i);
    }
return(ES$FAIL);    
}

/****************************** STRTOUPPER ***************************/

void strtoupper(s)
char *s;
{

while (*s != NULL)
    {
    *s = toupper(*s);
    s++;
    }
}

/****************************** NEXTTOKEN ****************************/

char *nexttoken(str,delimiter,rem)
char	*str;
int	delimiter;
char	**rem;
{

char	*ret;
char	*cloc;
    
ret	 = str;
cloc	 = strchr(str,delimiter);
if (cloc == NULL) 
    {
    *rem = NULL;			/* set remainder to NULL */
    return(ret);
    }
*(cloc) = '\0';			 /* terminate the string at the delimiter */
cloc++;
*rem = cloc;
return(ret);      
}

/****************************** TRIMSPACES ***************************/

char *trimspaces(str)
char *str;
{

char	*ret;
char	*strend;

ret	= str;
while (isspace(*ret) != 0)
    ret++;
strend = ret + strlen(ret) -1 ;
while (isspace(*strend) != 0)
    strend--;
strend++;
*(strend) = '\0';
return(ret);
}

/*************************** TRIMLEADINGSPACES ***************************/

char *trimleadingspaces(str)
char *str;
{

char	*ret;
char	*strend;

ret = str;
while (isspace(*ret) != 0)
    ret++;
return(ret);
}


/************************************************************************
*
*	.SBTTL	Make select node
*++
* FUNCTIONAL DESCRIPTION:		
*
*	This function creates a selection node. Space is allocated
*	for the node, a node is created, and the input parameters
*	are stored in the node.
*
* FUNCTION DECLARATION
*	SELNODE *es$mkselectnode();
*	
* CALLING SEQUENCE: node = es$mkselectnode(opr,left,right,args)
*
* FORMAL PARAMETERS:		
*
*   short opr; Must be on of the following:
*		ES$AND ES$OR ES$NOT ES$EQUAL ES$RANGE ES$LT ES$LE ES$GT ES$GE 
*   SELNODE *left;
*   SELNODE *right;
*   ARGLIST *args;
*
* IMPLICIT INPUTS: Paramters passed in the call
*
*	
* SIDE EFFECTS:			
*	Space is allocated for a node and data is
*	stored in that node.
*
* UNDESIRED EVENTS
*	This function returns a NULL pointer if the node cannot
*	be created for some reason. The only know reason is
*	insufficient memory to allocate for the node.
*	
*--
*************************************************************************/

SELNODE *es$mkselectnode(opr,lnode,rnode,args)
short opr;
SELNODE *lnode;
SELNODE	*rnode;
ARGLIST *args;
{

SELNODE *answer;

answer = (SELNODE *) malloc(sizeof(SELNODE));

answer->operator	= opr;
answer->left		= lnode;
answer->parent		= NULL;
answer->right		= rnode;
answer->operands	= args;

if (lnode != NULL)
    lnode->parent = answer;
if (rnode != NULL)
    rnode->parent = answer;

return(answer);
}

/*************************************************************************
*
*	.SBTTL	Make new argument list
*++
* FUNCTIONAL DESCRIPTION:		
*
*	This function creates a new argument list. The first
*	element of the list is also created.
*
* FUNCTION DECLARATION
*	ARGLIST arglist *es$mkarglist();
*	
* CALLING SEQUENCE: args = es$mkarglist(fldno,ftype,varadr)
*
* FORMAL PARAMETERS:		
*
*   short fldno; EIMS field number
*   short ftype; on of the following data types
*		    DT_SHORT DT_LONG DT_DATE DT_STRING
*   char *varadr;  address of a variable
*
* IMPLICIT INPUTS: Paramters passed in the call
*
*	
* SIDE EFFECTS:			
*	Space is allocated for the first (top) element of the list.
*
* UNDESIRED EVENTS
*	This function returns a NULL pointer if the list cannot
*	be created for some reason. The only know reason is
*	insufficient memory to allocate for the node.
*	
*--
*************************************************************************/

ARGLIST *es$mkarglist(fldno,fldtype,varadr)
short	fldno;
short	fldtype;
char	*varadr;
{

ARGLIST *answer;

answer = (ARGLIST *) malloc(sizeof(ARGLIST));

if (answer == NULL)
    return(NULL);

answer->fldno		 = fldno;
answer->varaddress	 = varadr;
answer->vartype		 = fldtype;
answer->next		 = NULL;

return(answer);
}

/************************************************************************
*
*	.SBTTL	Append to end of argument list
*++
* FUNCTIONAL DESCRIPTION:		
*
*	This function adds a new element to the end of an
*	argument list.
*
* FUNCTION DECLARATION
*	long es$apparglist();
*	
* CALLING SEQUENCE: status = es$apparglist(args,fldno,fldtype,varadr)
*
* FORMAL PARAMETERS:		
*
*   ARGLIST *args ; pointer to the argument list to append to
*   short fldno; EIMS field number
*   short vartype; on of the following data types
*		    DT_SHORT DT_LONG DT_DATE DT_STRING
*   char *varadr;  address of a user variable
*
* IMPLICIT INPUTS: Paramters passed in the call
*
*	
* SIDE EFFECTS:			
*	Space is allocated for the first (top) element of the list.
*
* FUNCTION VALUE
*	ES$SUCC - element appended to list
*	ES$NOMEM - no room to add element
*
* UNDESIRED EVENTS
*	This function returns ES$NOMEM if the list cannot
*	be created for some reason. The only know reason is
*	insufficient memory to allocate for the node.
*	
*--
*************************************************************************/

long es$apparglist(args,fldno,fldtype,varadr)
ARGLIST *args;
short	fldno;
short	fldtype;
char	*varadr;
{

ARGLIST *curr;
ARGLIST	*newone;

curr = args;
while (curr->next != NULL)
    curr = curr->next;

newone = (ARGLIST *) malloc(sizeof(ARGLIST));
if (newone == NULL)
    return(ES$NOMEM);

curr->next		 = newone;
newone->fldno		 = fldno;
newone->varaddress	 = varadr;
newone->vartype		 = fldtype;
newone->next		 = NULL;

return(ES$SUCC);
}
/**************************** DELETETREE *****************************/

/*  This function deletes and tree and frees up the space */

void deletetree(tree)
SELNODE *tree;
{

if (tree != NULL)
    {
    if (tree->left != NULL)  deletetree(tree->left);
    if (tree->right != NULL) deletetree(tree->right);
    free(tree);
    }
}

/**************************** NEXTINORDER ****************************/

/*  This function returns a pointer to the next node processed inorder. */

SELNODE *nextinorder(last)
SELNODE *last;
{

SELNODE	*ans;
SELNODE	*prev;

if (last->right != NULL)
    return(firstinorder(last->right));		/* the easy one */
ans	= last->parent;
prev	= last;
  
while (ans != NULL)
    {
    if (prev == ans->left)
	return(ans);			/* If we came from the left
					    then we now do the parent */
    prev = ans;
    ans = ans->parent;			/* back up one parent */
    }
return(NULL);				/* backed up to parent */
}

/****************************** FIRSTINORDER ****************************/

/*  This function returns a pointer to the first node processed inorder. */

SELNODE *firstinorder(top)
SELNODE *top;
{
SELNODE *ans;

ans = top;
while (ans->left != NULL)
    ans = ans->left;
return(ans);
}


