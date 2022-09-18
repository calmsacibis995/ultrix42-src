#		ULTRIX V4.2 Supported (VAX) Manufacturing Key File
#
#       @(#)ULT421.k	4.8	ULTRIX	4/30/91
#
#	001 	15-OCt-90	overman
#	copied the v4.1 to v4.2
#
#	000	17-Jul-1990	mdf
#	Copied from V4.0 (VAX) sources.
#	revsion update from V4.0 to V4.1
#

#% PRODUCT-LEVEL ATTRIBUTES
NAME='ULTRIX VAX Supported Software V4.2'
CODE=ULT
VERS=421
MI=ULT421.mi
ROOT=0
RXMAKE=0
COMPRESS=1
FLGEXPPGMR='`if [ -f ./usr/etc/subsets/ULTPGMR420.lk ]; then echo 0; else echo 2; fi `'
FLGEXPSEC='`if [ -f ./usr/etc/subsets/ULTSEC420.lk ]; then echo 0; else echo 2; fi `'
FLGEXPRPCRT='`if [ -f ./usr/etc/subsets/ULTRPCRT420.lk ]; then echo 0; else echo 2; fi `'
FLGEXPMAN='`if [ -f ./usr/etc/subsets/ULTMAN420.lk ]; then echo 0; else echo 2; fi `'
FLGEXPMANPGMR='`if [ -f ./usr/etc/subsets/ULTMANPGMR420.lk ]; then echo 0; else echo 2; fi `'
FLGEXPEXAMPLES='`if [ -f ./usr/etc/subsets/ULTEXAMPLES420.lk ]; then echo 0; else echo 2; fi `'
FLGEXPACCT='`if [ -f ./usr/etc/subsets/ULTACCT420.lk ]; then echo 0; else echo 2; fi `'
FLGEXPCOMM='`if [ -f ./usr/etc/subsets/ULTCOMM420.lk ]; then echo 0; else echo 2; fi `'
FLGEXPPRINT='`if [ -f ./usr/etc/subsets/ULTPRINT420.lk ]; then echo 0; else echo 2; fi `'

#% SUBSET-LEVEL ATTRIBUTES
%%
ULTBASE421	ULTBASE420	1	'Base System UPGRADE'
ULTBIN421	ULTBIN420	0	'Kernel Config Files UPGRADE'
ULTINET421      ULTINET420      0       'TCP/IP Networking Util UPGRADE'
ULTNFS421       ULTNFS420       0       'Network File Sys Util UPGRADE'
ULTPGMR421	ULTPGMR420	FLGEXPPGMR	'Software Dev. Utilities UPGRADE'
ULTSEC421       ULTSEC420       FLGEXPSEC       'Enhanced Sec. Features UPGRADE'
ULTRPCRT421	ULTRPCRT420	FLGEXPRPCRT	'RPC Runtime Environment UPGRADE'
ULTPRINT421	ULTPRINT420	FLGEXPPRINT	'Printer Support Env. UPGRADE'
ULTACCT421	ULTACCT420	FLGEXPACCT	'Accounting Software UPGRADE'
ULTCOMM421	ULTCOMM420	FLGEXPCOMM	'Communications Util. UPGRADE'
ULTMAN421       ULTMAN420       FLGEXPMAN       'Ref Pages Admin/Users UPGRADE'
ULTMANPGMR421   ULTMANPGMR420      FLGEXPMANPGMR       'Ref Pages for Prog UPGRADE'
ULTEXAMPLES421	ULTEXAMPLES420	FLGEXPEXAMPLES	'Programming Examples UPGRADE'
