# fancyhash.rb: sugar-coated hashes
# Copyright 2019 by CNRS/AMU
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

class FancyHash < Hash
  def method_missing(symb, *rest)
    k = symb.to_s
    if self.key? k
      return self[k]
    else
      raise NoMethodError.new(symb)
    end
  end

end

# a = FancyHash.new
# p a
# a["stuff"] = 23
# p a ["stuff"]
# p a.stuff
# a.sdf
