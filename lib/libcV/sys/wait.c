/*
	wait -- system call emulation for 4.2BSD or BRL PDP-11 UNIX

	last edit:	18-Sep-1983	D A Gwyn
*/

#include	<errno.h>
#include	<signal.h>

extern int	_wait();

int
wait( stat_loc )
	int	*stat_loc;		/* where to put status */
	{
	register void	(*sig)();	/* entry SIGCLD state */

	if ( (sig = signal( SIGCLD, SIG_IGN )) == SIG_IGN )
		{
		while ( _wait( stat_loc ) != -1 || errno != ECHILD )
			;		/* wait for all children */
		return -1;		/* ECHILD */
		}
	else	{
		(void)signal( SIGCLD, sig );	/* restore entry state */
		return _wait( stat_loc );
		}
	}
