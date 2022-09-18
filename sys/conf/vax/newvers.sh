if [ ! -r version ]; then echo 0 > version; fi
touch version
DATE=`date`
awk '	{	version = $1 + 1; }\
END	{	printf "char version[] = \"ULTRIX V4.2 (Rev. 96) System #%d: ", version > "vers.c";\
		printf "DATE-HERE\\n\";\n" >> "vers.c";\
		printf "%d\n", version > "version"; }' < version
ed - vers.c <<%
1,\$s/DATE-HERE/$DATE/
w
q
%
