############################################################
############################################################
#####
#####		Arpanet TCP Mailer specification
#####
# @(#)tcpm.m4	4.1	(ULTRIX)	7/2/90
#####
############################################################
############################################################

Mtcp,	P=[IPC], F=mDFMueCXLR, S=14, R=14, A=IPC $h, E=\r\n
Mtcplocal,	P=[IPC], F=mDFMueCXLR, S=19, R=19, A=IPC $h, E=\r\n

S14

# pass <route-addr>'s through
R<@$+>$*		$@<@$[$1$]>$2			resolve <route-addr>

# gatewayed to/from DNET
R$+<@$-.DNET>$*		$@$1%$2.dnet<@$A>$3	user@host.DNET

# gatewayed to/from UUCP
R$+<@$-.UUCP>$*		$@$2!$1<@$A>$3		user@host.DNET

# strip local host names if we are forwarder
R$+<@$=S>		$:$1<@$2>$?R<$R>$.		add forwarder
R$+<@$=S.$D>		$:$1<@$2.$3>$?R<$R>$.
R$+<@$+.LOCAL>		$:$?R$1<@$2.LOCAL><$R>$|$1<@$2>$.

R$+<@$=S><$=w>		$:$1				strip name
R$+<@$=S.$D><$=w>	$:$1
R$+<@$+.LOCAL><$=w>	$:$1
R$+<$R>			$:$1				remove forwarder name

# tack on our name and domain
R$-			$@$1<@$A.$D>			local names
R$+<@LOCAL>		$@$1<@$A.$D>			local names
R$+<@$+.LOCAL>		$@$1<@$2.$D>			local hosts
R$+<@$=S>		$@$1<@$2.$D>			local hosts

# canonicalize
R$+<@$->		$@$1<@$[$2$]>			simple name
R$+<@[$+]>		$@$1<@[$2]>			numeric ok
R$+<@$+>		$@$1<@$[$2$]>			user@host.domain


S19
# Delete domain on local hosts.
R$+<@LOCAL>		$@$1<@$w>
R$+<@$=S>		$@$1<@$2>
R$+<@$=S.$D>		$@$1<@$2>
R$+<@$=S.LOCAL>		$@$1<@$2>
R$+<@$+.$=D>		$@$1<@$2.$3>			leave local domains
R$-			$@$1<@$A>			qualify simple name

# Do normal rewrite if all else fails.
R$+			$@$>14$1

