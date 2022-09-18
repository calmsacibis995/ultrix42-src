#ifndef lint
static char sccsid[]  =  "@(#)erisel.c	4.2   (ULTRIX)   9/11/90"; 
#endif  lint

/*
*	.TITLE	ERISEL - Checks selection trees for later use
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
* FACILITY:		FMA - Event Information Management System
*
* ABSTRACT:
*
*	This module checks all selection trees created by the user
*	interface and determines whether to use them in later processing.
*	
* ENVIRONMENT:	VAX/VMS C,  ULTRIX-32 C
*
* AUTHOR:  Bob Winant,  CREATION DATE:  3-Jun-1986
*
* MODIFIED BY:
*
*
*--
*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/errlog.h>
#include "eiliterals.h"
#include "eims.h"
#include "select.h"
#include "generic_dsd.h"
#include "std_dsd.h"

/*
*++
*=
*=
*=MODULE FLOW - erisel.c
*=
*=  a - ei$sel(option,tree)                         Separates selection nodes
*=                                                  erms_tree (nodes for ERMS)
*=                                                  erit_tree (nodes for ERIT)
*=                                                  date/time nodes
*=          if(ES$FORGET)
*=              deletetree(trees)                           (select.c)
*=          val_sel(tree)                                   (* b)
*=          es$mkselectnode(ES$AND,ERIT_tree,tree,NULL)     (select.c)
*=          return(ERMS_tree)
*=
*=  b - val_sel(node)                               Creates new ERIT_tree and
*=                                                  ERMS_tree and separates
*=                                                  the nodes.
*=          get_item_info(node)                             (* c)
*=          val_sel(right)                                  (* b)
*=          val_sel(left)                                   (* b)
*=          es$mkselectnode(oper,tree,node,NULL)            (select.c)
*=          return(NULL | node_owner)
*=
*=  c - get_item_info(node)                         Fills varaddr for
*=                                                  erit_nodes. Saves addr of
*=                                                  date nodes. Decides node
*=                                                  ownership.
*=          get_buff_addr()                                 (ulfile.c)
*=          return(ERMS | EIMS)
*=
*=  d - check_selection()                           Checks for meeting of
*=                                                  selection criteria.
*=          es$eval(tree)                                   (select.c)
*=          return(ES$EOF | ES$SUCC | ES$FAIL)
*=
*
*--
*/




/*******************  MODULE WIDE DEFINITIONS  ***************/

#define  ERIT	1		/*  bin  0001  */
#define  ERMS	2		/*  bin  0010  */

/********************  MACRO to link nodes  ******************/

#define  LINK_NODE(flag,owner,xnode,owner_tree) \
	if(flag == owner) \
	    { \
	    xnode->parent = NULL; \
	    if(owner_tree == NULL) \
		owner_tree = xnode; \
	    else \
		owner_tree = es$mkselectnode(node->operator, \
					owner_tree, \
					xnode,NULL); \
	    }

/**********************  SELECTION TREES  *******************/

static SELNODE *ERIT_tree;
static SELNODE *ERMS_tree;
SELNODE *startree;
SELNODE *endtree;

/**********************  FUNCTION DEFINITIONS  **************/

extern SELNODE *es$mkselectnode();
extern long deletetree();
short val_sel();
short get_item_info();
extern struct el_rec *get_buff_addr();



/*
*	.SBTTL	EI$SEL - Removes selection nodes that ERIT will process
*			 from the tree it receives.
*++
* FUNCTIONAL DESCRIPTION:		
*
*	This routine receives selection trees from ES$SELECT and
*	removes those nodes that ERIT will use to select raw records.
*	A tree is then returned for ERMS to select standard segments.
*	
* CALLING SEQUENCE:		CALL EI$SEL (..See Below..)
*					Called from ES$SELECT with the
*					pointer to a selection tree
*					built by the top level caller
*					and the selection option desired
*					by the caller
*
* FORMAL PARAMETERS:		
*
*	treeptr			Address of the selection tree
*
*	option			What to do with this info
*
* IMPLICIT INPUTS:		NONE
*
* IMPLICIT OUTPUTS:		NONE
*
* ROUTINE VALUE:		treeptr - Tree with only ERMS selections.
*
* SIDE EFFECTS:			Strips off selection nodes to be used
*				by ERIT and builds an ERIT_tree.
*				It also saves the date/time nodes.
*				Returns a tree that ERMS will use for
*				selection on the standard record.
*--
*/

/*...	ROUTINE EI$SEL (option, treeptr)		*/
SELNODE *ei$sel(option, treeptr)

SELNODE *treeptr;
long option;
{

short tree_owner;

ERMS_tree = NULL;

switch (option)
    {
    case ES$FORGET:
	deletetree (ERIT_tree);
	deletetree (startree);
	deletetree (endtree);
	break;
    case ES$NOT:
	break;
    case ES$REMEMBER:
	break;
    default:
	return (treeptr);
    }
tree_owner = val_sel(treeptr);

if (tree_owner == ERMS)			/* ERMS ownes the whole tree */
    return (treeptr);

if (tree_owner == ERIT)			/* ERIT ownes the whole tree */
    {
    if (ERIT_tree == NULL)
	ERIT_tree = treeptr;
    else
	ERIT_tree =  es$mkselectnode(ES$AND,ERIT_tree,treeptr,NULL);
    }

					/* tree split to ERMS & ERIT */
if (option == ES$NOT)
    ERIT_tree =  es$mkselectnode(ES$NOT,ERIT_tree,NULL,NULL);

return (ERMS_tree);
}

/*...   ENDROUTINE EI$SEL ()				*/



/*
*	.SBTTL	VAL_SEL - Determines selection validity
*++
* FUNCTIONAL DESCRIPTION:		
*
*	This is a recursive routine which checks each node in the
*	selection tree and separates those items that are to be
*	handled by ERIT.  An ERMS_tree is created that contains the
*	leftover nodes.
*	
* CALLING SEQUENCE:		CALL VAL_SEL (..See Below..)
*					1st call from EI$SEL and then
*					recursively from itself.
*
* FORMAL PARAMETERS:		
*
*	node			Address of the selection node
*
* IMPLICIT INPUTS:		NONE
*
* IMPLICIT OUTPUTS:		NONE
*
* ROUTINE VALUE:		ERMS - If node is for ERMS.
*				ERIT - If node is for ERIT.
*				NULL - If node is for neither or both.
*				
* SIDE EFFECTS:			builds ERIT_node and ERMS_node.
*
*--
*/

/*...	ROUTINE VAL_SEL (node)				*/
short val_sel(node)

SELNODE *node;
{

short left_flag;
short right_flag;

if (node == NULL)
    return (NULL);

if (node->operands != NULL)			/* item node	*/
    return (get_item_info(node));

right_flag  = val_sel(node->right);

left_flag   = val_sel(node->left);

if ((right_flag == ERMS) && (left_flag == ERMS))	/* both ERMS  */
    return (ERMS);

if ((right_flag == ERIT) && (left_flag == ERIT))	/* both ERIT  */
    return (ERIT);

LINK_NODE (right_flag, ERMS, node->right, ERMS_tree);
LINK_NODE ( left_flag, ERMS, node->left , ERMS_tree);

LINK_NODE (right_flag, ERIT, node->right, ERIT_tree);
LINK_NODE ( left_flag, ERIT, node->left , ERIT_tree);

if (node->operator == ES$NOT)
    {
    if (ERIT_tree != NULL)
        ERIT_tree = es$mkselectnode(ES$NOT,ERIT_tree,NULL,NULL);
    ERMS_tree = es$mkselectnode(ES$NOT,ERMS_tree,NULL,NULL);
    }

return (NULL);
}



/*
*	.SBTTL	GET_ITEM_INFO - Gets address of item in raw record
*				for later selection.  It also saves
*				addresses of date/time nodes.
*
*++
* FUNCTIONAL DESCRIPTION:		
*
*	This routine:
*	- Fills in the varaddress portion of the selection tree
*	  for those items that ERIT will do selection on.
*	- Saves the address of the start and end date/time nodes.
*	- Returns whether ERIT or ERMS is owner of the node.
*	
* CALLING SEQUENCE:		CALL GET_ITEM_INFO (..See Below..)
*					Called from VAL_SEL with the
*					pointer to a selection node
*					built by the top level caller
*
* FORMAL PARAMETERS:		
*
*	node			Address of the selection node
*
* IMPLICIT INPUTS:		NONE
*
* IMPLICIT OUTPUTS:		NONE
*
* ROUTINE VALUE:		ERIT if the node is for ERIT_tree.
*				ERMS if the node is for ERMS_tree.
*				NULL if date/time node.
*
* SIDE EFFECTS:			NONE
*
*--
*/

/*...	ROUTINE GET_ITEM_INFO (node)					*/
short get_item_info(node)
SELNODE *node;

{

ARGLIST *argptr;
static char *hname_addr;
struct el_rec *buff_addr;
short type;

type = 0;

buff_addr = get_buff_addr();		/* 1st get buff address  */

switch (node->operands->fldno)
    {
    case DD$eventtype:
	node->operands->varaddress = (char *)&buff_addr->elsubid.subid_class;
	type = EM$SHORT;
	break;
    case DD$recordnumber:
	type = EM$SHORT;
	node->operands->varaddress = (char *)&buff_addr->elrhdr.rhdr_seqnum;
	break;
    case DD$datetime:
	node->operands->varaddress = (char *)&buff_addr->elrhdr.rhdr_time;
	if (node->operator == ES$GE)
	    startree = node;
	if (node->operator == ES$LE)
	    endtree  = node;
	return (NULL);
	break;
    case DD$hostname:
	hname_addr = (char *)buff_addr->elrhdr.rhdr_hname;
	node->operands->varaddress = (char *)&hname_addr;
	break;
    default:
	return (ERMS);			/* ERMS NODE */
	break;
    }
if (type > 0)
    {
    argptr = node->operands;
    do
        argptr->vartype = type;
    while ((argptr = argptr->next) != NULL);
    }
return(ERIT);				/* ERIT NODE */
}


/*
*	.SBTTL CHECK_SELECTION - validates record according to selection
*++
* FUNCTIONAL DESCRIPTION:		
*
*	This routine checks the record data against the ERIT_tree and
*	the date/time trees (startree, endtree) to determine if it
*	fits within the selection criteria.
*	
* CALLING SEQUENCE:		CALL CHECK_SELECTION (..See Below..)
*					Called from EI$GET to determine
*					validity of record according to
*					ERIT's selection criteria (if any)
*
* FORMAL PARAMETERS:		NONE
*
*
* IMPLICIT INPUTS:		A raw data record
*
* IMPLICIT OUTPUTS:		NONE
*
* ROUTINE VALUE:		ES$EOF  - If past date/time selection.
*				ES$SUCC - if selection evaluation is good
*				ES$FAIL - if selection criteria not met.
*
*
* SIDE EFFECTS:			NONE
*
*--
*/

/*...	ROUTINE CHECK_SELECTION ()					*/
long check_selection()
{

extern short reverse_flag;
short status;

status = ES$SUCC;

if (endtree != NULL)
    if ((status = es$eval(endtree)) != ES$SUCC) /* after end date    */
	if (reverse_flag != EI$TRUE)		/* and forward read  */
	    return (ES$EOF);			/* then EOF	     */
	else
	    return (status);

if (startree != NULL)
    if ((status = es$eval(startree)) != ES$SUCC) /*before start date */
	if (reverse_flag == EI$TRUE)		/* and reverse read  */
	    return (ES$EOF);			/* then EOF	     */
	else
	    return (status);

if (ERIT_tree != NULL)
    status = es$eval(ERIT_tree);
return (status);
}
