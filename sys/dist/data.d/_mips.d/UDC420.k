#
#		ULTRIX V4.2 (RISC) Encryption Mfg Key File
#
#	@(#)UDC420.k	4.1	(ULTRIX) 	12/3/90
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
NAME='ULTRIX V4.2 (RISC) Encryption Package'
CODE=UDC
VERS=420
MI=UDT420.mi
ROOT=0
RXMAKE=0

#% SUBSET-LEVEL ATTRIBUTES
%%
UDCCRYPT420	UDTBASE420	3	'DES Encryption Capabilities'
UDCPGMR420	UDTPGMR420	3	'DES Encryption Libraries'
