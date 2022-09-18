#ifndef lint
static char SccsId[]="@(#)net_ultrix.c	4.1\tLPS_ULT_DNU\t7/2/90";
#endif
/*
 *    net_ultrix.c	Ultrix specific network code
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
 *
 * EDIT HISTORY
 *--------------
 *
 * V0.002  28-Jun-1988   APK add interrupt_ps routine to send OOB message
 * V0.001  31-May-1988   APK add nonblock_io routine
 *
 */
#include <sys/ioctl.h>		/* i/o control definitions */
#include <sys/types.h>          /* type definition used by socket.h */
#include <sys/socket.h>         /* socket definition */
#define LAPS_OBJECT "#50"	/* string to describe LAPS object */

/*
 *		c o n n _ p s
 *
 * This routine is called to create a DECnet connection to a print
 * server.
 *
 * Returns:		socket to be used for connection,
 *			-1 on error
 *
 * Inputs:
 *	node		= Pointer to node name string
 */
conn_ps(node)
char *node;
{
	return(dnet_conn(node, LAPS_OBJECT, 0, 0, 0, 0, 0));
}

/*
 *		r e a d _ p s
 *
 * This routine is called to read a message from a print server.
 *
 * Returns:		length of message read,
 *			-1 on error
 *
 * Inputs:
 *	sock		= socket for connection
 *	buf		= buffer for message
 *	len		= length of buffer
 */
read_ps(sock, buf, len)
int sock;
char *buf;
int len;
{
	return(read(sock, buf, len));
}

/*
 *		w r i t e _ p s
 *
 * This routine is called to write a message to a print server.
 *
 * Returns:		length of message written,
 *			-1 on error
 *
 * Inputs:
 *	sock		= socket for connection
 *	msg		= message to be written
 *	len		= length of message
 */
write_ps(sock, msg, len)
int sock;
char *msg;
int len;
{
	return(write(sock, msg, len));
}

/*
 *              i n t e r r u p t _ p s 
 *
 * This routine is called to send a OOB message to a print server
 *
 * Returns:            length of message written,
 *                     -1 on error
 *
 * Inputs:
 *      sock           = socket for connection
 *      msg            = message to be send
 *      len            = length of message
 */
interrupt_ps(sock, msg, len)
int sock;
char *msg;
int len;
{
        return(send(sock, msg, len, MSG_OOB));
}

/*
 *		discon_ps
 *
 * This routine is called to disconnect from a print server.
 *
 * Returns:		0 on success
 *			-1 on error
 *
 * Inputs:
 *	sock		= socket used for connection
 */
disconn_ps(sock)
int sock;
{
	return(close(sock));
}

/*
 *		p o l l _ m s g
 *
 * This routine is called to see if a message is waiting for us to
 * read it.
 *
 * Returns:		0 if no message is waiting
 *			non-0 if message is waiting
 *
 * Inputs:
 *	sock		= socket for connection being polled
 */
poll_msg(sock)
int sock;
{
	int read_flag;

	if ((ioctl(sock, FIONREAD, &read_flag) == 0) && (read_flag != 0))
		return(1);
	else
		return(0);
}

/*
 *   n o n b l o c k _  i o 
 *
 * This routine is called to turn the nonblocking on or off.
 *
 * Returns:         0 on success
 *                 -1 on error
 *
 * Inputs:
 *      sock      = socket used to mark (non)blocking.
 *      sts_flag  = 1 for blocking, 0 for nonblocking
 */
nonblock_io(sock, sts_flag)
int sock;
int sts_flag;
{ 
    return (ioctl(sock, FIONBIO, &sts_flag));
}

