#ifndef lint
static char sccsid[]  =  "@(#)getcmd.c	4.2   (ULTRIX)   10/10/90";
#endif  lint
/*
*	.TITLE	GETCMD - Parse and verify a command line.
*	.IDENT	/1-001/
*
* COPYRIGHT (C) 1985 DIGITAL EQUIPMENT CORP.,
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
* FACILITY:		[ FMA Software Tools - Detail Design ]
*
* ABSTRACT:
*
*	These functions are used to read an ULTRIX command line
*	and to parse it into a parse tree that can be verified
*	
* ENVIRONMENT:	ULTRIX-32 C
*	
* AUTHOR:  Luis Arce,  CREATION DATE:  19-Nov-85
*
* MODIFIED BY:
*
*
*--
*/

#include "ueliterals.h"
#include "uestruct.h"
#include "uerror.h"
#include "eiliterals.h"
#include "erms.h"
#include "generic_dsd.h"
#include "std_dsd.h"
#include <stdio.h>
#include <sys/time.h>

/*
*++
*=
*=
*=MODULE FLOW - getcmd.c
*=
*=  a - parse_cmd(argc,argv)                        Validates syntax,
*=                                                  builds parse tree.
*=          while (args)
*=              {
*=              if (flag)
*=                  {
*=                  validate flags
*=                  make_tree(root_tree,flag)               (* b)
*=                  }
*=              if (param)
*=                  {
*=                  malloc(param struct)
*=                  chain to tree
*=                  }
*=              }
*=          return(UE$SUCC)
*=
*=  b - make_tree(tree,flag)                        Recursive routine 
*=                                                  searches tree for dup
*=                                                  entry, alloc new branch.
*=          if(tree == NULL)
*=              malloc(param struct)
*=          else
*=              make_tree(root_tree,flag)                   (* b)
*=          return(tree)
*=
*=  c - valid_tool(tree)                            Validates branches
*=          if (get_tree(UE$FLG_n))   socket                (* f)
*=              {
*=              es$open(ctx,event,mailbox,NULL,segs...)     (esopen.c)
*=              }
*=          if (get_tree(UE$FLG_f))   file                  (* f)
*=              {
*=              fopen(config_file)
*=              get path
*=              fclose()
*=              gethostname(string)
*=              es$open(ctx,event,stat,f_name,segs...)      (esopen.c)
*=              }
*=          if (get_tree(UE$FLG_b))   bin output            (* f)
*=              es$open(ctx,event,append,f_name,segs...)    (esopen.c)
*=
*=          if (get_tree(UE$FLG_o | Z | S))                 (* f)
*=              set flags on in_structure
*=          return(UE$SUCC)
*=
*=  d - valid_gen(parse_tree)                       Validates all other flags.
*=                                                  Builds a selection tree.
*=                                                  Lets es$parse validate tree.
*=          if (get_tree(UE$FLG_?))                         (* f)
*=              {
*=              get_devtype(device,status)                  (* e)
*=              asctotime(asc_date)                         (* h)
*=              es$parse(stat,item_id,input)                (select.c)
*=              es$mkselectnode(op,new_node,old_node,NULL)  (select.c)
*=              es$select(ctx,ES$AND,sel_tree)              (esopen.c)
*=              }
*=          return(UE$SUCC)
*=
*=  e - get_devtype(label,status)                   find label in bin file.
*=          find_std_item_dsd(item_id)                      (dsd_access.c)
*=          find_std_code_dsd(item_dsd_ptr,index)           (dsd_access.c)
*=          dsd_get_label(LABEL_IX)                         (dsd_access.c)
*=          es$parse(stat,item_id,input)                    (select.c)
*=          es$mkselectnode(op,new_node,old_node,NULL)      (select.c)
*=          return(node)
*=	
*=  g - help()                                      Prints the file 'uerf.hlp'.
*=          find_file(UE$HLP_FILE)                          (uerf.c)
*=          fopen()
*=          while(fgets(line))
*=              printf(line)
*=          return(UE$SUCC)
*=
*=  f - get_tree(flag)                              Searches for tree node
*=          return(tree)
*=
*=  h - asctotime(asc_date)                         Converts ascii time to
*=                                                  unsigned long.
*=          gettimeofday()
*=          localtime()
*=          parsetime(asc_date,time_struct)                 (* i)
*=          return(date)
*=
*=  i - parsetime(asc_date,time_struct)             Fills in time structure
*=
*--
*/

extern struct in_struc in_st;
ES$DECLARE(extern,EIS,ueis);
ES$DECLARE(extern,DIS,udis);
ES$DECLARE(extern,SDS,usds);
ES$DECLARE(extern,CDS,ucds);
ES$DECLARE(extern,ADS,uads);
ES$DECLARE(extern,SIS,usis);
ES$STREAM(extern,uctx);


/*
*	.SBTTL	parse_cmd - function to parse ULTRIX command line.
*++
* FUNCTIONAL DESCRIPTION:		
*
*	-  Validates command syntax.
*	-  Builds parse tree.
*	-  Returns syntax validated or error code.
*
* FORMAL PARAMETERS:		
*
*	argc			argument count
*	argv			command line arguments
*
* IMPLICIT INPUTS:		none
*
* IMPLICIT OUTPUTS:		none
*
* COMPLETION STATUS:		UE$SUCC = successful
*				UE$FAIL = failure
*
* SIDE EFFECTS:			none
*
*--
*/

/*...	FUNCTION parse_cmd (argc,argv)				*/
long  parse_cmd (argc,argv)
short argc;
char  *argv[];
{

struct parse_tree *tree;
struct parm_struc *parm, *xparm;
char *malloc();

static char flg_tbl[] =
    {
    UE$FLG_f,
    UE$FLG_r,
    UE$FLG_t,
    UE$FLG_h,
    UE$FLG_n,
    UE$FLG_x,
    UE$FLG_c,
    UE$FLG_D,
    UE$FLG_T,
    UE$FLG_u,
    UE$FLG_e,
    UE$FLG_O,
    UE$FLG_M,
    UE$FLG_A,
    UE$FLG_o,
    UE$FLG_H,
    UE$FLG_s,
    UE$FLG_R,
    UE$FLG_Z,
    UE$FLG_S,
    UE$FLG_b,
    '\0'
    };

long status;
char   *f;
short  i,j;
short  plen, len;
in_st.root = UE$NULL;

while (--argc > 0)
    {
    *++argv;
    if (*argv[0] == '-')
	{
	if (strlen(argv[0]) == 1)
	    return(UE$ERR_INVFLG);
	for (f = argv[0]+1; *f != '\0'; f++)
	    {
	    for (i=0;;i++)
		{
		if (flg_tbl[i] == *f)
		    break;
		if (flg_tbl[i] == '\0')
		    return(UE$ERR_INVFLG);
		if (*f == UE$FLG_h)
		    {
		    if ((status = help()) != UE$SUCC)
			return(status);
		    else
			return(UE$DONE);
		    }
		}
    	    status = make_tree(in_st.root,*f);
	    if (status == UE$ERR_DUPFLG)
		return(UE$ERR_DUPFLG);
    	    in_st.root = (struct parse_tree *)status;
	    }
	}
    else
	{
	if (in_st.last == UE$NULL)
	    return(UE$ERR_INVPAR);
	tree = in_st.last;
	plen = strlen(*argv);
	j = 0;
	while (j < plen)
	    {
	    if (tree->parm_list == UE$NULL)
		{
		parm = (struct parm_struc *) malloc(sizeof(struct parm_struc));
		tree->parm_list = parm;
		}
	    else
		{
		while (parm->next_parm != UE$NULL)
		    {
		    parm = parm->next_parm;
		    }
		parm->next_parm = (struct parm_struc *) malloc(sizeof(struct parm_struc));
		parm = parm->next_parm;
		}
	    parm->next_parm = UE$NULL;
	    parm->parm_string = *argv+j;
	    j += (1 + (len = strcspn(*argv+j,",")));
	    (*argv)[j-1] = '\0';
	    }
	}
    }
    return(UE$SUCC);
}

/*...	ENDFUNCTION parse_cmd					*/

/*
*	.SBTTL	make_tree - Function to create a parse tree limb.
*++
* FUNCTIONAL DESCRIPTION:		
*
*	-  Searches to end of tree for duplicate entry.
*	-  Allocates space for next tree structure.
*	-  Returns valid tree address or error code.
*
* FORMAL PARAMETERS:		
*
*
* IMPLICIT INPUTS:		NONE
*
* IMPLICIT OUTPUTS:		New tree branch
*
* COMPLETION STATUS:		Address of tree branch if successful
*				UE$FAIL = failure
*
* SIDE EFFECTS:			NONE
*
*--
*/
/*...	FUNCTION make_tree(flag)				*/
int make_tree (nx,fl)

struct parse_tree *nx;
char  fl;

{

long   status;
char *malloc();

if (nx == UE$NULL)			/* find end of chain	*/
    {
					/* Make a new node	*/
    nx = (struct parse_tree *) malloc(sizeof(struct parse_tree));
    in_st.last = nx;
    nx->flag[0] = fl;			/* Save the flag ident	*/
    nx->parm_list = UE$NULL;		/* NULL param list ptr	*/
    nx->next_tree = UE$NULL;		/* NULL next tree point	*/
    }
else
    {
    if (nx->flag[0] == fl)			/* Duplicate flag?	*/
	{
	return(UE$ERR_DUPFLG);
	}
					/* get next branch	*/
    status = make_tree(nx->next_tree,fl);
    if (status == UE$ERR_DUPFLG)
	return(UE$ERR_DUPFLG);
    nx->next_tree = (struct parse_tree *)status;
    }
return((int)nx);
}
/*...	ENDFUNCTION make_treee(root, flag)			*/


/*
*	.SBTTL	valid_tool - Function to validate UERF specific input.
*++
* FUNCTIONAL DESCRIPTION:		
*
*	-  Validates all UERF specific input.
*	-  Calls erm_open who validates input files.
*	-  Returns valid UERF info or error code.
*
* FORMAL PARAMETERS:		
*
* IMPLICIT INPUTS:		none
*
* IMPLICIT OUTPUTS:		validated UERF specific input
*
* COMPLETION STATUS:		UE$SUCC = successful
*				UE$FAIL = failure
*
* SIDE EFFECTS:			NONE
*
*--
*/
/*...	FUNCTION valid_tool ()					*/
long  valid_tool ()
{
struct parse_tree *get_tree();
struct parse_tree *tree;
struct parm_struc *parm;
char   pstring[UE$XFF];

int i, j;
FILE *cfp;
char *cp, *cp2;
char line[256];
int dataflg = 0;
long status;

/************** PROCESS "n" FLAG ********************************/

					/* get "n" flag tree	*/
if (( tree = get_tree(UE$FLG_n)) != UE$NULL)
    {
    if (tree->parm_list != UE$NULL)
	return(UE$ERR_INVPAR);
    in_st.kernel = TRUE;
    if (get_tree(UE$FLG_f) != UE$NULL)
	return(UE$ERR_INCOMP);
    if (get_tree(UE$FLG_t) != UE$NULL)
	return(UE$ERR_INCOMP);
    if (get_tree(UE$FLG_R) != UE$NULL)
	return(UE$ERR_INCOMP);
    if (get_tree(UE$FLG_S) != UE$NULL)
	return(UE$ERR_INCOMP);
    if (es$open(&uctx,ES$EVENT,ES$MAILBOX,NULL,
		    &ueis,&udis,&usds,&ucds,&uads,&usis) != ES$SUCC)
	{
	return(UE$ERR_K_OPEN);	/* unable to open socket*/
	}
    }
else
    {
/************** PROCESS "f" FLAG ********************************/

    in_st.kernel = FALSE;

    cfp = fopen(UE$FILE_CONF,"r");
    if (cfp == NULL)
	return(UE$ERR_NOCONF);
    i = 0;
    while (fgets(line,sizeof(line),cfp) != NULL)
	{
	cp = line;
	if (*cp == '#' && dataflg == 0)
	    continue;
	if (*cp == '}')
	    break;
	if (*cp == '{')
	    {
	    dataflg++;
	    continue;
	    }
	if (dataflg > 0)
	    {
	    if ( i == 2)
		{
		while (*cp == ' ' || *cp == '\t')
		    cp++;
	    	if (*cp == '#')		/* no data string	*/
		    {
		    (void)fclose(cfp);
	            return(UE$ERR_CONFIG);
		    }
	        cp2 = cp;
	        while (*cp2 != ' ' && *cp2 != '\t' &&
			       *cp2 != '\n' && *cp2 != '#') 
		    cp2++;
	        *cp2 = '\0';
	        (void)strcpy(pstring,cp);
		(void)strcat(pstring,"/syserr.");
		break;
		}
	    i++;
	    }
	}
    (void)fclose(cfp);
    gethostname(pstring+strlen(pstring), UE$HST_LEN);

					/* get "f" flag tree	*/
    if (( tree = get_tree(UE$FLG_f)) != UE$NULL)
        {
        if (( parm = tree->parm_list) == UE$NULL)
	    return(UE$ERR_INVPAR);
	(void)strcpy(pstring, parm->parm_string);
	if (parm->next_parm != UE$NULL)
	    return(UE$ERR_INVPAR);
        }
    if (get_tree(UE$FLG_R) != UE$NULL)
	status = EI$REVERSE;
    else
	status = ES$VIEW;
    if (es$open(&uctx,ES$EVENT,status,pstring,
		&ueis,&udis,&usds,&ucds,&uads,&usis) != ES$SUCC)
        return(UE$ERR_S_OPEN);
    }
/***************** PROCESS "b" FLAG *****************************/

in_st.out_file = FALSE;

if (( tree = get_tree(UE$FLG_b)) != UE$NULL)
    {
    if (( parm = tree->parm_list) == UE$NULL)
        return(UE$ERR_INVPAR);
    (void)strcpy(pstring, parm->parm_string);
    if (parm->next_parm != UE$NULL)
	return(UE$ERR_INVPAR);
    if (es$open(NULL,ES$EVENT,ES$APPEND,pstring,
		UE$NULL,UE$NULL,UE$NULL,UE$NULL,UE$NULL) != ES$SUCC)
        return(UE$ERR_S_OPEN);
    in_st.out_file = TRUE;
    }

/***************** PROCESS "o" FLAG *****************************/

in_st.out_form = UE$OUT_BRIEF;		/* default = brief	*/
					/* go get "o" tree	*/
if (( tree = get_tree(UE$FLG_o)) != UE$NULL)
    {
    if (( parm = tree->parm_list) == UE$NULL)
	return(UE$ERR_INVPAR);
    if      ((strncmp(parm->parm_string,"ful",3)) == 0)
        in_st.out_form = UE$OUT_FULL;
    else if ((strncmp(parm->parm_string,"ter",3)) == 0)
        in_st.out_form = UE$OUT_TERSE;
    else if ((strncmp(parm->parm_string,"bri",3)) == 0)
        in_st.out_form = UE$OUT_BRIEF;
    else
	return(UE$ERR_INVPAR);
    if (parm->next_parm != UE$NULL)
	return(UE$ERR_INVPAR);
    }
/***************** PROCESS "Z" FLAG *****************************/

if (( tree = get_tree(UE$FLG_Z)) != UE$NULL)
    in_st.dump_rec = TRUE;
else
    in_st.dump_rec = FALSE;

/***************** PROCESS "S" FLAG *****************************/

if (( tree = get_tree(UE$FLG_S)) != UE$NULL)
    in_st.summary = TRUE;
else
    in_st.summary = FALSE;

/****************************************************************/
return(UE$SUCC);
}
/*...	ENDFUNCTION valid_tool					*/


/*
*	.SBTTL	valid_gen - Function to validate generic input.
*++
* FUNCTIONAL DESCRIPTION:		
*
*	-  Validates all input which is generic to all tools.
*	-  Builds a selection tree,
*	-  Calls erm_select who validates selection criteria and
*	   sets up for later selection.
*	-  Returns valid generic info or error code.
*
* FORMAL PARAMETERS:		
*
* IMPLICIT INPUTS:		none
*
* IMPLICIT OUTPUTS:		validated generic input
*
* COMPLETION STATUS:		UE$SUCC = successful
*				UE$FAIL = failure
*
* SIDE EFFECTS:			NONE
*
*--
*/

/**********************  NODE BUILDING MACRO *********************/

#define BLD_NODE(new_node,op,old_node) \
	if (old_node == UE$NULL) \
	    old_node = new_node; \
	else \
	    old_node = es$mkselectnode(op,new_node,old_node,UE$NULL);

/****************************************************************/

/*...	FUNCTION valid_gen ()					*/
long  valid_gen ()
{
unsigned long   asctotime();
struct parse_tree *get_tree();
SELNODE *es$mkselectnode();
SELNODE *es$parse();
SELNODE *get_devtype();

SELNODE *sel_tree;		/* selection tree  	*/
SELNODE *flag_node;		/* node of option flag	*/
SELNODE *pars_node;		/* node from parse	*/
SELNODE *temp_node;		/* node for temp use	*/

struct parse_tree *tree;
struct parm_struc *parm;
char   pstring[UE$XFF];
char   tstring[UE$XFF];
long   stat;
long   status;
long   pcode;
unsigned long   datetime;
short  i;
short  j;
short  len;

sel_tree = UE$NULL;

/*  OR ********* PROCESS "c" FLAG *******************************/

if ((tree = get_tree(UE$FLG_c)) != UE$NULL)
    {
    flag_node = UE$NULL;
    if ((parm = tree->parm_list) != UE$NULL)
	{
	do
	    {
	    if      ((strncmp((parm->parm_string),"err",3)) == 0)
		pcode = ec$error;
	    else if ((strncmp((parm->parm_string),"ope",3)) == 0)
		pcode = ec$operator;
	    else if ((strncmp((parm->parm_string),"mai",3)) == 0)
		pcode = ec$maintenance;
	    else
		return(UE$ERR_INVPAR);
            pars_node = es$parse(&status,DD$eventclass,"eq %d", pcode);
	    if (status != ES$SUCC)
		return(UE$ERR_INVPAR);
	    BLD_NODE (pars_node,ES$OR,flag_node);
	    }
        while ((parm = parm->next_parm) != UE$NULL);
	BLD_NODE (flag_node,ES$OR,sel_tree);
	}
    else
	return(UE$ERR_INVPAR);
    }

/*  OR ********* PROCESS "T" FLAG *******************************/

if ((tree = get_tree(UE$FLG_T)) != UE$NULL)
    {
    flag_node = UE$NULL;
    if ((parm = tree->parm_list) != UE$NULL)
	{
	do
	    {
	    pars_node = get_devtype(parm->parm_string,&status);
	    if (status != UE$SUCC)
		return(UE$ERR_INVPAR);
	    BLD_NODE (pars_node,ES$OR,flag_node);
	    }
        while ((parm = parm->next_parm) != UE$NULL);
	}
    pars_node = es$parse(&status, DD$devclass, "eq %d, %d, %d,%d",
                dc$dstape,dc$mbtape,dc$ubtape,dc$sctape);
    if (status != ES$SUCC)		/* status from es$parse	*/
	return(UE$ERR_INVPAR);
    BLD_NODE (pars_node,ES$AND,flag_node);
    BLD_NODE (flag_node,ES$OR,sel_tree);
    }

/*  OR ********* PROCESS "D" FLAG *******************************/

if ((tree = get_tree(UE$FLG_D)) != UE$NULL)
    {
    flag_node = UE$NULL;
    if ((parm = tree->parm_list) != UE$NULL)
	{
	do
	    {
	    pars_node = get_devtype(parm->parm_string,&status);
	    if (status != UE$SUCC)
		return(UE$ERR_INVPAR);
	    BLD_NODE (pars_node,ES$OR,flag_node);
	    }
        while ((parm = parm->next_parm) != UE$NULL);
	}
    pars_node = es$parse(&status, DD$devclass, "eq %d, %d, %d, %d",
                dc$scdisk,dc$dsdisk,dc$mbdisk,dc$ubdisk);
    if (status != ES$SUCC)		/* status from es$parse	*/
	return(UE$ERR_INVPAR);
    BLD_NODE (pars_node,ES$AND,flag_node);
    BLD_NODE (flag_node,ES$OR,sel_tree);
    }

/*  OR *************** PROCESS "u" FLAG *************************/

if ((tree = get_tree(UE$FLG_u)) != UE$NULL)
    {
    if ((parm = tree->parm_list) != UE$NULL)
        {
        flag_node = UE$NULL;
	do
	    {
            strcpy(tstring,parm->parm_string);
            temp_node = UE$NULL;
            i = 0;
            while (tstring[i] != '\0')
                {
                (void)strcpy(pstring,"eq ");
                j = 3;
                while (isalpha(tstring[i]) != 0)
                    pstring[j++] = tstring[i++];
                pstring[j] = '\0';
                pars_node = es$parse(&status, DD$sum_dev_pre, pstring);
                if (status != ES$SUCC)	/* status from es$parse */
                    return(UE$ERR_INVPAR);
                BLD_NODE (pars_node,ES$AND,temp_node);

                (void)strcpy(pstring,"eq ");
                j = 3;
                while (isdigit(tstring[i]) != 0)
                    pstring[j++] = tstring[i++];
                pstring[j] = '\0';
                pars_node = es$parse(&status, DD$sum_unum, pstring);
                if (status != ES$SUCC)	/* status from es$parse */
                    return(UE$ERR_INVPAR);
                BLD_NODE (pars_node,ES$AND,temp_node);
                BLD_NODE (temp_node,ES$OR,flag_node);
                }
	    }
        while ((parm = parm->next_parm) != UE$NULL);
        BLD_NODE (flag_node,ES$OR,sel_tree);
	}
    else
	return(UE$ERR_INVPAR);
    }

/*  OR ********* PROCESS "A" FLAG *******************************/

if ((tree = get_tree(UE$FLG_A)) != UE$NULL)
    {
    flag_node = UE$NULL;
    if ((parm = tree->parm_list) != UE$NULL)
	{
	do
	    {
	    pars_node = get_devtype(parm->parm_string,&status);
	    if (status != UE$SUCC)
		return(UE$ERR_INVPAR);
	    BLD_NODE (pars_node,ES$OR,flag_node);
	    }
        while ((parm = parm->next_parm) != UE$NULL);
	}
    pars_node = es$parse(&status, DD$devclass, "eq %d",dc$adapter);
    if (status != ES$SUCC)		/* status from es$parse	*/
	return(UE$ERR_INVPAR);
    BLD_NODE (pars_node,ES$AND,flag_node);
    BLD_NODE (flag_node,ES$OR,sel_tree);
    }

/*  OR ********* PROCESS "M" FLAG *******************************/

if ((tree = get_tree(UE$FLG_M)) != UE$NULL)
    {
    flag_node = UE$NULL;
    if ((parm = tree->parm_list) != UE$NULL)
	{
	do
	    {
	    if      ((strncmp((parm->parm_string),"cpu",3)) == 0)
		pcode = dc$cpu;
	    else if ((strncmp((parm->parm_string),"mem",3)) == 0)
		pcode = dc$memory;
	    else
		return(UE$ERR_INVPAR);
	    pars_node = es$parse(&status, DD$devclass, "eq %d",pcode);
	    if (status != ES$SUCC)
		return(UE$ERR_INVPAR);
	    BLD_NODE (pars_node,ES$OR,flag_node);
	    }
        while ((parm = parm->next_parm) != UE$NULL);
	}
    else
	{
	pars_node = es$parse(&status, DD$devclass, 
                    "eq %d, %d",dc$cpu, dc$memory);
	if (status != ES$SUCC)
	    return(UE$ERR_INVPAR);
	BLD_NODE (pars_node,ES$OR,flag_node);
	}
    BLD_NODE (flag_node,ES$OR,sel_tree);
    }

/*************** PROCESS "O" FLAG *******************************/

if ((tree = get_tree(UE$FLG_O)) != UE$NULL)
    {
    flag_node = UE$NULL;
    if ((parm = tree->parm_list) != UE$NULL)
	{
	do
	    {
	    if      ((strncmp((parm->parm_string),"aef",3)) == 0)
		pcode = atflt;
	    else if ((strncmp((parm->parm_string),"ast",3)) == 0)
		pcode = astflt;
	    else if ((strncmp((parm->parm_string),"bpt",3)) == 0)
		pcode = bpt;
	    else if ((strncmp((parm->parm_string),"cmp",3)) == 0)
		pcode = cmpflt;
	    else if ((strncmp((parm->parm_string),"pag",3)) == 0)
		pcode = pgflt;
	    else if ((strncmp((parm->parm_string),"pif",3)) == 0)
		pcode = pif;
	    else if ((strncmp((parm->parm_string),"pro",3)) == 0)
		pcode = protect;
	    else if ((strncmp((parm->parm_string),"ptf",3)) == 0)
		pcode = pgtflt;
	    else if ((strncmp((parm->parm_string),"raf",3)) == 0)
		pcode = raf;
	    else if ((strncmp((parm->parm_string),"rof",3)) == 0)
		pcode = rof;
	    else if ((strncmp((parm->parm_string),"scf",3)) == 0)
		pcode = syscall;
	    else if ((strncmp((parm->parm_string),"seg",3)) == 0)
		pcode = segflt;
	    else if ((strncmp((parm->parm_string),"tra",3)) == 0)
		pcode = trace;
	    else if ((strncmp((parm->parm_string),"xfc",3)) == 0)
		pcode = xfc;
	    else
		return(UE$ERR_INVPAR);
	    pars_node = es$parse(&status, DD$coarsesyndrome, "eq %d",pcode);
	    if (status != ES$SUCC)
		return(UE$ERR_INVPAR);
	    BLD_NODE (pars_node,ES$OR,flag_node);
	    }
        while ((parm = parm->next_parm) != UE$NULL);
	}
    else
	{
	pars_node = es$parse(&status, DD$eventtype, "eq 109,200");
	if (status != ES$SUCC)
	    return(UE$ERR_INVPAR);
	BLD_NODE (pars_node,ES$OR,flag_node);
	}
    BLD_NODE (flag_node,ES$OR,sel_tree);
    }

/*  OR ********* PROCESS "s" FLAG *******************************/

if ((tree = get_tree(UE$FLG_s)) != UE$NULL)
    {
    if (get_tree(UE$FLG_r) != UE$NULL)
	return(UE$ERR_INCPAR);
    if (get_tree(UE$FLG_c) != UE$NULL)
	return(UE$ERR_INCPAR);
    (void)strcpy(pstring,"eq    ");
    flag_node = UE$NULL;
    if ((parm = tree->parm_list) != UE$NULL)
        {
	do
	    {
            (void)strcat(pstring,parm->parm_string);
            i = strcspn(pstring,"-");
            if ((i == 0) || (i == strlen(pstring)-1))
	        return(UE$ERR_INVPAR);
            if (i != strlen(pstring))
	        {
	        strncpy(pstring+i,",",1);
	        strncpy(pstring,"range",5);
	        }
            pars_node = es$parse(&status, DD$recordnumber, pstring);
            if (status != ES$SUCC)	/* status from es$parse */
	        return(UE$ERR_INVPAR);
	    BLD_NODE (pars_node,ES$OR,flag_node);
	    (void)strcpy(pstring,"eq   ");
	    }
        while ((parm = parm->next_parm) != UE$NULL);
        BLD_NODE (flag_node,ES$OR,sel_tree);
	}
    else
	return(UE$ERR_INVPAR);
    }

/*  OR ********* PROCESS "r" FLAG *******************************/

if ((tree = get_tree(UE$FLG_r)) != UE$NULL)
    {
    if (get_tree(UE$FLG_s) != UE$NULL)
	return(UE$ERR_INCPAR);
    if (get_tree(UE$FLG_c) != UE$NULL)
	return(UE$ERR_INCPAR);
    (void)strcpy(pstring,"eq    ");
    flag_node = UE$NULL;
    if ((parm = tree->parm_list) != UE$NULL)
        {
	do
	    {
            (void)strcat(pstring,parm->parm_string);
            i = strcspn(pstring,"-");
            if ((i == 0) || (i == strlen(pstring)-1))
	        return(UE$ERR_INVPAR);
            if (i != strlen(pstring))
	        {
	        strncpy(pstring+i,",",1);
	        strncpy(pstring,"range",5);
	        }
            pars_node = es$parse(&status, DD$eventtype, pstring);
            if (status != ES$SUCC)	/* status from es$parse */
	        return(UE$ERR_INVPAR);
	    BLD_NODE (pars_node,ES$OR,flag_node);
	    (void)strcpy(pstring,"eq   ");
	    }
        while ((parm = parm->next_parm) != UE$NULL);
        BLD_NODE (flag_node,ES$OR,sel_tree);
	}
    else
	return(UE$ERR_INVPAR);
    }

/* NOT ********* PROCESS "x" FLAG *******************************/

if ((tree = get_tree(UE$FLG_x)) != UE$NULL)
    {
    if (tree->parm_list != UE$NULL)
	return(UE$ERR_INVPAR);
    if (sel_tree != UE$NULL)
        sel_tree = es$mkselectnode(ES$NOT,sel_tree,UE$NULL,UE$NULL);
    }

/* AND ********* PROCESS "H" FLAG *******************************/

if ((tree = get_tree(UE$FLG_H)) != UE$NULL)
    {
    flag_node = UE$NULL;
    if ((parm = tree->parm_list) != UE$NULL)
        {
	do
	    {
            (void)strcpy(pstring,"eq ");
            (void)strcat(pstring,parm->parm_string);
            pars_node = es$parse(&status, DD$hostname, pstring);
            if (status != ES$SUCC)
	        return(UE$ERR_INVPAR);
	    BLD_NODE (pars_node,ES$OR,flag_node);
	    }
        while ((parm = parm->next_parm) != UE$NULL);
        BLD_NODE (flag_node,ES$AND,sel_tree);
	}
    else
	return(UE$ERR_INVPAR);
    }

/*************** PROCESS "t" FLAG *******************************/

if ((tree = get_tree(UE$FLG_t)) != UE$NULL)
    {
    flag_node = UE$NULL;
    if ((parm = tree->parm_list) != UE$NULL)
	{
        do
	    {
	    (void)strcpy(pstring,parm->parm_string);

				/* check for date,time combo */

	    if ((parm->next_parm != UE$NULL) &&
	        (strncmp(parm->next_parm->parm_string,"s:",2) != 0) &&
		(strncmp(parm->next_parm->parm_string,"e:",2) != 0))
		{
		parm = parm->next_parm;
		(void)strcat(pstring,",");
		(void)strcat(pstring,parm->parm_string);
		}
	    if ((datetime = asctotime(pstring+2)) == UE$NULL)
	        return(UE$ERR_INVPAR);
	    if      ((strncmp(pstring,"s:",2)) == 0)
	        pars_node = es$parse(&status, DD$datetime, "ge %d",datetime);
	    else if ((strncmp(pstring,"e:",2)) == 0)
	        pars_node = es$parse(&status, DD$datetime, "le %d",datetime);
	    else 
	        return(UE$ERR_INVPAR);
	    if (status != ES$SUCC)
	        return(UE$ERR_INVPAR);
	    BLD_NODE (pars_node,ES$AND,flag_node);
	    }
        while ((parm = parm->next_parm) != UE$NULL);
        BLD_NODE (flag_node,ES$AND,sel_tree);
	}
    else
	return(UE$ERR_INVPAR);
    }

if (sel_tree != UE$NULL)
    es$select(&uctx,ES$REMEMBER,sel_tree);
    
/****************************************************************/
return(UE$SUCC);
}
/*...	ENDFUNCTION valid_gen					*/



/****************************************************************/
/********** GET_DEVTYPE *****************************************/
/****************************************************************/

SELNODE *get_devtype(label,status)

char *label;
long  *status;

{
DD$STD_ITEMS_DSD_PTR	find_std_item_dsd();
DD$STD_CODES_DSD_PTR	find_std_code_dsd();
char			*dsd_get_label();

DD$STD_ITEMS_DSD_PTR	std_item_dsd_ptr;
DD$STD_CODES_DSD_PTR	std_code_dsd_ptr;

short item_cnt;
short next_code;

SELNODE *mult_node;
SELNODE *one_node;
short i, j;
short len;
long  stat;
long  pcode;
short len_label;
char  *code_label_ptr;

char  code_label[UE$XFF];
char  item[UE$XFF];

mult_node = one_node = UE$NULL;

len = strlen(label);
if (len < 2)
    {
    *status = UE$ERR_INVPAR;
    return (one_node);
    }
for (i = 0; i < len; i++)
    item[i] = toupper(label[i]);

if ((std_item_dsd_ptr = find_std_item_dsd(DD$devtype)) == DD$UNKNOWN_ITEM)
    {
    *status = UE$ERR_DEVTBL;
    return (mult_node);
    }
item_cnt = std_item_dsd_ptr->COUNT;
for (next_code = 0; next_code < item_cnt; next_code++)
    {
    if ((std_code_dsd_ptr = find_std_code_dsd(std_item_dsd_ptr, next_code))
		== DD$UNKNOWN_CODE)
        {
        *status = UE$ERR_DEVTBL;
        return (mult_node);
        }
    code_label_ptr = dsd_get_label(std_code_dsd_ptr->LABEL_IX);
    len_label = strlen(code_label_ptr);
    for (i = 0, j = 0; i < len_label; i++)
	{
	if (code_label_ptr[i] != ' ')
	    code_label[j++] = toupper(code_label_ptr[i]);
	}
    code_label[j] = '\0';
    if ((len >= 4) && (len != j))
	continue;
    if (strncmp(item,code_label,len) == 0)
	{
	pcode = std_code_dsd_ptr->CODE;
	one_node = es$parse(&stat, DD$devtype, "eq %d",pcode);
	if (stat != ES$SUCC)
	    {
	    *status = UE$ERR_INVPAR;
	    return (mult_node);
	    }
	BLD_NODE (one_node,ES$OR,mult_node);
	}
    }
if (mult_node == UE$NULL)
    {
    *status = UE$ERR_INVPAR;
    return(mult_node);
    }
*status = UE$SUCC;
return(mult_node);
}



/*
*	.SBTTL	help - UERF help routine
*++
* FUNCTIONAL DESCRIPTION:		
*
*	-  Displays the file "uerf.hlp"
*	
*	
* FORMAL PARAMETERS:		
*
*
*
*
* IMPLICIT INPUTS:		NONE
*
* IMPLICIT OUTPUTS:		NONE
*
* COMPLETION STATUS:		NONE
*
* SIDE EFFECTS:			NONE
*
*--
*/
/*...	FUNCTION help				*/

help ()
{
FILE *fpt;
char line[UE$XFF];

extern char  search_path[];
char  *find_file();
char  hlp_file[UE$XFF];

printf(UE$HELP_HDR);

strcpy(hlp_file,find_file(UE$HLP_FILE));
if (strlen(hlp_file) == 0)
    {
    return(UE$ERR_NOHLP);
    }

if ((fpt = fopen(hlp_file, "r")) == NULL)
    {
    return(UE$ERR_H_OPEN);
    }
while ((fgets(line,UE$XFF,fpt)) != NULL)
    {
    if ((strncmp(line,"/*",2)) == NULL)
	continue;
    printf("%s",line);
    }
return(UE$SUCC);
}
/*...	ENDFUNCTION help				*/


/*
*	.SBTTL	get_tree - Function to find a branch on parse tree
*++
* FUNCTIONAL DESCRIPTION:		
*
*	-  searches tree structure for branch with particular flag
*
* FORMAL PARAMETERS:		value of the flag (short)
*
* IMPLICIT INPUTS:		none
*
* IMPLICIT OUTPUTS:		none
*
* COMPLETION STATUS:		pointer to tree branch if successful
*				UE$NULL if failure
*
* SIDE EFFECTS:			NONE
*
*--
*/
/*...   FUNCTION get_tree(fl)					*/

struct parse_tree *get_tree(fl)

char  fl;
{
struct parse_tree *tree;

if (in_st.root != UE$NULL)
    {	
    tree = in_st.root;
    while (tree != UE$NULL)		/* go down the chain	*/
	{
	if (tree->flag[0] == fl)
	    return(tree);
	tree = tree->next_tree;
	}
    }
tree = UE$NULL;
return(tree);
}
/*...   ENDFUNCTION get_tree					*/


/*
*	.SBTTL	asctotime - Function to change ascii time to 
*			    unsigned long.
*++
* FUNCTIONAL DESCRIPTION:		
*
*   asctotime() converts an ascii string that contains a
*   date and time into the number of seconds since 31-dec-1969 midnight.
*   This is stored as an unsigned longword integer.
*   The ascii string must be in one of the following formats:
*
*    complete date/time:
*			dd-mmm-yyyy,hh:mm:ss
*    partial input:
*			dd-mmm-yyyy
*			dd-mmm-yyyy,hh:mm
*			dd-mmm-yyyy,hh
*
*			dd-mmm
*			dd-mmm,hh:mm
*			dd-mmm,hh
*
*			dd,hh:mm
*			dd,hh
*
*			hh:mm:ss
*			hh:mm
*			hh
*
*    partial date/time will be or'd with today's date and 00:00:00 time.
*
*
*   where
*	    mmm :  is 3 character month abbreviation (must be lower case)
*
*   The function returns 0 if the date/time is invalid.
*   The year of the date must be between 1970 and 2099 inclusive.
*
*
* FORMAL PARAMETERS:		date and time
*
* IMPLICIT INPUTS:		none
*
* IMPLICIT OUTPUTS:		none
*
* COMPLETION STATUS:		unsigned longword containing seconds
*				since 31 Dec 1969.
*				UE$NULL if failure
*
* SIDE EFFECTS:			NONE
*
*--
*/
/*...   FUNCTION asctotime(date)		*/

#define ISLEAP(y) (((((y)%4) == 0) && (((y)%100) != 0)) || (((y)%400) == 0))

unsigned long asctotime(asc_date)
char *asc_date;

{
struct tm	 *tp;
struct timeval	 tv;
struct timezone  tz;
short i;
unsigned long    t;
unsigned long    parsetime();

gettimeofday(&tv,&tz);
tp = localtime(&(tv.tv_sec));

if (parsetime(asc_date,tp) != UE$SUCC)
    return(UE$NULL);

i =  (tp->tm_year - 70);		/* full years since 70	*/
t =  (i * 365);				/* days in those years	*/
t += ((i + 2) / 4);			/* plus 1 per leap year	*/
if (ISLEAP(tp->tm_year+1900))
    t--;
t += tp->tm_yday;			/* full days this year	*/

t =  (t * 86400) +			/* secs in day	*/
     (tp->tm_hour * 3600) +		/* secs in hour	*/
     (tp->tm_min * 60) +		/* secs in min	*/
     (tp->tm_sec) +
     (tz.tz_minuteswest * 60); 		/* correct GMT	*/
tp = localtime(&t);
t -= (tp->tm_isdst * 3600);		/* correct DST	*/
return(t);

}
/*...   ENDFUNCTION asctotime()			*/


/*
*	.SBTTL	parsetime - Function to convert ascii time to 
*			    its integer components.
*++
* FUNCTIONAL DESCRIPTION:		
*
*
*   parsetime converts an ascii string that contains a
*   date and time into its integer components. 
*   The ascii string must be in one of the following formats:
*
*    complete date/time:
*			dd-mmm-yyyy,hh:mm:ss
*    partial input:
*			dd-mmm-yyyy
*			dd-mmm-yyyy,hh:mm
*			dd-mmm-yyyy,hh
*
*			dd-mmm
*			dd-mmm,hh:mm
*			dd-mmm,hh
*
*			dd,hh:mm
*			dd,hh
*
*			hh:mm:ss
*			hh:mm
*			hh
*
*    partial date/time will be or'd with today's date and 00:00:00 time.
*
*
*   where
*	    mmm :  is 3 character month abbreviation (must be lower case)
*
*   this function will either fill in the "struct tm" structure
*   or return NULL if given an invalid data/time combination
*
*
* FORMAL PARAMETERS:		ascii date and time
*
* IMPLICIT INPUTS:		none
*
* IMPLICIT OUTPUTS:		none
*
* COMPLETION STATUS:		UE$SUCC
*				UE$FAIL
*
* SIDE EFFECTS:			NONE
*
*--
*/
/*...   FUNCTION parsetime(tstr, tp)			*/

unsigned long parsetime(tstr,tp)
char      *tstr;
struct tm *tp;

{
short i;
short len;
short offset;

static char *months[] = { " ",
			  "jan", "feb", "mar", "apr",
			  "may", "jun", "jul", "aug",
			  "sep", "oct", "nov", "dec" };
static short daysinmonth[] = {0,31,28,31,30,31,30,31,31,30,31,30,31};

tp->tm_sec  = 0;
tp->tm_min  = 0;
tp->tm_hour = 0;
tp->tm_mon++;		/* month is stored in struc as 1 less	*/

len = strlen(tstr);
offset = 0;

/***********  dd-mmm-yyyy,hh:mm:ss  parsing  ********************/

i      = strcspn(tstr,",-");		/* split date/time	*/

/***************  dd-mmm-yyyy  parsing  *************************/

if (i < len)
    {					/* dd- or dd, present	*/
    tp->tm_mday = atoi(tstr);
    offset += (i + 1);
    if ((tstr[offset - 1]) != ',')
	{
	for (i = 0; i < 3; i++)
	    tstr[offset+i] = tolower(tstr[offset+i]);
	for (i = 1; i <= 12; i++)
	    {
	    if (strncmp(tstr+offset,months[i],3) == 0)
		break;
	    }
	if (i > 12)
	    return(UE$FAIL);
	tp->tm_mon = i;
	i = strcspn(tstr+offset,",-");
	offset += (i + 1);
	if ((tstr[offset - 1]) == '-')
	    {
	    tp->tm_year = atoi(tstr+offset);
	    if (tp->tm_year > 1900)
		tp->tm_year -= 1900;
	    if (tp->tm_year < 70)
		return(UE$FAIL);
	    i = strcspn(tstr+offset,",");
	    offset += (i + 1);
	    }
	}
    if (ISLEAP(tp->tm_year+1900))
	daysinmonth[2] = 29;
    else
	daysinmonth[2] = 28;
    tp->tm_yday = 0;
    if (tp->tm_mday > daysinmonth[tp->tm_mon])
	return(UE$FAIL);
    for (i = 1; i < tp->tm_mon; i++)
	tp->tm_yday += daysinmonth[i];
    tp->tm_yday += (tp->tm_mday - 1);
    if ((offset) >= len)
	return(UE$SUCC);		/* no time input	*/
    }

/***************  hh:mm:ss  parsing  ****************************/

tp->tm_hour = atoi(tstr+offset);
if (tp->tm_hour >= 24)
    return(UE$FAIL);
i = strcspn(tstr+offset,":");
if ((offset+i) >= len)
    return(UE$SUCC);			/* hh   input		*/
offset += (i + 1);

tp->tm_min = atoi(tstr+offset);
if (tp->tm_min >= 60)
    return(UE$FAIL);
i = strcspn(tstr+offset,":");
if ((offset+i) >= len)
    return(UE$SUCC);			/* hh:mm   input	*/
offset += (i + 1);

tp->tm_sec = atoi(tstr+offset);
if (tp->tm_sec >= 60)
    return(UE$FAIL);

return(UE$SUCC);			/* hh:mm:ss   input	*/
}

