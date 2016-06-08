#!/bin/sh
set -eu
TTY=$1

cat <<EOF
#!/bin/sh
exec utmpset -w $TTY
EOF
