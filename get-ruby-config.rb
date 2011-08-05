# A small wrapper around ruby to bypass qmake's bad word breaking
require 'rbconfig'

case ARGV[0]
when "libarg"
  puts RbConfig::CONFIG['LIBRUBYARG']
when "includedir"
  if RbConfig::CONFIG.key? 'rubyhdrdir'
    puts RbConfig::CONFIG['rubyhdrdir']
  else
    puts RbConfig::CONFIG['archdir']
  end
end
