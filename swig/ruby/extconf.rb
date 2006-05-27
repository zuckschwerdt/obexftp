require 'mkmf'

dir_config('obexftp')
if have_library('openobex', 'OBEX_Init') and
   have_library('bluetooth', 'str2ba') and
   find_library('bfb', 'bfb_io_open', '../../bfb/.libs') and
   find_library('multicobex', 'cobex_ctrans', '../../multicobex/.libs') and
   find_library('obexftp', 'obexftp_open', '../../obexftp/.libs')
  # hack 1: save old Makefile and move Makefile to Makefile.ruby
  File.rename('Makefile', 'Makefile.bak')
  create_makefile('obexftp')
  File.rename('Makefile', 'Makefile.new')
  File.rename('Makefile.bak', 'Makefile')
  # hack 2: strip all rpath references
  open('Makefile.ruby', 'w') do |out|
    IO.foreach('Makefile.new') do |line|
      out.puts line.gsub(/-Wl,-R'[^']*'/, '')
    end
  end
else
  puts 'obex libs not found'
end

