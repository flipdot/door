echo "[`ldapsearch -h rail.fd -D "cn=admin,dc=flipdot,dc=org" -W -b "ou=members,dc=flipdot,dc=org" employeeNumber | grep employee | grep "^employ" | awk '{ print "\x22" $2 "\x22"  }' | sed ':a;N;$!ba;s/\n/,/g'`]" > rfid_tags.json

