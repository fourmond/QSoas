# python-wrapper.py: Python code for wrapping external python functions
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

## CODE

import sys
import json

for l in sys.stdin:
    cmd = json.loads(l)
    rv = dict()
    try:
        fnc = lambda x: func(x, *cmd['parameters'])
        rv['values'] = list(map(fnc, cmd['xvalues']))
    except Exception as ex:
        rv['error'] = str(ex)
    print(json.dumps(rv))
    print("// -> DONE", flush=True)
    
             
