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
from subprocess import call
from shutil import rmtree

print "NovaProva running %s" % ' '.join(sys.argv)
if sys.argv[1] != "install":
    sys.exit(0)

topdir = os.path.normpath(os.path.join(os.path.dirname(__file__), '../..'))
os.chdir(topdir)
# exception on failure

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
