# Copyright 2012 by Vincent Fourmond
# Usable, modifiable and redistributable under the same terms as QSoas itself
#
# This small script is run by qmake on windows to prepare some of the
# files included in the nsis installer.
#
require 'rbconfig'

out = open('pkg\dependencies.nsi', "wt")

for f in %w[QtCore4 QtGui4 QtOpenGL4]
  lib_file = "#{ENV['QTDIR']}\\bin\\#{f}.dll"
  out.puts "File \"#{lib_file}\""
end

rb = File::join(RbConfig::CONFIG['bindir'], RbConfig::CONFIG['LIBRUBY_SO'])
rb.gsub!(/\//, '\\')

out.puts "File \"#{rb}\""

# And now, we look for g++ libraries:

path = ENV['PATH'].split(/\s*;\s*/)
for f in %w[libgcc_s_dw2-1.dll libstdc++-6.dll]
  for d in path
    t = File::join(d,f)
    if File.exists?(t)
      t.gsub!(/\//, '\\')
      out.puts "File \"#{t}\""
      break
    end
  end
end

