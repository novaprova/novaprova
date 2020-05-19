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
