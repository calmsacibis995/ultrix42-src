#! /bin/sh
#
#	@(#)ypxfr_1perhour.sh	4.1	ULTRIX	7/2/90
#
# @(#)ypxfr_1perhour.sh 1.1 86/02/05 Copyr 1985 Sun Microsystems, Inc.  
# @(#)ypxfr_1perhour.sh	2.1 86/04/16 NFSSRC
#
# ypxfr_1perhour.sh - Do hourly yp map check/updates
#

# set -xv
/etc/yp/ypxfr passwd.byname
/etc/yp/ypxfr passwd.byuid 
