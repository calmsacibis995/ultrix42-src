#	ULT400.k -
#		ULTRIX-32 V4.0 Supported (VAX) Mfg Key File
#
#	@(#)ULT400.k	4.1	(ULTRIX)	7/2/90
#
#	000	02-mar-1989	ccb
#	Copied from V3.0 (VAX) sources.
#	revsion update from V3.0 to V4.0
#

#% PRODUCT-LEVEL ATTRIBUTES
NAME='ULTRIX-32 VAX Supported Software V4.0'
CODE=ULT
VERS=400
MI=ULT400.mi
ROOT=1
RXMAKE=0
COMPRESS=1

#% SUBSET-LEVEL ATTRIBUTES
%%
ULTBASE400	.	1	'Base System'
ULTBIN400	.	0	'Kernel Configuration Files'
ULTINET400	.	0	'TCP/IP Networking Utilities'
ULTNFS400	ULTINET400	0	'Network File System Utilities'
ULTUMAIL400	ULTINET400	0	'Extended (Berkeley) Mailer'
ULTDL400	.	0	'Diskless Support Environment'
ULTPRINT400	.	2	'Printer Support Environment'
ULTACCT400	.	2	'Accounting Software'
ULTPGMR400	.	2	'Software Development Utilities'
ULTINTL400	ULTPGMR400	2	'Internationalization Tools'
ULTSCCS400	.	2	'Source Code Control System'
ULTPASCAL400	.	2	'Pascal Development Package'
ULTDCMT400	.	2	'Document Preparation Software'
ULTAFM400	.	2	'Adobe Font Metric Files'
ULTMH400	ULTUMAIL400	2	'RAND Mail Handler'
ULTCOMM400	ULTINET400	2	'Communications Utilities'
ULTRPCRT400	.	2	'RPC Runtime Environment'
ULTRPCDEV400	ULTPGMR400	2	'RPC Development Environment'
ULTKERB400	.	2	'Kerberos Network Authentication'
ULTSEC400	.	2	'Enhanced Security Features'
ULTMOP400	.	2	'Maintenance Operations Protocol'
ULTUUCP400	ULTCOMM400	2	'Unix-to-Unix Copy Facility'
ULTVAXC400	.	2	'VAX C/ULTRIX'
ULTEXER400	.	2	'System Exerciser Package'
ULTBSC400	.	2	'Bisynchronous Communications'
ULTMAN400	ULTDCMT400	2	'On Line Manual Pages'
