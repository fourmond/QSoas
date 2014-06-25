# Reads from standard input and prints to standard output everything
# between the /// INTERNAL -> /// END INTERNAL markers stripped.

keep = true
for l in STDIN
  if l =~ /^\s*(?:##|\/\/\/)\s+(END\s+)?INTERNAL\s*$/
    if $1
      keep = true
    else
      keep = false
    end
  else
    puts l if keep
  end
end
