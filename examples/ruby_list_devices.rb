#!/usr/bin/env ruby

require 'obexftp'

puts "Scanning USB..."
intfs = Obexftp.discover(Obexftp::USB)
intfs.each do |dev|
  puts "usb/#{dev}"
end

puts "Scanning BT..."
intfs = Obexftp.discover(Obexftp::BLUETOOTH)
intfs.each do |dev| 
  channel = Obexftp.browsebt(dev, Obexftp::PUSH)
  puts "bt/[#{dev}]:#{channel} (push)"
  channel = Obexftp.browsebt(dev, Obexftp::FTP)
  puts "bt/[#{dev}]:#{channel} (ftp)"
  channel = Obexftp.browsebt(dev, Obexftp::SYNC)
  puts "bt/[#{dev}]:#{channel} (sync)"
end
