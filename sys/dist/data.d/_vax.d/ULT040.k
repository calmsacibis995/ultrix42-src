#	ULT040.k -
#		ULTRIX-32 V4.0 Supported (VAX) Mfg Key File
#
#	@(#)ULT040.k	4.1	(ULTRIX)	7/2/90
#
#	000	02-mar-1989	ccb
#	Copied from V3.0 (VAX) sources.
#	revsion update from V3.0 to V4.0
#

#% PRODUCT-LEVEL ATTRIBUTES
NAME='ULTRIX-32 VAX Supported Software V4.0'
CODE=ULT
VERS=040
MI=ULT040.mi
ROOT=1
RXMAKE=0
COMPRESS=1

#% SUBSET-LEVEL ATTRIBUTES
%%
ULTBASE040	.	1	'Base System'
ULTBIN040	.	0	'Kernel Configuration Files'
ULTINET040	.	0	'TCP/IP Networking Utilities'
ULTNFS040	ULTINET040	0	'Network File System Utilities'
ULTUMAIL040	ULTINET040	0	'Extended (Berkeley) Mailer'
ULTDL040	.	0	'Diskless Support Environment'
ULTEXER040	.	0	'System Exerciser Package'
ULTACCT040	.	2	'Accounting Software'
ULTPGMR040	.	2	'Software Development Utilities'
ULTINTL040	ULTPGMR040	2	'Internationalization Tools'
ULTSCCS040	.	2	'Source Code Control System'
ULTPASCAL040	.	2	'Pascal Development Package'
ULTDCMT040	.	2	'Document Preparation Software'
ULTAFM040	.	2	'Adobe Font Metric Files'
ULTMH040	ULTUMAIL040	2	'RAND Mail Handler'
ULTCOMM040	ULTINET040	2	'Communications Utilities'
ULTRPCRT040	.	2	'RPC Runtime Environment'
ULTRPCDEV040	ULTPGMR040	2	'RPC Development Environment'
ULTKERB040	.	2	'Kerberos Network Authentication'
ULTSEC040	.	2	'Enhanced Security Features'
ULTMOP040	.	2	'Maintenance Operations Protocol'
ULTBSC040	.	2	'Bisynchronous Communications'
ULTUUCP040	ULTCOMM040	2	'Unix-to-Unix Copy Facility'
ULTVAXC040	.	2	'VAX C/ULTRIX'
ULTMAN040	ULTDCMT040	2	'On Line Manual Pages'
