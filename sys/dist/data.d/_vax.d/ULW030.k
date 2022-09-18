#	ULW030.k -
#		ULTRIX WS V3.0 Server 2/2 (VAX) Mfg Key File
#
#	"@(#)ULW030.k	4.1 (ULTRIX) 7/2/90"
#
#	000	02-mar-1989	ccb
#	Copy from V2.0 (VAX) Sources
#	Revsion update for V3.0

#% PRODUCT-LEVEL ATTRIBUTES
NAME='ULTRIX Worksystem Software V2.0'
CODE=UWS
VERS=021
MI=ULT040.mi
ROOT=0
RXMAKE=0
COMPRESS=1

%%
#% SUBSET-LEVEL ATTRIBUTES
UWSX11021	ULTINET040	0	'X11/DECwindows User Environment'
UWSFONT021	UWSX11021	0	'X11/DECwindows 75dpi Fonts'
UWSFONT15021	.	2	'X11/DECwindows 100dpi Fonts'
UWSDECW021	.	2	'Optional DECwindows Applications'
UWSXCOMP021	.	2	'X10 to X11 Compatibility Tools'
UWSXDEV021	ULTPGMR040	2	'Worksystems Development Software'
UWSMAN021	ULTDCMT040	2	'UWS Manual Pages'
