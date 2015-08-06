# A ruby file to generate series of data files with different
# separators and the like.

Fmt = Struct.new(:prefix, :separator, :suffix)

class Fmt

  def write_data(file, rows)
    File::open(file, "w") do |f|
      for r in rows
        data = r.map {|x| x.to_s}.join(self.separator)
        f.puts "#{self.prefix}#{data}#{self.suffix}"
      end
    end
  end
  
end

rows = []
nb = 200
nb.times do |i|
  x = i /(nb - 1.0) * 2.0*Math::PI
  rows << [x, Math::sin(x), i, Math::cos(x)]
end

seps = {
  'tab' => "\t", 'space' => " ",
  'tabs' => "\t\t", 'spaces' => "   ",
  'comma' => ',', 'comma-space' => ' , ',
  'colon' => ';', 'colon-space' => ' ; '
}

fmts = {}
for n,s in seps
  fmts["gen-#{n}.dat"] = Fmt.new("", s, "")
  fmts["gen-#{n}-p1.dat"] = Fmt.new("  ", s, "")
  fmts["gen-#{n}-p2.dat"] = Fmt.new("\t", s, "")
  fmts["gen-#{n}-s1.dat"] = Fmt.new("", s, "  ")
  fmts["gen-#{n}-s2.dat"] = Fmt.new("", s, "\t")
  fmts["gen-#{n}-sp1.dat"] = Fmt.new("  ", s, "  ")
  fmts["gen-#{n}-sp2.dat"] = Fmt.new("\t", s, "\t")
end

for n, f in fmts
  f.write_data(n, rows)
end
