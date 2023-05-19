# returns the number of elements the sequence starting at z = x + I*y
# stays below 2. Maxes at nb
def mandelbrot(x, y, nb = 100)
  c = Cplx(x,y)
  z = c
  idx = 1
  while idx < nb
    z = z**2 + c
    if z.abs >= 2
      return idx
    end
    idx += 1
  end
  return nb
end
  
