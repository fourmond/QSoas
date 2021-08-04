# A ruby file to generate series of data files with different
# separators and the like.

require 'fileutils'

$lst = []

def write_file(name, arg)
  File::open(name, 'w') do |f|
    11.times do |i|
      f.puts "#{i}\t#{i**arg}"
    end
  end
  $lst << name
end

FileUtils::mkdir_p("tz")

write_file("zip-1.dat", 1)
write_file("zip-3.dat", 3)
write_file("tz/zip-2.dat", 2)

begin
  File::unlink("test.zip")
rescue
end

system("zip", "test.zip", *$lst)

for f in $lst
  File::unlink(f)
end


