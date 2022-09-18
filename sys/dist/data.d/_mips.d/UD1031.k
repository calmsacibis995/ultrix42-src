# ULTRIX Worksystem software manufacturing key file.
#
#	@(#)UD1031.k	1.2	(ULTRIX)	11/9/89
#
# ULTRIX Worksystem Software product for TK50
#
# global (product-wide) definitions
#
NAME='ULTRIX-32 V3.1 SUPPORTED BASE (RISC)'
CODE=UDT
VERS=031
MI=ULTRIX.mi
ROOT=1
RXMAKE=0
COMPRESS=1
# subset definitions follow
%%
UDTBASE031	.	1	'Base System'
UDTBIN031	.	0	'Kernel Configuration Files'
UDTINET031	.	0	'TCP/IP Networking Utilities'
UDTEXER031	.	0	'System Exerciser Package'
UDTNFS031	UDTINET031	0	'Network File System Utilities'
UDTUMAIL031	UDTINET031	0	'Extended (Berkeley) Mailer'
UDTDL031	.	0	'Diskless Support Environment'
UDTMH031	UDTUMAIL031	2	'The RAND Mail Handler'
UDTDCMT031	.	2	'Document Preparation Software'
UDTAFM031	.	2	'Adobe Font Metric Files'
UDTPGMR031	.	2	'Software Development Utilities'
UDTINTL031	UDTPGMR031	2	'Internationalization Tools'
UDTSCCS031	.	2	'Source Code Control System'
UDTACCT031	.	2	'Accounting Software'
UDTCOMM031	UDTINET031	2	'Communications Utilities'
UDTMOP031	.	2	'Maintenance Operations Protocol'
UDTUUCP031	UDTCOMM031	2	'Unix-to-Unix Copy Facility'
UDTMAN031	UDTDCMT031	2	'On Line Manual Pages'
