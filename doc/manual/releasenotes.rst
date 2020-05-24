..
.. NovaProva Manual
.. Copyright (c) 2015-2020 Gregory Banks
..
.. Licensed under the Apache License, Version 2.0 (the "License");
.. you may not use this file except in compliance with the License.
.. You may obtain a copy of the License at
..
.. http://www.apache.org/licenses/LICENSE-2.0
..
.. Unless required by applicable law or agreed to in writing, software
.. distributed under the License is distributed on an "AS IS" BASIS,
.. WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
.. See the License for the specific language governing permissions and
.. limitations under the License.
..

Release Notes
=============

Release 1.5
-----------

- MacOS x86_64 port.
- All debug logging can now be enabled at runtime with a new --debug
  option on the default main() routine.  Log messages have a new format
  with more information.
- Multiple --format options can be used.
- Add a macro NP_USED which can be used to avoid the issue of modern
  toolchains aggressively eliding apparently unused static functions,
  like your test functions.
- Add dynamic mocking: np_mock() and np_mock_by_name()
- Linux (deb and RPM) packages replaced the "novaprova" package with
  split novaprova-devel and novaprova-doc packages to satisfy rpmlint
  on modern RPM-based distros.
- Support for DWARF versions 3 and 4.
- Support for ASLR (Address Space Layout Randomization) on modern Linux
  toolchains.
- The procedure for building from source has changed, please consult
  the documentation
- Read DWARF line number information and use it generate cleaner and
  more useful stack traces.
- Various bug fixes result in cleaner and more accurate stack traces for
  functions called from mocks or forward declared.
- Fixed bug where the test main loop could hang if a test exited
  uncleanly (Issue #30)
- Fixed bug mocking a function which returns a 64b quantity in the 32b
  ABI (Issue #20)
- Fixed bug where the overlapping ELF PHDR structures emitted by modern
  GNU toolchains could confuse NovaProva and break test discovery and
  mocking.
- Fixed bug which broke skip() for intercepts of a small fraction of
  libc functions.  This bug existed since the first versions in 2011.
- Fixed bug which broke stack alignment for mocked functions on x86_64
  Linux when built with clang.
- Remove the previously hardcoded Valgrind suppressions file.  A new
  can be supplied at configure time, but shouldn't be necessary.
- Detect (at configure time) and handle the incompatible ABI change in
  binutils 2.27
- Numerous minor bug fixes.
- Internal redesign of how memory permissions for intercepts are handled,
  on all platforms, for future proofing on Linux and to support Catalina
  on MacOS.
- Internal redesign of how reference_t works, making some progress
  towards removing the state_t singleton.
- Internally switched to C++11.  You now need a C++11 compiler if
  you're building NovaProva from source.  The requirements for
  using NovaProva as a library do NOT change.
- First attempts to use Travis for CI.

Release 1.4
-----------

The 1.4 release partially exists in a confusing way which is hard to
discover and reconcile given the large time gap since the 1.3 release.
To ensure a clean transition we will skip 1.4 entirely.

Release 1.3
-----------

- Add a default main() routine.
- Check for leaked file descriptors
- Add an autoconf configure script
- JUnit output captures stdout & stderr
- Silently ignore out-of-range DWARF attributes
- Silently ignore pipes to/from Valgrind's gdbserver
- Avoid Valgrind warning in intercept_tramp() on 64b
- Fix bug which broke test listing
- Added example 13_twoparams
- Various minor fixes

Release 1.2
-----------

- Support for building with SUSE OBS
- Added example for LCA2013 talk
- Bug fixes exposed by examples

Release 1.1
-----------

- Linux x86_64 port.
- All the meta-tests pass now.
- Minor cleanups.

Release 1.0
-----------

- Initial public release

.. vim:set ft=rst:
