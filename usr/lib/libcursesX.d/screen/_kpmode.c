#ifdef lint
static char *sccsid = "@(#)_kpmode.c	4.1	(ULTRIX)	7/2/90";
#endif lint


#include "curses.ext"

extern	int	_outch();

_kpmode(m)
{
#ifdef DEBUG
	if (outf) fprintf(outf, "kpmode(%d), SP->kp_state %d\n",
	m, SP->kp_state);
#endif
	if (m == SP->kp_state)
		return;
	if (m)
		tputs(keypad_xmit, 1, _outch);
	else
		tputs(keypad_local, 1, _outch);
	SP->kp_state = m;
}
