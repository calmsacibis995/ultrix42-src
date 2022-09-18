#		ULTRIX V4.1 Supported (RISC) Manufacturing Key File
#
#	@(#)UDT420.k	4.11	ULTRIX	4/4/91	
#
#	002	19-feb-1991	overman
#	added new subset name and descriptions
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
MI=UDT420.mi
ROOT=1
RXMAKE=0
COMPRESS=1

#% SUBSET-LEVEL ATTRIBUTES
%%
UDTBASE420	.	1	'Base System'
UDTBIN420	.	0	'Kernel Configuration Files'
UDTINET420	.	0	'TCP/IP Networking Utilities'
UDTNFS420	UDTINET420	0	'Network File System Utilities'
UDTUMAIL420	UDTINET420	0	'Extended (Berkeley) Mailer'
UDTDL420	.	0	'Diskless Support Environment'
UDTPRINT420	.	2	'Printer Support Environment'
UDTMH420	UDTUMAIL420	2	'RAND Mail Handler'
UDTDCMT420	.	2	'Doc. Preparation for Ref. Pages'
UDTDCMTEXT420	UDTDCMT420	2	'Doc. Preparation Extensions'
UDTAFM420	.	2	'Adobe Font Metric Files'
UDTPGMR420	.	2	'Software Development Utilities'
UDTINTLRT420	.	2	'Internationalization Runtime Env.'
UDTINTLPGMR420	UDTPGMR420	2	'Internationalization Dev. Env.'
UDTCDABASE420	.	2	'CDA Base System'
UDTCDAPGMR420	UDTPGMR420	2	'CDA Software Dev Libraries'
UDTSCCS420	.	2	'Source Code Control System'
UDTKERB420	.	2	'Kerberos Network Authentication'
UDTSEC420	.	2	'Enhanced Security Features'
UDTSMSCAMP420   .       2       'Sys. Config. Mgmt. Program (SCAMP)'
UDTPRESTO420    .       2       'Prestoserve Utilities'
UDTRPCRT420	.	2	'RPC Runtime Environment'
UDTRPCDEV420	UDTPGMR420	2	'RPC Development Environment'
UDTACCT420	.	2	'Accounting Software'
UDTCOMM420	UDTINET420	2	'Communications Utilities'
UDTMOP420	.	2	'Maintenance Operations Protocol'
UDTUUCP420	UDTCOMM420	2	'Unix-to-Unix Copy Facility'
UDTEXER420	.	2	'System Exerciser Package'
UDTMAN420	UDTDCMT420	2	'Ref. Pages for Sys. Admin. & Users'
UDTMANPGMR420	UDTDCMT420	2	'Reference Pages for Programmers'
UDTEXAMPLES420	.	2	'Programming Examples'
UDTFIS420	.	2	'FIS Client Tools'
UDTFISMFG420	.	2	'FIS Manufacturing Tools'
