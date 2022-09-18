#
#	@(#)make.sas	4.3 (ULTRIX) 3/8/91;
#
# ------------------------------------------------------------------------
# This make file is used to make the standalone kit images used for
# installation.
#
# Contents of kits:
#
# The size of all components of the kit must be a multiple of 512 bytes,
# except, of course, the TM (tape mark).  The kit is non file
# structured and layed out as follows.
#
# `ultrixload' `[TM]' `descriptor block' `[vmb.exe]' `[compressed] vmunix'
#
#	Note the following options:
#
#		[TM] (tape mark) applies only to a TK50 kit
#		[vmb.exe] is needed only for LYNX TK50 kits with V2.4
#		vmunix image is not compressed for TK50 kit
#
#	The 512 byte descriptor block contains:
#		bytes 0-31 = a.out header - struct exec
#		long @ 32  = size of the media     (80000000 to adb)
#		long @ 36  = size of vmb.exe       (80000004 to adb)
#		long @ 40  = 1 = vmunix compressed (80000008 to adb)
#			   = 0 = vmunix not compressed
#	
#	The vmunix image must contain a memory file system image,
#	installed with ${FSMRG}.
#
# ------------------------------------------------------------------------
#
# Modification History: /sys/sas/Makefile
#
# ------------------------------------------------------------------------

MASTERROOT=/
DISTROOT=/
TARGETDIR=./
#
# standalone images
#
VMUNIX.GEN=${MASTERROOT}/usr/sys/VAX/SAS.gen/vmunix
VMUNIX.RX01=${MASTERROOT}/usr/sys/VAX/SAS.rx01/vmunix
VMUNIX.TU58=${MASTERROOT}/usr/sys/VAX/SAS.tu58/vmunix
VMB=${MASTERROOT}/usr/mdec/vmb.exe
LOADER=${MASTERROOT}/usr/mdec/ultrixload
#MEMFS=../nfs

#
# Standalone kit building tools
#
#
COMPRESS=${DISTROOT}/usr/ucb/compress
RXINTLV=${DISTROOT}/sys/dist/rxintlv
FSMRG=${DISTROOT}/sys/dist/fsmrg

#
# Size of medium.  This number is plugged into the descriptor block at
# 32 off the beginning (or 0x80000000 when using adb).
#
RX50=800
RL02=20480
RX01=494
TU58=512
TK50=10000

ALL=	RL02.1 \
	TU58.1 TU58.2 TU58.3 TU58.4 \
	RX01.1 RX01.2 RX01.3 RX01.4 \
	RX50.1 RX50.2 RX50.3 \
	TK50.1A TK50.1B 

all: ${ALL} 

install: all
	install -c -m 440 ${ALL} ${TARGETDIR}

clean:
	rm -f ${ALL} errs
	rm -f vmb.exe descriptor.*
	rm -f vmunix.tmp 
	rm -f vmunix.tu58 vmunix.tu58.C
	rm -f vmunix.rx01 vmunix.rx01.C
	rm -f vmunix.gen vmunix.gen.C

vmunix.rx01 vmunix.rx01.C descriptor.rx01: ${VMUNIX.RX01}
	dd if=${VMUNIX.RX01} of=vmunix.tmp bs=8192
	${FSMRG} ${MEMFS} vmunix.tmp
	strip vmunix.tmp
	dd if=vmunix.tmp bs=32 count=1 | dd of=descriptor.rx01 bs=512 conv=sync
	dd if=vmunix.tmp of=vmunix.rx01 ibs=32 obs=8192 skip=1
	rm -f vmunix.tmp
	${COMPRESS} < vmunix.rx01 > vmunix.rx01.C

vmunix.tu58 vmunix.tu58.C descriptor.tu58: ${VMUNIX.TU58}
	dd if=${VMUNIX.TU58} of=vmunix.tmp bs=8192
	${FSMRG} ${MEMFS} vmunix.tmp
	strip vmunix.tmp
	dd if=vmunix.tmp bs=32 count=1 | dd of=descriptor.tu58 bs=512 conv=sync
	dd if=vmunix.tmp of=vmunix.tu58 ibs=32 obs=8192 skip=1
	rm -f vmunix.tmp
	${COMPRESS} < vmunix.tu58 > vmunix.tu58.C

vmunix.gen vmunix.gen.C descriptor.gen: ${VMUNIX.GEN}
	dd if=${VMUNIX.GEN} of=vmunix.tmp bs=8192
	${FSMRG} ${MEMFS} vmunix.tmp
	strip vmunix.tmp
	dd if=vmunix.tmp bs=32 count=1 | dd of=descriptor.gen bs=512 conv=sync
	dd if=vmunix.tmp of=vmunix.gen ibs=32 obs=8192 skip=1
	rm -f vmunix.tmp
	${COMPRESS} < vmunix.gen > vmunix.gen.C

vmb.exe: ${VMB}
	dd if=${VMB} of=vmb.exe conv=sync

#
# ------------------------------------------------------------------------
#
# Kit building rules
#

#
# Make a TK50 kit for the MVAX II and VAXstar
#	kit contents:
#		ultrixload
#		(tape mark)
#		descriptor block
#		vmunix	(uncompressed)
#
TK50.1A TK50.1B: ${LOADER} vmb.exe vmunix.gen descriptor.gen
	@echo "Making TK50 kit"
	cp descriptor.gen /tmp/sas.descriptor
	echo "80000000?W 0t${TK50}" > /tmp/sas.adb.cmd
	set F`ls -l vmb.exe`; \
	SIZE=$$4; \
	SIZE=`expr $$SIZE / 512`; \
	echo "80000004?W 0t$$SIZE" >> /tmp/sas.adb.cmd
	echo "80000008?W 0" >> /tmp/sas.adb.cmd
	adb -w /tmp/sas.descriptor < /tmp/sas.adb.cmd 
	cp ${LOADER} TK50.1A
	cat /tmp/sas.descriptor vmb.exe vmunix.gen > TK50.1B
	rm -f /tmp/sas.adb.cmd /tmp/sas.descriptor 
	chmod 644 TK50.1A TK50.1B

#
# ------------------------------------------------------------------------
#
# VAX kit building rules
#

#
# Make an RL02 kit for the VAX 8600 and VAX 8650
#	kit contents:
#		ultrixload
#		descriptor block
#		vmunix	(compressed)
#
RL02.1: ${LOADER} vmunix.gen.C descriptor.gen
	@echo "Making RL02 kit"
	cp descriptor.gen /tmp/sas.descriptor
	echo "80000000?W 0t${RL02}" > /tmp/sas.adb.cmd
	echo "80000004?W 0" >> /tmp/sas.adb.cmd
	echo "80000008?W 1" >> /tmp/sas.adb.cmd
	adb -w /tmp/sas.descriptor < /tmp/sas.adb.cmd 
	cat ${LOADER} /tmp/sas.descriptor vmunix.gen.C > RL02.1
	rm -f /tmp/sas.adb.cmd /tmp/sas.descriptor
	chmod 644 RL02.1

#
# Make an generic RX50 kit
#	kit contents:
#		ultrixload
#		descriptor block
#		vmunix	(compressed)
#
RX50.1 RX50.2 RX50.3: ${LOADER} vmunix.gen.C descriptor.gen
	@echo "Making RX50 kit"
	cp descriptor.gen /tmp/sas.descriptor
	echo "80000000?W 0t${RX50}" > /tmp/sas.adb.cmd
	echo "80000004?W 0" >> /tmp/sas.adb.cmd
	echo "80000008?W 1" >> /tmp/sas.adb.cmd
	adb -w /tmp/sas.descriptor < /tmp/sas.adb.cmd 
	cat ${LOADER} /tmp/sas.descriptor vmunix.gen.C > image
	dd if=image of=RX50.1 count=${RX50} conv=sync
	dd if=image of=RX50.2 skip=${RX50} count=${RX50} conv=sync
	dd if=image of=RX50.3 skip=1600 count=${RX50} conv=sync
	rm -f /tmp/sas.adb.cmd /tmp/sas.descriptor image
	chmod 644 RX50.1 RX50.2 RX50.3
	
#
# Make an RX01 kit for the VAX 11/780 and VAX 11/785
#	kit contents:
#		ultrixload
#		descriptor block
#		vmunix	(compressed)
#
RX01.1 RX01.2 RX01.3 RX01.4: ${LOADER} vmunix.rx01.C descriptor.rx01
	@echo "Making RX01 kit"
	cp descriptor.rx01 /tmp/sas.descriptor
	echo "80000000?W 0t${RX01}" > /tmp/sas.adb.cmd
	echo "80000004?W 0" >> /tmp/sas.adb.cmd
	echo "80000008?W 1" >> /tmp/sas.adb.cmd
	adb -w /tmp/sas.descriptor < /tmp/sas.adb.cmd 
	cat ${LOADER} /tmp/sas.descriptor vmunix.rx01.C > image
	dd if=image of=rximage count=${RX01} conv=sync
	${RXINTLV} rximage RX01.1
	dd if=image of=rximage skip=${RX01} count=${RX01} conv=sync
	${RXINTLV} rximage RX01.2
	dd if=image of=rximage skip=988 count=${RX01} conv=sync
	${RXINTLV} rximage RX01.3
	dd if=image of=rximage skip=1482 count=${RX01} conv=sync
	${RXINTLV} rximage RX01.4
	rm -f /tmp/sas.adb.cmd /tmp/sas.descriptor rximage image
	chmod 644 RX01.1 RX01.2 RX01.3 RX01.4
	
#
# Make a TU58 kit for the VAX 11/750 and the VAX 11/730
#	kit contents:
#		ultrixload
#		descriptor block
#		vmunix	(compressed)
#
TU58.1 TU58.2 TU58.3 TU58.4: ${LOADER} vmunix.tu58.C descriptor.tu58
	@echo "Making TU58 kit"
	cp descriptor.tu58 /tmp/sas.descriptor
	echo "80000000?W 0t${TU58}" > /tmp/sas.adb.cmd
	echo "80000004?W 0" >> /tmp/sas.adb.cmd
	echo "80000008?W 1" >> /tmp/sas.adb.cmd
	adb -w /tmp/sas.descriptor < /tmp/sas.adb.cmd 
	cat ${LOADER} /tmp/sas.descriptor vmunix.tu58.C > image
	dd if=image of=TU58.1 count=${TU58} conv=sync
	dd if=image of=TU58.2 skip=${TU58} count=${TU58} conv=sync
	dd if=image of=TU58.3 skip=1024 count=${TU58} conv=sync
	dd if=image of=TU58.4 skip=1536 count=${TU58} conv=sync
	rm -f /tmp/sas.adb.cmd /tmp/sas.descriptor image
	chmod 644 TU58.1 TU58.2 TU58.3 TU58.4
