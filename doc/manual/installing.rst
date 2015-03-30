
Installing Novaprova
====================

From The Open Build Service
---------------------------

Novaprova is available in installable repositories for many recent Linux
distributions at `the OpenSUSE Build Service <http://download.opensuse.org/repositories/home:/gnb:/novaprova/>`_

For Debian/Ubuntu systems, copy and paste the following commands.

.. highlight:: bash
::

    # Choose a distro
    distro=xUbuntu_12.04
    
    # This is the base URL for the Novaprova repo
    repo=http://download.opensuse.org/repositories/home:/gnb:/novaprova/$distro
    
    # Download and install the repository GPG key
    wget -O - $repo/Release.key | sudo apt-key add -
    
    # Add an APT source
    sudo bash -c "echo 'deb $repo ./' > /etc/apt/sources.list.d/novaprova.list"
    sudo apt-get update
    
    # Install Novaprova
    sudo apt-get install novaprova

See `here <http://en.opensuse.org/openSUSE:Build_Service_Enduser_Info>`_ for RedHat and SUSE instructions.

From A Release Tarball
----------------------

First, download a release tarball.

Novaprova is intended to be built in the usual way that any open source
C software is built.  It has a configure script generated from autoconf,
which you run before building.  To build you need to have g++ and gcc
installed.  You will also need to have the ``valgrind.h`` header file
installed, which is typically in a package named something like
``valgrind-dev``.

.. highlight:: bash
::

    # download the release tarball from http://sourceforge.net/projects/novaprova/files/
    tar -xvf novaprova-1.1.tar.bz2
    cd novaprova-1.1
    ./configure
    make
    make install

From Read-Only Git
------------------

For advanced users only.  Novaprova needs several more tools to build
from a Git checkout than from a release tarball, mainly for the
documentation.  You will need to have `Python Markdown
<http://freewisdom.org/projects/python-markdown/>`_, `Pygments
<http://pygments.org/>`_, and `Doxygen <http://www.doxygen.org/>`_
installed.  On an Ubuntu system these are in packages
``python-markdown``, ``python-pygments``, and ``doxygen`` respectively.

.. highlight:: bash
::

    git clone git://github.com/gnb/novaprova.git
    cd novaprova
    autoreconf -iv
    ./configure
    make
    make install

.. vim:set ft=rst:
