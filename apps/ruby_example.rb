#!/usr/bin/env ruby

require 'obexftp'

intfs = Obexftp.discover(Obexftp::USB)
intfs.each { |i| puts i }

# - or -

cli = Obexftp::Client.new(Obexftp::BLUETOOTH)
intfs = cli.discover
intfs.each { |i| puts i }

# - then -

dev = intfs.first

puts 'Sync channel'
puts Obexftp.browsebt(dev, Obexftp::SYNC)
puts 'Push channel'
puts Obexftp.browsebt(dev, Obexftp::PUSH)
puts 'FTP channel'
puts Obexftp.browsebt(dev, Obexftp::FTP)

channel = Obexftp.browsebt(dev, 0) # default is ftp
cli = Obexftp::Client.new(Obexftp::BLUETOOTH) # or reuse the above
puts cli.connect(dev, channel)
puts cli.list('/')
puts cli.disconnect

