#!/usr/bin/python

import obexftp

cli = obexftp.client(obexftp.BLUETOOTH)

print cli.connect("00:11:22:33:44:55", 6)

print cli.list("/")

print cli.disconnect

cli.delete

