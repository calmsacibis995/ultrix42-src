############################################################
############################################################
#####
#####		Local and Program Mailer specification
#####
#		@(#)localm.m4	2.2	(ULTRIX)	4/12/89
#####
############################################################
############################################################

Mlocal,	P=/bin/mail, F=lsDFmn, S=10, R=10, A=mail -r $f -d $u
Mprog,	P=/bin/sh,   F=lsDFRe,   S=10, R=10, A=sh -c $u

S10
R$+<@LOCAL>		$@$1				delete LOCAL
R$+<@$-.LOCAL>		$@$1<@$2>			delete .LOCAL
R@			$@MAILER-DAEMON			errors to mailer-daemon
