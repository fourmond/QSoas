# Copyright 2013 by Vincent Fourmond
# Usable, modifiable and redistributable under the same terms as QSoas itself
#
# This small script is run by qmake on windows to prepare the XML
# input file for WiX.
require 'rbconfig'
# For the generation of the UUID
require 'securerandom'

# Persistence of UUID across runs of this script.
require 'yaml'


# Takes two arguments: full version, and win-nice version
full_version = ARGV[0] || "full"
win_version = ARGV[1] || "0.0"

uuids = {}

# uuids.yaml contains the persistent UUIDs
if File.exist?("uuids.yaml")
  File.open("uuids.yaml", "r") { |f| uuids = YAML::load(f) }
end

files = []

# Qt libraries
for f in %w[QtCore4 QtGui4 QtOpenGL4 QtTest4]
  lib_file = "#{ENV['QTDIR']}\\bin\\#{f}.dll"
  files << lib_file
end

# Ruby library
rb = File::join(RbConfig::CONFIG['bindir'], RbConfig::CONFIG['LIBRUBY_SO'])
rb.gsub!(/\//, '\\')
files << rb

# And now, we look for g++ libraries:
path = ENV['PATH'].split(/\s*;\s*/)
for f in %w[libgcc_s_dw2-1.dll libstdc++-6.dll mingwm10.dll]
  for d in path
    t = File::join(d,f)
    if File.exists?(t)
      t.gsub!(/\//, '\\')
      files << t
      break
    end
  end
end


# We first get the MD5sum for all the files
md5sums = {}
for f in files
  md5 = `md5sum #{f}`
  md5.gsub!(/ .*/, '')
  md5.gsub!(/\\/, '')
  md5.chomp!
  print "#{f} -> #{md5}"
  md5sums[f] = md5
end

# We add a few keys
keys = md5sums.values

keys += ["UpgradeID", "ProductID", "QSoasID"]

# Now, we generate all the missing UUIDs
for k in keys
  if ! uuids.key?(k)
    uuids[k] = SecureRandom.uuid
  end
end

store = uuids.dup

# These are UUIDS that should change at every run
store.delete("QSoasID")

File.open("uuids.yaml", "w") do |f|
  YAML::dump(store, f)
end

p uuids

# Generation of the two components

dlls = ""
dll_refs = ""
idx = 0
for f in files
  next unless File::exists?(f)   # Only for testing !
  guid = uuids[md5sums[f]]
  bf = File::basename(f)
  dlls << "<Component Id='Dll#{idx}' Guid='#{guid}'>\n"
  dlls << "  <File Id='Dll_file#{idx}' Name='#{bf}' DiskId='1' Source='#{f}' KeyPath='yes' />\n"
  dlls << "</Component>"
  dll_refs << "<ComponentRef Id='Dll#{idx}' />\n"
  idx += 1
end

substs = uuids.dup
substs['DLLS'] = dlls
substs['DLL_REFS'] = dll_refs
substs['VERSION'] = full_version
substs['WIN_VERSION'] = win_version

# OK, now the generation of the XML source file, in the current directory ?

File.open("pkg/QSoas.wxs", "r") do |f|
  File.open("QSoas.wxs", "w") do |o|
    for line in f
      line.gsub!(/#([^#]+)#/) do |s|
        substs[$1]
      end
      o.print line 
    end
  end
end
