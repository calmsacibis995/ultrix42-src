/*  sccsid  =  @(#)select.h	4.1   (ULTRIX)   7/2/90  */

/*
*       .TITLE SELECT.H - SELECT typedefs
*	.IDENT	/1-001/
*++
*  ABSTRACT:
*	This module contains SELECT specific macros and typdefs.
*	It is used by all of the SELECT functions and by modules that
*       call SELECT functions.
*
*  AUTHOR: Don Zaremba  
*
*  TABLE OF CONTENTS
*
*   SELECT function declarations
*   SELECT_TREE typdef
*   ARGLIST typedef
*--
 */

/* Now declare this header */
#ifndef SELECT_H
#define SELECT_H


/*
*	.SBTTL	arglist - Selection argument list
*++
* STRUCTURE DESCRIPTION:		
*
*   This node is used to store a list of aruments. Each argument is
*   a symbol pointer. 
*	
* STRUCTURE TYPE:	    arglist
*	
* ACCESS METHODS:		
*
* SIDE EFFECTS:
*
*--
*/
/*...	STRUCTURE arglist					    */
/*++		*/
typedef struct arglist
	{
	short fldno ; /* EIMS field number */ 
	short vartype; /* EIMS data type */ 
	char *varaddress; 
	struct arglist *next;
        } ARGLIST
	; 
 
/*--	*/
/*...	ENDSTRUCTURE arglist					    */


/*
*	.SBTTL	SELECT_TREE - EIMS selection tree
*++
* STRUCTURE DESCRIPTION:		
*
*	The ERMS selection tree contains the data selection criteria
*	used in the data selection routines of ERMS and the validation
*       routines. This structure is a binary tree.
*	A node can be used to either link selection criteria together
*	with a conjuction or they contain an operator node.
*	    The EQUAL operator takes 2 or more operands.
*	    The INEQUAL operator takes exactly 2 operands
*	    The RANGE operator takes exactly 3 operands.
*	
* STRUCTURE TYPE:		select_tree
*	
* ACCESS METHODS:   See descriptions of the modules
*		es$mkselectnode, es$mkarglist, es$apparglist
*
*
*--
*/
/*...	STRUCTURE SELECT_TREE 						    */
/*									    */
/*++		*/
typedef struct selectnode
	{
	short operator; /* ES$AND ES$OR ES$NOT ES$EQUAL ES$RANGE ES$LT ES$LE ES$GT ES$GE */
	struct selectnode *parent;
	struct selectnode *left;
	struct selectnode *right;
	ARGLIST *operands; } SELNODE ;
/*--	*/
/*...	ENDSTRUCTURE SELECT_TREE					    */

#endif SELECT_H
