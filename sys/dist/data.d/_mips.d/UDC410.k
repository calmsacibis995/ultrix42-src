#
#		ULTRIX V4.1 (RISC) Encryption Mfg Key File
#
#	@(#)UDC410.k	4.1	(ULTRIX) 	7/17/90
#
#	004	17-jul-1990	mdf
#	revision update for V4.1
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
NAME='ULTRIX V4.1 (RISC) Encryption Package'
CODE=UDC
VERS=410
MI=UDT410.mi
ROOT=0
RXMAKE=0

#% SUBSET-LEVEL ATTRIBUTES
%%
UDCCRYPT410	UDTBASE410	3	'DES Encryption Capabilities'
UDCPGMR410	UDTPGMR410	3	'DES Encryption Libraries'
