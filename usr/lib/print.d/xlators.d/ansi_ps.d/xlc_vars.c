#ifndef lint
static char SccsId[] = "  @(#)xlc_vars.c	4.1   LPS_ULT_TRN   7/2/90";
#endif
/* file:	xlc_vars.c - Variables for Translator
 * created:	araj	12-AUG-1988 13:56
 * edit:
 *		cp	20-MAR-1989
 *		Included files xlc_font_metrics.hc and
 *		xlc_font_metrics.vc for Ultrix port.
 *
 *
 *
 */

/************************************************************************
 *                                                                      *
 *      COPYRIGHT  (c)  DIGITAL  EQUIPMENT CORPORATION 1985,            *
 *            1986, 1987, 1988, 1989 .   ALL RIGHTS RESERVED.           *
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


/* 
 *  Include files
 */
#include "portab.h"
#include "capdl.hc"
#include "xlate.h"
#include "xlc_codegen.hc"
#include "xlc_codegen.vc"
#include "xlc_graph.hc"
#include "xlc_graph.vc"
#include "xlc_font_dictionary.hc"
#include "xlc_font_dictionary.vc"
#include "xlc_font_metrics.hc"
#include "xlc_font_metrics.vc"
#include "xlc_debug.hc"
#include "xlc_debug.vc"
#include "xlc_ps.hc"
#include "xlc_ps.vc"
#include "xlc_dll.hc"
#include "xlc_dll.vc"
#include "xlc_main.hc"
#include "xlc_main.vc"
