#ifndef lint
static char SccsId[]="@(#)laps_msg.c	4.1\tLPS_ULT_DNU\t7/2/90";
#endif
/*
 *    laps_msg.c    --  Parse the LAPS messages received from PrintServer
 *    
 *    The VMS lps message file convertion to  lpsmsg_string.h is
 *    fully automated.
 *   
 *    Ajay Kachrani - June 21, 1988 
 *
 *****************************************************************************
 *									     *
 *  COPYRIGHT (c) 1986, 1987, 1988       				     *
 *  By DIGITAL EQUIPMENT CORPORATION, Maynard, Mass.			     *
 *									     *
 *  This software is furnished under a license and may be used and  copied   *
 *  only  in  accordance  with  the  terms  of  such  license and with the   *
 *  inclusion of the above copyright notice.  This software or  any  other   *
 *  copies  thereof may not be provided or otherwise made available to any   *
 *  other person.  No title to and ownership of  the  software  is  hereby   *
 *  transferred.      							     *
 *									     *
 *  The information in this software is subject to change  without  notice   *
 *  and  should  not  be  construed  as  a commitment by Digital Equipment   *
 *  Corporation.							     *
 *									     *
 *  Digital assumes no responsibility for the use or  reliability  of  its   *
 *   software on equipment which is not supplied by Digital.		     *
 *									     *
 *****************************************************************************
 *
 */

#include <stdio.h>		/* standard i/o library definitions */
#include "laps.h"		/* include laps defintions */
#include "lpsmsg.h"		/* include error codes */
#include "lpsmsg_string.h"      /* include message string-struct definitions */
#include "descrip.h"		/* VMS descriptor definitions */

#define MAX_MSG_PARAMS	3	/* maximum number of arguments in a
				 * condition record */


/*
 *		p r o c _ c o n d _ r e c
 *
 * This routine is called to process a condition record.
 *
 * Returns:		NULL on success
 *			EOF on error
 *
 * Inputs:
 *	rec		= Address of condition record
 *	reclen		= Length of record
 */
proc_cond_rec(rec, reclen)
FIELD8 *rec;
int reclen;
{
	char notify_buf[MAX_COND_REC_SIZE];
	char text_buf[MAX_COND_REC_SIZE];
	char msgbuf[MAX_COND_REC_SIZE];
	char param_buf[MAX_COND_REC_SIZE * MAX_MSG_PARAMS];
	extern int laps_abort;
	extern int (*cond_handler)();

        union
	{
             char *c[MAX_MSG_PARAMS];
             unsigned long l[MAX_MSG_PARAMS];
	} param_ptr;

	struct cond_hdr *condp;
	struct dsc$descriptor *descp;
	struct laps_msg *mp;
	unsigned long msg_code;
	int i;
	char *cp;

	/*
	 * process the record header
	 */
	condp = (struct cond_hdr *)rec;
	switch (condp->act_code)
	{
		case LAPS_ACTSTOPTASK:
		case LAPS_ACTSTOPJOB:
		case LAPS_ACTABORT:
			laps_abort = 1;
			break;
		default:
			break;
	}
	/*
	 * build pointers to the arguments - format ascii strings as null
	 * terminated.
	 */
	descp = (struct dsc$descriptor *)(rec + sizeof(struct cond_hdr));
	for (i = 0; i < condp->argc; i++)
	{
		switch (descp->dsc$b_dtype)
		{
			case DSC$K_DTYPE_T:
		                param_ptr.c[i] = (char *)(rec + (int)descp->dsc$a_pointer);
				switch (descp->dsc$b_class)
				{
					case DSC$K_CLASS_S:
					cp = &param_buf[i * MAX_COND_REC_SIZE];
					bcopy(param_ptr.c[i], cp, descp->dsc$w_length);
					param_ptr.c[i] = cp;
					*(cp + descp->dsc$w_length) = NULL;
						break;
					default:
						/*
						 * should report an error here - seems
						 * like a lot of  trouble
						 */
						break;
				}
				break;
			case DSC$K_DTYPE_Z:
			case DSC$K_DTYPE_LU:
			case DSC$K_DTYPE_L:
				param_ptr.l[i] =
				*(unsigned long *)(rec + (int)descp->dsc$a_pointer);
				/*
				 * these are all we expect - nothing special to do though
				 */
				break;
			default:
				/*
				 * should report an error here - seems like a lot of  trouble
				 */
				break;
		}
		/*
		 * point to next descriptor
		 */
		cp = (char *)descp + sizeof(struct dsc$descriptor) + descp->dsc$w_length;
		descp = (struct dsc$descriptor *)cp;
	}
	/*
	 * walk through error descriptors, looking for a match on the error code
	 */
	notify_buf[0] = NULL;
	msg_code = condp->msg_code & ~SEVERITY_BITS; /* strip off severity */
	for (mp = laps_msgs; mp < (laps_msgs + sizeof(laps_msgs)); mp ++)
	{
		if (msg_code == (mp->code & ~SEVERITY_BITS))
		{
			/*
			 * error code matched - format the message and notify the user
			 */
			sprintf(msgbuf, "%s %s", severity_strings[mp->severity], mp->text);
   		        sprintf(notify_buf, msgbuf, param_ptr.c[0], param_ptr.c[1], param_ptr.c[2]);
			break;
		}
	}
	if (notify_buf[0] == NULL)
	{
		/*
		 * error code not found - format a message with the numeric code
		 */
		sprintf(notify_buf, "-?- unknown error reported (%u)", condp->msg_code);
	}
        /*
         *  check if condition record has text from PostScript
         */
         text_buf[0] = NULL;
         if ( ((condp->argc - mp->FAOcount) > 0) &&
              (condp->argc <= MAX_MSG_PARAMS) )

                sprintf (text_buf, "%s", param_ptr.c[mp->FAOcount]);
               
	/*
	 * call the user's condition handler (if one has been set)
	 */
	if (cond_handler != NULL)
	{
		(*cond_handler)(condp->routing_info, notify_buf, strlen(notify_buf));
                if (text_buf[0] != NULL)
		      (*cond_handler)(condp->routing_info, text_buf, strlen(text_buf));
	}
	return;
}
