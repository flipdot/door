#!/bin/bash
TARGET=/home/opendoor/.ssh/authorized_keys
URL="http://ldapapp.fd/system/ssh_keys"
set -e

rm -f $TARGET.ldap.new
if curl --fail -s "$URL" > $TARGET.ldap.new; then
	mv $TARGET.ldap.new $TARGET.ldap
else
	:
fi

cat $TARGET.always > $TARGET
cat $TARGET.ldap >> $TARGET

chmod 644 $TARGET
chown opendoor: $TARGET

