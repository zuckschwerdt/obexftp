#!/usr/bin/python

import obexftp

cli = obexftp.client(obexftp.BLUETOOTH)

devs = cli.discover();
print devs;
dev = devs[0]
print "Using %s" % dev

print cli.connect(dev, 6)

print cli.list("/")

print cli.list("/images")

data = cli.get("/images/some.jpg")
file = open('downloaded.jpg', 'wb')
file.write(data)

print cli.disconnect

cli.delete

