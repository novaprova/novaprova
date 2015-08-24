#!/usr/bin/python
#
#  Copyright 2011-2015 Gregory Banks
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#

import sys
import os
import re

if __name__ == "__main__":
    # Cannot rely on argparse being present
    # so we parse args ourselves.
    defines = set()
    for a in sys.argv[1:]:
        if a.startswith('-D'):
            defines.add(a[2:])
        else:
            raise RuntimeError("Unknown option \"%s\"" % a)

    stack = []
    for line in sys.stdin:
        line = line.rstrip("\r\n")

        m = re.match(r'^@ifdef\s+(\S+)\s*$', line)
        if m is not None:
            stack.append(m.group(1) in defines)
            continue

        m = re.match(r'^@else\s*$', line)
        if m is not None:
            if len(stack) == 0:
                raise RuntimeError("@else without @ifdef")
            stack[-1] = not stack[-1]
            continue

        m = re.match(r'^@endif\s*$', line)
        if m is not None:
            if len(stack) == 0:
                raise RuntimeError("@endif without @ifdef")
            stack.pop(-1)
            continue

        if len(stack) == 0 or stack[-1]:
            sys.stdout.write(line + "\n")
