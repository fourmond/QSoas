# A helper to generate the src/build.hh build info

require 'socket'

f = File.open("src/build.hh", "w")

build_info = "Built #{Time.now} on '#{Socket.gethostname}'\\n"

f.puts "#define SOAS_BUILD_INFO \"#{build_info}\""

