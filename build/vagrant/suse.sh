

if [ -f /etc/os-release ] ; then
    VERSION=$(source /etc/os-release && echo "$VERSION_ID")
elif [ -f /etc/SuSE-release ] ; then
    VERSION=$(sed -n '/^VERSION/s/.*= *//p' /etc/SuSE-release)
else
    echo "Unable to determine SUSE version" 1>&2
    exit 1
fi

# Already installed: git binutils-devel
zypper install -y \
    valgrind-devel libxml2-devel doxygen \
    perl-XML-LibXML glibc-devel-static

case "$VERSION" in
13*)
    zypper install -y python3-pip
    pip install Sphinx breathe
    ;;
15*)
    zypper install -y python3-Sphinx python3-breathe
    ;;
esac
