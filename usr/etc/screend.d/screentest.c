#ifndef lint
static char *sccsid = "@(#)screentest.c	4.2	(ULTRIX)	10/12/90";
#endif
/* screentest.c								*/


/************************************************************************
 *			Modification History				*
 *									*
 *	19 December 1988	Jeffrey Mogul/DECWRL			*
 *		Created (mostly from ifconfig.c)			*
 *									*
 ************************************************************************/
/*
 *               Copyright 1989, 1990 Digital Equipment Corporation
 *                          All Rights Reserved
 * 
 * 
 * Permission to use, copy, and modify this software and its documentation
 * is hereby granted only under the following terms and conditions.  Both
 * the above copyright notice and this permission notice must appear in
 * all copies of the software, derivative works or modified versions, and
 * any portions threof, and both notices must appear in supporting
 * documentation.
 * 
 * Users of this software agree to the terms and conditions set forth herein,
 * and hereby grant back to Digital a non-exclusive, unrestricted, royalty-free
 * right and license under any changes, enhancements or extensions made to the
 * core functions of the software, including but not limited to those affording
 * compatibility with other hardware or software environments, but excluding
 * applications which incorporate this software.  Users further agree to use
 * their best efforts to return to Digital any such changes, enhancements or
 * extensions that they make and inform Digital of noteworthy uses of this
 * software.  Correspondence should be provided to Digital at:
 * 
 *                       Director of Licensing
 *                       Western Research Laboratory
 *                       Digital Equipment Corporation
 *                       100 Hamilton Avenue
 *                       Palo Alto, California  94301  
 * 
 * Comments and bug reports may also be sent using electronic mail to:
 * 			screend-reports@decwrl.dec.com
 * 
 * 	>> This software may NOT be distributed to third parties. <<
 *   
 * THE SOFTWARE IS PROVIDED "AS IS" AND DIGITAL EQUIPMENT CORP. DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS.   IN NO EVENT SHALL DIGITAL
 * EQUIPMENT CORPORATION BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <net/if.h>
#include <netinet/in.h>
#include <net/gw_screen.h>

#include <stdio.h>

struct	screen_data sdat;

main(argc, argv)
int argc;
char **argv;
{
	int s;
	int mode;
	struct screen_data scdata;
	int cointoss = 0;

	if (argc != 1) {
		fprintf(stderr, "usage: screentest\n");
		exit(1);
	}
	s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s < 0) {
		perror("screentest: socket");
		exit(1);
	}

	scdata.sd_xid = 0;

	while (1) {
	    scdata.sd_family = AF_UNSPEC;
	    if (ioctl(s, SIOCSCREEN, (caddr_t)&scdata) < 0) {
		perror("ioctl (SIOCSCREEN)");
		exit(1);
	    }
	    cointoss++;
	    if (cointoss & 1)
		scdata.sd_action = SCREEN_ACCEPT;
	    else if (cointoss & 2)
		scdata.sd_action = SCREEN_NOTIFY;
	    PrintScreenData(&scdata);
	    printf("\n");
	}
}

PrintScreenData(sdp)
register struct screen_data *sdp;
{
	printf("af %d count %d dlen %d xid %x action %x",
		sdp->sd_family,
		sdp->sd_count, sdp->sd_dlen, sdp->sd_xid, sdp->sd_action);
	if (sdp->sd_action & SCREEN_ACCEPT)
		printf(" ACCEPT");
	else
		printf(" REJECT");
	if (sdp->sd_action & SCREEN_NOTIFY)
		printf(" NOTIFY");
	printf("\n");

	PrintIPHeader(&(sdp->sd_arrival), sdp->sd_data, sdp->sd_dlen);
}

yyerror(s)
char *s;
{
	fflush(stdout);
	fprintf(stderr, "%s\n", s);
}

/* need this here so we can link without parser code */
yywarn(s)
char *s;
{
	fflush(stdout);
	fprintf(stderr, "Warning: %s\n", s);
}
