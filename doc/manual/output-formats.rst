
Output Formats
==============

Novaprova supports two different test result output formats.  You
can select between these using the ``--format`` option to the
test executable, or by calling ``np_set_output_format()`` if you
write your own ``main()`` routine.

``text``
    A simple text output format, designed to be read by humans.  Output
    goes to stderr.  Each completed test shows a single line, beginning
    with one of the key words ``PASS``, ``FAIL`` or ``N/A``, immediately
    when the test completes.  These lines are interspersed with whatever
    output to stdout or stderr the tests themselves may emit.  After all
    tests are complete a 1-line summary describes how many tests were
    run and how many failed.  This is the default output format.

``junit``
    An XML format, designed to emulate the test report emitted by the
    JUnit library and read by many other tools, such as `Jenkins CI
    <http://www.jenkins-ci.org/>`_.  This output format creates a
    directory called ``reports`` containing multiple XML files called
    ``TEST-filename.xml``, one for each test source file name.  Each
    test's pass/fail status, elapsed run time, and any output to stdout
    or stderr are stored in the XML file.

.. vim:set ft=rst:
