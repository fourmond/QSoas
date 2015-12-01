class Float

  def smaller_than?(what)
    return self.abs < what
  end
  
  def negligible?(scale = 1.0)
    return self.smaller_than?(Float::EPSILON*scale)
  end
end
