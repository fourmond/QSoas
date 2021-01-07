# complex.rb: Pure ruby side of the Cplx class
# Copyright 2021 by CNRS/AMU
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301
# USA

# This code is inpired from the code in mruby complex mrbgem, which
# was apparently written by Ukrainskiy Sergey
# <ukrainskiysergey@gmail.com>

class Cplx

  def inspect
    return "(#{self.to_s})"
  end

end

# Change the "usual" operators
[Fixnum, Float].each do |cls|
  [:+, :-, :*, :/, :==].each do |op|
    cls.instance_eval do
      original_operator_name = "__original_operator_#{op}_complex"
      alias_method original_operator_name, op
      define_method op do |rhs|
        if rhs.is_a? Cplx
          Cplx(self).__send__(op, rhs)
        else
          __send__(original_operator_name, rhs)
        end
      end
    end
  end
end
