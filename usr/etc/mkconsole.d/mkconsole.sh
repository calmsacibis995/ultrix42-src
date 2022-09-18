#!/bin/sh
# SCCSID = @(#)mkconsole.sh	1.2	(ULTRIX)	2/27/89
# ************************************************************************
# *									*
# *			Copyright (c) 1987 by				*
# *		Digital Equipment Corporation, Maynard, MA		*
# *			All rights reserved.				*
# *									*
# *   This software is furnished under a license and may be used and	*
# *   copied  only  in accordance with the terms of such license and	*
# *   with the  inclusion  of  the  above  copyright  notice.   This	*
# *   software  or  any  other copies thereof may not be provided or	*
# *   otherwise made available to any other person.  No title to and	*
# *   ownership of the software is hereby transferred.			*
# *									*
# *   The information in this software is subject to change  without	*
# *   notice  and should not be construed as a commitment by Digital	*
# *   Equipment Corporation.						*
# *									*
# *   Digital assumes no responsibility for the use  or  reliability	*
# *   of its software on equipment which is not supplied by Digital.	*
# *									*
# ************************************************************************
# Modification History
# ~~~~~~~~~~~~~~~~~~~~ 
# 002 - Feb. 27, 1989 - Tungning Cherng
#	Merge to the new pool.
# 001 - Jan 18, 1988 - Tung-Ning Cherng
#	Added sca portclass support.
# 000 - Apr 8, 1987 - Tung-Ning Cherng
# 		

VMUNIX=/vmunix
case $# in
1 ) 
	[ -f $1 ] || 
	{
		echo "$1 doesn't exist."
		exit 1
	}
	VMUNIX=$1
	;;
esac	
	
FAIL="Console update failed !!!"
NOUPDATE="Console update is not required."

PWD=`pwd`
CPU=`/etc/sizer -c -k $VMUNIX` || exit 1
case $CPU in
VAX780 )
	STDBOO="cira.cmd mbahp.cmd ubara.cmd cnsl.cmd"
	FROMDEC="du* dm* db* s[mbr]*.cmd *boo.cmd *gen *xdt boot \
		copy rabads format"
	FLOPPY="/dev/floppy"
	while :
	do
		echo "
Remove the RX01 diskette from the inside drive. 
Replace it with the RX01 diskette that was supplied with the processor.

Press the RETURN key when you are ready to continue. " 

		read ans 
		case $ans in
		"" )	break ;;
		* )	;;	
		esac	
	done
	cd /sys/consoles/vax/780cons ||  exit 1 
	/etc/sizer -b -k ${VMUNIX} || 
	{ 
		echo $FAIL; exit 1 
	}
	cp /usr/mdec/vmb.exe vmb.exe 
	cp /usr/mdec/ci780.bin ci780.bin 
	rm -rf fromdec floppy
	mkdir fromdec
	cd fromdec 
	echo "
Extracting files from the console diskette.
This takes several minutes." 
	dd if=${FLOPPY} of=../floppy bs=128x26x3 count=1  ||
	{
		 echo $FAIL; exit 1 
	}
	/etc/arff xf ${FLOPPY} 
	rm -f ${FROMDEC}
	echo -n "
Remove the RX01 console diskette.  Insert a blank RX01 diskette.

Press the RETURN key when you are ready to continue. " 
	read ans
	/etc/arff if ../floppy 494,1,ULTRIX-32,Console, 
	echo "
Building console diskette for ULTRIX. 
This takes several minutes." 
	/etc/arff rf ../floppy * 
	cd ..
	rm -rf fromdec
	/etc/arff rf ./floppy ${STDBOO} defboo.cmd askboo.cmd vmb.exe ci780.bin
	dd if=./floppy of=${FLOPPY} conv=sync ||
	{
		 echo $FAIL; exit 1 
	}
	echo "Directory listing of the new console diskette follows."
	/etc/arff tvf ${FLOPPY}
	;;
VAX8600 )
	STDBOO="defboo.com askboo.com cira.com ubara.com mbahp.com cnsl.com"
	cd /sys/consoles/vax/8600cons || exit 1 
	/etc/sizer -b -k ${VMUNIX} ||
	{
		 echo $FAIL; exit 1 
	}
	echo "Updating console RL02."
	arff rmvf /dev/crl ${STDBOO}
	;;
VAX750 )
	/etc/sizer -b -k ${VMUNIX} 
	case $? in
	0 ) 	;;
	2 )	
		echo $NOUPDATE; exit 2
		;;
	* )
		echo $FAIL; exit 1
		;;
	esac
	STDBOO="cira.cmd cnsl.cmd"
	CASSETTE="/dev/tu0"
	cd /sys/consoles/vax/750cons || exit 1
	# install ULTRIX commands files
	cp /usr/mdec/vmb.exe vmb.exe 
	cp /usr/mdec/ci780.bin ci780.bin 
	cp /usr/mdec/pcs750.bin pcs750.bin 
	/etc/arff rmf 750cons defboo.cmd askboo.cmd ${STDBOO} vmb.exe ci780.bin pcs750.bin
	echo -n "
Remove the TU58 cassette from the drive.  Insert a
blank TU58 cassette.  Make sure the cassette is write-enabled.

Press the RETURN key when you are ready to continue. " 
	read ans
	echo "
Building console cassette for ULTRIX.
This takes several minutes."
	dd if=750cons of=$CASSETTE bs=8k conv=sync ||
	{
		echo $FAIL; exit 1
	}
	;;
VAX8200 )
	/etc/sizer -b -k ${VMUNIX} ||
	case $? in
	0 ) 	;;
	2 )	
		echo $NOUPDATE; exit 2
		;;
	* )
		echo $FAIL; exit 1
		;;
	esac
	STDBOO="cira.cmd cnsl.cmd"
	FLOPPY="/dev/rcs1a"
	cd /sys/consoles/vax/8200cons || exit 1
	# install ULTRIX commands files
	cp /usr/mdec/vmb.exe vmb.exe 
	cp /usr/mdec/ci780.bin ci780.bin 
	cp /usr/mdec/cibca.bin cibca.bin  
	/etc/arff rmf 8200cons defboo.cmd askboo.cmd ${STDBOO} vmb.exe ci780.bin cibca.bin
	echo -n "
Remove the RX50 diskette from the left drive.  Insert a blank RX50
diskette in the same drive.  Make sure the diskette is write-enabled.

Press the RETURN key when you are ready to continue. " 
	read ans
	dd if=8200cons of=$FLOPPY bs=10b conv=sync ||
	{
		echo $FAIL; exit 1
	}
	;;
VAX730 )
	STDBOO="ubara.cmd ubaidc.cmd cnsl.cmd"
	# order is NOT random, it makes a big difference in how long it takes
	# to reboot and to reload microcode on power-up.
	FROMDEC="consol.exe power.cmd consle.cpu mmie.cpu power.cpu \
		code??.cmd fp.cpu fpsp.cpu bitfld.cpu cm.cpu basic.cpu \
		irdfpa.cpu queue.cpu idc.cpu"
	CASSETTE="/dev/tu0"
	BCASSETTE="/dev/tu1"
	while :
	do
		echo "
Remove the TU58 tape cassette from the inside drive.
Replace it with the TU58 cassette that was supplied with the processor.

Press the RETURN key when you are ready to continue. " 
		read ans
		case $ans in 
		"" )	break ;; 
		* )	;; 
		esac
	done
	cd /sys/consoles/vax/730cons || exit 1 
	/etc/sizer -b -k ${VMUNIX} ||
	{
		 echo $FAIL; exit 1 
	}
	rm -rf fromdec
	mkdir fromdec
	echo "
Extracting files from the console cassette.
This takes several minutes."
	(cd fromdec; arff xmf ${BCASSETTE} ; cp /usr/mdec/vmb.exe . ) 
	cp defboo.cmd askboo.cmd ${STDBOO} fromdec
	dd if=${BCASSETTE} of=cassette count=16 ||
	{
		 echo $FAIL; exit 1 
	}
	echo -n "
Remove the TU58 cassette from the front drive.  Insert a
blank TU58 cassette.  Make sure the cassette is write-enabled.

Press the RETURN key when you are ready to continue. " 
	read ans
	/etc/arff imf cassette 512,1,ULTRIX-32,Console,
	echo "
Building console cassette for ULTRIX.
This takes several minutes."
	(cd fromdec; /etc/arff rmf ../cassette defboo.cmd vmb.exe askboo.cmd \
		${FROMDEC} ${STDBOO} )
	dd if=cassette of=${CASSETTE} bs=20k conv=sync ||
	{
		 echo $FAIL; exit 1 
	}
	echo "
Directory listing of the new console cassette follows."
	/etc/arff tmvf ${CASSETTE}
	rm -rf fromdec
	echo "
Remove the TU58 cassette from the side drive.  Move the TU58
cassette from the front drive to the side drive."
	;;

VAX6200 | VAX3400)
	/etc/sizer -b -k ${VMUNIX} ||
	{
		echo $FAIL; exit 1
	}
	;;	

* )
	echo "No console update for this processor type."
	;;
esac
cd $PWD
exit 0
