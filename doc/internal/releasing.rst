
Releasing NovaProva
===================

This document describes the NovaProva release procedure.

Healthy Code
------------

Before a release can be made:

- The code must build and all unit tests must pass on all
  supported platforms.
- The documentation must be updated to reflect current behavior.

Change Logs
-----------

Build a human-readable list of changes for the release, starting with
the git version history

.. highlight:: bash

::

    LASTVERSION=$(git tag -l | sed -n -e 's/^novaprova-//p' | tail -1)
    git log --oneline novaprova-$LASTVERSION..HEAD


TODO: update the changelog in ``build/obs/debian.changelog`` and
``build/obs/novaprova.spec``

TODO: check whether package dependencies need updating in
``build/obs/debian.control`` and ``build/obs/novaprova.spec``

Tag The Release
---------------

Choose a new version number using `Semantic Versioning rules <https://semver.org>`_
Set a shell variable.

.. highlight:: bash

::

    VERSION=1.5

Edit ``configure.ac`` to update the version number in the ``AC_INIT()``
macro at the start of the file.

TODO: update the versions in the OBS packaging namely
``build/obs/novaprova.spec``, ``build/obs/novaprova.dsc``


Commit and push to GitHub (the examples
below assume that the git remote ``origin`` points to GitHub).

.. highlight:: bash

::

    vi configure.ac
    ...
    git commit -m "Bump to $VERSION" configure.ac
    git push origin master


Tag the tree and push the tag to GitHub.  Release tags are of the
form ``novaprova-$VERSION``.

.. highlight:: bash

::

    git tag novaprova-$VERSION
    git push --tags origin


TODO: build a release tarball
TODO: upload release tarball to github?
TODO: update readthedocs?


