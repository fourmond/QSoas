# qsoas-base.rb: Ruby base functions for QSoas 
# Copyright 2011, 2012 by Vincent Fourmond

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see
# <http://www.gnu.org/licenses/>.
    
## @file This file contains various utility functions

def soas_eval(__str)
  begin
    eval(__str)
  rescue SyntaxError => __e
    raise "Syntax error: #{__e.to_s}"
  end
end

def soas_make_block(__vars, __code)
  __vars2 = soas_find_vars(__code)
  __vars.concat(__vars2)
  __vars.uniq!
  begin
    __blk = eval "proc do |#{__vars.join(',')}|\n#{__code}\nend"
  rescue SyntaxError => __e
    raise "Syntax error: #{__e.to_s}"
  end
  return __blk
end


## Find all the undefined variables in the given code, while trying
## hard to avoid "out of range" errors...
def soas_find_vars(__code)
  __sandbox = Object.new

  # First, fill in with dummy methods
  for m in Math.methods - Object.methods do
    ## @todo handle proper arity ?
    __sandbox.send(:eval, "def self.#{m}(*a)\nreturn 1.0\nend")
  end

  __vars = []
  
  __done = false

  begin
    __blk = __sandbox.send(:eval,
                           "proc do |#{__vars.join(',')}|\n#{__code}\nend")
    __tmp = [1.0] * __vars.size
    __blk.call(*__tmp)
    __done = true
  rescue NameError => __e
    if __e.is_a? NoMethodError
      raise __e
    end
    __vars << __e.name.to_s
    __done = false
  rescue SyntaxError => __e
    raise "Syntax error: #{__e.to_s}"
  end while ! __done
  return __vars
end
