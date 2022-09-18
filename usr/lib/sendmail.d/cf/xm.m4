############################################################
############################################################
#####
#####		Mail-11 Mailer
#####
#####		@(#)xm.m4	4.1	(ULTRIX)	7/2/90
#####
############################################################
############################################################

MDmail,	P=/usr/bin/mail11v3, F=mnSXxH, S=17, R=18, A=mail11 $f $x $h

S17
R$+			$:$>18$1			preprocess
R$Y::$+			$@$Y::$1			ready to go

S18
R$+<@$-.UUCP>		$:$2!$1				back to old style
R$+<@$-.DNET>		$:$2::$1			convert to dnet style
R$+<@$-.LOCAL>		$:$2::$1			convert to dnet style
R$+<@$=S>		$:$2::$1			convert to dnet style
R$+<@$=S.$D>		$:$2::$1			convert to dnet style
R$=S::$+		$2				strip local names
R$+::$+			$@$1::$2			already qualified


