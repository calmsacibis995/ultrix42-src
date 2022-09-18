#	ULT410.k -
#		ULTRIX-32 V4.1 Supported (VAX) Mfg Key File
#
#	@(#)ULT410.k	4.1	(ULTRIX)	7/17/90
#
#	000	17-Jul-1990	mdf
#	Copied from V4.0 (VAX) sources.
#	revsion update from V4.0 to V4.1
#

#% PRODUCT-LEVEL ATTRIBUTES
NAME='ULTRIX VAX Supported Software V4.1'
CODE=ULT
VERS=410
MI=ULT410.mi
ROOT=1
RXMAKE=0
COMPRESS=1

#% SUBSET-LEVEL ATTRIBUTES
%%
ULTBASE410	.	1	'Base System'
ULTBIN410	.	0	'Kernel Configuration Files'
ULTINET410	.	0	'TCP/IP Networking Utilities'
ULTNFS410	ULTINET410	0	'Network File System Utilities'
ULTUMAIL410	ULTINET410	0	'Extended (Berkeley) Mailer'
ULTDL410	.	0	'Diskless Support Environment'
ULTPRINT410	.	2	'Printer Support Environment'
ULTACCT410	.	2	'Accounting Software'
ULTPGMR410	.	2	'Software Development Utilities'
ULTINTL410	ULTPGMR410	2	'Internationalization Tools'
ULTSCCS410	.	2	'Source Code Control System'
ULTPASCAL410	.	2	'Pascal Development Package'
ULTDCMT410	.	2	'Document Preparation Software'
ULTAFM410	.	2	'Adobe Font Metric Files'
ULTMH410	ULTUMAIL410	2	'RAND Mail Handler'
ULTCOMM410	ULTINET410	2	'Communications Utilities'
ULTRPCRT410	.	2	'RPC Runtime Environment'
ULTRPCDEV410	ULTPGMR410	2	'RPC Development Environment'
ULTKERB410	.	2	'Kerberos Network Authentication'
ULTSEC410	.	2	'Enhanced Security Features'
ULTMOP410	.	2	'Maintenance Operations Protocol'
ULTUUCP410	ULTCOMM410	2	'Unix-to-Unix Copy Facility'
ULTVAXC410	.	2	'VAX C/ULTRIX'
ULTEXER410	.	2	'System Exerciser Package'
ULTBSC410	.	2	'Bisynchronous Communications'
ULTMAN410	ULTDCMT410	2	'On Line Manual Pages'
