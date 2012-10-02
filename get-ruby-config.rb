# Copyright 2012 by Vincent Fourmond
# Usable, modifiable and redistributable under the same terms as QSoas itself
#
# This small script is run by qmake to setup Ruby-related compilation
# variables.
require 'rbconfig'

case ARGV[0]
when "libarg"
  puts RbConfig::CONFIG['LIBRUBYARG']
when "libdir"
  puts RbConfig::CONFIG['libdir']
when "includedir"
  if RbConfig::CONFIG.key? 'rubyhdrdir'
    puts "#{RbConfig::CONFIG['rubyhdrdir']} "+
      "#{RbConfig::CONFIG['rubyhdrdir']}/#{RbConfig::CONFIG['arch']}"
  else
    puts RbConfig::CONFIG['archdir']
  end
end
