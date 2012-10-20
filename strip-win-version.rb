# Copyright 2012 by Vincent Fourmond
# Usable, modifiable and redistributable under the same terms as QSoas itself
#
# This small script is run by qmake on windows to make a
# windres-"proper" version string.

version = ARGV[0]
version = version.gsub(/\+(\d+)\.(\d+)\.(\d+)/, '.\1\2\3.')
vc = version.split(/[.]/)
vc = vc[0..3]
for v in vc
  v.gsub!(/\D/,'')
end

puts vc.join(".")

