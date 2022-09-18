#	UDT021.k -
#		ULTRIX WS V3.0 (RISC) Manufacturing Key File
#
#	@(#)UDT021.k	4.1	(ULTRIX)	7/2/90
#
#	000	02-mar-1989	ccb
#	Copied in from V3.0 mips source pool,
#	revision revised for V4.0

#% PRODUCT-LEVEL ATTRIBUTES
NAME='ULTRIX Worksystem Software (RISC) T2.1'
CODE=UDT
VERS=040
MI=UDT040.mi
ROOT=1
RXMAKE=0
COMPRESS=1

%%
#% SUBSET-LEVEL ATTRIBUTES
UDTBASE040	.	1	'Base System'
UDTBIN040	.	0	'Kernel Configuration Files'
UDTINET040	.	0	'TCP/IP Networking Utilities'
UDTEXER040	.	0	'System Exerciser Package'
UDTNFS040	UDTINET040	0	'Network File System Utilities'
UDTUMAIL040	UDTINET040	0	'Extended (Berkeley) Mailer'
UDWX11021	UDTINET040	0	'X11/DECwindows User Environment'
UDWFONT021	UDWX11021	0	'X11/DECwindows 75dpi Fonts'
UDTDL040	.	0	'Diskless Support Environment'
UDTMH040	UDTUMAIL040	2	'The RAND Mail Handler'
UDWDECW021	.	2	'Optional DECwindows Applications'
UDWFONT15021	.	2	'X11/DECwindows 100dpi Fonts'
UDTDCMT040	.	2	'Document Preparation Software'
UDTAFM040	.	2	'Adobe Font Metric Files'
UDTPGMR040	.	2	'Software Development Utilities'
UDTINTL040	UDTPGMR040	2	'Internationalization Tools'
UDTSCCS040	.	2	'Source Code Control System'
UDWXDEV021	UDTPGMR040	2	'Worksystem Development Software'
UDTACCT040	.	2	'Accounting Software'
UDTCOMM040	UDTINET040	2	'Communications Utilities'
UDTMOP040	.	2	'Maintenance Operations Protocol'
UDTUUCP040	UDTCOMM040	2	'Unix-to-Unix Copy Facility'
UDWXCOMP021	.	2	'X10 to X11 Compatibility Tools'
UDTMAN040	UDTDCMT040	2	'On Line Manual Pages'
