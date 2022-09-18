#		ULTRIX V4.1 Supported (RISC) Manufacturing Key File
#
#	@(#)UDT410.k	4.1	(ULTRIX)	7/17/90
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
NAME='ULTRIX RISC Supported Software V4.1'
CODE=UDT
VERS=410
MI=UDT410.mi
ROOT=1
RXMAKE=0
COMPRESS=1

#% SUBSET-LEVEL ATTRIBUTES
%%
UDTBASE410	.	1	'Base System'
UDTBIN410	.	0	'Kernel Configuration Files'
UDTINET410	.	0	'TCP/IP Networking Utilities'
UDTNFS410	UDTINET410	0	'Network File System Utilities'
UDTUMAIL410	UDTINET410	0	'Extended (Berkeley) Mailer'
UDTDL410	.	0	'Diskless Support Environment'
UDTPRINT410	.	2	'Printer Support Environment'
UDTMH410	UDTUMAIL410	2	'The RAND Mail Handler'
UDTDCMT410	.	2	'Document Preparation Software'
UDTAFM410	.	2	'Adobe Font Metric Files'
UDTPGMR410	.	2	'Software Development Utilities'
UDTINTL410	UDTPGMR410	2	'Internationalization Tools'
UDTSCCS410	.	2	'Source Code Control System'
UDTKERB410	.	2	'Kerberos Network Authentication'
UDTSEC410	.	2	'Enhanced Security Features'
UDTRPCRT410	.	2	'RPC Runtime Environment'
UDTRPCDEV410	UDTPGMR410	2	'RPC Development Environment'
UDTACCT410	.	2	'Accounting Software'
UDTCOMM410	UDTINET410	2	'Communications Utilities'
UDTMOP410	.	2	'Maintenance Operations Protocol'
UDTUUCP410	UDTCOMM410	2	'Unix-to-Unix Copy Facility'
UDTEXER410	.	2	'System Exerciser Package'
UDTMAN410	UDTDCMT410	2	'On Line Manual Pages'
