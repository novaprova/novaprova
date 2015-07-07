# vi: set ft=ruby :

# Vagrantfile API/syntax version. Don't touch unless you know what you're doing!
VAGRANTFILE_API_VERSION = "2"

# Throw an error if required Vagrant plugins are not installed
unless Vagrant.has_plugin? 'vagrant-bindfs'
  raise error "The 'vagrant-bindfs' plugin is not installed!"
end

platforms_dir = 'build/vagrant/platforms'
platforms = {}

# Read all the .yaml files in the platforms directory;
# each one defines a VM
Dir["#{platforms_dir}/*.yaml"].each do |filename|
    name = File.basename(filename, ".yaml")
    opts = YAML.load_file(filename)
    # The yaml file can have either an inline 'script'
    # entry or a 'script_file' entry which is a filename
    # relative to the platforms dir
    if opts['script_file']
	s = ''
	File.open "#{platforms_dir}/#{opts['script_file']}", 'r' do |f|
	    while (line = f.gets)
		s += line
	    end
	end
	opts['script'] = s
    end
    platforms[name] = opts
end

# Main Vagrant Init
Vagrant.configure(VAGRANTFILE_API_VERSION) do |global_config|

    if Vagrant.has_plugin? 'vagrant-cachier'
	global_config.cache.scope = :box
    end

    platforms.each_pair do |name, options|

	global_config.vm.define name.intern do |config|

	    config.vm.provider "virtualbox" do |v|
		v.name = "NovaProva - #{name}"
	    end

	    config.vm.box = options['box']

	    # Remove the default Vagrant directory sync
	    config.vm.synced_folder ".", "/vagrant", disabled: true

	    config.vm.synced_folder ".", "/build/novaprova-nfs", type: "nfs", nfs_udp: false
	    config.bindfs.bind_folder "/build/novaprova-nfs", "/build/novaprova", owner: "vagrant", group: "vagrant"

	    config.vm.hostname = name
	    config.vm.network "private_network", ip: "172.17.2.101"

	    config.vm.provision :shell, :inline => options['script']
	end
    end
end
