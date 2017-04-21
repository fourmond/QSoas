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
debug_files = []

# Qt libraries
if ENV["QTDIR"] =~ /Qt5/i       # Qt 5 !
  for f in %w[Qt5Core  Qt5Gui  Qt5OpenGL  Qt5PrintSupport  Qt5Widgets Qt5Svg]
    lib_file = "#{ENV['QTDIR']}\\bin\\#{f}.dll"
    files << lib_file
    debug_files << lib_file.gsub(/.dll/, "d.dll")
  end  
else
  for f in %w[QtCore4 QtGui4 QtOpenGL4 QtNetwork4 QtSvg4]
    lib_file = "#{ENV['QTDIR']}\\bin\\#{f}.dll"
    files << lib_file
    debug_files << lib_file.gsub(/4.dll/, "d4.dll")
  end
end

# Ruby library
rb = File::join(RbConfig::CONFIG['bindir'], RbConfig::CONFIG['LIBRUBY_SO'])
rb.gsub!(/\//, '\\')
files << rb
debug_files << rb

# And now, we look for g++ libraries:
path = ENV['PATH'].split(/\s*;\s*/)
for f in %w[libgcc_s_dw2-1.dll libstdc++-6.dll mingwm10.dll libwinpthread-1.dll]
  for d in path
    t = File::join(d,f)
    if File.exists?(t)
      t.gsub!(/\//, '\\')
      files << t
      debug_files << t
      break
    end
  end
end


# We first get the MD5sum for all the files
file_ids = {}
for f in files + debug_files
  fid = "#{f}+#{File.size f}"
  file_ids[f] = fid
end

# We add a few keys
keys = file_ids.values

keys += ["UpgradeID", "ProductID", "QSoasID", "StartMenuID"]

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

# p uuids

# Generation of the two components

what = {
  'release' => files,
  'debug' => debug_files
}

dlls = {} 
dll_refs = {}
for k,v in what
  idx = 0
  dlls[k] = ""
  dll_refs[k] = ""
  for f in v
    next unless File::exists?(f)   # Only for testing !
    guid = uuids[file_ids[f]]
    bf = File::basename(f)
    dlls[k] << "<Component Id='Dll#{idx}' Guid='#{guid}'>\n"
    dlls[k] << "  <File Id='Dll_file#{idx}' Name='#{bf}' DiskId='1' Source='#{f}' KeyPath='yes' />\n"
    dlls[k] << "</Component>"
    dll_refs[k] << "<ComponentRef Id='Dll#{idx}' />\n"
    idx += 1
  end
end

substs = uuids.dup
substs['DLLS'] = dlls['release']
substs['DEBUG_DLLS'] = dlls['debug']
substs['DLL_REFS'] = dll_refs['release']
substs['DEBUG_DLL_REFS'] = dll_refs['debug']

substs['VERSION'] = full_version
substs['WIN_VERSION'] = win_version

# OK, now the generation of the XML source file, in the current directory ?

for src in ["pkg/QSoas.wxs", "pkg/QSoas-nint.wxs", "pkg/QSoas-debug.wxs"]
  tgt = File::basename(src)
  File.open(src, "r") do |f|
    File.open(tgt, "w") do |o|
      for line in f
        line.gsub!(/#([^#]+)#/) do |s|
          substs[$1]
        end
        o.print line 
      end
    end
  end
end
