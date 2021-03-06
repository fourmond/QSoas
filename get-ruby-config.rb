# Copyright 2012 by Vincent Fourmond
# Usable, modifiable and redistributable under the same terms as QSoas itself
#
# This small script is run by qmake to setup Ruby-related compilation
# variables.
require 'rbconfig'

case ARGV[0]
when "compatible"
  rv = RbConfig::CONFIG["MAJOR"].to_i*100 + RbConfig::CONFIG["MINOR"].to_i
  if rv <= 203
    puts "OK"
  else
    puts ""
  end
when "version"
  puts RUBY_VERSION
when "libarg"
  puts RbConfig::CONFIG['LIBRUBYARG']
when "libdir"
  puts RbConfig::CONFIG['libdir']
when "includedir"
  if RbConfig::CONFIG.key? 'rubyhdrdir'
    puts "#{RbConfig::CONFIG['rubyhdrdir']} " + 
      if RbConfig::CONFIG['rubyarchhdrdir']
        RbConfig::CONFIG['rubyarchhdrdir']
      else
        "#{RbConfig::CONFIG['rubyhdrdir']}/#{RbConfig::CONFIG['arch']}"
      end
  else
    puts RbConfig::CONFIG['archdir']
  end
end
