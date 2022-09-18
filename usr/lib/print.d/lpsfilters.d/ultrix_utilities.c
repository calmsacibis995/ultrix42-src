#ifndef lint
static char sccsid[] = "@(#)ultrix_utilities.c	4.1      LPS_ULT_IP 	11/15/90";
#endif
/*
 *          ultrix_utilities
 *
 * Author: John W.F. McClain
 * Date: 11-July-1990
 * 
 * History 11-July-1990 jwfm Initial creation
 *
 * This module contains random utility functions, in general the functions in
 * this module map to functions of the same name in vms_utilities.
 *
 */

#include <stdio.h>
#include <strings.h>
#include <sys/file.h>

/*
 *             build_path
 *
 * Given a ULTRIX directory name, a ULTRIX like path path specification,
 * build_path returns a ULTRIX file name.  Any leading '/'s in the path spec.
 * are ignored, as are any trailing '/' in path or root. 
 *
 * Inputs       root          ULTRIX directory name.
 *              path          ULTRIX like path.
 * Outputs      return_str    string result is retured in, must already have
 *                            space allocated to it.
 * Returns      return_str
 * Side effects none
 *
 */
char *build_path(return_str, root, path)
char *return_str, *root, *path;
{
  char *ptr;

  while (*path == '/' && *path != 0)           /* get rid of leading '/' */
    path++;

  while (ptr=rindex(path, '/'))                /* get rid of trialing '/' */
    if (*(ptr + 1) == 0) *ptr = 0;
    else break;

  while (ptr=rindex(root, '/'))                /* get rid of trialing '/' */
    if (*(ptr + 1) == 0) *ptr = 0;
    else break;
    
  sprintf(return_str, "%s/%s", root, path);

  return(return_str);
}

/*
 *             unv_open
 *
 * Takes the four arguments, the first 3 are the std. ULTRIX open args, these
 * are pased to open, the fourth arg is ignored.
 *
 * Inputs       path          file name.
 *              flags         how the file is to be opend. (for reading, etc.)
 *              mode          protection info
 *              ignored       str with type info, ignored.
 * Outputs      none
 * returns      file descriptor
 *
 */

int unv_open(path, flags, mode, ignored)
char *path, *ignored;
int flags, mode;
{
  return(open(path, flags, mode));
}

