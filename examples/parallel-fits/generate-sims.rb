# This Ruby code defines a function that can generate a number of data
# files

def generate_sims(fit_name, params, number, overrides, fit_opts)
  ov = {}
  for o in overrides
    # o is param:a..b
    o =~ /([^:]+):(.*)\.\.(.*)/
    ov[$1] = [$2.to_f, $3.to_f]
  end

  cmd = "sim-#{fit_name}".gsub('-','_').to_sym
  number.times do |i|
    cv = []
    for k,v in ov
      val = v.first + (v.last-v.first)*i/(number-1.0)
      cv << "#{k}=#{val}"
    end
    spec = cv.join(";")
    o = {'override' => spec}
    o.merge!(fit_opts)
    QSoas.send(cmd, params, "flagged:template", o)
    # QSoas.set_meta("idx", i.to_s)
    QSoas.rename("#{fit_name}-#{spec}")
    QSoas.flag(:flags => "#{fit_name}-cln")
    # QSoas.run("add-noise.cmds", fit_name)
  end
end

# This is how you launch it !
# generate_sims("exponential-decay", "exp.params", 51,
#               ["tau_1:1..10", "A_1:20..30"], {:exponentials => 2})
