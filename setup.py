#!/usr/bin/env python
#
# This is not a real setup.py.  It just exists to convince the
# readthedocs.org build process to run doxygen, so generate the
# doc/xml/ tree that is needed to feed C API information to
# Sphinx/breathe.
#
# RTD also has a custom build process, but that seems a little
# more complicated.
#

import sys
import os
import os.path
from subprocess import call, check_output
from shutil import rmtree

print "NovaProva running %s" % ' '.join(sys.argv)
if sys.argv[1] != "install":
    sys.exit(0)

# work out a reasonable string to use as the version
# for documentation purposes.
version = check_output(['git', 'tag', '--points-at', 'HEAD'])
if version.startswith('novaprova-'):
    version = version[10:]
if version == '':
    version = 'latest'

print "version \"%s\"" % version

# Expand @PACKAGE_VERSION@ in some files
for filename in ['Doxyfile']:
    print "expanding %s" % filename
    with open(filename + '.in', 'r') as fin:
        with open(filename, 'w') as fout:
            for line in fin:
                line = line.replace('@PACKAGE_VERSION@', version)
                fout.write(line)

# This is the first half of the docs: target in
# the top-level Makefile converted into Python
rmtree('doc/api-ref', ignore_errors=True)
rmtree('doc/man', ignore_errors=True)
rmtree('doc/xml', ignore_errors=True)

r = call(['doxygen'])
if r != 0:
    sys.exit(r)

# $(RM) doc/man/man*/_*

call(['./prune-doxygen-xml.pl', 'doc/xml'])

# this is the point at which the docs: target invoked
# Sphinx in doc/manual and began packaging.  RTD handles
# all that for us.
