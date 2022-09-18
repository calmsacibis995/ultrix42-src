#	ULT420.k -
#		ULTRIX-32 V4.2 Supported (VAX) Mfg Key File
#
#       @(#)ULT420.k	4.9	 ULTRIX	3/13/91
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
VERS=420
MI=ULT420.mi
ROOT=1
RXMAKE=0
COMPRESS=1

#% SUBSET-LEVEL ATTRIBUTES
%%
ULTBASE420	.	1	'Base System'
ULTBIN420	.	0	'Kernel Configuration Files'
ULTINET420	.	0	'TCP/IP Networking Utilities'
ULTNFS420	ULTINET420	0	'Network File System Utilities'
ULTUMAIL420	ULTINET420	0	'Extended (Berkeley) Mailer'
ULTDL420	.	0	'Diskless Support Environment'
ULTPRINT420	.	2	'Printer Support Environment'
ULTMH420        ULTUMAIL420     2       'RAND Mail Handler'
ULTACCT420	.	2	'Accounting Software'
ULTPGMR420	.	2	'Software Development Utilities'
ULTINTLRT420	.	2	'Internationalization Runtime Env.'
ULTINTLPGMR420	ULTPGMR420	2	'Internationalization Dev. Env.'
ULTCDABASE420	.	2	'CDA Base System'
ULTCDAPGMR420	ULTPGMR420	2	'CDA Software Dev Libraries'
ULTSCCS420	.	2	'Source Code Control System'
ULTPASCAL420	.	2	'Pascal Development Package'
ULTDCMTMAN420	.	2	'Doc. Preparation for Ref. Pages'
ULTDCMT420	ULTDCMTMAN420 	2	'Other Doc. Preparation Software'
ULTAFM420	.	2	'Adobe Font Metric Files'
ULTCOMM420	ULTINET420	2	'Communications Utilities'
ULTRPCRT420	.	2	'RPC Runtime Environment'
ULTRPCDEV420	ULTPGMR420	2	'RPC Development Environment'
ULTKERB420	.	2	'Kerberos Network Authentication'
ULTSEC420	.	2	'Enhanced Security Features'
ULTSMSCAMP420   .       2       'Sys. Config. Mgmt. Program (SCAMP)'
ULTPRESTO420    .       2       'Prestoserve Utilities'
ULTMOP420	.	2	'Maintenance Operations Protocol'
ULTUUCP420	ULTCOMM420	2	'Unix-to-Unix Copy Facility'
ULTVAXC420	.	2	'VAX C/ULTRIX'
ULTEXER420	.	2	'System Exerciser Package'
ULTBSC420	.	2	'Bisynchronous Communications'
ULTMAN420	ULTDCMTMAN420	2	'Ref. Pages for Sys. Admin. & Users'
ULTMANPGMR420	ULTDCMTMAN420	2	'Reference Pages for Programmers'
ULTEXAMPLES420	.	2	'Programming Examples'
ULTFIS420	.	.i	'FIS Client Tools - DO NOT SHIP'
