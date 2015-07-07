# Setup C development environment and prereqs
apt-get update
apt-get install -y \
    git autoconf automake libxml2-dev libxml2-utils \
    pkg-config valgrind binutils-dev doxygen \
    python-pygments python-markdown zlib1g-dev \
    libiberty-dev python-pip
pip install breathe Sphinx
