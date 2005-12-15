#!/usr/bin/tclsh

load obexftp.so
client c $BLUETOOTH
puts [c connect "00:11:22:33:44:55" 6]
puts [c list "/"]
c disconnect
c -delete
