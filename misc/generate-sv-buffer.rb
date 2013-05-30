# This file generates buffers suitable for testing SV decomposition

# We generate series of data that look like:
#
# sin(alpha * x) * cos(beta * t) + noise
#
# or
#
# sin(alpha * x) * 1/(1 + e) + noise

include Math

x_values = 1000
components = 6
t_values = 70

x_values.times do |i|
  x = i*10.0/(x_values - 1)
  data = []
  data << x
  t_values.times do |j|
    y_val = 0
    t = j*1.0/(t_values - 1)
    components.times do |k|
      ex = exp(4*(t - k/(components - 1.0)))
      y_val += sin(k*x) * (t+1)**(k/3.0)*cos(6*k*t)/(1 + ex)
    end
    data << y_val
  end
  puts data.join("\t")
end
