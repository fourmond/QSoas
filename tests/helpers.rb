class Float

  def smaller_than?(what)
    return self.abs < what
  end
  
  def negligible?(scale = 1.0)
    flt = begin
            Float::EPSILON
          rescue
            3e-16
          end
    return self.smaller_than?(flt*scale)
  end
end
