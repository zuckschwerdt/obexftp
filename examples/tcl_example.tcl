#!/bin/sh
# the next line restarts using tclsh \
exec tclsh "$0" "$@"

load obexftp.so

puts [discover $BLUETOOTH]

client c $BLUETOOTH
puts [c connect "00:11:22:33:44:55" 6]
puts [c list "/"]
c disconnect
c -delete
