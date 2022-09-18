#
# @(#)fis_.login	4.1	(ULTRIX)	2/28/91
#
tset -I -Q
set mail=/usr/spool/mail/root
set prompt="# "
#
# Start FIS specific login code
#
/usr/etc/sizer -wt >& /usr/tmp/noise
if ($status == 0) then
set WS=`cat /usr/tmp/noise`
if ($WS != 0) then
echo " "
echo "     Please move the mouse cursor into this window and press "
echo "      the left button on the mouse to activate this window."
endif
endif
echo " "
echo "           To start SCAMP, type scamp and press RETURN."
echo " "
rm -f /usr/tmp/noise
#
# End FIS specific login code
#
