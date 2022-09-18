# @(#)Makelocal.mk	4.6	(ULTRIX)	1/3/91
#
include $(GMAKEVARS)

DB_FILES= ../oitem.db ../sitem.db ../sseg.db ../hdr.db ../fddi.db \
	../kn230.db ../xbi_plus.db ../xma2.db ../mariah.db ../summary.db \
	../kn02ba.db ../xja.db ../vectors.db \
	../aqclk.db ../aqbi.db ../aqscan.db ../aqpcs.db ../aqpcst.db \
	../aqhese.db ../aqsyn.db ../aq.db ../aqmckspu.db ../aqmck.db \
	../xbia.db ../xvib.db ../version.db 

all: uerfdbc dsd_build uerf.bin

data uerf.bin: os_dsd.h std_dsd.h
	dsd_build

os_dsd.h std_dsd.h ultrix_dsd.bin: $(DB_FILES)
	uerfdbc -a -j $(DB_FILES)

dsd_build.o: ultrix_dsd.bin os_dsd.h std_dsd.h generic_dsd.h

dsd_build:	dsd_build.o
		${CC} dsd_build.o -o dsd_build

uerfdbc:	uerfdbc.o
		${CC} uerfdbc.o -o uerfdbc

uerfdbc.o: uerfdbc.c
dsd_build.o: dsd_build.c

include $(GMAKERULES)
