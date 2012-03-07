# qsoas-base.rb: implementation of (some of) the ruby communication code
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
  __done = true
  __blk = nil
  begin
    __blk = eval "proc do |#{__vars.join(',')}|\n#{__code}\nend"
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
  return __blk
end
