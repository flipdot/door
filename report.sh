
#!/bin/bash

KEYS=(
    -r B04800CE
    -r 08B0AE86
    -r 99CABD50
)
MAIL=(
    vorstand@flipdot.org
    c+door@cfs.im
)
export FROM=flipdot-noti@vega.uberspace.de
SUBJECT="Door Report"

tmp=$(mktemp)
function send_cleanup() {
    gpg -ea "${KEYS[@]}" --always-trust -o - < "$tmp" \
        | mailx -r "$FROM" -s "$SUBJECT" "${MAIL[@]}"
    rm -f "$tmp"*
}
trap send_cleanup EXIT TERM INT


journalctl --since "1 week 1 hour ago" | grep -e "DoorWeb -" -e "DoorSSH -" >> "$tmp"

