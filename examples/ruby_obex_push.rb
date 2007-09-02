#!/usr/bin/env ruby
=begin
  ruby_obex_push.rb - Ruby/GTK2 example of ObexFTP push client.

  Copyright (c) 2007 Christian W. Zuckschwerdt

  Original Ruby/GTK2 examples Copyright Ruby-GNOME2 Project Team
  This program is licenced under the same licence as Ruby-GNOME2.
=end

require 'gtk2'
require 'obexftp'

dialog =  Gtk::FileChooserDialog.new("Send file via bluetooth", nil, 
                                     Gtk::FileChooser::ACTION_OPEN,
                                     "gnome-vfs",
                                     [Gtk::Stock::COPY, Gtk::Dialog::RESPONSE_ACCEPT],
                                     [Gtk::Stock::CLOSE, Gtk::Dialog::RESPONSE_CANCEL]
                                     )
label = Gtk::Label.new("Select target device:")
combo = Gtk::ComboBox.new
hbox = Gtk::HBox.new
hbox.add(label).add(combo).show_all
dialog.extra_widget = hbox

dialog.signal_connect("response") do |widget, response|
  case response
  when Gtk::Dialog::RESPONSE_ACCEPT
    filename = dialog.filename
    dev = combo.active_text
    channel = Obexftp.browsebt(dev, Obexftp::PUSH)

    obex = Obexftp::Client.new(Obexftp::BLUETOOTH)
    puts obex.connectpush(dev, channel)
    puts obex.put_file(filename)
    puts obex.disconnect

  else
    dialog.destroy
    Gtk.main_quit
  end
end


# Should use gtk.timeout_add and/or Thread.new for the scanning...
puts "Scanning USB..."
intfs = Obexftp.discover(Obexftp::USB)
# intfs.each { |i| combo.append_text(i) } # enable this with >=ObexFTP-0.23
puts "Scanning BT..."
intfs = Obexftp.discover(Obexftp::BLUETOOTH)
intfs.each { |i| combo.append_text(i) }
combo.active = 0

dialog.show_all
Gtk.main
