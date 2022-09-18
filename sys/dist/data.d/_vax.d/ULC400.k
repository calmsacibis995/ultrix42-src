#
#		ULTRIX Encryption (VAX) V4.0 Manufacturing Key File
#
#	@(#)ULC400.k	4.1	(ULTRIX)	7/2/90
#
#	003	30-aug-1989	tresvik
#	remove manpage subset
#
#	002	02-mar-1989	ccb
#	Import from VAX V3.0 Sources.
#	Update revision information for V4.0
#
#	001	jon	18-apr-1988
#	added ULCPGMR entry
#

#% PRODUCT-LEVEL ATTRIBUTES
NAME='ULTRIX-32 V4.0 (VAX) Encryption Package'
CODE=ULC
VERS=400
MI=ULT400.mi
ROOT=0
RXMAKE=0

#% PRODUCT-LEVEL ATTRIBUTES
%%
ULCCRYPT400	ULTBASE400	3	'DES Encryption Capabilities'
ULCPGMR400	ULTPGMR400	3	'DES Encryption Libraries'
