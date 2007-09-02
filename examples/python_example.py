#!/usr/bin/python

import obexftp

obex = obexftp.client(obexftp.BLUETOOTH)

devs = obex.discover();
print devs;
dev = devs[0]
print "Using %s" % dev
channel = obexftp.browsebt(dev,0)
print "Channel %d" % channel

print obex.connect(dev, channel)

print obex.list("/")

print obex.list("/images")

data = obex.get("/images/some.jpg")
file = open('downloaded.jpg', 'wb')
file.write(data)

print obex.disconnect()

obex.delete

