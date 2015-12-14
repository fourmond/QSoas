# A ruby file to generate series of data files with different
# separators and the like.

Fmt = Struct.new(:prefix, :separator, :suffix, :encoding)

class Fmt

  def write_data(file, rows)
    mode = "w"
    if self.encoding
      found = false
      begin
        for e in Encoding.list
          if e.name == self.encoding && (not e.dummy?)
            mode << ":#{self.encoding}"
            found = true
          end
        end
      rescue
        return
      end
      if ! found
        return
      end
    end
    File::open(file, mode) do |f|
      # Write byte-order mark when applicable
      if self.encoding =~ /16/
        f.write "\uFEFF"
      end
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
  'comma' => ',', 'colon' => ';', 
}

# The ones that don't work as of now:
nw_seps = {
  'comma-space' => ' , ',
  'colon-space' => ' ; '
}

fmts = {}
for n,s in seps
  for c in [nil, "UTF-16LE", "UTF-16BE"]
    suff = ""
    if c
      suff = "-#{c}"
    end
    fmts["gen-#{n}#{suff}.dat"] = Fmt.new("", s, "", c)
    fmts["gen-#{n}-p1#{suff}.dat"] = Fmt.new("  ", s, "", c)
    fmts["gen-#{n}-p2#{suff}.dat"] = Fmt.new("\t", s, "", c)
    fmts["gen-#{n}-s1#{suff}.dat"] = Fmt.new("", s, "  ", c)
    fmts["gen-#{n}-s2#{suff}.dat"] = Fmt.new("", s, "\t", c)
    fmts["gen-#{n}-sp1#{suff}.dat"] = Fmt.new("  ", s, "  ", c)
    fmts["gen-#{n}-sp2#{suff}.dat"] = Fmt.new("\t", s, "\t", c)
  end
end

for n, f in fmts
  f.write_data(n, rows)
end
