#!/usr/bin/env ruby

# == Synopsis
#
# Transfer files from/to Mobile Equipment.
#
# == Usage
#
#    ruby_obexftp.rb <transport> <operations...>
#      transport: [ -i | -b <dev> [-B <chan>] | -U <intf> | -t <dev> | -N <host> ]
#      operations: [-c <dir> ...] [-C <dir> ] [-l [<dir>]]
#    [-g <file> ...] [-p <files> ...] [-k <files> ...]
#
#    -i, --irda                  connect using IrDA transport (default)
#    -b, --bluetooth [<device>]  use or search a bluetooth device
#    [ -B, --channel <number> ]  use this bluetooth channel when connecting
#    -u, --usb [<intf>]          connect to a usb interface or list interfaces
#    -t, --tty <device>          connect to this tty using a custom transport
#    -n, --network <host>        connect to this host
#    
#    -F, --ftp                   use FTP (default)
#    -P, --push                  use OPUSH
#    -S, --sync                  use IRMC
#    -c, --chdir <DIR>           chdir
#    -C, --mkdir <DIR>           mkdir and chdir
#    -l, --list [<FOLDER>]       list current/given folder
#    -o, --output <PATH>         specify the target file name
#                                get and put always specify the remote name.
#    -g, --get <SOURCE>          fetch files
#    -G, --getdelete <SOURCE>    fetch and delete (move) files
#    -p, --put <SOURCE>          send files
#    -k, --delete <SOURCE>       delete files
#
#    -X, --capability            retrieve capability object
#    -v, --verbose               verbose messages
#    -V, --version               print version info
#    -h, --help, --usage         this help text
#
# fmtstring::
#    A +strftime+ format string controlling the
#    display of the date and time. If omitted,
#    use <em>"%Y% M% d %H:%m"</em>
#
# == Author
# Christian W. Zuckschwerdt <zany@triq.net>
#
# == Copyright
# Copyright (c) 2006 Christian W. Zuckschwerdt <zany@triq.net>
# Licensed under the same terms as Ruby.

require 'optparse'
require 'rdoc/usage'
require 'obexftp'


@cli = nil
@transport = Obexftp::BLUETOOTH
@service = Obexftp::FTP
@device = nil
@channel = -1


# connect with given uuid. re-connect every time
def cli_connect
  if @cli.nil?
    # Open
    @cli = Obexftp::Client.new(@transport)
    raise "Error opening obexftp-client" if @cli.nil?
  end

  @cli.callback Proc.new { |num, msg|
    puts "Callback no.#{num} '#{msg}'" if msg.length < 30
  }
  
#  for (retry = 0; retry < 3; retry++)
    # Connect
    @cli.connect(@device, @channel) and return @cli
#    fprintf(stderr, "Still trying to connect\n")
#  end

  @cli.close
  @cli = nil
end

def cli_disconnect
  return if @cli.nil?
  @cli.disconnect
  # @cli.close
  @cli = nil
end


# preset mode of operation depending on our name

# preset from environment

OptionParser.new do |opts|
  verbose = false
  target_path = nil
  output_file = nil
  
  opts.on('-i', '--irda') do
    @transport = Obexftp::IRDA
    @device = nil
    @channel = 0
  end
  
  opts.on('-b', '--bluetooth [ADDR]') do |arg|
    @transport = Obexftp::BLUETOOTH
    @device = arg

    if arg.nil?
      intfs = Obexftp.discover(@transport)
      intfs.each { |i| puts i }
      @device = intfs.first
    end

    @channel = Obexftp.browsebt(@device, @service)

    puts "Got channel #{@channel}"
  end
    
  opts.on('-B', '--channel N', Integer) do |arg|
    @channel = arg
  end

  opts.on('-u', '--usb [N]', Integer) do |arg|
    # "If USB doesn't work setup permissions in udev or run as superuser."
    @transport = Obexftp::USB
    @device = nil
    @channel = arg
    intfs = Obexftp.discover(@transport)
  end
    
  opts.on('-t', '--tty DEVICE') do |arg|
    @transport, @device, @channel = Obexftp::CABLE, arg, -1

    # if (strstr(arg, "ir") != NULL)
    #	fprintf(stderr, "Do you really want to use IrDA via ttys?\n");
  end

  opts.on('-n', '--network HOST') do |arg|
    @transport = Obexftp::NETWORK
    @device = arg # dotted quad, hopefully
    @channel = 0
  end

      
  opts.on('-F', '--ftp') do
    cli_disconnect
    @service = Obexftp::FTP
  end
  opts.on('-P', '--push') do
    cli_disconnect
    @service = Obexftp::PUSH
  end
  opts.on('-S', '--sync') do
    cli_disconnect
    @service = Obexftp::SYNC
  end

    
  opts.on('-l', '--list [PATH]') do |arg|
    puts cli_connect.list(arg)
  end

  opts.on('-c', '--chdir [PATH]') do |arg|
    cli_connect.chpath(arg)
  end

  opts.on('-C', '--mkdir PATH') do |arg|
    cli_connect.mkpath(arg)
  end


  opts.on('-o', '--output PATH') do |arg|
    output_file = arg
  end

  opts.on('-g', '--get PATH') do |arg|
    output_file ||= File.basename(arg)
    
    result = cli_connect.get(arg)
    raise "No such file" if result.nil?
    open(output_file, 'w') do |file|
      file.print(result)
    end
    output_file = nil
  end

  opts.on('-G', '--getdelete PATH') do |arg|
    output_file ||= File.basename(arg)

    cli = cli_connect
    result = cli.get(arg)
    raise "No such file" if result.nil?
    open(output_file, 'w') do |file|
      file.print(result)
      cli.del(arg)
    end
    output_file = nil
  end

  opts.on('-p', '--put PATH') do |arg|
    output_file ||= File.basename(arg)

    cli_connect.put_file(arg, output_file)
    output_file = nil
  end

  opts.on('-k', '--delete PATH') do |arg|
    cli_connect.delete(arg)
  end

  opts.on('-X', '--capabilities [PATH]') do |arg|
    cli_connect.get_capability(arg)
  end

  opts.on('-v', '--[no-]verbose', 'Run verbosely') do |arg|
    verbose = arg
  end
      
  opts.on('-V', '--version') do
    puts "ObexFTP #{VERSION}"
  end

  opts.on('-h', '--help') do
    RDoc::usage
  end

  opts.parse!(ARGV) # rescue RDoc::usage('usage')
end

cli_disconnect

