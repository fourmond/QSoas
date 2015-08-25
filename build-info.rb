# A helper to generate the src/build.hh build info

require 'socket'

f = File.open("src/build.hh", "w")

host = Socket.gethostname
if host =~ /ccal/
  host = "mac-build-daemon"
end
build_info = "Built #{Time.now} on '#{host}'"

f.puts "#define SOAS_BUILD_INFO \"#{build_info}\""

