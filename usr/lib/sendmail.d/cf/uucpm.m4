############################################################
############################################################
#####
#####		UUCP Mailer specification
#####
#		@(#)uucpm.m4	4.1	(ULTRIX)	7/2/90
#####
############################################################
############################################################

# There are two options for the uucp mailer presented here. The only
# difference is the -r option to uux. With the -r option, uucp mail
# will be queued until some other process forces a call to be made.
# Without the -r option, a call will be attempted immediately to
# deliver this message, although the normal uucp rules about when to
# call still apply.
#
# This one just queues the message for a later uucp call.
#Muucp,	P=/usr/bin/uux, F=sDFhuU, S=13, R=23, M=100000,
#	A=uux - -r $h!rmail ($u)
#
# This one attempts a call immediately.
Muucp,	P=/usr/bin/uux, F=sDFhuU, S=13, R=23, M=100000,
	A=uux - $h!rmail ($u)
#
# This mailer lets you pretend to speak uucp to a host with which you have
# an IP/TCP connection. It uses UUCP address rewrite rules with TCP
# message transport. Hosts to be given this treatment must be identified
# by ruleset zero.
Muucptcp,	P=[IPC], F=msDFMhuX, S=13, R=23, M=100000, A=IPC $h

S13
R$+			$:$>23$1			prescan
R$U!$+			$@$U!$1				ready to go
R$+			$@$U!$1				stick on our host name

S23
R$+<@$-.UUCP>		$2!$1				old form
R$+<@$=S>		$1				strip local names
R$+<@$=S.$=D>		$1				strip local names
R$+<@$-.DNET>		$2.dnet!$1			old form
R$+<@$+>		$2!$1				old form
R$=S!$+			$2				strip local name
R$=S.$D!$+		$2				strip local name
R$=S.$=D!$+		$2				strip local name
#R$-			$U!$1				stick on our host name
