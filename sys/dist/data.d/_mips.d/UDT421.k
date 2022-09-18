#		ULTRIX V4.2 Supported (RISC) Manufacturing Key File
#
#	@(#)UDT421.k	4.7	ULTRIX	4/30/91
#
#	002	4-Apr-1991	overman
#	added FLAGS expressions to determine if the parent subset is
#	installed and if it is set the mup subset as mandatory.
#
#	001	17-jul-1990	mdf
#	revision update.
#
#	000	02-mar-1989	ccb
#	copied from mips.src source tree, revisions
#	revised for V4.0.
#

#% PRODUCT-LEVEL ATTRIBUTES
#
NAME='ULTRIX RISC Supported Software V4.2'
CODE=UDT
VERS=420
MI=UDT421.mi
ROOT=0
RXMAKE=0
COMPRESS=1
FLGEXPRPCRT='`if [ -f /usr/etc/subsets/UDTRPCRT420.lk ]; then echo 0 ;else echo 2;fi`'
FLGEXPSEC='`if [ -f ./usr/etc/subsets/UDTSEC420.lk ]; then echo 0; else echo 2; fi `'
FLGEXPPGMR='`if [ -f /usr/etc/subsets/UDTPGMR420.lk ]; then echo 0 ;else echo 2;fi`'
FLGEXPMAN='`if [ -f ./usr/etc/subsets/UDTMAN420.lk ]; then echo 0; else echo 2; fi `'
FLGEXPMANPGMR='`if [ -f ./usr/etc/subsets/UDTMANPGMR420.lk ]; then echo 0; else echo 2; fi `'
FLGEXPEXAMPLES='`if [ -f /usr/etc/subsets/UDTEXAMPLES420.lk ]; then echo 0 ;else echo 2;fi`'
FLGEXPACCT='`if [ -f ./usr/etc/subsets/UDTACCT420.lk ]; then echo 0; else echo 2; fi `'
FLGEXPCOMM='`if [ -f ./usr/etc/subsets/UDTCOMM420.lk ]; then echo 0; else echo 2; fi `'
FLGEXPPRINT='`if [ -f ./usr/etc/subsets/UDTPRINT420.lk ]; then echo 0; else echo 2; fi `'

#% SUBSET-LEVEL ATTRIBUTES
%%
UDTBASE421	UDTBASE420	1	'Base System UPGRADE'
UDTBIN421	UDTBIN420	0	'Kernel Config Files UPGRADE'
UDTINET421      UDTINET420      0	'TCP/IP Networking Util UPGRADE'
UDTNFS421       UDTNFS420	0	'Network File Sys Util UPGRADE'
UDTPGMR421	UDTPGMR420	FLGEXPPGMR	'Software Dev. Utilities UPGRADE'
UDTSEC421       UDTSEC420       FLGEXPSEC  'Enhanced Sec. Features UPGRADE'
UDTRPCRT421	UDTRPCRT420	FLGEXPRPCRT 'RPC Runtime Environment UPGRADE'
UDTACCT421	UDTACCT420	FLGEXPACCT	'Accounting Software UPGRADE'
UDTCOMM421	UDTCOMM420	FLGEXPCOMM	'Communications Util. UPGRADE'
UDTPRINT421	UDTPRINT420	FLGEXPPRINT	'Printer Support Env. UPGRADE'
UDTMAN421       UDTMAN420       FLGEXPMAN  'Ref Pages Admin/Users UPGRADE'
UDTMANPGMR421   UDTMANPGMR420   FLGEXPMANPGMR      'Ref Pages for Prog UPGRADE'
UDTEXAMPLES421	UDTEXAMPLES420	FLGEXPEXAMPLES	'Programming Examples UPGRADE'
