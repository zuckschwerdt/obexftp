#!/usr/bin/env ruby

require 'mkmf'

# hack 1: ruby black magic to write a Makefile.new instead of a Makefile
alias open_orig open
def open(path, mode=nil, perm=nil)
  path = 'Makefile.new' if path == 'Makefile'
  if block_given?
    open_orig(path, mode, perm) { |io| yield(io) }
  else
    open_orig(path, mode, perm)
  end
end

if ENV['PREFIX']
  prefix = CONFIG['prefix']
  %w[ prefix sitedir datadir infodir mandir oldincludedir ].each do |key|
    CONFIG[key] = CONFIG[key].sub(/#{prefix}/, ENV['PREFIX'])
  end
end

dir_config('obexftp')
if have_library('openobex', 'OBEX_Init') and
   find_library('bfb', 'bfb_io_open', '../../bfb/.libs') and
   find_library('multicobex', 'cobex_ctrans', '../../multicobex/.libs') and
   find_library('obexftp', 'obexftp_open', '../../obexftp/.libs')
  create_makefile('obexftp')

  # hack 2: strip all rpath references
  open('Makefile.ruby', 'w') do |out|
    IO.foreach('Makefile.new') do |line|
      out.puts line.gsub(/-Wl,-R'[^']*'/, '')
    end
  end
else
  puts 'obex libs not found'
end

