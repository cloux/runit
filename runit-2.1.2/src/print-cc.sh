cc="`head -n1 conf-cc`"
systype="`cat systype`"

cat warn-auto.sh
echo exec "$cc" "$CPPFLAGS" "$CFLAGS" '-c ${1+"$@"}'
