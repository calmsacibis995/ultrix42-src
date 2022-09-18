############################################################
############################################################
#####
#####		SENDMAIL CONFIGURATION FILE
#####
#
# @(#)generic.mc	4.1	(ULTRIX)	7/2/90
#
#####		generic configuration
#####
#####
############################################################
############################################################


############################################################
###	local info
############################################################


# This section specifies data about your environment and how to
# handle mail. The configuration has provisions for several (independent)
# mail relay machines for handling different kinds of mail.
# Below these are all given the name "RELAY". If you need or want this
# kind of relay you will need to uncomment the line and put in the name
# of the appropriate host.
#
# The relay machines are specified separately for each kind of mail
# so you can spread out the load. On the other hand they could all
# point to the same machine to centralize information. The relay can
# even name this machine and the "right thing" will happen. When adding
# relay names, be sure to put the first name directly next to the
# name of the macro or class - do not put a space between it. Also, remember
# to uncomment it by removing the pound sign "#".
# Relay machine names should be expressed as the simple unqualified host
# name. ie. Do not include the domain specifiation of the host in the macro
# definition of the relay name.
#
# For example, the general purpose relay example is given as:
#
#	start of line ->#DRRELAY
#
# To make "foo" your general purpose relay, change the line to:
#
#	start of line ->DRfoo
# 
#
# The $w macro is preset by sendmail to the current host's unqualified
# host name. Here we simply capture the value in our own $A macro.
#
DA$w


# The $D macro will be the domain for this machine. If your machine
# is part of a registered domain, that name should be defined here.
#
# If you don't have a domain, leave it defined as local. Some typical
# domains are DEC.COM, ARPA, MIT.EDU, CSS.GOV.
#
# Do not add domain definitions to the $D macro. The macro should be
# re-defined to specify the primary internet domain for this machine.
# Additional domains for this machine may ge given in the  CLASS  "D"
# definitions below.
#
DDlocal

# These are other "domains" in which this machine may be named. They
# are used to recognize all forms of the name for the local machine.
#
CDlocal uucp dnet
#
# These are pseudo domains used internally. They are not part of
# the real Internet system and must not be allowed to escape.
#
CIlocal uucp dnet


# The $j macro is the name presented to remote SMTP servers when
# establishing a mail connection. It should always be your fully
# qualified domain name.
#
Dj$w.$D


# The $U macro is the name of this machine for UUCP communications.
# It is usually the same as the normal host name, but may need to be
# different for various site-specific reasons.
#
DU$w


# The $Y macro should define the name of this system for  DECnet
# communications. It is intended for use in the DECnet mail ruleset.
# This is the DECnet node name to be used for DECnet mail. It does not
# necessarily need to be the same as the hostname and most probably could
# not be in any case. U**x hostnames can be 255 characters long and contain
# imbedded domain information, while DECnet node names are limited to a
# maximum of 6 plain ascii characters.
#
DY$w


# The $w CLASS can be used to define "nicknames" for the local host.
# These are alternate names that other systems may use to direct mail
# to the local host. If this feature is used, the class should contain
# the set of all names by which this host is known by. This feature might
# normally be used when the local host name is revised but it is not feasable
# to notify all other hosts of the new name. Additionally, it may be that
# the local host is known by one name on the internet and another name on
# additional networks to which this host is connected.
#
#
#CwMyUnqualifiedHostName  AnAlternateName  NameUsedOnOtherNetwork 


# The $S class can be used to identify "local" machines. Mail to these
# machines will include the sender's and receiver's host names but will
# omit the domain in order to reduce the visual clutter. Mail delivered
# by SMTP to other (non-local) hosts will use full qualification.
#
# If your /etc/hosts file contains only local hosts, you can read the names
# directly from /etc/hosts. Otherwise you should produce a separate list
# of local host names in /usr/lib/hosts.local. Both forms are shown below.
#
#FS/usr/lib/hosts.local
#FS/etc/hosts %*[0-9.]%s


# The $R macro is the simple unqualifed host name of your general purpose
# relay machine. Any mail that cannot be resolved locally is forwarded to
# this machine for processing. It gets anything not covered by the other
# relay hosts.
#
#DRRELAY


# The $F macro and class defines a named list of hosts and a relay machine
# to handle their mail. This is not normally used, but is included in the
# rewrite rules in case the need should arise.
#
#DFRELAY
#CFred blue green


# The $E macro defines the relay host for dnet (DECNET) connections.
# You need this only if your machine is not running DECNET but depending
# on some other local machine to relay DECNET mail.
#
#DERELAY


# The $Z macro names the relay for UUCP mail for sites not directly
# reachable by this host. The list of reachable sites is read from
# the UUCP L.sys file and assigned to the $Z class once when the
# config file is frozen.
#
# *WARNING* You must refreeze the sendmail config file every time
# you add a new host to uucp, or else sendmail will not know about
# it.  If you use uucpsetup to add new hosts, this will be done
# for you automatically - if you add hosts to L.sys by hand, you
# must do it yourself. Type: "/usr/lib/sendmail -bz" as root.
#
#DZRELAY
#FZ/usr/lib/uucp/L.sys %[0-9a-zA-Z_-]


# The $N macro and class allow you to establish a central site for resolving
# user names which are not known locally. If you use this the relay machine
# will be passed any mail which is not for someone listed in /etc/passwd.
# In particular this means that all alias processing will be passed to the
# relay. You can add additional names to the N class to handle specific local
# aliases such as postmaster.
#
#
###################################
### Notes for Name Server Users ###
###################################
#
# For installations running one or both name servers, Yellow Pages or
# Hesiod/BIND, locally known users include (by default) those users defined
# in the Yellow Pages maps, passwd.byname and mail.aliases, or the Hesiod
# passwd and aliases database.  If the user is not found in the /etc/passwd
# file, sendmail will check the above maps before passing the mail on to the
# central site for resolution.  If you do not want to consider such users as
# "local" for mailing purposes, comment out the rules defined under the
# sections labeled
#
#		############################################
#		### Optional Rules for Name Server Users ###
#		############################################
#
# in the  "Machine dependent part of ruleset zero"  definitions below. 
#
#######################################
### end Notes for Name Server Users ###
#######################################
#
#
# WARNING: If you use this feature you must refreeze the configuration file
# (via /usr/lib/sendmail -bz) every time a user is added or deleted from
# /etc/passwd. Also you will not be able to establish any local aliases
# other than those listed in class N below.
#
#DNRELAY
#FN/etc/passwd %[0-9a-zA-Z_-]
#CN MAILER-DAEMON postmaster admin
#

include(base.m4)

include(localbase.m4)

include(zerobase.m4)

################################################
###  Machine dependent part of ruleset zero  ###
################################################
#
# @(#)generic.mc	4.1	(ULTRIX)	7/2/90
#
# short circuit local hosts

R$*<@$=S.UUCP>$*	$1<@$2.LOCAL>$3			uucp => local
R$*<@$=S.dnet>$*	$1<@$2.LOCAL>$3			dnet => local
R$*<@$=S.$D>$*		$:$1<@$2.LOCAL>$3		domain => local

# if we do not have dnet, send to forwarder
R$+<@$+.dnet>$*		$:$?E$1%$2.dnet<@$E.LOCAL>$3$|$1<@$2.dnet>$3$.

# forward uucp we don't know to primary forwarder
R$*<@$~Z.UUCP>$*	$:$?Z$1%$2.UUCP<@$Z.LOCAL>$3$|$1<@$2.UUCP>$3$.

# forward around hosts with communication problems
R$*<@$=F.LOCAL>$*	$1%$2.LOCAL<@$F.LOCAL>$3	reroute message

# Undo forwarding if we are forwarder
R$+%$+.$-<@$=w.LOCAL>$*	$1<@$2.$3>$5			if we are forwarder

# resolve names we can handle locally
R$*<@LOCAL>$*		$#local$:$1$2			local

# first handle local network traffic double check that we know the host
#R$*<@$~S.LOCAL>$*	$#error$:Host $2 not connected to $w via tcp
#R$*<@$->$*		$#error$:Host $2 not connected to $w via tcp
R$*<@$-.LOCAL>		$#tcplocal$@$2$:$1<@$2>		user@tcphost.LOCAL
R$*<@$-.LOCAL>$+	$#tcplocal$@$2$:$1<@$2.LOCAL>$3	user@tcphost.LOCAL

# Do umc mailings
R$*<@$+.umc>$*		$#umc$@$2$:$1<@$2>$3		user@host.umc


# Echotest mailer (for debug via mail -v echotest::user)
R$+<@echotest.dnet>	$#Echo$@echotest$:$1


# handle dnet mail
R$+<@$+.dnet>		$#Dmail$@$2$:$1			dnet user


# handle uucp traffic.  double check that we know the host
R$*<@$~Z.UUCP>$*	$#error$:Host $2 not connected to $w via uucp
R<@$+.UUCP>:$+		$1!$2				to old format
R$+<@$+.UUCP>		$2!$1				to old format
R$-!$+			$#uucp$@$1$:$2			host!...


# all other domains try for forwarder, if defined
R$*<@$+.$->$*		$:$?R$1<@$2.$3>$4<@$R>$|$1<@$2.$3>$4$.
R$*<@$+.$->$*<@$=w>	$#tcp$@$2.$3$:$1<@$2.$3>$4	we are forwarder
R$*<@$+.$->$*<@$+>	$#tcplocal$@$5$:$1<@$2.$3>$4	pass it on


# no forwarder, try for tcp connection
R$*<@$+>$*		$#tcp$@$2$:$1<@$2>$3		user@tcphost.domain


# at this point, we are trying a local name.  if we don't
# recognize it, forward to name forwarder host if defined.
#
#		############################################
#		### Optional Rules for Name Server Users ###
#		############################################
#
# For installations running one or both name servers, Yellow Pages or
# Hesiod/BIND, names defined in the Yellow Pages maps, passwd.byname and
# mail.aliases, or the Hesiod passwd and aliases databases are considered to
# be local.  If the system is not running a name server, the next two rules
# produce no effect. The next TWO rules may be commented out if you are
# running Yellow Pages or Hesiod/BIND but do not want users defined in the
# YP passwd.byname and mail.aliases maps or the Hesiod passwd and aliases
# databases to be considered as "local" to your system. That is, delivery of
# mail for those users will be attempted on the local machine.  If the rules
# are commented out, the mail will be sent to the name forwarder for resolution
# if one is defined.  If no name forwarder is defined, the mail will be
# rejected as "user unknown".
#

R${$*			$#local$:$1	if in YP or Hesiod passwd map, assume local
R$"$*			$#local$:$1	if in YP or Hesiod aliases map, assume local


R$~N			$:$?N$1<@$N.LOCAL>$|$1$.	not local, redirect
R$*<@$=w.LOCAL>$*	$1				but, there's more to it.
#							If we are the forwarder,
#							hope it is an alias.
R$*<@$*$-.LOCAL>$*	$#tcplocal$@$3$:$1<@$2$3.LOCAL>$4	let name forwarder do it

# everything else must be a local name
R$+			$#local$:$1			local names

include(localm.m4)
include(umc.m4)
include(xm.m4)
include(tcpm.m4)
include(uucpm.m4)

