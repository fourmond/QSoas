# A helper to generate the src/build.hh build info

require 'socket'

f = File.open("src/build.hh", "w")

host = Socket.gethostname
if host =~ /ccal/
  host = "mac-build-daemon"
end
build_info = "Built #{Time.now} on '#{host}'"

f.puts "#define SOAS_BUILD_INFO \"#{build_info}\""

if File.readable?(".git") 
  version = `git describe --tags --abbrev=0`
  version.chomp!
  if version.size > 0
    `git branch` =~ /\*\s+(\S+)/
    branch = $1
    commit = `git rev-parse --short HEAD`
    commit.chomp!
    r = 
      version = "#{version}+#{branch}+#{commit}"
    if not system("git diff-index --quiet HEAD --")
      version += "~wip"
    end

    f.puts "#undef SOAS_VERSION"
    f.puts "#define SOAS_VERSION \"#{version}\""
  end

end

