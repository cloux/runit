#!/bin/sh
set -eu
TTY=$1

cat <<EOF
#!/bin/sh
! type fgetty >/dev/null 2>&1 || exec chpst -P fgetty $TTY
exec getty 38400 $TTY linux
EOF
