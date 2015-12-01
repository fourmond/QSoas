class Float
  def negligible?(scale = 1.0)
    return self.abs < Float::EPSILON*scale
  end
end
