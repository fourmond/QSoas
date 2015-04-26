# In this file, we define the functions to create fake data for a
# "redox titration"

# Lorentzian for one species ?
def spec_one(x, x0, delta, ampl)
  return ampl/(1 + (x - x0)**2/delta**2)
end

# An array of [pot, pos, ar, ao, delta] for all the species
Species = [
  [0, 400, 1, 0.4, 100],
  [0.2, 450, 1.3, 2, 20],
  [-0.2, 600, 1, 0.4, 100]
]

def spec_all(x, e)
  rv = 0
  fara = 96480/(8.314 * 298)
  for pot, pos, ao, ar, delta in Species
    ex = exp(fara * (pot - e))
    ampl = (ao*ex + ar)/(ex + 1)
    rv += spec_one(x, pos, delta, ampl)
  end
  return rv
end
