# Setup C development environment and prereqs
domainname localdomain

# Setup EPEL
repo=/etc/yum.repos.d/epel.repo
if [ ! -f $repo ] ; then

    cat <<EOM >/etc/yum.repos.d/epel-bootstrap.repo
[epel]
name=Bootstrap EPEL
mirrorlist=http://mirrors.fedoraproject.org/mirrorlist?repo=epel-\\\$releasever&arch=\\\$basearch
failovermethod=priority
enabled=0
gpgcheck=0
EOM

    yum --enablerepo=epel -y install epel-release
    rm -f /etc/yum.repos.d/epel-bootstrap.repo

    # Horrible hack no.1 to work around a broken mirror
    (
	grep -v mirrors.fedoraproject.org /etc/hosts
	echo '66.135.62.187	mirrors.fedoraproject.org'
    ) >> /etc/hosts.NEW && mv -f /etc/hosts.NEW /etc/hosts
    # Horrible hack no.2 to work around a broken mirror
    sed -e 's|https://|http://|g' < $repo > $repo.NEW && mv -f $repo.NEW $repo
fi

yum -y groupinstall 'Development Tools'
yum -y install \
    git valgrind-devel binutils-devel libxml2-devel \
    doxygen perl-XML-LibXML strace python-pip
pip install breathe Sphinx
