# Copyright 2012, 2013 by Vincent Fourmond
# Usable, modifiable and redistributable under the same terms as QSoas itself
#
# This small script is run by qmake on windows to make a
# windres-"proper" version string.

# QSoas.wxs
# C:\Prog\QSoas\QSoas.wxs(3) : error CNDL0242 : Invalid product version '0.0.20130618.8889'. Product version must have a major version less than 256, a minor version less than 256, and a build version less than 65536.
#
# That will make it very hard to make a windows date-based version.
#
# We assume now that the version number has the following structure:
#
# (version)+(date)-name-+r(rev)+...

version = ARGV[0]
# Remove the -win64 suffix, useless
version.sub!(/-win64$/,'')

if version =~ /(.*)\+.*\+r(\d+)/
  pre = $1
  rev = $2.to_i
  v = pre.split(/[.]/)
  v[2] = rev / 10000
  v[3] = rev % 10000
  version = v.join(".")
end

# Then, normal cleanup
vc = version.split(/[.]/)
vc = vc[0..3]
for v in vc
  v.gsub!(/\D/,'')
end

puts vc.join(".")

