#		ULTRIX-32 V4.0 Supported (RISC) Manufacturing Key File
#
#	@(#)UDT400.k	4.1	(ULTRIX)	7/2/90
#
#	000	02-mar-1989	ccb
#	copied from mips.src source tree, revisions
#	revised for V4.0.
#

#% PRODUCT-LEVEL ATTRIBUTES
#
NAME='ULTRIX-32 RISC Supported Software V4.0'
CODE=UDT
VERS=400
MI=UDT400.mi
ROOT=1
RXMAKE=0
COMPRESS=1

#% SUBSET-LEVEL ATTRIBUTES
%%
UDTBASE400	.	1	'Base System'
UDTBIN400	.	0	'Kernel Configuration Files'
UDTINET400	.	0	'TCP/IP Networking Utilities'
UDTNFS400	UDTINET400	0	'Network File System Utilities'
UDTUMAIL400	UDTINET400	0	'Extended (Berkeley) Mailer'
UDTDL400	.	0	'Diskless Support Environment'
UDTPRINT400	.	2	'Printer Support Environment'
UDTMH400	UDTUMAIL400	2	'The RAND Mail Handler'
UDTDCMT400	.	2	'Document Preparation Software'
UDTAFM400	.	2	'Adobe Font Metric Files'
UDTPGMR400	.	2	'Software Development Utilities'
UDTINTL400	UDTPGMR400	2	'Internationalization Tools'
UDTSCCS400	.	2	'Source Code Control System'
UDTKERB400	.	2	'Kerberos Network Authentication'
UDTSEC400	.	2	'Enhanced Security Features'
UDTRPCRT400	.	2	'RPC Runtime Environment'
UDTRPCDEV400	UDTPGMR400	2	'RPC Development Environment'
UDTACCT400	.	2	'Accounting Software'
UDTCOMM400	UDTINET400	2	'Communications Utilities'
UDTMOP400	.	2	'Maintenance Operations Protocol'
UDTUUCP400	UDTCOMM400	2	'Unix-to-Unix Copy Facility'
UDTEXER400	.	2	'System Exerciser Package'
UDTMAN400	UDTDCMT400	2	'On Line Manual Pages'
