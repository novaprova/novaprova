# -*- mode: ruby -*-
# vi: set ft=ruby :

# Vagrantfile API/syntax version. Don't touch unless you know what you're doing!
VAGRANTFILE_API_VERSION = "2"

# Throw an error if required Vagrant plugins are not installed
unless Vagrant.has_plugin? 'vagrant-bindfs'
  raise error "The 'vagrant-bindfs' plugin is not installed!"
end

downloaddir = ENV["HOME"] + "/Downloads"

# Basic setup for the host
$script_basic = <<SCRIPT
domainname localdomain
SCRIPT

# Setup C development environment and prereqs
$script_prereqs = <<SCRIPT

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
    doxygen python-pygments python-markdown \
    perl-XML-LibXML strace python-pip
pip install breathe Sphinx
SCRIPT

# Main Vagrant Init
Vagrant.configure(VAGRANTFILE_API_VERSION) do |config|

  # The dev machine is based off a CentOS box
  config.vm.box = "centos"
#  config.vm.box_url = "http://developer.nrel.gov/downloads/vagrant-boxes/CentOS-6.4-x86_64-v20130731.box"
  config.vm.box_url = "file://#{downloaddir}/CentOS-6.4-x86_64-v20130731.box"

  # Create a forwarded port mapping which allows access to the
  # pmwebd port from outside the guest.
  config.vm.network :forwarded_port, guest: 44323, host: 44323

  # Remove the default Vagrant directory sync
  config.vm.synced_folder ".", "/vagrant", disabled: true

  config.vm.synced_folder "../novaprova", "/build/novaprova-nfs", type: "nfs", nfs_udp: false
  config.bindfs.bind_folder "/build/novaprova-nfs", "/build/novaprova", owner: "vagrant", group: "vagrant"

  config.vm.define "centos64" do |pcpdev|
    pcpdev.vm.hostname = "centos64"
    pcpdev.vm.network "private_network", ip: "172.17.2.101"
  end

  config.vm.provision :shell, :inline => $script_basic
  config.vm.provision :shell, :inline => $script_prereqs

end
