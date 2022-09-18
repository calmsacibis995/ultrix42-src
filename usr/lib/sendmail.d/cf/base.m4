############################################################
#
# @(#)base.m4	4.1	(ULTRIX)	7/2/90
#
#	General configuration information
#
#	This information is basically just "boiler-plate"; it must be
#	there, but is essentially constant.
#
#	Information in this file should be independent of location --
#	i.e., although there are some policy decisions made, they are
#	not specific to any specific site.
#
#
############################################################

include(version.m4)

#
# @(#)base.m4	4.1	(ULTRIX)	7/2/90
#
##########################
###   Special macros   ###
##########################

# my name
DnMAILER-DAEMON
# UNIX header format
DlFrom $g  $d
# delimiter (operator) characters
Do.:%@!^=/[]
# format of a total name
Dq$g$?x ($x)$.
# SMTP login message
De$j Sendmail $v/$V ready at $b
#
# forwarding host -- redefine this if you can't talk to the relay directly
DF$R
#

###################
###   Options   ###
###################

# location of alias file
OA/usr/lib/aliases

# default delivery mode (deliver in background)
Odbackground

# (don't) connect to "expensive" mailers
#Oc

# temporary file mode
OF0600

# default GID
Og1

# location of help file
OH/usr/lib/sendmail.hf

# log level
OL9

# send to me to in alias expansion
Om

# default messages to old style
Oo

# queue directory
OQ/usr/spool/mqueue

# read timeout -- violates protocols
Or2h

# status file
OS/usr/lib/sendmail.st

# queue up everything before starting transmission
Os

# default timeout interval
OT3d

# time zone names (V6 only)
OtEST,EDT

# default UID
Ou1

# Configuration options to adjust for system load conditions.
#
# Option  "x"  specifies the load average at which point sendmail
# just queues messages for later delivery. The internal default is
# a load average of 8 and need not be explicitly stated in the
# configuration file.
#
# Large systems (8800 class) can set this value to about 20 to 25
# while small systems (uvax) should run at about 4.
#
# The exact setting depends on processor speed, swap space, memory
# size, and normal load average. The load average of your system
# should be checked during a normal work period and the "x" option
# set to a few degrees higher. Too low a value will cause mail to
# be unneccessarily delayed, while too high a value may tend to
# increase the load on the machine during transient peaks.
#
Ox8
#
# Option "X" (upper case) specifies the load average at which point
# sendmail will refuse further connections.
# The internal default is 12. 
#
# Large systems (8800 class) can set this at approximately 25 to 30. The
# same comments apply here as for option "x" (lower case). Small systems
# (uvax, small memory, limited swap space) should run at about 5 or 6.
#
OX12

###############################
###   Message precedences   ###
###############################

Pfirst-class=0
Pspecial-delivery=100
Pjunk=-100

#########################
###   Trusted users   ###
#########################

Troot
Tdaemon
Tuucp
Tnetwork

#############################
###   Format of headers   ###
#############################

H?P?Return-Path: <$g>
H?R?Received: $?sfrom $s $.by $j ($v/$V)
	id $i; $b
H?D?Resent-Date: $a
H?D?Date: $a
H?F?Resent-From: $q
H?F?From: $q
H?x?Full-Name: $x
HSubject:
# HPosted-Date: $a
# H?l?Received-Date: $b
H?M?Resent-Message-Id: <$t.$i@$j>
H?M?Message-Id: <$t.$i@$j>

###########################
###   Rewriting rules   ###
###########################


################################
#  Sender Field Pre-rewriting  #
################################
S1
#R$*<$*>$*		$1$2$3				defocus

###################################
#  Recipient Field Pre-rewriting  #
###################################
S2
#R$*<$*>$*		$1$2$3				defocus

#################################
#  Final Output Post-rewriting  #
#################################
S4

R@			$@				handle <> error addr

# resolve numeric addresses to name if possible
R$*<@[$+]>$*		$:$1<@$[[$2]$]>$3		lookup numeric addr

# externalize local domain info
R$*<@LOCAL>$*		$:$1<@$A>$2			change local info
R$*<$*.LOCAL>$*		$:$1<$2.$D>$3			change local info
R$*<$+>$*		$1$2$3				defocus
R@$+:$+:$+		@$1,$2:$3			<route-addr> canonical
R@$+:$+			$@<@$1:$2>			route-addr needs <>

# UUCP must always be presented in old form
R$+@$-.UUCP		$2!$1				u@h.UUCP => h!u

# Put MAIL-11 back in :: form
R$+@$-.DNET		$2::$1				u@h.dnet => h::u
R$+@$-.$-.DNET		$2.$3::$1			numeric form

# delete duplicate local names -- mostly for arpaproto.mc
R$+%$=w@$=w		$1@$3				u%UCB@UCB => u@UCB
R$+%$=w@$=w.$D		$1@$3.$D			u%UCB@UCB => u@UCB

###########################
#  Name Canonicalization  #
###########################
S3

# handle "from:<>" special case
R<>			$@@				turn into magic token

# basic textual canonicalization
R$*<$*<$*<$+>$*>$*>$*	$4				3-level <> nesting
R$*<$*<$+>$*>$*		$3				2-level <> nesting
R$*<$+>$*		$2				basic RFC821/822 parsing
R$+ at $+		$1@$2				"at" -> "@" for RFC 822
R$*<$*>$*		$1$2$3				in case recursive

# make sure <@a,@b,@c:user@d> syntax is easy to parse -- undone later
R@$+,$+			@$1:$2				change all "," to ":"

# localize and dispose of domain-based addresses
R@$+:$+			$@$>6<@$1>:$2			handle <route-addr>

# more miscellaneous cleanup
R$=w::$-$+		$2$3				clean off my name
R$+			$:$>8$1				host dependent cleanup
R$+:$*;@$+		$@$1:$2;@$3			list syntax
R$+@$+			$:$1<@$2>			focus on domain
R$+<$+@$+>		$1$2<@$3>			move gaze right
R$+<@$+%$+>		$1%$2<@$3>			move gaze right
R$-!$+<@$~S>		$1!$2@$3			defocus - not local host
R$-::$+<@$~S>		$1::$2@$3			defocus - not local host
R$+<@$+>		$@$>6$1<@$2>			already canonical

# localize mail-11 syntax
R$-::$+			$@$>6$2<@$1.dnet>		host::user
R$-.$-::$+		$@$>6$3<@$1.$2.dnet>		numeric decnet addr

# convert old-style addresses to a domain-based address
R$+%$+			$:$1<@$2>			user%host
R$+<@$+%$+>		$1%$2<@$3>			move "@" right
R$+<@$+>		$@$>6$1<@$2>			now canonical
R$-:$+			$@$>6$2<@$1>			host:user
R$-.$+!$+		$@$>6$3<@$1.$2>			host.domain!user
R$-!$+			$@$>6$2<@$1.UUCP>		resolve uucp names

