############################################################
############################################################
#####
#####		RULESET ZERO PREAMBLE
#####
#####	The beginning of ruleset zero is constant through all
#####	configurations.
#####
# @(#)zerobase.m4	4.1	(ULTRIX)	7/2/90
#####
############################################################
############################################################

S9
# rerun ruleset 3 and then call 0 again
R$+			$:$>3$1
R$+			$@$>0$1

S0

# first make canonical (necessary for recursion)
#R$+			$:$>3$1				make canonical

# handle special cases.....
R@			$#local$:MAILER-DAEMON		handle <> form
R$*<@[$+]>$*		$:$1<@$[[$2]$]>$3		lookup numeric addr
R$*<@[$+]>$*		$#tcp$@[$2]$:$1@[$2]$3		numeric internet spec

# arrange for local names to be fully qualified
R$*<$*$=S>$*		$1<$2$3.LOCAL>$4		user@etherhost

# canonicalize using the nameserver if not internal domain
R$*<@$*.$~I>$*		$:$1<@$[$2.$3$]>$4
R$*<@$->$*		$:$1<@$[$2$]>$3

# now delete the local info
R$*<$*$=w.$D>$*		$1<$2>$4			thishost.thisdomain
R$*<$*$=w.$=D>$*	$1<$2>$5			thishost.localdomain
R$*<$*$=w>$*		$1<$2>$4			thishost
R$*<$*.>$*		$1<$2>$3			drop trailing dot
R<@>:$*			$@$>9$1				retry after route strip
R$*<@>			$@$>9$1				strip null trash & retry
R$*<@LOCAL>		$@$>9$1				strip null trash & retry


##################################
#  End of ruleset zero preamble  #
##################################
