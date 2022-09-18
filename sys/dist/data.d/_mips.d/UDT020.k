# ULTRIX Worksystem software manufacturing key file.
#
#	@(#)UDT020.k	1.1	(ULTRIX)	6/6/89
#
# ULTRIX Worksystem Software product for TK50
#
# global (product-wide) definitions
#
NAME='ULTRIX Worksystem Software (RISC) V2.0'
CODE=UDT
VERS=030
MI=ULTRIX.mi
ROOT=1
RXMAKE=0
COMPRESS=1
# subset definitions follow
%%
UDTBASE030	.	1	'Base System'
UDTBIN030	.	0	'Kernel Configuration Files'
UDTINET030	.	0	'TCP/IP Networking Utilities'
UDTEXER030	.	0	'System Exerciser Package'
UDTNFS030	UDTINET030	0	'Network File System Utilities'
UDTUMAIL030	UDTINET030	0	'Extended (Berkeley) Mailer'
UDWX11020	UDTINET030	0	'X11/DECwindows User Environment'
UDWFONT020	UDWX11020	0	'X11/DECwindows 75dpi Fonts'
UDTMH030	UDTUMAIL030	2	'The RAND Mail Handler'
UDWDECW020	.	2	'Optional DECwindows Applications'
UDWFONT15020	.	2	'X11/DECwindows 100dpi Fonts'
UDTDCMT030	.	2	'Document Preparation Software'
UDTAFM030	.	2	'Adobe Font Metric Files'
UDTPGMR030	.	2	'Software Development Utilities'
UDTINTL030	UDTPGMR030	2	'Internationalization Tools'
UDTSCCS030	.	2	'Source Code Control System'
UDWXDEV020	UDTPGMR030	2	'Worksystem Development Software'
UDTACCT030	.	2	'Accounting Software'
UDTCOMM030	UDTINET030	2	'Communications Utilities'
UDTMOP030	.	2	'Maintenance Operations Protocol'
UDTUUCP030	UDTCOMM030	2	'Unix-to-Unix Copy Facility'
UDWXCOMP020	.	2	'X10 to X11 Compatibility Tools'
UDTMAN030	UDTDCMT030	2	'On Line Manual Pages'
