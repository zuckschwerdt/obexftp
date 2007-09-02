#!/usr/bin/env ruby

require 'obexftp'

intfs = Obexftp.discover(Obexftp::USB)
intfs.each { |i| puts i }

# - or -

obex = Obexftp::Client.new(Obexftp::BLUETOOTH)
intfs = obex.discover
intfs.each { |i| puts i }

# - then -

dev = intfs.first
channel = Obexftp.browsebt(dev, 0) # default is ftp

obex = Obexftp::Client.new(Obexftp::BLUETOOTH) # or reuse the above
puts obex.connect(dev, channel)
puts obex.list('/')
puts obex.disconnect

