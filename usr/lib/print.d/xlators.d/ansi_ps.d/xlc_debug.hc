/*        @(#)xlc_debug.hc	4.1      7/2/90      */
/* file:    xlc_debug.h- include file for external references to 
 *	    Variable declarations needed by MAKETEST for error
 *	    mesages. And error handler used for debug.
 *
 *	    This is a temporary file, until tools can handle long strings
 *
 * edit:	araj	18-AUG-1988 14:57
 *			creation 
 *
 */

/************************************************************************
 *                                                                      *
 *      COPYRIGHT  (c)  DIGITAL  EQUIPMENT CORPORATION 1985,            *
 *            1986.   ALL RIGHTS RESERVED.                              *
 *                                                                      *
 *      THIS SOFTWARE IS FURNISHED UNDER A LICENSE AND MAY BE           *
 *      USED AND COPIED ONLY IN ACCORDANCE  WITH THE TERMS OF           *
 *      SUCH  LICENSE  AND  WITH  THE  INCLUSION OF THE ABOVE           *
 *      COPYRIGHT  NOTICE.  THIS SOFTWARE OR ANY OTHER COPIES           *
 *      THEREOF   MAY  NOT  BE  PROVIDED  OR  OTHERWISE  MADE           *
 *      AVAILABLE  TO  ANY  OTHER  PERSON.  NO  TITLE  TO AND           *
 *      OWNERSHIP  OF  THE  SOFTWARE  IS  HEREBY TRANSFERRED.           *
 *                                                                      *
 *      THE INFORMATION IN THIS SOFTWARE IS SUBJECT TO CHANGE           *
 *      WITHOUT  NOTICE  AND SHOULD  NOT BE  CONSTRUED  AS A            *
 *      COMMITMENT  BY  DIGITAL EQUIPMENT CORPORATION.                  *
 *                                                                      *
 *      DIGITAL  ASSUMES  NO RESPONSIBILITY  FOR THE  USE  OR           *
 *      RELIABILITY  OF ITS SOFTWARE ON EQUIPMENT THAT IS NOT           *
 *      SUPPLIED BY DIGITAL.                                            *
 ************************************************************************/



CONST EXTERNAL  BYTE *type_of_error_condition [2] ;
CONST EXTERNAL  BYTE str_ps_vm_snapshot_1 [];
CONST EXTERNAL  BYTE str_ps_vm_snapshot_2 [];
CONST EXTERNAL  BYTE str_ps_vm_snapshot_3 [];
CONST EXTERNAL  BYTE str_ps_vm_snapshot_4 [];
CONST EXTERNAL  BYTE str_error_handler [];
