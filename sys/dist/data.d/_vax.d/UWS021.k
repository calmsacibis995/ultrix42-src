#	UWS021.k -
#		ULTRIX WS V3.0 (VAX) Manufacturing Key File
#
#	@(#)UWS021.k	4.1	(ULTRIX)	7/2/90
#
#	000	02-mar-1989	ccb
#	Copy from V3.0 (VAX) Sources
#	Revise versions from V3.0 to V4.0 and V2.0 to V3.0
#


#% PRODUCT-LEVEL ATTRIBUTES
NAME='ULTRIX Worksystem Software V3.0'
CODE=ULT
VERS=040
MI=ULT040.mi
ROOT=1
RXMAKE=0
COMPRESS=1

%%
#% SUBSET-LEVEL ATTRIBUTES
ULTBASE040	.	1	'Base System'
ULTBIN040	.	0	'Kernel Configuration Files'
ULTINET040	.	0	'TCP/IP Networking Utilities'
ULTEXER040	.	0	'System Exerciser Package'
ULTNFS040	ULTINET040	0	'Network File System Utilities'
ULTUMAIL040	ULTINET040	0	'Extended (Berkeley) Mailer'
UWSX11021	ULTINET040	0	'X11/DECwindows User Environment'
UWSFONT021	UWSX11021	0	'X11/DECwindows 75dpi Fonts'
ULTDL040	.	0	'Diskless Support Environment'
UWSFONT15021	.	2	'X11/DECwindows 100dpi Fonts'
UWSDECW021	.	2	'Optional DECwindows Applications'
UWSXCOMP021	.	2	'X10 to X11 Compatibility Tools'
ULTMH040	ULTUMAIL040	2	'The RAND Mail Handler'
ULTDCMT040	.	2	'Document Preparation Software'
ULTAFM040	.	2	'Adobe Font Metric Files'
ULTPGMR040	.	2	'Software Development Utilities'
ULTINTL040	ULTPGMR040	2	'Internationalization Tools'
ULTSCCS040	.	2	'Source Code Control System'
UWSXDEV021	ULTPGMR040	2	'Worksystem Development Software'
ULTPASCAL040	.	2	'Pascal Development Package'
ULTVAXC040	.	2	'VAX C/ULTRIX'
ULTMAN040	ULTDCMT040	2	'On Line Manual Pages'
UWSMAN021	ULTDCMT040	2	'UWS Manual Pages'
ULTACCT040	.	2	'Accounting Software'
ULTCOMM040	ULTINET040	2	'Communications Utilities'
ULTRPCRT040	.	2	'RPC Runtime Environment'
ULTRPCDEV040	ULTPGMR040	2	'RPC Development Environment'
ULTKERB040	.	2	'Kerberos Network Authentication'
ULTSEC040	.	2	'Enhanced Security Features'
ULTBSC040	.	2	'Bisynchronous Communications'
ULTMOP040	.	2	'Maintenance Operations Protocol'
ULTUUCP040	ULTCOMM040	2	'Unix-to-Unix Copy Facility'
