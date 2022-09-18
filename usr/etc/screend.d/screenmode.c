#ifndef lint
static char *sccsid = "@(#)screenmode.c	4.2	(ULTRIX)	10/12/90";
#endif
/* screenmode.c								*/

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

	if (argc > 2) {
		fprintf(stderr, "usage: screenmode [on | off]\n");
		exit(1);
	}
	s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s < 0) {
		perror("screenmode: socket");
		exit(1);
	}
	if (argc == 2) {
		if (strcmp(argv[1], "on") == 0)
			mode = SCREENMODE_ON;
		else if (strcmp(argv[1], "off") == 0)
			mode = SCREENMODE_OFF;
		else {
			fprintf(stderr, "screenmode: `%s' is not legal\n",
				argv[1]);
			exit(1);
		}
	}
	else {
		mode = SCREENMODE_NOCHANGE;
	}
	if (ioctl(s, SIOCSCREENON, (caddr_t)&mode) < 0) {
		perror("ioctl (SIOCSCREENON)");
		exit(1);
	}
	if (argc < 2) {
	    printf("screening is %s\n",
			(mode == SCREENMODE_ON) ? "on" : "off");
	}
}
