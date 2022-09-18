#	UDT040.k -
#		ULTRIX-32 V4.0 Supported (RISC) Manufacturing Key File
#
#	@(#)UDT040.k	4.1	(ULTRIX)	7/2/90
#
#	000	02-mar-1989	ccb
#	copied from mips.src source tree, revisions
#	revised for V4.0.
#

#% PRODUCT-LEVEL ATTRIBUTES
#
NAME='ULTRIX-32 RISC Supported Software V4.0'
CODE=UDT
VERS=040
MI=UDT040.mi
ROOT=1
RXMAKE=0
COMPRESS=1

#% SUBSET-LEVEL ATTRIBUTES
%%
UDTBASE040	.	1	'Base System'
UDTBIN040	.	0	'Kernel Configuration Files'
UDTINET040	.	0	'TCP/IP Networking Utilities'
UDTEXER040	.	0	'System Exerciser Package'
UDTNFS040	UDTINET040	0	'Network File System Utilities'
UDTUMAIL040	UDTINET040	0	'Extended (Berkeley) Mailer'
UDTMH040	UDTUMAIL040	2	'The RAND Mail Handler'
UDTDL040	.	0	'Diskless Support Environment'
UDTDCMT040	.	2	'Document Preparation Software'
UDTAFM040	.	2	'Adobe Font Metric Files'
UDTPGMR040	.	2	'Software Development Utilities'
UDTINTL040	UDTPGMR040	2	'Internationalization Tools'
UDTSCCS040	.	2	'Source Code Control System'
UDTKERB040	.	2	'Kerberos Network Authentication'
UDTSEC040	.	2	'Enhanced Security Features'
UDTRPCRT040	.	2	'RPC Runtime Environment'
UDTRPCDEV040	UDTPGMR040	2	'RPC Development Environment'
UDTACCT040	.	2	'Accounting Software'
UDTCOMM040	UDTINET040	2	'Communications Utilities'
UDTMOP040	.	2	'Maintenance Operations Protocol'
UDTUUCP040	UDTCOMM040	2	'Unix-to-Unix Copy Facility'
UDTMAN040	UDTDCMT040	2	'On Line Manual Pages'
