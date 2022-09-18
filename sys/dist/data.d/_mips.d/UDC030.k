#	UDC040.k -
#		ULTRIX-32 V4.0 Encryption (RISC) Mfg Key File
#
#	"@(#)UDC030.k	4.1 (ULTRIX) 7/2/90"
#
#	002	02-mar-1989	ccb
#	Rev up to V4.0 after migration to new pool.
#
#	001	jon	18-apr-1988
#	added ULCPGMR entry
#

#% PRODUCT-LEVEL ATTRIBUTES
NAME='ULTRIX-32 V4.0 Encryption Package (RISC)'
CODE=UDC
VERS=040
MI=UDT040.mi
ROOT=0
RXMAKE=0

%%
#% SUBSET-LEVEL ATTRIBUTES
UDCCRYPT040	UDTBASE040	3	'DES Encryption Capabilities'
UDCPGMR030	UDTPGMR040	3	'DES Encryption Libraries'
UDCMAN040	UDTMAN040	2	'DES Encryption Manual Pages'
