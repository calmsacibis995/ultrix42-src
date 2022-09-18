: run this script through /bin/sh
: '@(#)$Id: version.sh,v 1.2 90/11/25 19:03:26 sharpe Exp $'
case $# in
  0) set "6.6";;
esac

OFS="$IFS" IFS=:

for A in rprompt hostname uname who
do
    for D in $PATH
    do
	if [ ! -f $D/$A ]; then
	    continue
	fi
	case $A in
	    rprompt)    LOCAL=`$A %h`
			;;
	    hostname)   LOCAL=`$A`
			;;
	    uname)	LOCAL=`$A -n`
			;;
	    who)	LOCAL=`$A am i | sed -e 's%^\(.*\)!.*$%\1%'`
			;;
	esac
	break
    done
    if [ "x$LOCAL" != x ]; then
	break
    fi
done

IFS=

: 'derivative of ../config/version.sh'
if [ ! -r version ]; then echo "0" > version ; fi

cat version ../config/version |\
awk -F. '\
BEGIN	{ split ("'"${1}"'", ver, ".");\
	  major = ver[1]; minor = ver[2]; patch = ver[3]}\
NR == 1 { compile = $(NF) }\
NR == 2 { if (major != $1) {major = $1; minor = 0};\
	  if (minor != $2) {minor = $2; compile = 0}}\
END	 { compile++;\
	   printf "version: libmh %d.%d.%d\n", major, minor, compile; \
	   printf "%d.%d.%d\n", major, minor, compile > "_version";}'
mv _version version
