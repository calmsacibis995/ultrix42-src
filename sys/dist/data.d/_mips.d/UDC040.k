#
#		ULTRIX-32 V4.0 (RISC) Encryption Mfg Key File
#
#	"@(#)UDC040.k	4.1 (ULTRIX) 7/2/90"
#
#	003	30-aug-1989	tresvik
#	remove manpage subset
#
#	002	02-mar-1989	ccb
#	Rev up to V4.0 after migration to new pool.
#
#	001	jon	18-apr-1988
#	added ULCPGMR entry
#

#% PRODUCT-LEVEL ATTRIBUTES
NAME='ULTRIX-32 V4.0 (RISC) Encryption Package'
CODE=UDC
VERS=040
MI=UDT040.mi
ROOT=0
RXMAKE=0

#% SUBSET-LEVEL ATTRIBUTES
%%
UDCCRYPT040	UDTBASE040	3	'DES Encryption Capabilities'
UDCPGMR040	UDTPGMR040	3	'DES Encryption Libraries'
