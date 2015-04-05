
Introduction
============

What is NovaProva?
------------------

NovaProva is a new generation unit test framework for C and C++ programs.

NovaProva starts with the well-known xUnit paradigm for testing, but
extends some concepts in new directions.  For example, NovaProva
organises tests into trees rather than xUnit's two-level "suite and
test" approach.

Most importantly, NovaProva is designed using real world experience and
modern technologies to make it much easier to write tests, and much
easier to discover subtle bugs in the System Under Test, bugs that unit
other test frameworks miss.  NovaProva's mission is to make testing C
and C++ code as easy as possible.


Why Create NovaProva?
---------------------

At the time NovaProva was first written, the author had a day job
working on a large C (K&R in some places) `codebase
<http://git.cyrusimap.org/>`_  which was in parts decades old.
This code had *zero* buildable tests. The author created hundreds of
tests, both system tests using a new `application-specific Perl
framework <http://git.cyrusimap.org/cassandane/>`_ and C regression
tests using the venerable `CUnit library
<http://cunit.sourceforge.net>`_.

This experience showed that writing more than a handful of tests using
CUnit is very hard, and gets harder the more tests are written and the
more insightful the tests.  NovaProva is designed to make the process of
writing and running C and C++ unit tests as easy as possible.

Design Philosophy
-----------------

NovaProva's design philosophy is based on the following principles.

- Fully support C as well as C++.  This means that NovaProva does not
  rely on C++ compile time features for test discovery or reflection.
  Test code must be buildable with only a C compiler, without C++.

- Choose power over portability.  Portability is important, but for tests
  it's better to have good tests on one platform than bad tests on all
  platforms.  Bad tests are a waste of everyone's time.

- Simplify the tedious parts of testing.  Do as much as possible
  in the framework so that users don't need to.

- Choose correctness and test power over test run speed.  Slow tests are
  annoying but remotely debugging buggy code which shipped is much
  more annoying and expensive.

- Choose best practice as the default.  For example, maximise test
  isolation, and maximise detection of failure modes.

- Have the courage to do things that are “impossible” in C & C++,
  such as dynamic mocking.

.. vim:set ft=rst:
